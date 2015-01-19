/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include "SecurityManagerImpl.h"

#include <Identity.h>
#include <Common.h>
#include <StorageFactory.h>

#include <iostream>

#include <SecLibDef.h>

#define QCC_MODULE "SEC_MGR"
#define DEFAULT_STORAGE_PATH "secmgrstorage.db"
#define AJNKEY_STORE "/.alljoyn_keystore/c_ecdhe.ks"
#define STORAGE_PATH_KEY "STORAGE_PATH"

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

        // only allow ECDHE_NULL sessions for now
        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0 || strcmp(authMechanism, ECDHE_KEYX)) {
            creds.SetExpiration(100);             /* set the master secret expiry time to 100 seconds */
            return true;
        }
        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer,
                           const Credentials& creds)
    {
        QCC_DbgPrintf(("VerifyCredentials %s", authMechanism));
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer,
                                bool success)
    {
        QCC_DbgPrintf(("AuthenticationComplete %s", authMechanism));
    }
};

SecurityManagerImpl::SecurityManagerImpl(IdentityData* id,
                                         ajn::BusAttachment* ba,
                                         const qcc::ECCPublicKey& _pubKey,
                                         const qcc::ECCPrivateKey& privKey,
                                         const StorageConfig& _storageCfg,
                                         const SecurityManagerConfig& smCfg) :
    status(ER_OK), id(id), privKey(privKey), pubKey(_pubKey), storageCfg(_storageCfg),
    appMonitor(ApplicationMonitor::GetApplicationMonitor(ba, smCfg.pmNotificationIfn)), busAttachment(ba), config(smCfg)
{
    SessionOpts opts;
    InterfaceDescription* stubIntf;
    qcc::String storagePath(DEFAULT_STORAGE_PATH);
    qcc::Crypto_ECC* ecc = NULL;

    CertificateGen = NULL;
    proxyObjMgr = NULL;

    do {
        if (NULL == ba) {
            QCC_LogError(ER_FAIL, ("Null bus attachment."));
            status = ER_FAIL;
            break;
        }

        ecc = new qcc::Crypto_ECC();
        if (ecc != NULL) {
            ecc->SetDSAPrivateKey(&privKey);
            ecc->SetDSAPublicKey(&pubKey);
        } else {
            status = ER_FAIL;
            break;
        }

        // TODO For now only ALLJOYN_ECDHE_NULL sessions are enabled on the bus
        status = ba->EnablePeerSecurity(ECDHE_KEYX " " KEYX_ECDHE_NULL, new ECDHEKeyXListener(),
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

        CertificateGen = new X509CertificateGenerator(
            localGuid.ToString(), ecc);
        proxyObjMgr = new ProxyObjectManager(ba, config);

        if ((NULL == CertificateGen) || (NULL == proxyObjMgr)) {
            QCC_LogError(ER_FAIL,
                         ("Could not create certificate generator or proxy object manager !"));
            status = ER_FAIL;
            delete ecc;
            break;
        }

        if (storageCfg.settings.find(STORAGE_PATH_KEY)
            != storageCfg.settings.end()) {
            if (storageCfg.settings.at(STORAGE_PATH_KEY).empty()) {
                storageCfg.settings[STORAGE_PATH_KEY] = storagePath;
            }
        } else {
            storageCfg.settings[STORAGE_PATH_KEY] = storagePath;
        }

        QCC_DbgHLPrintf(("STORAGE PATH IS : %s", (storageCfg.settings.find(STORAGE_PATH_KEY)->second).c_str()));

        StorageFactory& sf = StorageFactory::GetInstance();
        storage = sf.GetStorage(storageCfg);

        if (NULL == storage) {
            QCC_LogError(ER_FAIL, ("Failed to create storage means."));
            status = ER_FAIL;
            break;
        }

        std::vector<ManagedApplicationInfo> managedApplications;
        storageMutex.Lock(__FILE__, __LINE__);
        status = storage->GetManagedApplications(managedApplications);
        if (ER_OK != status) {
            QCC_LogError(ER_FAIL, ("Could not get managed applications."));
            break;
        }

        storageMutex.Unlock(__FILE__, __LINE__);
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

        status = ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler(
            *ba, *this, NULL, 0);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register announce handler"));
            break;
        }

        status = ER_OK;
    } while (0);
}

