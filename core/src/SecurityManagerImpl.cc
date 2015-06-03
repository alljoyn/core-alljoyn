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

#include <ctime>
#include <alljoyn/version.h>
#include <alljoyn/Session.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObj.h>

#include <qcc/Debug.h>
#include <qcc/CertificateECC.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/Crypto.h>
#include <CredentialAccessor.h> // still in alljoyn_core/src!
#include <PermissionMgmtObj.h> // still in alljoyn_core/src!
#include "SecurityManagerImpl.h"
#include "ApplicationUpdater.h"

#include <Common.h>

#include <iostream>

#include <SecLibDef.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class ECDHEKeyXListener :
    public AuthListener {
  public:
    ECDHEKeyXListener()
    {
    }

    bool RequestCredentials(const char* authMechanism, const char* authPeer,
                            uint16_t authCount, const char* userId, uint16_t credMask,
                            Credentials& creds)
    {
        QCC_UNUSED(credMask);
        QCC_UNUSED(userId);
        QCC_UNUSED(authCount);
        QCC_UNUSED(authPeer);

        QCC_DbgPrintf(("RequestCredentials %s", authMechanism));
        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
            creds.SetExpiration(100);             /* set the master secret expiry time to 100 seconds */
            return true;
        }
        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer,
                           const Credentials& creds)
    {
        QCC_UNUSED(creds);
        QCC_UNUSED(authPeer);

        QCC_DbgPrintf(("SecMgr: VerifyCredentials %s", authMechanism));
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer,
                                bool success)
    {
        QCC_UNUSED(authPeer);

        QCC_DbgPrintf(("SecMgr: AuthenticationComplete '%s' success = %i", authMechanism, success));
    }
};

//No member method as we cannot include CredentialAccessor.h in SecurityMngrImpl.h
static QStatus ClaimSelf(CredentialAccessor& ca,
                         BusAttachment* ba,
                         qcc::GUID128 adminGroupId,
                         const qcc::ECCPublicKey* smPublicKey,
                         const qcc::GUID128 smPeerId,
                         X509CertificateGenerator* certGen)
{
    qcc::GUID128 localGuid;
    qcc::ECCPublicKey subjectPubKey;
    ca.GetGuid(localGuid);
    ca.GetDSAPublicKey(subjectPubKey);

    qcc::KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&subjectPubKey);
    keyInfo.SetKeyId(localGuid.GetBytes(), qcc::GUID128::SIZE);
    ajn::PermissionMgmtObj::TrustAnchor* anchor = new ajn::PermissionMgmtObj::TrustAnchor(
        ajn::PermissionMgmtObj::TRUST_ANCHOR_CA,
        keyInfo);

    if (anchor == NULL) {
        QCC_LogError(ER_FAIL, ("Failed to allocate TrustAnchor"));
        return ER_FAIL;
    }

    ajn::PermissionMgmtObj pmo(*ba);
    QStatus status = pmo.InstallTrustAnchor(anchor); //ownership is transferred
    if (status != ER_OK) {
        QCC_LogError(ER_FAIL, ("Failed to install TrustAnchor"));
        return ER_FAIL;
    }

    // Manifest
    size_t manifestSize = 1;
    PermissionPolicy::Rule* manifest = new PermissionPolicy::Rule[1];
    manifest[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* mfPrms = new PermissionPolicy::Rule::Member[1];
    mfPrms[0].SetMemberName("*");
    mfPrms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                            PermissionPolicy::Rule::Member::ACTION_MODIFY |
                            PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    manifest[0].SetMembers(1, mfPrms);

    MsgArg manifestMsgArg;
    status = PermissionPolicy::GenerateRules(manifest, manifestSize, manifestMsgArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to marshall manifest"));
        return status;
    }

    // Identity certificate
    PermissionConfigurator& pcf = ba->GetPermissionConfigurator();
    IdentityCertificate x509;
    x509.SetSerial(qcc::String("0"));
    x509.SetIssuerCN(localGuid.GetBytes(), qcc::GUID128::SIZE);
    x509.SetSubjectCN(localGuid.GetBytes(), qcc::GUID128::SIZE);
    x509.SetSubjectPublicKey(&subjectPubKey);
    qcc::CertificateX509::ValidPeriod period;
    period.validFrom = time(NULL) - 3600;
    period.validTo = period.validFrom + 3600 +  3153600;    // valid for 365 days
    x509.SetValidity(&period);
    x509.SetAlias("Admin");

    uint8_t mfDigest[Crypto_SHA256::DIGEST_SIZE];
    PermissionMgmtObj::GenerateManifestDigest(*ba, manifest,
                                              manifestSize, mfDigest,
                                              Crypto_SHA256::DIGEST_SIZE);
    x509.SetDigest(mfDigest, Crypto_SHA256::DIGEST_SIZE);

    status =  pcf.SignCertificate(x509);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to sign certificate"));
        return status;
    }
    qcc::String der;
    status = x509.EncodeCertificateDER(der);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to encode certificate"));
        return status;
    }
    MsgArg certs[1];
    certs[0].Set("(yay)", CertificateX509::ENCODING_X509_DER, der.size(), der.data());
    MsgArg chain("a(yay)", 1, certs);
    status = pmo.StoreIdentityCertChain(chain);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store own identity certificate"));
        return status;
    }

    status = pmo.StoreManifest(manifestMsgArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store local manifest"));
        return status;
    } else {
        QCC_DbgHLPrintf(("Successfully stored local manifest"));
    }

    PermissionPolicy localPolicy;
    localPolicy.SetSerialNum(1);

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    /* terms record 0  ANY-USER */
    if (terms == NULL || peers == NULL || rules == NULL || prms == NULL) {
        QCC_LogError(status, ("Failed to allocate policy resources"));
        delete[] terms;
        delete[] peers;
        delete[] rules;
        delete[] prms;
        return ER_FAIL;
    }

    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    rules[0].SetInterfaceName("*");
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE  | PermissionPolicy::Rule::Member::ACTION_MODIFY |
        PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    terms[0].SetRules(1, rules);

    localPolicy.SetTerms(1, terms);
    status = pmo.StorePolicy(localPolicy);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store local policy"));
    }

    // Generate membership certificate
    qcc::MembershipCertificate mCert;
    mCert.SetGuild(adminGroupId);
    mCert.SetSubjectPublicKey(smPublicKey);
    mCert.SetCA(false);
    mCert.SetSubjectCN(smPeerId.GetBytes(), qcc::GUID128::SIZE);
    mCert.SetSerial("42");
    qcc::CertificateX509::ValidPeriod mcPeriod;
    mcPeriod.validFrom = time(NULL) - 3600;
    mcPeriod.validTo = mcPeriod.validFrom + 3600 + 3153600;
    mCert.SetValidity(&mcPeriod);
    certGen->GenerateMembershipCertificate(mCert);

    // Marshall
    ajn::MsgArg mcArg[1];
    qcc::String mcDER((const char*)mCert.GetEncoded(), mCert.GetEncodedLen());
    mcArg[0].Set("(yay)", qcc::CertificateX509::ENCODING_X509_DER, mcDER.length(),
                 mcDER.data());
    ajn::MsgArg mChainArg("a(yay)", 1, mcArg);

    // Add to local storage
    pmo.StoreMembership(mChainArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store local membership certificate"));
    } else {
        QCC_DbgHLPrintf(("Successfully stored local membership certificate"));
    }

    return status;
}

