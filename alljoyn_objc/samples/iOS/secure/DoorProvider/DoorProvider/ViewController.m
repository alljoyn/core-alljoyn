////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
#import "DoorCommon.h"

@interface ViewController ()

@property (nonatomic, strong) DoorCommon* doorCommon;
@property (nonatomic, strong) DoorCommonPCL* doorCommonPCL;
@property (nonatomic, strong) Door* door;
@property (nonatomic) BOOL isServiceStarted;
@property (nonatomic, strong) NSCondition* waitCondition;

@end

@implementation ViewController

@synthesize startButton;
@synthesize appNameField;
@synthesize textView;
@synthesize autoSignalSwitch;


- (QStatus)run:(AJNBusAttachment*)withBus
{
    QStatus status = ER_FAIL;
    // Create bus object
    self.door = [[Door alloc] initWithBus:withBus andView:self];

    status = [self.door initialize];
    if (ER_OK != status) {
        NSLog(@"Failed to initialize Door - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [withBus registerBusObject:self.door];
    if (ER_OK != status) {
        NSLog(@"Failed to RegisterBusObject - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [self.doorCommon announceAbout];
    if (ER_OK != status) {
        NSLog(@"Failed to AnnounceAbout - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    //Wait until we are claimed
    [self didReceiveStatusUpdateMessage:@"Waiting to be claimed..."];
    status = [self.doorCommonPCL waitForClaimedState];
    if (ER_OK != status) {
        NSLog(@"Failed to WaitForClaimedState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    // After claiming, only allow ALLJOYN_ECDHE_ECDSA connections
    status = [self.doorCommon setSecurityForClaimedMode];
    if (ER_OK != status) {
        NSLog(@"Failed to SetSecurityForClaimedMode - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    [self didReceiveStatusUpdateMessage:@"Door provider initialized; Waiting for consumers ...\n"];

    [self.waitCondition lock];
    [self.waitCondition wait];
    [self.waitCondition unlock];

    self.doorCommon = nil;
    self.door = nil;
    self.doorCommonPCL = nil;

    return ER_OK;
}

- (QStatus)startDoorProvider:(NSString*)appName
{
    // Do the common set-up
    self.doorCommon = [[DoorCommon alloc] initWithAppName:appName andView:self];

    AJNBusAttachment* ba = self.doorCommon.BusAttachment;
    self.doorCommonPCL = [[DoorCommonPCL alloc] initWithBus:ba];

    QStatus status = [self.doorCommon initialize:YES withListener:self.doorCommonPCL];
    if (ER_OK != status) {
        NSLog(@"Failed to initialize DoorCommon - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.DoorProvider.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run:ba];
    });

    return status;
}

- (QStatus)stopDoorProvider
{
    [self.waitCondition signal];
    return ER_OK;
}

- (IBAction)didTouchStartButton:(id)sender
{
    NSString* appName = [appNameField text];
    [self.waitCondition lock];
    if (appName != nil && appName.length != 0) {
        QStatus status = ER_OK;
        if (!self.isServiceStarted) {
            status = [self startDoorProvider:appName];
            if (status == ER_OK) {
                [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
                self.isServiceStarted = YES;
            }
        } else {
            status = [self stopDoorProvider];
            [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
            self.isServiceStarted = NO;
        }
    } else {
        UIAlertController * alert = [UIAlertController
            alertControllerWithTitle:@"App Name Empty"
                             message:@"You must specify an app name"
                      preferredStyle:UIAlertControllerStyleAlert];

        UIAlertAction* okButton = [UIAlertAction
            actionWithTitle:@"Ok"
                      style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction * action) {
                    }];


        [alert addAction:okButton];

        [self presentViewController:alert animated:YES completion:nil];
    }
    [self.waitCondition unlock];
}

- (IBAction)didTouchSignalButton:(id)sender
{
    @synchronized (self) {
        QStatus status = [self.door sendDoorEvent];
        if (ER_OK != status) {
            [self didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Failed to SendDoorEvent - status (%@)", [AJNStatus descriptionForStatusCode:status]]];
        }
    }
}

- (IBAction)didToggleAutoSignal:(id)sender
{
    @synchronized (self) {

        [self didReceiveStatusUpdateMessage:@"Enabling automatic signaling of door events ... "];
        QStatus status = [DoorCommon updateDoorProviderManifest:self.doorCommon];
        if (ER_OK != status) {
            NSLog(@"Failed to UpdateDoorProviderManifest - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        }
        self.door.autoSignal = self.autoSignalSwitch.on;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.waitCondition = [[NSCondition alloc] init];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)didReceiveStatusUpdateMessage:(NSString *)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.textView.text.length ? [self.textView.text mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@] ",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [self.textView setText:string];
        NSLog(@"%@",message);
    });
}

@end
