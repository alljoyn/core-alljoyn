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

#import "AJNCChatObjectSignalHandler.h"
#import "AJNCChatObjectSignalHandlerImpl.h"

@implementation AJNCChatObjectSignalHandler

@synthesize handle = _handle;
@synthesize delegate = _delegate;

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new AJNCChatObjectSignalHandlerImpl(self);
    }
    return self;
}

- (void)chatMessageReceived:(NSString*)message from:(NSString*)sender onObjectPath:(NSString*)path forSession:(AJNSessionId)sessionId
{
    [self.delegate chatMessageReceived:message from:sender onObjectPath:path forSession:sessionId];
}


@end