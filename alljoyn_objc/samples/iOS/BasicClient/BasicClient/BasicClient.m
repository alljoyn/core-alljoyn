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

#import "BasicClient.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNSessionOptions.h"
#import "AJNVersion.h"
#import "BasicObject.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static NSString* kBasicClientInterfaceName = @"org.alljoyn.Bus.sample";
static NSString* kBasicClientServiceName = @"org.alljoyn.Bus.sample";
static NSString* kBasicClientServicePath = @"/sample";
static const AJNSessionPort kBasicClientServicePort = 25;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Client private interface
//

@interface BasicClient() <AJNBusListener, AJNSessionListener>

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
// BasicClient implementation
//

@implementation BasicClient

- (id)init {
    if (self = [super init]) {
        self.clientQueue = dispatch_queue_create("org.alljoyn.BasicClient.ClientQueue", nil);
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

- (void)run {
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library version: %@\n", AJNVersion.versionInformation]];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library build info: %@\n", AJNVersion.buildInformation]];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    QStatus status;

    // create the message bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"BasicClient" allowRemoteMessages:YES];

    // start the message bus
    //
    status =  [self.bus start];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Start failed\n"];
        return;
    } else {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment started.\n"];
    }

    // connect to the message bus
    //
    status = [self.bus connectWithArguments:@"null:"];

    if (status != ER_OK) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Connect(\"null:\") failed\n"];
        return;
    }
    else {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachement connected to null:\n"];
    }

    self.joinedSessionCondition = [[NSCondition alloc] init];
    [self.joinedSessionCondition lock];

    // register a bus listener in order to receive discovery notifications
    //
    [self.bus registerBusListener:self];

    // begin discovery of the well known name of the service to be called
    //
    [self.bus findAdvertisedName:kBasicClientServiceName];

    [self.delegate didReceiveStatusUpdateMessage:@"Waiting to discover service...\n"];

    // wait for the join session to complete
    // AJNBusListener::didFindAdvertisedName(_:withTransportMask:namePrefix) listener is finished
    //
    if ([self.joinedSessionCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:60]]) {

        // once joined to a session, use a proxy object to make the function call
        //
        BasicObjectProxy *basicObjectProxy = [[BasicObjectProxy alloc] initWithBusAttachment:self.bus serviceName:self.foundServiceName objectPath:kBasicClientServicePath sessionId:self.sessionId];

        // get a description of the interfaces implemented by the remote object before making the call
        //
        [basicObjectProxy introspectRemoteObject];

        // now make the function call
        //
        NSString *result = [basicObjectProxy concatenateString:@"Code " withString:@"monkeys!"];

        if (result) {
            // compare  result string with the expected one
            //
            NSLog(@"[%@] %@ concatenated string.", result, [result compare:@"Code monkeys!"] == NSOrderedSame ? @"Successfully" : @"Unsuccessfully");

            [self.delegate didReceiveStatusUpdateMessage:@"Successfully called method on remote object!\n"];
        }

        // We call ping to make sure the service can be pinged. This is to demonstrate usage of ping API
        [self ping];
    }
    else {
        [self.delegate didReceiveStatusUpdateMessage:@"Timed out while attempting to join a session with BasicService...\n"];
    }

    [self.joinedSessionCondition unlock];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");


    // Unregister and destroy listener
    //
    [self.bus unregisterBusListener:self];
    [self.bus destroyBusListener:self];

    NSLog(@"destroyed bus");

    [self.delegate didReceiveStatusUpdateMessage:@"Bus listener is unregistered and destroyed.\n"];
}

- (void)ping {

    if (self.bus != nil) {
        QStatus status = [self.bus pingPeer:kBasicClientServiceName withTimeout:5];
        [self.delegate didReceiveStatusUpdateMessage: status == ER_OK ? @"Ping returned successfully\n" : @"Ping failed\n"];
    }
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

    if ([namePrefix compare:kBasicClientServiceName] == NSOrderedSame) {

        BOOL shouldReturn;
        @synchronized(self) {
            shouldReturn = self.wasNameAlreadyFound;
            self.wasNameAlreadyFound = YES;
        }

        if (shouldReturn) {
            NSLog(@"Already found an advertised name, ignoring this name %@...", name);
            [self.joinedSessionCondition signal];
            return;
        }

        // Since we are in a callback we must enable concurrent callbacks before calling a synchronous method.
        //
        [self.bus enableConcurrentCallbacks];

        self.sessionId = [self.bus joinSessionWithName:name onPort:kBasicClientServicePort withDelegate:self options:[[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny]];

        if (self.sessionId) {
            self.foundServiceName = name;
            [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"JoinSession SUCCESS (Session id=%d)\n", self.sessionId]];
        }
        else {
            [self.delegate didReceiveStatusUpdateMessage:@"JoinSession failed\n"];
        }

        [self.joinedSessionCondition signal];
    }
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix {
    NSLog(@"AJNBusListener::didLoseAdvertisedName:%@ withTransportMask:%u namePrefix:%@", name, transport,namePrefix);
}

- (void)nameOwnerChanged:(NSString*)name to:(NSString*)newOwner from:(NSString*)previousOwner {
    NSLog(@"AJNBusListener::nameOwnerChanged:%@ to:%@ from:%@", name, newOwner, previousOwner);
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"NameOwnerChanged: name=%@, oldOwner=%@, newOwner=%@\n", name, previousOwner, newOwner]];
}

- (void)busWillStop {
    NSLog(@"AJNBusListener::busWillStop");
}

- (void)busDidDisconnect {
    NSLog(@"AJNBusListener::busDidDisconnect");
}

#pragma mark - AJNSessionListener methods

- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason {
    NSLog(@"AJNBusListener::sessionWasLost %u forReason:%u", sessionId, reason);
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId {
    NSLog(@"AJNBusListener::didAddMemberNamed:%@ toSession:%u", memberName, sessionId);
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId {
    NSLog(@"AJNBusListener::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId);
}


@end
