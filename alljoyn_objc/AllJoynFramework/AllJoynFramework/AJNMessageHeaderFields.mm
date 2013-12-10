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
