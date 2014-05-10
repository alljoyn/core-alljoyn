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

#import "ChatViewController.h"
#import "AJNCChatObjectSignalHandler.h"
#import "AJNCConversation.h"
#import "AJNCConstants.h"
#import "AJNInterfaceDescription.h"

@interface ChatController : NSObject<AJNBusListener, AJNSessionListener, AJNSessionPortListener, AJNChatReceiver>

@property (nonatomic, strong) id<AJNChatReceiver> delegate;
@property (nonatomic, strong) AJNCBusObject *chatObject;
@property (nonatomic, strong) AJNCChatObjectSignalHandler *chatObjectSignalHandler;
@property (nonatomic, strong) AJNBusAttachment *busAttachment;
@property (nonatomic) AJNSessionId sessionId;
@property (nonatomic) NSInteger sessionType;
@property (nonatomic) AJNTransportMask transportMask;
@property (nonatomic, strong) NSString *serviceName;

- (void)startSession:(NSString *)serviceName sessionType:(NSInteger)sessionType usingTransportMask:(AJNTransportMask)transportMask;
- (void)stop;
- (void)sendMessage:(NSString *)message;

@end

@implementation ChatController

- (void)startSession:(NSString *)serviceName sessionType:(NSInteger)sessionType usingTransportMask:(AJNTransportMask)transportMask
{
    QStatus status = ER_OK;
    
    self.sessionType = sessionType;
    self.transportMask = transportMask;
    self.serviceName = serviceName;
    
    self.busAttachment = [[AJNBusAttachment alloc] initWithApplicationName:kAppName allowRemoteMessages:YES];
    
    // create and register our interface
    //
    AJNInterfaceDescription* chatInterface = [self.busAttachment createInterfaceWithName:kInterfaceName];
    
    status = [chatInterface addSignalWithName:@"Chat" inputSignature:@"s" argumentNames:[NSArray arrayWithObject:@"str"]];
    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to add signal to chat interface. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    [chatInterface activate];
    
    // register signal handler
    //
    self.chatObjectSignalHandler = [[AJNCChatObjectSignalHandler alloc] init];
    self.chatObjectSignalHandler.delegate = self;
    [self.busAttachment registerSignalHandler:self.chatObjectSignalHandler];
    
    // create and register the chat bus object
    //
    self.chatObject = [[AJNCBusObject alloc] initWithBusAttachment:self.busAttachment onServicePath:kServicePath];
    
    self.chatObject.delegate = self;
    
    [self.busAttachment registerBusObject:self.chatObject];
    
    // start the bus
    //
    status = [self.busAttachment start];
    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to start bus. %@", [AJNStatus descriptionForStatusCode:status]);
    }
    
    // register our view controller as the bus listener
    //
    [self.busAttachment registerBusListener:self];
    
    // connect to the bus
    //
    status = [self.busAttachment connectWithArguments:@"null:"];
    if (status != ER_OK) {
        NSLog(@"ERROR: Failed to connect bus. %@", [AJNStatus descriptionForStatusCode:status]);
    }
        
    if (sessionType == 0) {
        // join an existing session by finding the name
        //
        [self.busAttachment findAdvertisedName:serviceName];
    }
    else {
        // request the service name for the chat object
        //
        [self.busAttachment requestWellKnownName:serviceName withFlags:kAJNBusNameFlagReplaceExisting|kAJNBusNameFlagDoNotQueue];
        
        // advertise a name and let others connect to our service
        //
        [self.busAttachment advertiseName:serviceName withTransportMask:transportMask];
        
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:transportMask];
        
        [self.busAttachment bindSessionOnPort:kServicePort withOptions:sessionOptions withDelegate:self];
    }
}

- (void)stop
{
    // leave the chat session
    //
    [self.busAttachment leaveSession:self.sessionId];
    
    // cancel the advertised name search, or the advertisement, depending on if this is a
    // service or client
    //
    if (self.sessionType == 0) {
        [self.busAttachment cancelFindAdvertisedName:self.serviceName];
    }
    else {
        [self.busAttachment cancelAdvertisedName:self.serviceName withTransportMask:self.transportMask];
    }
    
    // disconnect from the bus
    //
    [self.busAttachment disconnectWithArguments:@"null:"];
    
    // unregister our listeners and the chat bus object
    //
    [self.busAttachment unregisterBusListener:self];
    
    [self.busAttachment unregisterSignalHandler:self.chatObjectSignalHandler];
    
    [self.busAttachment unregisterBusObject:self.chatObject];
    
    // stop the bus and wait for the stop operation to complete
    //
    [self.busAttachment stop];
    
    [self.busAttachment waitUntilStopCompleted];
    
    // dispose of everything
    //
    self.chatObjectSignalHandler.delegate = nil;
    self.chatObjectSignalHandler = nil;
    
    self.chatObject.delegate = nil;
    self.chatObject = nil;
    
    self.busAttachment = nil;
}

