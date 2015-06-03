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

#include "RemoteApplicationManager.h"

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
QStatus RemoteApplicationManager::Claim(const ApplicationInfo& app,
                                        qcc::KeyInfoNISTP256& certificateAuthority,
                                        qcc::GUID128& adminGroupId,
                                        qcc::KeyInfoNISTP256& adminGroup,
                                        qcc::IdentityCertificate* identityCertChain,
                                        size_t identityCertChainSize,
                                        PermissionPolicy::Rule* manifest,
                                        size_t manifestSize)
{
    QStatus status;
    status = proxyObjectManager->Claim(app, certificateAuthority, adminGroupId,
                                       adminGroup, identityCertChain, identityCertChainSize, manifest,
                                       manifestSize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to claim"));
    }
    return status;
}

QStatus RemoteApplicationManager::InstallMembership(const ApplicationInfo& app,
                                                    qcc::MembershipCertificate& cert)
{
    QStatus status = ER_FAIL;
    qcc::String errorMsg;
    do {
        ajn::MsgArg inputs[1];
        qcc::String pem((const char*)cert.GetEncoded(), cert.GetEncodedLen());
        inputs[0].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, pem.length(),
                      pem.data());
        ajn::MsgArg arg("a(yay)", 1, inputs);
        Message replyMsg(*ba);
        status = proxyObjectManager->MethodCall(app, "InstallMembership", &arg, 1, replyMsg);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to install membership certificate"));
            break;
        }
    } while (0);
    return status;
}

QStatus RemoteApplicationManager::InstallIdentity(const ApplicationInfo& app,
                                                  qcc::IdentityCertificate* certChain,
                                                  size_t certChainSize,
                                                  const PermissionPolicy::Rule* manifest,
                                                  size_t manifestSize)
{
    QStatus status = ER_FAIL;

    status = proxyObjectManager->InstallIdentity(app, certChain, certChainSize,
                                                 manifest, manifestSize);

    return status;
}

QStatus RemoteApplicationManager::InstallPolicy(const ApplicationInfo& app,
                                                PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;

    qcc::String errorMsg;
    MsgArg msgArg;
    Message replyMsg(*ba);

    status = policy.Export(msgArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to export policy."));
        return status;
    }

    status = proxyObjectManager->MethodCall(app, "InstallPolicy", &msgArg, 1, replyMsg);

    return status;
}

QStatus RemoteApplicationManager::Reset(const ApplicationInfo& app)
{
    QStatus status = ER_FAIL;

    Message replyMsg(*ba);
    status = proxyObjectManager->MethodCall(app, "Reset", NULL, 0, replyMsg);
    // errors logged in MethodCall

    return status;
}

QStatus RemoteApplicationManager::GetIdentity(const ApplicationInfo& app,
                                              qcc::IdentityCertificate& idCert)
{
    QStatus status = ER_FAIL;

    qcc::IdentityCertificate* certChain;
    size_t certChainSize;

    status = proxyObjectManager->GetIdentity(app, &certChain, &certChainSize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetIdentity"));
        return status;
    }

    do {
        if (certChainSize != 1) {
            status = ER_FAIL;
            QCC_LogError(status, ("Identity certificate chain longer than expected"));
            break;
        }

        idCert = certChain[0];
    } while (0);

    delete[] certChain;
    certChainSize = 0;

    return status;
}

QStatus RemoteApplicationManager::GetPolicy(const ApplicationInfo& app, PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;
    Message replyMsg(*ba);
    uint8_t version;
    MsgArg* variant;

    status = proxyObjectManager->MethodCall(app, "GetPolicy", NULL, 0,
                                            replyMsg);

    if (ER_OK != status) {
        // errors logged in MethodCall
        return status;
    }

    replyMsg->GetArg(0)->Get("(yv)", &version, &variant);

    if (ER_OK != (status = policy.Import(version, *variant))) {
        QCC_LogError(status, ("Could not build policy of remote application."));
    }

    return status;
}

QStatus RemoteApplicationManager::GetManifest(const ApplicationInfo& app,
                                              PermissionPolicy::Rule** manifestRules, size_t* manifestRulesCount)
{
    QStatus status = ER_FAIL;

    if ((NULL == manifestRules) || (NULL == manifestRulesCount)) {
        QCC_LogError(status, ("Null argument"));
        return status;
    }

    QCC_DbgPrintf(("Retrieving manifest of remote app..."));
    Message reply(*ba);
    status = proxyObjectManager->MethodCall(app, "GetManifest", NULL, 0, reply);

    uint8_t type;
    MsgArg* variant;
    status = reply->GetArg(0)->Get("(yv)", &type, &variant);
    if (ER_OK != status) {
        return status;
    }

    return PermissionPolicy::ParseRules(*variant, manifestRules, manifestRulesCount);
}

QStatus RemoteApplicationManager::RemoveMembership(const ApplicationInfo& app,
                                                   const qcc::String& serialNum,
                                                   const qcc::String& issuerId)
{
    QStatus status = ER_FAIL;

    if (serialNum.empty()) {
        QCC_LogError(status, ("Empty certificate serial number"));
        return status;
    }

    MsgArg args[2];
    Message replyMsg(*ba);
    const char* serial = serialNum.c_str();
    args[0].Set("s", serial);
    args[1].Set("ay", issuerId.size(), (const uint8_t*)issuerId.data());
    QCC_DbgPrintf(("Removing membership certificate with serial number %s", serial));
    status = proxyObjectManager->MethodCall(app, "RemoveMembership", args, 2, replyMsg);

    return status;
}
}
}