SecurityManagerImpl::~SecurityManagerImpl()
{
    // TO DO: only unregister announce handler registered by secmgr
    ajn::services::AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *this, NULL,
                                                                    0);
    appMonitor->UnregisterSecurityInfoListener(this);

    std::map<qcc::String, PermissionPolicy*>::iterator itr = manifestCache.begin();
    for (; itr != manifestCache.end(); itr++) {
        delete itr->second;
    }

    delete CertificateGen;
    delete proxyObjMgr;
    delete appMonitor;
    delete storage;
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
    intf->AddMethod("Claim",     "(yv)ay(yay)",  "(yv)", "adminPublicKey,GUID,identityCert,publicKey");
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

QStatus SecurityManagerImpl::GetStatus() const
{
    return status;
}

QStatus SecurityManagerImpl::ClaimApplication(const ApplicationInfo& appInfo, const IdentityInfo& identityInfo,
                                              AcceptManifestCB amcb, void* cookie = NULL)
{
    QStatus funcStatus = ER_FAIL;

    if (!amcb) {
        return ER_FAIL;
    }

    if (ER_OK != status) {
        return status;
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
        storageMutex.Lock(__FILE__, __LINE__);
        status = storage->GetIdentity(idInfo);
        storageMutex.Unlock(__FILE__, __LINE__);
        if (status != ER_OK) {
            QCC_LogError(status, ("Identity Not found. guid = '%s'", identityInfo.guid.ToString().c_str()));
            break;
        }

        /*===========================================================
         * Step 1: Claim and install identity certificate
         */

        if (ER_OK != (funcStatus = Claim(app, identityInfo))) {
            QCC_LogError(funcStatus, ("Could not claim application"));
            break;
        }

        /*===========================================================
         * Step 2: Retrieve manifest and call accept manifest
         *         callback.
         */

        PermissionPolicy::Rule* manifestRules;
        size_t manifestRulesCount = 0;
        if (ER_OK != (funcStatus = GetManifest(app, &manifestRules, &manifestRulesCount))) {
            QCC_LogError(funcStatus, ("Could not retrieve manifest"));
            break;
        }

        if (!amcb(appInfo, manifestRules, manifestRulesCount, cookie)) {
            break;
        }

        /*===========================================================
         * Step 3: Persist claimed app data
         **/

        if (ER_OK != (funcStatus = PersistManifest(app, manifestRules, manifestRulesCount))) {
            QCC_LogError(funcStatus, ("Could not persist application"));
            break;
        }

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

        funcStatus = ER_OK;
    } while (0);

    return funcStatus;
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

QStatus SecurityManagerImpl::Claim(ApplicationInfo& appInfo, const IdentityInfo& identityInfo)
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
    storageMutex.Lock(__FILE__, __LINE__);
    status = storage->GetIdentity(id);
    storageMutex.Unlock(__FILE__, __LINE__);
    if (status != ER_OK) {
        QCC_LogError(status, ("Unknown identity"));
        return status;
    }

    MsgArg inputs[3];

    if (ER_OK != (status = MarshalPublicKey(&pubKey, localGuid, inputs[0]))) {
        QCC_LogError(status, ("Failed to marshal public key"));
        return status;
    }

    inputs[1].Set("ay", GUID128::SIZE, app.peerID.GetBytes());
    qcc::X509IdentityCertificate idCertificate;
    if (ER_OK != (status = GetIdentityCertificate(idCertificate, id, app))) {
        QCC_LogError(status, ("Failed to create IdentityCertificate"));
        return status;
    }
    qcc::String pem = idCertificate.GetPEM();
    inputs[2].Set("(yay)", Certificate::ENCODING_X509_DER_PEM, pem.size(), pem.data());

    Message reply(*busAttachment);
    status = proxyObjMgr->MethodCall(app, "Claim", inputs, 3, reply);
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
        storageMutex.Lock(__FILE__, __LINE__);
        status = PersistApplication(app);
        if (ER_OK != status) {
            storageMutex.Unlock(__FILE__, __LINE__);
            QCC_LogError(status, ("Could not persist application"));
            return status;
        }

        status = storage->StoreCertificate(idCertificate);
        storageMutex.Unlock(__FILE__, __LINE__);
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
    QStatus funcStatus = ER_FAIL;
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

    storageMutex.Lock(__FILE__, __LINE__);
    funcStatus = storage->GetGuild(gi);
    storageMutex.Unlock(__FILE__, __LINE__);

    if (funcStatus != ER_OK) {
        QCC_LogError(funcStatus, ("Unknown guild"));
        return funcStatus;
    }

    qcc::X509MemberShipCertificate cert;
    cert.SetGuildId(gi.guid.ToString());
    cert.SetSubject(&app.publicKey);
    PermissionPolicy* data = NULL;
    uint8_t* policyData = NULL;

    // Create marshaller for policies
    Message tmpMsg(*busAttachment);
    DefaultPolicyMarshaller marshaller(tmpMsg);

    storageMutex.Lock(__FILE__, __LINE__);
    do {
        // Check passed on authorizationData
        if ((NULL == authorizationData)) {
            QCC_DbgPrintf(("AuthorizationData is not provided"));
            //Fetch persisted manifest
            //Construct permission policy
            ManagedApplicationInfo mgdApp;
            mgdApp.publicKey = app.publicKey;
            funcStatus = storage->GetManagedApplication(mgdApp);

            if (ER_OK != funcStatus) {
                QCC_LogError(funcStatus,
                             ("Could not get application from storage"));
                break;
            }

            QCC_DbgPrintf(
                ("Retrieved Manifest is: %s", mgdApp.manifest.c_str()));

            const PermissionPolicy::Rule* manifestRules;
            size_t manifestRulesCount;

            funcStatus = DeserializeManifest(mgdApp, &manifestRules,
                                             &manifestRulesCount);
            if (ER_OK != funcStatus) {
                QCC_LogError(funcStatus, ("Could not get manifest !"));
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
            funcStatus = ER_FAIL;
            break;
        }

        /*********Generate a certificate***************************/
        cert.SetGuildId(guildInfo.guid.ToString());
        cert.SetDelegate(false);
        cert.SetApplicationID(app.peerID);

        qcc::String serialNumber;

        funcStatus = storage->GetNewSerialNumber(serialNumber);

        if (funcStatus != ER_OK) {
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
        if (ER_OK != (funcStatus = data->Export(marshaller, &policyData, &policySize))) {
            QCC_LogError(funcStatus, ("Could not export authorization data."));
            break;
        }
        authData = qcc::String((const char*)policyData, policySize);

        // compute digest and set it in certificate
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        data->Digest(marshaller, digest, Crypto_SHA256::DIGEST_SIZE);
        cert.SetDataDigest(qcc::String((const char*)digest, sizeof(digest)));

        funcStatus = CertificateGen->GenerateMembershipCertificate(cert);

        if (funcStatus != ER_OK) {
            QCC_DbgRemoteError(("Failed to generate membership certificate"));
            break;
        }
        /**********************************************************/

        /**************Install generated certificate***************/

        ajn::MsgArg inputs[1];
        qcc::String pem = cert.GetPEM();
        inputs[0].Set("(yay)", Certificate::ENCODING_X509_DER_PEM, pem.length(),
                      pem.data());
        ajn::MsgArg arg("a(yay)", 1, inputs);

        Message replyMsg(*busAttachment);
        funcStatus = proxyObjMgr->MethodCall(app, "InstallMembership", &arg, 1, replyMsg);
        if (funcStatus != ER_OK) {
            // errors logged by MethodCall
            break;
        }

        MsgArg args[3];
        args[0].Set("s", cert.GetSerialNumber().c_str());
        args[1].Set("ay", GUID128::SIZE, localGuid.GetBytes());
        data->Export(args[2]);
        Message replyMsg1(*busAttachment);
        funcStatus = proxyObjMgr->MethodCall(app, "InstallMembershipAuthData", args, 3, replyMsg1);
        if (funcStatus != ER_OK) {
            // errors logged by MethodCall
            break;
        }

        /******************************************************************************/

        /************Persist the certificate and the authorization data****************/

        if (ER_OK != (funcStatus = storage->StoreCertificate(cert, true))) { //update if already exists.
            QCC_LogError(funcStatus, ("Failed to store membership certificate"));
            break;
        }

        if (ER_OK != (funcStatus = storage->StoreAssociatedData(cert, authData, true))) { //update if already exists.
            QCC_LogError(ER_FAIL, ("Failed to store authorization data"));
            break;
        }
        /******************************************************************************/
    } while (0);

    storageMutex.Unlock(__FILE__, __LINE__);
    delete[] policyData;

    return funcStatus;
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

    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->GetCertificate(cert);
    storageMutex.Unlock(__FILE__, __LINE__);

    if (funcStatus != ER_OK) {
        QCC_DbgRemoteError(
            ("Could not retrieve certificate %d", funcStatus));
        return funcStatus;
    }
    storageMutex.Lock(__FILE__, __LINE__);
    funcStatus = storage->RemoveCertificate(cert);
    storageMutex.Unlock(__FILE__, __LINE__);
    if (funcStatus != ER_OK) {
        QCC_DbgRemoteError(
            ("Could not remove certificate %d", funcStatus));
        return funcStatus;
    }

    MsgArg args[2];
    Message replyMsg(*busAttachment);
    const char* serial = cert.GetSerialNumber().c_str();
    args[0].Set("s", serial);
    args[1].Set("ay", GUID128::SIZE, localGuid.GetBytes());
    QCC_DbgPrintf(("Removing membership certificate with serial number %s", serial));
    funcStatus = proxyObjMgr->MethodCall(app, "RemoveMembership", args, 2, replyMsg);

    return funcStatus;
}

QStatus SecurityManagerImpl::InstallPolicy(const ApplicationInfo& appInfo,
                                           PermissionPolicy& policy)
{
    QStatus funcStatus = ER_OK;
    uint8_t* policyData = NULL;
    size_t policySize;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);

    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unkown application."));
        return ER_FAIL;
    }

    ApplicationInfo app = appItr->second;
    storageMutex.Lock(__FILE__, __LINE__);

    do {
        MsgArg msgArg;
        funcStatus = policy.Export(msgArg);
        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to GeneratePolicyArgs."));
            break;
        }
        Message replyMsg(*busAttachment);
        funcStatus = proxyObjMgr->MethodCall(app, "InstallPolicy", &msgArg, 1, replyMsg);
        if (ER_OK != funcStatus) {
            break;
        }
        // Persisting the policy on successful installation on remote app

        ManagedApplicationInfo managedAppInfo;
        managedAppInfo.publicKey = app.publicKey;
        if (ER_OK
            != (funcStatus = storage->GetManagedApplication(managedAppInfo))) {
            QCC_LogError(funcStatus,
                         ("Trying to persist a policy for an unmanaged application."));
            break;
        }
        Message tmpMsg(*busAttachment);
        DefaultPolicyMarshaller marshaller(tmpMsg);
        if (ER_OK
            != (funcStatus = policy.Export(marshaller, &policyData,
                                           &policySize))) {
            QCC_LogError(funcStatus, ("Could not export policy from origin."));
            break;
        }

        managedAppInfo.policy = qcc::String((const char*)policyData,
                                            policySize);

        if (ER_OK
            != (funcStatus = storage->StoreApplication(managedAppInfo, true))) {                     //update flag set
            QCC_LogError(funcStatus, ("Could not persist policy !"));
            break;
        }
        qcc::String print = "Persisted/Updated policy : \n";
        print += policy.ToString();
        QCC_DbgHLPrintf((print.c_str()));
    } while (0);

    delete[] policyData;
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::GetPolicy(const ApplicationInfo& appInfo,
                                       PermissionPolicy& policy, bool remote)
{
    QStatus status = ER_FAIL;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unkown application."));
        return ER_FAIL;
    }
    ApplicationInfo app = appItr->second;

    if (remote) {
        Message replyMsg(*busAttachment);
        QStatus methodStatus;
        methodStatus =
            proxyObjMgr->MethodCall(app, "GetPolicy", NULL, 0, replyMsg);

        if (ER_OK != methodStatus) {
            // errors logged in MethodCall
            return methodStatus;
        }

        uint8_t version;
        MsgArg* variant;
        replyMsg->GetArg(0)->Get("(yv)", &version, &variant);

        if (ER_OK != (status = policy.Import(version, *variant))) {
            QCC_LogError(
                status, ("Could not build policy of remote application."));
        }
    } else {
        //fetching persisted policy if it exists
        ManagedApplicationInfo mgdAppInfo;
        mgdAppInfo.publicKey = app.publicKey;
        storageMutex.Lock(__FILE__, __LINE__);
        if (ER_OK != (status = storage->GetManagedApplication(mgdAppInfo))) {
            QCC_LogError(status, ("Could not find a persisted policy."));
            storageMutex.Unlock(__FILE__, __LINE__);
            return status;
        }

        if (!mgdAppInfo.policy.empty()) {
            Message tmpMsg(*busAttachment);
            DefaultPolicyMarshaller marshaller(tmpMsg);
            if (ER_OK !=
                (status =
                     policy.Import(marshaller, (const uint8_t*)mgdAppInfo.policy.data(),
                                   mgdAppInfo.policy.size()))) {
                QCC_LogError(status, ("Could not import policy to target."));
            }
        } else {
            QCC_DbgHLPrintf(("Empty policy"));
        }
        storageMutex.Unlock(__FILE__, __LINE__);
    }

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

    Message replyMsg(*busAttachment);
    if (ER_OK != (status = proxyObjMgr->MethodCall(app, "Reset", NULL, 0, replyMsg))) {
        // errors logged in MethodCall
        return status;
    }

    ManagedApplicationInfo managedApplicationInfo;
    managedApplicationInfo.publicKey = app.publicKey;
    if (ER_OK != (status = storage->RemoveApplication(managedApplicationInfo))) {
        QCC_LogError(status, ("Failed to remove application from storage"));
        return status;
    }

    return status;
}