- (void)sendMessage:(NSString *)message
{
    [self.chatObject sendChatMessage:message onSession:self.sessionId];
    [self chatMessageReceived:message from:@"Local" onObjectPath:@"local" forSession:self.sessionId];    
}

- (void)chatMessageReceived:(NSString *)message from:(NSString*)sender onObjectPath:(NSString *)path forSession:(AJNSessionId)sessionId
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate chatMessageReceived:message from:sender onObjectPath:path forSession:sessionId];
    });
}

#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString *)namePrefix
{
    NSString *conversationName = [NSString stringWithFormat:@"%@", [[name componentsSeparatedByString:@"."] lastObject]];
    
    NSLog(@"Discovered chat conversation: \"%@\"\n", conversationName);
    
    [self.busAttachment enableConcurrentCallbacks];
    
    /* Join the conversation */
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    AJNSessionId sessionId = [self.busAttachment joinSessionWithName:name onPort:kServicePort withDelegate:self options:sessionOptions];
    if (sessionId != 0) {
        self.sessionId = sessionId;
    }
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSString *conversationName = [NSString stringWithFormat:@"%@", [[name componentsSeparatedByString:@"."] lastObject]];
    
    NSLog(@"Lost chat conversation: \"%@\"\n", conversationName);
}

- (void)nameOwnerChanged:(NSString *)name to:(NSString *)newOwner from:(NSString *)previousOwner
{
    
}

#pragma mark - AJNSessionPortListener delegate methods

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    if (self.sessionType == 1) {
        self.sessionId = sessionId;
    }
    
    NSMutableString *string = [NSMutableString stringWithFormat:@"<%@> joined the conversation.", joiner];
    [string appendString:@"\n\n"];
    [self.delegate chatMessageReceived:string from:@"LOG" onObjectPath:nil forSession:sessionId];
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    return sessionPort == kServicePort;
}

@end

@interface ChatViewController () <AJNChatReceiver>

@property (nonatomic, strong) ChatController *chatController;

@end

@implementation ChatViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        self.chatController = [[ChatController alloc] init];
        self.chatController.delegate = self;
    }
    return self;
}

- (void)windowWillClose:(NSNotification *)notification
{
    [self.chatController stop];
}

- (IBAction)didTouchStartButton:(id)sender
{
    [self.chatController startSession:self.advertisedNameTextField.stringValue sessionType:self.sessionTypeSegmentedControl.selectedSegment usingTransportMask:kAJNTransportMaskAny ];
    [self.startButton setHidden:YES];
    [self.stopButton setHidden:NO];
}

- (IBAction)didTouchStopButton:(id)sender
{
    [self.chatController stop];
    
    [self.startButton setHidden:NO];
    [self.stopButton setHidden:YES];
}

- (IBAction)didTouchSendButton:(id)sender
{
    NSString *message = [NSString stringWithFormat:@"%@||%@", NSUserName(), self.messageTextField.stringValue];
    [self.chatController sendMessage:message];
    self.messageTextField.stringValue = @"";
}

- (void)chatMessageReceived:(NSString *)message from:(NSString*)sender onObjectPath:(NSString *)path forSession:(AJNSessionId)sessionId
{
    NSArray *messageTokens = [message componentsSeparatedByString:@"||"];
    NSMutableString *string = self.conversationTextView.string.length ? [self.conversationTextView.string mutableCopy] : [[NSMutableString alloc] init];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setTimeStyle:NSDateFormatterMediumStyle];
    [formatter setDateStyle:NSDateFormatterShortStyle];
    [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];    
    if (messageTokens.count == 2) {
        [string appendFormat:@"From:<%@>\n", [messageTokens objectAtIndex:0]];
        [string appendString:[messageTokens objectAtIndex:1]];
        [string appendString:@"\n\n"];
    }
    else {
        [string appendString:message];
    }
    [self.conversationTextView setString:string];
    NSLog(@"%@",string);
    [self.conversationTextView scrollRangeToVisible:NSMakeRange([self.conversationTextView.string length], 0)];
}

@end
