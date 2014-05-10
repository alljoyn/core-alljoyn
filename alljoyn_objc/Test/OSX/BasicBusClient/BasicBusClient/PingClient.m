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

#import "Constants.h"
#import "PingClient.h"
#import "AJNBusAttachment.h"
#import "AJNPingObject.h"
#import "AJNSessionOptions.h"

static PingClient *s_sharedInstance;

@interface PingClient() <AJNBusListener, AJNSessionListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) PingObjectProxy *pingObjectProxy;
@property (nonatomic, strong) NSString *serviceName;
@end

@implementation PingClient

@synthesize bus = _bus;
@synthesize pingObjectProxy = _pingObjectProxy;
@synthesize delegate = _delegate;
@synthesize serviceName = _serviceName;

- (id)initWithDelegate:(id<PingClientDelegate>)delegate
{
    self = [super init];
    if (self) {
        self.delegate = delegate;
    }
    return self;
}

- (void)connectToService:(NSString *)serviceName
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
        
        // connect the bus using the null transport
        //
        status = [self.bus connectWithArguments:@"null:"];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Bus failed to connect. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Unable to connect to bus" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:@"Bus connected successfully."];
        
        // register the PingClient object as a bus listener, which allows us to find
        // well known names that are advertised on the bus.
        //
        [self.bus registerBusListener:self];
        [self.delegate receivedStatusMessage:@"Bus listener registered successfully."];
        
        // begin searching for the name of the service on the bus that we
        // are interested in connecting with. Services let clients know of their existence
        // by advertising a well-known name.
        //
        status = [self.bus findAdvertisedName:serviceName];
        if (status != ER_OK) {
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to find advertised name. %@",[AJNStatus descriptionForStatusCode:status]]];
            @throw [NSException exceptionWithName:@"connectToService Failed" reason:@"Failed in call to find advertised name" userInfo:nil];
        }
        [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Searching for name %@ on the bus...", serviceName]];
    }
    @catch (NSException *exception) {
        [self.delegate receivedStatusMessage:@"ERROR: Exception thrown in PingClient::connectToService:."];
    }
}

- (void)disconnect
{
    QStatus status = ER_OK;
    
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
    
    // let our proxy object deallocate
    //
    self.pingObjectProxy = nil;
    [self.delegate receivedStatusMessage:@"Deallocated proxy object."];
    
    // destroy the listener
    //
    [self.bus destroyBusListener:self];
    [self.delegate receivedStatusMessage:@"Destroyed bus listener."];
    
    // deallocate the bus
    //
    self.bus = nil;    
    [self.delegate receivedStatusMessage:@"Deallocated bus attachment."];
}

- (NSString *)sendPingToService:(NSString *)str1
{
    // call the service via a proxy object we created to concatenate the strings together
    //
    [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Sending ping string [%@] to service.", str1]];
    NSString *receivedString = [self.pingObjectProxy sendPingString:str1];
    [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Received ping string [%@] back from service.", receivedString]];
    return receivedString;
}

#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString *)namePrefix
{
    NSLog(@"PingClient::didFindAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix);
    [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Discovered advertised name [%@] on the bus.", name]];
    if ([name compare:self.serviceName] == NSOrderedSame) {
        
        // enable concurrency within the handler
        //
        [self.bus enableConcurrentCallbacks];
        
        // we found the name of the service we are looking for, so attempt to
        // connect to the service and establish a session
        //
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:self.delegate.transportType];
        
        NSString *message = [NSString stringWithFormat:@"Attempting to join session with service %@ using transport %@...", name, @"Any"];
        
        NSLog(@"%@", message);
        [self.delegate receivedStatusMessage:message];
        
        AJNSessionId sessionId = [self.bus joinSessionWithName:name onPort:kServicePort withDelegate:self options:sessionOptions];
        
        if (sessionId > 0) {
            // let our delegate know that we connected to the service
            //
            if ([self.delegate respondsToSelector:@selector(didConnectWithService)]) {
                [self.delegate didConnectWithService:name];
            }
            
            [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Successfully joined session [%u].", sessionId]];
            
            // once we successfully join the session with service, create a proxy bus
            // object that we can use to communicate with the service.
            //
            self.pingObjectProxy = [[PingObjectProxy alloc] initWithBusAttachment:self.bus serviceName:self.serviceName objectPath:kServicePath sessionId:sessionId];
            [self.delegate receivedStatusMessage:@"Allocated Ping object proxy."];

            
            // next we need to dynamically get the interface descriptions for the remote
            // object we created above. Introspect the object to determine what
            // interfaces it supports and what members are included in those 
            // interfaces.
            // 
            QStatus status = [self.pingObjectProxy introspectRemoteObject];
            if (status != ER_OK) {
                [self.delegate receivedStatusMessage:[NSString stringWithFormat:@"Failed to introspect remote ping object. %@", [AJNStatus descriptionForStatusCode:status]]];
            }
            
            // now send a ping string
            //
            [self sendPingToService:@"Ping String 1"];
         
            // we are done so let the client know it should do a disconnect
            //
            [self.delegate shouldDisconnectFromService:name];
        }
        else {
            [self.delegate receivedStatusMessage:@"ERROR: Failed to join session with service."];
            NSLog(@"%@", @"ERROR: Failed to join session with service.");
        }
    }
    
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
}

#pragma mark - Class methods

+ (void)initialize
{
    @synchronized(self) {
        if (s_sharedInstance == nil) {
            s_sharedInstance = [[PingClient alloc] init];
        }
    }
}

+ (PingClient*)sharedInstance
{
    @synchronized(self) {
        return s_sharedInstance;
    }
}

@end
