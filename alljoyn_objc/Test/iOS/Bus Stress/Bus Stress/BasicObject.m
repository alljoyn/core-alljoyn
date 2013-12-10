////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