SecurityManagerImpl::SecurityManagerImpl(ajn::BusAttachment* ba,
                                         const Storage* _storage) :
    pubKey(),
    appMonitor(ApplicationMonitor::GetApplicationMonitor(ba,
                                                         "org.allseen.Security.PermissionMgmt.Notification")),
    busAttachment(ba),
    storage((Storage*)_storage),
    queue(TaskQueue<AppListenerEvent*, SecurityManagerImpl>(this)), mfListener(NULL)
{
    CertificateGen = NULL;
    remoteApplicationManager = NULL;
    proxyObjMgr = NULL;
    applicationUpdater = NULL;
    adminGroupId = qcc::GUID128(0xab); // TODO: retrieve from storage
}

QStatus SecurityManagerImpl::Init()
{
    SessionOpts opts;
    QStatus status = ER_OK;

    do {
        if (NULL == storage) {
            status = ER_FAIL;
            QCC_LogError(status, ("Invalid storage means."));
            break;
        }

        if (NULL == busAttachment) {
            status = ER_FAIL;
            QCC_LogError(status, ("Null bus attachment."));
            break;
        }

        ProxyObjectManager::listener = new ECDHEKeyXListener();
        if (ProxyObjectManager::listener == NULL) {
            status = ER_FAIL;
            QCC_LogError(status, ("Failed to allocate ECDHEKeyXListener"));
            break;
        }

        status = busAttachment->EnablePeerSecurity(KEYX_ECDHE_NULL, ProxyObjectManager::listener,
                                                   AJNKEY_STORE, true);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Failed to enable security on the security manager bus attachment."));
            break;
        }

        CredentialAccessor ca(*busAttachment);
        if (ER_OK != (status = ca.GetGuid(localGuid))) {
            QCC_LogError(status, ("Failed to retrieve local Peer ID."));
            break;
        }

        ca.GetDSAPublicKey(pubKey);

        CertificateGen = new X509CertificateGenerator(localGuid, busAttachment);
        proxyObjMgr = new ProxyObjectManager(busAttachment);
        remoteApplicationManager = new RemoteApplicationManager(proxyObjMgr, busAttachment);

        if ((NULL == CertificateGen) || (NULL == proxyObjMgr) || (NULL == remoteApplicationManager)) {
            QCC_LogError(ER_FAIL,
                         ("Could not create certificate generator or proxy object manager !"));
            status = ER_FAIL;
            break;
        }

        if (!remoteApplicationManager->Initialized()) {
            delete CertificateGen;
            delete proxyObjMgr;     //To be removed later
            delete remoteApplicationManager;
            QCC_LogError(ER_FAIL, ("Could not initialize the remote application manager"));
            break;
        }

        if (PermissionConfigurator::STATE_CLAIMABLE ==
            busAttachment->GetPermissionConfigurator().GetClaimableState()) {
            status = ClaimSelf(ca, busAttachment, adminGroupId, &pubKey, localGuid, CertificateGen);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to claim self"));
                break;
            }
        }

        applicationUpdater =
            new ApplicationUpdater(busAttachment, storage, remoteApplicationManager, this, pubKey);
        if (NULL == applicationUpdater) {
            status = ER_FAIL;
            QCC_LogError(status, ("Failed to initialize application updater."));
            break;
        }

        std::vector<ManagedApplicationInfo> managedApplications;

        status = storage->GetManagedApplications(managedApplications);
        if (ER_OK != status) {
            QCC_LogError(ER_FAIL, ("Could not get managed applications."));
            break;
        }

        std::vector<ManagedApplicationInfo>::const_iterator it =
            managedApplications.begin();
        for (; it != managedApplications.end(); ++it) {
            ApplicationInfo info;
            info.claimState = ajn::PermissionConfigurator::STATE_CLAIMED;
            info.runningState = STATE_UNKNOWN_RUNNING;
            info.publicKey = it->publicKey;
            info.userDefinedName = it->userDefinedName;
            info.busName = "";                        // To be filled in when discovering the app is online.
            info.peerID = it->peerID;
            info.deviceName = it->deviceName;
            info.appName = it->appName;
            info.updatesPending = it->updatesPending;
            appsMutex.Lock(__FILE__, __LINE__);
            applications[info.publicKey] = info;
            appsMutex.Unlock(__FILE__, __LINE__);
        }

        if (NULL == appMonitor) {
            QCC_LogError(status, ("NULL Application Monitor"));
            status = ER_FAIL;
            break;
        }
        appMonitor->RegisterSecurityInfoListener(this);
        appMonitor->RegisterSecurityInfoListener(applicationUpdater);

        busAttachment->RegisterAboutListener(*this);
    } while (0);

    return status;
}

