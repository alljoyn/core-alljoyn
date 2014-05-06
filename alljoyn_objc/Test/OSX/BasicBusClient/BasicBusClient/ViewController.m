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

#import "ViewController.h"
#import "AJNBusAttachment.h"

@interface ViewController ()

@property (nonatomic) bool isConnectedToService;

@end

@implementation ViewController

- (AJNTransportMask)transportType
{
    AJNTransportMask transportMask;
    switch (self.transportTypeSegmentedControl.selectedSegment) {
        case 0:
            transportMask = kAJNTransportMaskAny;
            break;
            
        default:
            break;
    }
    return transportMask;
}

- (void)setIsConnectedToService:(bool)isConnectedToService
{
    NSString *title = isConnectedToService ? @"Stop" : @"Start";
    [self.startButton setTitle:title];
    _isConnectedToService = isConnectedToService;
}


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {

    }
    
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        [PingClient.sharedInstance setDelegate:self];
    }
    return self;
}

- (IBAction)didTouchStartButton:(id)sender
{
    if (!self.isConnectedToService) {
        [PingClient.sharedInstance connectToService:self.advertisedNameTextField.stringValue];
    }
    else {
        [PingClient.sharedInstance disconnect];
    }
    
    self.isConnectedToService = !self.isConnectedToService;
}

#pragma mark - PingClientDelegate implementation

- (void)didConnectWithService:(NSString *)serviceName
{
    [self receivedStatusMessage:[NSString stringWithFormat:@"Successfully connected with the service named %@", serviceName]];
    
    self.isConnectedToService = true;
}

- (void)shouldDisconnectFromService:(NSString *)serviceName
{
    [self receivedStatusMessage:[NSString stringWithFormat:@"Disconnected from the service named %@", serviceName]];
    
    if (self.isConnectedToService) {
        [PingClient.sharedInstance disconnect];
    }
    
    self.isConnectedToService = false;
}

- (void)receivedStatusMessage:(NSString *)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.eventsTextView.string.length ? [self.eventsTextView.string mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@] ",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [string appendString:@"\r\n\r\n"];
        [self.eventsTextView setString:string];
        NSLog(@"%@",string);
    });
}


@end
