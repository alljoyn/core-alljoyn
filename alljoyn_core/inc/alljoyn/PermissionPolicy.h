#ifndef _ALLJOYN_PERMISSION_POLICY_H
#define _ALLJOYN_PERMISSION_POLICY_H
/**
 * @file
 * This file defines the Permission Policy classes that provide the interface to
 * parse the authorization data
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef __cplusplus
#error Only include PermissionPolicy.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/GUID.h>
#include <qcc/CertificateECC.h>
#include <alljoyn/Status.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Message.h>

#include <vector>
#include <string>

namespace ajn {

/**
 * Class to allow the application to specify a permission policy
 */

class PermissionPolicy {
  public:
    /**
     * The current specification version.
     */
    static const uint16_t SPEC_VERSION = 1;

    /**
     * Class to allow the application to specify a permission rule
     */
    class Rule {
      public:

        /** MsgArg signature for a manifest or policy rule. */
        static AJ_PCSTR s_manifestOrPolicyRuleMsgArgSignature;

        /** MsgArg signature for a manifestTemplate rule. */
        static AJ_PCSTR s_manifestTemplateRuleMsgArgSignature;

        /**
         * Enum indicating the suggested security level for the interface
         */
        typedef enum {
            PRIVILEGED = 0,
            NON_PRIVILEGED = 1,
            UNAUTHENTICATED = 2
        } SecurityLevel;

        typedef enum {
            MANIFEST_POLICY_RULE = 0,
            MANIFEST_TEMPLATE_RULE = 1
        } RuleType;

        /**
         * Class to allow the application to specify a permission rule at the interface member level
         */
        class Member {
          public:
            /**
             * Enumeration for the different type of members
             */
            typedef enum {
                NOT_SPECIFIED = 0,  ///< not specified
                METHOD_CALL = 1,    ///< method call
                SIGNAL = 2,         ///< signal
                PROPERTY = 3        ///< property
            } MemberType;

            /**
             * The permission action masks
             */
            static const uint8_t ACTION_PROVIDE = 0x01;   /** < allow to provide */
            static const uint8_t ACTION_OBSERVE = 0x02;   /** < allow to observe */
            static const uint8_t ACTION_MODIFY = 0x04;   /** < allow to modify */

            /**
             * Constructor
             *
             */
            Member() : memberName(), memberType(NOT_SPECIFIED), actionMask(0)
            {
            }

            /**
             * virtual destructor
             */
            virtual ~Member()
            {
            }

            /**
             * Set the MemberName, MemberType and action mask
             *
             * @param[in] memberName the name of the interface member
             * @param[in] memberType the type of member
             * @param[in] actionMask the permission action mask
             */
            void Set(const qcc::String& memberName, MemberType memberType, uint8_t actionMask);

            /**
             * Set the MemberName
             * @param[in] memberName the name of the interface member
             */
            void SetMemberName(const qcc::String& memberName);

            /**
             * Get the MemberName
             * @return the MemberName
             */
            const qcc::String GetMemberName() const;

            /**
             * Set the MemberType
             * @param[in] memberType the type of the member
             */
            void SetMemberType(MemberType memberType);

            /**
             * Get the MemberType
             * @return the MemberType
             */
            const MemberType GetMemberType() const;

            /**
             * Set the ActionMask
             * param[in] actionMask Action mask value
             */
            void SetActionMask(uint8_t actionMask);

            /**
             * Get the ActionMask
             * @return the ActionMask
             */
            const uint8_t GetActionMask() const;

            /**
             * A String representation of the Member
             *
             * @param indent Number of space chars to indent the start of each line
             *
             * @return A String representation of the Member
             */
            qcc::String ToString(size_t indent = 0) const;

            /**
             * Comparison operators equality
             * @param[in] other right hand side Member
             * @return true if Members are equal
             */
            bool operator==(const Member& other) const;

            /**
             * Comparison operators non-equality
             * @param[in] other right hand side Member
             * @return true if Members are not equal
             */
            bool operator!=(const Member& other) const;

          private:
            qcc::String memberName;
            MemberType memberType;
            uint8_t actionMask;
        };