// no real constructor for ApplicationInfo as to not expose SecurityInfo in public API
void SecurityManagerImpl::CopySecurityInfo(ApplicationInfo& ai, const SecurityInfo& si)
{
    ai.busName = si.busName;
    ai.runningState = si.runningState;
    ai.claimState = si.claimState;
    ai.publicKey = si.publicKey;
    ai.policySerialNum = si.policySerialNum;
}

void SecurityManagerImpl::OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                                const SecurityInfo* newSecInfo)
{
    if ((NULL == oldSecInfo) || (NULL == newSecInfo)) {
        QCC_LogError(ER_FAIL, ("NULL args!"));
        return;
    }

    std::map<qcc::String, ApplicationInfo>::iterator appCachedAboutInfoItr;
    bool exist;
    ApplicationInfoMap::iterator foundAppItr = SafeAppExist(newSecInfo->publicKey, exist);

    if (exist) {
        // We already know this application
        ApplicationInfo old(foundAppItr->second);
        CopySecurityInfo(foundAppItr->second, *newSecInfo);

        aboutCacheMutex.Lock(__FILE__, __LINE__);
        appCachedAboutInfoItr = aboutCache.find(newSecInfo->busName);
        if (appCachedAboutInfoItr != aboutCache.end()) {
            foundAppItr->second.appName = appCachedAboutInfoItr->second.appName;
            foundAppItr->second.deviceName =
                appCachedAboutInfoItr->second.deviceName;
            aboutCache.erase(newSecInfo->busName);
        }
        aboutCacheMutex.Unlock(__FILE__, __LINE__);
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnApplicationStateChange(&old, &foundAppItr->second);
        }
    } else {
        // New application
        ApplicationInfo old;
        CopySecurityInfo(old, *oldSecInfo);

        ApplicationInfo info;
        CopySecurityInfo(info, *newSecInfo);

        aboutCacheMutex.Lock(__FILE__, __LINE__);
        appCachedAboutInfoItr = aboutCache.find(newSecInfo->busName);
        if (appCachedAboutInfoItr != aboutCache.end()) {
            info.appName = appCachedAboutInfoItr->second.appName;
            info.deviceName = appCachedAboutInfoItr->second.deviceName;
            aboutCache.erase(newSecInfo->busName);
        }
        aboutCacheMutex.Unlock(__FILE__, __LINE__);

        appsMutex.Lock(__FILE__, __LINE__);
        applications[info.publicKey] = info;
        appsMutex.Unlock(__FILE__, __LINE__);

        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnApplicationStateChange(&old, &info);
        }
    }
}

