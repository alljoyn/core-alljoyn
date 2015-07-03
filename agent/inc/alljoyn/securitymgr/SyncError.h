/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#ifndef ALLJOYN_SECMGR_SYNCERROR_H_
#define ALLJOYN_SECMGR_SYNCERROR_H_

#include <qcc/CertificateECC.h>

#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/Status.h>

#include "Application.h"

using namespace qcc;

namespace ajn {
namespace securitymgr {
/*
 * @brief Represent the type of a SyncError.
 */
typedef enum {
    SYNC_ER_UNKNOWN, /**< Unknown */
    SYNC_ER_STORAGE, /**< Storage error */
    SYNC_ER_REMOTE, /**< Remote error */
    SYNC_ER_RESET, /**< Reset */
    SYNC_ER_IDENTITY, /**< UpdateIdentity */
    SYNC_ER_MEMBERSHIP, /**< InstallMembership */
    SYNC_ER_POLICY /**< UpdatePolicy */
} SyncErrorType;

/**
 * Represent an error when synchronizing a remote application with its
 * security configuration as persisted by the security agent.
 */
class SyncError {
  public:
    /**
     * @brief Generic constructor for SyncError.
     *
     * To construct a SyncError of type SYNC_ER_IDENTITY, SYNC_ER_MEMBERSHIP,
     * or SYNC_ER_POLICY, the more specific constructors should be used.
     *
     * @param[in] app     The application that could not be synchronized
     * @param[in] status  The status as returned by the application
     * @param[in] type    The type of the SyncError
     */
    SyncError(OnlineApplication app, QStatus status, SyncErrorType type) :
        app(app), status(status), type(type),
        idCert(nullptr), membCert(nullptr), policy(nullptr)
    {
    }

    /**
     * @brief Constructor for SyncError of type SYNC_ER_IDENTITY.
     *
     * @param[in] app     The application that could not be synchronized.
     * @param[in] status  The status as returned by the application.
     * @param[in] ic      The identity certificate that could not be
     *                    synchronized.
     */
    SyncError(OnlineApplication app, QStatus status, const IdentityCertificate& ic) :
        app(app), status(status), type(SYNC_ER_IDENTITY),
        membCert(nullptr), policy(nullptr)
    {
        idCert = new IdentityCertificate(ic);
    }

    /**
     * @brief Constructor for SyncError of type SYNC_ER_MEMBERSHIP.
     *
     * @param[in] app     The application that could not be synchronized.
     * @param[in] status  The status as returned by the application.
     * @param[in] mc      The membership certificate that could not be
     *                    synchronized.
     */
    SyncError(OnlineApplication app, QStatus status, const MembershipCertificate& mc) :
        app(app), status(status), type(SYNC_ER_MEMBERSHIP),
        idCert(nullptr), policy(nullptr)
    {
        membCert = new MembershipCertificate(mc);
    }

    /**
     * @brief Constructor for SyncError of type SYNC_ER_POLICY.
     *
     * @param[in] app     The application that could not be synchronized.
     * @param[in] status  The status as returned by the application.
     * @param[in] p       The policy that could not be synchronized.
     */
    SyncError(OnlineApplication app, QStatus status, const PermissionPolicy& p) :
        app(app), status(status), type(SYNC_ER_POLICY),
        idCert(nullptr), membCert(nullptr)
    {
        policy = new PermissionPolicy(p);
    }

    /**
     * @brief Copy constructor for SyncError.
     *
     * @param[in] other   The error to copy.
     */
    SyncError(const SyncError& other) :
        app(other.app), status(other.status), type(other.type),
        idCert(nullptr), membCert(nullptr), policy(nullptr)
    {
        if (other.idCert) {
            idCert = new IdentityCertificate(*(other.idCert));
        }
        if (other.membCert) {
            membCert = new MembershipCertificate(*(other.membCert));
        }
        if (other.policy) {
            policy = new PermissionPolicy(*(other.policy));
        }
    }

    /**
     * @brief Destructor for SyncError.
     */
    ~SyncError()
    {
        delete idCert;
        delete membCert;
        delete policy;
    }

    /**
     * @brief Get the IdentityCertificate that caused this SyncError.
     *
     * @return a pointer To the IdentityCertificate that caused this error,
     *                   if type is equal to SYNC_ER_IDENTITY.
     * @return nullptr   If type is different from SYNC_ER_IDENTITY.
     */
    const IdentityCertificate* GetIdentityCertificate() const
    {
        return idCert;
    }

    /**
     * @brief Get the MembershipCertificate that caused this SyncError.
     *
     * @return a pointer To the IdentityCertificate that caused this error,
     *                   if type is equal to SYNC_ER_MEMBERSHIP.
     * @return nullptr   If type is different from SYNC_ER_MEMBERSHIP.
     */
    const MembershipCertificate* GetMembershipCertificate() const
    {
        return membCert;
    }

    /**
     * @brief Get the Policy that caused this SyncError.
     *
     * @return a pointer To the PermissionPolicy that caused this error,
     *                   if type is equal to SYNC_ER_POLICY.
     * @return nullptr   If type is different from SYNC_ER_POLICY.
     */
    const PermissionPolicy* GetPolicy() const
    {
        return policy;
    }

    /*@{*/
    OnlineApplication app; /**< The application that could not be synchronized. */
    QStatus status; /**< The status as returned by the application. */
    SyncErrorType type; /**< The type of the synchronization error. */

  private:
    IdentityCertificate* idCert; /**< Identity certificate. */
    MembershipCertificate* membCert; /**< Membership certificate. */
    PermissionPolicy* policy; /**< Policy that could not be installed. */
    /*@}*/
};
}
}

#endif /* ALLJOYN_SECMGR_SYNCERROR_H_ */