        /**
         * Constructor
         *
         */
        Rule() :
            objPath("*"),
            interfaceName(),
            members(NULL),
            membersSize(0),
            m_ruleType(MANIFEST_POLICY_RULE),
            m_recommendedSecurityLevel(PRIVILEGED)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Rule()
        {
            delete [] members;
        }

        /**
         * Export the rule to a MsgArg.
         * @param[out]   msgArg  MsgArg to be set with Rule's contents.
         * @return
         *      - #ER_OK if export was successful.
         *      - Error code if fail.
         */
        QStatus ToMsgArg(MsgArg& msgArg) const;

        /**
         * Import the rule from a MsgArg.
         * @param[in]   msgArg   MsgArg to import the Rule's contents from.
         * @param[in]   ruleType Type of imported rule.
         * @return
         *      - #ER_OK if export was successful.
         *      - Error code if fail.
         */
        QStatus FromMsgArg(const MsgArg& msgArg, RuleType ruleType);

        /**
         * Set the rule type.
         * @param[in]   ruleType  The rule's type.
         */
        void SetRuleType(RuleType ruleType);

        /**
         * Get the rule's type.
         * @return The rule's type.
         */
        RuleType GetRuleType() const;

        /**
         * Set the security level.
         * @param[in]   securityLevel  The interface's security level.
         */
        void SetRecommendedSecurityLevel(SecurityLevel securityLevel);

        /**
         * Get the security level.
         * @return The interface's security level.
         */
        SecurityLevel GetRecommendedSecurityLevel() const;

        /**
         * Set the object path
         * @param[in] objPath the object path
         */
        void SetObjPath(const qcc::String& objPath);

        /**
         * Get the Object Path
         * @return the Object Path
         */
        const qcc::String GetObjPath() const;

        /**
         * Set the Interface Name
         * @param[in] interfaceName the interface name.
         */
        void SetInterfaceName(const qcc::String& interfaceName);

        /**
         * Get the InterfaceName
         * @return the interface name
         */
        const qcc::String GetInterfaceName() const;

        /**
         * Set the array of members for the given interface.
         * @param[in] count   the size of the array
         * @param[in] members  The array of member fields.
         */
        void SetMembers(size_t count, Member* members);

        /**
         * Get the array of inferface members.
         * @return the array of interface members.
         */
        const Member* GetMembers() const;

        /**
         * Get the number of Members in the Rule
         * @return the number of Members in the rule.
         */
        const size_t GetMembersSize() const;

        /**
         * String representation of the Rule
         *
         * @param indent Number of space chars to indent the start of each line
         *
         * @return string representation of the rule
         */
        qcc::String ToString(size_t indent = 0) const;

        /**
         * Comparison operators equality
         * @param[in] other right hand side Rule
         * @return true if Rules are equal
         */
        bool operator==(const Rule& other) const;

        /**
         * Comparison operators non-equality
         * @param[in] other right hand side Rule
         * @return true if Rules are not equal
         */
        bool operator!=(const Rule& other) const;

        /**
         * Assignment operator for Rule
         */
        Rule& operator=(const Rule& other);

        /**
         * Copy constructor for Rule
         */
        Rule(const Rule& other);


      private:

        qcc::String objPath;
        qcc::String interfaceName;
        Member* members;
        size_t membersSize;
        RuleType m_ruleType;
        SecurityLevel m_recommendedSecurityLevel;
    };


    /**
     * Class to allow the application to specify a permission peer
     */
    class Peer {
      public:

        /**
         * Enumeration for the different types of peer
         */
        typedef enum {
            PEER_ALL = 0,          ///< all peers including anonymous peers
            PEER_ANY_TRUSTED = 1,  ///< any peer trusted by the application
            PEER_FROM_CERTIFICATE_AUTHORITY = 2,  ///< peers with identity certificates issued by the specified certificate authority
            PEER_WITH_PUBLIC_KEY = 3,  ///< peer identified by specific public key
            PEER_WITH_MEMBERSHIP = 4  ///< all members of a security group
        } PeerType;

        /**
         * Constructor
         *
         */
        Peer() : type(PEER_ANY_TRUSTED), securityGroupId(0), keyInfo(NULL)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Peer()
        {
            delete keyInfo;
        }

        /**
         * Set the peer type
         */
        void SetType(PeerType peerType);

