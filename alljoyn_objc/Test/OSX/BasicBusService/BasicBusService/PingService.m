////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, 2014 AllSeen Alliance. All rights reserved.
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

#import "PingService.h"

#import "Constants.h"
#import "PingService.h"
#import "AJNBusAttachment.h"
#import "AJNPingObject.h"
#import "PingObject.h"
#import "AJNSessionOptions.h"

static PingService *s_sharedInstance;

@interface PingService() <AJNBusListener, AJNSessionListener, AJNSessionPortListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) PingObject *pingObject;
@property (nonatomic, strong) NSString *serviceName;
@end

@implementation PingService

@synthesize bus = _bus;
@synthesize pingObject = _pingObject;
@synthesize delegate = _delegate;
@synthesize serviceName = _serviceName;

- (id)initWithDelegate:(id<PingServiceDelegate>)delegate
{
    self = [super init];
    if (self) {
        self.delegate = delegate;
    }
    return self;
}

- (void)start:(NSString *)serviceName
{
    QStatus status = ER_OK;
    
    self.serviceName = serviceName;
    
    @try {
        // allocate and initalize the bus
        //
        self.bus = [[AJNBusAttachment alloc] initWithApplicationName:kAppName allowRemoteMessages:YES];
        
        // start the bus
        //
        status = [self.bus start];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Bus failed to start. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Unable to start bus" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:@"Bus started successfully."];
        
        // create the bus object that the Ping service will expose
        //
        self.pingObject = [[PingObject alloc] initWithBusAttachment:self.bus onPath:kServicePath];
        
        // register the ping object with the bus
        //
        status = [self.bus registerBusObject:self.pingObject];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Bus object failed to register. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Unable to register bus object" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:@"Bus object registered successfully."];
        
        // connect the bus using the null transport
        //
        status = [self.bus connectWithArguments:@"null:"];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Bus failed to connect. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Unable to connect to bus" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:@"Bus connected successfully."];
        
        // register the PingService object as a bus listener, which allows us to find
        // well known names that are advertised on the bus.
        //
        [self.bus registerBusListener:self];
        [self.delegate receivedStatusMessage:@"Bus listener registered successfully."];
        
        // create the session options for this service's connections with its clients
        //
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:self.delegate.transportType];
        
        // request to become the owner of the service name for the ping object
        //
        status = [self.bus requestWellKnownName:self.serviceName withFlags:kAJNBusNameFlagReplaceExisting|kAJNBusNameFlagDoNotQueue];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to gain ownership of service name. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Request for well-known name failed" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:@"Request for service name succeeded."];
        
        // bind the session to a specific service port
        //
        status = [self.bus bindSessionOnPort:kServicePort withOptions:sessionOptions withDelegate:self];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to bind session on port. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Unable to bind session to port" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:@"Session to port binding successfully."];
        
        NSString *message = [NSString stringWithFormat:@"Attempting to advertise service %@ using transport %@.", self.serviceName, @"Any"];
        NSLog(@"%@", message);
        [self.delegate receivedStatusMessage:message];

        // let others on the bus know that this service exists
        //
        status = [self.bus advertiseName:self.serviceName withTransportMask:sessionOptions.transports];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to advertise service name. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Unable to advertise service name" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Now advertising name [%@]. Waiting for clients to join session...", self.serviceName]];
        
    }
    @catch (NSException *exception) {
        [self.delegate receivedStatusMessage:@"ERROR: Exception thrown in PingClient::connectToService:."];
    }
}

- (void)stop
{
    QStatus status = ER_OK;
    
    // cancel name advertisement
    //
    [self.bus cancelAdvertisedName:self.serviceName withTransportMask:self.delegate.transportType];
    
    // unregister as a listener
    //
    [self.bus unregisterBusListener:self];
    [self.delegate receivedStatusMessage:@"Bus listener registered successfully."];
    
    // disconnect the bus
    //
    status = [self.bus disconnectWithArguments:@"null:"];
    if (status != ER_OK) {
        [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to disconnect from bus. %@",[AJNStatus descriptionForStatusCode:status]]];
        @throw [NSException exceptionWithName:@"PingClient::stop: Failed" reason:@"Failed to disconnect from bus" userInfo:nil];
    }
    [self.delegate receivedStatusMessage:@"Bus disconnected successfully."];
    
    
    // stop the bus
    //
    status = [self.bus stop];
    if (status != ER_OK) {
        [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to stop the bus. %@",[AJNStatus descriptionForStatusCode:status]]];
        @throw [NSException exceptionWithName:@"PingClient::stop: Failed" reason:@"Failed to stop the bus" userInfo:nil];
    }
    [self.delegate receivedStatusMessage:@"Bus stopped successfully."];
    
    // let our ping object deallocate
    //
    self.pingObject = nil;
    [self.delegate receivedStatusMessage:@"Deallocated ping object."];
    
    // destroy the listener
    //
    [self.bus destroyBusListener:self];
    [self.delegate receivedStatusMessage:@"Destroyed bus listener."];
    
    // deallocate the bus
    //
    self.bus = nil;
    [self.delegate receivedStatusMessage:@"Deallocated bus attachment."];
}

#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString *)namePrefix
{
    NSLog(@"PingClient::didFindAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix);
    [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Discovered advertised name [%@] on the bus.", name]];    
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSString *message = [NSString stringWithFormat:@"PingClient::didLoseAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix];
    NSLog(@"%@", message);
    [self.delegate receivedStatusMessage:message];
    
}

- (void)nameOwnerChanged:(NSString *)name to:(NSString *)newOwner from:(NSString *)previousOwner
{
    NSString *message = [NSString stringWithFormat:@"PingClient::nameOwnerChanged:%@ to:%@ namePrefix:%@", name, newOwner, previousOwner];
    NSLog(@"%@", message);
    [self.delegate receivedStatusMessage:message];
}

#pragma mark - AJNSessionPortListener delegate methods

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"SampleService::didJoin:%@ inSessionWithId:%u onSessionPort:%u", joiner, sessionId, sessionPort);
    
    [self.delegate client:joiner didJoinSession:sessionId];
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    NSLog(@"SampleService::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u", joiner, sessionPort);
    
    [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Received join request from %@ on port %u.", joiner, sessionPort]];
    
    BOOL shouldAllowClientToJoinSession = sessionPort == kServicePort;
    
    if (shouldAllowClientToJoinSession) {
        [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Accepted join request from %@ on port %u.", joiner, sessionPort]];
    }
    
    // only allow session joiners who use our designated port number
    //
    return shouldAllowClientToJoinSession;
}

#pragma mark - AJNSessionListener delegate methods

- (void)sessionWasLost:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"PingClient::sessionWasLost:%u", sessionId];
    NSLog(@"%@", message);
    [self.delegate receivedStatusMessage:message];
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"PingClient::didAddMemberNamed:%@ toSession:%u", memberName, sessionId];
    NSLog(@"%@", message);
    [self.delegate receivedStatusMessage:message];
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"PingClient::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId];
    NSLog(@"%@", message);
    [self.delegate receivedStatusMessage:message];
    [self.delegate client:memberName didLeaveSession:sessionId];
}

#pragma mark - Class methods

+ (void)initialize
{
    @synchronized(self) {
        if (s_sharedInstance == nil) {
            s_sharedInstance = [[PingService alloc] init];
        }
    }
}

+ (PingService*)sharedInstance
{
    @synchronized(self) {
        return s_sharedInstance;
    }
}

@end
