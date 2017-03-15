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

#import "AboutService.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNAboutDataListener.h"
#import "AJNAboutObject.h"
#import "AJNVersion.h"
#import "AboutSampleObject.h"
#import "AJNAboutData.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static NSString* kAboutServiceName = @"com.example.about.feature.interface.sample";
static NSString* kAboutServicePath = @"/example/path";
static const AJNSessionPort kAboutServicePort = 900;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// About Service private interface
//

@interface AboutService() <AJNBusListener, AJNSessionPortListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) AboutSampleObject *aboutSampleObject;
@property (nonatomic, strong) NSCondition *waitCondition;
@property (nonatomic, strong) dispatch_queue_t serviceQueue;

- (void)run;
- (AJNAboutData*) setupAboutData;

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// About Service implementation
//

@implementation AboutService

- (id)init {
    if (self = [super init]) {
        self.serviceQueue = dispatch_queue_create("org.alljoyn.AboutService.ServiceQueue", nil);
        return self;
    }
    return nil;
}

- (void)startService
{
    dispatch_async(self.serviceQueue, ^{
        [self run];
    });
}

- (void)run
{
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn library version: %@", [AJNVersion versionInformation]]];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn library build info: %@", [AJNVersion buildInformation]]];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    QStatus status;

    // create the message bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"AboutService" allowRemoteMessages:YES];

    self.waitCondition = [[NSCondition alloc] init];
    [self.waitCondition lock];

    // register a bus listener
    //
    [self.bus registerBusListener:self];

    // allocate the service's bus object, which also creates and activates the service's interface
    //
    self.aboutSampleObject = [[AboutSampleObject alloc] initWithBusAttachment:self.bus onPath:kAboutServicePath];

    // start the message bus
    //
    status =  [self.bus start];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"Bus start is failed."];
        return;
    } else {
        [self.delegate didReceiveStatusUpdateMessage:@"Bus is started."];
    }

    // register the bus object
    //
    status = [self.bus registerBusObject:self.aboutSampleObject];
    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"ERROR: Could not register bus object."];
        [self.bus stop];
        return;
    }

    [self.delegate didReceiveStatusUpdateMessage:@"Object registered successfully."];

    // connect to the message bus
    //
    status = [self.bus connectWithArguments:@"null:"];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Connect(\"null:\") failed"];
        [self.bus stop];
        return;
    }
    else {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachement connected to null:"];
    }

    // bind a session to a service port
    //
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];

    status = [self.bus bindSessionOnPort:kAboutServicePort withOptions:sessionOptions withDelegate:self];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"ERROR: Could not bind session on port (%d)", kAboutServicePort]];
    }

    // Setup the about data
    //
    AJNAboutData *aboutData = [self setupAboutData];

    // Check if the aboutData is valid before sending the About Announcement
    //
    [self.delegate didReceiveStatusUpdateMessage:[aboutData isValid] ? @"About data is setup correctly." : @"Failed to setup about data."];

    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    status = [aboutObj announceForSessionPort:kAboutServicePort withAboutDataListener:aboutData];

    if (status == ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"---------------"];
        [self.delegate didReceiveStatusUpdateMessage:@"Announce sent"];
        [self.delegate didReceiveStatusUpdateMessage:@"---------------"];
    } else {
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Announce failed (%s)", QCC_StatusText(status)]];
        return;
    }

    // wait until the client leaves before tearing down the service
    //
    [self.waitCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:600]];

    [self.waitCondition unlock];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    // clean up
    //
    [self.bus unregisterBusObject:self.aboutSampleObject];

    [self.bus disconnect];
    [self.bus stop];

    [self.delegate didReceiveStatusUpdateMessage:@"Bus stopped"];

}

- (AJNAboutData*) setupAboutData
{
    // The default language is specified in the constructor. If the default language
    // is not specified any Field that should be localized will return an error
    //
    AJNAboutData *aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];

    //AppId is a 128bit uuid
    //
    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
        0x1E, 0x82, 0x11, 0xE4,
        0x86, 0x51, 0xD1, 0x56,
        0x1D, 0x5D, 0x46, 0xB0 };
    [aboutData setAppId:appId];
    [aboutData setDeviceName:@"iPhone" andLanguage:@"en"];

    //DeviceId is a string encoded 128bit UUID
    //
    [aboutData setDeviceId:@"93c06771-c725-48c2-b1ff-6a2a59d445b8"];
    [aboutData setAppName:@"Application" andLanguage:@"en"];
    [aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];
    [aboutData setModelNumber:@"123456"];
    [aboutData setDescription:@"A poetic description of this application" andLanguage:@"en"];
    [aboutData setDateOfManufacture:@"14/07/2016"];
    [aboutData setSoftwareVersion:@"0.1.2"];
    [aboutData setHardwareVersion:@"0.0.1"];
    [aboutData setSupportUrl:@"http://www.example.org"];

    // The default language is automatically added to the `SupportedLanguages`
    // Users don't have to specify the AJSoftwareVersion its automatically added
    // to the AboutData

    // Adding Spanish Localization values to the AboutData. All strings MUST be
    // UTF-8 encoded.
    //
    [aboutData setDeviceName:@"Mi dispositivo Nombre" andLanguage:@"es"];
    [aboutData setAppName:@"aplicación" andLanguage:@"es"];
    [aboutData setManufacturer:@"fabricante" andLanguage:@"es"];
    [aboutData setDescription:@"Una descripción poética de esta aplicación" andLanguage:@"es"];

    return aboutData;
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
    NSLog(@"AJNBusListener::didLoseAdvertisedName:%@ withTransportMask:%u namePrefix:%@",name,transport,namePrefix);
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

#pragma mark - AJNSessionPortListener implementation

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString*)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions*)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    BOOL shouldAcceptSessionJoiner = kAboutServicePort == sessionPort;
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Request from %@ to join session is %@.", joiner, shouldAcceptSessionJoiner ? @"accepted" : @"rejected"]];
    return shouldAcceptSessionJoiner;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
}

@end
