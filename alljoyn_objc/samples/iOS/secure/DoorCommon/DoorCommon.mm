/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#import <Foundation/Foundation.h>
#import <alljoyn/AuthListener.h>
#import <string.h>
#import "AJNAuthenticationListener.h"
#import <alljoyn/BusAttachment.h>
#import "AJNBusObjectImpl.h"
#import "AJNAboutData.h"
#import "AJNAboutObject.h"
#import "AJNInterfaceDescription.h"
#import "AJNPermissionConfigurationListener.h"
#import "AJNSessionPortListener.h"
#import "AJNPermissionConfigurator.h"
#import "AJNGUID.h"
#import "AJNSessionOptions.h"
#import "DoorCommon.h"

@interface DoorCommonPCL() <AJNPermissionConfigurationListener>

@property (nonatomic, strong) AJNBusAttachment* bus;
@property (nonatomic, strong) NSCondition* condition;

@end

/* Door about listener */

@interface DoorAboutListener()

@end

@implementation DoorAboutListener

- (id)init
{
    self = [super init];
    if (self != nil) {
        self.doors = [[NSMutableSet alloc] init];
    }
    return self;
}

- (void)didReceiveAnnounceOnBus:(NSString *)busName withVersion:(uint16_t)version withSessionPort:(AJNSessionPort)port withObjectDescription:(AJNMessageArgument *)objectDescriptionArg withAboutDataArg:(AJNMessageArgument *)aboutDataArg
{
    AJNAboutData* about = [[AJNAboutData alloc] initWithMsgArg:aboutDataArg andLanguage:@"en"];
    NSString* appName = nil;
    QStatus status = [about getAppName:&appName andLanguage:@"en"];
    if (ER_OK != status) {
        NSLog(@"Failed to GetAppName - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return;
    }

    NSString* deviceName = nil;
    status = [about getDeviceName:&deviceName andLanguage:@"en"];
    if (ER_OK != status) {
        NSLog(@"Failed to GetDeviceName - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return;
    }

    [self.doors addObject:[NSString stringWithString:busName]];
}

- (void)removeDoorName:(NSString*)doorName
{
    [self.doors removeObject:doorName];
}

@end

@implementation DoorCommonPCL

- (id)initWithBus:(AJNBusAttachment*)bus
{
    self = [super init];
    if (self != nil) {
        self.bus = bus;
        self.condition = [[NSCondition alloc] init];
    }

    return self;
}

- (void)startManagement
{
    NSLog(@"StartManagement called.\n");
}

- (void)endManagement
{
    NSLog(@"EndManagement called.\n");
    [self.condition lock];
    AJNApplicationState appState;
    QStatus status = [self.bus.permissionConfigurator getApplicationState:&appState];
    if (status == ER_OK) {
        if (appState == CLAIMED) {
            [self.condition signal];
        } else {
            NSLog(@"App not claimed after management finished. Continuing to wait.\n");
        }
    } else {
        NSLog(@"Failed to GetApplicationState - status (%s)\n", QCC_StatusText(status));
    }
    [self.condition unlock];
}

- (QStatus)waitForClaimedState
{
    [self.condition lock];
    AJNApplicationState appState;
    QStatus status = [self.bus.permissionConfigurator getApplicationState:&appState];
    if (ER_OK != status) {
        NSLog(@"Failed to GetApplicationState - status (%s)\n",
                QCC_StatusText(status));
        [self.condition unlock];
        return status;
    }

    if (appState == CLAIMED) {
        NSLog(@"Already claimed !\n");
        [self.condition unlock];
        return ER_OK;
    }

    NSLog(@"Waiting to be claimed...\n");
    [self.condition wait];
    if (ER_OK != status) {
        [self.condition unlock];
        return status;
    }

    NSLog(@"Claimed !\n");
    [self.condition unlock];
    return ER_OK;
}

@end

@interface SPListener() <AJNSessionPortListener>

@end

@implementation SPListener

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    return YES;
}

@end

@interface Door()

@property (nonatomic) BOOL IsOpen;
@property (nonatomic, strong) AJNBusAttachment* bus;

@end

class DoorImpl : public AJNBusObjectImpl
{
    private:

    const InterfaceDescription::Member* stateSignal;

    public:
        DoorImpl(BusAttachment &bus, const char *path, id<AJNDoorDelegate> aDelegate);

        virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

        QStatus SendDoorEvent();

        void Open(const InterfaceDescription::Member* member, Message& msg);
        void Close(const InterfaceDescription::Member* member, Message& msg);
        void GetState(const InterfaceDescription::Member* member, Message& msg);

        void ReplyWithBoolean(bool answer, Message& msg);

        bool autoSignal;

};

DoorImpl::DoorImpl(BusAttachment &bus, const char *path, id<AJNDoorDelegate> aDelegate) :
    AJNBusObjectImpl(bus, path, aDelegate)
{
    const InterfaceDescription* interfaceDescription = NULL;
    QStatus status = ER_OK;
    autoSignal = false;

    interfaceDescription = bus.GetInterface([DOOR_INTERFACE UTF8String]);
    assert(interfaceDescription);
    AddInterface(*interfaceDescription, ANNOUNCED);


    // Register the method handlers for interface DoorObjectDelegate with the object
    //
    const MethodEntry methodEntriesForDoorObjectDelegate[] = {

        {
            interfaceDescription->GetMember([DOOR_OPEN UTF8String]), static_cast<MessageReceiver::MethodHandler>(&DoorImpl::Open)
        },

        {
            interfaceDescription->GetMember([DOOR_CLOSE UTF8String]), static_cast<MessageReceiver::MethodHandler>(&DoorImpl::Close)
        },
        {
            interfaceDescription->GetMember([DOOR_GET_STATE UTF8String]), static_cast<MessageReceiver::MethodHandler>(&DoorImpl::GetState)
        }


    };

    status = AddMethodHandlers(methodEntriesForDoorObjectDelegate, sizeof(methodEntriesForDoorObjectDelegate) / sizeof(methodEntriesForDoorObjectDelegate[0]));
    if (ER_OK != status) {
        NSLog(@"ERROR: An error occurred while adding method handlers for interface" DOOR_INTERFACE " to the interface description. %@", [AJNStatus descriptionForStatusCode:status]);
    }

    stateSignal = interfaceDescription->GetMember([DOOR_STATE_CHANGED UTF8String]);
}

QStatus DoorImpl::SendDoorEvent()
{
    printf("Sending door event ...\n");
    MsgArg outArg;
    outArg.Set("b", open);

    QStatus status = Signal(nullptr, SESSION_ID_ALL_HOSTED, *stateSignal, &outArg, 1, 0, 0,  nullptr);
    if (status != ER_OK) {
        fprintf(stderr, "Failed to send Signal - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}


void DoorImpl::Open(const InterfaceDescription::Member* member, Message& msg)
{
    @autoreleasepool {

        // call the Objective-C delegate method
        //

        [(id<AJNDoorDelegate>)delegate open:[[AJNMessage alloc] initWithHandle:&msg]];
        if (autoSignal) {
            SendDoorEvent();
        }
        ReplyWithBoolean(true, msg);
    }

}

void DoorImpl::Close(const InterfaceDescription::Member* member,
                 Message& msg)
{
    @autoreleasepool {
        [(id<AJNDoorDelegate>)delegate close:[[AJNMessage alloc] initWithHandle:&msg]];
        if (autoSignal) {
            SendDoorEvent();
        }
        ReplyWithBoolean(true, msg);
    }

}

void DoorImpl::ReplyWithBoolean(bool answer, Message& msg)
{
    MsgArg outArg;
    outArg.Set("b", answer);

    QStatus status = MethodReply(msg, &outArg, 1);
    if (status != ER_OK) {
        fprintf(stderr, "Failed to send MethodReply - status (%s)\n", QCC_StatusText(status));
    }
}


QStatus DoorImpl::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    NSString* nsIfcName = [NSString stringWithCString:ifcName encoding:NSUTF8StringEncoding];
    NSString* nsPropName = [NSString stringWithCString:propName encoding:NSUTF8StringEncoding];
    if (([nsIfcName isEqualToString:DOOR_INTERFACE]) && ([nsPropName isEqualToString:DOOR_STATE])) {
        val.Set("b", ((id<AJNDoorDelegate>)delegate).IsOpen);
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

void DoorImpl::GetState(const InterfaceDescription::Member* member,
                    Message& msg)
{
    QCC_UNUSED(member);

    NSLog(@"Door GetState method was called\n");
    ReplyWithBoolean(((id<AJNDoorDelegate>)delegate).IsOpen, msg);
}

@interface Door()

@property (nonatomic, weak) ViewController* view;

@end


@implementation Door

- (DoorImpl*)busObject
{
    return static_cast<DoorImpl*>(self.handle);
}

- (void)setAutoSignal:(BOOL)autoSignal
{
    _autoSignal = autoSignal;
    self.busObject->autoSignal = autoSignal ? true : false;
}

- (id)initWithBus:(AJNBusAttachment*)bus
{
    self = [super init];
    if (self != nil) {
        self.bus = bus;
        self.handle = new DoorImpl(*((ajn::BusAttachment*)bus.handle), [DOOR_OBJECT_PATH UTF8String], (id<AJNDoorDelegate>)self);
    }

    return self;
}

- (id)initWithBus:(AJNBusAttachment *)bus andView:(ViewController *)view
{
    self = [super init];
    if (self != nil) {
        self.bus = bus;
        self.handle = new DoorImpl(*((ajn::BusAttachment*)bus.handle), [DOOR_OBJECT_PATH UTF8String], (id<AJNDoorDelegate>)self);
        self.view = view;
    }

    return self;
}

- (void)dealloc
{
    DoorImpl *busObject = [self busObject];
    delete busObject;
    self.handle = nil;
}


- (QStatus)initialize
{
    QStatus status = ER_OK;
    AJNInterfaceDescription* secPermIntf = [self.bus interfaceWithName:DOOR_INTERFACE];
    if (!secPermIntf) {
        NSLog(@"Failed to get interface");
        return status;
    }

    return status;
}

- (void)open:(AJNMessage*)methodCallMessage
{
    [self.view didReceiveStatusUpdateMessage:@"Door open method was called\n"];
    self.IsOpen = YES;
}

- (void)close:(AJNMessage*)methodCallMessage
{
    [self.view didReceiveStatusUpdateMessage:@"Door close method was called\n"];
    self.IsOpen = NO;
}

- (QStatus)sendDoorEvent
{
    return [self busObject]->SendDoorEvent();
}


@end

@interface DefaultECDHEListenerDelegate : NSObject<AJNAuthenticationListener>

@property (nonatomic) DefaultECDHEAuthListener* handle;

@end

@implementation DefaultECDHEListenerDelegate : NSObject

- (id)init
{
    self = [super init];
    if (self != nil) {
        self.handle = new DefaultECDHEAuthListener();
    }
    return self;
}

- (QStatus)setPSK:(const uint8_t *)secret secretSize:(size_t)secretSize
{
    return self.handle->SetPSK(secret, secretSize);
}

- (QStatus)setPassword:(const uint8_t*)password passwordSize:(size_t)passwordSize
{
    return self.handle->SetPassword(password, passwordSize);
}

- (AJNSecurityCredentials *)requestSecurityCredentialsWithAuthenticationMechanism:(NSString *)authenticationMechanism peerName:(NSString *)peerName authenticationCount:(uint16_t)authenticationCount userName:(NSString *)userName credentialTypeMask:(AJNSecurityCredentialType)mask;

{
    DefaultECDHEAuthListener::Credentials* creds = new DefaultECDHEAuthListener::Credentials();
    if (self.handle->RequestCredentials([authenticationMechanism UTF8String], [peerName UTF8String], authenticationCount, [userName UTF8String], mask, *creds)) {
        return [[AJNSecurityCredentials alloc] initWithHandle:creds];
    }

    delete creds;

    return nil;
}

- (void)authenticationUsing:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peerName didCompleteWithStatus:(BOOL)success
{
    return self.handle->AuthenticationComplete([authenticationMechanism UTF8String], [peerName UTF8String], success);
}

@end

@interface DoorCommon()

@property (nonatomic, strong) AJNAboutData* aboutData;

@property (nonatomic, strong) AJNAboutObject* aboutObj;

@property (nonatomic, strong) SPListener* sessionPortListener;

@property (nonatomic, strong) id<AJNPermissionConfigurationListener> permissionConfigListener;

@property (nonatomic) DefaultECDHEListenerDelegate * authListener;

@property (nonatomic, weak) ViewController* view;

@end


@implementation DoorCommon

- (id)initWithAppName:(NSString *)appName
{
    self = [super init];
    if (self != nil) {
        self.BusAttachment = [[AJNBusAttachment alloc] initWithApplicationName:appName allowRemoteMessages:YES];
        self.aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
        self.aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.BusAttachment withAnnounceFlag:ANNOUNCED];
        self.authListener = [[DefaultECDHEListenerDelegate alloc] init];
        self.sessionPortListener = [[SPListener alloc] init];
        self.AppName = appName;
    }
    return self;
}

- (id)initWithAppName:(NSString *)appName andView:(ViewController *)view
{
    self = [super init];
    if (self != nil) {
        self.BusAttachment = [[AJNBusAttachment alloc] initWithApplicationName:appName allowRemoteMessages:YES];
        self.aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
        self.aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.BusAttachment withAnnounceFlag:ANNOUNCED];
        self.authListener = [[DefaultECDHEListenerDelegate alloc] init];
        self.sessionPortListener = [[SPListener alloc] init];
        self.AppName = appName;
        self.view = view;
    }
    return self;
}

- (AJNInterfaceMember*)getDoorSignal
{
    return [[self.BusAttachment interfaceWithName:DOOR_INTERFACE] memberWithName:DOOR_STATE_CHANGED];
}

- (void)createInterface
{
    // Create a secure door interface on the bus attachment
    AJNInterfaceDescription* doorIntf = [self.BusAttachment createInterfaceWithName:DOOR_INTERFACE withInterfaceSecPolicy:AJN_IFC_SECURITY_REQUIRED];
    if (doorIntf != nil) {
        NSLog(@"Secure door interface was created.\n");
        NSLog(@"Secure door interface was created.\n");
        [doorIntf addMethodWithName:DOOR_OPEN inputSignature:@"" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"succes", nil]];
        [doorIntf addMethodWithName:DOOR_CLOSE inputSignature:@"" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"succes", nil]];
        [doorIntf addMethodWithName:DOOR_GET_STATE inputSignature:@"" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"state", nil]];
        [doorIntf addSignalWithName:DOOR_STATE_CHANGED inputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"state", nil]];
        [doorIntf addPropertyWithName:DOOR_STATE signature:@"b" accessPermissions:kAJNInterfacePropertyAccessReadWriteFlag];
        [doorIntf activate];
    } else {
        NSLog(@"Failed to create Secure PermissionMgmt interface.\n");
    }
}

