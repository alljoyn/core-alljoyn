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

@interface AJNCMessage : NSObject

@property (nonatomic, strong) NSString *senderName;
@property (nonatomic, strong) NSString *dateTime;
@property (nonatomic, strong) NSString *text;

- (id)initWithText:(NSString *)text fromSender:(NSString *)senderName atDateTime:(NSString *)dateTime;

@end