QStatus SecurityManagerImpl::InstallIdentity(const ApplicationInfo& appInfo, const IdentityInfo& id)
{
    qcc::X509IdentityCertificate idCertificate;
    QStatus funcStatus = ER_FAIL;
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
        storageMutex.Lock(__FILE__, __LINE__);
        status = storage->GetIdentity(idInfo);
        storageMutex.Unlock(__FILE__, __LINE__);
        if (status != ER_OK) {
            QCC_LogError(status, ("Identity Not found. guid = '%s'", id.guid.ToString().c_str()));
            break;
        }
        if (ER_OK != (funcStatus = GetIdentityCertificate(idCertificate, idInfo, app))) {
            QCC_LogError(funcStatus, ("Failed to get identity certificate"));
            break;
        }

        if (ER_OK !=
            (funcStatus = InstallIdentityCertificate(idCertificate, app))) {
            QCC_LogError(funcStatus,
                         ("Failed to install identity certificate on remote application"));
            break;
        }
    } while (0);

    return funcStatus;
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
        listeners.push_back(al);
        ApplicationInfoMap::const_iterator appItr;
        for (appItr = applications.begin(); appItr != applications.end();
             ++appItr) {
            const ApplicationInfo& appInfo = appItr->second;
            al->OnApplicationStateChange(&appInfo, &appInfo);
        }
    }
}

