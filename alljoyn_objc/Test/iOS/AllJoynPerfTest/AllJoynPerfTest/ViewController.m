////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, 2014 AllSeen Alliance. All rights reserved.
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
#import "AJNClientController.h"
#import "AJNServiceController.h"
#import "AJNMessageArgument.h"
#import "AJNInterfaceDescription.h"
#import "AJNPerformanceObject.h"
#import "PerformanceObject.h"
#import "Constants.h"

@interface ViewController () <AJNClientDelegate, AJNServiceDelegate, PerformanceObjectDelegateSignalHandler>

@property (nonatomic, strong) PerformanceObjectProxy *proxyObject;
@property (nonatomic, strong) PerformanceObject *performanceObject;
@property (nonatomic, strong) AJNClientController *clientController;
@property (nonatomic, strong) AJNServiceController *serviceController;

- (AJNProxyBusObject *)proxyObjectOnBus:(AJNBusAttachment *)bus inSession:(AJNSessionId)sessionId;
- (void)shouldUnloadProxyObjectOnBus:(AJNBusAttachment *)bus;
- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus;
- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus;

- (void)didStartBus:(AJNBusAttachment *)bus;

@end

@implementation ViewController

@synthesize proxyObject = _proxyObject;
@synthesize clientController = _clientController;
@synthesize handle = _handle;
@synthesize performanceStatistics = _performanceStatistics;

- (NSString *)applicationName
{
    return kAppName;
}

- (NSString *)serviceName
{
    return kServiceName;
}

- (AJNBusNameFlag)serviceNameFlags
{
    return kAJNBusNameFlagDoNotQueue | kAJNBusNameFlagReplaceExisting;
}

- (AJNSessionPort)sessionPort
{
    return kServicePort;
}

- (AJNBusObject *)object
{
    return self.performanceObject;
}

- (PerformanceStatistics *)performanceStatistics
{
    if (_performanceStatistics == nil) {
        _performanceStatistics = [[PerformanceStatistics alloc] init];
    }
    return _performanceStatistics;
}

- (void)setUIControlsEnabled:(BOOL)shouldEnable
{
    [self.operationTypeSegmentedControl setEnabled:shouldEnable];
    [self.transportTypeSegmentedControl setEnabled:shouldEnable];
    [self.messageTypeSegmentedControl setEnabled:shouldEnable];
    [self.messageSizeSegmentedControl setEnabled:shouldEnable];
    [self.headerCompressionSwitch setEnabled:shouldEnable];
    [self.operationTypeSegmentedControl setEnabled:shouldEnable];
}

- (void)resetPerformanceStatistics
{
    int kilobitsPerTransfer;
    
    switch(self.messageSizeSegmentedControl.selectedSegmentIndex) {
        case 0:
            kilobitsPerTransfer = 1;
            break;
        case 1:
            kilobitsPerTransfer = 10;
            break;
        case 2:
            kilobitsPerTransfer = 50;
            break;
        case 3:
            kilobitsPerTransfer = 100;
            break;
        case 4:
            kilobitsPerTransfer = 1000;
            break;
    }

    [self.performanceStatistics resetStatisticsWithPacketSize:kilobitsPerTransfer];
    
    self.progressView.progress = 0;
    self.statusTextField1.text = nil;
    self.statusTextField2.text = nil;
}

