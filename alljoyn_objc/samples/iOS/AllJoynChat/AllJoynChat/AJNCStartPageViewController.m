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

#import <QuartzCore/QuartzCore.h>
#import "AJNCStartPageViewController.h"
#import "AJNCChatObjectSignalHandler.h"
#import "AJNCConversation.h"
#import "AJNCConstants.h"
#import "AJNInterfaceDescription.h"

@interface AJNCStartPageViewController () <AJNBusListener, AJNSessionListener, AJNSessionPortListener, AJNChatReceiver>

@property (nonatomic, strong) AJNCBusObject *chatObject;
@property (nonatomic, strong) AJNCChatObjectSignalHandler *chatObjectSignalHandler;
@property (nonatomic, strong) AJNBusAttachment *busAttachment;
@property (nonatomic) AJNSessionId sessionId;
@property (nonatomic) BOOL isChatConversationUIVisible;

@property (nonatomic) CGSize keyboardSize;
@property (nonatomic) CGSize originalViewSize;
@property (nonatomic, readonly) NSString *sessionlessSignalMatchRule;
@end

@implementation AJNCStartPageViewController

@synthesize chatSessionTypeSegmentedControl = _chatSessionTypeSegmentedControl;
@synthesize chatConversationTextView = _chatConversationTextView;
@synthesize chatMessageTextField = _chatMessageTextField;
@synthesize startButton = _startButton;
@synthesize sendButton = _sendButton;
@synthesize stopButton = _stopButton;
@synthesize keyboardSize = _keyboardSize;
@synthesize chatObject = _chatObject;
@synthesize chatObjectSignalHandler = _chatObjectSignalHandler;
@synthesize busAttachment = _busAttachment;
@synthesize sessionId = _sessionId;
@synthesize isChatConversationUIVisible = _isChatConversationUIVisible;
@synthesize originalViewSize = _originalViewSize;

