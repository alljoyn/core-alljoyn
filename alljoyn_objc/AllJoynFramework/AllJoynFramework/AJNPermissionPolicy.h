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
#import <alljoyn/Status.h>
#import <alljoyn/PermissionPolicy.h>
#import "AJNObject.h"
#import "AJNMessageArgument.h"
#import "AJNKeyInfoECC.h"
#import "AJNGUID.h"
#import "AJNCertificateX509.h"

/**
 * Enum indicating the suggested security level for the interface
 */
typedef enum {
    PRVILEGED       = 0,
    NON_PRIVILEGED  = 1,
    UNAUTHENTICATED = 2
} AJNSecurityLevel;

typedef enum {
    MANIFEST_POLICY_RULE   = 0,
    MANIFEST_TEMPLATE_RULE = 1
} AJNRuleType;

/**
 * Enumeration for the different type of members
 */
typedef enum {
    NOT_SPECIFIED = 0,
    METHOD_CALL   = 1,
    SIGNAL        = 2,
    PROPERTY      = 3
} AJNMemberType;

typedef enum {
    PEER_ALL                        = 0,
    PEER_ANY_TRUSTED                = 1,
    PEER_FROM_CERTIFICATE_AUTHORITY = 2,
    PEER_WITH_PUBLIC_KEY            = 3,
    PEER_WITH_MEMBERSHIP            = 4
} AJNPeerType;

@class AJNPermissionPolicy;
@class AJNMarshaller;

/**
 * Class to allow the application to specify a permission rule at the interface member level
 */
@interface AJNMember : AJNObject

/**
 * Get the MemberName
 * @return the MemberName
 */
@property (nonatomic, readonly) NSString *memberName;

/**
 * Get the MemberType
 * @return the MemberType
 */
@property (nonatomic, readonly) AJNMemberType memberType;

/**
 * Get the ActionMask
 * @return the ActionMask
 */
@property (nonatomic, readonly) uint8_t actionMask;

/**
 * Construct an AJNMember
 */
- (id)init;

/**
 * The permission action masks
 */
+ (uint8_t)ACTION_PROVIDE;  /** < allow to provide  */
+ (uint8_t)ACTION_OBSERVE;  /** < allow to observer */
+ (uint8_t)ACTION_MODIFY;   /** < allow to modify   */

/**
 * Set the MemberName, MemberType and action mask
 *
 * @param[in] memberName the name of the interface member
 * @param[in] memberType the type of member
 * @param[in] actionMask the permission action mask
 */
- (void)setFields:(NSString*)memberName memberType:(AJNMemberType)memberType actionMask:(uint8_t)actionMask;

/**
 * Set the MemberName
 * @param[in] memberName the name of the interface member
 */
- (void)setMemberName:(NSString*)memberName;

/**
 * Set the MemberType
 * @param[in] memberType the type of the member
 */
- (void)setMemberType:(AJNMemberType)memberType;

/**
 * Set the ActionMask
 * param[in] actionMask Action mask value
 */
- (void)setActionMask:(uint8_t)actionMask;

/**
 * A String representation of the Member
 * @return A String representation of the Member
 */
- (NSString*)description;

/**
 * Comparison operators equality
 * @param[in] toMember right hand side Member
 * @return true if Members are equal
 */
- (BOOL)isEqual:(AJNMember*)toMember;

/**
 * Comparison operators non-equality
 * @param[in] toMember right hand side Member
 * @return true if Members are not equal
 */
- (BOOL)isNotEqual:(AJNMember*)toMember;

@end

/**
 * Class to allow the application to specify a permission rule
 */
@interface AJNRule : AJNObject

/**
 * Get the rule's type.
 * @return The rule's type.
 */
@property (nonatomic, readonly) AJNRuleType ruleType;

/**
 * Get the security level.
 * @return The interface's security level.
 */
@property (nonatomic, readonly) AJNSecurityLevel recommendedSecurityLevel;

/**
 * Get the Object Path
 * @return the Object Path
 */
@property (nonatomic, readonly) NSString *objPath;

/**
 * Get the InterfaceName
 * @return the interface name
 */
@property (nonatomic, readonly) NSString *interfaceName;

/**
 * Get the array of inferface members.
 * @return the array of interface members.
 */
@property (nonatomic, readonly) NSArray* members;

