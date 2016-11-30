////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
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
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <XCTest/XCTest.h>
#import "AJNBusAttachment.h"
#import "AutoPingerTests.h"
#import "AJNPingListener.h"
#import "AJNAutoPinger.h"
#import "AJNInit.h"

@interface TestPingListener : NSObject

@end

@interface TestPingListener() <AJNPingListener>

@property (nonatomic, strong) NSMutableArray *found;

@property (nonatomic, strong) NSMutableArray *lost;

@property (atomic, strong) NSLock *foundMutex;

@property (atomic, strong) NSLock *lostMutex;

- (void)destinationLost:(NSString*)group forDestination:(NSString*)destination;

- (void)destinationFound:(NSString*)group forDestination:(NSString*)destination;

- (BOOL)waitUntilFound:(NSString*)uniqueName forTime:(NSTimeInterval)timeoutSeconds;

- (BOOL)waitUntilLost:(NSString*)uniqueName forTime:(NSTimeInterval)timeoutSeconds;

@end

@implementation TestPingListener

- (id)init
{
    self = [super init];
    if (self) {
        self.foundMutex = [[NSLock alloc] init];
        self.lostMutex = [[NSLock alloc] init];
        self.lost = [[NSMutableArray alloc] init];
        self.found = [[NSMutableArray alloc] init];
    }
    
    return self;
}

- (void)destinationLost:(NSString*)group forDestination:(NSString*)destination
{
    NSLog(@"on lost %@", group);

    [self.lostMutex lock];
    [self.lost addObject:destination];
    [self.lostMutex unlock];
}

- (void)destinationFound:(NSString*)group forDestination:(NSString*)destination
{
    NSLog(@"on found %@", group);

    [self.foundMutex lock];
    [self.found addObject:destination];
    [self.foundMutex unlock];

}

- (BOOL)waitUntilFound:(NSString*)uniqueName forTime:(NSTimeInterval)timeoutSeconds
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSeconds];
    
    BOOL didFind = NO;
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if ([self.found containsObject:uniqueName]) {
            didFind = YES;
            break;
        }
        if ([timeoutDate timeIntervalSinceNow] < 0.0) {
            break;
        }
    } while (true);
    
    return didFind;
}

- (BOOL)waitUntilLost:(NSString*)uniqueName forTime:(NSTimeInterval)timeoutSeconds
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSeconds];
    
    BOOL didLose = NO;
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if ([self.lost containsObject:uniqueName]) {
            didLose = YES;
            break;
        }
        if ([timeoutDate timeIntervalSinceNow] < 0.0) {
            break;
        }
    } while (true);
    
    return didLose;
}

@end

@interface AutoPingerTests()

@property (nonatomic, strong) AJNBusAttachment *serviceBus;

@property (nonatomic, strong) AJNAutoPinger *autoPinger;

@end


@implementation AutoPingerTests

//- (id)init
//{
//    self = [super init];
//    if (self) {
//        self.serviceBus = [[AJNBusAttachment alloc] initWithApplicationName:@"AutoPingerTest" allowRemoteMessages:YES];
//        XCTAssertNotNil(self.serviceBus, @"Could not allocate AutoPingerTest BusAttachment");
//    }
//
//    return self;
//}

+(void)setUp
{
    [AJNInit alljoynInit];
    [AJNInit alljoynRouterInit];
}

+(void)tearDown
{
    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];
}

- (void)setUp
{
    [super setUp];
//    
//    [AJNInit alljoynInit];
//    [AJNInit alljoynRouterInit];
    
    self.serviceBus = [[AJNBusAttachment alloc] initWithApplicationName:@"AutoPingerTest" allowRemoteMessages:YES];
    self.autoPinger = [[AJNAutoPinger alloc] initWithBusAttachment:self.serviceBus];
    
    QStatus status = ER_OK;
    status = [self.serviceBus start];
    XCTAssertEqual(ER_OK, status, @"Bus could not start");
    XCTAssertFalse([self.serviceBus isConnected], @"Bus is already connected");
    
    status = [self.serviceBus connectWithArguments:@"null"];
    XCTAssertEqual(ER_OK, status, @"Bus could not connect");
    XCTAssertTrue([self.serviceBus isConnected], @"Bus could not connect");
}

- (void)tearDown
{
    [self.serviceBus disconnect];
    [self.serviceBus stop];
    [self.serviceBus waitUntilStopCompleted];
    [self.serviceBus destroy];
    self.serviceBus = nil;
    
//    [AJNInit alljoynRouterShutdown];
//    [AJNInit alljoynShutdown];
    
    [super tearDown];
}

- (void)testBasicAutoPinger
{
    QStatus status = ER_OK;
    AJNBusAttachment *clientBus = [[AJNBusAttachment alloc] initWithApplicationName:@"app" allowRemoteMessages:YES];
    
    status = [clientBus start];
    XCTAssert(status == ER_OK, @"Unable to start client bus.");
    
    status = [clientBus connectWithArguments:@"null"];
    XCTAssert(status == ER_OK, @"Client bus unable to connect with null arguments.");

    TestPingListener *listener = [[TestPingListener alloc] init];
    
    [self.autoPinger addPingGroup:@"testgroup" withDelegate:listener withInterval:2];
    NSString *uniqueName = [clientBus uniqueName];
    
    status = [self.autoPinger addDestination:@"badgroup" forDestination:uniqueName];
    XCTAssert(status == ER_BUS_PING_GROUP_NOT_FOUND, @"Didn't find expected group not found status.");
    
    status = [self.autoPinger addDestination:@"testgroup" forDestination:uniqueName];
    XCTAssert(status == ER_OK, @"Unable to add first destination to auto pinger.");
    
    status = [self.autoPinger addDestination:@"testgroup" forDestination:uniqueName];
    XCTAssert(status == ER_OK, @"Unable to add second destination to auto pinger.");
    
    XCTAssertTrue([listener waitUntilFound:uniqueName forTime:10], @"Listener didn't find client bus");
    
    [clientBus disconnect];
    XCTAssertTrue([listener waitUntilLost:uniqueName forTime:10], @"Listener didn't lost client bus");
    
    [self.autoPinger pause];
    [self.autoPinger pause];
    [self.autoPinger resume];
    [self.autoPinger resume];
    
    status = [clientBus connectWithArguments:@"null"];
    XCTAssert(status == ER_OK, @"Client bus unable to connect with null arguments.");
    
    uniqueName = [clientBus uniqueName];
    
    status = [self.autoPinger addDestination:@"testgroup" forDestination:uniqueName];
    XCTAssert(status == ER_OK, @"Unable to add destination to auto pinger.");

    XCTAssertTrue([listener waitUntilFound:uniqueName forTime:10], @"Listener didn't find client bus");
    
    [self.autoPinger removePingGroup:@"badgroup"];
    [self.autoPinger removePingGroup:@"testgroup"];
    [self.autoPinger pause];
    self.autoPinger = nil;
    
    [clientBus disconnect];
    [clientBus stop];
    [clientBus waitUntilStopCompleted];
    [clientBus destroy];
    clientBus = nil;
}

@end