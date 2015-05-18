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
            static const uint8_t ACTION_DENIED = 0x01;   /** < permission denied */
            static const uint8_t ACTION_PROVIDE = 0x02;   /** < allow to provide */
            static const uint8_t ACTION_OBSERVE = 0x04;   /** < allow to observe */
            static const uint8_t ACTION_MODIFY = 0x08;   /** < allow to modify */

            /**
             * Enumeration for the different types of field id
             */
            typedef enum {
                TAG_MEMBER_NAME = 1,  ///< tag for member field
                TAG_MEMBER_TYPE = 2,  ///< tag for member type field
                TAG_ACTION_MASK = 3,    ///< tag for action mask field
                TAG_MUTUAL_AUTH = 4   ///< tag for mutual auth field
            } TagType;

            /**
             * Constructor
             *
             */
            Member() : memberName(), memberType(NOT_SPECIFIED), actionMask(0), mutualAuth(true)
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

            void SetMutualAuth(bool flag)
            {
                mutualAuth = flag;
            }

            const bool GetMutualAuth() const
            {
                return mutualAuth;
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

                if (mutualAuth != m.mutualAuth) {
                    return false;
                }

                return true;
            }

          private:
            /**
             * Assignment operator is private
             */
            Member& operator=(const Member& other);

            /**
             * Copy constructor is private
             */
            Member(const Member& other);

            qcc::String memberName;
            MemberType memberType;
            uint8_t actionMask;
            bool mutualAuth;
        };

        /**
         * Enumeration for the different types of field id
         */
        typedef enum {
            TAG_OBJPATH = 1,           ///< tag for object path field
            TAG_INTERFACE_NAME = 2,    ///< tag for interface name field
            TAG_INTERFACE_MEMBERS = 3  ///< tag for interface members field
        } TagType;

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

      private:
        /**
         * Assignment operator is private
         */
        Rule& operator=(const Rule& other);

        /**
         * Copy constructor is private
         */
        Rule(const Rule& other);

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
            PEER_LEVEL_NONE = 0,            ///< No encryption
            PEER_LEVEL_ENCRYPTED = 1,       ///< message are encrypted
            PEER_LEVEL_AUTHENTICATED = 2,   ///< peer is authenticated
            PEER_LEVEL_AUTHORIZED = 3       ///< peer is fully authorized
        } PeerAuthLevel;

        /**
         * Enumeration for the different types of peer
         */
        typedef enum {
            PEER_ANY = 0,   ///< any peer
            PEER_GUID = 1,  ///< peer GUID
            PEER_GUILD = 2  ///< peer is a guild
        } PeerType;

        /**
         * Constructor
         *
         */
        Peer() : level(PEER_LEVEL_AUTHENTICATED), type(PEER_ANY), guildId(0), keyInfo(NULL)
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
         * Set the peer authentication level
         */
        void SetLevel(PeerAuthLevel peerLevel)
        {
            level = peerLevel;
        }

        /**
         * Get the peer authentication level
         */
        const PeerAuthLevel GetLevel() const
        {
            return level;
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
         * Set the guild id
         * @param guid the guild id
         */
        void SetGuildId(const qcc::GUID128& guid)
        {
            guildId = guid;
        }

        /**
         * Get the guild id
         * @return the guild id
         */
        const qcc::GUID128& GetGuildId() const
        {
            return guildId;
        }

        /**
         * Set the keyInfo field.
         * When peer type is PEER_ANY the keyInfo is not relevant.
         * When peer type is PEER_GUID the only the public key in the keyInfo is relevant.
         * When peer type is PEER_GUILD the keyInfo.keyId holds the ID of the guild authority and the keyInfo.PublicKey is the public key of the guild authority.
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
            if (level != p.level) {
                return false;
            }

            if (type != p.type) {
                return false;
            }

            if (type == PEER_GUILD) {
                if (guildId != p.GetGuildId()) {
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

      private:
        /**
         * Assignment operator is private
         */
        Peer& operator=(const Peer& other);

        /**
         * Copy constructor is private
         */
        Peer(const Peer& other);

        PeerAuthLevel level;
        PeerType type;
        qcc::GUID128 guildId;
        qcc::KeyInfoECC* keyInfo;
    };

    /**
     * Class to allow the application to specify a permission term
     */

    class Term {

      public:

        /**
         * Enumeration for the different types of field id
         */
        typedef enum {
            TAG_PEERS = 1,            ///< tag for the peers field
            TAG_RULES = 2           ///< tag for the rules field
        } TagType;

        /**
         * Constructor
         *
         */
        Term() : peersSize(0), peers(NULL), rulesSize(0), rules(NULL)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Term()
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

        bool operator==(const Term& t) const
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

      private:

        /**
         * Assignment operator is private
         */
        Term& operator=(const Term& other);

        /**
         * Copy constructor is private
         */
        Term(const Term& other);

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
     * Enumeration for the different types of field id
     */
    typedef enum {
        TAG_ADMINS = 1,     ///< tag for the admins field
        TAG_TERMS = 2       ///< tag for the terms field
    } TagType;


    /**
     * Constructor
     *
     */
    PermissionPolicy() : version(1), serialNum(0), adminsSize(0), admins(NULL), termsSize(0), terms(NULL)
    {
    }

    /**
     * virtual destructor
     */
    virtual ~PermissionPolicy()
    {
        delete [] admins;
        delete [] terms;
    }


    void SetVersion(uint8_t version)
    {
        this->version = version;
    }
    const uint8_t GetVersion() const
    {
        return version;
    }

    void SetSerialNum(uint32_t serialNum)
    {
        this->serialNum = serialNum;
    }
    const uint32_t GetSerialNum() const
    {
        return serialNum;
    }

    /**
     * Set the array of admins
     * @param count the number of admins
     * @param admins the array of admins. The array must be new'd by the caller. It will be deleted byt this object.
     */
    void SetAdmins(size_t count, Peer* admins)
    {
        this->admins = admins;
        adminsSize = count;
    }

    const size_t GetAdminsSize() const
    {
        return adminsSize;
    }

    const Peer* GetAdmins() const
    {
        return admins;
    }

    /**
     * Set the array of permission terms
     * @param count the number of permission terms
     * @param provider the array of permission terms. The array must be new'd by the caller. It will be deleted by this object.
     */
    void SetTerms(size_t count, Term* terms)
    {
        termsSize = count;
        this->terms = terms;
    }

    const size_t GetTermsSize() const
    {
        return termsSize;
    }

    const Term* GetTerms() const
    {
        return terms;
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
    QStatus Export(MsgArg& msgArg);

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
     * @param version     the authorization data spec version
     * @param msgArg      the message arg
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    QStatus Import(uint8_t version, const MsgArg& msgArg);

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

  private:

    /**
     * Assignment operator is private
     */
    PermissionPolicy& operator=(const PermissionPolicy& other);

    /**
     * Copy constructor is private
     */
    PermissionPolicy(const PermissionPolicy& other);

    uint8_t version;
    uint32_t serialNum;
    size_t adminsSize;
    Peer* admins;
    size_t termsSize;
    Term* terms;
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
