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

#include <ctime>
#include <alljoyn/version.h>
#include <alljoyn/Session.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <qcc/Debug.h>
#include <qcc/CertificateECC.h>
#include <qcc/KeyInfoECC.h>
#include <CredentialAccessor.h> // still in alljoyn_core/src!
#include <PermissionMgmtObj.h> // still in alljoyn_core/src!
#include "SecurityManagerImpl.h"
#include "ApplicationUpdater.h"

#include <Common.h>

#include <iostream>

#include <SecLibDef.h>

#define QCC_MODULE "SEC_MGR"

using namespace ajn::services;

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
        QCC_DbgPrintf(("SecMgr: VerifyCredentials %s", authMechanism));
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer,
                                bool success)
    {
        QCC_DbgPrintf(("SecMgr: AuthenticationComplete '%s' success = %i", authMechanism, success));
    }
};

//No member method as we cannot include CredentialAccessor.h in SecurityMngrImpl.h
static QStatus ClaimSelf(CredentialAccessor& ca,
                         BusAttachment* ba)
{
    qcc::GUID128 localGuid;
    qcc::ECCPublicKey subjectPubKey;
    ca.GetGuid(localGuid);
    ca.GetDSAPublicKey(subjectPubKey);

    qcc::KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&subjectPubKey);
    keyInfo.SetKeyId(localGuid.GetBytes(), qcc::GUID128::SIZE);
    ajn::PermissionMgmtObj::TrustAnchor* anchor = new ajn::PermissionMgmtObj::TrustAnchor(
        ajn::PermissionMgmtObj::TRUST_ANCHOR_ANY,
        keyInfo);

    if (anchor == NULL) {
        QCC_LogError(ER_FAIL, ("Failed to allocate TrustAnchor"));
        return ER_FAIL;
    }

    ajn::PermissionMgmtObj pmo(*ba);
    pmo.InstallTrustAnchor(anchor); //ownership is transferred

    PermissionConfigurator& pcf = ba->GetPermissionConfigurator();
    CertificateX509 x509(CertificateX509::IDENTITY_CERTIFICATE);
    x509.SetSerial("0");
    x509.SetIssuer(localGuid);
    x509.SetSubject(localGuid);
    x509.SetSubjectPublicKey(&subjectPubKey);
    x509.SetAlias("Admin");

    QStatus status =  pcf.SignCertificate(x509);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to sign certificate"));
        return status;
    }
    qcc::String der;
    x509.EncodeCertificateDER(der);
    MsgArg cert;
    cert.Set("(yay)", Certificate::ENCODING_X509_DER, der.size(), der.data());
    status = pmo.StoreIdentityCertificate(cert);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store own identity certificate"));
        return status;
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
    return status;
}

SecurityManagerImpl::SecurityManagerImpl(ajn::BusAttachment* ba,
                                         const Storage* _storage) :
    pubKey(),
    appMonitor(ApplicationMonitor::GetApplicationMonitor(ba,
                                                         "org.allseen.Security.PermissionMgmt.Notification")),
    busAttachment(ba),
    storage((Storage*)_storage),
    queue(TaskQueue<AppInfoEvent*, SecurityManagerImpl>(this)), mfListener(NULL)
{
    CertificateGen = NULL;
    remoteApplicationManager = NULL;
    proxyObjMgr = NULL;
    applicationUpdater = NULL;
}

QStatus SecurityManagerImpl::Init()
{
    SessionOpts opts;
    InterfaceDescription* stubIntf;
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

        CertificateGen = new X509CertificateGenerator(localGuid.ToString(), busAttachment);
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
            ClaimSelf(ca, busAttachment);
        }

        applicationUpdater = new ApplicationUpdater(busAttachment, storage, remoteApplicationManager, localGuid, pubKey);
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

            appsMutex.Lock(__FILE__, __LINE__);
            applications[info.publicKey] = info;
            appsMutex.Unlock(__FILE__, __LINE__);
        }

        status = CreateStubInterface(busAttachment, stubIntf);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create security interface"));
            break;
        }

        if (NULL == appMonitor) {
            QCC_LogError(status, ("NULL Application Monitor"));
            status = ER_FAIL;
            break;
        }
        appMonitor->RegisterSecurityInfoListener(this);
        appMonitor->RegisterSecurityInfoListener(applicationUpdater);

        status = ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler(
            *busAttachment, *this, NULL, 0);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register announce handler"));
            break;
        }
    } while (0);

    return status;
}

