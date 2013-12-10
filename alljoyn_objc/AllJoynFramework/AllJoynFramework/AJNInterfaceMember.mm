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

#import <alljoyn/InterfaceDescription.h>
#import "AJNHandle.h"
#import "AJNInterfaceMember.h"

@interface AJNInterfaceMember()

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) ajn::InterfaceDescription::Member *member;

@end

@implementation AJNInterfaceMember

- (AJNMessageType)type
{
    return (AJNMessageType)self.member->memberType;
}

- (NSString *)name
{
    return [NSString stringWithCString:self.member->name.c_str() encoding:NSUTF8StringEncoding];
}

- (NSString *)inputSignature
{
    return [NSString stringWithCString:self.member->signature.c_str() encoding:NSUTF8StringEncoding];
}

- (NSString *)outputSignature
{
    return [NSString stringWithCString:self.member->returnSignature.c_str() encoding:NSUTF8StringEncoding];
}

- (NSArray *)argumentNames
{
    return [[NSString stringWithCString:self.member->argNames.c_str() encoding:NSUTF8StringEncoding] componentsSeparatedByString:@","];
}

- (NSString *)accessPermissions
{
    return [NSString stringWithCString:self.member->accessPerms.c_str() encoding:NSUTF8StringEncoding];
}

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (ajn::InterfaceDescription::Member*)member
{
    return static_cast<ajn::InterfaceDescription::Member*>(self.handle);
}

- (id)initWithHandle:(AJNHandle)handle
{
    self = [super initWithHandle:handle];
    if (self) {
    }
    return self;
}

- (NSString *)annotationWithName:(NSString *)annotationName
{
    qcc::String value;
    bool wasMemberFound = self.member->GetAnnotation([annotationName UTF8String], value);
    NSString *annotationValue;
    if (wasMemberFound) {
        annotationValue = [NSString stringWithCString:value.c_str() encoding:NSUTF8StringEncoding];
    }
    return annotationValue;
}

@end
