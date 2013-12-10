////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#import "SampleClient.h"
#import "AJNBusAttachment.h"
#import "AJNSampleObject.h"
#import "AJNSessionOptions.h"

static SampleClient *s_sharedInstance;

@interface SampleClient() <AJNBusListener, AJNSessionListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) SampleObjectProxy *sampleObjectProxy;
@property (nonatomic, weak) id<SampleClientDelegate> delegate;

@end

@implementation SampleClient

@synthesize bus = _bus;
@synthesize sampleObjectProxy = _sampleObjectProxy;
@synthesize delegate = _delegate;

- (id)initWithDelegate:(id<SampleClientDelegate>)delegate
{
    self = [super init];
    if (self) {
        self.delegate = delegate;
    }
    return self;
}

- (void)start
{
    // NOTE: no error handling is in place here to keep the code easy to read.
    // you should add error checking for each of these calls
    //
    
    // allocate and initalize the bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:kAppName allowRemoteMessages:YES];
    
    // start the bus
    //
    [self.bus start];
    
    // connect the bus using the null transport
    //
    [self.bus connectWithArguments:@"null:"];
    
    // register the SampleClient object as a bus listener, which allows us to find
    // well known names that are advertised on the bus.
    //
    [self.bus registerBusListener:self];
    
    // begin searching for the name of the service on the bus that we 
    // are interested in connecting with. Services let clients know of their existence
    // by advertising a well-known name.
    //
    [self.bus findAdvertisedName:kServiceName];
    
}

- (void)stop
{
    // unregister as a listener
    //
    [self.bus unregisterBusListener:self];
    
    // disconnect the bus
    //
    [self.bus disconnectWithArguments:@"null:"];
    
    // stop the bus
    //
    [self.bus stop];
    
    // let our proxy object deallocate
    //
    self.sampleObjectProxy = nil;
    
    // destroy the listener
    //
    [self.bus destroyBusListener:self];
    
    // deallocate the bus
    //
    self.bus = nil;    
}

- (NSString*)callServiceToConcatenateString:(NSString*)str1 withString:(NSString*)str2
{
    // call the service via a proxy object we created to concatenate the strings together
    //
    return [self.sampleObjectProxy concatenateString:str1 withString:str2];
}

#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString *)namePrefix
{
    NSLog(@"SampleClient::didFindAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix);
    
    if ([name compare:kServiceName] == NSOrderedSame) {
        
        // enable concurrency within the handler
        //
        [self.bus enableConcurrentCallbacks];
        
        // we found the name of the service we are looking for, so attempt to
        // connect to the service and establish a session
        //
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
        
        AJNSessionId sessionId = [self.bus joinSessionWithName:name onPort:kServicePort withDelegate:self options:sessionOptions];
        
        if (sessionId != -1) {
            // once we successfully join the session with service, create a proxy bus
            // object that we can use to communicate with the service.
            //
            self.sampleObjectProxy = [[SampleObjectProxy alloc] initWithBusAttachment:self.bus serviceName:kServiceName objectPath:kServicePath sessionId:sessionId];
            
            // next we need to dynamically get the interface descriptions for the remote
            // object we created above. Introspect the object to determine what
            // interfaces it supports and what members are included in those 
            // interfaces.
            // 
            [self.sampleObjectProxy introspectRemoteObject];
            
            // let our delegate know that we connected to the service
            //
            if ([self.delegate respondsToSelector:@selector(didConnectWithService)]) {
                [self.delegate didConnectWithService];
            }
            
        }
    }
    
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"SampleClient::didLoseAdvertisedName:%@ withTransportMask:%i namePrefix:%@", name, transport, namePrefix);
    
}

- (void)nameOwnerChanged:(NSString *)name to:(NSString *)newOwner from:(NSString *)previousOwner
{
    NSLog(@"SampleClient::nameOwnerChanged:%@ to:%@ namePrefix:%@", name, newOwner, previousOwner);    
}

#pragma mark - AJNSessionListener delegate methods

- (void)sessionWasLost:(AJNSessionId)sessionId
{
    NSLog(@"SampleClient::sessionWasLost:%u", sessionId);            
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSLog(@"SampleClient::didAddMemberNamed:%@ toSession:%u", memberName, sessionId);  
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSLog(@"SampleClient::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId);                    
}

#pragma mark - Class methods

+ (void)initialize
{
    @synchronized(self) {
        if (s_sharedInstance == nil) {
            s_sharedInstance = [[SampleClient alloc] init];
        }
    }
}

+ (SampleClient*)sharedInstance
{
    @synchronized(self) {
        return s_sharedInstance;
    }
}

@end