SecurityManagerImpl::~SecurityManagerImpl()
{
    // TO DO: only unregister announce handler registered by secmgr
    ajn::services::AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *this, NULL,
                                                                    0);
    appMonitor->UnregisterSecurityInfoListener(applicationUpdater);
    appMonitor->UnregisterSecurityInfoListener(this);

    std::map<qcc::String, PermissionPolicy*>::iterator itr = manifestCache.begin();
    for (; itr != manifestCache.end(); itr++) {
        delete itr->second;
    }

    queue.Stop();

    delete applicationUpdater;
    delete CertificateGen;
    delete proxyObjMgr;
    delete remoteApplicationManager;
    delete appMonitor;
    delete ProxyObjectManager::listener;
    ProxyObjectManager::listener = NULL;
}

QStatus SecurityManagerImpl::CreateStubInterface(ajn::BusAttachment* bus,
                                                 ajn::InterfaceDescription*& intf)
{
    // TODO: remove stub specific code from production code
    qcc::String stubIfn("org.allseen.Security.PermissionMgmt.Stub");
    QStatus status = bus->CreateInterface(stubIfn.c_str(), intf,
                                          AJ_IFC_SECURITY_REQUIRED);
    if (ER_OK != status) {
        QCC_LogError(status,
                     ("Failed to create interface '%s' on security manager bus attachment",
                      stubIfn.c_str()));
        return status;
    }
    intf->AddMethod("Claim",     "(yv)(yay)",  "(yv)", "adminPublicKey,identityCert,publicKey");
    intf->AddMethod("InstallIdentity", "(yay)", NULL, "cert,result", 0);
    intf->AddMethod("GetIdentity",     NULL, "(yay)", "cert");
    intf->AddMethod("InstallMembership", "a(yay)", NULL, "certChain", 0);
    intf->AddMethod("RemoveMembership",     "say", NULL, "serialNum,issuer");
    intf->AddMethod("GetManifest", NULL, "(yv)",  "manifest");
    intf->AddMethod("InstallMembershipAuthData", "say(yv)", NULL, "serialNum,issuer,authorization", 0);
    intf->AddMethod("InstallPolicy", "(yv)",  NULL, "authorization");
    intf->AddMethod("GetPolicy", NULL, "(yv)", "authorization");
    intf->Activate();

    return ER_OK;
}

void SecurityManagerImpl::SetManifestListener(ManifestListener* mfl)
{
    mfListener = mfl;
}

QStatus SecurityManagerImpl::Claim(const ApplicationInfo& appInfo, const IdentityInfo& identityInfo)
{
    QStatus status = ER_FAIL;
    if (mfListener == NULL) {
        QCC_LogError(status, ("No ManifestListener set."));
        return ER_FAIL;
    }

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
        IdentityInfo idInfo = identityInfo;

        status = storage->GetIdentity(idInfo);

        if (status != ER_OK) {
            QCC_LogError(status, ("Identity Not found. guid = '%s'", identityInfo.guid.ToString().c_str()));
            break;
        }

        /*===========================================================
         * Step 1: Claim and install identity certificate
         */

        if (ER_OK != (status = PartialClaim(app, identityInfo))) {
            QCC_LogError(status, ("Could not claim application"));
            break;
        }

        QCC_DbgPrintf(("Application %s was claimed", app.appName.c_str()));

        /*===========================================================
         * Step 2: Retrieve manifest and call accept manifest
         *         callback.
         */

        PermissionPolicy::Rule* manifestRules;
        size_t manifestRulesCount = 0;
        if (ER_OK != (status = remoteApplicationManager->GetManifest(app, &manifestRules, &manifestRulesCount))) {
            QCC_LogError(status, ("Could not retrieve manifest"));
            break;
        }

        if (!mfListener->ApproveManifest(app, manifestRules, manifestRulesCount)) {
            status = Reset(appInfo);
            if (ER_OK == status) {
                status = ER_MANIFEST_REJECTED;
            }
            break;
        }

        /*===========================================================
         * Step 3: Persist claimed app manifest
         **/

        if (ER_OK != (status = PersistApplication(app, true, manifestRules, manifestRulesCount))) {
            QCC_LogError(status, ("Could not persist application's manifest"));
            break;
        }

        QCC_DbgPrintf(("Application %s was persisted", app.appName.c_str()));

        appItr = SafeAppExist(appInfo.publicKey, exist);

        if (!exist) {
            QCC_LogError(ER_FAIL, ("App does not exist"));
            break;
        }

        /* TODO: refactor the lines below */
        ECCPublicKey pk = app.publicKey;
        appsMutex.Lock(__FILE__, __LINE__);
        app = appItr->second;
        app.publicKey = pk;
        applications[app.publicKey] =  app;
        appsMutex.Unlock(__FILE__, __LINE__);

        status = ER_OK;
    } while (0);

    return status;
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

