////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <qcc/GUID.h>
#import "AJNGUID.h"

using namespace qcc;

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@implementation AJNGUID128

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new GUID128();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (id)initWithValue:(uint8_t)initValue
{
    self = [super init];
    if (self) {
        self.handle = new GUID128(initValue);
        self.shouldDeleteHandleOnDealloc = YES;

    }

    return self;
}

- (id)initWithHexString:(NSString*)hexStr
{
    self = [super init];
    if (self) {
        self.handle = new GUID128([hexStr UTF8String]);
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        GUID128 *pArg = static_cast<GUID128*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

+ (size_t)SIZE
{
    return GUID128::SIZE;
}

+ (size_t)SIZE_SHORT
{
    return GUID128::SIZE_SHORT;
}

- (const uint8_t*)bytes
{
    return self.guid128->GetBytes();
}

- (GUID128*)guid128
{
    return static_cast<GUID128*>(self.handle);
}

- (BOOL)isEqual:(AJNGUID128 *)toGUID
{
    return self.guid128->operator==(*toGUID.guid128) ? YES : NO;
}

- (BOOL)isNotEqual:(AJNGUID128 *)toGUID
{
    return self.guid128->operator!=(*toGUID.guid128) ? YES : NO;
}

- (BOOL)isLessThan:(AJNGUID128 *)toGUID
{
    return self.guid128->operator<(*toGUID.guid128) ? YES : NO;
}

- (BOOL)compare:(NSString *)other
{
    return self.guid128->Compare([other UTF8String]);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.guid128->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (NSString*)shortDescription
{
    return [NSString stringWithCString:self.guid128->ToShortString().c_str() encoding:NSUTF8StringEncoding];
}

- (uint8_t*)render:(uint8_t*)data len:(size_t)len
{
    return self.guid128->Render(data, len);
}

- (NSString*)renderByteString
{
    return [NSString stringWithCString:self.guid128->RenderByteString().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setBytes:(const uint8_t *)buf
{
    self.guid128->SetBytes(buf);
}

+ (BOOL)isGuid:(NSString*)thisString
{
    return GUID128::IsGUID([thisString UTF8String]) ? YES : NO;
}

+ (BOOL)isGuid:(NSString *)thisString withExactLen:(BOOL)exactlLen
{
    return GUID128::IsGUID([thisString UTF8String], exactlLen) ? YES : NO;
}

- (NSMutableData *)getGuidData
{
    NSMutableData *guidData = [NSMutableData dataWithBytes:self.bytes length:GUID128::SIZE];
    return guidData;
}

@end