void SecurityManagerImpl::UnregisterApplicationListener(ApplicationListener* al)
{
    std::vector<ApplicationListener*>::iterator it = std::find(
        listeners.begin(), listeners.end(), al);
    if (listeners.end() != it) {
        listeners.erase(it);
    }
}

QStatus SecurityManagerImpl::GetApplication(ApplicationInfo& ai) const
{
    ApplicationInfoMap::const_iterator appItr;
    QStatus funcStatus = ER_OK;

    appsMutex.Lock(__FILE__, __LINE__);
    for (appItr = applications.begin(); appItr != applications.end();
         ++appItr) {
        if (ai.busName == appItr->second.busName) {
            ai = ApplicationInfo(appItr->second);
            funcStatus = ER_OK;
        }
    }

    appsMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::StoreGuild(const GuildInfo& guildInfo, const bool update)
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus =  storage->StoreGuild(guildInfo, update);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::RemoveGuild(const GUID128& guildId)
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->RemoveGuild(guildId);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::GetGuild(GuildInfo& guildInfo) const
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus =  storage->GetGuild(guildInfo);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus =  storage->GetManagedGuilds(guildsInfo);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::StoreIdentity(const IdentityInfo& identityInfo,
                                           const bool update)
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->StoreIdentity(identityInfo, update);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::RemoveIdentity(const GUID128& idId)
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->RemoveIdentity(idId);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::GetIdentity(IdentityInfo& idInfo) const
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->GetIdentity(idInfo);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SecurityManagerImpl::GetManagedIdentities(std::vector<IdentityInfo>& identityInfos) const
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->GetManagedIdentities(identityInfos);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
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
            for (size_t i = 0; i < listeners.size(); ++i) {
                listeners[i]->OnApplicationStateChange(&old, &(appItr->second));
            }
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
    PermissionPolicy::Term* terms;

    bool useCache = false;

    if (manifestCache.find(managedAppInfo.manifest) != manifestCache.end()) {
        QCC_DbgPrintf(("Returning cached manifest"));
        useCache = true;
    } else {
        policy = new PermissionPolicy();

        QStatus status = policy->Import(marshaller, (const uint8_t*)managedAppInfo.manifest.data(),
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

    *manifestRules = terms[0].GetRules();
    *manifestRulesCount = terms[0].GetRulesSize();
    // avoid rules being freed by policy
    terms[0].SetRules(0, NULL);

    return status;
}

QStatus SecurityManagerImpl::GetManifest(const ApplicationInfo& appInfo,
                                         PermissionPolicy::Rule** manifestRules,
                                         size_t* manifestRulesCount)
{
    QStatus status = ER_FAIL;

    if ((NULL == manifestRules) || (NULL == manifestRulesCount)) {
        QCC_LogError(status, ("Null argument"));
        return status;
    }

    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("App does not exist."));
        return ER_FAIL;
    }
    ApplicationInfo app = appItr->second;

    QCC_DbgPrintf(("Retrieving manifest of remote app..."));
    ajn::Message reply(*busAttachment);
    status = proxyObjMgr->MethodCall(app, "GetManifest", NULL, 0, reply);

    uint8_t type;
    MsgArg* variant;
    status = reply->GetArg(0)->Get("(yv)", &type, &variant);
    if (ER_OK != status) {
        return status;
    }

    return PermissionPolicy::ParseRules(*variant, manifestRules, manifestRulesCount);
}