- (void)setAboutData
{
    AJNGUID128* appId = [[AJNGUID128 alloc] init];
    [self.aboutData setAppId:(uint8_t*)appId.bytes];

    char buf[64];
    gethostname(buf, sizeof(buf));
    [self.aboutData setDeviceName:[NSString stringWithCString:buf encoding:NSUTF8StringEncoding] andLanguage:@"en"];

    AJNGUID128* deviceId = [[AJNGUID128 alloc] init];
    [self.aboutData setDeviceId:[deviceId description]];
    [self.aboutData setAppName:self.AppName andLanguage:@"en"];
    [self.aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];
    [self.aboutData setModelNumber:@"1"];
    [self.aboutData setDescription:self.AppName andLanguage:@"en"];
    [self.aboutData setDateOfManufacture:@"2015-04-14"];
    [self.aboutData setSoftwareVersion:@"0.1"];
    [self.aboutData setHardwareVersion:@"0.0.1"];
    [self.aboutData setSupportUrl:@"https://allseenalliance.org/"];
}

- (QStatus)hostSession
{
    AJNSessionOptions* opts = [[AJNSessionOptions alloc] init];
    AJNSessionPort port = DOOR_APPLICATION_PORT;

    return [self.BusAttachment bindSessionOnPort:port withOptions:opts withDelegate:self.sessionPortListener];
}

