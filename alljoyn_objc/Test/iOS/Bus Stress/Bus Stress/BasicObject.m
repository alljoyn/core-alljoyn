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
//  BasicObject.m
//
////////////////////////////////////////////////////////////////////////////////

#import "BasicObject.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for BasicObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation BasicObject

- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2
{
    return [str1 stringByAppendingString:str2];
}

- (void)methodWithOutString:(NSString*)str1 inString2:(NSString*)str2 outString1:(NSString**)outStr1 outString2:(NSString**)outStr2
{
    // TODO: complete the implementation of this method
    //
     @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must implement this method" userInfo:nil]);   
}

- (void)methodWithOnlyOutString:(NSString**)outStr1 outString2:(NSString**)outStr2
{
    // TODO: complete the implementation of this method
    //
     @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must implement this method" userInfo:nil]);   
}

- (void)methodWithNoReturnAndNoArgs
{
    // TODO: complete the implementation of this method
    //
     @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must implement this method" userInfo:nil]);   
}


@end

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Objective-C Bus Object implementation for PingObject
//
////////////////////////////////////////////////////////////////////////////////

@implementation PingObject

- (void)pingWithValue:(NSNumber*)value
{
    // TODO: complete the implementation of this method
    //
     @throw([NSException exceptionWithName:@"NotImplementedException" reason:@"You must implement this method" userInfo:nil]);   
}


@end

////////////////////////////////////////////////////////////////////////////////