QStatus SecurityManagerImpl::GetIdentityCertificate(X509IdentityCertificate& idCert,
                                                    const IdentityInfo& idInfo,
                                                    const ApplicationInfo& appInfo)
{
    qcc::String serialNumber;
    qcc::Certificate::ValidPeriod period;
    QStatus funcStatus = ER_FAIL;

    storageMutex.Lock(__FILE__, __LINE__);
    funcStatus = storage->GetNewSerialNumber(serialNumber);
    storageMutex.Unlock(__FILE__, __LINE__);

    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to get a new serial number"));
        return funcStatus;
    }

    idCert.SetAlias(idInfo.guid);
    idCert.SetName(idInfo.name);
    idCert.SetApplicationID(appInfo.peerID);
    idCert.SetSerialNumber(serialNumber);
    period.validFrom = time(NULL) - 3600;
    period.validTo = period.validFrom + 3600 +  3153600;                        // valid for 365 days
    idCert.SetValidity(&period);
    idCert.SetSubject(dynamic_cast<const ECCPublicKey*>(&(appInfo.publicKey)));
    //TODO are all fields set properly?
    if (ER_OK != (funcStatus = CertificateGen->GetIdentityCertificate(idCert))) {
        QCC_LogError(funcStatus, ("Failed to get identity certificate"));
    }

    return funcStatus;
}

