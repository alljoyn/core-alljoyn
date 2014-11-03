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

#ifndef STORAGE_H_
#define STORAGE_H_

#include <AppGuildInfo.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Certificate.h>
#include <qcc/CryptoECC.h>

#include <vector>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
/**
 * \class Storage
 * \brief An abstract class that is meant to define the interfacing with a persistent storage means.
 *
 *  Applications and Guilds can be managed persistently through this API.
 *
 */
class Storage {
  protected:
    QStatus status;

  public:

    Storage() :
        status(ER_OK) { }

    QStatus GetStatus() const
    {
        return status;
    }

    /**
     * \brief Store the information pertaining to a managed application.
     *
     * \param[in] managedApplicationInfo the application info
     * \param[in] update a boolean to allow/deny application overwriting
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreApplication(const ManagedApplicationInfo& managedApplicationInfo,
                                     const bool update = false) = 0;

    /**
     * \brief Remove the information pertaining to a previously managed application.
     *
     * \param[in] managedApplicationInfo the application info, ONLY the publicKey is mandatory here
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveApplication(const ManagedApplicationInfo& managedApplicationInfo) = 0;

    /**
     * \brief Retrieve a list of managed applications.
     *
     * \param[in,out] managedApplications a vector of managed applications
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetManagedApplications(std::vector<ManagedApplicationInfo>& managedApplications) const = 0;

    /**
     * \brief Get a managed application if it already exists.
     *
     *
     * \param[in] managedApplicationInfo the managed application info to be filled in. Only the publicKey field is required
     * \param[in, out] managed a boolean stating whether the application is managed or not
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetManagedApplication(ManagedApplicationInfo& managedApplicationInfo) const = 0;

    /**
     * \brief Store a certificate with the option to update it, if it is already present.
     *
     * \param[in] certificate a certificate of some type
     * \param[in] update a boolean to allow/deny certificate overwriting
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreCertificate(const qcc::Certificate& certificate,
                                     bool update = false) = 0;

    /**
     * \brief Store a given data that is assoiciated with a given certificate.
     *
     * \param[in] certificate a certificate of some type that is aware of the associated data size
     * \param[in] data associated data, e.g., authorization data, vcard
     * \param[in] update a boolean to allow/deny overwriting
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreAssociatedData(const qcc::Certificate& certificate,
                                        const qcc::String& data,
                                        bool update = false) = 0;

    /**
     * \brief Remove a given certificate.
     *
     * \param[in] certificate a certificate of some type
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveCertificate(qcc::Certificate& certificate) = 0;

    /**
     * \brief Remove a given data that is associated with a given certificate.
     *
     * \param[in] certificate a certificate of some type that is aware of the associated data ID
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveAssociatedData(const qcc::Certificate& certificate) = 0;

    /**
     * \brief Retrieve a certificate of a certain type.
     *
     * \param[in, out] certificate a certificate of some type
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetCertificate(qcc::Certificate& certificate) = 0;

    /**
     * \brief Retrieve a given data that is associated with a given certificate.
     *
     * \param[in] certificate a certificate of some type that is aware of the associated data size
     * \param[in, out] data the associated data
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetAssociatedData(const qcc::Certificate& certificate,
                                      qcc::String& data) const = 0;

    /**
     * \brief Retrieve a new serial number to be assigned to a certificate
     *
     * \param[out] a string to contain the new serial number.
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetNewSerialNumber(qcc::String& serialNumber) const = 0;

    /**
     * \brief Add a Guild info to be persistently stored.
     *
     * \param[in] guildInfo the info of a given guild that needs to be stored
     * \param[in] update a flag to specify whether guild info should be updated if the guild already exists
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreGuild(const GuildInfo& guildInfo,
                               const bool update = false) = 0;

    /**
     * \brief Remove the stored information pertaining to a given Guild.
     *
     * \param[in] guildId the identifier of a given Guild
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveGuild(const qcc::GUID128& guildId) = 0;

    /**
     * \brief Get the info stored for a given Guild.
     *
     * \param[in, out] guildInfo info of a given Guild. Only the GUID is mandatory for input
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetGuild(GuildInfo& guildInfo) const = 0;

    /**
     * \brief Get the info of all managed Guilds.
     *
     * \param[in, out] guildsInfo all info of currently managed Guilds
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const = 0;

    /**
     * \brief Add an Identity info to be persistently stored.
     *
     * \param[in] identityInfo the info of a given identity that needs to be stored
     * \param[in] update a flag to specify whether identity info should be updated if the identity already exists
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreIdentity(const IdentityInfo& identityInfo,
                                  const bool update = false) = 0;

    /**
     * \brief Remove the stored information pertaining to a given Identity.
     *
     * \param[in] idId the identifier of a given identity
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveIdentity(const qcc::GUID128& idId) = 0;

    /**
     * \brief Get the info stored for a Identity Guild.
     *
     * \param[in, out] idInfo info of a given Identity. Only the GUID is mandatory for input
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetIdentity(IdentityInfo& idInfo) const = 0;

    /**
     * \brief Get the info of all managed Identities.
     *
     * \param[in, out] identityInfos all info of currently managed Identities
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetManagedIdentities(std::vector<IdentityInfo>& identityInfos) const = 0;

    /**
     * \brief Reset the storage and delete the database.
     */
    virtual void Reset() = 0;

    virtual ~Storage()
    {
    }
};
}
}
#undef QCC_MODULE
#endif /* STORAGE_H_ */
