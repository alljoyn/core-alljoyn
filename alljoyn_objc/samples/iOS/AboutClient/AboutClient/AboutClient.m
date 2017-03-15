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

#import "AboutClient.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNSessionOptions.h"
#import "AJNVersion.h"
#import "AJNAboutProxy.h"
#import "String.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static NSString* kAboutClientName = @"com.example.about.feature.interface.sample";

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// About Client private interface
//

@interface AboutClient() <AJNBusListener, AJNSessionListener, AJNAboutListener>

@property (nonatomic, strong) NSCondition *receivedAnnounce;
@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) NSCondition *joinedSessionCondition;
@property (nonatomic) AJNSessionId sessionId;
@property (nonatomic, strong) NSString *foundServiceName;
@property (nonatomic, strong) dispatch_queue_t clientQueue;
@property (nonatomic) BOOL wasNameAlreadyFound;

- (void)run;

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// About Client implementation
//

@implementation AboutClient

- (id)init {
    if (self = [super init]) {
        self.clientQueue = dispatch_queue_create("org.alljoyn.AboutClient.ClientQueue", nil);
        return self;
    }
    return nil;
}

- (void)sendHelloMessage {
    dispatch_async(self.clientQueue, ^{
        self.wasNameAlreadyFound = NO;
        [self run];
    });
}

- (void)run
{
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library version: %@", AJNVersion.versionInformation]];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library build info: %@", AJNVersion.buildInformation]];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    QStatus status;

    // create the message bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"AboutClient" allowRemoteMessages:YES];

    self.receivedAnnounce = [[NSCondition alloc] init];
    [self.receivedAnnounce lock];

    [self.bus registerAboutListener:self];
    // register a bus listener in order to receive discovery notifications
    //
    [self.bus registerBusListener:self];


    // start the message bus
    //
    status =  [self.bus start];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Start failed"];
        return;
    } else {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment started"];
    }

    // connect to the message bus
    //
    status = [self.bus connectWithArguments:@"null:"];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Connect(\"null:\") failed"];
        return;
    }
    else {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachement connected to null:"];
    }

    // ask for announced interfaces
    //
    [self.bus whoImplementsInterface:kAboutClientName];

    [self.delegate didReceiveStatusUpdateMessage:@"Waiting to discover service..."];

    [self.receivedAnnounce waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:60]];

    [self.receivedAnnounce unlock];

    // We call ping to make sure the service can be pinged. This is to demonstrate usage of ping API
    [self ping];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    [self.bus disconnect];
    [self.bus stop];

    [self.delegate didReceiveStatusUpdateMessage:@"Bus stopped"];
}


- (void)ping
{
    if (self.bus != nil) {
        QStatus status = [self.bus pingPeer:_foundServiceName withTimeout:5];
        [self.delegate didReceiveStatusUpdateMessage: status == ER_OK ? @"Ping returned successfully" : @"Ping failed"];
    }
}

#pragma mark - AJNAboutListener delegate method

- (void)didReceiveAnnounceOnBus:(NSString *)busName withVersion:(uint16_t)version withSessionPort:(AJNSessionPort)port withObjectDescription:(AJNMessageArgument *)objectDescriptionArg withAboutDataArg:(AJNMessageArgument *)aboutDataArg
{
    [self.delegate didReceiveStatusUpdateMessage:@"---------------------------"];
    [self.delegate didReceiveStatusUpdateMessage:@"Received Announce Signal"];
    [self.delegate didReceiveStatusUpdateMessage:@"---------------------------"];

    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    // Since we are in a callback we must enable concurrent callbacks before calling a synchronous method.
    //
    [self.bus enableConcurrentCallbacks];
    _sessionId = [self.bus joinSessionWithName:busName onPort:port withDelegate:self options:sessionOptions];

    if (_sessionId) {
        _foundServiceName = busName;
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Successfully joined session with id %d", self.sessionId]];
    }
    else {
        [self.delegate didReceiveStatusUpdateMessage:@"JoinSession failed"];
    }

    // Create AboutProxy
    AJNAboutProxy *aboutProxy = [[AJNAboutProxy alloc] initWithBusAttachment:self.bus busName:busName sessionId:_sessionId];

    // Make a call to GetAboutData and GetVersion
    uint16_t aboutVersion;
    NSMutableDictionary *aboutData;
    [aboutProxy getVersion:&aboutVersion];
    [aboutProxy getAboutDataForLanguage:@"en" usingDictionary:&aboutData];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"About proxy version is %d", version]];
    [self.receivedAnnounce signal];
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
    if ([name isEqualToString:kAboutClientName]) {
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"NameOwnerChanged: name=%@, oldOwner=%@, newOwner=%@", name, previousOwner, newOwner]];
    }
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


@end