QStatus SecurityManagerImpl::InstallIdentityCertificate(X509IdentityCertificate& idCert,
                                                        const ApplicationInfo& app)
{
    QStatus funcStatus = ER_FAIL;

    QCC_DbgPrintf(("Sending PEM of identity certificate: %s", idCert.GetPEM().c_str()));

    Message pemReply(*busAttachment);
    qcc::String pem = idCert.GetPEM();
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER_PEM, pem.size(), pem.data());

    funcStatus = proxyObjMgr->MethodCall(app, "InstallIdentity", &arg, 1, pemReply);
    if (ER_OK != funcStatus) {
        QCC_DbgRemoteError(
            (
                "Remote app returned an error or timed out when calling the \"InstallIdentity\" function"));
        return funcStatus;
    }
    if (pemReply->GetErrorName()) {
        QCC_DbgRemoteError(
            ("Identity certificate could not be installed on claimed application '%s'", pemReply->GetErrorName()));
        funcStatus = ER_FAIL;
        return funcStatus;
    }

    return funcStatus;
}

QStatus SecurityManagerImpl::GetRemoteIdentityCertificate(const ApplicationInfo& appInfo,
                                                          IdentityCertificate& idCert)
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

    do {
        Message reply(*busAttachment);
        status = proxyObjMgr->MethodCall(app, "GetIdentity", NULL, 0, reply);
        if (ER_OK != status) {
            // errors logged in MethodCall
            break;
        }

        if (reply->GetErrorName()) {
            QCC_DbgRemoteError(
                ("Identity certificate could not be retrieved from application '%s'", reply->GetErrorName()));
            status = ER_FAIL;
            break;
        }
        uint8_t encoding;
        uint8_t* encoded;
        size_t encodedLen;
        QStatus status = reply->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
        if (ER_OK != status) {
            break;
        }
        if (encoding != Certificate::ENCODING_X509_DER) {
            status = ER_FAIL;
            QCC_LogError(status, ("Unknown/Unsupported encoding received %d", encoding));
            break;
        }
        status = idCert.LoadEncoded(encoded, encodedLen);
    } while (0);

    return status;
}

