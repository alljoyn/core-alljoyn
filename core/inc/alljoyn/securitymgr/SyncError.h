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

#ifndef SYNCERROR_H_
#define SYNCERROR_H_

#include <qcc/CertificateECC.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/Status.h>
#include <alljoyn/securitymgr/ApplicationInfo.h>

namespace ajn {
namespace securitymgr {
/*
 * Represents the type of a SyncError.
 */
typedef enum {
    SYNC_ER_UNKNOWN = 0, /**< Unknown */
    SYNC_ER_STORAGE = 1,     /**< Storage error */
    SYNC_ER_RESET = 2, /**< Reset */
    SYNC_ER_IDENTITY = 3, /**< UpdateIdentity */
    SYNC_ER_MEMBERSHIP = 4, /**< InstallMembership */
    SYNC_ER_POLICY = 5 /**< UpdatePolicy */
} SyncErrorType;

/**
 * Represents an error when synchronizing a remote application with its
 * security configuration as persisted by the security manager.
 */
class SyncError {
  public:
    SyncError(ApplicationInfo appInfo, QStatus status, SyncErrorType type) :
        appInfo(appInfo), status(status), type(type),
        idCert(NULL), membCert(NULL), policy(NULL)
    {
    }

    SyncError(ApplicationInfo appInfo, QStatus status, const qcc::IdentityCertificate& ic) :
        appInfo(appInfo), status(status), type(SYNC_ER_IDENTITY),
        membCert(NULL), policy(NULL)
    {
        idCert = new qcc::IdentityCertificate(ic);
    }

    SyncError(ApplicationInfo appInfo, QStatus status, const qcc::MembershipCertificate& mc) :
        appInfo(appInfo), status(status), type(SYNC_ER_MEMBERSHIP),
        idCert(NULL), policy(NULL)
    {
        membCert = new qcc::MembershipCertificate(mc);
    }

    SyncError(ApplicationInfo appInfo, QStatus status, const PermissionPolicy& p) :
        appInfo(appInfo), status(status), type(SYNC_ER_POLICY),
        idCert(NULL), membCert(NULL)
    {
        policy = new PermissionPolicy(p);
    }

    SyncError(const SyncError& other) :
        appInfo(other.appInfo), status(other.status), type(other.type),
        idCert(NULL), membCert(NULL), policy(NULL)
    {
        if (other.idCert) {
            idCert = new qcc::IdentityCertificate(*(other.idCert));
        }
        if (other.membCert) {
            membCert = new qcc::MembershipCertificate(*(other.membCert));
        }
        if (other.policy) {
            policy = new PermissionPolicy(*(other.policy));
        }
    }

    ~SyncError()
    {
        delete idCert;
        delete membCert;
        delete policy;
    }

    const qcc::IdentityCertificate* GetIdentityCertificate() const
    {
        return idCert;
    }

    const qcc::MembershipCertificate* GetMembershipCertificate() const
    {
        return membCert;
    }

    const PermissionPolicy* GetPolicy() const
    {
        return policy;
    }

    /*@{*/
    ApplicationInfo appInfo; /**< the application that could not be synchronized */
    QStatus status; /**< the status as returned by the application */
    SyncErrorType type; /**< the type of the synchronization error */

  private:
    qcc::IdentityCertificate* idCert; /**< identity certificate */
    qcc::MembershipCertificate* membCert; /**< membership certificate */
    PermissionPolicy* policy; /**< policy that could not be installed */
    /*@}*/
};
}
}

#endif /* SYNCERROR_H_ */