- (void)displayPerformanceStatistics
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self.operationTypeSegmentedControl.selectedSegmentIndex == 0) {
            self.statusTextField1.text = [NSString stringWithFormat:@"Transfer: %.2fs", self.performanceStatistics.timeRequiredToCompleteAllPacketTransfers];
            self.statusTextField2.text = [NSString stringWithFormat:@"Total: %.2f MB (%.2f Mbps) %d/%d", self.performanceStatistics.megaBytesTransferred, self.performanceStatistics.throughputInMegaBitsPerSecond, self.performanceStatistics.packetTransfersCompleted, self.performanceStatistics.totalPacketTransfersExpected];
        }
        else if (self.operationTypeSegmentedControl.selectedSegmentIndex == 1) {
            self.statusTextField1.text = [NSString stringWithFormat:@"Start/Discovery: %.2fs  Transfer: %.2fs", self.performanceStatistics.timeRequiredToCompleteDiscoveryOfService, self.performanceStatistics.timeRequiredToCompleteAllPacketTransfers];
            self.statusTextField2.text = [NSString stringWithFormat:@"Total: %.2f MB (%.2f Mbps) %d/%d", self.performanceStatistics.megaBytesTransferred, self.performanceStatistics.throughputInMegaBitsPerSecond, self.performanceStatistics.packetTransfersCompleted, self.performanceStatistics.totalPacketTransfersExpected];
        }
    });
}

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

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source
/*
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
#warning Potentially incomplete method implementation.
    // Return the number of sections.
    return 0;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
#warning Incomplete method implementation.
    // Return the number of rows in the section.
    return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    
    // Configure the cell...
    
    return cell;
}
*/
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
    
    // empty the text view that contains the status message log
    //
    dispatch_async(dispatch_get_main_queue(), ^{
        self.eventsTextView.text = nil;
    });
    
    
    // determine the transport mask used based on the selection in the UI
    //
    AJNTransportMask transportMask;
    
    if (self.transportTypeSegmentedControl.selectedSegmentIndex == 0) {
        transportMask = kAJNTransportMaskAny;
    }
    
    // reset the performance statistics for the operations (signal or method)
    //
    [self resetPerformanceStatistics];
    
    
    // depending on the UI selection, start/stop either a client or service
    //
    if (self.operationTypeSegmentedControl.selectedSegmentIndex == 0) {
        
        // start or stop the service controller
        //
        if (self.serviceController.bus.isStarted) {
            [self.serviceController stop];
            [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
            [self setUIControlsEnabled:YES];
            self.serviceController = nil;
        }
        else {
            // allocate the service bus controller and set bus properties
            //
            [self.serviceController stop];
            self.serviceController = nil;
            self.serviceController = [[AJNServiceController alloc] init];
            self.serviceController.trafficType = kAJNTrafficMessages;
            self.serviceController.proximityOptions = kAJNProximityAny;
            self.serviceController.transportMask = transportMask;
            self.serviceController.allowRemoteMessages = YES;
            self.serviceController.multiPointSessionsEnabled = NO;
            self.serviceController.delegate = self;
            [self.serviceController start];
            [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
            [self setUIControlsEnabled:NO];            
        }
    }
    else if (self.operationTypeSegmentedControl.selectedSegmentIndex == 1) {
        
        // start or stop the client controller
        //
        if (self.clientController.bus.isStarted) {
            [self.clientController stop];
            [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
            [self setUIControlsEnabled:YES];
            self.clientController = nil;
        }
        else {
            // allocate the client bus controller and set bus properties
            //
            [self.clientController stop];
            self.clientController = nil;
            self.clientController = [[AJNClientController alloc] init];
            self.clientController.trafficType = kAJNTrafficMessages;
            self.clientController.proximityOptions = kAJNProximityAny;
            self.clientController.transportMask = transportMask;
            self.clientController.allowRemoteMessages = YES;
            self.clientController.multiPointSessionsEnabled = NO;
            self.clientController.delegate = self;

            [self.performanceStatistics markDiscoveryStartTime];
            
            [self.clientController start];
            
            [self.startButton setTitle:@"Stop" forState:UIControlStateNormal];
            [self setUIControlsEnabled:NO];
        }
    }
}

#pragma mark - Client Controller delegate methods

- (void)shouldUnloadProxyObjectOnBus:(AJNBusAttachment *)bus
{
    self.proxyObject = nil;
    [self setUIControlsEnabled:YES];
    [self.startButton setTitle:@"Start" forState:UIControlStateNormal];    
}

- (AJNProxyBusObject *)proxyObjectOnBus:(AJNBusAttachment *)bus inSession:(AJNSessionId)sessionId
{
    self.proxyObject = [[PerformanceObjectProxy alloc] initWithBusAttachment:bus serviceName:self.serviceName objectPath:kServicePath sessionId:sessionId];
    
    return self.proxyObject;
}

- (void)didStartBus:(AJNBusAttachment *)bus
{
    if (self.operationTypeSegmentedControl.selectedSegmentIndex == 1) {
        
        // register our signal handler
        //
        AJNInterfaceDescription *interfaceDescription;
        
        //
        // PerformanceObjectDelegate interface (org.alljoyn.bus.test.perf.both)
        //
        // create an interface description, or if that fails, get the interface as it was already created
        //
        interfaceDescription = [self.clientController.bus createInterfaceWithName:@"org.alljoyn.bus.test.perf.both"];
        
        
        // add the methods to the interface description
        //
        
        QStatus status = [interfaceDescription addMethodWithName:@"CheckPacket" inputSignature:@"iayi" outputSignature:@"b" argumentNames:[NSArray arrayWithObjects:@"packetIndex",@"byteArray",@"packetSize",@"result", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: CheckPacket" userInfo:nil];
        }
        
        // add the signals to the interface description
        //
        
        status = [interfaceDescription addSignalWithName:@"SendPacket" inputSignature:@"iay" argumentNames:[NSArray arrayWithObjects:@"packetIndex",@"byteArray", nil]];
        
        if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
            @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  SendPacket" userInfo:nil];
        }
        
        
        [interfaceDescription activate];
        
        [self.clientController.bus registerPerformanceObjectDelegateSignalHandler:self];
    }
}

- (void)didJoinInSession:(AJNSessionId)sessionId withService:(NSString *)serviceName
{
    [self.performanceStatistics markDiscoveryEndTime];
    
    // if this is a receiver and the message type selected is method
    // then begin calling methods on the remote service's bus object
    // using the proxy
    //
    if (self.operationTypeSegmentedControl.selectedSegmentIndex == 1 &&
        self.messageTypeSegmentedControl.selectedSegmentIndex == 1) {
        
        // begin making method calls using the proxy object on a background
        // thread
        //
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            
            [self.performanceStatistics markTransferStartTime];
            
            for (int i = 0; i < self.performanceStatistics.totalPacketTransfersExpected; i++) {
                AJNMessageArgument *byteArrayArg = [[AJNMessageArgument alloc] init];
                UInt8 *byteArray = malloc(self.performanceStatistics.packetSize);
                memset(byteArray, 0, self.performanceStatistics.packetSize);
                
                byteArray[0] = 'g';
                byteArray[1] = 'p';
                byteArray[2] = 'a';
                
                [byteArrayArg setValue:@"ay", self.performanceStatistics.packetSize, byteArray];
                
                if(![self.proxyObject checkPacketAtIndex:[NSNumber numberWithInt:i] payLoad:byteArrayArg packetSize:[NSNumber numberWithInt:self.performanceStatistics.packetSize]]){
                    break;
                }
                
                free(byteArray);
                
                [self.performanceStatistics incrementSuccessfulMethodCallsCount];
                
                if([self.performanceStatistics shouldRefreshUserInterfaceForPacketAtIndex:i]) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        float progress = (i + 1.0) / (float)self.performanceStatistics.totalPacketTransfersExpected;
                        NSLog(@"Progress=%f", progress);
                        [self.progressView setProgress:progress];
                        [self.performanceStatistics markTransferEndTime];
                        [self displayPerformanceStatistics];
                    });
                }
            }
            [self.performanceStatistics markTransferEndTime];
            [self displayPerformanceStatistics];
        });
    }
}

