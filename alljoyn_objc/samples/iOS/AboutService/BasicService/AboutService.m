////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright 2016 Open Connectivity Foundation and Contributors to
//    AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "AboutService.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNSessionOptions.h"
#import "AJNAboutDataListener.h"
#import "AJNAboutObject.h"
#import "AJNVersion.h"
#import "BasicObject.h"
#import "AJNAboutData.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static NSString* kAboutServiceInterfaceName = @"com.example.about.feature.interface.sample";
static NSString* kAboutServiceName = @"com.example.about.feature.interface.sample";
static NSString* kAboutServicePath = @"/example/path";
static const AJNSessionPort kAboutServicePort = 900;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service private interface
//

@interface AboutService() <AJNBusListener, AJNSessionPortListener, AJNSessionListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) BasicObject *basicObject;
@property (nonatomic, strong) NSCondition *waitCondition;

- (void)run;

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service implementation
//

@implementation AboutService

@synthesize bus = _bus;
@synthesize basicObject = _basicObject;
@synthesize waitCondition = _waitCondition;
@synthesize delegate = _delegate;

- (void)startService
{
    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.basic-service.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run];
    });

}

- (void)run
{

    NSLog(@"AllJoyn Library version: %@", AJNVersion.versionInformation);
    NSLog(@"AllJoyn Library build info: %@\n", AJNVersion.buildInformation);
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library version: %@\n", [AJNVersion versionInformation]]];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library build info: %@\n", [AJNVersion buildInformation]]];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    QStatus status;

    // create the message bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"BasicService" allowRemoteMessages:YES];

    self.waitCondition = [[NSCondition alloc] init];
    [self.waitCondition lock];

    // register a bus listener
    //
    [self.bus registerBusListener:self];

    // allocate the service's bus object, which also creates and activates the service's interface
    //
    self.basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kAboutServicePath];

    // start the message bus
    //
    status =  [self.bus start];

    if (ER_OK != status) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Start failed\n"];

        NSLog(@"Bus start failed.");
    }

    // register the bus object
    //
    status = [self.bus registerBusObject:self.basicObject];
    if (ER_OK != status) {
        NSLog(@"ERROR: Could not register bus object");
    }

    [self.delegate didReceiveStatusUpdateMessage:@"Object registered successfully.\n"];

    // connect to the message bus
    //
    status = [self.bus connectWithArguments:@"null:"];

    if (ER_OK != status) {
        NSLog(@"Bus connect failed.");
        [self.delegate didReceiveStatusUpdateMessage:@"Failed to connect to null: transport"];
    }

    [self.delegate didReceiveStatusUpdateMessage:@"Bus now connected to null: transport\n"];

    // Advertise this service on the bus
    // There are three steps to advertising this service on the bus
    //   1) Request a well-known name that will be used by the client to discover
    //       this service
    //   2) Create a session
    //   3) Advertise the well-known name
    //

    // request the name
    //
    status = [self.bus requestWellKnownName:kAboutServiceName withFlags:kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue];
    if (ER_OK != status) {
        NSLog(@"ERROR: Request for name failed (%@)", kAboutServiceName);
    }


    // bind a session to a service port
    //
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];

    status = [self.bus bindSessionOnPort:kAboutServicePort withOptions:sessionOptions withDelegate:self];
    if (ER_OK != status) {
        NSLog(@"ERROR: Could not bind session on port (%d)", kAboutServicePort);
    }



    // Setup the about data
    // The default language is specified in the constructor. If the default language
    // is not specified any Field that should be localized will return an error
    AJNAboutData *aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];

    //AppId is a 128bit uuid
    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
        0x1E, 0x82, 0x11, 0xE4,
        0x86, 0x51, 0xD1, 0x56,
        0x1D, 0x5D, 0x46, 0xB0 };
    status = [aboutData setAppId:appId];
    status = [aboutData setDeviceName:@"iPhone" andLanguage:@"en"];
    //DeviceId is a string encoded 128bit UUID
    status = [aboutData setDeviceId:@"93c06771-c725-48c2-b1ff-6a2a59d445b8"];
    status = [aboutData setAppName:@"Application" andLanguage:@"en"];
    status = [aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];
    status = [aboutData setModelNumber:@"123456"];
    status = [aboutData setDescription:@"A poetic description of this application" andLanguage:@"en"];
    status = [aboutData setDateOfManufacture:@"14/07/2016"];
    status = [aboutData setSoftwareVersion:@"0.1.2"];
    status = [aboutData setHardwareVersion:@"0.0.1"];
    status = [aboutData setSupportUrl:@"http://www.example.org"];

    // The default language is automatically added to the `SupportedLanguages`
    // Users don't have to specify the AJSoftwareVersion its automatically added
    // to the AboutData

    // Adding Spanish Localization values to the AboutData. All strings MUST be
    // UTF-8 encoded.
    status = [aboutData setDeviceName:@"Mi dispositivo Nombre" andLanguage:@"es"];
    status = [aboutData setAppName:@"aplicación" andLanguage:@"es"];
    status = [aboutData setManufacturer:@"fabricante" andLanguage:@"es"];
    status = [aboutData setDescription:@"Una descripción poética de esta aplicación" andLanguage:@"es"];

    // Check to see if the aboutData is valid before sending the About Announcement
    if (![aboutData isValid]) {
        printf("failed to setup about data.\n");
    }

    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    status = [aboutObj announceForSessionPort:kAboutServicePort withAboutDataListener:aboutData];
    if (ER_OK == status) {
        printf("AboutObj Announce Succeeded.\n");
    } else {
        printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
        exit(1);
    }


    [self.delegate didReceiveStatusUpdateMessage:@"-------------\n"];
    [self.delegate didReceiveStatusUpdateMessage:@"Announce sent\n"];
    [self.delegate didReceiveStatusUpdateMessage:@"-------------\n"];

    // wait until the client leaves before tearing down the service
    //
    [self.waitCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:600]];

    // clean up
    //
    [self.bus unregisterBusObject:self.basicObject];

    [self.waitCondition unlock];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    // deallocate the bus
    //
    self.bus = nil;
}