SecurityManagerImpl::~SecurityManagerImpl()
{
    busAttachment->UnregisterAboutListener(*this);

    appMonitor->UnregisterSecurityInfoListener(applicationUpdater);
    appMonitor->UnregisterSecurityInfoListener(this);

    queue.Stop();

    delete applicationUpdater;
    delete CertificateGen;
    delete proxyObjMgr;
    delete remoteApplicationManager;
    delete appMonitor;
    delete ProxyObjectManager::listener;
    ProxyObjectManager::listener = NULL;
}

void SecurityManagerImpl::SetManifestListener(ManifestListener* mfl)
{
    mfListener = mfl;
}

QStatus SecurityManagerImpl::Claim(const ApplicationInfo& appInfo,
                                   const IdentityInfo& identityInfo)
{
    QStatus status;

    // Check ManifestListener
    if (mfListener == NULL) {
        status = ER_FAIL;
        QCC_LogError(status, ("No ManifestListener set"));
        return status;
    }

    // Check appInfo
    ApplicationInfo app;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        status = ER_FAIL;
        QCC_LogError(status, ("Unknown application"));
        return status;
    }
    app = appItr->second;

    // Check identityInfo
    IdentityInfo idInfo = identityInfo;
    status = storage->GetIdentity(idInfo);
    if (ER_OK != status) {
        QCC_LogError(status, ("Unknown identity"));
        return status;
    }

    /*===========================================================
     * Step 1: Accept manifest
     */
    PermissionPolicy::Rule* manifest;
    size_t manifestSize = 0;
    status = remoteApplicationManager->GetManifest(app, &manifest, &manifestSize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not retrieve manifest"));
        return status;
    }

    if (!mfListener->ApproveManifest(app, manifest, manifestSize)) {
        return ER_MANIFEST_REJECTED;
    }

    /*===========================================================
     * Step 2: Claim
     */

    qcc::KeyInfoNISTP256 CAKeyInfo;
    CAKeyInfo.SetPublicKey(&pubKey);
    CAKeyInfo.SetKeyId(localGuid.GetBytes(), GUID128::SIZE);

    qcc::IdentityCertificate idCertificate;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    PermissionMgmtObj::GenerateManifestDigest(*busAttachment, manifest,
                                              manifestSize, digest, Crypto_SHA256::DIGEST_SIZE);
    idCertificate.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);

    status = GenerateIdentityCertificate(idCertificate, idInfo, app);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to create IdentityCertificate"));
        return status;
    }

    status = remoteApplicationManager->Claim(app, CAKeyInfo, adminGroupId,
                                             CAKeyInfo, &idCertificate, 1, manifest, manifestSize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not claim application"));
        return status;
    }

    /*===========================================================
     * Step 3: Persist
     */

    status = PersistApplicationInfo(app, false);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not persist application"));
        return status;
    }

    status = PersistManifest(app, manifest, manifestSize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not persist application's manifest"));
        return status;
    }

    status = storage->StoreCertificate(idCertificate);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not persist identity certificate"));
        return status;
    }
    ;

    return ER_OK;
}

// TODO: move to ECCPublicKey class
QStatus SecurityManagerImpl::MarshalPublicKey(const ECCPublicKey* pubKey, const GUID128& localPeerID,
                                              MsgArg& ma)
{
    QCC_DbgPrintf(("Marshalling PublicKey"));
    QStatus status = ER_OK;

    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(pubKey);

    QCC_DbgPrintf(("localPeerID = %s", localPeerID.ToString().c_str()));
    ma.Set("(yv)", qcc::KeyInfo::FORMAT_ALLJOYN,
           new MsgArg("(ayyyv)", GUID128::SIZE, localPeerID.GetBytes(), KeyInfo::USAGE_SIGNING,
                      KeyInfoECC::KEY_TYPE,
                      new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(),
                                 new MsgArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(),
                                            ECC_COORDINATE_SZ, keyInfo.GetYCoord()))));
    ma.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    ma.Stabilize();

    return status;
}

// TODO: move to ECCPublicKey class
QStatus SecurityManagerImpl::UnmarshalPublicKey(const MsgArg* ma, ECCPublicKey& pubKey)
{
    QStatus status = ER_OK;

    if (NULL == ma) {
        status = ER_FAIL;
        QCC_LogError(status, ("NULL args!"));
        return status;
    }

    uint8_t keyFormat;
    MsgArg* variantArg;
    status = ma->Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to unmarshal public key"));
        return status;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        status = ER_FAIL;
        QCC_LogError(status, ("Invalid public key format"));
        return status;
    }

    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to unmarshal public key"));
        return status;
    }
    if ((keyUsageType != KeyInfo::USAGE_SIGNING) &&
        (keyUsageType != KeyInfo::USAGE_ENCRYPTION)) {
        status = ER_FAIL;
        QCC_LogError(status, ("Invalid public key usage types"));
        return status;
    }
    if (keyType != KeyInfoECC::KEY_TYPE) {
        status = ER_FAIL;
        QCC_LogError(status, ("Invalid public key type"));
        return status;
    }

    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to unmarshal public key"));
        return status;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        status = ER_FAIL;
        QCC_LogError(status, ("Invalid public key curve"));
        return status;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to unmarshal public key"));
        return status;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        status = ER_FAIL;
        QCC_LogError(status, ("Invalid public coordinate size"));
        return status;
    }

    KeyInfoNISTP256 keyInfo;
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    memcpy(&pubKey, keyInfo.GetPublicKey(), sizeof(ECCPublicKey));

    return ER_OK;
}

