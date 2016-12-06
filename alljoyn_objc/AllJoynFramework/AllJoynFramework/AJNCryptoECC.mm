////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
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
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import <alljoyn/Status.h>
#import "AJNKeyInfoECC.h"
#import <qcc/KeyInfoECC.h>

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

using namespace qcc;

@implementation AJNECCPublicKey

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new ECCPublicKey();
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        ECCPublicKey *pArg = static_cast<ECCPublicKey*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

- (size_t)size
{
    return self.publicKey->GetSize();
}

- (ECCPublicKey*)publicKey
{
    return static_cast<ECCPublicKey*>(self.handle);
}

- (void)clear
{
    self.publicKey->Clear();
}

- (BOOL)isEmpty
{
    return self.publicKey->empty() ? YES : NO;
}

- (BOOL)isEqualTo:(AJNECCPublicKey *)toKey
{
    return self.publicKey->operator==(*toKey.publicKey);
}

- (BOOL)isNotEqualTo:(AJNECCPublicKey *)toKey
{
    return self.publicKey->operator!=(*toKey.publicKey);
}

- (BOOL)isLessThan:(AJNECCPublicKey *)thisKey
{
    return self.publicKey->operator<(*thisKey.publicKey);
}

- (QStatus)export:(uint8_t *)buf size:(size_t*)size
{
    return self.publicKey->Export(buf, size);
}

- (QStatus)import:(uint8_t *)data size:(size_t)size
{
    return self.publicKey->Import(data, size);
}

- (QStatus)import:(uint8_t *)xData xSize:(size_t)xSize yData:(uint8_t *)yData ySize:(size_t)ySize
{
    return self.publicKey->Import(xData, xSize, yData, ySize);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.publicKey->ToString().c_str() encoding:NSUTF8StringEncoding];
}

@end

@implementation AJNECCPrivateKey

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new ECCPrivateKey();
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        ECCPrivateKey *pArg = static_cast<ECCPrivateKey*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

- (ECCPrivateKey*)privateKey
{
    return static_cast<ECCPrivateKey*>(self.handle);
}

- (size_t)size
{
    return self.privateKey->GetSize();
}

- (QStatus)import:(NSData*)data
{
    uint8_t* dataArray = (uint8_t*)data.bytes;
    QStatus status = self.privateKey->Import(dataArray, data.length);

    delete dataArray;

    return status;
}

- (QStatus)export:(NSMutableData*)data
{
    uint8_t *inData = new uint8_t[data.length];
    size_t size = data.length;
    QStatus status = self.privateKey->Export(inData, &size);

    if (status == ER_OK || status == ER_BUFFER_TOO_SMALL) {
        [data appendBytes:inData length:size];
    }

    return status;
}

- (BOOL)isEqualTo:(AJNECCPrivateKey*)otherKey
{
    return self.privateKey->operator==(*otherKey.privateKey);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.privateKey->ToString().c_str() encoding:NSUTF8StringEncoding];
}

@end

@implementation AJNECCSignature

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new ECCSignature();
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        ECCSignature *pArg = static_cast<ECCSignature*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

- (ECCSignature*)signature
{
    return static_cast<ECCSignature*>(self.handle);
}

@end
