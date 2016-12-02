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
////////////////////////////////////////////////////////////////////////////////
//
//  ALLJOYN MODELING TOOL - GENERATED CODE
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  PerformanceObject.m
//
////////////////////////////////////////////////////////////////////////////////

#import "PerformanceObject.h"
#import "AJNMessageArgument.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for PerformanceObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation PerformanceObject

- (BOOL)checkPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray packetSize:(NSNumber*)packetSize message:(AJNMessage *)methodCallMessage
{
    uint8_t *buffer;
    size_t bufferLength;
    [byteArray value:@"ay", &bufferLength, &buffer];

    PerformanceStatistics *performanceStatistics = self.viewController.performanceStatistics;
    [performanceStatistics incrementSuccessfulMethodCallsCount];
    
    if (performanceStatistics.packetTransfersCompleted == 1) {
        // this is the first time we have received a packet for this session
        [performanceStatistics markTransferStartTime];
    }
    else if(performanceStatistics.totalPacketTransfersExpected == [packetIndex intValue] + 1) {
        [performanceStatistics markTransferEndTime];
        dispatch_async(dispatch_get_main_queue(), ^{
            float progress = performanceStatistics.packetTransfersCompleted / (float)performanceStatistics.totalPacketTransfersExpected;
            [self.viewController.progressView setProgress:progress];
            [self.viewController displayPerformanceStatistics];
            NSLog(@"Updating UI for packet at index %@ progress %f", packetIndex, progress);
        });
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.viewController didTouchStartButton:self];
        });
    }
    else if([performanceStatistics shouldRefreshUserInterfaceForPacketAtIndex:[packetIndex intValue]]) {
        [performanceStatistics markTransferEndTime];        
        dispatch_async(dispatch_get_main_queue(), ^{
            float progress = performanceStatistics.packetTransfersCompleted / (float)performanceStatistics.totalPacketTransfersExpected;
            [self.viewController.progressView setProgress:progress];
            [self.viewController displayPerformanceStatistics];
            NSLog(@"Updating UI for packet at index %@ progress %f", packetIndex, progress);
        });
    }

    return buffer != NULL && bufferLength == [packetSize longValue];
}


@end

////////////////////////////////////////////////////////////////////////////////