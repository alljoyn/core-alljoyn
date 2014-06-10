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

#include <Status.h>
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNVersion.h"
#import "BasicService.h"
#import "BasicSampleObject.h"

static BasicService *s_basicService;

@interface ObjectTranslator : NSObject<AJNTranslator>

- (size_t)numTargetLanguages;
- (NSString*)getTargetLanguage:(size_t)index;
- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang;

@end

@implementation ObjectTranslator
- (size_t)numTargetLanguages
{
    return 1;
}

- (NSString*)getTargetLanguage:(size_t)index
{
    return @"en";
}

- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang
{
    if(![toLang isEqualToString:(@"en")])
    {
        return nil;
    }
    if([text isEqualToString:@"Isthay siay naay jectobay"])
    {
        return @"This is an object";
    }
    
    return nil;
}

@end

@interface BasicService() <AJNBusListener,AJNSessionPortListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) MyBasicSampleObject *basicSampleObject;

- (void)startService;

- (void)ownershipOfName:(NSString *)busName didChangeFromPreviousOwner:(NSString *)previousOwner toNewOwner:(NSString *)newOwner;

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options;

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort;

@end

@implementation BasicService

@synthesize delegate = _delegate;
@synthesize bus = _bus;
@synthesize basicSampleObject = _basicSampleObject;

+ (BasicService*)sharedInstance 
{
    @synchronized(self) {
        if (s_basicService == nil) {
            s_basicService = [[BasicService alloc] init];
        }
    }
    return s_basicService;
}

- (void)run
{
    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.basic-service.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self startService];
    });
    dispatch_release(serviceQueue);
}

- (void)startService
{    
    @try 
    {
        QStatus status;
        
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
        
        // output the version of alljoyn being consumed
        //
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library version: %@\n", [AJNVersion versionInformation]]];
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library build info: %@\n", [AJNVersion buildInformation]]];
        
        // allocate and initialize the bus attachment
        //
        self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"myApp" allowRemoteMessages:YES];        
        
        // register as a bus listener
        //
        [self.bus registerBusListener:self];
        
        // create our custom sample bus object that will concatenate two strings
        // and return the result of the concatenation
        //
        self.basicSampleObject = [[MyBasicSampleObject alloc] initWithBusAttachment:self.bus onPath:kBasicObjectServicePath];
        
        
        
        [self.basicSampleObject setDescription:@"Isthay siay naay jectobay" inLanguage:@"pig"];
        [self.basicSampleObject setDescriptionTranslator:[ObjectTranslator alloc]];
        

        
        // this is the delegate used for displaying status messages in the UI
        //
        self.basicSampleObject.delegate = self.delegate;

        // start the bus
        //
        status = [self.bus start];
        
        if (status != ER_OK) {
            [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Start failed\n"];            
            
            @throw [NSException exceptionWithName:@"StartServiceFailed" reason:@"Unable to start bus" userInfo:nil];
        }
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachement started.\n"];        
        
        // register the sample object with the bus
        //
        status = [self.bus registerBusObject:self.basicSampleObject];
        
        if (status != ER_OK) {
            @throw [NSException exceptionWithName:@"StartServiceFailed" reason:@"Unable to register bus object" userInfo:nil];
        }
        
        [self.delegate didReceiveStatusUpdateMessage:@"Object registered successfully.\n"];
        
        // connect the bus
        //
        status = [self.bus connectWithArguments:@"null:"];
        
        if (status != ER_OK) {
            [self.delegate didReceiveStatusUpdateMessage:@"Failed to connect to null:"];
            
            @throw [NSException exceptionWithName:@"StartServiceFailed" reason:@"Unable to connect to bus" userInfo:nil];
        }
        
        [self.delegate didReceiveStatusUpdateMessage:@"Bus now connected to null:\n"];        
        
        // request the service name for the sample object
        //
        status = [self.bus requestWellKnownName:kBasicObjectServiceName withFlags:kAJNBusNameFlagReplaceExisting|kAJNBusNameFlagDoNotQueue];
        
        if (status != ER_OK) {
            @throw [NSException exceptionWithName:@"StartServiceFailed" reason:@"Request for service name failed" userInfo:nil];
        }        
        
        // bind the session to a specific service port
        //
        status = [self.bus bindSessionOnPort:kBasicObjectServicePort withOptions:sessionOptions withDelegate:self];

        if (status != ER_OK) {
            @throw [NSException exceptionWithName:@"StartServiceFailed" reason:@"Unable to bind to session" userInfo:nil];
        }

        // let others on the bus know that this service exists
        //
        status = [self.bus advertiseName:kBasicObjectServiceName withTransportMask:sessionOptions.transports];

        if (status != ER_OK) {
            @throw [NSException exceptionWithName:@"StartServiceFailed" reason:@"Unable to advertise service name on bus" userInfo:nil];
        }
        
    }
    @catch (NSException *exception) {
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"ERROR: Exception thrown: %@. %@.", exception.name, exception.reason]];
    }
    @finally {

    }
}

- (void)ownershipOfName:(NSString *)busName didChangeFromPreviousOwner:(NSString *)previousOwner toNewOwner:(NSString *)newOwner
{
    if (newOwner && [busName compare:kBasicObjectServiceName] == NSOrderedSame) {
        [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Bus named %@ changed ownership from: %@ to %@\n", busName, previousOwner, newOwner]];
    }    
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    BOOL shouldAcceptConnection = sessionPort == kBasicObjectServicePort;
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Request from %@ to join session is %@.\n", joiner, shouldAcceptConnection ? @"accepted" : @"rejected"]];
    return shouldAcceptConnection;
}

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"%@ successfully joined session %u on port %d.\n", joiner, sessionId, sessionPort]];
}


@end
