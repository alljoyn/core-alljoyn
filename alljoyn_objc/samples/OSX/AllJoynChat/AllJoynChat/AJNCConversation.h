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
#import "AJNCBusObject.h"
#import "AJNSessionOptions.h"
#import "AJNBusAttachment.h"

typedef enum {
    kAJNCConversationTypeRemoteHost = 0,
    kAJNCConversationTypeLocalHost = 1
} AJNCConversationType;

@interface AJNCConversation : NSObject<AJNChatReceiver>

@property (nonatomic, strong) NSString *name;
@property (nonatomic, readonly) NSString *displayName;
@property (nonatomic) AJNSessionId identifier;
@property (nonatomic, strong) AJNCBusObject *chatObject;
@property (nonatomic, strong) NSMutableArray *messages;
@property (nonatomic) AJNCConversationType type;
@property (nonatomic) NSInteger totalParticipants;

- (id)initWithName:(NSString *)name identifier:(AJNSessionId)sessionId busObject:(AJNCBusObject *)chatObject;

@end