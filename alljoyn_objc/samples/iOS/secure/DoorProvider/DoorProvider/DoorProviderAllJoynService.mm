////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "DoorProviderAllJoynService.h"
#import "AJNAboutData.h"
#import "AJNAboutObject.h"
#import "AJNPermissionConfigurator.h"
#import "AJNPermissionConfigurationListener.h"
#import "DefaultECDHEListener.h"
#import "AJNBusAttachment.h"
#import "DoorCommonPCL.h"
#import "AJNGUID.h"
#import "SecureDoor.h"
#import "AJNInit.h"

#include <string>

static NSString *const kKeyxEcdheNull = @"ALLJOYN_ECDHE_NULL";
static NSString *const kKeyxEcdhePsk = @"ALLJOYN_ECDHE_PSK";
static NSString *const kKeyxEcdheSpeke = @"ALLJOYN_ECDHE_SPEKE";
static NSString *const kKeyxEcdheDsa = @"ALLJOYN_ECDHE_ECDSA";

static NSString *const kDoorObjectPath = @"/sample/security/Door";
static AJNSessionPort const kPort = 12345;

@interface DoorProviderAllJoynService () <AJNSessionPortListener, DoorServiceToogleToClaimedModeListener>

@property (nonatomic, strong) NSCondition* waitCondition;

@property (nonatomic, strong) AJNAboutData *aboutData;
@property (nonatomic, strong) AJNAboutObject *aboutObj;

@property (nonatomic, strong) Door *door;

@property (nonatomic, strong) AJNBusAttachment* busAttachment;

@property (nonatomic, strong) DoorCommonPCL* doorCommonPCL;
@property (nonatomic) DefaultECDHEListener *authListener;

@property (nonatomic, weak) id<AllJoynStatusMessageListener> messageListener;

@property (nonatomic) BOOL isClaimed;

@property (nonatomic, strong) NSString *password;

@end

@implementation DoorProviderAllJoynService

- (id)init
{
    return [self initWithMessageListener:nil];
}

- (id)initWithMessageListener:(id<AllJoynStatusMessageListener>)messageListener
{
    self = [super init];
    if (self != nil) {

        if ([AJNInit alljoynInit] != ER_OK) {
            return nil;
        }
        if ([AJNInit alljoynRouterInit] != ER_OK) {
            [AJNInit alljoynShutdown];
            return nil;
        }

        _waitCondition = [[NSCondition alloc] init];
        _serviceState = STOPED;

        _isClaimed = NO;

        _messageListener = messageListener;
    }
    return self;
}