- (NSString *)sessionlessSignalMatchRule
{
    return @"sessionless='t'";
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)viewDidUnload
{
    [self setChatSessionTypeSegmentedControl:nil];
    [self setChatConversationTextView:nil];
    [self setChatMessageTextField:nil];
    [self setStartButton:nil];
    [self setSendButton:nil];
    [self setStopButton:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
}

-(void)keyboardWillShow:(NSNotification*)notification 
{
    self.keyboardSize = [[[notification userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
    
    // resize the view to accomodate the keyboard
    //
    if (self.view.frame.size.height >= 416.0)
    {
        [self setViewMovedUp:YES];
    }
}

-(void)keyboardWillHide:(NSNotification*)notification
{
    // resize the view when the keyboard disappears
    //
    if (self.view.frame.size.height < 416.0)
    {
        [self setViewMovedUp:NO];
    }
}

- (void)setChatUIVisible:(BOOL)visible
{
    [UIView animateWithDuration:1 animations:^{
        if (visible && !self.isChatConversationUIVisible) {
            self.chatSessionTypeSegmentedControl.frame = CGRectOffset(self.chatSessionTypeSegmentedControl.frame, 0, 235);
            self.startButton.frame = CGRectOffset(self.startButton.frame, 0, 300);
            self.chatConversationTextView.frame = CGRectOffset(self.chatConversationTextView.frame, -300, 0);
            self.chatMessageTextField.frame = CGRectOffset(self.chatMessageTextField.frame, -300, 0);
            self.sendButton.frame = CGRectOffset(self.sendButton.frame, -300, 0);
            self.stopButton.frame = CGRectOffset(self.stopButton.frame, -300, 0);
            self.sessionLabel.frame = CGRectOffset(self.sessionLabel.frame, -350, 0);
            self.sessionSwitch.frame = CGRectOffset(self.sessionSwitch.frame, -350, 0);
            self.isChatConversationUIVisible = YES;
        }
        else if (!visible && self.isChatConversationUIVisible) {
            self.chatSessionTypeSegmentedControl.frame = CGRectOffset(self.chatSessionTypeSegmentedControl.frame, 0, -235);
            self.startButton.frame = CGRectOffset(self.startButton.frame, 0, -300);
            self.chatConversationTextView.frame = CGRectOffset(self.chatConversationTextView.frame, 300, 0);
            self.chatMessageTextField.frame = CGRectOffset(self.chatMessageTextField.frame, 300, 0);
            self.sendButton.frame = CGRectOffset(self.sendButton.frame, 300, 0);     
            self.stopButton.frame = CGRectOffset(self.stopButton.frame, 300, 0);                
            self.sessionLabel.frame = CGRectOffset(self.sessionLabel.frame, 350, 0);
            self.sessionSwitch.frame = CGRectOffset(self.sessionSwitch.frame, 350, 0);
            self.isChatConversationUIVisible = NO;
        }

    } completion:^(BOOL finished) {
        if (self.isChatConversationUIVisible) {
            [self.chatMessageTextField becomeFirstResponder];
            [self.chatConversationTextView scrollRangeToVisible:NSMakeRange([self.chatConversationTextView.text length], 0)];
            [self.chatConversationTextView setNeedsDisplay];

        }
        else {
            [self.chatMessageTextField resignFirstResponder];
        }
        
    }];
    
}

-(void)setViewMovedUp:(BOOL)movedUp
{
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:0.3];
    
    CGRect rect = self.view.frame;
    if (movedUp)
    {
        rect.size.height -= self.keyboardSize.height;
    }
    else
    {
        // go back to the normal size for the view.
        rect.size.height += self.keyboardSize.height;
    }
    self.view.frame = rect;
    
    [UIView commitAnimations];
}

- (void)viewWillAppear:(BOOL)animated
{
    // register for keyboard notifications
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillShow:)
                                                 name:UIKeyboardWillShowNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillHide:)
                                                 name:UIKeyboardWillHideNotification
                                               object:nil];
    
    // set up some graphical niceties
    //
    self.chatConversationTextView.layer.cornerRadius = 5;
    self.chatConversationTextView.layer.masksToBounds = YES;    
    
    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    // unregister for keyboard notifications while not visible.
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillShowNotification
                                                  object:nil];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillHideNotification
                                                  object:nil];
    
    [super viewWillDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (IBAction)didTouchStartButton:(id)sender 
{
    [self setChatUIVisible:YES];
 
    QStatus status = ER_OK;
    
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

    if (gMessageFlags == kAJNMessageFlagSessionless) {
        NSLog(@"Adding match rule : [%@]", self.sessionlessSignalMatchRule);
        status = [self.busAttachment addMatchRule:self.sessionlessSignalMatchRule];
        
        if (status != ER_OK) {
            NSLog(@"ERROR: Unable to %@ match rule. %@", self.sessionSwitch.isOn ? @"remove" : @"add", [AJNStatus descriptionForStatusCode:status]);
        }
    }
    else {

        // get the type of session to create
        //
        NSString *serviceName = [NSString stringWithFormat:@"%@%@", kServiceName, @"AllChatz"];
        
        if (self.chatSessionTypeSegmentedControl.selectedSegmentIndex == 0) { 
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
            [self.busAttachment advertiseName:serviceName withTransportMask:kAJNTransportMaskAny];
            
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
            
            [self.busAttachment bindSessionOnPort:kServicePort withOptions:sessionOptions withDelegate:self];
        }
    }
}

- (IBAction)didTouchStopButton:(id)sender 
{
    NSString *serviceName = [NSString stringWithFormat:@"%@%@", kServiceName, @"AllChatz"];
    
    // leave the chat session
    //
    [self.busAttachment leaveSession:self.sessionId];
    
    // cancel the advertised name search, or the advertisement, depending on if this is a
    // service or client
    //
    if (self.chatSessionTypeSegmentedControl.selectedSegmentIndex == 0) { 
        [self.busAttachment cancelFindAdvertisedName:serviceName];
    }
    else {
        [self.busAttachment cancelAdvertisedName:serviceName withTransportMask:kAJNTransportMaskAny];
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
    
    [self setChatUIVisible:NO];
}

- (IBAction)didTouchSendButton:(id)sender 
{
    NSString *message = [NSString stringWithFormat:@"%@||%@", [[UIDevice currentDevice] name], self.chatMessageTextField.text];
    [self.chatObject sendChatMessage:message onSession:self.sessionId];
    if (gMessageFlags != kAJNMessageFlagSessionless) {
        [self chatMessageReceived:message from:@"Local" onObjectPath:@"local" forSession:self.sessionId];
    }
    self.chatMessageTextField.text = @"";    
}

- (IBAction)didBeginEditingChatConversationTextField:(id)sender 
{
    if ([sender isEqual:self.chatMessageTextField])
    {
        // resize the view so that the keyboard does not hide it
        //
        if  (self.view.frame.origin.y >= 0)
        {
            [self setViewMovedUp:YES];
        }
    }
}

- (IBAction)didEndEditingChatConversationTextField:(id)sender 
{
    [self didTouchSendButton:self];
}

- (IBAction)sessionSignalValueDidChange:(id)sender
{
    if (self.sessionSwitch.isOn) {
        gMessageFlags = 0x0;
        [self.chatSessionTypeSegmentedControl setEnabled:YES];
    }
    else {
        gMessageFlags = kAJNMessageFlagSessionless;
        [self.chatSessionTypeSegmentedControl setEnabled:NO];
    }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [self didTouchSendButton:self];
    return YES;
}

- (void)chatMessageReceived:(NSString *)message from:(NSString*)sender onObjectPath:(NSString *)path forSession:(AJNSessionId)sessionId
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSArray *messageTokens = [message componentsSeparatedByString:@"||"];
        NSMutableString *string = self.chatConversationTextView.text.length ? [self.chatConversationTextView.text mutableCopy] : [[NSMutableString alloc] init];
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
            [string appendString:@"\n\n"];            
        }
        [self.chatConversationTextView setText:string];
        NSLog(@"%@",string);
        [self.chatConversationTextView scrollRangeToVisible:NSMakeRange([self.chatConversationTextView.text length], 0)];
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
    if (self.chatSessionTypeSegmentedControl.selectedSegmentIndex == 1) {
        self.sessionId = sessionId;
    }
    
    NSMutableString *string = self.chatConversationTextView.text.length ? [self.chatConversationTextView.text mutableCopy] : [[NSMutableString alloc] init];    
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setTimeStyle:NSDateFormatterMediumStyle];
    [formatter setDateStyle:NSDateFormatterShortStyle];
    [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
    [string appendFormat:@"<%@> joined the conversation.", joiner];
    [string appendString:@"\n\n"];
    [self.chatConversationTextView setText:string];        
    NSLog(@"%@",string);
    [self.chatConversationTextView scrollRangeToVisible:NSMakeRange([self.chatConversationTextView.text length], 0)];
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    return sessionPort == kServicePort;
}

@end