QStatus SecurityManagerImpl::PartialClaim(const ApplicationInfo& appInfo, const IdentityInfo& identityInfo)
{
    QStatus status = ER_OK;

    // Check application
    ApplicationInfo app;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unknown application"));
        return ER_FAIL;
    }
    app = appItr->second;

    // Check identity
    IdentityInfo id = identityInfo;

    status = storage->GetIdentity(id);

    if (status != ER_OK) {
        QCC_LogError(status, ("Unknown identity"));
        return status;
    }

    MsgArg inputs[2];

    if (ER_OK != (status = MarshalPublicKey(&pubKey, localGuid, inputs[0]))) {
        QCC_LogError(status, ("Failed to marshal public key"));
        return status;
    }

    qcc::X509IdentityCertificate idCertificate;
    if (ER_OK != (status = GenerateIdentityCertificate(idCertificate, id, app))) {
        QCC_LogError(status, ("Failed to create IdentityCertificate"));
        return status;
    }

    qcc::String pem = idCertificate.GetDER();
    inputs[1].Set("(yay)", Certificate::ENCODING_X509_DER, pem.size(), pem.data());
    Message reply(*busAttachment);

    fprintf(stderr, "Calling claim ...");
    status = proxyObjMgr->MethodCall(app, "Claim", inputs, 2, reply);
    if (ER_OK != status) {
        // errors logged in MethodCall
        return status;
    }

    ECCPublicKey appPubKey;
    status = UnmarshalPublicKey(reply->GetArg(0), appPubKey);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to unmarshal application public key"));
    }
    QCC_DbgPrintf(("appPubKey = %s...", appPubKey.ToString().substr(0, 20).c_str()));
    if (app.publicKey != appPubKey) {
        status = ER_FAIL;
        QCC_LogError(status, ("Found wrong key in claim response!!!!"));
        app.publicKey = appPubKey;
        return status;
    }

    if (ER_OK == status) {
        status = PersistApplication(app);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not persist application"));
            return status;
        }

        status = storage->StoreCertificate(idCertificate);

        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Failed to persist identity certificate"));
            return status;
        }
    }

    return status;
}