        /**
         * Get the peer type
         */
        const PeerType GetType() const;

        /**
         * Set the security group id
         * @param guid the security group id
         */
        void SetSecurityGroupId(const qcc::GUID128& guid);

        /**
         * Get the security group id
         * @return the security group id
         */
        const qcc::GUID128& GetSecurityGroupId() const;

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
        void SetKeyInfo(const qcc::KeyInfoNISTP256* keyInfo);

        /**
         * Get the keyInfo field.
         * @return keyInfo the keyInfo.
         */
        const qcc::KeyInfoNISTP256* GetKeyInfo() const;

        /**
         * A String representation of the Peer
         *
         * @param indent Number of space chars to indent the start of each line
         *
         * @return A String representation of the Peer
         */
        qcc::String ToString(size_t indent = 0) const;

        /**
         * Comparison operators equality
         * @param[in] other right hand side Peer
         * @return true is Peers are equal
         */
        bool operator==(const Peer& other) const;

        /**
         * Comparison operators non-equality
         * @param[in] other right hand side Peer
         * @return true if Peers are not equal
         */
        bool operator!=(const Peer& other) const;

        /**
         * Assignment operator for Peer
         * @param[in] other right hand side of the `=` operator
         */
        Peer& operator=(const Peer& other);

        /**
         * Copy constructor for Peer
         * @param[in] other the Peer to copy
         */
        Peer(const Peer& other);

      private:
        PeerType type;
        qcc::GUID128 securityGroupId;
        qcc::KeyInfoNISTP256* keyInfo;
    };

    /**
     * Class to allow the application to specify an access control list
     */

    class Acl {

      public:

        /**
         * Constructor
         *
         */
        Acl() : peersSize(0), peers(NULL), rulesSize(0), rules(NULL)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Acl()
        {
            delete [] peers;
            delete [] rules;
        }

        /**
         * Set the array of peers
         * @param[in] count the number of peers
         * @param[in] peers the array of peers.
         */
        void SetPeers(size_t count, const Peer* peers);

        /**
         * Get the number of Peers in the Acl
         * @return the number of Peers in the Acl
         */
        const size_t GetPeersSize() const;

        /**
         * Get a pointer to the Peers array stored in the Acl
         */
        const Peer* GetPeers() const;

        /**
         * Set the array of rules.
         * @param[in] count the number of rules
         * @param[in] rules the array of rules.
         */
        void SetRules(size_t count, const Rule* rules);

        /**
         * Get the number of Rules in the Acl
         * @return the number of Rules in the Acl
         */
        const size_t GetRulesSize() const;

        /**
         * Get a pointer to the Rules array stored in the Acl
         */
        const Rule* GetRules() const;

        /**
         * Get a string representation of the Acl
         *
         * @param indent Number of space chars to indent the start of each line
         *
         * @return a string representation of the Acl
         */
        qcc::String ToString(size_t indent = 0) const;

        /**
         * Comparison operators equality
         * @param[in] other right hand side Acl
         * @return true if Acls are equal
         */
        bool operator==(const Acl& other) const;

        /**
         * Comparison operators non-equality
         * @param[in] other right hand side Acl
         * @return true if Acls are not equal
         */
        bool operator!=(const Acl& other) const;

        /**
         * Assignment operator for Term
         */
        Acl& operator=(const Acl& other);

        /**
         * Copy constructor for Term
         */
        Acl(const Acl& other);

      private:
        size_t peersSize;
        Peer* peers;
        size_t rulesSize;
        Rule* rules;
    };

    /**
     * Class to specify the marshal/unmarshal utility for the policy data
     */
    class Marshaller {
      public:
        Marshaller()
        {
        }

        virtual ~Marshaller()
        {
        }