QStatus SecurityManagerImpl::InstallMembership(const ApplicationInfo& appInfo,
                                               const GuildInfo& guildInfo)
{
    // Check application
    QStatus status = ER_FAIL;
    ApplicationInfo app;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);

    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unknown application"));
        return ER_FAIL;
    }
    app = appItr->second;

    // Check guild
    GuildInfo gi = guildInfo;

    status = storage->GetGuild(gi);

    if (status != ER_OK) {
        QCC_LogError(status, ("Unknown guild"));
        return status;
    }

    qcc::MembershipCertificate cert;
    cert.SetGuild(gi.guid);
    cert.SetSubjectPublicKey(&app.publicKey);

    do {
        cert.SetGuild(guildInfo.guid);
        cert.SetCA(false);
        cert.SetSubjectCN(app.peerID.GetBytes(), qcc::GUID128::SIZE);

        qcc::String serialNumber;

        status = storage->GetNewSerialNumber(serialNumber);

        if (status != ER_OK) {
            QCC_LogError(ER_FAIL, ("Could not get a serial number."));
            break;
        }

        cert.SetSerial(serialNumber);
        qcc::CertificateX509::ValidPeriod period;
        period.validFrom = time(NULL) - 3600;
        period.validTo = period.validFrom + 3600 +  3153600;    // valid for 365 days
        cert.SetValidity(&period);

        status = CertificateGen->GenerateMembershipCertificate(cert);

        if (status != ER_OK) {
            QCC_DbgRemoteError(("Failed to generate membership certificate"));
            break;
        }
        /******************************************************************************/

        /************Persist the certificate and the authorization data****************/

        if (ER_OK != (status = storage->StoreCertificate(cert, true))) {     //update if already exists.
            QCC_LogError(status, ("Failed to store membership certificate"));
            break;
        }

        /******************************************************************************/

        applicationUpdater->UpdateApplication(app);
    } while (0);

    return status;
}

QStatus SecurityManagerImpl::RemoveMembership(const ApplicationInfo& appInfo,
                                              const GuildInfo& guildInfo)
{
    //Sanity check. Make sure we use our internal collected data. Don't trust what is passed to us.
    ApplicationInfo app;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);

    if (!exist) {
        QCC_LogError(ER_FAIL, ("App does not exist."));
        return ER_FAIL;
    }
    app = appItr->second;

    qcc::MembershipCertificate cert;
    cert.SetGuild(guildInfo.guid);
    qcc::ECCPublicKey eccAppPubKey;

    memcpy(eccAppPubKey.x, app.publicKey.x,
           sizeof(eccAppPubKey.x));
    memcpy(eccAppPubKey.y, app.publicKey.y,
           sizeof(eccAppPubKey.y));

    cert.SetSubjectPublicKey(&eccAppPubKey);

    QStatus status = storage->GetCertificate(cert);

    if (status != ER_OK) {
        QCC_DbgRemoteError(
            ("Could not retrieve certificate %d", status));
        return status;
    }

    status = storage->RemoveCertificate(cert);

    if (status != ER_OK) {
        QCC_DbgRemoteError(
            ("Could not remove certificate %d", status));
        return status;
    }
    CredentialAccessor ca(*busAttachment);
    ECCPublicKey key;
    ca.GetDSAPublicKey(key);
    cert.GenerateAuthorityKeyId(&key);
    status = remoteApplicationManager->RemoveMembership(app, cert.GetSerial(), cert.GetAuthorityKeyId());

    return status;
}

QStatus SecurityManagerImpl::GetPersistedPolicy(const ApplicationInfo& appInfo,
                                                PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;

    ManagedApplicationInfo mgdAppInfo;
    mgdAppInfo.publicKey = appInfo.publicKey;
    status = storage->GetManagedApplication(mgdAppInfo);

    if (ER_OK != status) {
        QCC_LogError(status, ("Could not find managed application."));
        return status;
    }

    if (!mgdAppInfo.policy.empty()) {
        Message tmpMsg(*busAttachment);
        DefaultPolicyMarshaller marshaller(tmpMsg);
        status = policy.Import(marshaller, (const uint8_t*)mgdAppInfo.policy.data(),
                               mgdAppInfo.policy.size());
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not import policy to target."));
        }
    } else {
        status = ER_END_OF_DATA;
    }
    return status;
}

QStatus SecurityManagerImpl::UpdatePolicy(const ApplicationInfo& appInfo,
                                          PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);

    if (!exist) {
        QCC_LogError(status, ("Unkown application."));
        return status;
    }

    ApplicationInfo app = appItr->second;

    if (policy.GetSerialNum() == 0) {
        PermissionPolicy persistedPolicy;
        uint32_t newPolicyNumber = 1;
        status = GetPersistedPolicy(app, persistedPolicy);
        if (ER_OK == status) {
            newPolicyNumber = persistedPolicy.GetSerialNum() + 1;
            policy.SetSerialNum(newPolicyNumber);
        } else if (ER_END_OF_DATA == status) {
            policy.SetSerialNum(newPolicyNumber);
        } else {
            status = ER_FAIL;
            QCC_LogError(status, ("Could not determine next policy serial number"));
            return status;
        }
    }

    status = PersistPolicy(app, policy);
    if (ER_OK == status) {
        applicationUpdater->UpdateApplication(app);
    }

    return status;
}

