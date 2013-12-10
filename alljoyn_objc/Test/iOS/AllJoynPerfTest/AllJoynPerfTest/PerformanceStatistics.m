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

#import "PerformanceStatistics.h"

@interface PerformanceStatistics()

@property (nonatomic) int bitsPerByte;
@property (nonatomic) int totalDataSize;
@property (nonatomic) int packetSize;
@property (nonatomic) int packetTransfersCompleted;
@property (nonatomic) int totalPacketTransfersExpected;
@property (nonatomic) int refreshUIInterval;

@property (nonatomic, strong) NSDate *discoveryStartTime;
@property (nonatomic, strong) NSDate *discoveryEndTime;
@property (nonatomic) NSTimeInterval discoveryTimespan;

@property (nonatomic, strong) NSDate *startTime;
@property (nonatomic, strong) NSDate *endTime;
@property (nonatomic) NSTimeInterval timespan;

@property (nonatomic) int successfulSignalsCount;
@property (nonatomic) int successfulMethodCallsCount;

@end

@implementation PerformanceStatistics

- (double)megaBytesTransferred
{
    return self.packetSize * self.packetTransfersCompleted / 1024.0 / 1024.0;
}

- (double)throughputInMegaBitsPerSecond
{
    return self.packetSize * self.packetTransfersCompleted * self.bitsPerByte / 1000000.0 / self.timespan;
}

- (NSTimeInterval)timeRequiredToCompleteDiscoveryOfService
{
    return self.discoveryTimespan;
}

- (NSTimeInterval)timeRequiredToCompleteAllPacketTransfers
{
    return self.timespan;
}

- (void)markDiscoveryStartTime
{
    self.discoveryStartTime = [NSDate date];
}

- (void)markDiscoveryEndTime
{
    self.discoveryEndTime = [NSDate date];
    self.discoveryTimespan = [self.discoveryEndTime timeIntervalSinceDate:self.discoveryStartTime];
}

- (void)markTransferStartTime
{
    self.startTime = [NSDate date];
}

- (void)markTransferEndTime
{
    self.endTime = [NSDate date];
    self.timespan = [self.endTime timeIntervalSinceDate:self.startTime];
}

- (void)resetStatisticsWithPacketSize:(int)kilobitsPerPacket
{
    self.bitsPerByte = 8;
    self.totalDataSize = 100 * 1000 * 1000 / self.bitsPerByte;
    self.packetSize = kilobitsPerPacket * 1000 / self.bitsPerByte;
    self.packetTransfersCompleted = self.totalDataSize / self.packetSize;
    self.totalPacketTransfersExpected = self.packetTransfersCompleted + 1;
    if (self.packetTransfersCompleted > 100) {
        self.refreshUIInterval = self.packetTransfersCompleted / 100;
    }
    else {
        self.refreshUIInterval = 2;
    }
    self.discoveryStartTime = nil;
    self.discoveryEndTime = nil;
    self.discoveryTimespan = 0;
    self.startTime = nil;
    self.endTime = nil;
    self.timespan = 0;
    self.successfulSignalsCount = 0;
    self.successfulMethodCallsCount = 0;
    self.packetTransfersCompleted = 0;
}

- (BOOL)shouldRefreshUserInterfaceForPacketAtIndex:(int)packetIndex
{
    return (packetIndex % self.refreshUIInterval) == 0 || packetIndex == self.totalPacketTransfersExpected - 1;
}

- (void)incrementSuccessfulMethodCallsCount
{
    self.packetTransfersCompleted++;
    self.successfulMethodCallsCount++;
}

- (void)incrementSuccessfulSignalTransmissionsCount
{
    self.packetTransfersCompleted++;
    self.successfulSignalsCount++;
}

@end