- (QStatus)announceAbout
{
    [self setAboutData];

    if (![self.aboutData isValid]) {
        NSLog(@"Invalid aboutData\n");
        return ER_FAIL;
    }

    return [self.aboutObj announceForSessionPort:DOOR_APPLICATION_PORT withAboutDataListener:self.aboutData];
}

+ (void)callDeprecatedSetPSK:(DefaultECDHEAuthListener*) authListener pskBytes:(const uint8_t*)pskBytes pskLength:(size_t)pskLength
{
    /*
     * This function suppresses compiler warnings when calling SetPSK, which is deprecated.
     * Use of PSK will be replaced by SPEKE in this sample.
     * https://jira.allseenalliance.org/browse/ASACORE-2761
     */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

    QStatus status = authListener->SetPSK(pskBytes, pskLength);
    if (status != ER_OK) {
        NSLog(@"Failed to set PSK - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}

- (QStatus)initialize:(BOOL)provider withListener:(id<AJNPermissionConfigurationListener>)inPcl
{
    [self createInterface];

    self.permissionConfigListener = inPcl;

    QStatus status = [self.BusAttachment start];
    if (status != ER_OK) {
        NSLog(@"Failed to start bus attachment - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [self.BusAttachment connectWithArguments:@"nil"];
    if (status != ER_OK) {
        NSLog(@"Failed to connect bus attachment - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    AJNGUID128* psk = [[AJNGUID128 alloc] init];
    if (provider) {
       // [self callDeprecatedSetPSK:self.authListener, [psk bytes], [AJNGUID128 SIZE]];
        [DoorCommon callDeprecatedSetPSK:self.authListener.handle pskBytes:psk.bytes pskLength:[AJNGUID128 SIZE]];
    }

    NSString* securitySuites = [NSString stringWithFormat:@"%@ %@ %@", KEYX_ECDHE_DSA, KEYX_ECDHE_NULL, KEYX_ECDHE_PSK];
    NSString *keystoreFilePath = @"Documents/alljoyn_keystore/s_central.ks";
    status = [self.BusAttachment enablePeerSecurity:securitySuites authenticationListener:self.authListener keystoreFileName:keystoreFilePath sharing:NO permissionConfigurationListener:self.permissionConfigListener];
    if (status != ER_OK) {
        NSLog(@"Failed to enablePeerSecurity - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    if (provider) {
        NSLog(@"Allow doors to be claimable using a PSK.\n");
        AJNClaimCapabilities capabilities = (CAPABLE_ECDHE_PSK | CAPABLE_ECDHE_NULL);
        status = [self.BusAttachment.permissionConfigurator setClaimCapabilities:capabilities];
        if (status != ER_OK) {
            NSLog(@"Failed to setClaimCapabilities - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return status;
        }

        status = [self.BusAttachment.permissionConfigurator setClaimCapabilityAdditionalInfo:PSK_GENERATED_BY_APPLICATION];
        if (status != ER_OK) {
            NSLog(@"Failed to setClaimCapabilityAdditionalInfo - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return status;
        }
    }

    PermissionPolicy::Rule manifestRule;
    manifestRule.SetInterfaceName([DOOR_INTERFACE UTF8String]);

    if (provider) {
        // Set a very flexible default manifest for the door provider
        PermissionPolicy::Rule::Member members[2];
        members[0].SetMemberName("*");
        members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        members[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        members[1].SetMemberName("*");
        members[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        members[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        manifestRule.SetMembers(2, members);
    } else {
        // Set a very flexible default manifest for the door consumer
        PermissionPolicy::Rule::Member member;
        member.SetMemberName("*");
        member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY |
                             PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
        manifestRule.SetMembers(1, &member);
    }

    NSLog(@"%@", [NSString stringWithCString:manifestRule.ToString().c_str() encoding:NSUTF8StringEncoding]);

    status = ((PermissionConfigurator*)self.BusAttachment.permissionConfigurator.handle)->SetPermissionManifestTemplate(&manifestRule, 1);
    if (ER_OK != status) {
        NSLog(@"Failed to setPermissionManifestTemplate - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    if (provider) {
        AJNApplicationState state;
        status = [self.BusAttachment.permissionConfigurator getApplicationState:&state];
        if (ER_OK != status) {
            NSLog(@"Failed to getApplicationState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return status;
        }

        if (state == CLAIMABLE) {
            NSLog(@"Door provider is not claimed.\n");
            NSLog(@"The provider can be claimed using PSK with an application generated secret.\n");
            NSLog(@"PSK = (%@)\n", [psk description]);
        }
    }

    return [self hostSession];
}

- (QStatus)setSecurityForClaimedMode
{
    QStatus status = [self.BusAttachment enablePeerSecurity:@"" authenticationListener:nil keystoreFileName:nil sharing:true];
    if (ER_OK != status) {
        fprintf(stderr, "SetSecurityForClaimedMode: Could not clear peer security - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = [self.BusAttachment enablePeerSecurity:KEYX_ECDHE_DSA authenticationListener:self.authListener keystoreFileName:@"" sharing:false permissionConfigurationListener:self.permissionConfigListener];
    if (ER_OK != status) {
        fprintf(stderr, "SetSecurityForClaimedMode: Could not reset peer security - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    return ER_OK;
}

+ (QStatus)updateDoorProviderManifest:(DoorCommon*)common
{
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName([DOOR_INTERFACE UTF8String]);

    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);

    rules[0].SetMembers(3, prms);

    PermissionPolicy::Acl manifest;
    manifest.SetRules(1, rules);

    QStatus status = ((PermissionConfigurator*)common.BusAttachment.permissionConfigurator.handle)->SetPermissionManifestTemplate(rules, manifest.GetRulesSize());
    if (ER_OK != status) {
        NSLog(@"Failed to SetPermissionManifestTemplate - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = ((PermissionConfigurator*)common.BusAttachment.permissionConfigurator.handle)->SetApplicationState(PermissionConfigurator::NEED_UPDATE);
    if (ER_OK != status) {
        NSLog(@"Failed to SetApplicationState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    return status;
}

- (void)dealloc
{
    /**
     * This is needed to make sure that the authentication listener is removed before the
     * bus attachment is destructed.
     * Use an empty string as a first parameter (authMechanism) to avoid resetting the keyStore
     * so previously claimed apps can still be so after restarting.
     **/
    [self.BusAttachment enablePeerSecurity:@"" authenticationListener:nil keystoreFileName:nil sharing:true];

    [self.BusAttachment disconnect];
    [self.BusAttachment stop];
    [self.BusAttachment waitUntilStopCompleted];

    self.authListener = nil;
    self.aboutObj = nil;
    self.BusAttachment = nil;
}

@end