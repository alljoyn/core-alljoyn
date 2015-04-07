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

QStatus PermissionMgmtProxy::Claim(const MsgArg& publicKeyArg, const MsgArg& identityCertArg, qcc::ECCPublicKey* returnKeyArg) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    MsgArg inputs[2];
    inputs[0] = publicKeyArg;
    inputs[1] = identityCertArg;

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "Claim", inputs, 2, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    if (ER_OK == status) {
        status = RetrieveECCPublicKeyFromMsgArg(*reply->GetArg(0), returnKeyArg);
    }
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

QStatus PermissionMgmtProxy::InstallMembershipAuthData(const char* serialNum, const qcc::GUID128& issuer, PermissionPolicy& authorization) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    MsgArg inputs[3];
    inputs[0].Set("s", serialNum);
    // TODO should the uint8_t issuer be changed to a GUID128?
    inputs[1].Set("ay", qcc::GUID128::SIZE, issuer.GetBytes());
    authorization.Export(inputs[2]);
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallMembershipAuthData", inputs, 3, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::RemoveMembership(const char* serialNum, const qcc::GUID128& issuer) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    MsgArg inputs[2];
    inputs[0].Set("s", serialNum);
    inputs[1].Set("ay", qcc::GUID128::SIZE, issuer.GetBytes());
    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "RemoveMembership", inputs, 2, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::InstallIdentity(const MsgArg& certArg) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallIdentity", &certArg, 1, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::GetIdentity(qcc::IdentityCertificate* cert) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "GetIdentity", NULL, 0, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    if (status != ER_OK) {
        return status;
    }

    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    status = reply->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        return status;
    }
    status = ER_NOT_IMPLEMENTED;
    if (encoding == qcc::CertificateX509::ENCODING_X509_DER) {
        status = cert->DecodeCertificateDER(qcc::String((const char*) encoded, encodedLen));
    } else if (encoding == qcc::CertificateX509::ENCODING_X509_DER_PEM) {
        status = cert->DecodeCertificatePEM(qcc::String((const char*) encoded, encodedLen));
    }
    return status;
}

QStatus PermissionMgmtProxy::InstallGuildEquivalence(const MsgArg& certArg) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallGuildEquivalence", &certArg, 1, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::RemoveGuildEquivalence(uint8_t* guildSerialNum, size_t guildSerialNumSize, uint8_t* issuer, size_t issuerSize) {
    QCC_UNUSED(guildSerialNum);
    QCC_UNUSED(guildSerialNumSize);
    QCC_UNUSED(issuer);
    QCC_UNUSED(issuerSize);
    return ER_FAIL;
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

QStatus PermissionMgmtProxy::InstallCredential(const uint8_t credentialType, const MsgArg& credential) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    MsgArg input[2];
    input[0].Set("y", credentialType);
    input[1].Set("v", &credential);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "InstallCredential", input, 2, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtProxy::RemoveCredential(const uint8_t credentialType, const MsgArg& credentialID) {
    QCC_DbgTrace(("PermissionMgmtProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(*bus);

    MsgArg input[2];
    input[0].Set("y", credentialType);
    input[1].Set("v", &credentialID);

    status = MethodCall(org::allseen::Security::PermissionMgmt::InterfaceName, "RemoveCredential", input, 2, reply);
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (IsPermissionDeniedError(reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
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

