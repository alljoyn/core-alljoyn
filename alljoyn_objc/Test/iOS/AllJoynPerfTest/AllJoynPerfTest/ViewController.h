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
#import "PerformanceStatistics.h"

@interface ViewController : UITableViewController

@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UITextView *eventsTextView;
@property (weak, nonatomic) IBOutlet UIProgressView *progressView;
@property (weak, nonatomic) IBOutlet UISegmentedControl *transportTypeSegmentedControl;
@property (weak, nonatomic) IBOutlet UISegmentedControl *messageTypeSegmentedControl;
@property (weak, nonatomic) IBOutlet UISegmentedControl *messageSizeSegmentedControl;
@property (weak, nonatomic) IBOutlet UISegmentedControl *operationTypeSegmentedControl;
@property (weak, nonatomic) IBOutlet UILabel *statusTextField1;
@property (weak, nonatomic) IBOutlet UILabel *statusTextField2;

@property (nonatomic, strong) PerformanceStatistics *performanceStatistics;

- (IBAction)didTouchStartButton:(id)sender;
- (void)displayPerformanceStatistics;
- (void)resetPerformanceStatistics;

@end