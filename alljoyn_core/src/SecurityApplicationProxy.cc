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
#include <alljoyn/SecurityApplicationProxy.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/Util.h>
#include <string>
#include "PermissionMgmtObj.h"
#include "KeyInfoHelper.h"
#include "XmlManifestConverter.h"
#include "XmlRulesConverter.h"
#include "XmlPoliciesConverter.h"

#define QCC_MODULE "ALLJOYN_SECURITY"

namespace ajn {
SecurityApplicationProxy::SecurityApplicationProxy(BusAttachment& bus, const char* busName, SessionId sessionId) :
    ProxyBusObject(bus, busName, org::alljoyn::Bus::Security::ObjectPath, sessionId)
{
    QStatus status = ER_OK;
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    {
        /* Add org.alljoyn.Bus.Security.Application interface */
        const InterfaceDescription* intf = bus.GetInterface(org::alljoyn::Bus::Security::Application::InterfaceName);
        QCC_ASSERT(intf);
        status = AddInterface(*intf);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add the %s interface to the %s", org::alljoyn::Bus::Security::Application::InterfaceName, __FUNCTION__));
        }
    }
    {
        /* Add org.alljoyn.Bus.Security.ClaimableApplication interface */
        const InterfaceDescription* intf = bus.GetInterface(org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName);
        QCC_ASSERT(intf);
        status = AddInterface(*intf);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add the %s interface to the %s", org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName, __FUNCTION__));
        }
    }
    {
        /* Add org.alljoyn.Bus.Security.ManagedApplication interface */
        const InterfaceDescription* intf = bus.GetInterface(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName);
        QCC_ASSERT(intf);
        status = AddInterface(*intf);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add the %s interface to the %s", org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, __FUNCTION__));
        }
    }
}

SecurityApplicationProxy::~SecurityApplicationProxy()
{
}

/**
 * compute the status code base on the reply error name.
 * @return true if it can figure it out; false otherwise.
 */
static bool GetStatusBasedOnErrorName(Message& reply, QStatus& status)
{
    if (reply->GetErrorName() == NULL) {
        return false;
    }
    if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_PERMISSION_DENIED) == 0) {
        status = ER_PERMISSION_DENIED;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_INVALID_CERTIFICATE) == 0) {
        status = ER_INVALID_CERTIFICATE;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_INVALID_CERTIFICATE_USAGE) == 0) {
        status = ER_INVALID_CERTIFICATE_USAGE;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_DIGEST_MISMATCH) == 0) {
        status = ER_DIGEST_MISMATCH;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_POLICY_NOT_NEWER) == 0) {
        status = ER_POLICY_NOT_NEWER;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_CERTIFICATE_NOT_FOUND) == 0) {
        status = ER_CERTIFICATE_NOT_FOUND;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_DUPLICATE_CERTIFICATE) == 0) {
        status = ER_DUPLICATE_CERTIFICATE;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_MANAGEMENT_ALREADY_STARTED) == 0) {
        status = ER_MANAGEMENT_ALREADY_STARTED;
    } else if (strcmp(reply->GetErrorName(), PermissionMgmtObj::ERROR_MANAGEMENT_NOT_STARTED) == 0) {
        status = ER_MANAGEMENT_NOT_STARTED;
    } else if (strcmp(reply->GetErrorName(), org::alljoyn::Bus::ErrorName) == 0 && reply->GetArg(1)) {
        status = static_cast<QStatus>(reply->GetArg(1)->v_uint16);
    } else {
        return false;
    }
    return true;
}

QStatus SecurityApplicationProxy::GetSecurityApplicationVersion(uint16_t& version)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "Version", arg);
    if (ER_OK == status) {
        version = arg.v_variant.val->v_uint16;
    }

    return status;
}

QStatus SecurityApplicationProxy::GetApplicationState(PermissionConfigurator::ApplicationState& applicationState)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "ApplicationState", arg);
    if (ER_OK == status) {
        applicationState = static_cast<PermissionConfigurator::ApplicationState>(arg.v_variant.val->v_uint16);
    }

    return status;
}

