//
//  ViewController.h
//  DoorProvider
//
//  Created by Jorge Martinez on 9/21/16.
//  Copyright © 2016 AllseenAlliance. All rights reserved.
//

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