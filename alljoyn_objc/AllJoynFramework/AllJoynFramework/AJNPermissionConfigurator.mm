////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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

#import "AJNPermissionConfigurator.h"
#import "AJNBusAttachment.h"
#import "alljoyn/BusAttachment.h"

using namespace ajn;

@interface AJNBusAttachment(Private)

@property (nonatomic, readonly) BusAttachment *busAttachment;

@end

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@interface AJNKeyInfoNISTP256(Private)

@property (nonatomic) qcc::KeyInfoNISTP256 *keyInfoNISTP256;

@end

@interface  AJNCertificateX509(Private)

@property (nonatomic, readonly) qcc::CertificateX509 *certificate;

@end

@interface AJNGUID128(Private)

@property (nonatomic, readonly) qcc::GUID128* guid128;

@end

@interface AJNECCPublicKey(Private)

@property (nonatomic, readonly) qcc::ECCPublicKey *publicKey;

@end

@implementation AJNPermissionConfigurator

- (id)init:(AJNBusAttachment*)withBus
{
    self = [super init];
    if (self) {
        self.handle = new PermissionConfigurator(*((BusAttachment*)withBus.handle));
        self.shouldDeleteHandleOnDealloc = YES;
    }

    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        PermissionConfigurator *pArg = static_cast<PermissionConfigurator*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

- (PermissionConfigurator*)permissionConfigurator
{
    return static_cast<PermissionConfigurator*>(self.handle);
}

- (QStatus)getApplicationState:(AJNApplicationState *)applicationState
{
    PermissionConfigurator::ApplicationState fetchedState;

    QStatus status = self.permissionConfigurator->GetApplicationState(fetchedState);
    *applicationState = (AJNApplicationState)fetchedState;

    return status;
}

- (QStatus)setApplicationState:(AJNApplicationState)withApplicationState
{
    return self.permissionConfigurator->SetApplicationState((PermissionConfigurator::ApplicationState)withApplicationState);
}

- (QStatus)getManifestTemplateAsXml:(NSString **)fromManifestTemplateXml
{
    std::string manifestTemplate;
    QStatus status = self.permissionConfigurator->GetManifestTemplateAsXml(manifestTemplate);

    *fromManifestTemplateXml = [NSString stringWithCString:manifestTemplate.c_str() encoding:NSUTF8StringEncoding];

    return status;
}

- (QStatus)setManifestTemplateFromXml:(NSString*)manifestTemplateXml
{
    return self.permissionConfigurator->SetManifestTemplateFromXml([manifestTemplateXml UTF8String]);
}

- (QStatus)setClaimCapabilities:(AJNClaimCapabilities)claimCapabilities
{
    return self.permissionConfigurator->SetClaimCapabilities((PermissionConfigurator::ClaimCapabilities)claimCapabilities);
}

- (QStatus)getClaimCapabilities:(AJNClaimCapabilities*)claimCapabilities
{
    PermissionConfigurator::ClaimCapabilities claimCapbs;

    QStatus status = self.permissionConfigurator->GetClaimCapabilities(claimCapbs);
    *claimCapabilities = (AJNClaimCapabilities)claimCapbs;

    return status;
}

- (QStatus)setClaimCapabilityAdditionalInfo:(AJNClaimCapabilityAdditionalInfo)additionalInfo
{
    return self.permissionConfigurator->SetClaimCapabilityAdditionalInfo((PermissionConfigurator::ClaimCapabilityAdditionalInfo)additionalInfo);
}

- (QStatus)getClaimCapabilityAdditionalInfo:(AJNClaimCapabilityAdditionalInfo*)additionalInfo
{
    PermissionConfigurator::ClaimCapabilityAdditionalInfo addedInfo;

    QStatus status = self.permissionConfigurator->GetClaimCapabilityAdditionalInfo(addedInfo);
    *additionalInfo = (AJNClaimCapabilityAdditionalInfo)addedInfo;

    return status;
}

- (QStatus)reset
{
    return self.permissionConfigurator->Reset();
}

- (QStatus)claim:(AJNKeyInfoNISTP256*)certificateAuthority adminGroupGuid:(AJNGUID128*)adminGroupGuid adminGroupAuthority:(AJNKeyInfoNISTP256*)adminGroupAuthority identityCertChain:(NSArray*)identityCertChain manifestsXmls:(NSArray*)manifestsXmls
{
    qcc::CertificateX509 *certs = new qcc::CertificateX509[identityCertChain.count];
    const char** manifests = new const char*[manifestsXmls.count];

    for (int i = 0; i < identityCertChain.count; i++) {
        AJNCertificateX509 *cert = identityCertChain[i];
        certs[i] = *cert.certificate;
    }

    for (int i = 0; i < manifestsXmls.count; i++) {
        NSString *manifest = manifestsXmls[i];
        manifests[i] = [manifest UTF8String];
    }

    QStatus status = self.permissionConfigurator->Claim(*certificateAuthority.keyInfoNISTP256, *adminGroupGuid.guid128, *adminGroupAuthority.keyInfoNISTP256, certs, identityCertChain.count, manifests, manifestsXmls.count);

    delete [] certs;
    delete [] manifests;

    return status;
}

- (QStatus)updateIdentity:(NSArray*)certs manifestsXmls:(NSArray*)manifestsXmls
{
    size_t certCount = certs.count;
    size_t manifestCount = manifestsXmls.count;
    qcc::CertificateX509 *certChain = new qcc::CertificateX509[certCount];
    const char** manifests = new const char*[manifestCount];

    for (int i = 0; i < certCount; i++) {
        AJNCertificateX509 *cert = certs[i];
        certChain[i] = *cert.certificate;
    }

    for (int i = 0; i < manifestCount; i++) {
        NSString *manifest = manifestsXmls[i];
        manifests[i] = [manifest UTF8String];
    }

    QStatus status = self.permissionConfigurator->UpdateIdentity(certChain, certCount, manifests, manifestCount);

    delete [] certChain;
    delete [] manifests;

    return status;
}

- (QStatus)getIdentity:(NSMutableArray*)certChain
{
    std::vector<qcc::CertificateX509> cert;
    QStatus status = self.permissionConfigurator->GetIdentity(cert);
    for (auto &it : cert) {
        [certChain addObject:[[AJNCertificateX509 alloc] initWithHandle:(AJNHandle)&it]];
    }

    return status;
}

- (QStatus)getIdentityCertificateId:(NSString**)serial issuerKeyInfo:(AJNKeyInfoNISTP256*)issuerKeyInfo
{
    qcc::String serialStr;
    QStatus status = self.permissionConfigurator->GetIdentityCertificateId(serialStr, *issuerKeyInfo.keyInfoNISTP256);

    *serial = [NSString stringWithCString:serialStr.c_str() encoding:NSUTF8StringEncoding];

    return status;
}

- (QStatus)resetPolicy
{
    return self.permissionConfigurator->ResetPolicy();
}

- (QStatus)getMembershipSummaries:(NSMutableArray*)serials keyInfos:(NSMutableArray*)keyInfos
{
    std::vector<qcc::String> serialStrs;
    std::vector<qcc::KeyInfoNISTP256> keyInfoList;
    QStatus status = self.permissionConfigurator->GetMembershipSummaries(serialStrs, keyInfoList);

    for (auto &it : serialStrs) {
        [serials addObject:[NSString stringWithCString:it.c_str() encoding:NSUTF8StringEncoding]];
    }

    for (auto &it : keyInfoList) {
        [keyInfos addObject:[[AJNKeyInfoNISTP256 alloc] initWithHandle:&it]];
    }

    return status;
}

- (QStatus)installMembership:(NSArray*)certChain;
{
    qcc::CertificateX509* certs = new qcc::CertificateX509[certChain.count];

    for (int i = 0; i < certChain.count; i++) {
        AJNCertificateX509 *cert = certChain[i];
        certs[i] = *cert.certificate;
    }

    QStatus status = self.permissionConfigurator->InstallMembership(certs, certChain.count);

    delete [] certs;
    return status;

}

- (QStatus)removeMembership:(NSString*)serial issuerPubKey:(AJNECCPublicKey*)issuerPubKey issuerAki:(NSString*)issuerAki
{
    return self.permissionConfigurator->RemoveMembership([serial UTF8String], issuerPubKey.publicKey, [issuerAki UTF8String]);
}

- (QStatus)removeMembership:(NSString*)serial issuerKeyInfo:(AJNKeyInfoNISTP256*)issuerKeyInfo
{
    return self.permissionConfigurator->RemoveMembership([serial UTF8String], *issuerKeyInfo.keyInfoNISTP256);
}

- (QStatus)startManagement
{
    return self.permissionConfigurator->StartManagement();
}

- (QStatus)endManagement
{
    return self.permissionConfigurator->EndManagement();
}

- (QStatus)installManifests:(NSMutableArray*)manifestsXmls append:(BOOL)append
{
    const char** manifests = new const char*[manifestsXmls.count];

    for (int i = 0; i < manifestsXmls.count; i++) {
        manifests[i] = [manifestsXmls[i] UTF8String];
    }

    QStatus status = self.permissionConfigurator->InstallManifests(manifests, manifestsXmls.count, append ? true : false);

    delete [] manifests;

    return status;
}

- (QStatus)getSigningPublicKey:(AJNKeyInfoNISTP256 **)keyInfo
{
    if (keyInfo == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::KeyInfoNISTP256 *qccKeyInfo = new qcc::KeyInfoNISTP256(); //object qccKeyInfo will be released in the AJNKeyInfoNISTP256 keyInfo dealloc method
    QStatus status = self.permissionConfigurator->GetSigningPublicKey(*qccKeyInfo);

    if (status == ER_OK) {
        *keyInfo = [[AJNKeyInfoNISTP256 alloc] initWithHandle:(AJNHandle)qccKeyInfo];
    }

    return status;
}

- (QStatus)signCertificate:(AJNCertificateX509 *)cert
{
    if (cert == nil) {
        return ER_BAD_ARG_1;
    }

    QStatus status = self.permissionConfigurator->SignCertificate(*cert.certificate);
    return status;
}

- (QStatus)computeThumbprintAndSignManifestXml:(AJNCertificateX509 *)cert manifestXml:(NSString **)manifestXml
{
    if (cert == nil) {
        return ER_BAD_ARG_1;
    }

    std::string manifest([*manifestXml UTF8String]);
    QStatus status = self.permissionConfigurator->ComputeThumbprintAndSignManifestXml(*cert.certificate, manifest);
    *manifestXml = [NSString stringWithUTF8String:manifest.c_str()];

    return status;
}

- (QStatus)getConnectedPeerPublicKeyForUid:(AJNGUID128 *)guid publicKey:(AJNECCPublicKey **)publicKey
{
    if (guid == nil) {
        return ER_BAD_ARG_1;
    }

    qcc::ECCPublicKey *qccPublicKey = new qcc::ECCPublicKey(); //object qccPublicKey will be released in the AJNECCPublicKey publicKey dealloc method
    QStatus status = self.permissionConfigurator->GetConnectedPeerPublicKey(*guid.guid128, qccPublicKey);

    if (status == ER_OK) {
        *publicKey = [[AJNECCPublicKey alloc] initWithHandle:(AJNHandle)qccPublicKey];
    }

    return status;
}

@end
