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

- (ECCSignature*)signature
{
    return static_cast<ECCSignature*>(self.handle);
}

@end

