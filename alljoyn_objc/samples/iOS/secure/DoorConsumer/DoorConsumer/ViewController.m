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
#import "DoorConsumerAllJoynService.h"

@interface ViewController ()  <AllJoynStatusMessageListener>

@property (nonatomic, strong) DoorConsumerAllJoynService *doorConsumerService;

//@property (nonatomic, strong) DoorCommon* doorCommon;
//@property (nonatomic, strong) DoorCommonPCL* doorCommonPCL;
//@property (nonatomic, strong) Door* door;
//@property (nonatomic) BOOL isConsumerStarted;
//@property (nonatomic, strong) NSCondition* waitCondition;
//@property (nonatomic) DoorSessionManager* sessionManager;
//@property (nonatomic) DoorMessageReceiver* dmr;
//@property (nonatomic, strong) DoorAboutListener* dal;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    _doorConsumerService = [[DoorConsumerAllJoynService alloc] initWithMessageListener:self];

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

- (IBAction)didTouchOpenButton:(id)sender
{
    [_doorConsumerService openDoors];
}

- (IBAction)didTouchCloseButton:(id)sender {
    [_doorConsumerService closeDoors];
}

- (IBAction)didTouchStartButton:(id)sender
{
    NSString* appName = [_appNameField text];
    if (appName != nil && appName.length != 0) {
        QStatus status = ER_OK;
        if (_doorConsumerService.serviceState == STOPED) {
            status = [_doorConsumerService startWithName:appName];
            if (status == ER_OK) {
                [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
            }
        } else {
            [_doorConsumerService stop];
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
}

@end
