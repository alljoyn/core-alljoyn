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

@interface BusStressViewController : UITableViewController

@property (weak, nonatomic) IBOutlet UILabel *numberOfThreadsLabel;
@property (weak, nonatomic) IBOutlet UILabel *numberOfIterationsLabel;
@property (weak, nonatomic) IBOutlet UISwitch *stopThreadsBeforeJoinSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *deleteBusAttachmentsSwitch;
@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UILabel *iterationsCompletedLabel;
@property (weak, nonatomic) IBOutlet UIProgressView *iterationsCompletedProgressView;
@property (weak, nonatomic) IBOutlet UISlider *numberOfIterationsSlider;
@property (weak, nonatomic) IBOutlet UISlider *numberOfThreadsSlider;
@property (weak, nonatomic) IBOutlet UISegmentedControl *operationModeSegmentedControl;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *stressTestActivityIndicatorView;
@property (weak, nonatomic) IBOutlet UISegmentedControl *transportTypeSegmentedControl;
@property (nonatomic) BOOL hasTestStarted;

- (IBAction)didTouchStartButton:(id)sender;
- (IBAction)didTouchStopButton:(id)sender;
- (IBAction)numberOfIterationsValueChanged:(id)sender;
- (IBAction)numberOfThreadsValueChanged:(id)sender;

@end