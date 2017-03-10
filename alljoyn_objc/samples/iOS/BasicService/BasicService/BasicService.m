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

#import "BasicService.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNSessionOptions.h"
#import "AJNVersion.h"
#import "BasicObject.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static NSString* kBasicServiceName = @"org.alljoyn.Bus.sample";
static NSString* kBasicServicePath = @"/sample";
static const AJNSessionPort kBasicServicePort = 25;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service private interface
//

@interface BasicService() <AJNBusListener, AJNSessionPortListener, AJNSessionListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) BasicObject *basicObject;
@property (nonatomic, strong) NSCondition *waitCondition;

- (void)startService;
- (void)stopService;

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service implementation
//

@implementation BasicService

- (void)startServiceAsync {

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self startService];
    });
}

- (void)stopServiceAsync {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self stopService];
    });
}

- (void)printVersionInformation {
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library version: %@\n", [AJNVersion versionInformation]]];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library build info: %@\n", [AJNVersion buildInformation]]];
}

- (void)startService {
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    QStatus status;

    // create the message bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"BasicService" allowRemoteMessages:YES];

    // register a bus listener
    //
    [self.bus registerBusListener:self];

    // allocate the service's bus object, which also creates and activates the service's interface
    //
    self.basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBasicServicePath];

    // start the message bus
    //
    status =  [self.bus start];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Start failed\n"];
        return;
    }

    // register the bus object
    //
    status = [self.bus registerBusObject:self.basicObject];
    [self.delegate didReceiveStatusUpdateMessage: status == ER_OK ? @"Object registered successfully.\n" : @"ERROR: Could not register bus object\n"];

    // connect to the message bus
    //
    status = [self.bus connectWithArguments:@"null:"];
    [self.delegate didReceiveStatusUpdateMessage: status == ER_OK ? @"Bus is connected to null: transport\n" : @"Failed to connect to null: transport\n"];

    // Advertise this service on the bus
    // There are three steps to advertising this service on the bus
    //   1) Request a well-known name that will be used by the client to discover
    //       this service
    //   2) Create a session
    //   3) Advertise the well-known name
    //

    // request the name
    //
    status = [self.bus requestWellKnownName:kBasicServiceName withFlags:kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue];
    if (status != ER_OK) {
        NSLog(@"ERROR: Request for name %@ is failed", kBasicServiceName);
    }

    // bind a session to a service port
    //
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];

    status = [self.bus bindSessionOnPort:kBasicServicePort withOptions:sessionOptions withDelegate:self];
    if (status != ER_OK) {
        NSLog(@"ERROR: Could not bind session on port %d", kBasicServicePort);
    }

    // advertise a name
    //
    status = [self.bus advertiseName:kBasicServiceName withTransportMask:kAJNTransportMaskAny];
    if (ER_OK != status) {
        NSLog(@"Could not advertise (%@)", kBasicServiceName);
    }
}

- (void)stopService {

    //cancel name advertising
    //
    [self.bus cancelAdvertisedName:kBasicServiceName withTransportMask:kAJNTransportMaskAny];

    // unbind session
    //
    [self.bus unbindSessionFromPort:kBasicServicePort];

    // clean up
    //
    [self.bus unregisterBusObject:self.basicObject];

    // unregister listener
    //
    [self.bus unregisterBusListener:self];

    // destroy listener
    //
    [self.bus destroyBusListener:self];
    [self.delegate didReceiveStatusUpdateMessage:@"Bus listener is unregistered and destroyed.\n"];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
}

#pragma mark - AJNBusListener delegate methods

- (void)listenerDidRegisterWithBus:(AJNBusAttachment*)busAttachment {
    NSLog(@"AJNBusListener::listenerDidRegisterWithBus:%@",busAttachment);
}

- (void)listenerDidUnregisterWithBus:(AJNBusAttachment*)busAttachment {
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@",busAttachment);
}

- (void)didFindAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix {
    NSLog(@"AJNBusListener::didFindAdvertisedName:%@ withTransportMask:%u namePrefix:%@", name, transport, namePrefix);
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix {
    NSLog(@"AJNBusListener::didLoseAdvertisedName:%@ withTransportMask:%u namePrefix:%@",name,transport,namePrefix);
}

- (void)nameOwnerChanged:(NSString*)name to:(NSString*)newOwner from:(NSString*)previousOwner {
    NSLog(@"AJNBusListener::nameOwnerChanged:%@ to:%@ from:%@", name, newOwner, previousOwner);
}

- (void)busWillStop {
    NSLog(@"AJNBusListener::busWillStop");
}

- (void)busDidDisconnect {
    NSLog(@"AJNBusListener::busDidDisconnect");
}

#pragma mark - AJNSessionListener methods

- (void)sessionWasLost:(AJNSessionId)sessionId {
    NSLog(@"AJNBusListener::sessionWasLost %u", sessionId);
}


- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason {
    NSLog(@"AJNBusListener::sessionWasLost %u forReason:%u", sessionId, reason);
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId {
    NSLog(@"AJNBusListener::didAddMemberNamed:%@ toSession:%u", memberName, sessionId);
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId {
    NSLog(@"AJNBusListener::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId);
}

#pragma mark - AJNSessionPortListener implementation

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString*)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions*)options {
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    BOOL shouldAcceptSessionJoiner = kBasicServicePort == sessionPort;
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Request from %@ to join session is %@.\n", joiner, shouldAcceptSessionJoiner ? @"accepted" : @"rejected"]];
    return shouldAcceptSessionJoiner;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort {
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);

    //set session listener to be able get callbacks of AJNSessionListener delegate
    //
    [self.bus enableConcurrentCallbacks];
    [self.bus setSessionListener:self toSession:sessionId];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"%@ successfully joined session %u on port %d.\n", joiner, sessionId, sessionPort]];
}

@end