- (void)didReceiveStatusMessage:(NSString*)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.eventsTextView.text.length ? [self.eventsTextView.text mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@]\n",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [string appendString:@"\n\n"];
        [self.eventsTextView setText:string];
        NSLog(@"%@",message);
        [self.eventsTextView scrollRangeToVisible:NSMakeRange([self.eventsTextView.text length], 0)];
    });
}

#pragma mark - Client Controller - Performance Object Signal Handler implementation

- (void)didReceivePacketAtIndex:(NSNumber *)packetIndex payLoad:(AJNMessageArgument *)byteArray inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        [self.performanceStatistics incrementSuccessfulSignalTransmissionsCount];
        
        uint8_t *buffer;
        size_t bufferLength;
        [byteArray value:@"ay", &bufferLength, &buffer];

        if (self.performanceStatistics.packetTransfersCompleted == 1) {
            // this is the first time we have received a packet for this session
            [self.performanceStatistics markTransferStartTime];
        }
        else if(self.performanceStatistics.totalPacketTransfersExpected == [packetIndex intValue] + 1) {
            [self.performanceStatistics markTransferEndTime];                            
            dispatch_async(dispatch_get_main_queue(), ^{
                float progress = self.performanceStatistics.packetTransfersCompleted / (float)self.performanceStatistics.totalPacketTransfersExpected;
                [self.progressView setProgress:progress];
                [self displayPerformanceStatistics];
                NSLog(@"Updating UI for packet at index %@ progress %f", packetIndex, progress);
            });
            //[self didTouchStartButton:self];
        }
        else if([self.performanceStatistics shouldRefreshUserInterfaceForPacketAtIndex:[packetIndex intValue]]) {
            [self.performanceStatistics markTransferEndTime];            
            dispatch_async(dispatch_get_main_queue(), ^{
                float progress = self.performanceStatistics.packetTransfersCompleted / (float)self.performanceStatistics.totalPacketTransfersExpected;
                [self.progressView setProgress:progress];
                [self displayPerformanceStatistics];
                NSLog(@"Updating UI for packet at index %@ progress %f", packetIndex, progress);
            });
        }
    });
}