- (QStatus)startWithName:(NSString *)appName;
{
    if (_serviceState != STOPED) {
        return ER_FAIL;
    }

    if (appName == nil || appName.length == 0) {
        return ER_FAIL;
    }
    
    QStatus status = ER_OK;

    _appName = appName;

    _busAttachment = [[AJNBusAttachment alloc] initWithApplicationName:appName allowRemoteMessages:YES];

    _aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
    _aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:_busAttachment withAnnounceFlag:ANNOUNCED];

    _authListener = [[DefaultECDHEListener alloc] init];

    _doorCommonPCL = [[DoorCommonPCL alloc] initWithBus:_busAttachment service:self];

    status = [_busAttachment start];
    if (status != ER_OK) {
        NSLog(@"Failed to start bus attachment - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [_busAttachment connectWithArguments:@"nil"];
    if (status != ER_OK) {
        NSLog(@"Failed to connect bus attachment - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [self enablePeerSecure];
    if (status != ER_OK) {
        NSLog(@"Failed to enablePeerSecurity - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    status = [self setupPermissionConfigurator];
    if (status != ER_OK) {
        NSLog(@"Failed to setup permission configurator - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    status = [self displayClaimingInformation];
    if (status != ER_OK) {
        NSLog(@"Failed to display claiming status - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    //host Session
    AJNSessionOptions* opts = [[AJNSessionOptions alloc] init];
    status = [_busAttachment bindSessionOnPort:kPort withOptions:opts withDelegate:self];
    if (ER_OK != status) {
        NSLog(@"Failed to initialize DoorCommon - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.DoorProvider.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run];
    });

    _serviceState = STARTED;
    return ER_OK;
}

- (QStatus)enablePeerSecure
{
    QStatus status = ER_OK;

    NSString* securitySuites;

    status = [self setupSecureRules:&securitySuites];
    if (ER_OK != status) {
        NSLog(@"Failed to setup secure rules - status (%@)", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    //check is file exist
    BOOL isKeyStoreExist = NO;

    NSString *keystoreFilePathShort = @"Documents/s_central.ks";
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *keystoreFilePathFull = [NSHomeDirectory() stringByAppendingPathComponent:keystoreFilePathShort];

    if ([fileManager fileExistsAtPath:keystoreFilePathFull]) {
        isKeyStoreExist = YES;
    }

    //try to enable peer secure 1st time
    status = [_busAttachment enablePeerSecurity:securitySuites authenticationListener:_authListener keystoreFileName:keystoreFilePathShort sharing:YES permissionConfigurationListener:_doorCommonPCL];
    if (status == ER_OK) {
        if (isKeyStoreExist) {
            NSLog(@"Application enable peer secure with existed key store.");
        } else {
            NSLog(@"Application enable peer secure with new key store.");
        }
    } else if (status == ER_AUTH_FAIL && isKeyStoreExist) {
        NSLog(@"Application failed to enable peer secure with existed key store.");
        NSLog(@"Try to delete keystore and enable peer secure again.");
        //delete keystore
        NSError *error;
        BOOL isFileDeleted = NO;
        isFileDeleted = [fileManager createFileAtPath:keystoreFilePathFull contents:[NSData data] attributes:nil];

        if (isFileDeleted) {
            //try to enable peer secure 2nd time
            status = [_busAttachment enablePeerSecurity:securitySuites authenticationListener:_authListener keystoreFileName:keystoreFilePathShort sharing:YES permissionConfigurationListener:_doorCommonPCL];
            if (status == ER_OK) {
                NSLog(@"Application delete old key store and enable peer secure with new one.");
            }
        } else {
            return ER_FAIL;
        }
    }

    return status;
}

- (QStatus)setupSecureRules:(NSString **)securitySuites
{
    QStatus status = ER_OK;

    *securitySuites = [NSString stringWithFormat:@"%@ %@ %@", kKeyxEcdheDsa, kKeyxEcdheNull, kKeyxEcdheSpeke];

    status = [self setRandomPasswordWithLengh:@6 toAuthListener:_authListener password:&_password];
    if (status != ER_OK) {
        NSLog(@"Failed to set password to auth listener - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    return status;
}

- (QStatus)setupPermissionConfigurator
{
    QStatus status = ER_OK;

    NSLog(@"Allow doors to be claimable using a PSK.\n");
    AJNClaimCapabilities capabilities = (CAPABLE_ECDHE_SPEKE | CAPABLE_ECDHE_NULL);
    status = [_busAttachment.permissionConfigurator setClaimCapabilities:capabilities];
    if (status != ER_OK) {
        NSLog(@"Failed to setClaimCapabilities - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [_busAttachment.permissionConfigurator setClaimCapabilityAdditionalInfo:PSK_GENERATED_BY_APPLICATION];
    if (status != ER_OK) {
        NSLog(@"Failed to setClaimCapabilityAdditionalInfo - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    NSString* manifestXml =
//    @"<manifest>"
//    "<node>"
//    "<interface = name=\"sample.securitymgr.door.Door\">"
//    "<method>"
//    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
//    "</method>"
//    "<property>"
//    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
//    "</property>"
//    "</interface>"
//    "</node>"
//    "</manifest>";
    @"<manifest>"
    "<node>"
    "<interface name = \"sample.securitymgr.door.Door\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

    status = [_busAttachment.permissionConfigurator setManifestTemplateFromXml:manifestXml];
    if (ER_OK != status) {
        NSLog(@"Failed to setPermissionManifestTemplate - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    return status;
}

- (QStatus)setRandomPasswordWithLengh:(NSNumber *)length toAuthListener:(DefaultECDHEListener *)authListener password:(NSString * __strong *)password
{
    AJNGUID128 *randomGuid = [[AJNGUID128 alloc] init];
    const uint8_t *randomData = randomGuid.bytes;

    size_t passwordLength = (length < [[NSNumber alloc] initWithLong:[AJNGUID128 SIZE]]) ? [length intValue] : [AJNGUID128 SIZE];
    std::string randomPassword;
    randomPassword.resize(passwordLength);

    for (size_t i = 0; i < passwordLength; ++i) {
        uint8_t value = (randomData[i] % 16);
        if (value < 10) {
            randomPassword[i] = ('0' + value);
        } else {
            randomPassword[i] = ('A' + value - 10);
        }
    }

    *password = [NSString stringWithCString:randomPassword.c_str() encoding:[NSString defaultCStringEncoding]];

    QStatus status = [authListener setPassword:(uint8_t*)randomPassword.data() passwordSize:passwordLength];

    return status;
}

- (QStatus)displayClaimingInformation
{
    AJNApplicationState state;
    QStatus status = ER_OK;
    status = [_busAttachment.permissionConfigurator getApplicationState:&state];
    if (ER_OK != status) {
        NSLog(@"Failed to getApplicationState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    if (state == CLAIMABLE) {
        NSLog(@"Door provider is not claimed.\n");
        NSLog(@"The provider can be claimed using SPEKE with an application generated secret.\n");
        NSLog(@"Password = (%@)\n", [_password description]);
        [_messageListener didReceiveAllJoynStatusMessage:[NSString stringWithFormat:@"Password = (%@)\n", [_password description]]];
    } else if (state == CLAIMED) {
        NSLog(@"Door provider is already claimed.\n");
        [_messageListener didReceiveAllJoynStatusMessage:[NSString stringWithFormat:@"Door provider is already claimed.\n"]];
    } else {
        NSLog(@"Unexpected application state. Expected CLAIMABLE or CLAIMED, current state = (%u)\n", state);
        return ER_FAIL;
    }

    return ER_OK;
}

- (QStatus)stop
{
    [self.waitCondition signal];

    _serviceState = STOPED;

    return ER_OK;
}

- (void)sendDoorOpenEvent
{
    QStatus status = [_door sendstate:YES inSession:kAJNSessionIdAllHosted toDestination:NULL];
    NSLog(@"sendDoorOpenEvent status: %@", status);

}

- (QStatus)toggleAutoSignal
{
    NSString* manifestXml =
    @"<manifest>"
    "<node>"
    "<interface name = \"sample.securitymgr.door.Door\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

    QStatus status = [_busAttachment.permissionConfigurator setManifestTemplateFromXml:manifestXml];
    if (ER_OK != status) {
        NSLog(@"Failed to SetPermissionManifestTemplate - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [_busAttachment.permissionConfigurator setApplicationState:NEED_UPDATE];
    if (ER_OK != status) {
        NSLog(@"Failed to SetApplicationState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    return status;
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    NSLog(@"Accepting session from %@ on session port %i", joiner, sessionPort);
    return YES;
}

- (QStatus)run
{
    QStatus status = ER_FAIL;

    _door = [[Door alloc] initWithBusAttachment:_busAttachment onPath:kDoorObjectPath andMessageListener:_messageListener];

    status = [_busAttachment registerBusObject:_door enableSecurity:YES];
    if (ER_OK != status) {
        NSLog(@"Failed to RegisterBusObject - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    [self setAboutData];

    if (![_aboutData isValid]) {
        NSLog(@"Invalid aboutData\n");
        return ER_FAIL;
    }

    status = [_aboutObj announceForSessionPort:kPort withAboutDataListener:_aboutData];
    if (ER_OK != status) {
        NSLog(@"Failed to AnnounceAbout - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    if (!_isClaimed) {
        //Wait until we are claimed
        [_messageListener didReceiveAllJoynStatusMessage:@"Waiting to be claimed...\n"];
        status = [_doorCommonPCL waitForClaimedState];
        if (ER_OK != status) {
            NSLog(@"Failed to WaitForClaimedState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return status;
        }
    }

    [_messageListener didReceiveAllJoynStatusMessage:@"Door provider claimed; Waiting for consumers ...\n"];

    [self.waitCondition lock];
    [self.waitCondition wait];
    [self.waitCondition unlock];

    _aboutObj = nil;
    _busAttachment = nil;
    _door = nil;
    _doorCommonPCL = nil;

    return ER_OK;
}

- (void)setAboutData
{
    AJNGUID128* appId = [[AJNGUID128 alloc] init];
    [_aboutData setAppId:(uint8_t*)appId.bytes];

    char buf[64];
    gethostname(buf, sizeof(buf));
    [_aboutData setDeviceName:[NSString stringWithCString:buf encoding:NSUTF8StringEncoding] andLanguage:@"en"];

    AJNGUID128* deviceId = [[AJNGUID128 alloc] init];
    [_aboutData setDeviceId:[deviceId description]];
    [_aboutData setAppName:_appName andLanguage:@"en"];
    [_aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];
    [_aboutData setModelNumber:@"1"];
    [_aboutData setDescription:_appName andLanguage:@"en"];
    [_aboutData setDateOfManufacture:@"2015-04-14"];
    [_aboutData setSoftwareVersion:@"0.1"];
    [_aboutData setHardwareVersion:@"0.0.1"];
    [_aboutData setSupportUrl:@"https://allseenalliance.org/"];
}

- (QStatus)setSecurityForClaimedMode
{
    QStatus status = [_busAttachment enablePeerSecurity:@"" authenticationListener:nil keystoreFileName:nil sharing:true];
    if (ER_OK != status) {
        NSLog(@"SetSecurityForClaimedMode: Could not clear peer security - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [_busAttachment enablePeerSecurity:kKeyxEcdheDsa authenticationListener:_authListener keystoreFileName:@"" sharing:YES permissionConfigurationListener:_doorCommonPCL];
    if (ER_OK != status) {
        NSLog(@"Failed to SetSecurityForClaimedMode - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }
    
    return ER_OK;
}

- (void)dealloc
{
    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];
}

@end
