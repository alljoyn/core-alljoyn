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

#import "AJNCViewController.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNCBusObject.h"
#import "AJNCConstants.h"

@interface AJNCViewController () <AJNBusListener, AJNSessionListener, AJNSessionPortListener, AJNChatReceiver>

@property (nonatomic, strong) AJNCBusObject *chatObject;
@property (nonatomic, strong) AJNBusAttachment *busAttachment;
@property (nonatomic) AJNSessionId sessionId;

@end

@implementation AJNCViewController

@synthesize sessionTypeSegmentedControl = _sessionTypeSegmentedControl;
@synthesize sessionNameTextField = _sessionNameTextField;
@synthesize conversationTextView = _conversationTextView;
@synthesize messageTextField = _messageTextField;
@synthesize chatObject = _chatObject;
@synthesize busAttachment = _busAttachment;
@synthesize sessionId = _sessionId;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
	// Do any additional setup after loading the view, typically from a nib.
    self.busAttachment = [[AJNBusAttachment alloc] initWithApplicationName:kAppName allowRemoteMessages:YES];
    
    // register our interface
    //
    /* Create org.alljoyn.bus.samples.chat interface */
    AJNInterfaceDescription* chatInterface = [self.busAttachment interfaceWithName:kInterfaceName];
    
    [chatInterface addSignalWithName:@"Chat" inputSignature:@"s" argumentNames:[NSArray arrayWithObject:@"str"]];
    
    [chatInterface activate];
        
    [self.busAttachment start];
    
    [self.busAttachment registerBusListener:self];
    
    [self.busAttachment connectWithArguments:@"null:"];   
}

- (void)viewDidUnload
{
    [self setMessageTextField:nil];
    [self setConversationTextView:nil];
    [self setSessionTypeSegmentedControl:nil];
    [self setSessionNameTextField:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [self.sessionNameTextField becomeFirstResponder];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
    } else {
        return YES;
    }
}

- (IBAction)didTouchStartButton:(id)sender {
        
    // get the type of session to create
    //
    NSString *serviceName = [NSString stringWithFormat:@"%@%@", kServiceName, @"MooGooGaiPan"];
    self.chatObject = [[AJNCBusObject alloc] initWithBusAttachment:self.busAttachment onServicePath:kServicePath];
    
    self.chatObject.delegate = self;
    
    [self.busAttachment registerBusObject:self.chatObject];

    if (self.sessionTypeSegmentedControl.selectedSegmentIndex == 0) { 
        // join an existing session by finding the name
        //
        [self.busAttachment findAdvertisedName:serviceName];
    }
    else {
        // request the service name for the sample object
        //
        [self.busAttachment requestWellKnownName:serviceName withFlags:kAJNBusNameFlagReplaceExisting|kAJNBusNameFlagDoNotQueue];
        
        // advertise a name and let others connect to our service
        //
        [self.busAttachment advertiseName:serviceName withTransportMask:kAJNTransportMaskAny];
        
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
        
        [self.busAttachment bindSessionOnPort:kServicePort withOptions:sessionOptions withDelegate:self];
    }
}

- (IBAction)didTouchSendButton:(id)sender 
{
    [self.chatObject sendChatMessage:self.messageTextField.text onSession:self.sessionId];
    [self chatMessageReceived:self.messageTextField.text from:@"Me" onObjectPath:@"local" forSession:self.sessionId];
    self.messageTextField.text = @"";
}

- (IBAction)messageTextEditingDidEnd:(id)sender {
    [self didTouchSendButton:self];
}

- (void)chatMessageReceived:(NSString *)message from:(NSString*)sender onObjectPath:(NSString *)path forSession:(AJNSessionId)sessionId
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.conversationTextView.text.length ? [self.conversationTextView.text mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
        [string appendFormat:@"From:<%@>\n", sender];
        [string appendFormat:@"Path:<%@>\n", path];
        [string appendString:message];
        [string appendString:@"\n\n"];
        [self.conversationTextView setText:string];        
        NSLog(@"%@",string);
        [self.conversationTextView scrollRangeToVisible:NSMakeRange([self.conversationTextView.text length], 0)];
    });    
    
}

#pragma mark - AJNBusListener delegate methods

- (void)didFindAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString *)namePrefix
{
    NSString *conversationName = [NSString stringWithFormat:@"%@", [[name componentsSeparatedByString:@"."] lastObject]];

    NSLog(@"Discovered chat conversation: \"%@\"\n", conversationName);
    
    /* Join the conversation */
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    self.sessionId = [self.busAttachment joinSessionWithName:name onPort:kServicePort withDelegate:self options:sessionOptions];
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
    self.sessionId = sessionId;

    NSMutableString *string = self.conversationTextView.text.length ? [self.conversationTextView.text mutableCopy] : [[NSMutableString alloc] init];    
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setTimeStyle:NSDateFormatterMediumStyle];
    [formatter setDateStyle:NSDateFormatterShortStyle];
    [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
    [string appendFormat:@"<%@> joined the conversation.", joiner];
    [string appendString:@"\n\n"];
    [self.conversationTextView setText:string];        
    NSLog(@"%@",string);
    [self.conversationTextView scrollRangeToVisible:NSMakeRange([self.conversationTextView.text length], 0)];
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    return sessionPort == kServicePort;
}

@end
