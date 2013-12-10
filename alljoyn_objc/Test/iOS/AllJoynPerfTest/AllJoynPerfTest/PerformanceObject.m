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