QStatus SecurityApplicationProxy::GetManifestTemplateDigest(uint8_t* digest, size_t expectedSize)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "ManifestTemplateDigest", arg);
    if (ER_OK != status) {
        return status;
    }
    MsgArg* resultArg;
    status = arg.Get("v", &resultArg);
    if (ER_OK != status) {
        return status;
    }
    uint8_t algo;
    uint8_t* digestVal;
    size_t len;
    status = resultArg->Get("(yay)", &algo, &len, &digestVal);
    if (ER_OK != status) {
        return status;
    }
    if (algo != qcc::SigInfo::ALGORITHM_ECDSA_SHA_256) {
        return ER_INVALID_DATA;
    }
    if (len != expectedSize) {
        return ER_BAD_ARG_2;
    }
    memcpy(digest, digestVal, len);
    return ER_OK;
}

QStatus SecurityApplicationProxy::GetEccPublicKey(qcc::ECCPublicKey& eccPublicKey)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));

    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "EccPublicKey", arg);
    if (ER_OK == status) {
        qcc::KeyInfoNISTP256 keyInfo;
        status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(arg, keyInfo);
        if (ER_OK == status) {
            eccPublicKey = *keyInfo.GetPublicKey();
        }
    }

    return status;
}

QStatus SecurityApplicationProxy::GetManufacturerCertificate(MsgArg& certificate)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "ManufacturerCertificate", arg);
    if (ER_OK == status) {
        certificate = arg;
        certificate.Stabilize();
    }

    return status;
}

QStatus SecurityApplicationProxy::GetManifestTemplate(MsgArg& rules)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "ManifestTemplate", arg);
    if (ER_OK == status) {
        MsgArg* resultArg;
        status = arg.Get("v", &resultArg);
        rules = *resultArg;
        rules.Stabilize();
    }

    return status;
}

QStatus SecurityApplicationProxy::GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "ClaimCapabilities", arg);
    if (ER_OK == status) {
        claimCapabilities = static_cast<PermissionConfigurator::ClaimCapabilities>(arg.v_variant.val->v_uint16);
    }

    return status;
}

QStatus SecurityApplicationProxy::GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapabilitiesAdditionalInfo)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::Application::InterfaceName, "ClaimCapabilityAdditionalInfo", arg);
    if (ER_OK == status) {
        claimCapabilitiesAdditionalInfo = static_cast<PermissionConfigurator::ClaimCapabilityAdditionalInfo>(arg.v_variant.val->v_uint16);
    }

    return status;
}

