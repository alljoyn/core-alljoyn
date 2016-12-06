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
#import "AutoPingerTest.h"
#import "AJNPingListener.h"
#import "AJNAutoPinger.h"

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
    }
    
    return self;
}

- (void)destinationLost:(NSString*)group forDestination:(NSString*)destination
{
    NSLog(@"on lost %@", group);
    [self.foundMutex lock];
    [self.found addObject:destination];
    [self.foundMutex unlock];
}

- (void)destinationFound:(NSString*)group forDestination:(NSString*)destination
{
    NSLog(@"on found %@", group);
    [self.lostMutex lock];
    [self.lost addObject:destination];
    [self.lostMutex unlock];
}

- (BOOL)waitUntilFound:(NSString*)uniqueName forTime:(NSTimeInterval)timeoutSeconds
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSeconds];
    
    BOOL didFind = NO;
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if ([timeoutDate timeIntervalSinceNow] < 0.0) {
            break;
        }
        if (![self.found containsObject:uniqueName]) {
            didFind = YES;
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
        if ([timeoutDate timeIntervalSinceNow] < 0.0) {
            break;
        }
        if (![self.lost containsObject:uniqueName]) {
            didLose = YES;
            break;
        }
    } while (true);
    
    return didLose;
}

@end

@interface AutoPingerTests : XCTestCase

@property (nonatomic, strong) AJNBusAttachment *serviceBus;

@property (nonatomic, strong) AJNAutoPinger *autoPinger;

@end


@implementation AutoPingerTests

- (id)init
{
    self = [super init];
    if (self) {
        self.serviceBus = [[AJNBusAttachment alloc] initWithApplicationName:@"AutoPingerTest" allowRemoteMessages:YES];
        XCTAssertNotNil(self.serviceBus, @"Could not allocate AutoPingerTest BusAttachment");
    }

    return self;
}

- (void)setUp
{
    [super setUp];
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
    [super tearDown];
    [self.serviceBus disconnect];
    [self.serviceBus stop];
    [self.serviceBus waitUntilStopCompleted];
}

- (void)testBasicAutoPinger
{
    AJNBusAttachment *clientBus = [[AJNBusAttachment alloc] initWithApplicationName:@"app" allowRemoteMessages:YES];
    XCTAssertEqual(ER_OK, [clientBus start]);
    XCTAssertEqual(ER_OK, [clientBus connectWithArguments:@"null"]);

    TestPingListener *listener = [[TestPingListener alloc] init];
    
    [self.autoPinger addPingGroup:@"testgroup" withDelegate:listener withInterval:5];
    NSString *uniqueName = [clientBus uniqueName];
    XCTAssertEqual(ER_BUS_PING_GROUP_NOT_FOUND, [self.autoPinger addDestination:@"badgroup" forDestination:uniqueName]);
    XCTAssertEqual(ER_OK, [self.autoPinger addDestination:@"testgroup" forDestination:uniqueName]);
    XCTAssertEqual(ER_OK, [self.autoPinger addDestination:@"testgroup" forDestination:uniqueName]);
    
    XCTAssertTrue([listener waitUntilFound:uniqueName forTime:10]);
    [clientBus disconnect];
    XCTAssertTrue([listener waitUntilLost:uniqueName forTime:10]);
    
    [self.autoPinger pause];
    [self.autoPinger pause];
    [self.autoPinger resume];
    [self.autoPinger resume];
    
    [clientBus connectWithArguments:@"null:"];
    
    uniqueName = [clientBus uniqueName];
    XCTAssertEqual(ER_OK, [self.autoPinger addDestination:@"testgroup" forDestination:uniqueName]);

    [listener waitUntilFound:uniqueName forTime:10];
    
    [self.autoPinger removePingGroup:@"badgroup"];
    [self.autoPinger removePingGroup:@"testgroup"];
    
    [clientBus disconnect];
    [clientBus stop];
    [clientBus waitUntilStopCompleted];

}

@end