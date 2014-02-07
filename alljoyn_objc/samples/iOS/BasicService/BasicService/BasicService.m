////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

static NSString* kBasicServiceInterfaceName = @"org.alljoyn.Bus.sample";
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
@property (nonatomic, strong) NSCondition *lostSessionCondition;

- (void)run;

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service implementation
//

@implementation BasicService

@synthesize bus = _bus;
@synthesize basicObject = _basicObject;
@synthesize lostSessionCondition = _lostSessionCondition;
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
    
    self.lostSessionCondition = [[NSCondition alloc] init];
    [self.lostSessionCondition lock];
    
    // register a bus listener
    //
    [self.bus registerBusListener:self];
    
    // allocate the service's bus object, which also creates and activates the service's interface
    //
    self.basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBasicServicePath];
    
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
    status = [self.bus requestWellKnownName:kBasicServiceName withFlags:kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue];
    if (ER_OK != status) {
        NSLog(@"ERROR: Request for name failed (%@)", kBasicServiceName);
    }
    
    
    // bind a session to a service port
    //
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBasicServicePort withOptions:sessionOptions withDelegate:self];
    if (ER_OK != status) {
        NSLog(@"ERROR: Could not bind session on port (%d)", kBasicServicePort);
    }
    
    // advertise a name
    //
    status = [self.bus advertiseName:kBasicServiceName withTransportMask:kAJNTransportMaskAny];
    if (ER_OK != status) {
        NSLog(@"Could not advertise (%@)", kBasicServiceName);
    }
    
    // wait until the client leaves before tearing down the service
    //
    [self.lostSessionCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:60]];
    
    // clean up
    //
    [self.bus unregisterBusObject:self.basicObject];
    
    [self.lostSessionCondition unlock];
    
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
    
    [self.lostSessionCondition lock];
    [self.lostSessionCondition signal];
    [self.lostSessionCondition unlock];
}


- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSLog(@"AJNBusListener::sessionWasLost %u forReason:%u", sessionId, reason);
    [self.lostSessionCondition lock];
    [self.lostSessionCondition signal];
    [self.lostSessionCondition unlock];
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
    BOOL shouldAcceptSessionJoiner = kBasicServicePort == sessionPort;
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Request from %@ to join session is %@.\n", joiner, shouldAcceptSessionJoiner ? @"accepted" : @"rejected"]];
    return shouldAcceptSessionJoiner;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
    [self.bus enableConcurrentCallbacks];
    [self.bus bindSessionListener:self toSession:sessionId];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"%@ successfully joined session %u on port %d.\n", joiner, sessionId, sessionPort]];
    
}

@end