QStatus SecurityApplicationProxy::Claim(const qcc::KeyInfoNISTP256& certificateAuthority,
                                        const qcc::GUID128& adminGroupId,
                                        const qcc::KeyInfoNISTP256& adminGroup,
                                        const qcc::CertificateX509* identityCertChain, size_t identityCertChainCount,
                                        const Manifest* manifests, size_t manifestCount)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    Message reply(GetBusAttachment());

    if ((identityCertChain == NULL) || (manifestCount < 1)) {
        return ER_INVALID_DATA;
    }
    if (identityCertChain[0].GetType() != qcc::CertificateX509::IDENTITY_CERTIFICATE) {
        QCC_DbgHLPrintf(("Leaf certificate in identity chain is not of type IDENTITY_CERTIFICATE. This is not recommended."));
    }
    MsgArg inputs[7];
    qcc::KeyInfoNISTP256 caKeyInfo(certificateAuthority);
    KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(caKeyInfo, inputs[0]);
    KeyInfoHelper::KeyInfoKeyIdToMsgArg(caKeyInfo, inputs[1]);

    status = inputs[2].Set("ay", qcc::GUID128::SIZE, adminGroupId.GetBytes());
    if (ER_OK != status) {
        return status;
    }
    qcc::KeyInfoNISTP256 adminGroupKeyInfo(adminGroup);
    KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(adminGroupKeyInfo, inputs[3]);
    KeyInfoHelper::KeyInfoKeyIdToMsgArg(adminGroupKeyInfo, inputs[4]);

    std::unique_ptr<MsgArg[]> identityArgs;
    if (identityCertChainCount == 0) {
        status = inputs[5].Set("a(yay)", 0, NULL);
        if (ER_OK != status) {
            return status;
        }
    } else {
        identityArgs.reset(new (std::nothrow) MsgArg[identityCertChainCount]);
        if (nullptr == identityArgs.get()) {
            return ER_OUT_OF_MEMORY;
        }
        for (size_t cnt = 0; cnt < identityCertChainCount; cnt++) {
            qcc::String der;
            status = identityCertChain[cnt].EncodeCertificateDER(der);
            if (ER_OK != status) {
                return status;
            }
            status = identityArgs[cnt].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, der.size(), der.data());
            if (ER_OK != status) {
                return status;
            }
            identityArgs[cnt].Stabilize();
        }
        status = inputs[5].Set("a(yay)", identityCertChainCount, identityArgs.get());
        if (ER_OK != status) {
            return status;
        }
    }
    if (manifestCount == 0) {
        status = inputs[6].Set(_Manifest::s_MsgArgArraySignature, 0, NULL);
        if (ER_OK != status) {
            return status;
        }
    } else {
        status = _Manifest::GetArrayMsgArg(manifests, manifestCount, inputs[6]);
        if (ER_OK != status) {
            return status;
        }
    }

    status = MethodCall(org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName, "Claim", inputs, 7, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }

    return status;
}

QStatus SecurityApplicationProxy::Claim(const qcc::KeyInfoNISTP256& certificateAuthority,
                                        const qcc::GUID128& adminGroupId,
                                        const qcc::KeyInfoNISTP256& adminGroup,
                                        const qcc::CertificateX509* identityCertChain, size_t identityCertChainCount,
                                        AJ_PCSTR* manifestsXmls, size_t manifestsCount)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));

    std::vector<Manifest> manifests;
    QStatus status = ExtractManifests(manifestsXmls, manifestsCount, manifests);

    if (ER_OK == status) {
        status = Claim(certificateAuthority,
                       adminGroupId,
                       adminGroup,
                       identityCertChain, identityCertChainCount,
                       manifests.data(), manifests.size());
    }

    return status;
}

QStatus SecurityApplicationProxy::GetClaimableApplicationVersion(uint16_t& version)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName, "Version", arg);
    if (ER_OK == status) {
        version = arg.v_variant.val->v_uint16;
    }
    return status;
}

QStatus SecurityApplicationProxy::Reset()
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(GetBusAttachment());

    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "Reset", NULL, 0, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
        return status;
    }
    return status;
}

QStatus SecurityApplicationProxy::UpdateIdentity(const qcc::CertificateX509* identityCertificateChain, size_t identityCertificateChainCount,
                                                 const Manifest* manifests, size_t manifestCount)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));

    if ((identityCertificateChain == NULL) || (identityCertificateChainCount == 0) || (manifestCount < 1)) {
        return ER_INVALID_DATA;
    }
    if (identityCertificateChain[0].GetType() != qcc::CertificateX509::IDENTITY_CERTIFICATE) {
        QCC_DbgHLPrintf(("Leaf certificate in identity chain is not of type IDENTITY_CERTIFICATE. This is not recommended."));
    }
    QStatus status = ER_OK;
    Message reply(GetBusAttachment());
    MsgArg inputs[2];
    std::unique_ptr<MsgArg[]> certArgs(new (std::nothrow) MsgArg[identityCertificateChainCount]);
    if (nullptr == certArgs.get()) {
        return ER_OUT_OF_MEMORY;
    }
    for (size_t cnt = 0; cnt < identityCertificateChainCount; cnt++) {
        qcc::String der;
        status = identityCertificateChain[cnt].EncodeCertificateDER(der);
        if (ER_OK != status) {
            return status;
        }
        status = certArgs[cnt].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, der.size(), der.data());
        if (ER_OK != status) {
            return status;
        }
        certArgs[cnt].Stabilize();
    }
    status = inputs[0].Set("a(yay)", identityCertificateChainCount, certArgs.get());
    if (ER_OK != status) {
        return status;
    }

    if (manifestCount == 0) {
        status = inputs[1].Set(_Manifest::s_MsgArgArraySignature, 0, NULL);
        if (ER_OK != status) {
            return status;
        }
    } else {
        status = _Manifest::GetArrayMsgArg(manifests, manifestCount, inputs[1]);
        if (ER_OK != status) {
            return status;
        }
    }
    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "UpdateIdentity", inputs, 2, reply);

    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }

    return status;
}

