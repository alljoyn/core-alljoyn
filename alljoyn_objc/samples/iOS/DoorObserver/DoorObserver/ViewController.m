////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "ViewController.h"
#import "AppDelegate.h"
#import "AddDoorViewController.h"
#import "DoorObserverAllJoynService.h"

@interface ViewController ()

@property (nonatomic,strong) NSTimer* propertyPoller;
@property (strong, nonatomic) IBOutlet UITableView *doorTableView;
@property (strong, nonatomic) DoorObserverAllJoynService *doorObserverAllJoynService;

- (void)pollPropertiesOfAllDoorProxyObjects:(NSTimer *)timer;

@end

@implementation ViewController

#pragma mark -  UI

- (void)viewDidLoad
{
    [super viewDidLoad];

    AppDelegate* appDelegate = [[UIApplication sharedApplication] delegate];
    _doorObserverAllJoynService = appDelegate.doorObserverAllJoynService;
    _doorTableView.dataSource = _doorObserverAllJoynService;
    [_doorObserverAllJoynService setTableView:_doorTableView];
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

- (void)viewWillDisappear:(BOOL)animated
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
    //[self.bus deleteInterface: [self.bus interfaceWithName:kDoorInterfaceName]]; TODO:check this
}

#pragma mark - TableView protocols

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    [_doorObserverAllJoynService clickOnProxyDoorAtIndex:indexPath.row];
    [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
}

#pragma mark - Varia

- (IBAction)unwindToList:(UIStoryboardSegue *)segue
{
    [_doorTableView reloadData];
}

- (void)pollPropertiesOfAllDoorProxyObjects:(NSTimer *)timer
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [_doorTableView reloadData];
    });
}

@end
