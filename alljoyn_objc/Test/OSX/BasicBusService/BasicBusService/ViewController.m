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

#import "ViewController.h"
#import "PingService.h"

@interface ViewController () <PingServiceDelegate>

@property (nonatomic) BOOL isStarted;

@end

@implementation ViewController

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
        [PingService.sharedInstance setDelegate:self];
    }
    return self;
}

- (IBAction)didTouchStartButton:(id)sender
{
    if (self.isStarted) {
        [PingService.sharedInstance stop];
        [self.startButton setTitle:@"Start"];
    }
    else {
        [PingService.sharedInstance start:self.advertisedNameTextField.stringValue];
        [self.startButton setTitle:@"Stop"];
    }
    self.isStarted = !self.isStarted;
}

#pragma mark - PingServiceDelegate implementation

- (AJNTransportMask)transportType
{
    return kAJNTransportMaskAny;
}

// The delegate is called once a client joins a session with the service
//
- (void)client:(NSString *)clientName didJoinSession:(AJNSessionId)sessionId
{
    [self receivedStatusMessage:[NSString stringWithFormat:@"Client %@ joined session %u", clientName, sessionId]];
}

// The delegate is called when a client leaves a session
//
- (void)client:(NSString *)clientName didLeaveSession:(AJNSessionId)sessionId
{
    [self receivedStatusMessage:[NSString stringWithFormat:@"Client %@ left session %u", clientName, sessionId]];
}

// Send updates on internal state of Ping Service
//
- (void)receivedStatusMessage:(NSString*)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.eventsTextView.string.length ? [self.eventsTextView.string mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [string appendString:@"\n\n"];
        [self.eventsTextView setString:string];
        NSLog(@"%@",string);
    });
}

@end