QStatus SecurityManagerImpl::InstallMembership(const ApplicationInfo& appInfo,
                                               const GuildInfo& guildInfo,
                                               const PermissionPolicy* authorizationData)
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

    qcc::X509MemberShipCertificate cert;
    cert.SetGuildId(gi.guid.ToString());
    cert.SetSubject(&app.publicKey);
    PermissionPolicy* data = NULL;
    uint8_t* permPolicyData = NULL;

    // Create marshaller for policies
    Message tmpMsg(*busAttachment);
    DefaultPolicyMarshaller marshaller(tmpMsg);

    do {
        // Check passed on authorizationData
        if ((NULL == authorizationData)) {
            QCC_DbgPrintf(("AuthorizationData is not provided"));
            //Fetch persisted manifest
            //Construct permission policy
            ManagedApplicationInfo mgdApp;
            mgdApp.publicKey = app.publicKey;
            status = storage->GetManagedApplication(mgdApp);

            if (ER_OK != status) {
                QCC_LogError(status,
                             ("Could not get application from storage"));
                break;
            }

            QCC_DbgPrintf(
                ("Retrieved Manifest is: %s", mgdApp.manifest.c_str()));

            const PermissionPolicy::Rule* manifestRules;
            size_t manifestRulesCount;

            status = DeserializeManifest(mgdApp, &manifestRules,
                                         &manifestRulesCount);
            if (ER_OK != status) {
                QCC_LogError(status, ("Could not get manifest !"));
                break;
            }

            PermissionPolicy* manifest = new PermissionPolicy();
            PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];
            terms[0].SetRules(manifestRulesCount,
                              const_cast<PermissionPolicy::Rule*>(manifestRules));
            manifest->SetTerms(1, terms);

            data = manifest;
        } else {         //Use passed on authorization data
            data = const_cast<PermissionPolicy*>(authorizationData);
        }

        if (!data) {
            QCC_LogError(ER_FAIL,
                         ("Null authorization data and no persisted manifest"));
            status = ER_FAIL;
            break;
        }

        /***************************Generate a certificate*****************************/
        cert.SetGuildId(guildInfo.guid.ToString());
        cert.SetDelegate(false);
        cert.SetApplicationID(app.peerID);

        qcc::String serialNumber;

        status = storage->GetNewSerialNumber(serialNumber);

        if (status != ER_OK) {
            QCC_LogError(ER_FAIL, ("Could not get a serial number."));
            break;
        }

        cert.SetSerialNumber(serialNumber);
        qcc::Certificate::ValidPeriod period;
        period.validFrom = time(NULL) - 3600;
        period.validTo = period.validFrom + 3600 +  3153600;    // valid for 365 days
        cert.SetValidity(&period);

        // serialize data to string containing byte array
        qcc::String authData;
        size_t policySize;
        if (ER_OK != (status = data->Export(marshaller, &permPolicyData, &policySize))) {
            QCC_LogError(status, ("Could not export authorization data."));
            break;
        }
        authData = qcc::String((const char*)permPolicyData, policySize);

        // compute digest and set it in certificate
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        data->Digest(marshaller, digest, Crypto_SHA256::DIGEST_SIZE);
        cert.SetDataDigest(qcc::String((const char*)digest, sizeof(digest)));

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

        if (ER_OK != (status = storage->StoreAssociatedData(cert, authData, true))) {     //update if already exists.
            QCC_LogError(ER_FAIL, ("Failed to store authorization data"));
            break;
        }
        /******************************************************************************/

        applicationUpdater->UpdateApplication(app);
    } while (0);

    delete[] permPolicyData;

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

    qcc::X509MemberShipCertificate cert;
    cert.SetGuildId(guildInfo.guid.ToString());
    qcc::ECCPublicKey eccAppPubKey;

    memcpy(eccAppPubKey.x, app.publicKey.x,
           sizeof(eccAppPubKey.x));
    memcpy(eccAppPubKey.y, app.publicKey.y,
           sizeof(eccAppPubKey.y));

    cert.SetSubject(&eccAppPubKey);

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

    status = remoteApplicationManager->RemoveMembership(app, cert.GetSerialNumber(), localGuid);

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
    uint8_t* policyData = NULL;
    size_t policySize;
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

    do {
        ManagedApplicationInfo managedAppInfo;
        managedAppInfo.publicKey = app.publicKey;
        if (ER_OK
            != (status = storage->GetManagedApplication(managedAppInfo))) {
            QCC_LogError(status,
                         ("Trying to persist a policy for an unmanaged application."));
            break;
        }
        Message tmpMsg(*busAttachment);
        DefaultPolicyMarshaller marshaller(tmpMsg);
        if (ER_OK
            != (status = policy.Export(marshaller, &policyData,
                                       &policySize))) {
            QCC_LogError(status, ("Could not export policy from origin."));
            break;
        }

        managedAppInfo.policy = qcc::String((const char*)policyData,
                                            policySize);

        if (ER_OK
            != (status = storage->StoreApplication(managedAppInfo, true))) {                     //update flag set
            QCC_LogError(status, ("Could not persist policy !"));
            break;
        }
        qcc::String print = "Persisted/Updated policy : \n";
        print += policy.ToString();
        QCC_DbgHLPrintf((print.c_str()));

        applicationUpdater->UpdateApplication(app);
    } while (0);

    delete[] policyData;

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
    qcc::X509IdentityCertificate idCertificate;
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

    const PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount = 0;
    if (ER_OK != (status = GetManifest(appItr->second, &manifestRules, &manifestRulesCount))) {
        appsMutex.Unlock(__FILE__, __LINE__);
        return status;
    }

    appItr->second.userDefinedName = appInfo.userDefinedName;
    status = PersistApplication(appItr->second, true, manifestRules, manifestRulesCount);

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

