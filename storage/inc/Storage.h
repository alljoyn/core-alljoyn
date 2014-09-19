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

#include <ClaimedApplicationInfo.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Certificate.h>
#include <qcc/CryptoECC.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
typedef enum {
    IDENTITY,
    MEMBERSHIP,
    POLICY,
    MEMBERSHIP_EQ,
    USER_EQ
}CertificateType;

/**
 * \class Storage
 * \brief An abstract class that is meant to define the interfacing with a persistent storage means.
 *
 *  Initial stage is to allow for tracking claimed applications.
 *  //TODO:: define GUILD management functionality
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

    virtual QStatus TrackClaimedApplication(const ClaimedApplicationInfo& claimedApplicationInfo) = 0;

    virtual QStatus UnTrackClaimedApplication(const ClaimedApplicationInfo& claimedApplicationInfo) = 0;

    virtual QStatus GetClaimedApplications(std::vector<ClaimedApplicationInfo>* claimedApplications) = 0;

    virtual QStatus ClaimedApplication(const qcc::ECCPublicKey& appECCPublicKey,
                                       bool* claimed) = 0;

    virtual QStatus StoreCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                                     const qcc::Certificate& certificate) = 0;

    virtual QStatus RemoveCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                                      const CertificateType certificateType) = 0;

    virtual qcc::Certificate GetCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                                            const CertificateType certificateType) = 0;

    virtual void Reset() = 0;

    virtual ~Storage()
    {
    }
};
}
}
#undef QCC_MODULE
#endif /* STORAGE_H_ */