#pragma mark - AJNBusListener delegate methods
- (void)listenerDidRegisterWithBus:(AJNBusAttachment*)busAttachment
{
    NSLog(@"AJNBusListener::listenerDidRegisterWithBus:%@",busAttachment);
}

- (void)listenerDidUnregisterWithBus:(AJNBusAttachment*)busAttachment
{
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@",busAttachment);
}

- (void)didFindAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::didFindAdvertisedName:%@ withTransportMask:%u namePrefix:%@", name, transport, namePrefix);
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@ withTransportMask:%u namePrefix:%@",name,transport,namePrefix);
}

- (void)nameOwnerChanged:(NSString*)name to:(NSString*)newOwner from:(NSString*)previousOwner
{
    NSLog(@"AJNBusListener::nameOwnerChanged:%@ to:%@ from:%@", name, newOwner, previousOwner);
}

- (void)busWillStop
{
    NSLog(@"AJNBusListener::busWillStop");
}

- (void)busDidDisconnect
{
    NSLog(@"AJNBusListener::busDidDisconnect");
}

#pragma mark - AJNSessionListener methods

- (void)sessionWasLost:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::sessionWasLost %u", sessionId);
}


- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSLog(@"AJNBusListener::sessionWasLost %u forReason:%u", sessionId, reason);
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::didAddMemberNamed:%@ toSession:%u", memberName, sessionId);
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId);
}

#pragma mark - AJNSessionPortListener implementation

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString*)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions*)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    BOOL shouldAcceptSessionJoiner = kAboutServicePort == sessionPort;
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Request from %@ to join session is %@.\n", joiner, shouldAcceptSessionJoiner ? @"accepted" : @"rejected"]];
    return shouldAcceptSessionJoiner;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
    [self.bus enableConcurrentCallbacks];
    [self.bus setSessionListener:self toSession:sessionId];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"%@ successfully joined session %u on port %d.\n", joiner, sessionId, sessionPort]];

}

@end