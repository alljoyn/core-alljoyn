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

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/PermissionMgmtProxy.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/CertificateECC.h>
#include "KeyInfoHelper.h"

#define QCC_MODULE "ALLJOYN_PERMISSION_MGMT"

namespace ajn {
PermissionMgmtProxy::PermissionMgmtProxy(BusAttachment& bus, const char* busName, SessionId sessionId) :
    ProxyBusObject(bus, busName, org::allseen::Security::PermissionMgmt::ObjectPath, sessionId)
{
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    const InterfaceDescription* intf = bus.GetInterface(org::allseen::Security::PermissionMgmt::InterfaceName);
    assert(intf);
    AddInterface(*intf);
}

PermissionMgmtProxy::~PermissionMgmtProxy()
{
}

QStatus PermissionMgmtProxy::Claim(qcc::KeyInfoNISTP256& certificateAuthority, qcc::GUID128& adminGroupId, qcc::KeyInfoNISTP256& adminGroup, qcc::IdentityCertificate* identityCertChain, size_t identityCertChainSize, PermissionPolicy::Rule* manifest, size_t manifestSize) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    if ((identityCertChain == NULL) || (manifest == NULL)) {
        return ER_INVALID_DATA;
    }
    MsgArg inputs[7];
    if (certificateAuthority.GetKeyIdLen() == 0) {
        KeyInfoHelper::GenerateKeyId(certificateAuthority);
    }
    KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(certificateAuthority, inputs[0]);
    KeyInfoHelper::KeyInfoKeyIdToMsgArg(certificateAuthority, inputs[1]);

    status = inputs[2].Set("ay", qcc::GUID128::SIZE, adminGroupId.GetBytes());
    if (ER_OK != status) {
        return status;
    }
    if (adminGroup.GetKeyIdLen() == 0) {
        KeyInfoHelper::GenerateKeyId(adminGroup);
    }
    KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(adminGroup, inputs[3]);
    KeyInfoHelper::KeyInfoKeyIdToMsgArg(adminGroup, inputs[4]);

    MsgArg* identityArgs = NULL;
    if (identityCertChainSize == 0) {
        status = inputs[5].Set("a(yay)", 0, NULL);
        if (ER_OK != status) {
            return status;
        }
    } else {
        identityArgs = new MsgArg[identityCertChainSize];
        for (size_t cnt = 0; cnt < identityCertChainSize; cnt++) {
            identityArgs[cnt].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, identityCertChain[cnt].GetEncodedLen(), identityCertChain[cnt].GetEncoded());
        }
        status = inputs[5].Set("a(yay)", identityCertChainSize, identityArgs);
        if (ER_OK != status) {
            return status;
        }
    }
    if (manifestSize == 0) {
        status = inputs[6].Set("a(ssa(syy))", 0, NULL);
        if (ER_OK != status) {
            return status;
        }
    } else {
        status = PermissionPolicy::GenerateRules(manifest, manifestSize, inputs[6]);
        if (ER_OK != status) {
            return status;
        }
    }
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "Claim", inputs, 7, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    delete [] identityArgs;
    return status;
}

QStatus PermissionMgmtProxy::InstallPolicy(PermissionPolicy& authorization) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);
    MsgArg policyArg;

    status = authorization.Export(policyArg);
    if (ER_OK != status) {
        return status;
    }
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallPolicy", &policyArg, 1, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::InstallEncryptedPolicy(const MsgArg& encryptedAuthorizationArg) {
    QCC_UNUSED(encryptedAuthorizationArg);
    return ER_FAIL;
}

QStatus PermissionMgmtProxy::GetPolicy(PermissionPolicy* authorization) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status  = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "GetPolicy", NULL, 0, reply);

    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    if (status != ER_OK) {
        return status;
    }

    uint8_t version;
    MsgArg* variant;
    status = reply->GetArg(0)->Get("(yv)", &version, &variant);
    if (status != ER_OK) {
        return status;
    }
    return authorization->Import(version, *variant);
}

QStatus PermissionMgmtProxy::RemovePolicy() {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "RemovePolicy", NULL, 0, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::InstallMembership(const MsgArg& certChainArg) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallMembership", &certChainArg, 1, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::RemoveMembership(const char* serialNum, const qcc::String& issuerAki) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    MsgArg inputs[2];
    inputs[0].Set("s", serialNum);
    inputs[1].Set("ay", issuerAki.size(), issuerAki.data());
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "RemoveMembership", inputs, 2, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::InstallIdentity(qcc::IdentityCertificate* certChain, size_t certChainSize, const PermissionPolicy::Rule* manifest, size_t manifestSize)
{
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    if ((certChain == NULL) || (certChainSize == 0) || (manifest == NULL) || (manifestSize == 0)) {
        return ER_INVALID_DATA;
    }
    QStatus status = ER_OK;
    Message reply(*bus);
    MsgArg inputs[2];
    MsgArg* certArgs = new MsgArg[certChainSize];
    for (size_t cnt = 0; cnt < certChainSize; cnt++) {
        status = certArgs[cnt].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, certChain[cnt].GetEncodedLen(), certChain[cnt].GetEncoded());
        if (ER_OK != status) {
            goto Exit;
        }
    }
    status = inputs[0].Set("a(yay)", certChainSize, certArgs);
    if (ER_OK != status) {
        goto Exit;
    }
    status = PermissionPolicy::GenerateRules(manifest, manifestSize, inputs[1]);
    if (ER_OK != status) {
        goto Exit;
    }

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallIdentity", inputs, 2, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
Exit:
    delete [] certArgs;
    return status;
}

