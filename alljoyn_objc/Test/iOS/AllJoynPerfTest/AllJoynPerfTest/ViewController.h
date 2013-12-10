////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#import <UIKit/UIKit.h>
#import "PerformanceStatistics.h"

@interface ViewController : UITableViewController

@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UITextView *eventsTextView;
@property (weak, nonatomic) IBOutlet UIProgressView *progressView;
@property (weak, nonatomic) IBOutlet UISegmentedControl *transportTypeSegmentedControl;
@property (weak, nonatomic) IBOutlet UISegmentedControl *messageTypeSegmentedControl;
@property (weak, nonatomic) IBOutlet UISegmentedControl *messageSizeSegmentedControl;
@property (weak, nonatomic) IBOutlet UISwitch *headerCompressionSwitch;
@property (weak, nonatomic) IBOutlet UISegmentedControl *operationTypeSegmentedControl;
@property (weak, nonatomic) IBOutlet UILabel *statusTextField1;
@property (weak, nonatomic) IBOutlet UILabel *statusTextField2;

@property (nonatomic, strong) PerformanceStatistics *performanceStatistics;

- (IBAction)didTouchStartButton:(id)sender;
- (void)displayPerformanceStatistics;
- (void)resetPerformanceStatistics;

@end