/**
 * Get the number of Members in the Rule
 * @return the number of Members in the rule.
 */
@property (nonatomic, readonly) size_t membersSize;

/**
 * AJNPermissionPolicy Constructor
 */
- (id)init;

/** MsgArg signature for a manifest or policy rule. */
+ (NSString*)manifestOrPolicyRuleMsgArgSignature;

/** MsgArg signature for a manifestTemplate rule. */
+ (NSString*)manifestTemplateRuleMsgArgSignature;

/**
 * Set the rule type.
 * @param[in]   ruleType  The rule's type.
 */
- (void)setRuleType:(AJNRuleType)ruleType;

/**
 * Set the security level.
 * @param[in]   securityLevel  The interface's security level.
 */
- (void)setRecommendedSecurityLevel:(AJNSecurityLevel)securityLevel;

/**
 * Set the object path
 * @param[in] objPath the object path
 */
- (void)setObjPath:(NSString *)objPath;

/**
 * Set the Interface Name
 * @param[in] interfaceName the interface name.
 */
- (void)setInterfaceName:(NSString *)interfaceName;

/**
 * Set the array of members for the given interface.
 * @param[in] members  The array of member fields.
 */
- (void)setMembers:(NSArray *)members;

/**
 * Export the rule to a MsgArg.
 * @param[out]   msgArg  MsgArg to be set with Rule's contents.
 * @return
 *      - #ER_OK if export was successful.
 *      - Error code if fail.
 */
- (QStatus)toMsgArg:(AJNMessageArgument**)msgArg;

/**
 * Import the rule from a MsgArg.
 * @param[in]   msgArg   MsgArg to import the Rule's contents from.
 * @param[in]   ruleType Type of imported rule.
 * @return
 *      - #ER_OK if export was successful.
 *      - Error code if fail.
 */
- (QStatus)fromMsgArg:(AJNMessageArgument**)msgArg ruleType:(AJNRuleType)ruleType;

/**
 * Comparison operators equality
 * @param[in] toRule Rule to compare with
 * @return true if Rules are equal
 */
- (BOOL)isEqual:(AJNRule*)toRule;

/**
 * Comparison operators non-equality
 * @param[in] toRule Rule to compare with
 * @return true if Rules are not equal
 */
- (BOOL)isNotEqual:(AJNRule*)toRule;

/**
 * String representation of the Rule
 * @return string representation of the rule
 */
- (NSString*)description;

@end

/**
 * Class to allow the application to specify a permission peer.
 */
@interface AJNPeer : AJNObject

/**
 * Get the peer type
 */
@property (nonatomic, readonly) AJNPeerType type;

/**
 * Get the security group id
 * @return the security group id
 */
@property (nonatomic, readonly) AJNGUID128 *securityGroupId;

/**
 * Get the keyInfo field.
 * @return keyInfo the keyInfo.
 */
@property (nonatomic) AJNKeyInfoNISTP256 *keyInfo;

/**
 * Construct an AJNPeer
 */
- (id)init;

/**
 * Set the peer type
 */
- (void)setType:(AJNPeerType)type;

/**
 * Set the security group id
 * @param guid the security group id
 */
- (void)setSecurityGroupId:(AJNGUID128 *)securityGroupId;

/**
 * Set the keyInfo field.
 * When peer type is PEER_ALL the keyInfo is not relevant.
 * When peer type is PEER_ANY_TRUSTED the keyInfo is not relevant.
 * When peer type is PEER_FROM_CERTIFICATE_AUTHORITY the
 *     keyInfo.PublicKey is the public key of the certificate authority.
 * When peer type is PEER_WITH_PUBLIC_KEY the keyInfo.PublicKey is
 *     the public key of the peer.
 * When peer type is PEER_WITH_MEMBERSHIP the keyInfo.PublicKey is the
 *     public key of the security group authority.
 * @param[in] keyInfo the keyInfo. Set to NULL to removed keyInfo.
 */
- (void)setKeyInfo:(AJNKeyInfoNISTP256 *)keyInfo;

/**
 * A String representation of the Peer
 * @return A String representation of the Peer
 */
- (NSString*)description;

/**
 * Comparison operators equality
 * @param[in] toPeer Peer to compare with
 * @return true is Peers are equal
 */
- (BOOL)isEqual:(AJNPeer*)toPeer;

