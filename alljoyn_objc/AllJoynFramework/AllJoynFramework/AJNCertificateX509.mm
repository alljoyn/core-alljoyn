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

#import "AJNCertificateX509.h"

using namespace qcc;

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@interface AJNKeyInfoNISTP256 (Private)

@property (nonatomic, readonly) qcc::KeyInfoNISTP256 *keyInfo;

@end

@interface AJNECCPublicKey (Private)

@property (nonatomic, readonly) qcc::ECCPublicKey *publicKey;

@end

@interface AJNECCPrivateKey (Private)

@property (nonatomic, readonly) qcc::ECCPrivateKey *privateKey;

@end

@interface AJNECCSignature (Private)

@property (nonatomic, readonly) qcc::ECCSignature *signature;

@end

/*
 * SHA256 Digest Size
 */
size_t CRYPTO_SHA256_DIGEST_SIZE = 32;

@implementation AJNCertificateX509

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new qcc::CertificateX509();
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (id)initWithCertificateType:(AJNCertificateType)type
{
    self = [super init];
    if (self) {
        self.handle = new CertificateX509((CertificateX509::CertificateType)type);
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (CertificateX509*)certificate
{
    return static_cast<CertificateX509*>(self.handle);
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        CertificateX509 *pArg = static_cast<CertificateX509*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

- (NSMutableData*)serial
{
    const uint8_t *data = self.certificate->GetSerial();
    NSMutableData *outData = [[NSMutableData alloc] initWithLength:self.certificate->GetSerialLen()];

    [outData appendBytes:data length:self.certificate->GetSerialLen()];

    return outData;
}

- (void)setSerial:(NSMutableData*)serialNumber
{
    uint8_t *data = (uint8_t*)serialNumber.bytes;
    return self.certificate->SetSerial(data, serialNumber.length);
}

- (size_t)serialLength
{
    return self.certificate->GetSerialLen();
}

- (size_t)issuerOULength
{
    return self.certificate->GetIssuerOULength();
}

- (NSData*)issuerOU
{
    const uint8_t *data = self.certificate->GetIssuerOU();
    NSMutableData *outData = [[NSMutableData alloc] initWithLength:self.certificate->GetIssuerOULength()];

    [outData appendBytes:data length:self.certificate->GetIssuerOULength()];

    return outData;
}

- (void)setIssuerOU:(NSData*)ou
{
    uint8_t *data = (uint8_t*)ou.bytes;
    return self.certificate->SetIssuerOU(data, ou.length);
}

- (size_t)issuerCNLength
{
    return self.certificate->GetIssuerCNLength();
}

- (NSMutableData*)issuerCN
{
    const uint8_t *data = self.certificate->GetIssuerCN();
    NSMutableData *outData = [[NSMutableData alloc] initWithLength:self.certificate->GetIssuerCNLength()];

    [outData appendBytes:data length:self.certificate->GetIssuerCNLength()];

    return outData;
}

- (void)setIssuerCN:(NSMutableData*)cn
{
    uint8_t *data = (uint8_t*)cn.bytes;
    return self.certificate->SetIssuerCN(data, cn.length);
}

- (size_t)subjectOULength
{
    return self.certificate->GetSubjectOULength();
}

- (NSMutableData*)subjectOU
{
    const uint8_t *data = self.certificate->GetSubjectOU();
    NSMutableData *outData = [[NSMutableData alloc] initWithLength:self.certificate->GetSubjectOULength()];

    [outData appendBytes:data length:self.certificate->GetSubjectOULength()];

    return outData;
}

- (void)setSubjectOU:(NSMutableData*)ou
{
    uint8_t *data = (uint8_t*)ou.bytes;
    return self.certificate->SetSubjectOU(data, ou.length);
}

- (size_t)subjectCNLength
{
    return self.certificate->GetSubjectCNLength();
}

- (NSMutableData*)subjectCN
{
    const uint8_t *data = self.certificate->GetSubjectCN();
    NSMutableData *outData = [[NSMutableData alloc] initWithLength:self.certificate->GetSubjectCNLength()];

    [outData appendBytes:data length:self.certificate->GetSubjectCNLength()];

    return outData;
}

- (void)setSubjectCN:(NSMutableData*)cn;
{
    uint8_t *data = (uint8_t*)cn.bytes;
    return self.certificate->SetSubjectCN(data, cn.length);
}

- (NSString*)subjectAltName
{
    return [NSString stringWithCString:self.certificate->GetSubjectAltName().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setSubjectAltName:(NSString*)subjectAltName;
{
    return self.certificate->SetSubjectAltName([subjectAltName UTF8String]);
}

- (NSString*)authorityKeyId
{
    return [NSString stringWithCString:self.certificate->GetAuthorityKeyId().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setAuthorityKeyId:(NSString*)newAki
{
    self.certificate->SetAuthorityKeyId([newAki UTF8String]);
}

- (AJNValidPeriod*)validity
{
    return (AJNValidPeriod*)self.certificate->GetValidity();
}

- (void)setValidity:(AJNValidPeriod*)validity
{
    self.certificate->SetValidity((CertificateX509::ValidPeriod*)validity);
}

- (AJNECCPublicKey*)subjectPublicKey
{
    return [[AJNECCPublicKey alloc] initWithHandle:(AJNHandle)self.certificate->GetSubjectPublicKey()];
}

- (void)setSubjectPublicKey:(AJNECCPublicKey*)key
{
    if (key != nil) {
        self.certificate->SetSubjectPublicKey(key.publicKey);
    }
}

- (BOOL)isCA
{
    return self.certificate->IsCA() ? YES : NO;
}

- (NSMutableData*)digest
{
    const uint8_t *data = self.certificate->GetDigest();
    NSMutableData *outData = [[NSMutableData alloc] initWithLength:self.certificate->GetDigestSize()];

    [outData appendBytes:data length:self.certificate->GetDigestSize()];

    return outData;
}

- (void)setDigest:(NSMutableData*)digest
{
    uint8_t *data = (uint8_t*)digest.bytes;
    return self.certificate->SetDigest(data, digest.length);
}

- (size_t)digestSize
{
    return self.certificate->GetDigestSize();
}

- (AJNCertificateType)type
{
    return (AJNCertificateType)self.certificate->GetType();
}

- (QStatus)decodeCertificatePEM:(NSString*)pem
{
    return self.certificate->DecodeCertificatePEM([pem UTF8String]);
}

- (QStatus)encodeCertificatePEM:(NSString**)pem
{
    if (pem == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String str;
    QStatus status = self.certificate->EncodeCertificatePEM(str);

    if (status == ER_OK) {
        *pem = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *pem = nil;
    }

    return status;
}

+ (QStatus)encodeCertificatePEM:(NSString*)der pem:(NSString**)pem
{
    if (pem != nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String str;
    qcc::String derStr = [der UTF8String];
    QStatus status = qcc::CertificateX509::EncodeCertificatePEM(derStr, str);

    if (status == ER_OK) {
        *pem = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *pem = nil;
    }

    return status;
}

- (QStatus)decodeCertificateDER:(NSString*)der
{
    return self.certificate->DecodeCertificateDER([der UTF8String]);
}

- (QStatus)encodeCertificateDER:(NSString**)der
{
    if (der == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String str;
    QStatus status = self.certificate->EncodeCertificateDER(str);

    if (status == ER_OK) {
        *der = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *der = nil;
    }

    return status;
}

- (QStatus)encodeCertificateTBS:(NSString**)tbsder
{
    if (tbsder == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String str;
    QStatus status = self.certificate->EncodeCertificateTBS(str);

    if (status == ER_OK) {
        *tbsder = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *tbsder = nil;
    }

    return status;
}

+ (QStatus)encodePrivateKeyPEM:(AJNECCPrivateKey*)privateKey encoded:(NSString**)encoded
{
    if (encoded == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String str;
    QStatus status = qcc::CertificateX509::EncodePrivateKeyPEM(privateKey.privateKey, str);

    if (status == ER_OK) {
        *encoded = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *encoded = nil;
    }

    return status;
}

+ (QStatus)decodePrivateKeyPEM:(NSString*)encoded privateKey:(AJNECCPrivateKey*)privateKey
{
    if (privateKey == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String encodedStr = [encoded UTF8String];
    return qcc::CertificateX509::DecodePrivateKeyPEM(encodedStr, privateKey.privateKey);
}

+ (QStatus)encodePublicKeyPEM:(AJNECCPublicKey*)publicKey encoded:(NSString**)encoded
{
    if (encoded == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String str;
    QStatus status = qcc::CertificateX509::EncodePublicKeyPEM(publicKey.publicKey, str);

    if (status == ER_OK) {
        *encoded = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *encoded = nil;
    }

    return status;
}

+ (QStatus)decodePublicKeyPEM:(NSString*)encoded publicKey:(AJNECCPublicKey*)publicKey
{
    if (publicKey == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::String encodedStr = [encoded UTF8String];
    return qcc::CertificateX509::DecodePublicKeyPEM(encodedStr, publicKey.publicKey);
}

- (QStatus)sign:(AJNECCPrivateKey*)key
{
    if (key == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->Sign(key.privateKey);
}

- (void)setSignature:(AJNECCSignature*)sig
{
    self.certificate->SetSignature(*sig.signature);
}

- (QStatus)signAndGenerateAuthorityKeyId:(AJNECCPrivateKey*)privateKey publicKey:(AJNECCPublicKey*)publicKey
{
    if (privateKey == nil) {
        return ER_BAD_ARG_1;
    }

    if (publicKey == nil) {
        return ER_BAD_ARG_2;
    }

    return self.certificate->SignAndGenerateAuthorityKeyId(privateKey.privateKey, publicKey.publicKey);
}

- (QStatus)verifySelfSigned
{
    return self.certificate->Verify();
}

- (QStatus)verifyWithPublicKey:(AJNECCPublicKey*)key
{
    if (key == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->Verify(key.publicKey);
}

- (QStatus)verifyWithTrustAnchor:(AJNKeyInfoNISTP256*)trustAnchor
{
    if (trustAnchor == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->Verify(*trustAnchor.keyInfo);
}

- (QStatus)verifyValidity
{
    return self.certificate->VerifyValidity();
}

- (QStatus)generateRandomSerial
{
    return self.certificate->GenerateRandomSerial();
}

+ (QStatus)generateAuthorityKeyId:(AJNECCPublicKey*)issuerPubKey authorityKeyId:(NSString**) authorityKeyId
{
    if (issuerPubKey == nil) {
        return ER_BAD_ARG_1;
    }

    if (authorityKeyId == nil) {
        return ER_BAD_ARG_2;
    }

    qcc::String str;
    QStatus status = qcc::CertificateX509::GenerateAuthorityKeyId(issuerPubKey.publicKey, str);

    if (status == ER_OK) {
        *authorityKeyId = [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
    } else {
        *authorityKeyId = nil;
    }

    return status;
}

- (QStatus)generateAuthorityKeyId:(AJNECCPublicKey*)issuerPubKey
{
    if (issuerPubKey == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->GenerateAuthorityKeyId(issuerPubKey.publicKey);
}

- (void)setCA:(BOOL)flag
{
    self.certificate->SetCA(flag ? true : false);
}

- (QStatus)loadPEM:(NSString*)pem
{
    return self.certificate->LoadPEM([pem UTF8String]);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.certificate->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isIssuerOf:(AJNCertificateX509*)issuedCertificate
{
    if (issuedCertificate == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->IsIssuerOf(*issuedCertificate.certificate) ? YES : NO;
}

- (BOOL)isDNEqual:(NSMutableData*)cn ou:(NSMutableData*)ou
{
    uint8_t *cnData = (uint8_t*)cn.bytes;
    uint8_t *ouData = (uint8_t*)ou.bytes;

    return self.certificate->IsDNEqual(cnData, cn.length, ouData, ou.length);
}

- (BOOL)isDNEqual:(AJNCertificateX509*)other
{
    if (other == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->IsDNEqual(*other.certificate) ? YES : NO;
}

- (BOOL)isSubjectPublicKeyEqual:(AJNECCPublicKey*)publicKey
{
    if (publicKey == nil) {
        return ER_BAD_ARG_1;
    }

    return self.certificate->IsSubjectPublicKeyEqual(publicKey.publicKey) ? YES : NO;
}

- (QStatus)getSHA256Thumbprint:(NSMutableData*)thumbprint
{
    uint8_t *outData = NULL;
    QStatus status = self.certificate->GetSHA256Thumbprint(outData);
    NSData *tmp = [NSData dataWithBytes:outData length:CRYPTO_SHA256_DIGEST_SIZE];
    [thumbprint setData:tmp];

    return status;
}

+ (QStatus)decodeCertChainPEM:(NSString*)encoded certChain:(NSMutableArray*)certChain
{
    size_t certCount = certChain.count;
    qcc::CertificateX509 *certs = new qcc::CertificateX509[certCount];
    QStatus status = ER_OK;

    for (int i = 0; i < certCount; i++) {
        AJNCertificateX509 *cert = certChain[i];
        certs[i] = *cert.certificate;
    }

    status = qcc::CertificateX509::DecodeCertChainPEM([encoded UTF8String], certs, certCount);
    [certChain removeAllObjects];

    for (int i = 0; i < certCount; i++) {
        AJNCertificateX509 *cert = [[AJNCertificateX509 alloc] initWithHandle:(AJNHandle)&certs[i]];
        [certChain addObject:cert];
    }

    return status;
}

+ (BOOL)validateCertificateTypeInCertChain:(NSArray*)certChain
{
    size_t certCount = certChain.count;
    qcc::CertificateX509 *certs = new qcc::CertificateX509[certCount];
    BOOL status = NO;

    for (int i = 0; i < certCount; i++) {
        AJNCertificateX509 *cert = certChain[i];
        certs[i] = *cert.certificate;
    }

    status = qcc::CertificateX509::ValidateCertificateTypeInCertChain(certs, certCount) ? YES : NO;

    delete [] certs;

    return status;
}

@end