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

#import "AJNCChatReceiver.h"
#import "AJNSessionOptions.h"
#import "AJNSignalHandler.h"

@interface AJNCChatObjectSignalHandler : NSObject<AJNSignalHandler, AJNChatReceiver>

@property (nonatomic, strong) id<AJNChatReceiver> delegate;

- (void)chatMessageReceived:(NSString *)message from:(NSString *)sender onObjectPath:(NSString *)path forSession:(AJNSessionId)sessionId;

@end