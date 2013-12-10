////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#import "AJNServiceController.h"

@interface AJNServiceController() <AJNBusListener, AJNSessionListener, AJNSessionPortListener>

@property (nonatomic) AJNSessionId sessionId;

- (void)sendStatusMessage:(NSString *)message;

@end

@implementation AJNServiceController

@synthesize bus = _bus;
@synthesize delegate = _delegate;
@synthesize sessionId = _sessionId;

@synthesize allowRemoteMessages = _allowRemoteMessages;
@synthesize trafficType = _trafficType;
@synthesize proximityOptions = _proximityOptions;
@synthesize transportMask = _transportMask;
@synthesize multiPointSessionsEnabled = _multiPointSessionsEnabled;

@synthesize connectionArguments = _connectionArguments;

- (id)init
{
    self = [super init];

    if (self) {
        // set reasonable defaults for the service
        //
        self.allowRemoteMessages = YES;
        self.trafficType = kAJNTrafficMessages;
        self.proximityOptions = kAJNProximityAny;
        self.transportMask = kAJNTransportMaskAny;
        self.multiPointSessionsEnabled = YES;
        self.connectionArguments = @"null:";
    }
    
    return self;
}

- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment
{
    self = [super init];
    
    if (self) {
        // set reasonable defaults for the service
        //
        self.allowRemoteMessages = YES;
        self.trafficType = kAJNTrafficMessages;
        self.proximityOptions = kAJNProximityAny;
        self.transportMask = kAJNTransportMaskAny;
        self.multiPointSessionsEnabled = YES;
        self.connectionArguments = @"null:";
        
        self.bus = busAttachment;
    }
    
    return self;
}

- (void)sendStatusMessage:(NSString *)message
{
    if ([self.delegate respondsToSelector:@selector(didReceiveStatusMessage:)]) {
        [self.delegate didReceiveStatusMessage:message];
    }
}

