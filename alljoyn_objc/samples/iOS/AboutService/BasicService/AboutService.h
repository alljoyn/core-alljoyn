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

@protocol AboutServiceDelegate <NSObject>

- (void)didReceiveStatusUpdateMessage:(NSString *)message;

@end

@interface AboutService : NSObject

@property (strong, nonatomic) id<AboutServiceDelegate> delegate;

- (void)startService;

@end