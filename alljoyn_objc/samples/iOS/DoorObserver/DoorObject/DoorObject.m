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