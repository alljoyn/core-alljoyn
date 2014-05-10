////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, 2014 AllSeen Alliance. All rights reserved.
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

#import "BusStressViewController.h"
#import "BusStressManager.h"

@interface BusStressViewController () <BusStressManagerDelegate>

- (void)didCompleteIteration:(NSInteger)iterationNumber totalIterations:(NSInteger)totalInterations;

@end

@implementation BusStressViewController
@synthesize numberOfThreadsLabel = _numberOfThreadsLabel;
@synthesize numberOfIterationsLabel = _numberOfIterationsLabel;
@synthesize stopThreadsBeforeJoinSwitch = _stopThreadsBeforeJoinSwitch;
@synthesize deleteBusAttachmentsSwitch = _deleteBusAttachmentsSwitch;
@synthesize startButton = _startButton;
@synthesize iterationsCompletedLabel = _iterationsCompletedLabel;
@synthesize iterationsCompletedProgressView = _iterationsCompletedProgressView;
@synthesize numberOfIterationsSlider = _numberOfIterationsSlider;
@synthesize numberOfThreadsSlider = _numberOfThreadsSlider;
@synthesize operationModeSegmentedControl = _operationModeSegmentedControl;
@synthesize stopButton = _stopButton;
@synthesize stressTestActivityIndicatorView = _stressTestActivityIndicatorView;

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)viewDidUnload
{
    [self setNumberOfIterationsLabel:nil];
    [self setNumberOfThreadsLabel:nil];
    [self setStopThreadsBeforeJoinSwitch:nil];
    [self setDeleteBusAttachmentsSwitch:nil];
    [self setStartButton:nil];
    [self setIterationsCompletedLabel:nil];
    [self setIterationsCompletedProgressView:nil];
    [self setNumberOfIterationsSlider:nil];
    [self setNumberOfThreadsSlider:nil];
    [self setOperationModeSegmentedControl:nil];
    [self setStopButton:nil];
    [self setStressTestActivityIndicatorView:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

#pragma mark - Table view data source

//- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
//{
//#warning Potentially incomplete method implementation.
//    // Return the number of sections.
//    return 0;
//}
//
//- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
//{
//#warning Incomplete method implementation.
//    // Return the number of rows in the section.
//    return 0;
//}
//
//- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
//{
//    static NSString *CellIdentifier = @"Cell";
//    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
//    
//    // Configure the cell...
//    
//    return cell;
//}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Navigation logic may go here. Create and push another view controller.
    /*
     <#DetailViewController#> *detailViewController = [[<#DetailViewController#> alloc] initWithNibName:@"<#Nib name#>" bundle:nil];
     // ...
     // Pass the selected object to the new view controller.
     [self.navigationController pushViewController:detailViewController animated:YES];
     */
}

- (IBAction)didTouchStartButton:(id)sender
{
    [BusStressManager runStress:(NSInteger)self.numberOfIterationsSlider.value threadCount:(NSInteger)self.numberOfThreadsSlider.value deleteBusFlag:self.deleteBusAttachmentsSwitch.isOn stopThreadsFlag:self.stopThreadsBeforeJoinSwitch.isOn operationMode:self.operationModeSegmentedControl.selectedSegmentIndex delegate:self];
    self.startButton.hidden = YES;
    self.stopButton.hidden = NO;
    [self.stressTestActivityIndicatorView startAnimating];
}

- (IBAction)didTouchStopButton:(id)sender
{
    [BusStressManager stopStress];
    self.startButton.hidden = NO;
    self.stopButton.hidden = YES;
    [self.stressTestActivityIndicatorView stopAnimating];
}

- (IBAction)numberOfIterationsValueChanged:(id)sender
{
    self.numberOfIterationsLabel.text = [NSString stringWithFormat:@"%d", (NSInteger)self.numberOfIterationsSlider.value];
}

- (IBAction)numberOfThreadsValueChanged:(id)sender
{
    self.numberOfThreadsLabel.text = [NSString stringWithFormat:@"%d", (NSInteger)self.numberOfThreadsSlider.value];
}

- (AJNTransportMask)transportMask
{
    return kAJNTransportMaskAny;
}

- (void)didCompleteIteration:(NSInteger)iterationNumber totalIterations:(NSInteger)totalInterations
{
    dispatch_async(dispatch_get_main_queue(), ^{
        self.iterationsCompletedProgressView.progress = (float)iterationNumber / (float)totalInterations;
        self.iterationsCompletedLabel.text = [NSString stringWithFormat:@"%d", iterationNumber];
    });
}

@end