QStatus SecurityApplicationProxy::UpdateIdentity(const qcc::CertificateX509* identityCertificateChain, size_t identityCertificateChainCount,
                                                 AJ_PCSTR* manifestsXmls, size_t manifestsCount)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));

    std::vector<Manifest> manifests;
    QStatus status = ExtractManifests(manifestsXmls, manifestsCount, manifests);

    if (ER_OK == status) {
        status = UpdateIdentity(identityCertificateChain, identityCertificateChainCount,
                                manifests.data(), manifests.size());
    }

    return status;
}

QStatus SecurityApplicationProxy::UpdatePolicy(const PermissionPolicy& policy)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(GetBusAttachment());
    MsgArg inputs[1];

    status = policy.Export(inputs[0]);
    if (ER_OK != status) {
        return status;
    }

    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "UpdatePolicy", inputs, 1, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }
    return status;
}

QStatus SecurityApplicationProxy::UpdatePolicy(AJ_PCSTR policyXml)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status;
    PermissionPolicy policy;

    status = XmlPoliciesConverter::FromXml(policyXml, policy);

    if (ER_OK == status) {
        status = UpdatePolicy(policy);
    }

    return status;
}

QStatus SecurityApplicationProxy::ResetPolicy()
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(GetBusAttachment());

    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "ResetPolicy", NULL, 0, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }
    return status;
}

QStatus SecurityApplicationProxy::InstallMembership(const qcc::CertificateX509* certificateChain, size_t certificateChainCount)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(GetBusAttachment());

    if ((certificateChainCount >= 0) && (certificateChain[0].GetType() != qcc::CertificateX509::MEMBERSHIP_CERTIFICATE)) {
        QCC_DbgHLPrintf(("Leaf certificate in membership chain is not of type MEMBERSHIP_CERTIFICATE. This is not recommended."));
    }

    MsgArg inputs[1];
    MsgArg* certArgs = new MsgArg[certificateChainCount];
    for (size_t cnt = 0; cnt < certificateChainCount; cnt++) {
        qcc::String der;
        status = certificateChain[cnt].EncodeCertificateDER(der);
        if (ER_OK != status) {
            goto Exit;
        }
        status = certArgs[cnt].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, der.size(), der.data());
        if (ER_OK != status) {
            goto Exit;
        }
        certArgs[cnt].Stabilize();
    }
    status = inputs[0].Set("a(yay)", certificateChainCount, certArgs);
    if (ER_OK != status) {
        goto Exit;
    }

    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "InstallMembership", inputs, 1, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }

Exit:
    delete [] certArgs;
    return status;
}

QStatus SecurityApplicationProxy::RemoveMembership(const qcc::String& serial, const qcc::KeyInfoNISTP256& issuerKeyInfo)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;
    Message reply(GetBusAttachment());

    MsgArg inputs[1];

    size_t coordSize = issuerKeyInfo.GetPublicKey()->GetCoordinateSize();
    uint8_t* xData = new uint8_t[coordSize];
    uint8_t* yData = new uint8_t[coordSize];
    KeyInfoHelper::ExportCoordinates(*issuerKeyInfo.GetPublicKey(), xData, coordSize, yData, coordSize);
    status = inputs[0].Set("(ayay(yyayay))",
                           serial.size(), (uint8_t*) serial.data(),
                           issuerKeyInfo.GetKeyIdLen(), issuerKeyInfo.GetKeyId(),
                           issuerKeyInfo.GetAlgorithm(), issuerKeyInfo.GetCurve(),
                           coordSize, xData, coordSize, yData);
    if (ER_OK != status) {
        goto Exit;
    }

    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "RemoveMembership", inputs, 1, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }
Exit:
    delete [] xData;
    delete [] yData;
    return status;
}

QStatus SecurityApplicationProxy::InstallManifests(const Manifest* manifests, size_t manifestCount)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    Message reply(GetBusAttachment());

    MsgArg inputs[1];

    status = _Manifest::GetArrayMsgArg(manifests, manifestCount, inputs[0]);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not get MsgArg for manifests"));
        return status;
    }

    status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "InstallManifests", inputs, ArraySize(inputs), reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }

    return status;
}

QStatus SecurityApplicationProxy::GetManagedApplicationVersion(uint16_t& version)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "Version", arg);
    if (ER_OK == status) {
        version = arg.v_variant.val->v_uint16;
    }

    return status;
}

QStatus SecurityApplicationProxy::MsgArgToIdentityCertChain(const MsgArg& arg, qcc::CertificateX509* certs, size_t expectedSize)
{
    size_t certChainSize = 0;
    MsgArg* certArgs;
    QStatus status = arg.Get("a(yay)", &certChainSize, &certArgs);
    if (ER_OK != status) {
        return status;
    }
    if (certChainSize != expectedSize) {
        return ER_BAD_ARG_3;
    }

    for (size_t cnt = 0; cnt < certChainSize; cnt++) {
        uint8_t encoding;
        uint8_t* encoded;
        size_t encodedLen;
        status = certArgs[cnt].Get("(yay)", &encoding, &encodedLen, &encoded);
        if (ER_OK == status) {
            status = ER_NOT_IMPLEMENTED;
            if (encoding == qcc::CertificateX509::ENCODING_X509_DER) {
                status = certs[cnt].DecodeCertificateDER(qcc::String((const char*) encoded, encodedLen));
            } else if (encoding == qcc::CertificateX509::ENCODING_X509_DER_PEM) {
                status = certs[cnt].DecodeCertificatePEM(qcc::String((const char*) encoded, encodedLen));
            }
            if (ER_OK != status) {
                return status;
            }
        }
    }
    return status;
}

QStatus SecurityApplicationProxy::MsgArgToCertificateIds(const MsgArg& arg, qcc::String* serials, qcc::KeyInfoNISTP256* issuerKeyInfos, size_t expectedSize)
{
    MsgArg* membershipsArg;
    size_t count;
    QStatus status = arg.Get("a(ayay(yyayay))", &count, &membershipsArg);
    if (ER_OK != status) {
        return status;
    }
    if (count != expectedSize) {
        return ER_BAD_ARG_4;
    }

    for (size_t cnt = 0; cnt < count; cnt++) {
        uint8_t* serialVal;
        size_t serialLen;
        uint8_t* akiVal;
        size_t akiLen;
        uint8_t algorithm;
        uint8_t curve;
        uint8_t* xCoord;
        size_t xLen;
        uint8_t* yCoord;
        size_t yLen;
        status = membershipsArg[cnt].Get("(ayay(yyayay))", &serialLen, &serialVal, &akiLen, &akiVal, &algorithm, &curve, &xLen, &xCoord, &yLen, &yCoord);
        if (ER_OK != status) {
            return status;
        }
        if (algorithm != qcc::SigInfo::ALGORITHM_ECDSA_SHA_256) {
            return ER_INVALID_DATA;
        }
        if (curve != qcc::Crypto_ECC::ECC_NIST_P256) {
            return ER_INVALID_DATA;
        }
        if ((xLen != qcc::ECC_COORDINATE_SZ) || (yLen != qcc::ECC_COORDINATE_SZ)) {
            return ER_INVALID_DATA;
        }
        qcc::ECCPublicKey publicKey;
        publicKey.Import(xCoord, publicKey.GetCoordinateSize(), yCoord, publicKey.GetCoordinateSize());
        issuerKeyInfos[cnt].SetPublicKey(&publicKey);
        issuerKeyInfos[cnt].SetKeyId(akiVal, akiLen);
        serials[cnt].assign((const char*) serialVal, serialLen);
    }
    return status;
}