/**
 * Comparison operators non-equality
 * @param[in] other Peer to compare with
 * @return true if Peers are not equal
 */
- (BOOL)isNotEqual:(AJNPeer*)toPeer;

@end
/*
 * Class to specify the marshal/unmarshal utility for the policy data.
 */
@protocol AJNMarshaller <NSObject>

/* uint8_t data is used over NSMutableData or NSData to allow for implementation specific conversion. */

@optional

/**
 * Marshal the permission policy to a byte array.
 *
 * @param[in] policy the policy to marshal into a byte array
 * @param[out] buf the newly allocated byte array holding the serialized data. The caller must delete[] this buffer after use.
 * @param[out] size the variable holding the size of the allocated byte array
 * @return
 *      - #ER_OK if export was successful.
 *      - error code if fail
 */
- (QStatus)marhsal:(AJNPermissionPolicy*)policy buf:(uint8_t**)buf size:(size_t*)size;

/**
 * Unmarshal the permission policy from a byte array.
 *
 * @param[out] policy the policy the byte array will be unmarshalled into
 * @param buf the byte array holding the serialized data. The serialized data must be generated by the Export call.
 * @param size the size of the byte array
 * @return
 *      - #ER_OK if import was successful.
 *      - error code if fail
 */
- (QStatus)unmarshal:(AJNPermissionPolicy*)policy buf:(const uint8_t*)buf size:(size_t)size;

/**
 * Generate a hash digest for the policy data.  Each marshaller can use its own digest algorithm.
 * @param[in] policy the policy used to generate the hash digest
 * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
 * @param len the length of the digest buffer.
 * @return
 *      - #ER_OK if digest was generated successfully.
 *      - error code if fail
 */
- (QStatus)digestWithPolicy:(AJNPermissionPolicy*)policy digest:(uint8_t*)digest len:(size_t)len;


/**
 * Generate a hash digest for the manifest template data.  Each marshaller can use its own digest algorithm.
 * @param[in] rules the array of rules in the manifest
 * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
 * @param len the length of the digest buffer.
 * @return
 *      - #ER_OK if digest was successful.
 *      - error code if fail
 */
- (QStatus)digestWithRules:(NSArray*)rules digest:(uint8_t*)digest len:(size_t)len;

@end

@interface AJNAcl : AJNObject

/**
 * Get a pointer to the Peers array stored in the Acl
 */
@property (nonatomic, readonly) NSMutableArray *peers;

/**
 * Get the number of Peers in the Acl
 * @return the number of Peers in the Acl
 */
@property (nonatomic, readonly) size_t peersSize;

/**
 * Get a pointer to the Rules array stored in the Acl
 */
@property (nonatomic, readonly) NSMutableArray *rules;

/**
 * Get the number of Rules in the Acl
 * @return the number of Rules in the Acl
 */
@property (nonatomic, readonly) size_t rulesSize;

/**
 * Construct an AJNAcl
 */
- (id)init;

/**
 * Set the array of peers
 * @param[in] peers the array of peers.
 */
- (void)setPeers:(NSMutableArray*)peers;

/**
 * Set the array of rules.
 * @param[in] rules the array of rules.
 */
- (void)setRules:(NSMutableArray*)rules;

/**
 * Get a string representation of the Acl
 * @return a string representation of the Acl
 */
- (NSString*)description;

/**
 * Comparison operators equality
 * @param[in] other right hand side Acl
 * @return true if Acls are equal
 */
- (BOOL)isEqual:(AJNAcl*)other;

/**
 * Comparison operators non-equality
 * @param[in] other right hand side Acl
 * @return true if Acls are not equal
 */
- (BOOL)isNotEqual:(AJNAcl*)other;

@end

@interface AJNManifest : AJNObject

/**
 * Get version number of this manifest.
 *
 * @return version number
 */
@property (nonatomic, readonly) uint32_t version;

/**
 * Get rules of this manifest.
 *
 * @return NSMutableArray of PermissionPolicy::Rule objects
 */
@property (nonatomic, readonly) NSMutableArray *rules;

/**
 * Get the OID of the algorithm used to compute the certificate thumbprint.
 *
 * @return NSString containing the OID
 */
@property (nonatomic, readonly) NSString *thumbprintAlgorithmOid;

/**
 * Get the certificate thumbprint.
 *
 * @return NSMutableArray of bytes containing the thumbprint
 */
