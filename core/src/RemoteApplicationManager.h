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

#ifndef REMOTEAPPLICATIONMANAGER_H_
#define REMOTEAPPLICATIONMANAGER_H_

#include <alljoyn/securitymgr/ApplicationInfo.h>
#include <alljoyn/securitymgr/cert/X509Certificate.h>

#include "ProxyObjectManager.h"

#include <qcc/Certificate.h>
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

    QStatus InstallMembership(const ApplicationInfo& app,
                              qcc::X509MemberShipCertificate& cert,
                              PermissionPolicy& authData,
                              const qcc::GUID128& rotGuid);

    QStatus InstallIdentityCertificate(const ApplicationInfo& app,
                                       qcc::X509IdentityCertificate& cert);

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
                             const qcc::GUID128& GuidId);
};
}
}
#endif /* REMOTEAPPLICATIONMANAGER_H_ */