// TODO GetIdentity function breaks the pattern used for most other methods.
// It allocates memory for the user that they have to free. Figure out a way
// to let the user allocate there own memory without making multiple remote
// method calls.
QStatus PermissionMgmtProxy::GetIdentity(qcc::IdentityCertificate** certChain, size_t* certChainSize) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    Message reply(*bus);
    *certChain = NULL;
    *certChainSize = 0;

    QStatus status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "GetIdentity", NULL, 0, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    if (status != ER_OK) {
        return status;
    }

    MsgArg* certArgs;
    status = reply->GetArg(0)->Get("a(yay)", certChainSize, &certArgs);
    if (ER_OK != status) {
        return status;
    }
    if (*certChainSize == 0) {
        return ER_OK;
    }

    qcc::IdentityCertificate* chain = new qcc::IdentityCertificate[*certChainSize];
    for (size_t cnt = 0; cnt < *certChainSize; cnt++) {
        uint8_t encoding;
        uint8_t* encoded;
        size_t encodedLen;
        status = certArgs[cnt].Get("(yay)", &encoding, &encodedLen, &encoded);
        if (ER_OK == status) {
            status = ER_NOT_IMPLEMENTED;
            if (encoding == qcc::CertificateX509::ENCODING_X509_DER) {
                status = chain[cnt].DecodeCertificateDER(qcc::String((const char*) encoded, encodedLen));
            } else if (encoding == qcc::CertificateX509::ENCODING_X509_DER_PEM) {
                status = chain[cnt].DecodeCertificatePEM(qcc::String((const char*) encoded, encodedLen));
            }
            if (ER_OK != status) {
                delete [] chain;
                *certChainSize = 0;
                return status;
            }
        }
    }
    *certChain = chain;
    return status;
}

// TODO GetManifest function breaks the pattern used for most other methods.
// It allocates memory for the user that they have to free. Figure out a way
// to let the user allocate there own memory without making multiple remote
// method calls.
QStatus PermissionMgmtProxy::GetManifest(PermissionPolicy::Rule** rules, size_t* count) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "GetManifest", NULL, 0, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }

    uint8_t type;
    MsgArg* variant;
    status = reply->GetArg(0)->Get("(yv)", &type, &variant);
    if (ER_OK != status) {
        return status;
    }

    return PermissionPolicy::ParseRules(*variant, rules, count);
}

QStatus PermissionMgmtProxy::Reset() {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "Reset", NULL, 0, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::GetPublicKey(qcc::ECCPublicKey* pubKey) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "GetPublicKey", NULL, 0, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    if (ER_OK != status) {
        return status;
    }
    /* retrieve ECCPublicKey from MsgArg */
    return RetrieveECCPublicKeyFromMsgArg(*reply->GetArg(0), pubKey);
}

QStatus PermissionMgmtProxy::GetVersion(uint16_t& version) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::allseen::Security::PermissionMgmt::InterfaceName, "Version", arg);
    if (ER_OK == status) {
        version = arg.v_variant.val->v_int16;
    }

    return status;
}


bool PermissionMgmtProxy::IsPermissionDeniedError(const Message& msg)
{
    qcc::String errorMsg;
    const char* errorName = msg->GetErrorName(&errorMsg);
    if (errorName == NULL) {
        return false;
    }
    if (strcmp(errorName, "org.alljoyn.Bus.ER_PERMISSION_DENIED") == 0) {
        return true;
    }
    if (strcmp(errorName, "org.alljoyn.Bus.ErStatus") != 0) {
        return false;
    }
    if (errorMsg == "ER_PERMISSION_DENIED") {
        return true;
    }
    return false;
}

QStatus PermissionMgmtProxy::RetrieveECCPublicKeyFromMsgArg(const MsgArg& arg, qcc::ECCPublicKey* pubKey) {
    uint8_t keyFormat;
    MsgArg* variantArg;
    QStatus status = arg.Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return status;
    }
    if (keyFormat != qcc::KeyInfo::FORMAT_ALLJOYN) {
        return status;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return status;
    }
    if ((keyUsageType != qcc::KeyInfo::USAGE_SIGNING) && (keyUsageType != qcc::KeyInfo::USAGE_ENCRYPTION)) {
        return status;
    }
    if (keyType != qcc::KeyInfoECC::KEY_TYPE) {
        return status;
    }
    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        return status;
    }
    if (curve != qcc::Crypto_ECC::ECC_NIST_P256) {
        return status;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return status;
    }
    if ((xLen != qcc::ECC_COORDINATE_SZ) || (yLen != qcc::ECC_COORDINATE_SZ)) {
        return status;
    }
    qcc::KeyInfoNISTP256 keyInfo;
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    memcpy(pubKey, keyInfo.GetPublicKey(), sizeof(qcc::ECCPublicKey));
    return ER_OK;
}
}