@property (nonatomic, readonly) NSMutableData *thumbprint;

/**
 * Get the OID used to compute the signature.
 *
 * @return NSString containing the OID
 */
@property (nonatomic, readonly) NSString *signatureAlgorithmOid;

/**
 * Get the signature.
 *
 * @return NSMutableArray of bytes containing the signature
 */
@property (nonatomic, readonly) NSMutableData *signature;

- (id)init;

/** MsgArg signature for an array of signed manifests. */
+ (NSString*)msgArgArraySignature;
/** MsgArg signature for a single signed manifest. */
+ (NSString*)msgArgSignature;
/** MsgArg signature for a single signed manifest without the cryptographic signature field. */
+ (NSString*)msgArgDigestSignature;
/** MsgArg signature for a 16.10 manifest template. */
+ (NSString*)manifestTemplateMsgArgSignature;

/** Default version number for new manifests. */
+ (uint32_t)defaultVersion;

/**
 * Equality operator.
 * @param[in] toOther Manifest to compare against
 *
 * @return true if contents are equal, false otherwise
 */
- (BOOL)isEqual:(AJNManifest*)toOther;

/**
 * Inequality operator.
 * @param[in] toOther Manifest to compare against
 *
 * @return true if the contents are different, false if they are equal
 */
- (BOOL)isNotEqual:(AJNManifest*)toOther;

/**
 * Set the rules to be set on this manifest. After calling SetRules, the cryptographic
 * signature on this Manifest will no lnoger be valid; it will need to be signed again with the
 * Sign method before applying to an application.
 *
 * @param[in] rules Array of PermissionPolicy::Rule objects
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)setManifestRules:(NSArray*)rules;

/**
 * Set the rules on this manifest from a manifest template XML.
 * After calling SetRules, the cryptographic signature on this Manifest will
 * no lnoger be valid; it will need to be signed again with the
 * Sign method before applying to an application.
 *
 * @param[in]    manifestTemplateXml Input manifest template XML.
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)setManifestRulesFromXml:(NSString*)manifestTemplateXml;

/**
 * Cryptographically sign this manifest for the use of a particular subject certificate using
 * the provided signing key. issuerPrivateKey must be the private key that signed subjectCertificate for
 * apps to consider it valid. Caller must ensure the correct issuer public key is provided; this method
 * does not verify the correct key is provided.
 *
 * Caller is responsible for verifying subjectCertificate is the signed certificate which will be used
 * by the peer using this manifest; no validation of this is done.
 *
 * @param[in] subjectCertificate Signed certificate of the app which will use this manifest
 * @param[in] issuerPrivateKey Private key of subjectCertificate's issuer to sign the manifest
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)computeThumbprintAndSign:(AJNCertificateX509*)subjectCertificate issuerPrivateKey:(AJNECCPrivateKey*)issuerPrivateKey;

/**
 * Set this manifest for the use of a particular subject certificate and compute the digest, to be signed
 * with ECDSA_SHA256 by the caller. That signature can then be set on this manifest with the SetSignature
 * method. Calling this method also internally sets the other fields needed to be a valid signed manifest,
 * leaving only the signature to be set later.
 *
 * Caller is responsible for verifying subjectCertificate is the signed certificate which will be used
 * by the peer using this manifest; no validation of this is done.
 *
 * @see setSignature:signature
 *
 * @param[in] subjectCertificate Signed certificate of the app which will use this manifest
 * @param[out] digest NSMutableData of bytes containing the digest to be signed
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)computeThumbprintAndDigest:(AJNCertificateX509*)subjectCertificate digest:(NSMutableData*)digest;

/**
 * Set this manifest for the use of a particular subject certificate thumbprint and compute the digest, to be signed
 * with ECDSA_SHA256 by the caller. That signature can then be set on this manifest with the SetSignature
 * method. Calling this method also internally sets the other fields needed to be a valid signed manifest,
 * leaving only the signature to be set later.
 *
 * Caller is responsible for verifying subjectCertificate is the signed certificate which will be used
 * by the peer using this manifest; no validation of this is done.
 *
 * @see ajn::_Manifest::SetSignature(const ECCSignature& signature)
 *
 * @param[in] subjectThumbprint SHA-256 thumbprint of the signed certificate of the app which will use this manifest
 * @param[out] digest Vector of bytes containing the digest to be signed
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)computeDigest:(NSMutableData*)subjectThumbprint digest:(NSMutableData*)digest;

/**
 * Set the subject certificate thumbprint for this manifest.
 *
 * @param[in] subjectThumbprint SHA-256 thumbprint of the signed certificate of the app which will use this manifest
 */
