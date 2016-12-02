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

#import "AJNTransportMask.h"

typedef enum _BusStressManagerOperationMode {
    kBusStressManagerOperationModeNone = 0,
    kBusStressManagerOperationModeClient = 1,
    kBusStressManagerOperationModeService = 2
} BusStressManagerOperationMode;

@protocol BusStressManagerDelegate<NSObject>

- (void)didCompleteIteration:(NSInteger)iterationNumber totalIterations:(NSInteger)totalInterations;

- (AJNTransportMask)transportMask;

@end

@interface BusStressManager : NSObject

+ (void)runStress:(NSInteger)iterations threadCount:(NSInteger)threadCount deleteBusFlag:(BOOL)shouldDeleteBusAttachment stopThreadsFlag:(BOOL)stopThreads operationMode:(BusStressManagerOperationMode)mode delegate:(id<BusStressManagerDelegate>)delegate;

+ (void)stopStress;

@end