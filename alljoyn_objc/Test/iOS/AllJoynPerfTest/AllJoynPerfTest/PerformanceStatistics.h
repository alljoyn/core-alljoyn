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

#import <Foundation/Foundation.h>

@interface PerformanceStatistics : NSObject

@property (nonatomic, readonly) double megaBytesTransferred;
@property (nonatomic, readonly) double throughputInMegaBitsPerSecond;
@property (nonatomic, readonly) NSTimeInterval timeRequiredToCompleteDiscoveryOfService;
@property (nonatomic, readonly) NSTimeInterval timeRequiredToCompleteAllPacketTransfers;

- (int)packetTransfersCompleted;
- (int)totalPacketTransfersExpected;
- (int)packetSize;

- (void)markDiscoveryStartTime;
- (void)markDiscoveryEndTime;
- (void)markTransferStartTime;
- (void)markTransferEndTime;
- (void)resetStatisticsWithPacketSize:(int)kiloBitsPerPacket;
- (BOOL)shouldRefreshUserInterfaceForPacketAtIndex:(int)packetIndex;
- (void)incrementSuccessfulMethodCallsCount;
- (void)incrementSuccessfulSignalTransmissionsCount;

@end