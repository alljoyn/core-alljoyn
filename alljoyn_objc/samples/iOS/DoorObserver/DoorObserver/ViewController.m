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
#import "AJNObserver.h"
#import "AJNObserverListener.h"
#import "DoorObject.h"
#import "AppDelegate.h"
#import "AddDoorViewController.h"

static NSString * const kTmpDoorPathPrefix = @"/org/alljoyn/sample/door/";
static NSString * const kDoorInterfaceName = @"com.example.Door";

#pragma mark Extension
@interface ViewController () <AJNObserverListener>
@property (nonatomic,strong) AJNBusAttachment *bus;
@property (nonatomic, strong) AJNObserver *doorObserver;
@property (nonatomic, strong) NSMutableArray *doorProxies;
@property (nonatomic,strong) NSTimer* propertyPoller;
@property (strong, nonatomic) IBOutlet UITableView *DoorTableView;

- (void)pollPropertiesOfAllDoorProxyObjects:(NSTimer *)timer;
@end

#pragma mark -
@implementation ViewController

#pragma mark Door Observer

-(void)createDoorObserver
{
    self.doorProxies = [[NSMutableArray alloc] init];

    // register door interface
    QStatus status = ER_OK;
    AJNInterfaceDescription *interfaceDescription = [self.bus createInterfaceWithName:@"com.example.Door"];
    status = [interfaceDescription addPropertyWithName:@"IsOpen" signature:@"b"];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  IsOpen" userInfo:nil];
    }
    status = [interfaceDescription addPropertyWithName:@"Location" signature:@"s"];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  Location" userInfo:nil];
    }
    status = [interfaceDescription addPropertyWithName:@"KeyCode" signature:@"u"];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add property to interface:  KeyCode" userInfo:nil];
    }

    status = [interfaceDescription addMethodWithName:@"Open" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Open" userInfo:nil];
    }
    status = [interfaceDescription addMethodWithName:@"Close" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: Close" userInfo:nil];
    }
    status = [interfaceDescription addMethodWithName:@"KnockAndRun" inputSignature:@"" outputSignature:@"" argumentNames:[NSArray arrayWithObjects: nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add method to interface: KnockAndRun" userInfo:nil];
    }

    status = [interfaceDescription addSignalWithName:@"PersonPassedThrough" inputSignature:@"s" argumentNames:[NSArray arrayWithObjects:@"name", nil]];
    if (status != ER_OK && status != ER_BUS_MEMBER_ALREADY_EXISTS) {
        @throw [NSException exceptionWithName:@"BusObjectInitFailed" reason:@"Unable to add signal to interface:  PersonPassedThrough" userInfo:nil];
    }
    [interfaceDescription activate];

    self.doorObserver = [[AJNObserver alloc]initWithProxyType:[DoorObjectProxy class]
                                                busAttachment:self.bus
                                          mandatoryInterfaces:[NSArray arrayWithObject:kDoorInterfaceName]];

    [self.doorObserver registerObserverListener:self triggerOnExisting:YES];
}

#pragma mark - AJNObserverListener protocol

-(void)didDiscoverObject:(AJNProxyBusObject *)obj forObserver:(AJNObserver *)observer
{
    [self.doorProxies addObject:(DoorObjectProxy*)obj];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.DoorTableView reloadData];
    });
}

-(void)didLoseObject:(AJNProxyBusObject *)obj forObserver:(AJNObserver *)observer;
{
    // AJNProxyBusObject does not support hash and isEqual
    NSUInteger idx = 0;
    for (;idx<[self.doorProxies count];++idx){
        NSString *objBusName = [NSString stringWithString:((AJNBusAttachment *)[obj valueForKey:@"bus"]).uniqueName];
        NSString *itemBusName = [NSString stringWithString:((AJNBusAttachment *)[[self.doorProxies objectAtIndex:idx] valueForKey:@"bus"]).uniqueName];
        if ([obj.path isEqualToString:((AJNProxyBusObject *)[self.doorProxies objectAtIndex:idx]).path] &&
            [objBusName isEqualToString:itemBusName]){
            [self.doorProxies removeObjectAtIndex:idx];
            break;
        }
    }
}

#pragma mark - TableView protocols

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    return [self.doorProxies count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"DoorPrototypeCell" forIndexPath:indexPath];
    DoorObjectProxy *proxy = [self.doorProxies objectAtIndex:indexPath.row];
    cell.textLabel.text = proxy.Location;
    if (proxy.IsOpen) {
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
    } else {
        cell.accessoryType = UITableViewCellAccessoryNone;
    }
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    DoorObjectProxy *tappedItem = [self.doorProxies objectAtIndex:indexPath.row];
    if (tappedItem.IsOpen) {
        [tappedItem close];
    } else {
        [tappedItem open];
    }
    [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
}

#pragma mark - Varia

- (IBAction)unwindToList:(UIStoryboardSegue *)segue
{
    [self.DoorTableView reloadData];
}

- (void)pollPropertiesOfAllDoorProxyObjects:(NSTimer *)timer
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.DoorTableView reloadData];
    });
}

#pragma mark -  UI

- (void)viewDidLoad
{
    [super viewDidLoad];

    AppDelegate* appDelegate = [[UIApplication sharedApplication] delegate];
    self.bus = appDelegate.bus;
    [self createDoorObserver];
    self.propertyPoller = nil;
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    // Missing propertiesChange Event handling => Use polling for now
    if (nil == self.propertyPoller) {
        self.propertyPoller = [NSTimer timerWithTimeInterval:5.0
                                                      target:self
                                                    selector:@selector(pollPropertiesOfAllDoorProxyObjects:)
                                                    userInfo:nil
                                                     repeats:YES];
        NSRunLoop *runner = [NSRunLoop currentRunLoop];
        [runner addTimer:self.propertyPoller forMode: NSDefaultRunLoopMode];
    }
}

-(void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    // Stop polling
    if (nil != self.propertyPoller) {
        [self.propertyPoller invalidate];
        self.propertyPoller = nil;
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    [self.bus deleteInterfaceWithName:kDoorInterfaceName];
}

@end
