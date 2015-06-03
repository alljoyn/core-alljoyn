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

#ifndef REMOTEAPPLICATIONMANAGER_H_
#define REMOTEAPPLICATIONMANAGER_H_

#include <alljoyn/securitymgr/ApplicationInfo.h>

#include "ProxyObjectManager.h"

#include <qcc/CertificateECC.h>
#include <alljoyn/PermissionPolicy.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>

namespace ajn {
namespace securitymgr {
class RemoteApplicationManager {
  private:
    ProxyObjectManager* proxyObjectManager;          // Not Owned
    ajn::BusAttachment* ba;                          // Not owned

  public:

    RemoteApplicationManager(ProxyObjectManager* proxyObjectManager,
                             ajn::BusAttachment* _ba) :
        proxyObjectManager(proxyObjectManager), ba(_ba)
    {
    }

    ~RemoteApplicationManager()
    {
    }

    bool Initialized(void)
    {
        return (NULL != proxyObjectManager);
    }

    QStatus Claim(const ApplicationInfo& app,
                  qcc::KeyInfoNISTP256& certificateAuthority,
                  qcc::GUID128& adminGroupId,
                  qcc::KeyInfoNISTP256& adminGroup,
                  qcc::IdentityCertificate* identityCertChain,
                  size_t identityCertChainSize,
                  PermissionPolicy::Rule* manifest,
                  size_t manifestSize);

    QStatus InstallMembership(const ApplicationInfo& app,
                              qcc::MembershipCertificate& cert);

    QStatus InstallIdentity(const ApplicationInfo& app,
                            qcc::IdentityCertificate* certChain,
                            size_t certChainSize,
                            const PermissionPolicy::Rule* manifest,
                            size_t manifestSize);

    QStatus InstallPolicy(const ApplicationInfo& app,
                          PermissionPolicy& policy);

    QStatus Reset(const ApplicationInfo& app);

    QStatus GetIdentity(const ApplicationInfo& app,
                        qcc::IdentityCertificate& idCert);

    QStatus GetPolicy(const ApplicationInfo& app,
                      PermissionPolicy& policy);

    QStatus GetManifest(const ApplicationInfo& app,
                        PermissionPolicy::Rule** manifestRules,
                        size_t* manifestRulesCount);

    QStatus RemoveMembership(const ApplicationInfo& app,
                             const qcc::String& serialNum,
                             const qcc::String& issuerKeyId);
};
}
}
#endif /* REMOTEAPPLICATIONMANAGER_H_ */
