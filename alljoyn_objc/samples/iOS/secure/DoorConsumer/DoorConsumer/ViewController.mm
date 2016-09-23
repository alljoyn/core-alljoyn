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
#import <alljoyn/BusAttachment.h>
#import "DoorCommon.h"

using namespace ajn;

#define METHOD_CALL_TIMEOUT 10000

@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) BusAttachment *busAttachment;

@end

@interface ViewController ()

@property (nonatomic, strong) DoorCommon* doorCommon;
@property (nonatomic, strong) DoorCommonPCL* doorCommonPCL;
@property (nonatomic, strong) Door* door;
@property (nonatomic) BOOL isConsumerStarted;
@property (nonatomic, strong) NSCondition* waitCondition;
@property (nonatomic) DoorSessionManager* sessionManager;
@property (nonatomic) DoorMessageReceiver* dmr;
@property (nonatomic, strong) DoorAboutListener* dal;

@end

@implementation ViewController

@synthesize startButton;
@synthesize appNameField;
@synthesize textView;
@synthesize openButton;
@synthesize closeButton;
@synthesize stateLabel;

- (void)viewDidLoad {
    [super viewDidLoad];
    self.waitCondition = [[NSCondition alloc] init];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (QStatus)run:(AJNBusAttachment*)bus
{

    self.sessionManager = new DoorSessionManager(bus.busAttachment, METHOD_CALL_TIMEOUT, self);
    self.dmr = new DoorMessageReceiver;

    self.dal = [[DoorAboutListener alloc] init];

    QStatus status = [self.doorCommon initialize:NO withListener:self.doorCommonPCL];
    if (ER_OK != status) {
        NSLog(@"Failed to initialize DoorCommon - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [self.doorCommon announceAbout];
    if (ER_OK != status) {
        NSLog(@"Failed to AnnounceAbout - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    [self.doorCommonPCL waitForClaimedState];
    if (ER_OK != status) {
        NSLog(@"Failed to WaitForClaimedState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = bus.busAttachment->RegisterSignalHandlerWithRule(self.dmr,
                                                             static_cast<MessageReceiver::SignalHandler>(&DoorMessageReceiver::DoorEventHandler),
                                                             static_cast<InterfaceDescription::Member*>([self.doorCommon getDoorSignal].handle),
                                                             [DOOR_SIGNAL_MATCH_RULE UTF8String]);
    if (ER_OK != status) {
        NSLog(@"Failed to register signal handler - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    //Look for doors and Register About Listener
    status = [bus whoImplementsInterface:DOOR_INTERFACE];
    if (ER_OK != status) {
        NSLog(@"Failed to call WhoImplements - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    [bus registerAboutListener:self.dal];

    [self.waitCondition lock];
    [self.waitCondition wait];
    [self.waitCondition unlock];

    // Release door objects
    self.doorCommon = nil;
    self.door = nil;
    self.doorCommonPCL = nil;
    delete self.dmr;
    delete self.sessionManager;

    return status;
}

- (QStatus)startDoorConsumer:(NSString*)appName
{
    // Do the common set-up
    self.doorCommon = [[DoorCommon alloc] initWithAppName:appName andView:self];

    AJNBusAttachment* ba = self.doorCommon.BusAttachment;
    self.doorCommonPCL = [[DoorCommonPCL alloc] initWithBus:ba];
    if (!self.doorCommonPCL) {
        return ER_FAIL;
    }

    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.DoorConsumer.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run:ba];
    });

    return ER_OK;
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
    for (id door in self.dal.doors) {
        NSString* doorBusName = door;
        self.sessionManager->MethodCall([doorBusName UTF8String], [DOOR_OPEN UTF8String]);
    }
}

- (IBAction)didTouchCloseButton:(id)sender {
    for (id door in self.dal.doors) {
        NSString* doorBusName = door;
        self.sessionManager->MethodCall([doorBusName UTF8String], [DOOR_CLOSE UTF8String]);
    }
}

- (IBAction)didTouchStartButton:(id)sender
{
    NSString* appName = [appNameField text];
    [self.waitCondition lock];
    if (appName != nil && appName.length != 0) {
        QStatus status = ER_OK;
        if (!self.isConsumerStarted) {
            status = [self startDoorConsumer:appName];
            if (status == ER_OK) {
                [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
                self.isConsumerStarted = YES;
            }
        } else {
            [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
            self.isConsumerStarted = NO;
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

@end
