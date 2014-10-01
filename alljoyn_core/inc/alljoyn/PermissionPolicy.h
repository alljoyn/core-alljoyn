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
             * Enumeration for the different types of field id
             */
            typedef enum {
                TAG_MEMBER_NAME = 1,  ///< tag for member field
                TAG_MEMBER_TYPE = 2,  ///< tag for member type field
                TAG_READ_ONLY = 3,    ///< tag for read-only field
                TAG_MUTUAL_AUTH = 4   ///< tag for mutual auth field
            } TagType;

            /**
             * Constructor
             *
             */
            Member() : memberName(), memberType(NOT_SPECIFIED), readOnly(false), mutualAuth(false)
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

            void SetReadOnly(bool readOnly)
            {
                this->readOnly = readOnly;
            }

            const bool GetReadOnly()
            {
                return readOnly;
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
            bool readOnly;
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
        Rule() : objPath(), interfaceName(), members(NULL), numOfMembers(0)
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
         * @param members  The array of member fields.  The memory for the array must be new'd and
         *          will be deleted by this object.
         */
        void SetMembers(size_t count, Member* members)
        {
            this->members = members;
            numOfMembers = count;
        }

        /**
         * Get the array of inferface members.
         * @return the array of interface members.
         */
        const Member* GetMembers()
        {
            return members;
        }

        const size_t GetNumOfMembers()
        {
            return numOfMembers;
        }

        qcc::String ToString();

      private:
        qcc::String objPath;
        qcc::String interfaceName;
        Member* members;
        size_t numOfMembers;
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
     * Class to allow the application to specify an access control list
     */

    class ACL {

      public:

        /**
         * Enumeration for the different types of field id
         */
        typedef enum {
            TAG_ALLOW = 2,            ///< tag for allow rules field
            TAG_ALLOWALLEXCEPT = 3    ///< tag for allowAllExcept field
        } TagType;


        /**
         * Constructor
         *
         */
        ACL() : allowRuleSize(0), allowRules(NULL), allowAllExceptRuleSize(0), allowAllExceptRules(NULL)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~ACL()
        {
            delete [] allowRules;
            delete [] allowAllExceptRules;
        }


        /**
         * Set the list of allow rules.
         * @param count the number of rules
         * @param allow the list of allow rules. The list must be new'd by the caller.  It will be deleted by this object.
         */
        void SetAllowRules(size_t count, Rule* allowRules)
        {
            allowRuleSize = count;
            this->allowRules = allowRules;
        }

        const size_t GetAllowRuleSize() {
            return allowRuleSize;
        }

        const Rule* GetAllowRules()
        {
            return allowRules;
        }

        /**
         * Set the list of allow all except rules.
         * @param count the number of rules
         * @param exceptions the list of allow all except rules. The list must be new'd by the caller.  It will be deleted by this object.
         */
        void SetAllowAllExceptRules(size_t count, Rule* exceptions);

        const size_t GetAllowAllExceptRuleSize()
        {
            return allowAllExceptRuleSize;
        }

        const Rule* GetAllowAllExceptRules()
        {
            return allowAllExceptRules;
        }

        qcc::String ToString();

      private:
        size_t allowRuleSize;
        Rule* allowRules;
        size_t allowAllExceptRuleSize;
        Rule* allowAllExceptRules;
    };

    /**
     * Class to allow the application to specify a permission term
     */

    class Term : public ACL {

      public:

        /**
         * Enumeration for the different types of field id
         */
        typedef enum {
            TAG_PEER = 1,            ///< tag for the peer field
            TAG_ALLOW = 2,           ///< tag for the allow field
            TAG_ALLOWALLEXCEPT = 3   ///< tag for the allowAllExcept field
        } TagType;


        /**
         * Constructor
         *
         */
        Term() : ACL(), peerSize(0), peers(NULL)
        {
        }

        /**
         * virtual destructor
         */
        virtual ~Term()
        {
            delete [] peers;
        }

        /**
         * Set the list of peers
         * @param count the number of peers
         * @param peer the list of peers. The list must be new'd. It will be deleted by this object.
         */
        void SetPeers(size_t count, Peer* peers)
        {
            this->peers = peers;
            peerSize = count;
        }

        const size_t GetPeerSize() {
            return peerSize;
        }

        const Peer* GetPeers()
        {
            return peers;
        }

        qcc::String ToString();


      private:
        size_t peerSize;
        Peer* peers;
    };


    /**
     * Enumeration for the different types of field id
     */
    typedef enum {
        TAG_ADMIN = 1,      ///< tag for the admin field
        TAG_PROVIDER = 2,   ///< tag for the provider field
        TAG_CONSUMER = 3   ///< tag for the consumer field
    } TagType;


    /**
     * Constructor
     *
     */
    PermissionPolicy() : version(1), serialNum(0), adminSize(0), admins(NULL), providerSize(0), providers(NULL), consumerSize(0), consumers(NULL)
    {
    }

    /**
     * virtual destructor
     */
    virtual ~PermissionPolicy()
    {
        delete [] admins;
        delete [] providers;
        delete [] consumers;
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
     * Set the list of admins
     * @param count the number of admins
     * @param admins the list of admins. The list must be new'd by the caller. It will be deleted byt this object.
     */
    void SetAdmins(size_t count, Peer* admins)
    {
        this->admins = admins;
        adminSize = count;
    }

    const size_t GetAdminSize() {
        return adminSize;
    }

    const Peer* GetAdmins()
    {
        return admins;
    }

    /**
     * Set the list of permissions as a provider
     * @param count the number of permission terms
     * @param provider the list of permission terms as a provider. The list must be new'd by the caller. It will be deleted by this object.
     */
    void SetProviders(size_t count, Term* providers)
    {
        providerSize = count;
        this->providers = providers;
    }

    const size_t GetProviderSize() {
        return providerSize;
    }

    const Term* GetProviders()
    {
        return providers;
    }

    /**
     * Set the list of permissions as a consumer
     * @param count the number of permission terms
     * @param provider the list of permission terms as a consumer. The list must be new'd by the caller. It will be deleted by this object.
     */
    void SetConsumers(size_t count, ACL* consumers)
    {
        consumerSize = count;
        this->consumers = consumers;
    }

    const size_t GetConsumerSize()
    {
        return consumerSize;
    }

    const ACL* GetConsumers()
    {
        return consumers;
    }

    qcc::String ToString();

    /**
     * Generate the message arg for the Policy object.
     * @param[out] msgArg the resulting message arg
     * @param policy the input policy
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    static QStatus GeneratePolicyArgs(MsgArg& msgArg, PermissionPolicy& policy);

    /**
     * Build the policy object from the message arg object
     * @param version     the authorization data spec version
     * @param msgArg         the message arg
     * @param[out] policy the input policy
     * @return
     *      - #ER_OK if creation was successful.
     *      - error code if fail
     */
    static QStatus BuildPolicyFromArgs(uint8_t version, const MsgArg& msgArg, PermissionPolicy& policy);

  private:
    uint8_t version;
    uint32_t serialNum;
    size_t adminSize;
    Peer* admins;
    size_t providerSize;
    Term* providers;
    size_t consumerSize;
    ACL* consumers;

};

}
#endif
