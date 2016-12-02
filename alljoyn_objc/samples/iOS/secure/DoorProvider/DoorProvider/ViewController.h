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

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UIButton *signalButton;
@property (weak, nonatomic) IBOutlet UITextField *appNameField;
@property (weak, nonatomic) IBOutlet UITextView *textView;
@property (weak, nonatomic) IBOutlet UISwitch *autoSignalSwitch;

- (IBAction)didTouchStartButton:(id)sender;

- (IBAction)didTouchSignalButton:(id)sender;

- (IBAction)didToggleAutoSignal:(id)sender;

- (void)didReceiveStatusUpdateMessage:(NSString *)message;

@end