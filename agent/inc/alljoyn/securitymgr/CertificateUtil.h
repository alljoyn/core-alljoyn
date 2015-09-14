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

#ifndef ALLJOYN_SECMGR_CERTIFICATEUTIL_H_
#define ALLJOYN_SECMGR_CERTIFICATEUTIL_H_

#include <qcc/CertificateECC.h>

#include <alljoyn/Status.h>

#include "Application.h"
#include "GroupInfo.h"
#include "IdentityInfo.h"

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
/**
 * @brief CertificateUtil, a utility class to help in certificate generation.
 */
class CertificateUtil {
  public:
    /**
     * @brief Generates a MembershipCertificate.
     *
     * @param[in] app              The application for which a membership
     *                             certificate should be generated.
     * @param[in] groupInfo        The security group to which the application
     *                             should be added.
     * @param[in] validityPeriod   The validity period for the certificate in
     *                             seconds from now. The validity period starts
     *                             one hour before the current time to allow for
     *                             some clock drift.
     * @param[out] membershipCert  The generated membership certificate.
     *
     * @return ER_OK  on success.
     * @return others on failure.
     */
    static QStatus ToMembershipCertificate(const Application& app,
                                           const GroupInfo& groupInfo,
                                           const uint64_t validityPeriod,
                                           MembershipCertificate& membershipCert);

    /**
     * @brief Generates an IdentityCertificate.
     *
     * @param[in] app              The application for which an identity
     *                             certificate should be generated.
     * @param[in] identityInfo     The identity that should be encoded in the
     *                             certificate.
     * @param[in] validityPeriod   The validity period for the certificate in
     *                             seconds from now. The validity period starts
     *                             one hour before the current time to allow for
     *                             some clock drift.
     * @param[out] identityCert    The generated identity certificate.
     *
     * @return ER_OK  on success.
     * @return others on failure.
     */
    static QStatus ToIdentityCertificate(const Application& app,
                                         const IdentityInfo& identityInfo,
                                         const uint64_t validityPeriod,
                                         IdentityCertificate& identityCert);

    /**
     * @brief Set the validity period of a certificate.
     *
     * @param[in] validityPeriod  The validity period for the certificate in
     *                            seconds from now. The validity period starts
     *                            one hour before the current time to allow for
     *                            some clock drift.
     * @param[in,out] cert        The certificate for which the validity period
     *                            is set.
     *
     */
    static void SetValityPeriod(const uint64_t validityPeriod,
                                CertificateX509& cert);

  private:
    /**
     * @brief Set the subject name of a certificate based on an application.
     *
     * @param[in,out] cert   The certificate for which the subject name is set.
     * @param[in] app        The application used to set the subject name.
     *
     * @return ER_OK  on success.
     * @return others on failure.
     */

    static QStatus SetSubjectName(CertificateX509& cert,
                                  const Application& app);

    CertificateUtil() { }

    ~CertificateUtil() { }
};
}
}

#endif /* ALLJOYN_SECMGR_CERTIFICATEUTIL_H_ */
