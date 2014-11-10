#ifndef _ALLJOYN_PERMISSION_POLICY_H
#define _ALLJOYN_PERMISSION_POLICY_H
/**
 * @file
 * This file defines the Permission Policy classes that provide the interface to
 * parse the authorization data
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>


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

            const qcc::String GetMemberName()
            {
                return memberName;
            }

            void SetMemberType(MemberType memberType)
            {
                this->memberType = memberType;
            }

            const MemberType GetMemberType()
            {
                return memberType;
            }

            void SetActionMask(uint8_t actionMask)
            {
                this->actionMask = actionMask;
            }

            const uint8_t GetActionMask()
            {
                return actionMask;
            }

            void SetMutualAuth(bool flag)
            {
                mutualAuth = flag;
            }

            const bool GetMutualAuth()
            {
                return mutualAuth;
            }

            qcc::String ToString();

          private:
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

        const qcc::String GetObjPath()
        {
            return objPath;
        }

        void SetInterfaceName(qcc::String interfaceName)
        {
            this->interfaceName = interfaceName;
        }

        const qcc::String GetInterfaceName()
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
        const Member* GetMembers()
        {
            return members;
        }

        const size_t GetMembersSize()
        {
            return membersSize;
        }

        qcc::String ToString();

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
            PEER_ANY = 0,   ///< any peer
            PEER_GUID = 1,  ///< peer GUID
            PEER_PSK = 2,   ///< peer using PSK
            PEER_DSA = 3,   ///< peer using DSA
            PEER_GUILD = 4  ///< peer is a guild
        } PeerType;

        /**
         * Enumeration for the different types of field id
         */
        typedef enum {
            TAG_ID = 1,               ///< tag for ID field
            TAG_GUILD_AUTHORITY = 2,  ///< tag for guild authority field
            TAG_PSK = 3               ///< tag for PSK field
        } TagType;

        /**
         * Constructor
         *
         */
        Peer() : type(PEER_ANY), ID(NULL), IDLen(0), guildAuthority(NULL), guildAuthorityLen(0), psk(NULL), pskLen(0)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Peer()
        {
            delete [] ID;
            delete [] guildAuthority;
            delete [] psk;
        }

        void SetType(PeerType peerType)
        {
            type = peerType;
        }
        const PeerType GetType()
        {
            return type;
        }

        /**
         * Set the ID field.
         * @param id the id to be copied
         * @param len the number of bytes in the id field
         */
        void SetID(const uint8_t* id, size_t len)
        {
            delete [] ID;
            ID = NULL;
            IDLen = len;
            if (len == 0) {
                return;
            }
            ID = new uint8_t[len];
            memcpy(ID, id, len);
        }

        const size_t GetIDLen()
        {
            return IDLen;
        }

        const uint8_t* GetID()
        {
            return ID;
        }

        /**
         * Set the Guild authority field.
         * @param guildAuthority the guild authority to be copied
         * @param len the number of bytes in the guild authority field
         */
        void SetGuildAuthority(const uint8_t* guildAuthority, size_t len)
        {
            delete [] this->guildAuthority;
            this->guildAuthority = NULL;
            guildAuthorityLen = len;
            if (len == 0) {
                return;
            }
            this->guildAuthority = new uint8_t[len];
            memcpy(this->guildAuthority, guildAuthority, len);
        }

        const size_t GetGuildAuthorityLen()
        {
            return guildAuthorityLen;
        }

        const uint8_t* GetGuildAuthority()
        {
            return guildAuthority;
        }

        /**
         * Set the PSK field.
         * @param psk the PSK to be copied
         * @param len the number of bytes in the PSK field
         */
        void SetPsk(const uint8_t* psk, size_t len)
        {
            delete [] this->psk;
            this->psk = NULL;
            pskLen = len;
            if (len == 0) {
                return;
            }
            this->psk = new uint8_t[len];
            memcpy(this->psk, psk, len);
        }

        const size_t GetPskLen()
        {
            return pskLen;
        }

        const uint8_t* GetPsk()
        {
            return psk;
        }

        qcc::String ToString();

      private:
        PeerType type;
        uint8_t* ID;
        size_t IDLen;
        uint8_t* guildAuthority;
        size_t guildAuthorityLen;
        uint8_t* psk;
        size_t pskLen;
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

        const size_t GetPeersSize() {
            return peersSize;
        }

        const Peer* GetPeers()
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

        const size_t GetRulesSize() {
            return rulesSize;
        }

        const Rule* GetRules()
        {
            return rules;
        }

        qcc::String ToString();

      private:
        size_t peersSize;
        Peer* peers;
        size_t rulesSize;
        Rule* rules;
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
    const uint8_t GetVersion()
    {
        return version;
    }

    void SetSerialNum(uint32_t serialNum)
    {
        this->serialNum = serialNum;
    }
    const uint32_t GetSerialNum()
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

    const size_t GetAdminsSize() {
        return adminsSize;
    }

    const Peer* GetAdmins()
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

    const size_t GetTermsSize() {
        return termsSize;
    }

    const Term* GetTerms()
    {
        return terms;
    }

    qcc::String ToString();

    /**
     * Serialize the permission policy to a byte array.
     * @param bus the bus attachment
     * @param[out] buf the newly allocated byte array holding the serialized data. The caller must delete[] this buffer after use.
     * @param[out] size the variable holding the size of the allocated byte array
     * @return
     *      - #ER_OK if export was successful.
     *      - error code if fail
     */
    QStatus Export(BusAttachment& bus, uint8_t** buf, size_t* size);

    /**
     * Deserialize the permission policy from a byte array.
     * @param bus the bus attachment
     * @param buf the byte array holding the serialized data. The serialized data must be generated by the Export call.
     * @param size the size of the byte array
     * @return
     *      - #ER_OK if import was successful.
     *      - error code if fail
     */
    QStatus Import(BusAttachment& bus, const uint8_t* buf, size_t size);

    /**
     * Export the Policy to a MsgArg object.
     * @param[out] msgArg the resulting message arg
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    QStatus Export(MsgArg& msgArg);

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
     * @param bus the bus attachment
     * @param hashUtil the hash utility to use
     * @param[out] digest the buffer to hold the output digest.  It must be new[] by the caller and must have enough space to hold the digest based on the hash utility.
     * @return
     *      - #ER_OK if digest was successful.
     *      - error code if fail
     */
    QStatus Digest(BusAttachment& bus, qcc::Crypto_Hash& hashUtil, uint8_t* digest);

  private:
    QStatus Export(Message& msg);

    uint8_t version;
    uint32_t serialNum;
    size_t adminsSize;
    Peer* admins;
    size_t termsSize;
    Term* terms;

};

}
#endif
