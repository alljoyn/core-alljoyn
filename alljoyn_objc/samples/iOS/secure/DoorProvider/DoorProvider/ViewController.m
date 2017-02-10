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
#import "DoorProviderAllJoynService.h"

@interface ViewController () <AllJoynStatusMessageListener>

@property (nonatomic, strong) DoorProviderAllJoynService *doorProviderService;
@property (nonatomic, strong) NSCondition* waitCondition;

@end

@implementation ViewController

- (IBAction)didTouchStartButton:(id)sender
{
    [self.waitCondition lock];
    NSString* appName = [_appNameField text];
    if (appName != nil && appName.length != 0) {
        QStatus status = ER_OK;
        if (_doorProviderService.serviceState == STOPED) {
            status = [_doorProviderService startWithName:appName];
            if (status == ER_OK) {
                [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
            }
        } else {
            status = [_doorProviderService stop];
            [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
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
        [_doorProviderService sendDoorEvent];
    }
}

- (IBAction)didToggleAutoSignal:(id)sender
{
    @synchronized (self) {
        [self didReceiveStatusUpdateMessage:@"Enabling automatic signaling of door events ... "];
        [_doorProviderService toggleAutoSignal];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    _doorProviderService = [[DoorProviderAllJoynService alloc] initWithMessageListener:self];
    _waitCondition = [[NSCondition alloc] init];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)didReceiveAllJoynStatusMessage:(NSString *)message
{
    [self didReceiveStatusUpdateMessage:message];
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