- (void)start
{
    QStatus status = ER_OK;
    
    @try {
        
        if (self.bus == nil) {
            // allocate and initalize the bus
            //
            self.bus = [[AJNBusAttachment alloc] initWithApplicationName:self.delegate.applicationName allowRemoteMessages:self.allowRemoteMessages];
        }
        
        if (!self.bus.isStarted) {
            // start the bus
            //
            status = [self.bus start];
            if (status != ER_OK) {
                [self sendStatusMessage:[NSString stringWithFormat:@"Bus failed to start. %@",[AJNStatus descriptionForStatusCode:status]]];
                @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Unable to start bus" userInfo:nil];
            }
            [self sendStatusMessage:@"Bus started successfully."];

            // notify the delegate that the bus started
            //
            if ([self.delegate respondsToSelector:@selector(didStartBus:)]) {
                [self.delegate didStartBus:self.bus];
            }
        }
        
        
        // inform the delegate that it is time to create the bus object
        //
        if (![self.delegate objectOnBus:self.bus]) {
            [self sendStatusMessage:@"Failed to create bus object."];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Failed to create bus object" userInfo:nil];
            
        }

        // register the bus object with the bus
        //
        status = [self.bus registerBusObject:self.delegate.object];
        if (status != ER_OK) {
            [self sendStatusMessage:[NSString stringWithFormat:@"Bus object failed to register. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Unable to register bus object" userInfo:nil];
        }
        
        [self sendStatusMessage:@"Bus object registered successfully."];
        
        if (!self.bus.isConnected) {
            // connect the bus using the arguments specified
            //
            status = [self.bus connectWithArguments:self.connectionArguments];
            if (status != ER_OK) {
                [self sendStatusMessage:[NSString stringWithFormat:@"Bus failed to connect. %@",[AJNStatus descriptionForStatusCode:status]]];
                @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Unable to connect to bus" userInfo:nil];
            }
            [self sendStatusMessage:@"Bus connected successfully."];
            
            // notify the delegate that the bus connected
            //
            if ([self.delegate respondsToSelector:@selector(didConnectBus:)]) {
                [self.delegate didConnectBus:self.bus];
            }
        }

        // register the service controller as a bus listener, which allows us to find
        // well known names that are advertised on the bus, among other tasks.
        //
        [self.bus registerBusListener:self];
        [self sendStatusMessage:@"Bus listener registered successfully."];
        
        // create the session options for this service's connections with its clients
        //
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:self.trafficType supportsMultipoint:self.multiPointSessionsEnabled proximity:self.proximityOptions transportMask:self.transportMask];
        
        // request to become the owner of the service name
        //
        status = [self.bus requestWellKnownName:self.delegate.serviceName withFlags:self.delegate.serviceNameFlags];
        
        if (status != ER_OK) {
            [self sendStatusMessage:[NSString stringWithFormat:@"Failed to gain ownership of service name. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Request for well-known name failed" userInfo:nil];
        }
        
        [self sendStatusMessage:[NSString stringWithFormat:@"Request for service name [%@] succeeded.", self.delegate.serviceName]];
        
        // bind the session to a specific service port
        //
        status = [self.bus bindSessionOnPort:self.delegate.sessionPort withOptions:sessionOptions withDelegate:self];
        if (status != ER_OK) {
            [self sendStatusMessage:[NSString stringWithFormat:@"Failed to bind session on port. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Unable to bind session to port" userInfo:nil];
        }
        [self sendStatusMessage:@"Session to port binding successfully."];
        
        // let others on the bus know that this service exists
        //
        status = [self.bus advertiseName:self.delegate.serviceName withTransportMask:sessionOptions.transports];
        
        if (status != ER_OK) {
            [self sendStatusMessage:[NSString stringWithFormat:@"Failed to advertise service name [%@]. %@", self.delegate.serviceName, [AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"start Failed" reason:@"Unable to advertise service name" userInfo:nil];
        }
        
        [self sendStatusMessage:[NSString stringWithFormat:@"Now advertising name [%@]. Waiting for clients to join session...", self.delegate.serviceName]];
    }
    @catch (NSException *exception) {
        [self sendStatusMessage:@"ERROR: Exception thrown in ServiceController::start."];
        // rethrow the exception
        //
        @throw exception;
    }    
}

- (void)stop
{
    QStatus status = ER_OK;
    
    // cancel name advertisement
    //
    [self.bus cancelAdvertisedName:self.delegate.serviceName withTransportMask:self.transportMask];
    
    // unregister as a listener
    //
    [self.bus unregisterBusListener:self];
    [self sendStatusMessage:@"Bus listener registered successfully."];
    
    // disconnect the bus
    //
    status = [self.bus disconnectWithArguments:@"null:"];
    if (status != ER_OK) {
        [self sendStatusMessage:[NSString stringWithFormat:@"Failed to disconnect from bus. %@",[AJNStatus descriptionForStatusCode:status]]];
        @throw [NSException exceptionWithName:@"AJNServiceController::stop: Failed" reason:@"Failed to disconnect from bus" userInfo:nil];
    }
    [self sendStatusMessage:@"Bus disconnected successfully."];
    
        
    // stop the bus
    //
    status = [self.bus stop];
    if (status != ER_OK) {
        [self sendStatusMessage:[NSString stringWithFormat:@"Failed to stop the bus. %@",[AJNStatus descriptionForStatusCode:status]]];
        @throw [NSException exceptionWithName:@"AJNServiceController::stop: Failed" reason:@"Failed to stop the bus" userInfo:nil];
    }
    
    [self.bus waitUntilStopCompleted];
    
    [self sendStatusMessage:@"Bus stopped successfully."];
    
    // let our service delegate handle the stop event
    //
    [self.delegate shouldUnloadObjectOnBus:self.bus];
    
    // destroy the listener
    //
    [self.bus destroyBusListener:self];
    [self sendStatusMessage:@"Destroyed bus listener."];
    
    // deallocate the bus
    //
    self.bus = nil;
    [self sendStatusMessage:@"Deallocated bus attachment."];
    
    // zero out the session id
    //
    self.sessionId = 0;
}


#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString *)namePrefix
{
    NSLog(@"AJNServiceController::didFindAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix);
    [self sendStatusMessage:[NSString stringWithFormat:@"Discovered advertised name [%@] on the bus.", name]];
    if ([self.delegate respondsToSelector:@selector(didFindAdvertisedName:withTransportMask:namePrefix:)]) {
        [self.delegate didFindAdvertisedName:name withTransportMask:transport namePrefix:namePrefix];
    }
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSString *message = [NSString stringWithFormat:@"AJNServiceController::didLoseAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(didLoseAdvertisedName:withTransportMask:namePrefix:)]) {
        [self.delegate didLoseAdvertisedName:name withTransportMask:transport namePrefix:namePrefix];
    }
}

- (void)nameOwnerChanged:(NSString *)name to:(NSString *)newOwner from:(NSString *)previousOwner
{
    NSString *message = [NSString stringWithFormat:@"AJNServiceController::nameOwnerChanged:%@ to:%@ namePrefix:%@", name, newOwner, previousOwner];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(nameOwnerChanged:to:from:)]) {
        [self.delegate nameOwnerChanged:name to:newOwner from:previousOwner];
    }
}

- (void)busWillStop
{
    // inform the service delegate that the bus will stop
    //
    if ([self.delegate respondsToSelector:@selector(busWillStop)]) {
        [self.delegate busWillStop];
    }
}

- (void)busDidDisconnect
{
    if ([self.delegate respondsToSelector:@selector(busDidDisconnect)]) {
        [self.delegate busDidDisconnect];
    }
}

#pragma mark - AJNSessionPortListener delegate methods

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNServiceController::didJoin:%@ inSessionWithId:%u onSessionPort:%u", joiner, sessionId, sessionPort);

    self.sessionId = sessionId;
    
    if ([self.delegate respondsToSelector:@selector(didJoin:inSessionWithId:onSessionPort:)]) {
        [self.delegate didJoin:joiner inSessionWithId:sessionId onSessionPort:sessionPort];
    }

}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    NSLog(@"AJNServiceController::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u", joiner, sessionPort);
    
    [self sendStatusMessage:[NSString stringWithFormat:@"Received join request from %@ on port %d.", joiner, sessionPort]];
    
    BOOL shouldAllowClientToJoinSession = NO;
    
    if ([self.delegate respondsToSelector:@selector(shouldAcceptSessionJoinerNamed:onSessionPort:withSessionOptions:)]) {

        shouldAllowClientToJoinSession = [self.delegate shouldAcceptSessionJoinerNamed:joiner onSessionPort:sessionPort withSessionOptions:options];

    }
    else {
    
        shouldAllowClientToJoinSession = sessionPort == self.delegate.sessionPort;
        
        if (shouldAllowClientToJoinSession) {
            [self sendStatusMessage:[NSString stringWithFormat:@"Accepted join request from %@ on port %d.", joiner, sessionPort]];
        }
    
    }
    
    // only allow session joiners who pass our criteria
    //
    return shouldAllowClientToJoinSession;
}

#pragma mark - AJNSessionListener delegate methods

- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSString *message = [NSString stringWithFormat:@"AJNServiceController::sessionWasLost:%u forReason:%u", sessionId, reason];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(sessionWasLost:forReason:)]) {
        [self.delegate sessionWasLost:sessionId];
    }
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"AJNServiceController::didAddMemberNamed:%@ toSession:%u", memberName, sessionId];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(didAddMemberNamed:toSession:)]) {
        [self.delegate didAddMemberNamed:memberName toSession:sessionId];
    }
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"AJNServiceController::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(didRemoveMemberNamed:fromSession:)]) {
        [self.delegate didRemoveMemberNamed:memberName fromSession:sessionId];
    }
}

@end
