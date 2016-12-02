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