#pragma mark - Service Controller delegate methods

- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus
{
    self.performanceObject = nil;
    [self.startButton setTitle:@"Start" forState:UIControlStateNormal];
    [self setUIControlsEnabled:YES];
}

- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus
{
    self.performanceObject = [[PerformanceObject alloc] initWithBusAttachment:self.serviceController.bus onPath:kServicePath];
    self.performanceObject.viewController = self;
    
    return self.performanceObject;
}

- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    [self.serviceController.bus enableConcurrentCallbacks];

    // if this is a sender and the messages to send are signals
    // then start sending packets as signals to the remote client
    //
    if (self.operationTypeSegmentedControl.selectedSegmentIndex == 0 &&
        self.messageTypeSegmentedControl.selectedSegmentIndex == 0) {
        
        // start sending packets to the client that joined
        //
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [self.performanceStatistics markTransferStartTime];
            for (int i = 0; i < self.performanceStatistics.totalPacketTransfersExpected; i++) {
                AJNMessageArgument *byteArrayArg = [[AJNMessageArgument alloc] init];
                UInt8 *byteArray = malloc(self.performanceStatistics.packetSize);
                memset(byteArray, 0, self.performanceStatistics.packetSize);
                
                byteArray[0] = 'g';
                byteArray[1] = 'p';
                byteArray[2] = 'a';
                
                [byteArrayArg setValue:@"ay", self.performanceStatistics.packetSize, byteArray];
                
                [self.performanceObject sendPacketAtIndex:[NSNumber numberWithInt:i] payLoad:byteArrayArg inSession:self.serviceController.sessionId toDestination:nil flags:self.headerCompressionSwitch.isOn ? kAJNMessageFlagCompressed : 0];
                
                free(byteArray);
                
                [self.performanceStatistics incrementSuccessfulSignalTransmissionsCount];
                
                if([self.performanceStatistics shouldRefreshUserInterfaceForPacketAtIndex:i]) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        float progress = (i + 1.0) / (float)self.performanceStatistics.totalPacketTransfersExpected;
                        [self.performanceStatistics markTransferEndTime];
                        NSLog(@"Progress=%f", progress);
                        [self.progressView setProgress:progress];
                        [self displayPerformanceStatistics];
                    });
                }
            }
            [self.performanceStatistics markTransferEndTime];
            [self displayPerformanceStatistics];
        });
    }
}

@end