- (void)setSubjectThumbprintWithSHA:(NSMutableData*)subjectThumbprint;

/**
 * Set the subject certificate thumbprint for this manifest.
 *
 * @param[in] subjectCertificate Certificate those thumbprint will be set in this manifest.
 *
 * @return
 * - #ER_OK if successful
 * - other error if the thumbprint could not be computed
 */
- (QStatus)setSubjectThumbprintWithCertificate:(AJNCertificateX509*)subjectCertificate;

/**
 * Set the signature for this manifest generated by the caller.
 *
 * @param[in] signature ECCSignature containing the ECDSA-SHA256 signature.
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)setManifestSignature:(AJNECCSignature*)signature;

/**
 * Cryptographically sign this manifest for the use of a particular subject certificate using
 * the provided signing key. issuerPrivateKey must be the private key that signed the certificate
 * corresponding to the given thumbprint for apps to consider it valid. Caller must ensure the correct
 * issuer public key is provided; this method does not verify the correct key is provided.
 *
 * @param[in] subjectThumbprint SHA-256 thumbprint of the signed certificate of the app which will use this manifest
 * @param[in] issuerPrivateKey Private key of the issuer of a Subject Certificate corresponding to the subjectThumbprint
 *
 * @return
 * - #ER_OK if successful
 * - other error indicating failure
 */
- (QStatus)sign:(NSMutableData*)subjectThumbprint issuerPrivateKey:(AJNECCPrivateKey*)issuerPrivateKey;

/**
 * Cryptographically verify this manifest for the use of a particular subject certificate using
 * the provided issuer public key. issuerPublicKey must be the public key corresponding to the private key
 * which signed subjectCertificate.
 *
 * @param[in] subjectCertificate Signed certificate of the app using this manifest
 * @param[in] issuerPublicKey Public key of the issuer to verify the signature of this manifest
 *
 * @return
 * - #ER_OK if the manifest is cryptographically verified for use by subjectCertificate
 * - #ER_UNKNOWN_CERTIFICATE if the manifest is not for the use of subjectCertificate
 * - #ER_DIGEST_MISMATCH if the cryptographic signature is invalid
 * - #ER_NOT_IMPLEMENTED if the manifest uses an unsupported thumbprint or signature algorithm
 * - other error indicating failure
 */
- (QStatus)computeThumbprintAndVerify:(AJNCertificateX509*)subjectCertificate issuerPublicKey:(AJNECCPublicKey*)issuerPublicKey;

/**
 * Get a serialized form of this signed manifest.
 *
 * @param[out] serializedForm NSMutableArray containing the bytes of the serialized manifest.
 *
 * @return
 * - #ER_OK if serialization was successful
 * - other error code indicating failure
 */
- (QStatus)serialize:(NSMutableData*)serializedForm;

/**
 * Deserialize a manifest from a vector of bytes.
 *
 * @param[in] serializedForm NSMutableArray of bytes containing the serialized manifest
 *
 * @return
 * - #ER_OK if the manifest was successfully deserialized
 * - other error indicating failure
 */
- (QStatus)deserialize:(NSMutableData*)serializedForm;

/*
 * Get a string representation of this manifest.
 *
 * @return NSString containing the representation.
 */
- (NSString*)description;

@end

/**
 * Class to allow the application to specify a permission policy
 */
@interface AJNPermissionPolicy : AJNObject

@property (nonatomic, readonly) uint16_t specificationVersion;

@property (nonatomic, readonly) uint32_t version;

@property (nonatomic, readonly) NSMutableArray *acls;

/**
 * Construct an AJNPermissionPolicy
 */
- (id)init;

- (void)setSpecificationVersion:(uint16_t)specificationVersion;

/**
 * Set the version of the permission policy
 *
 * @param version policy version
 */
- (void)setVersion:(uint32_t)version;

/**
 * Set the array of permission acls
 * @param[in] acls      the array of permission acls.
 */
- (void)setAcls:(NSMutableArray*)acls;

