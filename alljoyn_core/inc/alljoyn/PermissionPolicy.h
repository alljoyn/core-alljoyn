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
     * Class to allow the application to specify a permission rule
     */

    /**
     * The current specification version.
     */
    static const uint16_t SPEC_VERSION = 1;

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

            void SetMemberName(qcc::String memberName)
            {
                this->memberName = memberName;
            }

            const qcc::String GetMemberName() const
            {
                return memberName;
            }

            void SetMemberType(MemberType memberType)
            {
                this->memberType = memberType;
            }

            const MemberType GetMemberType() const
            {
                return memberType;
            }

            void SetActionMask(uint8_t actionMask)
            {
                this->actionMask = actionMask;
            }

            const uint8_t GetActionMask() const
            {
                return actionMask;
            }

            qcc::String ToString() const;

            bool operator==(const Member& m) const
            {
                if (memberName != m.memberName) {
                    return false;
                }

                if (memberType != m.memberType) {
                    return false;
                }

                if (actionMask != m.actionMask) {
                    return false;
                }

                return true;
            }

            /**
             * Assignment operator for Member
             */
            Member& operator=(const Member& other);

            /**
             * Copy constructor for Member
             */
            Member(const Member& other);

          private:

            qcc::String memberName;
            MemberType memberType;
            uint8_t actionMask;
        };

        /**
         * Constructor
         *
         */
        Rule() : objPath(), interfaceName(), members(NULL), membersSize(0)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Rule()
        {
            delete [] members;
        }

        void SetObjPath(qcc::String objPath)
        {
            this->objPath = objPath;
        }

        const qcc::String GetObjPath() const
        {
            return objPath;
        }

        void SetInterfaceName(qcc::String interfaceName)
        {
            this->interfaceName = interfaceName;
        }

        const qcc::String GetInterfaceName() const
        {
            return interfaceName;
        }

        /**
         * Set the array of members for the given interface.
         * @param   count   the size of the array
         * @param members  The array of member fields.  The memory for the array must be new'd by the caller and
         *          will be deleted by this object.
         */
        void SetMembers(size_t count, Member* members)
        {
            delete [] this->members;
            this->members = members;
            membersSize = count;
        }

        /**
         * Get the array of inferface members.
         * @return the array of interface members.
         */
        const Member* GetMembers() const
        {
            return members;
        }

        const size_t GetMembersSize() const
        {
            return membersSize;
        }

        qcc::String ToString() const;

        bool operator==(const Rule& r) const
        {
            if (objPath != r.objPath) {
                return false;
            }

            if (interfaceName != r.interfaceName) {
                return false;
            }

            if (membersSize != r.membersSize) {
                return false;
            }

            for (size_t i = 0; i < membersSize; i++) {
                if (!(members[i] == r.members[i])) {
                    return false;
                }
            }

            return true;
        }

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
        void SetType(PeerType peerType)
        {
            type = peerType;
        }

        /**
         * Get the peer type
         */
        const PeerType GetType() const
        {
            return type;
        }

        /**
         * Set the security group id
         * @param guid the security group id
         */
        void SetSecurityGroupId(const qcc::GUID128& guid)
        {
            securityGroupId = guid;
        }

        /**
         * Get the security group id
         * @return the security group id
         */
        const qcc::GUID128& GetSecurityGroupId() const
        {
            return securityGroupId;
        }

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
         * @param keyInfo the keyInfo. The keyInfo must be new'd by the caller. It will be deleted by this object.
         */
        void SetKeyInfo(qcc::KeyInfoECC* keyInfo)
        {
            delete this->keyInfo;
            this->keyInfo = keyInfo;
        }

        /**
         * Get the keyInfo field.
         * @return keyInfo the keyInfo.
         */
        const qcc::KeyInfoECC* GetKeyInfo() const
        {
            return keyInfo;
        }

        qcc::String ToString() const;

        bool operator==(const Peer& p) const
        {
            if (type != p.type) {
                return false;
            }

            if (type == PEER_WITH_MEMBERSHIP) {
                if (securityGroupId != p.GetSecurityGroupId()) {
                    return false;
                }
            }
            if (keyInfo == NULL || p.keyInfo == NULL) {
                return keyInfo == p.keyInfo;
            }

            if (!(*keyInfo == *p.keyInfo)) {
                return false;
            }

            return true;
        }

        /**
         * Assignment operator for Peer
         */
        Peer& operator=(const Peer& other);

        /**
         * Copy constructor for Peer
         */
        Peer(const Peer& other);

      private:
        PeerType type;
        qcc::GUID128 securityGroupId;
        qcc::KeyInfoECC* keyInfo;
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
         * @param count the number of peers
         * @param peer the array of peers. The array must be new'd by the caller. It will be deleted by this object.
         */
        void SetPeers(size_t count, Peer* peers)
        {
            delete [] this->peers;
            this->peers = peers;
            peersSize = count;
        }

        const size_t GetPeersSize() const
        {
            return peersSize;
        }

        const Peer* GetPeers() const
        {
            return peers;
        }

        /**
         * Set the array of rules.
         * @param count the number of rules
         * @param rules the array of rules. The array must be new'd by the caller.  It will be deleted by this object.
         */
        void SetRules(size_t count, Rule* rules)
        {
            delete [] this->rules;
            rulesSize = count;
            this->rules = rules;
        }

        const size_t GetRulesSize() const
        {
            return rulesSize;
        }

        const Rule* GetRules() const
        {
            return rules;
        }

        qcc::String ToString() const;

        bool operator==(const Acl& t) const
        {
            if (peersSize != t.peersSize) {
                return false;
            }

            for (size_t i = 0; i < peersSize; i++) {
                if (!(peers[i] == t.peers[i])) {
                    return false;
                }
            }

            if (rulesSize != t.rulesSize) {
                return false;
            }

            for (size_t i = 0; i < rulesSize; i++) {
                if (!(rules[i] == t.rules[i])) {
                    return false;
                }
            }

            return true;
        }

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
     * @param count the number of permission acls
     * @param provider the array of permission acls. The array must be new'd by the caller. It will be deleted by this object.
     */
    void SetAcls(size_t count, Acl* acls)
    {
        delete [] this->acls;
        aclsSize = count;
        this->acls = acls;
    }

    const size_t GetAclsSize() const
    {
        return aclsSize;
    }

    const Acl* GetAcls() const
    {
        return acls;
    }

    qcc::String ToString() const;

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

