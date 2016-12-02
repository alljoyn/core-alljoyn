/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
//
//  ALLJOYN MODELING TOOL - GENERATED CODE
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ObserverTestObjects.m
//
////////////////////////////////////////////////////////////////////////////////

#import "ObserverTestObjects.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for TestJustA
//
////////////////////////////////////////////////////////////////////////////////

@implementation TestJustA

- (void)identifyAWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
{
    // get hidden property via KVO observing
    *busname = [NSString stringWithString:((AJNBusAttachment *)[self valueForKey:@"bus"]).uniqueName];
    *path = [NSString stringWithString:self.path];

}


@end

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for TestJustB
//
////////////////////////////////////////////////////////////////////////////////

@implementation TestJustB

- (void)identifyBWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
{
    // get hidden property via KVO observing
    *busname = [NSString stringWithString:((AJNBusAttachment *)[self valueForKey:@"bus"]).uniqueName];
    *path = [NSString stringWithString:self.path];
}


@end

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for TestBoth
//
////////////////////////////////////////////////////////////////////////////////

@implementation TestBoth

- (void)identifyAWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
{
    // get hidden property via KVO observing
    *busname = [NSString stringWithString:((AJNBusAttachment *)[self valueForKey:@"bus"]).uniqueName];
    *path = [NSString stringWithString:self.path];
}

- (void)identifyBWithBusname:(NSString**)busname andPath:(NSString**)path message:(AJNMessage *)methodCallMessage
{
    // get hidden property via KVO observing
    *busname = [NSString stringWithString:((AJNBusAttachment *)[self valueForKey:@"bus"]).uniqueName];
    *path = [NSString stringWithString:self.path];
}


@end

////////////////////////////////////////////////////////////////////////////////