/**
 * Serialize the permission policy to a byte array.
 * @param marshaller the marshaller
 * @param[out] buf the newly allocated byte array holding the serialized data. The caller must delete[] this buffer after use.
 * @param[out] size the variable holding the size of the allocated byte array
 * @return
 *      - #ER_OK if export was successful.
 *      - error code if fail
 */
- (QStatus)export:(AJNMarshaller*)marshaller buf:(uint8_t**)buf size:(size_t*)size;

/**
 * Export the Policy to a MsgArg object.
 * @param[out] msgArg the resulting message arg
 * @return
 *      - #ER_OK if creation was successful.
 *      - error code if fail
 */
- (QStatus)export:(AJNMessageArgument*)msgArg;

/**
 * Deserialize the permission policy from a byte array.
 * @param marshaller the marshaller
 * @param buf the byte array holding the serialized data. The serialized data must be generated by the Export call.
 * @param size the size of the byte array
 * @return
 *      - #ER_OK if import was successful.
 *      - error code if fail
 */
- (QStatus)import:(AJNMarshaller*)marshaller buf:(uint8_t*)buf size:(size_t)size;

/**
 * Build the policy object from the message arg object
 * @param specificationVersion  the specification version
 * @param msgArg      the message arg
 * @return
 *      - #ER_OK if creation was successful.
 *      - error code if fail
 */
- (QStatus)import:(uint16_t)specificationVersion msgArg:(AJNMessageArgument*)msgArg;

/**
 * Generate a hash digest for the policy data
 * @param marshaller the marshaller utility
 * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
 * @param len the length of the digest buffer.
 * @return
 *      - #ER_OK if digest was successful.
 *      - error code if fail
 */
- (QStatus)digest:(AJNMarshaller*)marshaller digest:(uint8_t*)digest length:(size_t)len;

/**
 * A String representation of the PermissionPolicy
 *
 * @return A String representation of the PermissionPolicy
 */
- (NSString*)description;

/**
 * A String representation of the PermissionPolicy
 *
 * @param indent Number of space chars to indent the start of each line
 *
 * @return A String representation of the PermissionPolicy
 */
- (NSString*)descriptionWithIndent:(size_t)indent;

/**
 * Comparison operator equality
 * @param[in] toPermissionPolicy right hand side PermissionPolicy
 * @return true if PermissionPolicies are equal
 */
- (BOOL)isEqual:(AJNPermissionPolicy*)toPermissionPolicy;

/**
 * Comparison operator non-equality
 * @param[in] toPermissionPolicy right hand side PermissionPolicy
 * @return true if PermissionPolicies are not equal
 */
- (BOOL)isNotEqual:(AJNPermissionPolicy*)toPermissionPolicy;

/**
 * Build a MsgArg object to represent the manifest template.
 * @param[in]   rules   The array of rules representing a manifest template.
 * @param[out]  msgArg  The resulting message arg.
 * @return
 *      - #ER_OK if creation was successful.
 *      - error code if fail.
 */
+ (QStatus)manifestTemplateToMsgArg:(NSArray*)rules inMsgArg:(AJNMessageArgument*)msgArg;

/**
 * Parse the MsgArg object to retrieve the manifest template rules.
 * @param[in]    msgArg  The MsgArg containing the manifest template rules.
 * @param[out]   rules   The vector to hold the array of rules.
 * @return
 *      - #ER_OK if creation was successful.
 *      - error code if fail.
 */
+ (QStatus)msgArgToManifestTemplate:(AJNMessageArgument*)msgArg withRules:(NSMutableArray*)rules;

/**
 * Helper method to change the Rule objects' type.
 * @param[in]    rules           Input rules.
 * @param[in]    ruleType        Type to change in to.
 * @param[out]   changedRules    Output changed rules.
 */
+ (void)changeRulesType:(NSMutableArray*)rules ruleType:(AJNRuleType)ruleType changedRules:(NSMutableArray*)changedRules;

@end

@interface AJNPermissionPolicy (Private)

@property (nonatomic, readonly) ajn::PermissionPolicy *permissionPolicy;

@end

class AJNMarshallerImpl : public ajn::PermissionPolicy::Marshaller {
    protected:
        /**
         * Objective C delegate called when one of the below virtual functions
         * is called.
         */
        __weak id<AJNMarshaller> m_delegate;

