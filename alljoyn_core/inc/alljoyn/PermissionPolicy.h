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
#include <alljoyn/Status.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Message.h>

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
        Rule() : objPath("*"), interfaceName(), members(NULL), membersSize(0)
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
         * @param[in] peer the array of peers.
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
         * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
         * @param len the length of the digest buffer.
         * @return
         *      - #ER_OK if digest was successful.
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
         * Generate a hash digest for the manifest data.  Each marshaller can use its own digest algorithm.
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
     * @param[in] provider the array of permission acls. The array must be new'd by the caller. It will be deleted by this object.
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
     * Build a MsgArg object to represent the array of rules.
     * @param rules the array of rules
     * @param count the number of rules
     * @param[out] msgArg the resulting message arg
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    static QStatus GenerateRules(const Rule* rules, size_t count, MsgArg& msgArg);

    /**
     * Parse the MsgArg object to retrieve the rules.
     * @param msgArg the message arg
     * @param[out] rules the buffer to hold the array of rules
     * @param[out] count the output number of rules
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    static QStatus ParseRules(const MsgArg& msgArg, Rule** rules, size_t* count);

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
 * Class used to serialize/deserialize the permission policy.
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
    QStatus Marshal(PermissionPolicy& policy, uint8_t** buf, size_t* size);

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
    QStatus Unmarshal(PermissionPolicy& policy, const uint8_t* buf, size_t size);

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
     * Generate a hash digest for the manifest data.  Each marshaller can use its own digest algorithm.
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
    Message& msg;
};

}
#endif