qcc::String SecurityManagerImpl::GetString(::ajn::services::PropertyStoreKey key,
                                           const AboutData& aboutData) const
{
    const qcc::String& keyName =
        ajn::services::AboutPropertyStoreImpl::getPropertyStoreName(key);
    ::ajn::services::AnnounceHandler::AboutData::const_iterator it =
        aboutData.find(keyName);
    if (it == aboutData.end()) {
        QCC_DbgTrace(
            ("Received invalid About data, ignoring '%s", keyName.data()));
        return "";
    }

    const ajn::MsgArg& value = it->second;
    const qcc::String& str = value.v_string.str;

    return str;
}

void SecurityManagerImpl::Announce(unsigned short version, SessionPort port,
                                   const char* busName, const ObjectDescriptions& objectDescs,
                                   const AboutData& aboutData)
{
    QCC_DbgPrintf(("Received About signal!!!"));
    QCC_DbgPrintf(("busName = %s", busName));
    QCC_DbgPrintf(("appID = %s", GetAppId(aboutData).c_str()));
    QCC_DbgPrintf(("appName = %s", GetString(APP_NAME, aboutData).c_str()));
    QCC_DbgPrintf(("deviceName = %s", GetString(DEVICE_NAME, aboutData).c_str()));

    ApplicationInfoMap::iterator appItr;

    appsMutex.Lock(__FILE__, __LINE__);

    for (appItr = applications.begin(); appItr != applications.end(); ++appItr) {
        if (appItr->second.busName == busName) {
            ApplicationInfo old = appItr->second;
            appItr->second.appName = GetString(APP_NAME, aboutData);
            appItr->second.deviceName = GetString(DEVICE_NAME, aboutData);
            NotifyApplicationListeners(&old, &appItr->second);
            appsMutex.Unlock(__FILE__, __LINE__);
            return;
        }
    }

    ApplicationInfo info;
    info.busName = busName;
    info.appName = GetString(APP_NAME, aboutData);
    info.deviceName = GetString(DEVICE_NAME, aboutData);
    info.runningState = STATE_RUNNING;
    info.claimState = ajn::PermissionConfigurator::STATE_UNKNOWN;
    info.userDefinedName = "";

    appsMutex.Unlock(__FILE__, __LINE__);

    aboutCacheMutex.Lock(__FILE__, __LINE__);
    aboutCache[busName] = info;
    aboutCacheMutex.Unlock(__FILE__, __LINE__);
}

