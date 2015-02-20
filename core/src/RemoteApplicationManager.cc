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

#include "RemoteApplicationManager.h"

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
QStatus RemoteApplicationManager::InstallMembership(const ApplicationInfo& app,
                                                    qcc::X509MemberShipCertificate& cert,
                                                    PermissionPolicy& authData, const qcc::GUID128& rotGuid)
{
    QStatus status = ER_FAIL;
    qcc::String errorMsg;
    do {
        ajn::MsgArg inputs[1];
        qcc::String pem = cert.GetDER();
        inputs[0].Set("(yay)", qcc::Certificate::ENCODING_X509_DER, pem.length(),
                      pem.data());
        ajn::MsgArg arg("a(yay)", 1, inputs);
        Message replyMsg(*ba);
        status = proxyObjectManager->MethodCall(app, "InstallMembership", &arg, 1, replyMsg);

        if (ER_OK != status) {
            break;
        }

        MsgArg args[3];
        args[0].Set("s", cert.GetSerialNumber().c_str());
        args[1].Set("ay", qcc::GUID128::SIZE, rotGuid.GetBytes());
        authData.Export(args[2]);
        Message replyMsgAuthData(*ba);
        status = proxyObjectManager->MethodCall(app, "InstallMembershipAuthData", args, 3, replyMsgAuthData);

        // TODO: RemoveMembership when ER_OK != status
    } while (0);
    return status;
}

QStatus RemoteApplicationManager::InstallIdentityCertificate(const ApplicationInfo& app,
                                                             qcc::X509IdentityCertificate& cert)
{
    QStatus status = ER_FAIL;

    qcc::String errorMsg;
    Message replyMsg(*ba);
    qcc::String pem = cert.GetDER();
    MsgArg arg("(yay)", qcc::Certificate::ENCODING_X509_DER_PEM, pem.size(), pem.data());

    status = proxyObjectManager->MethodCall(app, "InstallIdentity", &arg, 1, replyMsg);

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

QStatus RemoteApplicationManager::GetIdentity(const ApplicationInfo& app, qcc::IdentityCertificate& idCert)
{
    QStatus status = ER_FAIL;
    qcc::String errorMsg;
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;

    do {
        Message replyMsg(*ba);
        status = proxyObjectManager->MethodCall(app, "GetIdentity", NULL, 0, replyMsg);

        if (ER_OK != status) {
            break;
        }

        status = replyMsg->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to extract identity from reply"));
            break;
        }

        if (encoding == qcc::Certificate::ENCODING_X509_DER_PEM) {
            status = idCert.DecodeCertificatePEM(qcc::String((const char*)encoded, encodedLen));
        } else if (encoding == qcc::Certificate::ENCODING_X509_DER) {
            status = idCert.DecodeCertificateDER(qcc::String((const char*)encoded, encodedLen));
        } else {
            status = ER_FAIL;
        }
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to load encoded identity data"));
        }
    } while (0);

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
                                                   const qcc::GUID128& GuidId)
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
    args[1].Set("ay", qcc::GUID128::SIZE, GuidId.GetBytes());
    QCC_DbgPrintf(("Removing membership certificate with serial number %s", serial));
    status = proxyObjectManager->MethodCall(app, "RemoveMembership", args, 2, replyMsg);

    return status;
}
}
}