QStatus SecurityManagerImpl::PersistManifest(const ApplicationInfo& appInfo,
                                             const PermissionPolicy::Rule* manifestRules,
                                             size_t manifestRulesCount)
{
    QStatus status = ER_FAIL;
    ManagedApplicationInfo managedApplicationInfo;
    managedApplicationInfo.publicKey = appInfo.publicKey;

    storageMutex.Lock(__FILE__, __LINE__);
    status = storage->GetManagedApplication(managedApplicationInfo);
    if (ER_OK != status) {
        storageMutex.Unlock(__FILE__, __LINE__);
        QCC_LogError(status, ("Application not yet persisted"));
        return status;
    }

    if (ER_OK != (status = SerializeManifest(managedApplicationInfo, manifestRules, manifestRulesCount))) {
        storageMutex.Unlock(__FILE__, __LINE__);
        // errors logged in SerializeManifest
        return status;
    }

    status = storage->StoreApplication(managedApplicationInfo, true);
    storageMutex.Unlock(__FILE__, __LINE__);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not update persisted application info"));
        return status;
    }

    return status;
}

QStatus SecurityManagerImpl::PersistApplication(const ApplicationInfo& appInfo)
{
    QCC_DbgPrintf(("Persisting ApplicationInfo"));
    ManagedApplicationInfo managedApplicationInfo;
    QStatus funcStatus = ER_FAIL;

    managedApplicationInfo.publicKey = appInfo.publicKey;
    managedApplicationInfo.appName = appInfo.appName;
    managedApplicationInfo.deviceName = appInfo.deviceName;
    managedApplicationInfo.userDefinedName = appInfo.userDefinedName;
    managedApplicationInfo.peerID = appInfo.peerID.ToString();

    storageMutex.Lock(__FILE__, __LINE__);
    funcStatus = storage->StoreApplication(managedApplicationInfo);
    storageMutex.Unlock(__FILE__, __LINE__);

    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to store claimed application info !"));
        return funcStatus;
    }

    if (ER_OK != (funcStatus = storage->GetManagedApplication(managedApplicationInfo))) {
        QCC_LogError(funcStatus, ("Claimed application cannot be managed !"));
        return funcStatus;
    }

    QCC_DbgPrintf(("Application is now persistently managed"));
    return funcStatus;
}
}
}
#undef QCC_MODULE
