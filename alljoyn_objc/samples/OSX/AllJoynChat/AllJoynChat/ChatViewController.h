////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import <Cocoa/Cocoa.h>

@interface ChatViewController : NSViewController<NSWindowDelegate>

@property (weak) IBOutlet NSTextField *advertisedNameTextField;
@property (weak) IBOutlet NSSegmentedControl *sessionTypeSegmentedControl;
@property (weak) IBOutlet NSSegmentedControl *transportTypeSegmentedControl;
@property (weak) IBOutlet NSButton *startButton;
@property (unsafe_unretained) IBOutlet NSTextView *conversationTextView;
@property (weak) IBOutlet NSTextField *messageTextField;
@property (weak) IBOutlet NSButton *stopButton;

- (IBAction)didTouchStartButton:(id)sender;
- (IBAction)didTouchStopButton:(id)sender;
- (IBAction)didTouchSendButton:(id)sender;

- (void)windowWillClose:(NSNotification *)notification;

@end