QStatus SecurityManagerImpl::GetPolicy(const ApplicationInfo& appInfo,
                                       PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unkown application."));
        return ER_FAIL;
    }
    ApplicationInfo app = appItr->second;

    status = GetPersistedPolicy(app, policy);
    return status;
}

QStatus SecurityManagerImpl::Reset(const ApplicationInfo& appInfo)
{
    QStatus status = ER_FAIL;

    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("App does not exist."));
        return ER_FAIL;
    }
    ApplicationInfo app = appItr->second;

    ManagedApplicationInfo managedApplicationInfo;
    managedApplicationInfo.publicKey = app.publicKey;
    if (ER_OK != (status = storage->RemoveApplication(managedApplicationInfo))) {
        QCC_LogError(status, ("Failed to remove application from storage"));
        return status;
    }

    applicationUpdater->UpdateApplication(app);

    return status;
}

void SecurityManagerImpl::AddAboutInfo(ApplicationInfo& ai)
{
    std::map<qcc::String, ApplicationInfo>::iterator appCachedAboutInfoItr;
    aboutCacheMutex.Lock(__FILE__, __LINE__);
    appCachedAboutInfoItr = aboutCache.find(ai.busName);
    if (appCachedAboutInfoItr != aboutCache.end()) {
        ai.appName = appCachedAboutInfoItr->second.appName;
        ai.deviceName = appCachedAboutInfoItr->second.deviceName;
        aboutCache.erase(ai.busName);
    }
    aboutCacheMutex.Unlock(__FILE__, __LINE__);
}

void SecurityManagerImpl::AddSecurityInfo(ApplicationInfo& ai, const SecurityInfo& si)
{
    ai.busName = si.busName;
    ai.runningState = si.runningState;
    ai.claimState = si.claimState;
    ai.publicKey = si.publicKey;
    ai.rootsOfTrust = si.rootsOfTrust;
}

void SecurityManagerImpl::RemoveSecurityInfo(ApplicationInfo& ai, const SecurityInfo& si)
{
    // update application info if the busName is still relevant
    if (ai.busName == si.busName) {
        // ai.busName = ""; // still used in CLI
        ai.runningState = STATE_NOT_RUNNING;
    }
}

void SecurityManagerImpl::OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                                const SecurityInfo* newSecInfo)
{
    if ((NULL == newSecInfo) && (NULL == oldSecInfo)) {
        QCC_LogError(ER_FAIL, ("Both OnSecurityStateChange args are NULL!"));
        return;
    }

    qcc::ECCPublicKey pubKey =
        (NULL != newSecInfo) ? newSecInfo->publicKey : oldSecInfo->publicKey;
    bool exist;
    ApplicationInfoMap::iterator foundAppItr = SafeAppExist(pubKey, exist);

    if (exist) {
        ApplicationInfo old(foundAppItr->second);
        if (NULL != newSecInfo) {
            // update of known application
            AddSecurityInfo(foundAppItr->second, *newSecInfo);
            AddAboutInfo(foundAppItr->second);
            NotifyApplicationListeners(&old, &foundAppItr->second);
        } else {
            // removal of known application
            RemoveSecurityInfo(foundAppItr->second, *oldSecInfo);
            NotifyApplicationListeners(&old, &foundAppItr->second);
        }
    } else {
        if (NULL == newSecInfo) {
            // removal of unknown application
            return;
        }
        // add new application
        ApplicationInfo info;
        AddSecurityInfo(info, *newSecInfo);
        AddAboutInfo(info);

        appsMutex.Lock(__FILE__, __LINE__);
        applications[info.publicKey] = info;
        appsMutex.Unlock(__FILE__, __LINE__);

        NotifyApplicationListeners(NULL, &info);
    }
}

QStatus SecurityManagerImpl::UpdateIdentity(const ApplicationInfo& appInfo, const IdentityInfo& id)
{
    qcc::IdentityCertificate idCertificate;
    QStatus status = ER_FAIL;
    do {
        //Sanity check. Make sure we use our internal collected data. Don't trust what is passed to us.
        ApplicationInfo app;
        bool exist;
        ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);

        if (!exist) {
            QCC_LogError(ER_FAIL, ("App does not exist."));
            break;
        }
        app = appItr->second;

        //Check Identity;
        IdentityInfo idInfo = id;

        status = storage->GetIdentity(idInfo);

        if (status != ER_OK) {
            QCC_LogError(status, ("Identity Not found. guid = '%s'", id.guid.ToString().c_str()));
            break;
        }

        ManagedApplicationInfo mgdAppInfo;
        mgdAppInfo.publicKey = app.publicKey;
        if (ER_OK != (status = storage->GetManagedApplication(mgdAppInfo))) {
            QCC_LogError(status, ("Could not find persisted application"));
            return status;
        }

        const PermissionPolicy::Rule* manifest;
        size_t manifestSize;
        status = DeserializeManifest(mgdAppInfo, &manifest, &manifestSize);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not deserialize persisted manifest"));
            return status;
        }

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        PermissionMgmtObj::GenerateManifestDigest(*busAttachment, manifest,
                                                  manifestSize, digest, Crypto_SHA256::DIGEST_SIZE);
        idCertificate.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);

        if (ER_OK != (status = GenerateIdentityCertificate(idCertificate, idInfo, app))) {
            QCC_LogError(status, ("Failed to get identity certificate"));
            break;
        }

        if (ER_OK != (status = storage->StoreCertificate(idCertificate, true))) {
            QCC_LogError(status,
                         ("Failed to persist identity certificate"));
            break;
        }

        applicationUpdater->UpdateApplication(app);
    } while (0);

    return status;
}

