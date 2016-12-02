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

#import "AJNCMessage.h"

@implementation AJNCMessage

@synthesize senderName = _senderName;
@synthesize dateTime = _dateTime;
@synthesize text = _text;

- (id)initWithText:(NSString*)text fromSender:(NSString*)senderName atDateTime:(NSString*)dateTime
{
    self = [super init];
    if (self) {
        self.senderName = senderName;
        self.dateTime = dateTime;
        self.text = text;
    }
    return self;
}

@end