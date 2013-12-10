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