const ECCPublicKey& SecurityManagerImpl::GetPublicKey() const
{
    return pubKey;
}

std::vector<ApplicationInfo> SecurityManagerImpl::GetApplications(ajn::PermissionConfigurator::ClaimableState acs)
const
{
    std::vector<ApplicationInfo> apps;
    ApplicationInfoMap::const_iterator appItr;

    appsMutex.Lock(__FILE__, __LINE__);

    for (appItr = applications.begin(); appItr != applications.end();
         ++appItr) {
        const ApplicationInfo& appInfo = appItr->second;

        if (ajn::PermissionConfigurator::STATE_UNKNOWN != acs) {
            if (appItr->second.claimState == acs) {
                apps.push_back(appInfo);
            }
        } else {
            apps.push_back(appInfo);
        }
    }

    appsMutex.Unlock(__FILE__, __LINE__);
    return apps;
}

void SecurityManagerImpl::RegisterApplicationListener(ApplicationListener* al)
{
    if (NULL != al) {
        applicationListenersMutex.Lock(__FILE__, __LINE__);
        listeners.push_back(al);
        applicationListenersMutex.Unlock(__FILE__, __LINE__);
    }
}

void SecurityManagerImpl::UnregisterApplicationListener(ApplicationListener* al)
{
    applicationListenersMutex.Lock(__FILE__, __LINE__);
    std::vector<ApplicationListener*>::iterator it = std::find(
        listeners.begin(), listeners.end(), al);
    if (listeners.end() != it) {
        listeners.erase(it);
    }
    applicationListenersMutex.Unlock(__FILE__, __LINE__);
}

QStatus SecurityManagerImpl::GetApplication(ApplicationInfo& ai) const
{
    ApplicationInfoMap::const_iterator appItr;
    QStatus status = ER_END_OF_DATA;

    appsMutex.Lock(__FILE__, __LINE__);

    if (!ai.publicKey.empty() && ((appItr = applications.find(ai.publicKey)) != applications.end())) {
        ai = appItr->second;
        status = ER_OK;
    } else {  //Search based on busName
        for (appItr = applications.begin(); appItr != applications.end();
             ++appItr) {
            if (ai.busName == appItr->second.busName) {
                ai = appItr->second;
                status = ER_OK;
                break;
            }
        }
    }
    appsMutex.Unlock(__FILE__, __LINE__);
    return status;
}

QStatus SecurityManagerImpl::SetApplicationName(ApplicationInfo& appInfo)
{
    QStatus status = ER_OK;
    appsMutex.Lock(__FILE__, __LINE__);

    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unkown application."));
        return ER_FAIL;
    }

    appItr->second.userDefinedName = appInfo.userDefinedName;
    status = PersistApplicationInfo(appItr->second, true);

    appsMutex.Unlock(__FILE__, __LINE__);
    return status;
}

QStatus SecurityManagerImpl::StoreGuild(GuildInfo& guildInfo)
{
    if (guildInfo.authority.empty()) {
        guildInfo.authority = pubKey;
    }

    QStatus status =  storage->StoreGuild(guildInfo);

    return status;
}

QStatus SecurityManagerImpl::RemoveGuild(GuildInfo& guildInfo)
{
    if (guildInfo.authority.empty()) {
        guildInfo.authority = pubKey;
    }

    QStatus status = storage->RemoveGuild(guildInfo);

    return status;
}

QStatus SecurityManagerImpl::GetGuild(GuildInfo& guildInfo) const
{
    if (guildInfo.authority.empty()) {
        guildInfo.authority = pubKey;
    }

    QStatus status =  storage->GetGuild(guildInfo);

    return status;
}

QStatus SecurityManagerImpl::GetGuilds(std::vector<GuildInfo>& guildInfos) const
{
    QStatus status =  storage->GetGuilds(guildInfos);

    return status;
}

QStatus SecurityManagerImpl::StoreIdentity(IdentityInfo& idInfo)
{
    if (idInfo.authority.empty()) {
        idInfo.authority = pubKey;
    }

    QStatus status = storage->StoreIdentity(idInfo);

    return status;
}

QStatus SecurityManagerImpl::RemoveIdentity(IdentityInfo& idInfo)
{
    if (idInfo.authority.empty()) {
        idInfo.authority = pubKey;
    }

    QStatus status = storage->RemoveIdentity(idInfo);

    return status;
}

QStatus SecurityManagerImpl::GetIdentity(IdentityInfo& idInfo) const
{
    if (idInfo.authority.empty()) {
        idInfo.authority = pubKey;
    }

    QStatus status = storage->GetIdentity(idInfo);

    return status;
}

QStatus SecurityManagerImpl::GetIdentities(std::vector<IdentityInfo>& idInfos) const
{
    QStatus status = storage->GetIdentities(idInfos);

    return status;
}