QStatus SecurityApplicationProxy::MsgArgToRules(const MsgArg& arg, PermissionPolicy::Rule* rules, size_t expectedSize)
{
    PermissionPolicy::Rule* localRules = NULL;
    size_t count = 0;
    QStatus status = PermissionPolicy::ParseRules(arg, &localRules, &count);
    if (ER_OK != status) {
        goto Exit;
    }
    if (count != expectedSize) {
        status = ER_BAD_ARG_3;
        goto Exit;
    }
    for (size_t cnt = 0; cnt < count; cnt++) {
        rules[cnt] = localRules[cnt];
    }
Exit:
    delete [] localRules;
    return status;
}

QStatus SecurityApplicationProxy::GetIdentity(MsgArg& identityCertificate)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "Identity", arg);
    if (ER_OK == status) {
        /* GetProperty returns a variant wrapper */
        MsgArg* resultArg;
        status = arg.Get("v", &resultArg);
        identityCertificate = *resultArg;
        identityCertificate.Stabilize();
    }

    return status;
}

QStatus SecurityApplicationProxy::GetManifests(std::vector<Manifest>& manifests)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    manifests.clear();

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "Manifests", arg);
    if (ER_OK == status) {
        /* GetProperty returns a variant wrapper */
        MsgArg* resultArg;
        status = arg.Get("v", &resultArg);
        if (ER_OK != status) {
            return status;
        }
        MsgArg* signedManifestArg;
        size_t signedManifestCount;
        status = resultArg->Get(_Manifest::s_MsgArgArraySignature, &signedManifestCount, &signedManifestArg);
        if (ER_OK != status) {
            return status;
        }
        /* Potentially save a reallocation for each push_back later by reserving now. */
        manifests.reserve(signedManifestCount);
        for (size_t i = 0; i < signedManifestCount; i++) {
            Manifest manifest;
            status = manifest->SetFromMsgArg(signedManifestArg[i]);
            if (ER_OK != status) {
                manifests.clear();
                return status;
            }
            manifests.push_back(std::move(manifest));
        }
    }

    return status;
}

QStatus SecurityApplicationProxy::GetManifestTemplate(AJ_PSTR* manifestTemplateXml)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));

    QStatus status;
    MsgArg argManifestTemplate;
    PermissionPolicy::Rule* rules = nullptr;
    size_t rulesCount = 0;
    std::string manifestTemplate;

    *manifestTemplateXml = nullptr;
    status = GetManifestTemplate(argManifestTemplate);

    if (ER_OK == status) {
        status = PermissionPolicy::ParseRules(argManifestTemplate, &rules, &rulesCount);
    }

    if (ER_OK == status) {
        status = XmlRulesConverter::RulesToXml(rules, rulesCount, manifestTemplate);
    }

    if (ER_OK == status) {
        *manifestTemplateXml = qcc::CreateStringCopy(manifestTemplate);
    }

    delete[] rules;

    return status;
}

void SecurityApplicationProxy::DestroyManifestTemplate(AJ_PSTR manifestTemplateXml)
{
    qcc::DestroyStringCopy(manifestTemplateXml);
}

