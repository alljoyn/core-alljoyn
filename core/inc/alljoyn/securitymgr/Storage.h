/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/securitymgr/ManagedApplicationInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/GuildInfo.h>
#include <alljoyn/securitymgr/cert/X509Certificate.h>
#include <alljoyn/Status.h>

#include <qcc/String.h>
#include <qcc/Certificate.h>
#include <qcc/CryptoECC.h>

#include <vector>

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
     * \brief Remove the information pertaining to a previously managed application, including
     * its certificates.
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
     * \retval ER_END_OF_DATA if no data is found
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
     * \retval ER_END_OF_DATA if no data is found
     * \retval others on failure
     */
    virtual QStatus GetCertificate(qcc::Certificate& certificate) = 0;

    /*
     * \brief Retrieve all matching membership certificates based on optional
     * application key (subject) and/or guildId.
     *
     * \param[in]  certificate   input certificate that is used to match
     * \param[out] certificates  the matching certificates will be pushed to
     *                           the back of this vector
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetCertificates(const qcc::X509MemberShipCertificate& certificate,
                                    std::vector<qcc::X509MemberShipCertificate>& certificates) const = 0;

    /**
     * \brief Retrieve a given data that is associated with a given certificate.
     *
     * \param[in] certificate a certificate of some type that is aware of the associated data size
     * \param[in, out] data the associated data
     *
     * \retval ER_OK  on success
     * \retval ER_END_OF_DATA if no data is found
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
     * \retval ER_END_OF_DATA if no data is found
     * \retval others on failure
     */
    virtual QStatus GetNewSerialNumber(qcc::String& serialNumber) const = 0;

    /**
     * \brief Store a guild. If a guild with the same keys was stored before,
     * it will be updated.
     *
     * \param[in] guildInfo  the info of a guild that needs to be stored;
     *                       both authority and guid must be provided
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreGuild(const GuildInfo& guildInfo) = 0;

    /**
     * \brief Remove a guild from storage.
     *
     * \param[in] guildInfo  the info of a guild that needs to be removed;
     *                       both authority and guid must be provided
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveGuild(const GuildInfo& guildInfo) = 0;

    /**
     * \brief Get the stored info for a provided guild.
     *
     * \param[in, out] guildInfo  the info of a guild that should be retrieved;
     *                            both authority and guid must be provided
     *
     * \retval ER_OK  on success
     * \retval ER_END_OF_DATA if no data is found
     * \retval others on failure
     */
    virtual QStatus GetGuild(GuildInfo& guildInfo) const = 0;

    /**
     * \brief Get all stored guild information.
     *
     * \param[in, out] guildInfos  a vector to which any stored guild info
     *                             object will be pushed
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetGuilds(std::vector<GuildInfo>& guildInfos) const = 0;

    /**
     * \brief Store an identity. If an identity with the same keys was stored
     * before, it will be updated.
     *
     * \param[in] idInfo  the info of an identity that needs to be stored;
     *                    both authority and guid must be provided
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus StoreIdentity(const IdentityInfo& idInfo) = 0;

    /**
     * \brief Remove an identity from storage.
     *
     * \param[in] idInfo  the info of an identity that needs to be removed;
     *                    both authority and guid must be provided
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus RemoveIdentity(const IdentityInfo& idInfo) = 0;

    /**
     * \brief Get the stored info for a provided identity.
     *
     * \param[in, out] idInfo  the info of an identity that should be
     *                         retrieved; both authority and guid must be
     *                         provided
     *
     * \retval ER_OK  on success
     * \retval ER_END_OF_DATA if no data is found
     * \retval others on failure
     */
    virtual QStatus GetIdentity(IdentityInfo& idInfo) const = 0;

    /**
     * \brief Get all stored identity information.
     *
     * \param[in, out] idInfos  a vector to which any stored identity info
     *                          object will be pushed
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    virtual QStatus GetIdentities(std::vector<IdentityInfo>& idInfos) const = 0;

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
#endif /* STORAGE_H_ */
