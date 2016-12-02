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