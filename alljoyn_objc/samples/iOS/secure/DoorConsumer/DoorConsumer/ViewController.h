//
//  ViewController.h
//  SecureDoorConsumer
//
//  Created by Jorge Martinez on 9/19/16.
//  Copyright © 2016 AllseenAlliance. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

@property (weak, nonatomic) IBOutlet UITextView *textView;
@property (weak, nonatomic) IBOutlet UIButton *openButton;
@property (weak, nonatomic) IBOutlet UIButton *closeButton;
@property (weak, nonatomic) IBOutlet UILabel *stateLabel;
@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UITextField *appNameField;

- (void)didReceiveStatusUpdateMessage:(NSString *)message;
- (IBAction)didTouchOpenButton:(id)sender;
- (IBAction)didTouchCloseButton:(id)sender;
- (IBAction)didTouchStartButton:(id)sender;

@end

