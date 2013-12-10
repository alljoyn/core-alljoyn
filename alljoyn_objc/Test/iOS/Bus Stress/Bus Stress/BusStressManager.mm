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

#import "BusStressManager.h"
#import "AJNBusAttachment.h"
#import "AJNBusObject.h"
#import "AJNBusObjectImpl.h"
#import "AJNBasicObject.h"
#import "BasicObject.h"

static BOOL s_stopStressFlag;

@interface AJNBusAttachment(Private)

- (ajn::BusAttachment*)busAttachment;

@end

@interface StressOperation : NSOperation<BasicStringsDelegateSignalHandler>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) BasicObject *busObject;
@property (nonatomic) BOOL shouldDeleteBusAttachment;
@property (nonatomic) AJNTransportMask transportMask;

- (void)setup;
- (void)tearDown;

@end

@interface ClientStressOperation : StressOperation<AJNBusListener, AJNSessionListener>

@property (nonatomic, strong) NSCondition *joinedSessionCondition;
@property (nonatomic) AJNSessionId sessionId;
@property (nonatomic, strong) NSString *foundServiceName;
@property (nonatomic, strong) BasicObjectProxy *basicObjectProxy;
@property BOOL wasNameAlreadyFound;

@end

@interface ServiceStressOperation : StressOperation<AJNBusListener, AJNSessionListener, AJNSessionPortListener>

@property (nonatomic, strong) NSCondition *lostSessionCondition;

@end

@implementation ClientStressOperation

@synthesize joinedSessionCondition = _joinedSessionCondition;
@synthesize sessionId = _sessionId;
@synthesize foundServiceName = _foundServiceName;
@synthesize basicObjectProxy = _basicObjectProxy;
@synthesize wasNameAlreadyFound = _wasNameAlreadyFound;

- (void)main
{
    [self setup];
    
    self.joinedSessionCondition = [[NSCondition alloc] init];
    [self.joinedSessionCondition lock];
    
    [self.bus registerBusListener:self];
    
    [self.bus findAdvertisedName:@"org.alljoyn.bus.sample.strings"];
    
    if ([self.joinedSessionCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:rand()%5 + 5]]) {
        
        self.basicObjectProxy = [[BasicObjectProxy alloc] initWithBusAttachment:self.bus serviceName:self.foundServiceName objectPath:@"/basic_object" sessionId:self.sessionId];
        
        [self.basicObjectProxy introspectRemoteObject];
        
        NSString *result = [self.basicObjectProxy concatenateString:@"Code " withString:@"Monkies!!!!!!!"];
        
        if (result) {
            NSLog(@"[%@] %@ concatenated string.", result, [result compare:@"Code Monkies!!!!!!!"] == NSOrderedSame ? @"Successfully":@"Unsuccessfully");
        }
        
        self.basicObjectProxy = nil;
        
    }
    if (self.sessionId) {
        [self.bus leaveSession:self.sessionId];
    }

    [self.joinedSessionCondition unlock];
    
    [self tearDown];
}

#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::didFindAdvertisedName:%@ withTransportMask:%u namePrefix:%@", name, transport, namePrefix);
    
    if ([namePrefix compare:@"org.alljoyn.bus.sample.strings"] == NSOrderedSame) {
        
        BOOL shouldReturn;
        @synchronized(self) {
            shouldReturn = self.wasNameAlreadyFound;
            self.wasNameAlreadyFound = true;
        }
        
        if (shouldReturn) {
            NSLog(@"Already found an advertised name, ignoring this name %@...", name);
            return;
        }
        
        [self.bus enableConcurrentCallbacks];
        
        self.sessionId = [self.bus joinSessionWithName:name onPort:999 withDelegate:self options:[[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:self.transportMask]];
        self.foundServiceName = name;
        
        NSLog(@"Client joined session %d", self.sessionId);
        
        [self.joinedSessionCondition signal];
    }
}


@end

@implementation ServiceStressOperation

@synthesize lostSessionCondition = _lostSessionCondition;

- (void)main
{
    [self setup];
    
    self.lostSessionCondition = [[NSCondition alloc] init];
    [self.lostSessionCondition lock];
    
    [self.bus registerBusListener:self];
    
    NSString *path = @"/basic_object";
    self.busObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:path];
    
    [self.bus registerBusObject:self.busObject];

    NSString *name = [NSString stringWithFormat:@"org.alljoyn.bus.sample.strings.i%d", rand()];
    QStatus status = [self.bus requestWellKnownName:name withFlags:kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue];
    if (ER_OK != status) {
        NSLog(@"Request for name failed (%@)", name);
    }
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:self.transportMask];
    
    status = [self.bus bindSessionOnPort:999 withOptions:sessionOptions withDelegate:self];
    
    status = [self.bus advertiseName:name withTransportMask:self.transportMask];
    if (ER_OK != status) {
        NSLog(@"Could not advertise (%@)", name);
    }

    [self.lostSessionCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:rand()%5 + 10]];
    
    [self.bus unregisterBusObject:self.busObject];
    
    [self.lostSessionCondition unlock];
    
    [self tearDown];
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

- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSLog(@"AJNBusListener::sessionWasLost %u forReason %u", sessionId, reason);

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
    if (sessionPort == 999) {
        return YES;
    }
    return NO;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
    
}

@end

@implementation StressOperation

@synthesize bus = _bus;
@synthesize shouldDeleteBusAttachment = _shouldDeleteBusAttachment;
@synthesize handle = _handle;

- (void)setup
{
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSString *name = [NSString stringWithFormat:@"bastress%d", rand()];
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:name allowRemoteMessages:YES];
    QStatus status =  [self.bus start];
    if (ER_OK != status) {
        NSLog(@"Bus start failed.");
    }
    status = [self.bus connectWithArguments:@"null:"];
    if (ER_OK != status) {
        NSLog(@"Bus connect failed.");
    }
}

- (void)tearDown
{
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    self.busObject = nil;
    
    if (self.shouldDeleteBusAttachment) {
        self.bus = nil;
    }    
}

- (void)main
{
    [self setup];
    
    NSString *name = [NSString stringWithFormat:@"org.alljoyn.test.bastress.stressrun%d", rand()];
    QStatus status = [self.bus requestWellKnownName:name withFlags:kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue];
    if (ER_OK != status) {
        NSLog(@"Request for name failed (%@)", name);
    }
    
    status = [self.bus advertiseName:name withTransportMask:self.transportMask];
    if (ER_OK != status) {
        NSLog(@"Could not advertise (%@)", name);
    }
    
    NSString *path = @"/org/cool";
    self.busObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:path];
    
    [self.bus registerBusObject:self.busObject];
    
    [self.bus registerBasicStringsDelegateSignalHandler:self];
    
    [self.bus unregisterSignalHandler:self];
    
    [self.bus unregisterBusObject:self.busObject];
    
    [self tearDown];
}

#pragma mark - BasicStringsDelegateSignalHandler implementation

- (void)didReceiveTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId fromSender:(NSString *)sender
{
    NSLog(@"BasicStringsDelegateSignalHandler::didReceiveTestStringPropertyChangedFrom:%@ to:%@ inSession:%u fromSender:%@", oldString, newString, sessionId, sender);
}

- (void)didReceiveTestSignalWithNoArgsInSession:(AJNSessionId)sessionId fromSender:(NSString*)sender
{
    NSLog(@"BasicStringsDelegateSignalHandler::didReceiveTestSignalWithNoArgsInSession:%u fromSender:%@", sessionId, sender);
}

@end

@implementation BusStressManager

+ (void)runStress:(NSInteger)iterations threadCount:(NSInteger)threadCount deleteBusFlag:(BOOL)shouldDeleteBusAttachment stopThreadsFlag:(BOOL)stopThreads operationMode:(BusStressManagerOperationMode)mode delegate:(id<BusStressManagerDelegate>)delegate
{
    dispatch_queue_t queue = dispatch_queue_create("bastress", NULL);
    dispatch_async(queue, ^{

        [delegate didCompleteIteration:0 totalIterations:iterations];
        for (NSInteger i = 0; i < iterations; i++) {
            
            @synchronized(self) {
                if (s_stopStressFlag) {
                    s_stopStressFlag = NO;
                    break;
                }
            }
            NSOperationQueue *queue = [[NSOperationQueue alloc] init];
            
            [queue setMaxConcurrentOperationCount:threadCount];
            
            for (NSInteger i = 0; i < threadCount; i++) {
                StressOperation *stressOperation;
                if (mode == kBusStressManagerOperationModeNone) {
                    stressOperation = [[StressOperation alloc] init];
                }
                else if (mode == kBusStressManagerOperationModeClient) {
                    stressOperation = [[ClientStressOperation alloc] init];
                }
                else if (mode == kBusStressManagerOperationModeService) {
                    stressOperation = [[ServiceStressOperation alloc] init];
                }
                stressOperation.shouldDeleteBusAttachment = shouldDeleteBusAttachment;
                stressOperation.transportMask = delegate.transportMask;
                
                [queue addOperation:stressOperation];
            }
            
            
            
            if (stopThreads) {
                sleep(rand() % 2);
                [queue cancelAllOperations];
            }
            
            [queue waitUntilAllOperationsAreFinished];
            
            queue = nil;
            
            NSLog(@"Threads completed for iteration %d", i);
            
            [delegate didCompleteIteration:i totalIterations:iterations];
        }

    });
}

+ (void)stopStress
{
    @synchronized(self) {
        s_stopStressFlag = YES;
    }
}

@end
