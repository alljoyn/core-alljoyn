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

#import <alljoyn/Message.h>
#import "AJNMessageHeaderFields.h"
#import "AJNMessageArgument.h"

@implementation AJNMessageHeaderFields

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (ajn::HeaderFields*)messageHeaderFields 
{
    return static_cast<ajn::HeaderFields*>(self.handle);
}

- (NSArray *)values
{
    NSMutableArray *theValues = [[NSMutableArray alloc] initWithCapacity:kAJNMessageHeaderFieldTypeFieldUnknown];
    for (int i = 0; i < kAJNMessageHeaderFieldTypeFieldUnknown; i++) {
        [theValues addObject:[[AJNMessageArgument alloc] initWithHandle:&(self.messageHeaderFields->field[i])]];
    }
    return theValues;
}

- (NSString *)stringValue
{
    return [NSString stringWithCString:self.messageHeaderFields->ToString().c_str() encoding:NSUTF8StringEncoding];
}

@end