    public:
        /**
         * Constructor for the AJNMarshaller implementation.
         *
         * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
         */
        AJNMarshallerImpl(id<AJNMarshaller> aDelegate);

        /**
         * Virtual destructor for derivable class.
         */
        virtual ~AJNMarshallerImpl();

        /**
         * Marshal the permission policy to a byte array.
         *
         * @param[in] policy the policy to marshal into a byte array
         * @param[out] buf the newly allocated byte array holding the serialized data. The caller must delete[] this buffer after   use.
         * @param[out] size the variable holding the size of the allocated byte array
         * @return
         *      - #ER_OK if export was successful.
         *      - error code if fail
         */
        virtual QStatus Marshal(ajn::PermissionPolicy &policy, uint8_t **buf, size_t *size)
        {

            if ([m_delegate respondsToSelector:@selector(marshal:buf:size:)]) {
                AJNPermissionPolicy *ajnPolicy = [[AJNPermissionPolicy alloc] initWithHandle:(AJNHandle)&policy];
                return [m_delegate marhsal:ajnPolicy buf:buf size:size];
            }

            return ER_FAIL;
        }

        /**
         * Unmarshal the permission policy from a byte array.
         *
         * @param[out] policy the policy the byte array will be unmarshalled into
         * @param buf the byte array holding the serialized data. The serialized data must be generated by the Export call.
         * @param size the size of the byte array
         * @return
         *      - #ER_OK if import was successful.
         *      - error code if fail
         */
        virtual QStatus Unmarshal(ajn::PermissionPolicy& policy, const uint8_t* buf, size_t size)
        {
            QStatus status = ER_FAIL;
            if ([m_delegate respondsToSelector:@selector(unmarshal:policy:buf:size:)]) {
                AJNPermissionPolicy *ajnPolicy = nil;
                status = [m_delegate unmarshal:ajnPolicy buf:buf size:size];
                if (status == ER_OK && ajnPolicy != nil) {
                    policy = *ajnPolicy.permissionPolicy;
                }
            }

            return status;
        }
        /**
         * Generate a hash digest for the policy data.  Each marshaller can use its own digest algorithm.
         * @param[in] policy the policy used to generate the hash digest
         * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
         * @param len the length of the digest buffer.
         * @return
         *      - #ER_OK if digest was generated successfully.
         *      - error code if fail
         */
        virtual QStatus Digest(ajn::PermissionPolicy& policy, uint8_t* digest, size_t len)
        {
            if ([m_delegate respondsToSelector:@selector(digestWithPolicy:policy:digest:len:)]) {
                AJNPermissionPolicy *ajnPolicy = [[AJNPermissionPolicy alloc] initWithHandle:(AJNHandle)&policy];
                return [m_delegate digestWithPolicy:ajnPolicy digest:digest len:len];
            }

            return ER_FAIL;
        }

        /**
         * Generate a hash digest for the manifest template data.  Each marshaller can use its own digest algorithm.
         * @param[in] rules the array of rules in the manifest
         * @param[in] count the number of rules in the manifest
         * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
         * @param len the length of the digest buffer.
         * @return
         *      - #ER_OK if digest was successful.
         *      - error code if fail
         */
        virtual QStatus Digest(const ajn::PermissionPolicy::Rule* rules, size_t count, uint8_t* digest, size_t len)
        {
            if ([m_delegate respondsToSelector:@selector(digestWithRules:rules:digest:len:)]) {
                NSMutableArray *ruleList = [[NSMutableArray alloc] initWithCapacity:count];
                for (int i = 0; i < count; i++) {
                    ruleList[i] = [[AJNRule alloc] initWithHandle:(AJNHandle)&rules[i]];
                }

                return [m_delegate digestWithRules:ruleList digest:digest len:len];
            }

            return ER_FAIL;
        }

        /**
         * Accessor for Objective-C delegate.
         *
         * return delegate         The Objective-C delegate called to handle the above event methods.
         */
        id<AJNMarshaller> getDelegate();

        /**
         * Mutator for Objective-C delegate.
         *
         * @param delegate    The Objective-C delegate called to handle the above event methods.
         */
        void setDelegate(id<AJNMarshaller> delegate);
};

inline id<AJNMarshaller> AJNMarshallerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNMarshallerImpl::setDelegate(id<AJNMarshaller> delegate)
{
    m_delegate = delegate;
}