class DefaultPolicyMarshaller : public PermissionPolicy::Marshaller {

  public:

    DefaultPolicyMarshaller(Message& msg) : Marshaller(), msg(msg)
    {
    }

    ~DefaultPolicyMarshaller()
    {
    }

    /**
     * Marshal the permission policy to a byte array.
     * @param[out] buf the newly allocated byte array holding the serialized data. The caller must delete[] this buffer after use.
     * @param[out] size the variable holding the size of the allocated byte array
     * @return
     *      - #ER_OK if export was successful.
     *      - error code if fail
     */
    QStatus Marshal(PermissionPolicy& policy, uint8_t** buf, size_t* size);

    /**
     * Unmarshal the permission policy from a byte array.
     * @param buf the byte array holding the serialized data. The serialized data must be generated by the Export call.
     * @param size the size of the byte array
     * @return
     *      - #ER_OK if import was successful.
     *      - error code if fail
     */
    QStatus Unmarshal(PermissionPolicy& policy, const uint8_t* buf, size_t size);

    /**
     * Generate a hash digest for the policy data
     * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest
     * @param len the length of the digest buffer.
     * @return
     *      - #ER_OK if digest was successful.
     *      - error code if fail
     */
    QStatus Digest(PermissionPolicy& policy, uint8_t* digest, size_t len);

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
    Message& msg;
};

}
#endif
