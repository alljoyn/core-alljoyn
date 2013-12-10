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

#import "AJNClientController.h"
#import "AJNProxyBusObject.h"

@interface AJNClientController()

@property (nonatomic) AJNSessionId sessionId;

- (void)sendStatusMessage:(NSString *)message;

@end

@implementation AJNClientController

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
        //
        // set reasonable defaults for the client
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
        //
        // set reasonable defaults for the client
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
        
        if (!self.bus.isConnected) {
            
            // connect the bus using the specified transport
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
        
        // register the client controller object as a bus listener, which allows us to find
        // well known names that are advertised on the bus.
        //
        [self.bus registerBusListener:self];
        [self sendStatusMessage:@"Bus listener registered successfully."];
        
        // begin searching for the name of the service on the bus that we
        // are interested in connecting with. Services let clients know of their existence
        // by advertising a well-known name.
        //
        status = [self.bus findAdvertisedName:self.delegate.serviceName];
        if (status != ER_OK) {
            [self sendStatusMessage:[NSString stringWithFormat:@"Failed to find advertised name. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Failed in call to find advertised name" userInfo:nil];
        }
        [self sendStatusMessage:[NSString stringWithFormat:@"Searching for name %@ on the bus...", self.delegate.serviceName]];
    }
    @catch (NSException *exception) {
        [self sendStatusMessage:@"ERROR: Exception thrown in AJNClientController::start."];
    }    
}

- (void)stop
{
    QStatus status = ER_OK;
    
    // unregister as a listener
    //
    [self.bus unregisterBusListener:self];
    [self sendStatusMessage:@"Bus listener unregistered successfully."];
    
    // disconnect the bus
    //
    status = [self.bus disconnectWithArguments:@"null:"];
    if (status != ER_OK) {
        [self sendStatusMessage:[NSString stringWithFormat:@"Failed to disconnect from bus. %@",[AJNStatus descriptionForStatusCode:status]]];
        @throw [NSException exceptionWithName:@"AJNClientController::stop: Failed" reason:@"Failed to disconnect from bus" userInfo:nil];
    }
    [self sendStatusMessage:@"Bus disconnected successfully."];
    
    // stop the bus
    //
    status = [self.bus stop];
    if (status != ER_OK) {
        [self sendStatusMessage:[NSString stringWithFormat:@"Failed to stop the bus. %@",[AJNStatus descriptionForStatusCode:status]]];
        @throw [NSException exceptionWithName:@"AJNClientController::stop: Failed" reason:@"Failed to stop the bus" userInfo:nil];
    }
    [self.bus waitUntilStopCompleted];
    [self sendStatusMessage:@"Bus stopped successfully."];
    
    // let our proxy object deallocate
    //
    [self.delegate shouldUnloadProxyObjectOnBus:self.bus];
    [self sendStatusMessage:@"Deallocated proxy object."];
    
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
    NSLog(@"AJNClientController::didFindAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix);
    [self sendStatusMessage:[NSString stringWithFormat:@"Discovered advertised name [%@] on the bus.", name]];
    
    if ([self.delegate respondsToSelector:@selector(didFindAdvertisedName:withTransportMask:namePrefix:)]) {
        [self.delegate didFindAdvertisedName:name withTransportMask:transport namePrefix:namePrefix];
    }
    else {
    
        if ([name compare:self.delegate.serviceName] == NSOrderedSame) {
            
            // enable concurrency within the handler
            //
            [self.bus enableConcurrentCallbacks];
            
            // we found the name of the service we are looking for, so attempt to
            // connect to the service and establish a session
            //
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:self.trafficType supportsMultipoint:self.multiPointSessionsEnabled proximity:self.proximityOptions transportMask:self.transportMask];
            
            AJNSessionId sessionId = [self.bus joinSessionWithName:name onPort:self.delegate.sessionPort withDelegate:self options:sessionOptions];
            
            if (sessionId > 0 && self.sessionId == 0) {
                
                self.sessionId = sessionId;
                
                [self sendStatusMessage:[NSString stringWithFormat:@"Successfully joined session [%u].", sessionId]];
                
                // let our delegate know that we connected to the service
                //
                
                AJNProxyBusObject *proxyBusObject = [self.delegate proxyObjectOnBus:self.bus inSession:sessionId];
                QStatus status = [proxyBusObject introspectRemoteObject];
                if (status != ER_OK) {
                    NSString *message = [NSString stringWithFormat:@"ERROR: Failed to introspect remote object in AJNClientController. %@", [AJNStatus descriptionForStatusCode:status]];
                    NSLog(@"%@", message);
                    [self sendStatusMessage:message];
                }
                
                if ([self.delegate respondsToSelector:@selector(didJoinInSession:withService:)]) {
                    [self.delegate didJoinInSession:sessionId withService:name];
                }
            }
        }
    }
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSString *message = [NSString stringWithFormat:@"AJNClientController::didLoseAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(didLoseAdvertisedName:withTransportMask:namePrefix:)]) {
        [self.delegate didLoseAdvertisedName:name withTransportMask:transport namePrefix:namePrefix];
    }
}

- (void)nameOwnerChanged:(NSString *)name to:(NSString *)newOwner from:(NSString *)previousOwner
{
    NSString *message = [NSString stringWithFormat:@"AJNClientController::nameOwnerChanged:%@ to:%@ namePrefix:%@", name, newOwner, previousOwner];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(nameOwnerChanged:to:from:)]) {
        [self.delegate nameOwnerChanged:name to:newOwner from:previousOwner];
    }
}

- (void)busWillStop
{
    NSString *message = @"The bus will stop...";
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(busWillStop)]) {
        [self.delegate busWillStop];
    }
}

- (void)busDidDisconnect
{
    NSString *message = @"The bus disconnected.";
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(busDidDisconnect)]) {
        [self.delegate busDidDisconnect];
    }
}


#pragma mark - AJNSessionListener delegate methods

- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSString *message = [NSString stringWithFormat:@"AJNClientController::sessionWasLost:%u forReason:%u", sessionId, reason];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(sessionWasLost:forReason:)]) {
        [self.delegate sessionWasLost:sessionId];
    }
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"AJNClientController::didAddMemberNamed:%@ toSession:%u", memberName, sessionId];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(didAddMemberNamed:toSession:)]  ) {
        [self.delegate didAddMemberNamed:memberName toSession:sessionId];
    }
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSString *message = [NSString stringWithFormat:@"AJNClientController::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId];
    NSLog(@"%@", message);
    [self sendStatusMessage:message];
    if ([self.delegate respondsToSelector:@selector(didRemoveMemberNamed:fromSession:)]) {
        [self.delegate didRemoveMemberNamed:memberName fromSession:sessionId];
    }
}


@end