void SecurityManagerImpl::Announced(const char* busName, uint16_t version,
                                    SessionPort port, const MsgArg& objectDescriptionArg,
                                    const MsgArg& aboutDataArg)
{
    QCC_UNUSED(objectDescriptionArg);
    QCC_UNUSED(port);
    QCC_UNUSED(version);

    AboutData aboutData(aboutDataArg);
    char* appName;
    aboutData.GetAppName(&appName);
    char* deviceName;
    aboutData.GetDeviceName(&deviceName);

    QCC_DbgPrintf(("Received About signal!!!"));
    QCC_DbgPrintf(("busName = %s", busName));
    QCC_DbgPrintf(("appName = %s", appName));
    QCC_DbgPrintf(("deviceName = %s", deviceName));

    ApplicationInfoMap::iterator appItr;

    appsMutex.Lock(__FILE__, __LINE__);

    for (appItr = applications.begin(); appItr != applications.end(); ++appItr) {
        if (appItr->second.busName == busName) {
            ApplicationInfo old = appItr->second;
            appItr->second.appName = qcc::String(appName);
            appItr->second.deviceName = qcc::String(deviceName);
            NotifyApplicationListeners(&old, &appItr->second);
            appsMutex.Unlock(__FILE__, __LINE__);
            return;
        }
    }

    ApplicationInfo info;
    info.busName = busName;
    info.appName = qcc::String(appName);
    info.deviceName = qcc::String(deviceName);
    info.runningState = STATE_RUNNING;
    info.claimState = ajn::PermissionConfigurator::STATE_UNKNOWN;
    info.userDefinedName = "";

    appsMutex.Unlock(__FILE__, __LINE__);

    aboutCacheMutex.Lock(__FILE__, __LINE__);
    aboutCache[busName] = info;
    aboutCacheMutex.Unlock(__FILE__, __LINE__);
}

SecurityManagerImpl::ApplicationInfoMap::iterator SecurityManagerImpl::SafeAppExist(const qcc::
                                                                                    ECCPublicKey
                                                                                    pubKey,
                                                                                    bool& exist)
{
    appsMutex.Lock(__FILE__, __LINE__);
    ApplicationInfoMap::iterator ret = applications.find(pubKey);
    exist = (ret != applications.end());
    appsMutex.Unlock(__FILE__, __LINE__);
    return ret;
}

QStatus SecurityManagerImpl::SerializeManifest(ManagedApplicationInfo& managedAppInfo,
                                               const PermissionPolicy::Rule* manifestRules,
                                               size_t manifestRulesCount)
{
    // wrap the manifest in a policy
    PermissionPolicy policy;
    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];
    terms[0].SetRules(manifestRulesCount, const_cast<PermissionPolicy::Rule*>(manifestRules));
    policy.SetTerms(1, terms);

    // serialize wrapped manifest to a byte array
    uint8_t* buf = NULL;
    size_t size;
    Message tmpMsg(*busAttachment);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    QStatus status = policy.Export(marshaller, &buf, &size);
    terms[0].SetRules(0, NULL); /* does not manage the lifetime of the input rules */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to serialize manifest"));
        delete[] buf;
        return status;
    }

    // update managed app info
    managedAppInfo.manifest = qcc::String((const char*)buf, size);

    delete[] buf;
    return status;
}

QStatus SecurityManagerImpl::DeserializeManifest(const ManagedApplicationInfo managedAppInfo,
                                                 const PermissionPolicy::Rule** manifestRules,
                                                 size_t* manifestRulesCount)
{
    // reconstruct policy containing manifest
    Message tmpMsg(*busAttachment);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    PermissionPolicy* policy = NULL;
    PermissionPolicy::Term* terms = NULL;
    QStatus status = ER_FAIL;

    policy = new PermissionPolicy();

    status = policy->Import(marshaller, (const uint8_t*)managedAppInfo.manifest.data(),
                            managedAppInfo.manifest.size());

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to reconstruct manifest from persistency"));
        return status;
    }

    if (policy->GetTermsSize() == 0) {
        QCC_LogError(ER_FAIL, ("Unexpected persisted manifest"));
        return ER_FAIL;
    }

    terms = (PermissionPolicy::Term*)policy->GetTerms();

    status = (terms != NULL ? ER_OK : ER_FAIL);

    *manifestRules = terms[0].GetRules();
    *manifestRulesCount = terms[0].GetRulesSize();
    // avoid rules being freed by policy
    terms[0].SetRules(0, NULL);

    return status;
}

QStatus SecurityManagerImpl::GetManifest(const ApplicationInfo& appInfo,
                                         const PermissionPolicy::Rule** manifestRules,
                                         size_t* manifestRulesCount)
{
    QStatus status = ER_FAIL;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unkown application."));
        return ER_FAIL;
    }
    ApplicationInfo app = appItr->second;
    //fetching persisted manifest if it exists
    ManagedApplicationInfo mgdAppInfo;
    mgdAppInfo.publicKey = app.publicKey;

    if (ER_OK != (status = storage->GetManagedApplication(mgdAppInfo))) {
        QCC_LogError(status, ("Could not find a persisted manifest."));

        return status;
    }

    if (!mgdAppInfo.manifest.empty()) {
        status = DeserializeManifest(mgdAppInfo, manifestRules,
                                     manifestRulesCount);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not get manifest !"));
        }
    } else {
        QCC_DbgHLPrintf(("Empty manifest"));
    }

    return status;
}

QStatus SecurityManagerImpl::GenerateIdentityCertificate(IdentityCertificate& idCert,
                                                         const IdentityInfo& idInfo,
                                                         const ApplicationInfo& appInfo)
{
    qcc::String serialNumber;
    qcc::CertificateX509::ValidPeriod period;
    QStatus status = ER_FAIL;

    status = storage->GetNewSerialNumber(serialNumber);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to get a new serial number"));
        return status;
    }

    idCert.SetAlias(idInfo.name);
    idCert.SetSubjectOU(idInfo.guid.GetBytes(), qcc::GUID128::SIZE);
    idCert.SetSubjectCN(appInfo.peerID.GetBytes(), qcc::GUID128::SIZE);
    idCert.SetSerial(serialNumber);
    period.validFrom = time(NULL) - 3600;
    period.validTo = period.validFrom + 3600 +  3153600;                        // valid for 365 days
    idCert.SetValidity(&period);
    idCert.SetSubjectPublicKey(&appInfo.publicKey);
    //idCert.SetIssuer(&appInfo.publicKey);
    //TODO are all fields set properly?
    if (ER_OK != (status = CertificateGen->GetIdentityCertificate(idCert))) {
        QCC_LogError(status, ("Failed to get identity certificate"));
    }
    return status;
}