qcc::String SecurityManagerImpl::GetAppId(const AboutData& aboutData) const
{
    const qcc::String& keyName = AboutPropertyStoreImpl::getPropertyStoreName(
        APP_ID);
    ::ajn::services::AnnounceHandler::AboutData::const_iterator it =
        aboutData.find(keyName);
    if (it == aboutData.end()) {
        QCC_DbgTrace(("Received invalid About data, ignoring"));
        return "";
    }

    uint8_t* AppIdBuffer;
    size_t numElements;
    const ajn::MsgArg& value = it->second;
    value.Get("ay", &numElements, &AppIdBuffer);

    return ByteArrayToHex(AppIdBuffer, numElements);
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
    PermissionPolicy* policy;
    PermissionPolicy::Term* terms = NULL;
    QStatus status = ER_FAIL;
    bool useCache = false;

    if (manifestCache.find(managedAppInfo.manifest) != manifestCache.end()) {
        QCC_DbgPrintf(("Returning cached manifest"));
        useCache = true;
    } else {
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
        manifestCache[managedAppInfo.manifest] = policy;
    }

    // retrieve manifest from policy
    if (!useCache) {
        terms = (PermissionPolicy::Term*)policy->GetTerms();
    } else {
        terms = (PermissionPolicy::Term*)manifestCache[managedAppInfo.manifest]->GetTerms();
    }

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

QStatus SecurityManagerImpl::GenerateIdentityCertificate(X509IdentityCertificate& idCert,
                                                         const IdentityInfo& idInfo,
                                                         const ApplicationInfo& appInfo)
{
    qcc::String serialNumber;
    qcc::Certificate::ValidPeriod period;
    QStatus status = ER_FAIL;

    status = storage->GetNewSerialNumber(serialNumber);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to get a new serial number"));
        return status;
    }

    idCert.SetAlias(idInfo.guid);
    idCert.SetName(idInfo.name);
    idCert.SetApplicationID(appInfo.peerID);
    idCert.SetSerialNumber(serialNumber);
    period.validFrom = time(NULL) - 3600;
    period.validTo = period.validFrom + 3600 +  3153600;                        // valid for 365 days
    idCert.SetValidity(&period);
    idCert.SetSubject(dynamic_cast<const ECCPublicKey*>(&(appInfo.publicKey)));
    idCert.SetIssuer(dynamic_cast<const ECCPublicKey*>(&(appInfo.publicKey)));
    //TODO are all fields set properly?
    if (ER_OK != (status = CertificateGen->GetIdentityCertificate(idCert))) {
        QCC_LogError(status, ("Failed to get identity certificate"));
    }

    return status;
}

QStatus SecurityManagerImpl::PersistApplication(const ApplicationInfo& appInfo,
                                                bool update,
                                                const PermissionPolicy::Rule* manifestRules,
                                                size_t manifestRulesCount)
{
    QCC_DbgPrintf(("Persisting ApplicationInfo"));
    QStatus status = ER_FAIL;

    if ((manifestRulesCount > 0) && (NULL == manifestRules)) {
        status = ER_FAIL;
        QCC_LogError(status, ("Null manifestRules !"));
        return status;
    }

    ManagedApplicationInfo managedApplicationInfo;

    managedApplicationInfo.publicKey = appInfo.publicKey;
    managedApplicationInfo.appName = appInfo.appName;
    managedApplicationInfo.deviceName = appInfo.deviceName;
    managedApplicationInfo.userDefinedName = appInfo.userDefinedName;

    managedApplicationInfo.peerID = appInfo.peerID.ToString();

    if (manifestRulesCount > 0) {
        if (ER_OK != (status = SerializeManifest(managedApplicationInfo, manifestRules, manifestRulesCount))) {
            QCC_LogError(status, ("Failed to SerializeManifest !"));
            return status;
        }
    }

    status = storage->StoreApplication(managedApplicationInfo, update);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store claimed application info !"));
        return status;
    }

    QCC_DbgPrintf(("ApplicationInfo is now persisted"));
    return status;
}

void SecurityManagerImpl::NotifyApplicationListeners(const ApplicationInfo* oldAppInfo,
                                                     const ApplicationInfo* newAppInfo)
{
    queue.AddTask(new AppInfoEvent(oldAppInfo ? new ApplicationInfo(*oldAppInfo) : NULL,
                                   newAppInfo ? new ApplicationInfo(*newAppInfo) : NULL));
}

void SecurityManagerImpl::HandleTask(AppInfoEvent* event)
{
    applicationListenersMutex.Lock(__FILE__, __LINE__);
    for (size_t i = 0; i < listeners.size(); ++i) {
        listeners[i]->OnApplicationStateChange(event->oldAppInfo, event->newAppInfo);
    }
    applicationListenersMutex.Unlock(__FILE__, __LINE__);
}
}
}
#undef QCC_MODULE
