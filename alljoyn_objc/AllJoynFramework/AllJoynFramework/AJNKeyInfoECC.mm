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

#import <Foundation/Foundation.h>
#import "AJNKeyInfoECC.h"
#import <qcc/KeyInfoECC.h>

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@interface AJNECCPublicKey(Private)

@property (nonatomic, readonly) qcc::ECCPublicKey *publicKey;

@end

using namespace qcc;

@implementation AJNKeyInfoNISTP256

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new KeyInfoNISTP256();
    }

    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        KeyInfoNISTP256 *pArg = static_cast<KeyInfoNISTP256*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

- (KeyInfoNISTP256*)keyInfo
{
    return static_cast<KeyInfoNISTP256*>(self.handle);
}

- (AJNECCPublicKey*)publicKey
{
    return [[AJNECCPublicKey alloc] initWithHandle:(AJNHandle)self.keyInfo->GetPublicKey()];
}

- (void)setPublicKey:(AJNECCPublicKey *)publicKey
{
    self.keyInfo->SetPublicKey(publicKey.publicKey);
}

- (size_t)publicSize
{
    return self.keyInfo->GetPublicSize();
}

- (size_t)exportSize
{
    return self.keyInfo->GetExportSize();
}

- (BOOL)empty
{
    return self.keyInfo->empty() ? YES : NO;
}

- (QStatus)export:(uint8_t *)buf
{
    return self.keyInfo->Export(buf);
}

- (QStatus)import:(uint8_t *)buf count:(size_t)count
{
    return self.keyInfo->Import(buf, count);
}
- (BOOL)isEqual:(AJNKeyInfoNISTP256 *)toKeyInfo
{
    return self.keyInfo->operator==(*toKeyInfo.keyInfo) ? YES : NO;
}

- (BOOL)isNotEqual:(AJNKeyInfoNISTP256 *)toKeyInfo
{
    return self.keyInfo->operator!=(*toKeyInfo.keyInfo) ? YES : NO;
}

- (BOOL)isLessThan:(AJNKeyInfoNISTP256 *)thisKeyInfo
{
    return self.keyInfo->operator<(*thisKeyInfo.keyInfo) ? YES : NO;
}

- (NSString*)description
{
    return [NSString stringWithCString:self.keyInfo->ToString().c_str() encoding:NSUTF8StringEncoding];
}


@end
