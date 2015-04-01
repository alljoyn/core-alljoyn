////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
////////////////////////////////////////////////////////////////////////////////
//
//  ALLJOYN MODELING TOOL - GENERATED CODE
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  DoorObject.m
//
////////////////////////////////////////////////////////////////////////////////

#import "DoorObject.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for DoorObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation DoorObject

- (instancetype)initWithLocation:(NSString *)location
                         keyCode:(NSNumber *)keyCode
                          isOpen:(BOOL)isOpen
                   busAttachment:(AJNBusAttachment *)busAttachment
                            path:(NSString *)path
{
    self = [super initWithBusAttachment:busAttachment onPath:path];
    if (self) {
        [super setLocation:location];
        [super setKeyCode:keyCode];
        [super setIsOpen:isOpen];
    }
    return self;
}

- (void)setIsOpen:(BOOL)IsOpen
{
    NSLog(@"%s", __PRETTY_FUNCTION__);
    [super setIsOpen:IsOpen];
    AJNMessageArgument *msgProperty = [[AJNMessageArgument alloc] init];
    [msgProperty setValue:@"b", IsOpen];
    [msgProperty stabilize];
    [self emitPropertyWithName:@"IsOpen" onInterfaceWithName:@"com.example.Door" changedToValue:msgProperty inSession:0];
}

- (void)setKeyCode:(NSNumber *)KeyCode
{
    [super setKeyCode:KeyCode];
    AJNMessageArgument *msgProperty = [[AJNMessageArgument alloc] init];
    [msgProperty setValue:@"u", [KeyCode unsignedIntValue]];
    [msgProperty stabilize];
    [self emitPropertyWithName:@"KeyCode" onInterfaceWithName:@"com.example.Door" changedToValue:msgProperty inSession:0];
}

- (void)setLocation:(NSString *)Location
{
    [super setLocation:Location];
    AJNMessageArgument *msgProperty = [[AJNMessageArgument alloc] init];
    [msgProperty setValue:@"s", [Location UTF8String]];
    [msgProperty stabilize];
    [self emitPropertyWithName:@"Location" onInterfaceWithName:@"com.example.Door" changedToValue:msgProperty inSession:0];
}

- (void)open:(AJNMessage *)methodCallMessage
{
    if(!self.IsOpen) {
        NSLog(@"%s", __PRETTY_FUNCTION__);
        self.IsOpen = YES;
    }
}

- (void)close:(AJNMessage *)methodCallMessage
{
    if(self.IsOpen) {
        self.IsOpen = NO;
    }
}

- (void)knockAndRun:(AJNMessage *)methodCallMessage
{
    // TODO: complete the implementation of this method
    //
    // @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must implement this method" userInfo:nil]);
}


@end

////////////////////////////////////////////////////////////////////////////////
