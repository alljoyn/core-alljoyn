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

#import <Foundation/Foundation.h>
#import "AJNPermissionPolicy.h"
#import "AJNHandle.h"

using namespace ajn;

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@interface AJNMessageArgument (Private)

@property (nonatomic, readonly) MsgArg* msgArg;

@end

@interface AJNPeer (Private)

@property (nonatomic, readonly) PermissionPolicy::Peer *peer;

@end

@interface AJNMember (Private)

@property (nonatomic, readonly) ajn::PermissionPolicy::Rule::Member *member;

@end

@interface AJNRule (Private)

@property (nonatomic, readonly) PermissionPolicy::Rule *rule;

@end

@interface AJNGUID128 (Private)

@property (nonatomic, readonly) qcc::GUID128 *guid128;

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

@interface AJNCertificateX509 (Private)

@property (nonatomic, readonly) qcc::CertificateX509 *certificate;

@end

@implementation AJNMember

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new ajn::PermissionPolicy::Rule::Member();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

+ (uint8_t)ACTION_PROVIDE
{
    return PermissionPolicy::Rule::Member::ACTION_PROVIDE;
}

+ (uint8_t)ACTION_OBSERVE
{
    return PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

+ (uint8_t)ACTION_MODIFY
{
    return PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

- (ajn::PermissionPolicy::Rule::Member*)member
{
    return static_cast<ajn::PermissionPolicy::Rule::Member*>(self.handle);
}

- (void)setFields:(NSString *)memberName memberType:(AJNMemberType)memberType actionMask:(uint8_t)actionMask
{
    return self.member->Set([memberName UTF8String], (PermissionPolicy::Rule::Member::MemberType)memberType, actionMask);
}

- (NSString*)memberName
{
    return [NSString stringWithCString:self.member->GetMemberName().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setMemberName:(NSString *)memberName
{
    self.member->SetMemberName([memberName UTF8String]);
}

- (AJNMemberType)memberType
{
    return (AJNMemberType)self.member->GetMemberType();
}

- (void)setMemberType:(AJNMemberType)memberType
{
    self.member->SetMemberType((PermissionPolicy::Rule::Member::MemberType)memberType);
}

- (uint8)actionMask
{
    return self.member->GetActionMask();
}

- (void)setActionMask:(uint8_t)actionMask
{
    self.member->SetActionMask(actionMask);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.member->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isEqual:(AJNMember *)toMember
{
    return self.member->operator==(*toMember.member);
}

- (BOOL)isNotEqual:(AJNMember *)toMember
{
    return self.member->operator!=(*toMember.member);
}

@end

@implementation AJNRule

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new PermissionPolicy::Rule();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

+ (NSString*)manifestOrPolicyRuleMsgArgSignature
{
    return [NSString stringWithCString:PermissionPolicy::Rule::s_manifestOrPolicyRuleMsgArgSignature encoding:NSUTF8StringEncoding];
}

+ (NSString*)manifestTemplateRuleMsgArgSignature
{
    return [NSString stringWithCString:PermissionPolicy::Rule::s_manifestTemplateRuleMsgArgSignature encoding:NSUTF8StringEncoding];
}

- (PermissionPolicy::Rule*)rule
{
    return static_cast<PermissionPolicy::Rule*>(self.handle);
}

- (AJNRuleType)ruleType
{
    return (AJNRuleType)self.rule->GetRuleType();
}

- (void)setRuleType:(AJNRuleType)ruleType
{
    self.rule->SetRuleType((PermissionPolicy::Rule::RuleType)ruleType);
}

- (AJNSecurityLevel)recommendedSecurityLevel
{
    return (AJNSecurityLevel)self.rule->GetRecommendedSecurityLevel();
}

- (void)setRecommendedSecurityLevel:(AJNSecurityLevel)recommendedSecurityLevel
{
    self.rule->SetRecommendedSecurityLevel((PermissionPolicy::Rule::SecurityLevel)recommendedSecurityLevel);
}

- (NSString*)objPath
{
    return [NSString stringWithCString:self.rule->GetObjPath().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setObjPath:(NSString *)objPath
{
    self.rule->SetObjPath([objPath UTF8String]);
}

- (NSString*)interfaceName
{
    return [NSString stringWithCString:self.rule->GetInterfaceName().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setInterfaceName:(NSString*)interfaceName
{
    self.rule->SetInterfaceName([interfaceName UTF8String]);
}

- (NSArray*)members
{
    size_t membersSize = self.rule->GetMembersSize();
    const PermissionPolicy::Rule::Member *ajMembers = self.rule->GetMembers();
    NSMutableArray *ajnMembers = [[NSMutableArray alloc] initWithCapacity:membersSize];
    
    for (int i = 0; i < membersSize; i++) {
        [ajnMembers addObject:[[AJNMember alloc] initWithHandle:(AJNHandle)&ajMembers[i]]];
    }
    
    return ajnMembers;
}

- (void)setMembers:(NSArray *)members
{
    size_t membersSize = members.count;
    PermissionPolicy::Rule::Member *ajMembers = new PermissionPolicy::Rule::Member[membersSize];
    
    for (int i = 0; i < membersSize; i++) {
        AJNMember *member = members[i];
        ajMembers[i] = *member.member;
    }
    
    self.rule->SetMembers(membersSize, ajMembers);
}

- (size_t)membersSize
{
    return self.rule->GetMembersSize();
}

- (QStatus)toMsgArg:(AJNMessageArgument **)msgArg
{
    return self.rule->ToMsgArg(*(*msgArg).msgArg);
}

- (QStatus)fromMsgArg:(AJNMessageArgument **)msgArg ruleType:(AJNRuleType)ruleType
{
    return self.rule->FromMsgArg(*(*msgArg).msgArg, (PermissionPolicy::Rule::RuleType)ruleType);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.rule->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isEqual:(AJNRule *)toRule
{
    return self.rule->operator==(*toRule.rule);
}

- (BOOL)isNotEqual:(AJNRule *)toRule
{
    return self.rule->operator!=(*toRule.rule);
}

@end

@implementation AJNPeer

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new PermissionPolicy::Peer();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

- (AJNPeerType)type
{
    return (AJNPeerType)self.peer->GetType();
}

- (void)setType:(AJNPeerType)type
{
    self.peer->SetType((PermissionPolicy::Peer::PeerType)type);
}

- (AJNGUID128*)securityGroupId
{
    return [[AJNGUID128 alloc] initWithHandle:(AJNHandle)&self.peer->GetSecurityGroupId()];
}

- (void)setSecurityGroupId:(AJNGUID128 *)securityGroupId
{
    self.peer->SetSecurityGroupId(*securityGroupId.guid128);
}

- (AJNKeyInfoNISTP256*)keyInfo
{
    return [[AJNKeyInfoNISTP256 alloc] initWithHandle:(AJNHandle)self.peer->GetKeyInfo()];
}

- (void)setKeyInfo:(AJNKeyInfoNISTP256 *)keyInfo
{
    self.peer->SetKeyInfo(keyInfo.keyInfo);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.peer->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isEqual:(AJNPeer *)toPeer
{
    return self.peer->operator==(*toPeer.peer);
}

- (BOOL)isNotEqual:(AJNPeer *)toPeer
{
    return self.peer->operator!=(*toPeer.peer);
}

@end

@implementation AJNAcl

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new PermissionPolicy::Acl();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

- (PermissionPolicy::Peer*)peer
{
    return static_cast<PermissionPolicy::Peer*>(self.handle);
}

- (NSMutableArray*)peers
{
    size_t peerSize = self.acl->GetPeersSize();
    const PermissionPolicy::Peer *ajPeers = self.acl->GetPeers();
    NSMutableArray *retPeers = [[NSMutableArray alloc] initWithCapacity:peerSize];

    for (int i = 0; i < peerSize; i++) {
        [retPeers addObject:[[AJNPeer alloc] initWithHandle:(AJNHandle)&ajPeers[i]]];
    }
    
    return retPeers;
}

- (void)setPeers:(NSMutableArray *)peers
{
    size_t peerSize = peers.count;
    PermissionPolicy::Peer *ajPeers = new PermissionPolicy::Peer[peerSize];
    
    for (int i = 0; i < peerSize; i++) {
        AJNPeer *peer = peers[i];
        ajPeers[i] = *peer.peer;
    }
    
    return self.acl->SetPeers(peerSize, ajPeers);
}

- (size_t)peersSize
{
    return self.acl->GetPeersSize();
}

- (NSMutableArray*)rules
{
    size_t rulesSize = self.acl->GetRulesSize();
    const PermissionPolicy::Rule *ajRules = self.acl->GetRules();
    NSMutableArray *aclRules = [[NSMutableArray alloc] initWithCapacity:rulesSize];
    
    for (int i = 0; i < rulesSize; i++) {
        [aclRules addObject:[[AJNRule alloc] initWithHandle:(AJNHandle)&ajRules[i]]];
    }
    
    return aclRules;
}

- (void)setRules:(NSMutableArray *)rules
{
    size_t rulesSize = rules.count;
    PermissionPolicy::Rule *ajRules = new PermissionPolicy::Rule[rulesSize];
    
    for (int i = 0; i < rulesSize; i++) {
        AJNRule *rule = rules[i];
        ajRules[i] = *rule.rule;
    }
    
    return self.acl->SetRules(rulesSize, ajRules);
}

- (PermissionPolicy::Acl*)acl
{
    return static_cast<PermissionPolicy::Acl*>(self.handle);
}

- (NSString*)description
{
    return [NSString stringWithCString:self.acl->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isEqual:(AJNAcl *)acl
{
    return self.acl->operator==(*acl.acl);
}

- (BOOL)isNotEqual:(AJNAcl *)acl
{
    return self.acl->operator!=(*acl.acl);
}

@end

@implementation AJNManifest

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new ajn::Manifest();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

- (ajn::Manifest*)manifest
{
    return static_cast<ajn::Manifest*>(self.handle);
}

+ (NSString*)msgArgArraySignature
{
    Manifest mnfst;
    return [NSString stringWithCString:mnfst->s_MsgArgArraySignature encoding:NSUTF8StringEncoding];
}

+ (NSString*)msgArgSignature
{
    Manifest mnfst;
    return [NSString stringWithCString:mnfst->s_MsgArgSignature encoding:NSUTF8StringEncoding];
}

+ (NSString*)msgArgDigestSignature
{
    Manifest mnfst;
    return [NSString stringWithCString:mnfst->s_MsgArgDigestSignature encoding:NSUTF8StringEncoding];
}

+ (NSString*)manifestTemplateMsgArgSignature
{
    Manifest mnfst;
    return [NSString stringWithCString:mnfst->s_ManifestTemplateMsgArgSignature encoding:NSUTF8StringEncoding];
}

+ (uint32_t)defaultVersion
{
    Manifest mnfst;
    return mnfst->DefaultVersion;
}

- (uint32_t)version
{
    return (*self.manifest)->GetVersion();
}

- (NSMutableArray*)rules
{
    const std::vector<PermissionPolicy::Rule> ajRules = (*self.manifest)->GetRules();
    NSMutableArray *ajnRules = [[NSMutableArray alloc] initWithCapacity:ajRules.size()];
    
    for (int i = 0; i < ajRules.size(); i++) {
        [ajnRules addObject:[[AJNRule alloc] initWithHandle:new PermissionPolicy::Rule(ajRules[i])]];
    }
    
    return ajnRules;
}

- (QStatus)setManifestRules:(NSArray*)rules
{
    size_t ruleCount = rules.count;
    PermissionPolicy::Rule *ajRules = new PermissionPolicy::Rule[ruleCount];
    for (int i = 0; i < ruleCount; i++) {
        AJNRule *rule = rules[i];
        ajRules[i] = *rule.rule;
    }
    
    return (*self.manifest)->SetRules(ajRules, ruleCount);
}

- (QStatus)setManifestRulesFromXml:(NSString*)manifestTemplateXml;
{
    return (*self.manifest)->SetRules([manifestTemplateXml UTF8String]);
}

- (NSString*)thumbprintAlgorithmOid
{
    return [NSString stringWithCString:(*self.manifest)->GetThumbprintAlgorithmOid().c_str() encoding:NSUTF8StringEncoding];
}

-(NSMutableData*)thumbprint
{
    std::vector<uint8_t> ajThumbprint = (*self.manifest)->GetThumbprint();
    uint8_t *byteArray = new uint8_t[ajThumbprint.size()];
    NSMutableData *data = [[NSMutableData alloc] initWithCapacity:ajThumbprint.size()];
    
    for (int i = 0; i < ajThumbprint.size(); i++) {
        byteArray[i] = ajThumbprint[i];
    }
    
    [data appendBytes:byteArray length:ajThumbprint.size()];
    
    return data;
}

- (NSString*)signatureAlgorithmOid
{
    return [NSString stringWithCString:(*self.manifest)->GetSignatureAlgorithmOid().c_str() encoding:NSUTF8StringEncoding];
}

- (NSMutableData*)signature
{
    std::vector<uint8_t> ajSignature = (*self.manifest)->GetSignature();
    uint8_t *byteArray = new uint8_t[ajSignature.size()];
    NSMutableData *data = [[NSMutableData alloc] initWithCapacity:ajSignature.size()];
    
    for (int i = 0; i < ajSignature.size(); i++) {
        byteArray[i] = ajSignature[i];
    }
    
    [data appendBytes:byteArray length:ajSignature.size()];
    
    return data;
}

- (BOOL)isEqual:(AJNManifest*)toOther
{
    return (*self.manifest)->operator==(**toOther.manifest);
}

- (BOOL)isNotEqual:(AJNManifest*)toOther
{
    return (*self.manifest)->operator!=(**toOther.manifest);
}

- (QStatus)computeThumbprintAndSign:(AJNCertificateX509*)subjectCertificate issuerPrivateKey:(AJNECCPrivateKey*)issuerPrivateKey;
{
    return (*self.manifest)->ComputeThumbprintAndSign(*subjectCertificate.certificate, issuerPrivateKey.privateKey);
}

- (QStatus)computeThumbprintAndDigest:(AJNCertificateX509*)subjectCertificate digest:(NSMutableData*)digest
{
    std::vector<uint8_t> ajDigest;
    QStatus status = (*self.manifest)->ComputeThumbprintAndDigest(*subjectCertificate.certificate, ajDigest);
    if (status == ER_OK) {
        uint8_t *tmpByteArray = new uint8_t[ajDigest.size()];
        for (int i = 0; i < ajDigest.size(); i++) {
            tmpByteArray[i] = ajDigest[i];
        }
        
        [digest appendBytes:tmpByteArray length:ajDigest.size()];
    }
    
    return status;
}

- (QStatus)computeDigest:(NSMutableData*)subjectThumbprint digest:(NSMutableData*)digest
{
    std::vector<uint8_t> ajDigest;
    std::vector<uint8_t> ajThumbprint;
    uint8_t *thumbprintArray = (uint8_t*)subjectThumbprint.bytes;
    
    for (int i = 0; i < subjectThumbprint.length; i++) {
        ajThumbprint.push_back(thumbprintArray[i]);
    }
    
    QStatus status = (*self.manifest)->ComputeDigest(ajThumbprint, ajDigest);
    if (status == ER_OK) {
        uint8_t *tmpByteArray = new uint8_t[ajDigest.size()];
        for (int i = 0; i < ajDigest.size(); i++) {
            tmpByteArray[i] = ajDigest[i];
        }
        
        [digest appendBytes:tmpByteArray length:ajDigest.size()];
    }
    
    return status;
}

- (void)setSubjectThumbprintWithSHA:(NSMutableData*)subjectThumbprint
{
    std::vector<uint8_t> ajThumbprint;
    uint8_t *thumbprintArray = (uint8_t*)subjectThumbprint.bytes;
    
    for (int i = 0; i < subjectThumbprint.length; i++) {
        ajThumbprint.push_back(thumbprintArray[i]);
    }
    
    (*self.manifest)->SetSubjectThumbprint(ajThumbprint);
}

- (QStatus)setSubjectThumbprintWithCertificate:(AJNCertificateX509*)subjectCertificate
{
    return (*self.manifest)->SetSubjectThumbprint(*subjectCertificate.certificate);
}

- (QStatus)setManifestSignature:(AJNECCSignature*)signature
{
    return (*self.manifest)->SetSignature(*signature.signature);
}

- (QStatus)sign:(NSMutableData*)subjectThumbprint issuerPrivateKey:(AJNECCPrivateKey*)issuerPrivateKey
{
    std::vector<uint8_t> ajThumbprint;
    uint8_t *thumbprintArray = (uint8_t*)subjectThumbprint.bytes;
    
    for (int i = 0; i < subjectThumbprint.length; i++) {
        ajThumbprint.push_back(thumbprintArray[i]);
    }
    
    return (*self.manifest)->Sign(ajThumbprint, issuerPrivateKey.privateKey);
}

- (QStatus)computeThumbprintAndVerify:(AJNCertificateX509*)subjectCertificate issuerPublicKey:(AJNECCPublicKey*)issuerPublicKey
{
    return (*self.manifest)->ComputeThumbprintAndVerify(*subjectCertificate.certificate, issuerPublicKey.publicKey);
}

- (NSString*)description
{
    return [NSString stringWithCString:(*self.manifest)->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (QStatus)serialize:(NSMutableData*)serializedForm
{
    std::vector<uint8_t> serial;
    QStatus status = (*self.manifest)->Serialize(serial);
    
    if (status == ER_OK) {
        uint8_t *data = new uint8_t[serial.size()];
        for (int i = 0; i < serial.size(); i++) {
            data[i] = serial[i];
        }
        
        [serializedForm appendBytes:data length:serial.size()];
    }
    
    return status;
}

- (QStatus)deserialize:(NSMutableData*)serializedForm
{
    std::vector<uint8_t> serial;
    uint8_t *data = (uint8_t*)serializedForm.bytes;
    
    for (int i = 0; i < serial.size(); i++) {
        serial[i] = data[i];
    }
    
    return (*self.manifest)->Deserialize(serial);
}


@end

@implementation AJNPermissionPolicy

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new PermissionPolicy();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    
    return self;
}

- (PermissionPolicy*)permissionPolicy
{
    return static_cast<PermissionPolicy*>(self.handle);
}

- (uint16_t)specificationVersion
{
    return self.permissionPolicy->GetSpecificationVersion();
}

- (void)setSpecificationVersion:(uint16_t)specificationVersion

{
    self.permissionPolicy->SetSpecificationVersion(specificationVersion);
}

- (uint32_t)version
{
    return self.permissionPolicy->GetVersion();
}

- (void)setVersion:(uint32_t)version
{
    return self.permissionPolicy->SetVersion(version);
}

- (NSMutableArray*)acls
{
    size_t numAcls = self.permissionPolicy->GetAclsSize();
    const PermissionPolicy::Acl *cAcls = self.permissionPolicy->GetAcls();
    NSMutableArray *ajnAcls = [[NSMutableArray alloc] initWithCapacity:numAcls];
    
    for (int i = 0; i < numAcls; i++) {
        [ajnAcls addObject:[[AJNAcl alloc] initWithHandle:(AJNHandle)&cAcls[i]]];
    }
    
    delete cAcls;
    
    return ajnAcls;
}

- (void)setAcls:(NSMutableArray *)acls
{
    size_t numAcls = acls.count;
    PermissionPolicy::Acl *cAcls = new PermissionPolicy::Acl[numAcls];
    
    for (int i = 0; i < numAcls; i++) {
        AJNAcl *currentAcl = acls[i];
        cAcls[i] = *static_cast<PermissionPolicy::Acl*>(currentAcl.handle);
    }
    
    self.permissionPolicy->SetAcls(numAcls, cAcls);
}

- (QStatus)export:(AJNMessageArgument *)msgArg
{
    return self.permissionPolicy->Export(*msgArg.msgArg);
}

- (QStatus)export:(id<AJNMarshaller>)marshaller buf:(uint8_t **)buf size:(size_t*)size
{
    AJNMarshallerImpl *marshImpl = new AJNMarshallerImpl(marshaller);
    QStatus status = self.permissionPolicy->Export(*marshImpl, buf, size);
    
    delete marshImpl;
    
    return status;
}

- (QStatus)import:(uint16_t)specificationVersion msgArg:(AJNMessageArgument *)msgArg
{
    return self.permissionPolicy->Import(specificationVersion, *msgArg.msgArg);
}

- (QStatus)import:(id<AJNMarshaller>)marshaller buf:(uint8_t *)buf size:(size_t)size
{
    AJNMarshallerImpl *marshImpl = new AJNMarshallerImpl(marshaller);
    QStatus status = self.permissionPolicy->Import(*marshImpl, buf, size);
    
    delete marshImpl;
    
    return status;
}

- (QStatus)digest:(id<AJNMarshaller>)marshaller digest:(uint8_t *)digest length:(size_t)len
{
    AJNMarshallerImpl *marshImpl = new AJNMarshallerImpl(marshaller);
    QStatus status = self.permissionPolicy->Digest(*marshImpl, digest, len);
    
    delete marshImpl;
    
    return status;
}

- (NSString*)description
{
    return [NSString stringWithCString:self.permissionPolicy->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (NSString*)descriptionWithIndent:(size_t)indent
{
    return [NSString stringWithCString:self.permissionPolicy->ToString(indent).c_str() encoding:NSUTF8StringEncoding];
}

- (BOOL)isEqual:(AJNPermissionPolicy *)toPermissionPolicy
{
    return self.permissionPolicy->operator==(*toPermissionPolicy.permissionPolicy);
}

- (BOOL)isNotEqual:(AJNPermissionPolicy *)toPermissionPolicy
{
    return self.permissionPolicy->operator!=(*toPermissionPolicy.permissionPolicy);
}

+ (QStatus)manifestTemplateToMsgArg:(NSArray*)rules inMsgArg:(AJNMessageArgument*)msgArg
{
    PermissionPolicy policy;
    size_t ruleCount = rules.count;
    PermissionPolicy::Rule *ruleList = new PermissionPolicy::Rule[rules.count];
    for (int i = 0; i < ruleCount; i++) {
        AJNRule *rule = rules[i];
        ruleList[i] = *rule.rule;
    }
    
    return policy.ManifestTemplateToMsgArg(ruleList, ruleCount, *msgArg.msgArg);
}

+ (QStatus)msgArgToManifestTemplate:(AJNMessageArgument*)msgArg withRules:(NSMutableArray*)rules
{
    PermissionPolicy policy;
    std::vector<PermissionPolicy::Rule> ajRules;
    QStatus status = policy.MsgArgToManifestTemplate(*msgArg.msgArg, ajRules);

    for (int i = 0; i < ajRules.size(); i++) {
        AJNRule *rule = [[AJNRule alloc] initWithHandle:(AJNHandle)new PermissionPolicy::Rule(ajRules[i]) shouldDeleteHandleOnDealloc:YES];
        [rules addObject:rule];
    }
    
    return status;
}

+ (void)changeRulesType:(NSMutableArray*)rules ruleType:(AJNRuleType)ruleType changedRules:(NSMutableArray*)changedRules
{
    std::vector<PermissionPolicy::Rule> oldRules, newRules;
    PermissionPolicy policy;
    
    for (int i = 0; i < rules.count; i++) {
        AJNRule *rule = rules[i];
        oldRules.push_back(*rule.rule);
    }
    
    policy.ChangeRulesType(oldRules, (PermissionPolicy::Rule::RuleType)ruleType, newRules);
    
    for (int i = 0; i < newRules.size(); i++) {
        AJNRule *rule = [[AJNRule alloc] initWithHandle:(AJNHandle)new PermissionPolicy::Rule(newRules[i]) shouldDeleteHandleOnDealloc:YES];
        [changedRules addObject:rule];
    }
}

@end