QStatus SecurityManagerImpl::PersistPolicy(const ApplicationInfo& appInfo,
                                           PermissionPolicy& policy)
{
    QStatus status = ER_OK;

    ManagedApplicationInfo managedAppInfo;
    managedAppInfo.publicKey = appInfo.publicKey;
    if (ER_OK != (status = storage->GetManagedApplication(managedAppInfo))) {
        QCC_LogError(status,
                     ("Trying to persist a policy for an unmanaged application"));
        return status;
    }

    uint8_t* policyData = NULL;
    size_t policySize;
    Message tmpMsg(*busAttachment);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    if (ER_OK != (status = policy.Export(marshaller, &policyData,
                                         &policySize))) {
        QCC_LogError(status, ("Could not export policy"));
        return status;
    }

    managedAppInfo.policy = qcc::String((const char*)policyData, policySize);

    if (ER_OK != (status = storage->StoreApplication(managedAppInfo, true))) {
        QCC_LogError(status, ("Could not persist policy"));
    }

    delete[] policyData;
    return status;
}

QStatus SecurityManagerImpl::PersistManifest(const ApplicationInfo& appInfo,
                                             const PermissionPolicy::Rule* manifestRules,
                                             size_t manifestRulesCount)
{
    QStatus status = ER_OK;

    if ((manifestRulesCount > 0) && (NULL == manifestRules)) {
        status = ER_FAIL;
        QCC_LogError(status, ("Null manifestRules"));
        return status;
    }

    ManagedApplicationInfo managedAppInfo;
    managedAppInfo.publicKey = appInfo.publicKey;
    if (ER_OK != (status = storage->GetManagedApplication(managedAppInfo))) {
        QCC_LogError(status,
                     ("Trying to persist a manifest for an unmanaged application"));
        return status;
    }

    if (manifestRulesCount > 0) {
        if (ER_OK != (status = SerializeManifest(managedAppInfo, manifestRules, manifestRulesCount))) {
            QCC_LogError(status, ("Failed to serialize manifest"));
            return status;
        }
    }

    if (ER_OK != (status = storage->StoreApplication(managedAppInfo, true))) {
        QCC_LogError(status, ("Could not persist manifest"));
        return status;
    }

    return status;
}

QStatus SecurityManagerImpl::PersistApplicationInfo(const ApplicationInfo& appInfo,
                                                    bool update)
{
    QStatus status = ER_FAIL;

    ManagedApplicationInfo managedAppInfo;
    if (update) {
        managedAppInfo.publicKey = appInfo.publicKey;
        if (ER_OK != (status = storage->GetManagedApplication(managedAppInfo))) {
            QCC_LogError(status,
                         ("Trying to update application info for an unmanaged application"));
            return status;
        }
    }

    managedAppInfo.publicKey = appInfo.publicKey;
    managedAppInfo.appName = appInfo.appName;
    managedAppInfo.deviceName = appInfo.deviceName;
    managedAppInfo.userDefinedName = appInfo.userDefinedName;
    managedAppInfo.peerID = appInfo.peerID.ToString();
    managedAppInfo.updatesPending = appInfo.updatesPending;

    if (ER_OK != (status = storage->StoreApplication(managedAppInfo, update))) {
        QCC_LogError(status, ("Could not persist application info"));
        return status;
    }

    return status;
}

void SecurityManagerImpl::NotifyApplicationListeners(const ApplicationInfo* oldAppInfo,
                                                     const ApplicationInfo* newAppInfo)
{
    queue.AddTask(new AppListenerEvent(oldAppInfo ? new ApplicationInfo(*oldAppInfo) : NULL,
                                       newAppInfo ? new ApplicationInfo(*newAppInfo) : NULL,
                                       NULL));
}

void SecurityManagerImpl::NotifyApplicationListeners(const SyncError* error)
{
    queue.AddTask(new AppListenerEvent(NULL, NULL, error));
}

void SecurityManagerImpl::HandleTask(AppListenerEvent* event)
{
    applicationListenersMutex.Lock(__FILE__, __LINE__);
    if (event->syncError) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnSyncError(event->syncError);
        }
    } else {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnApplicationStateChange(event->oldAppInfo, event->newAppInfo);
        }
    }
    applicationListenersMutex.Unlock(__FILE__, __LINE__);
}

QStatus SecurityManagerImpl::SetUpdatesPending(const ApplicationInfo& appInfo, bool updatesPending)
{
    appsMutex.Lock(__FILE__, __LINE__);

    bool exist;
    ApplicationInfoMap::iterator it = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        appsMutex.Unlock(__FILE__, __LINE__);
        QCC_LogError(ER_FAIL, ("Application does not exist !"));
        return ER_FAIL;
    }

    QStatus status = ER_FAIL;
    ApplicationInfo oldAppInfo = it->second;
    if (oldAppInfo.updatesPending != updatesPending) {
        it->second.updatesPending = updatesPending;

        status = PersistApplicationInfo(it->second, true);
        if (status != ER_OK) {
            QCC_DbgPrintf(("Did not persist application info this time!"));
        }
        NotifyApplicationListeners(&oldAppInfo, &(it->second));
    }

    appsMutex.Unlock(__FILE__, __LINE__);
    return ER_OK;
}

QStatus SecurityManagerImpl::GetApplicationSecInfo(SecurityInfo& secInfo) const
{
    return appMonitor->GetApplication(secInfo);
}
}
}
#undef QCC_MODULE