QStatus SecurityApplicationProxy::GetIdentityCertificateId(qcc::String& serial, qcc::KeyInfoNISTP256& issuerKeyInfo)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "IdentityCertificateId", arg);
    if (ER_OK == status) {
        /* GetProperty returns a variant wrapper */
        MsgArg* resultArg;
        status = arg.Get("v", &resultArg);
        if (ER_OK != status) {
            return status;
        }
        uint8_t* serialVal;
        size_t serialLen;
        uint8_t* akiVal;
        size_t akiLen;
        uint8_t algorithm;
        uint8_t curve;
        uint8_t* xCoord;
        size_t xLen;
        uint8_t* yCoord;
        size_t yLen;
        status = resultArg->Get("(ayay(yyayay))", &serialLen, &serialVal, &akiLen, &akiVal, &algorithm, &curve, &xLen, &xCoord, &yLen, &yCoord);
        if (ER_OK != status) {
            return ER_INVALID_DATA;
        }
        if (algorithm != qcc::SigInfo::ALGORITHM_ECDSA_SHA_256) {
            return ER_INVALID_DATA;
        }
        if (curve != qcc::Crypto_ECC::ECC_NIST_P256) {
            return ER_INVALID_DATA;
        }
        if ((xLen != qcc::ECC_COORDINATE_SZ) || (yLen != qcc::ECC_COORDINATE_SZ)) {
            return ER_INVALID_DATA;
        }
        qcc::ECCPublicKey publicKey;
        publicKey.Import(xCoord, publicKey.GetCoordinateSize(), yCoord, publicKey.GetCoordinateSize());
        issuerKeyInfo.SetPublicKey(&publicKey);
        issuerKeyInfo.SetKeyId(akiVal, akiLen);

        if (serialLen == 0) {
            serial = qcc::String::Empty;
        } else {
            serial.assign((const char*)serialVal, serialLen);
        }
    }

    return status;
}

QStatus SecurityApplicationProxy::GetPolicyVersion(uint32_t& policyVersion)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "PolicyVersion", arg);
    if (ER_OK == status) {
        policyVersion = arg.v_variant.val->v_uint32;
    }

    return status;
}

QStatus SecurityApplicationProxy::GetPolicy(PermissionPolicy& policy)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "Policy", arg);
    if (ER_OK == status) {
        status = policy.Import(PermissionPolicy::SPEC_VERSION, arg);
    }

    return status;
}

QStatus SecurityApplicationProxy::GetDefaultPolicy(PermissionPolicy& defaultPolicy)
{
    QCC_UNUSED(defaultPolicy);
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "DefaultPolicy", arg);
    if (ER_OK == status) {
        status = defaultPolicy.Import(PermissionPolicy::SPEC_VERSION, arg);
    }

    return status;
}

QStatus SecurityApplicationProxy::GetMembershipSummaries(MsgArg& membershipSummaries)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = GetProperty(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "MembershipSummaries", arg);
    if (ER_OK == status) {
        /* GetProperty returns a variant wrapper */
        MsgArg* resultArg;
        status = arg.Get("v", &resultArg);
        membershipSummaries = *resultArg;
        membershipSummaries.Stabilize();
    }

    return status;
}

QStatus SecurityApplicationProxy::StartManagement()
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    Message reply(GetBusAttachment());
    QStatus status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "StartManagement", nullptr, 0, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }
    return status;
}

QStatus SecurityApplicationProxy::EndManagement()
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));
    Message reply(GetBusAttachment());
    QStatus status = MethodCall(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, "EndManagement", nullptr, 0, reply);
    if (ER_OK != status) {
        if (!GetStatusBasedOnErrorName(reply, status)) {
            QCC_LogError(status, ("SecurityApplicationProxy::%s error %s", __FUNCTION__, reply->GetErrorDescription().c_str()));
        }
    }
    return status;
}

QStatus SecurityApplicationProxy::ExtractManifests(AJ_PCSTR* manifestsXmls, size_t manifestsCount, std::vector<Manifest>& manifests)
{
    QCC_DbgTrace(("SecurityApplicationProxy::%s", __FUNCTION__));

    QStatus status = ER_OK;

    for (size_t index = 0; index < manifestsCount; index++) {
        Manifest manifest;

        status = XmlManifestConverter::XmlToManifest(manifestsXmls[index], manifest);

        if (ER_OK != status) {
            break;
        }

        manifests.push_back(manifest);
    }

    return status;
}

} // end namespace ajn