        /**
         * Marshal the permission policy to a byte array.
         * @param[in] policy the policy to marshal into a byte array
         * @param[out] buf the buffer containing the marshalled data. The buffer is new[]'d by this object and will be delete[]'d by the caller of this method.
         * @param[out] size the variable holding the size of the allocated byte array
         * @return
         *      - #ER_OK if export was successful.
         *      - error code if fail
         */
        virtual QStatus Marshal(PermissionPolicy& policy, uint8_t** buf, size_t* size)
        {
            QCC_UNUSED(policy);
            QCC_UNUSED(buf);
            QCC_UNUSED(size);
            return ER_NOT_IMPLEMENTED;
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
        virtual QStatus Unmarshal(PermissionPolicy& policy, const uint8_t* buf, size_t size)
        {
            QCC_UNUSED(policy);
            QCC_UNUSED(buf);
            QCC_UNUSED(size);
            return ER_NOT_IMPLEMENTED;
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
        virtual QStatus Digest(PermissionPolicy& policy, uint8_t* digest, size_t len)
        {
            QCC_UNUSED(policy);
            QCC_UNUSED(digest);
            QCC_UNUSED(len);
            return ER_NOT_IMPLEMENTED;
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
        virtual QStatus Digest(const PermissionPolicy::Rule* rules, size_t count, uint8_t* digest, size_t len)
        {
            QCC_UNUSED(rules);
            QCC_UNUSED(count);
            QCC_UNUSED(digest);
            QCC_UNUSED(len);
            return ER_NOT_IMPLEMENTED;
        }
    };

    /**
     * Constructor
     *
     */
    PermissionPolicy() : specificationVersion(SPEC_VERSION), version(0), aclsSize(0), acls(NULL)
    {
    }

    /**
     * virtual destructor
     */
    virtual ~PermissionPolicy()
    {
        delete [] acls;
    }


    void SetSpecificationVersion(uint16_t specificationVersion)
    {
        this->specificationVersion = specificationVersion;
    }
    const uint16_t GetSpecificationVersion() const
    {
        return specificationVersion;
    }

    void SetVersion(uint32_t version)
    {
        this->version = version;
    }
    const uint32_t GetVersion() const
    {
        return version;
    }

    /**
     * Set the array of permission acls
     * @param[in] count the number of permission acls
     * @param[in] acls the array of permission acls.
     */
    void SetAcls(size_t count, const Acl* acls);

    const size_t GetAclsSize() const
    {
        return aclsSize;
    }

    const Acl* GetAcls() const
    {
        return acls;
    }

    /**
     * A String representation of the PermissionPolicy
     *
     * @param indent Number of space chars to indent the start of each line
     *
     * @return A String representation of the PermissionPolicy
     */
    qcc::String ToString(size_t indent = 0) const;

    /**
     * Comparison operator equality
     * @param[in] other right hand side PermissionPolicy
     * @return true if PermissionPolicies are equal
     */
    bool operator==(const PermissionPolicy& other) const;

    /**
     * Comparison operator non-equality
     * @param[in] other right hand side PermissionPolicy
     * @return true if PermissionPolicies are not equal
     */
    bool operator!=(const PermissionPolicy& other) const;

    /**
     * Serialize the permission policy to a byte array.
     * @param marshaller the marshaller
     * @param[out] buf the newly allocated byte array holding the serialized data. The caller must delete[] this buffer after use.
     * @param[out] size the variable holding the size of the allocated byte array
     * @return
     *      - #ER_OK if export was successful.
     *      - error code if fail
     */
    QStatus Export(Marshaller& marshaller, uint8_t** buf, size_t* size);

    /**
     * Deserialize the permission policy from a byte array.
     * @param marshaller the marshaller
     * @param buf the byte array holding the serialized data. The serialized data must be generated by the Export call.
     * @param size the size of the byte array
     * @return
     *      - #ER_OK if import was successful.
     *      - error code if fail
     */
    QStatus Import(Marshaller& marshaller, const uint8_t* buf, size_t size);

    /**
     * Export the Policy to a MsgArg object.
     * @param[out] msgArg the resulting message arg
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    QStatus Export(MsgArg& msgArg) const;

    /**
     * Build a MsgArg object to represent the manifest template.
     * @param[in]   rules   The array of rules representing a manifest template.
     * @param[in]   count   The number of rules.
     * @param[out]  msgArg  The resulting message arg.
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail.
     */
    static QStatus ManifestTemplateToMsgArg(const Rule* rules, size_t count, MsgArg& msgArg);

    /**
     * Parse the MsgArg object to retrieve the manifest template rules.
     * @param[in]    msgArg  The MsgArg containing the manifest template rules.
     * @param[out]   rules   The vector to hold the array of rules.
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail.
     */
    static QStatus MsgArgToManifestTemplate(const MsgArg& msgArg, std::vector<Rule>& rules);

    /**
     * Helper method to change the Rule objects' type.
     * @param[in]    rules           Input rules.
     * @param[in]    ruleType        Type to change in to.
     * @param[out]   changedRules    Output changed rules.
     */
    static void ChangeRulesType(const std::vector<PermissionPolicy::Rule>& rules, PermissionPolicy::Rule::RuleType ruleType, std::vector<PermissionPolicy::Rule>& changedRules);

    /**
     * Build the policy object from the message arg object
     * @param specificationVersion  the specification version
     * @param msgArg      the message arg
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    QStatus Import(uint16_t specificationVersion, const MsgArg& msgArg);

    /**
     * Generate a hash digest for the policy data
     * @param marshaller the marshaller utility
     * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
     * @param len the length of the digest buffer.
     * @return
     *      - #ER_OK if digest was successful.
     *      - error code if fail
     */
    QStatus Digest(Marshaller& marshaller, uint8_t* digest, size_t len);

    /**
     * Assignment operator for PermissionPolicy
     */
    PermissionPolicy& operator=(const PermissionPolicy& other);

    /**
     * Copy constructor for PermissionPolicy
     */
    PermissionPolicy(const PermissionPolicy& other);

  private:
    uint16_t specificationVersion;
    uint32_t version;
    size_t aclsSize;
    Acl* acls;
};

/**
 * Class used to serialize/deserialize the permission policy contents.
 */
class DefaultPolicyMarshaller : public PermissionPolicy::Marshaller {

  public:

    /**
     * Default constructor
     */
    DefaultPolicyMarshaller(Message& msg) : Marshaller(), msg(msg)
    {
    }

    /**
     * Destructor
     */
    ~DefaultPolicyMarshaller()
    {
    }

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
    virtual QStatus Marshal(PermissionPolicy& policy, uint8_t** buf, size_t* size);

    /**
     * Marshals the rules array representing a manifest template to a byte array.
     * @param[in]    rules       Rules array to marshal into a byte array.
     * @param[in]    rulesCount  Number of rules in the array.
     * @param[out]   buf         The buffer containing the marshalled data.
     * @return
     *      - #ER_OK if export was successful.
     *      - Error code if fail.
     */
    QStatus MarshalManifestTemplate(const PermissionPolicy::Rule* rules, size_t rulesCount, std::vector<uint8_t>& buf);

    /**
     * Unmarshals the rules array representing a manifest template from a byte array.
     * @param[in]   buf     The byte array holding the serialized data.
     *                      The serialized data must be generated by the MarshalManifestTemplate call.
     * @param[in]   size    The size of the byte array.
     * @param[out]  rules   The rules vector the byte array will be unmarshalled into.
     * @return
     *      - #ER_OK if import was successful.
     *      - Error code if fail.
     */
    QStatus UnmarshalManifestTemplate(const uint8_t* buf, size_t size, std::vector<PermissionPolicy::Rule>& rules);

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
    virtual QStatus Unmarshal(PermissionPolicy& policy, const uint8_t* buf, size_t size);

    /**
     * Generate a hash digest for the policy data
     *
     * @param[in] policy the policy used to generate the hash digest
     * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
     * @param len the length of the digest buffer.
     * @return
     *      - #ER_OK if digest was successful.
     *      - error code if fail
     */
    QStatus Digest(PermissionPolicy& policy, uint8_t* digest, size_t len);

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
    QStatus Digest(const PermissionPolicy::Rule* rules, size_t count, uint8_t* digest, size_t len);

  private:
    /**
     * Assignment operator is private
     */
    DefaultPolicyMarshaller& operator=(const DefaultPolicyMarshaller& other);

    /**
     * Copy constructor is private
     */
    DefaultPolicyMarshaller(const DefaultPolicyMarshaller& other);

    QStatus MarshalPrep(PermissionPolicy& policy);
    QStatus MarshalPrep(const PermissionPolicy::Rule* rules, size_t count);
    QStatus SaveArgInErrorMessage(const MsgArg& arg);
    Message& msg;
};

/**
 * Class used to encapsulate and manipulate a manifest.
 */

class _Manifest;

typedef qcc::ManagedObj<_Manifest> Manifest;

class _Manifest {

    friend class AllJoynPeerObj;
    friend class PermissionMgmtObj;
    friend class SecurityApplicationObj;
    friend class SecurityApplicationProxy;
    friend class XmlManifestConverter;

  public:
    /** MsgArg signature for an array of signed manifests. */
    static AJ_PCSTR s_MsgArgArraySignature;
    /** MsgArg signature for a single signed manifest. */
    static AJ_PCSTR s_MsgArgSignature;
    /** MsgArg signature for a single signed manifest without the cryptographic signature field. */
    static AJ_PCSTR s_MsgArgDigestSignature;
    /** MsgArg signature for a 16.10 manifest template. */
    static AJ_PCSTR s_ManifestTemplateMsgArgSignature;

    /** Default version number for new manifests. */
    static const uint32_t DefaultVersion;

    /**
     * Constructor.
     */
    _Manifest();

    /**
     * Copy constructor.
     * @param[in] other Manifest being copied
     */
    _Manifest(const _Manifest& other);

    /**
     * Destructor.
     */
    ~_Manifest();

    /**
     * Assignment operator.
     * @param[in] other Manifest being assigned from
     */
    _Manifest& operator=(const _Manifest& other);

    /**
     * Equality operator.
     * @param[in] other Manifest to compare against
     *
     * @return true if contents are equal, false otherwise
     */
    bool operator==(const _Manifest& other) const;

    /**
     * Inequality operator.
     * @param[in] other Manifest to compare against
     *
     * @return true if the contents are different, false if they are equal
     */
    bool operator!=(const _Manifest& other) const;

    /**
     * Set the rules to be set on this manifest. After calling SetRules, the cryptographic
     * signature on this Manifest will no lnoger be valid; it will need to be signed again with the
     * Sign method before applying to an application.
     *
     * @param[in] rules Array of PermissionPolicy::Rule objects
     * @param[in] ruleCount Number of elements in rules array
     *
     * @return
     * - #ER_OK if successful
     * - other error indicating failure
     */
    QStatus SetRules(const PermissionPolicy::Rule* rules, size_t rulesCount);

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
    QStatus ComputeThumbprintAndSign(const qcc::CertificateX509& subjectCertificate, const qcc::ECCPrivateKey* issuerPrivateKey);

    /**
     * Set this manifest for the use of a particular subject certificate and compute the digest, to be signed
     * with ECDSA_SHA256 by the caller. That signature can then be set on this manifest with the SetSignature
     * method. Calling this method also internally sets the other fields needed to be a valid signed manifest,
     * leaving only the signature to be set later.
     *
     * Caller is responsible for verifying subjectCertificate is the signed certificate which will be used
     * by the peer using this manifest; no validation of this is done.
     *
     * @see ajn::_Manifest::SetSignature(const uint8_t* signature, size_t signatureSize)
     *
     * @param[in] subjectCertificate Signed certificate of the app which will use this manifest
     * @param[out] digest Vector of bytes containing the digest to be signed
     *
     * @return
     * - #ER_OK if successful
     * - other error indicating failure
     */
    QStatus ComputeThumbprintAndDigest(const qcc::CertificateX509& subjectCertificate, std::vector<uint8_t>& digest);

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
    QStatus ComputeDigest(const std::vector<uint8_t>& subjectThumbprint, std::vector<uint8_t>& digest);

    /**
     * Set the subject certificate thumbprint for this manifest.
     *
     * @param[in] subjectThumbprint SHA-256 thumbprint of the signed certificate of the app which will use this manifest
     */
    void SetSubjectThumbprint(const std::vector<uint8_t>& subjectThumbprint);

    /**
     * Set the subject certificate thumbprint for this manifest.
     *
     * @param[in] subjectCertificate Certificate those thumbprint will be set in this manifest.
     *
     * @return
     * - #ER_OK if successful
     * - other error if the thumbprint could not be computed
     */
    QStatus SetSubjectThumbprint(const qcc::CertificateX509& subjectCertificate);

    /**
     * Set the signature for this manifest generated by the caller.
     *
     * @param[in] signature ECCSignature containing the ECDSA-SHA256 signature.
     *
     * @return
     * - #ER_OK if successful
     * - other error indicating failure
     */
    QStatus SetSignature(const qcc::ECCSignature& signature);

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
    QStatus Sign(const std::vector<uint8_t>& subjectThumbprint, const qcc::ECCPrivateKey* issuerPrivateKey);

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
    QStatus ComputeThumbprintAndVerify(const qcc::CertificateX509& subjectCertificate, const qcc::ECCPublicKey* issuerPublicKey) const;

    /**
     * @internal
     *
     * Cryptographically verify this manifest for the use of a particular subject certificate thumbprint using
     * the provided issuer public key.
     *
     * @param[in] subjectThumbprint SHA-256 thumbprint of the signed certificate of the app using this manifest
     * @param[in] issuerPublicKey Public key of the issuer of a Subject Certificate corresponding to the subjectThumbprint
     *
     * @return
     * - #ER_OK if the manifest is cryptographically verified for use by subjectThumbprint
     * - #ER_UNKNOWN_CERTIFICATE if the manifest is not for the use of subjectThumbprint
     * - #ER_DIGEST_MISMATCH if the cryptographic signature is invalid
     * - #ER_NOT_IMPLEMENTED if the manifest uses an unsupported thumbprint or signature algorithm
     * - other error indicating failure
     */
    QStatus Verify(const std::vector<uint8_t>& subjectThumbprint, const qcc::ECCPublicKey* issuerPublicKey) const;

    /**
     * Get version number of this manifest.
     *
     * @return version number
     */
    uint32_t GetVersion() const;

    /**
     * Get rules of this manifest.
     *
     * @return Vector of PermissionPolicy::Rule objects
     */
    const std::vector<PermissionPolicy::Rule>& GetRules() const;

    /**
     * Get the OID of the algorithm used to compute the certificate thumbprint.
     *
     * @return std::string containing the OID
     */
    std::string GetThumbprintAlgorithmOid() const;

    /**
     * Get the certificate thumbprint.
     *
     * @return Vector of bytes containing the thumbprint
     */
    std::vector<uint8_t> GetThumbprint() const;

    /**
     * Get the OID used to compute the signature.
     *
     * @return std::string containing the OID
     */
    std::string GetSignatureAlgorithmOid() const;

    /**
     * Get the signature.
     *
     * @return Vector of bytes containing the signature
     */
    std::vector<uint8_t> GetSignature() const;

    /**
     * Get a serialized form of this signed manifest.
     *
     * @param[out] serializedForm Vector containing the bytes of the serialized manifest.
     *
     * @return
     * - #ER_OK if serialization was successful
     * - other error code indicating failure
     */
    QStatus Serialize(std::vector<uint8_t>& seralizedForm) const;

    /**
     * Deserialize a manifest from a vector of bytes.
     *
     * @param[in] serializedForm Vector of bytes containing the serialized manifest
     *
     * @return
     * - #ER_OK if the manifest was successfully deserialized
     * - other error indicating failure
     */
    QStatus Deserialize(const std::vector<uint8_t>& serializedForm);

    /*
     * Get a string representation of this manifest.
     *
     * @return String containing the representation.
     */
    std::string ToString() const;

  private:

    typedef enum {
        MANIFEST_FULL = 0,
        MANIFEST_FOR_DIGEST = 1,
        MANIFEST_PURPOSE_MAX
    } ManifestPurpose;

    /**
     * @internal
     *
     * Set the contents of this Manifest from a signed manifest MsgArg.
     *
     * @param[in] manifestArg A MsgArg with the MsgArgSignature signature containing a signed manifest
     *
     * @return
     * - #ER_OK if successful
     * - #ER_INVALID_DATA if the version number is unsupported
     * - other error indicating failure
     */
    QStatus SetFromMsgArg(const MsgArg& manifestArg);

    /**
     * @internal
     *
     * Get a MsgArg containing the contents of this manifest.
     *
     * @param[in] manifestPurpose Specifies the purpose for the output form of this manifest
     * @param[out] outputArg MsgArg to receive the output
     *
     * @return
     * - #ER_OK if successful
     * - other error indicating failure
     */
    QStatus GetMsgArg(ManifestPurpose manifestPurpose, MsgArg& outputArg) const;

    /**
     * @internal
     *
     * Serialize a manifest to a vector of bytes
     *
     * @param[in] manifestPurpose Specifies the purpose for the serialized form of this manifest
     * @param[out] serializedForm Vector of bytes to receive serialized form
     *
     * @return
     * - #ER_OK if the manifest was successfully serialized
     * - other error indicating failure
     */
    QStatus Serialize(ManifestPurpose manifestPurpose, std::vector<uint8_t>& serializedForm) const;

    /**
     * @internal
     *
     * Serialize a vector of manifests into a vector of bytes.
     *
     * @param[in] manifests Vector of manifests to serialize.
     * @param[out] serializedForm Vector to receive bytes of serialized array.
     *
     * @return
     * - #ER_OK if manifests are successfully serialized
     * - other error indicating failure
     */
    static QStatus SerializeArray(const std::vector<Manifest>& manifests, std::vector<uint8_t>& serializedForm);

    /**
     * @internal
     * Deserialize a vector of manifests from a vector of bytes.
     *
     * @param[in] serializedForm Vector of bytes containing serialized array of manifests
     * @param[out] manifests Vector to receive deserialized manifests
     *
     * @return
     * - #ER_OK if manifests are successfully deserialized
     * - other error indicating failure
     */
    static QStatus DeserializeArray(const std::vector<uint8_t>& serializedForm, std::vector<Manifest>& manifests);

    /**
     * @internal
     *
     * Deserialize a vector of manifests from a byte array.
     *
     * @param[in] serializedForm Array of bytes containing serialized array of manifests
     * @param[in] serializedSize Number of bytes in serializedForm
     * @param[out] manifests Vector to receive deserialized manifests
     *
     * @return
     * - #ER_OK if manifests are successfully deserialized
     * - other error indicating failure
     */
    static QStatus DeserializeArray(const uint8_t* serializedForm, size_t serializedSize, std::vector<Manifest>& manifests);

    /**
     * @internal
     *
     * Get a MsgArg that contains an array of signed manifests suitable for method calls that transport
     * arrays of signed manifests.
     *
     * @param[in] manifests Vector of manifests to put into the array MsgArg.
     * @param[out] outputArg MsgArg to receive the output.
     *
     * @return
     * - #ER_OK if outputArg is successfully populated with the array of signed manifests.
     * - other error indicating failure
     */
    static QStatus GetArrayMsgArg(const std::vector<Manifest>& manifests, MsgArg& outputArg);

    /**
     * @internal
     *
     * Get a MsgArg that contains an array of signed manifests suitable for method calls that transport
     * arrays of signed manifests.
     *
     * @param[in] manifests Array of manifests to put into the array MsgArg.
     * @param[in] manifestCount Number of elements in manifests array
     * @param[out] outputArg MsgArg to receive the output.
     *
     * @return
     * - #ER_OK if outputArg is successfully populated with the array of signed manifests.
     * - other error indicating failure
     */
    static QStatus GetArrayMsgArg(const Manifest* manifests, size_t manifestCount, MsgArg& outputArg);

    /**
     * @internal
     *
     * Determine whether or not a given manifest version number is supported by this version of AllJoyn.
     *
     * @param[in] version Version number to check.
     *
     * @return true if manifest version is supported, false otherwise
     */
    static bool IsVersionSupported(uint32_t version);

    /**
     * @internal
     *
     * Determine if a manifest has been signed by looking for the presence of the thumbprint and
     * signature fields. This does not verify the cryptographic signature, as that requires access
     * to the public key of the signer which may not be available. Instead, this allows rejecting
     * unsigned manifests which can never be valid.
     *
     * @return true if the manifest has been signed, false otherwise
     */
    bool HasSignature() const;

    QStatus GetDigest(std::vector<uint8_t>& digest) const;
    QStatus GetECCSignature(qcc::ECCSignature& signature) const;
    QStatus SetECCSignature(const qcc::ECCSignature& signature);

    uint32_t m_version;
    std::vector<PermissionPolicy::Rule> m_rules;
    std::string m_thumbprintAlgorithmOid;
    std::vector<uint8_t> m_thumbprint;
    std::string m_signatureAlgorithmOid;
    std::vector<uint8_t> m_signature;
};



}
#endif
