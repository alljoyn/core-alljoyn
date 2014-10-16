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

#include <SecurityManagerImpl.h>
#include <ctime>
#include <alljoyn/version.h>
#include <alljoyn/Session.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <qcc/Debug.h>
#include <qcc/CertificateECC.h>

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
static const char* KEYX_ECDHE_NULL = "ALLJOYN_ECDHE_NULL";
static const char* ECDHE_KEYX = "ALLJOYN_ECDHE_NULL";     //"ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA";

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
        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
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

SecurityManagerImpl::SecurityManagerImpl(qcc::String userName,             //TODO userName and password don't seem to be used anywhere
                                         qcc::String password,
                                         IdentityData* id,
                                         ajn::BusAttachment* ba,
                                         const qcc::ECCPublicKey& pubKey,
                                         const qcc::ECCPrivateKey& privKey,
                                         const StorageConfig& _storageCfg) :
    status(ER_OK), id(id), privKey(privKey), rot(pubKey), storageCfg(_storageCfg),
    appMonitor(ApplicationMonitor::GetApplicationMonitor(ba)), busAttachment(
        ba)
{
    SessionOpts opts;
    InterfaceDescription* serverintf;
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

        CertificateGen = new X509CertificateGenerator(
            busAttachment->GetGlobalGUIDString(), ecc);
        proxyObjMgr = new ProxyObjectManager(ba);

        if ((NULL == CertificateGen) || (NULL == proxyObjMgr)) {
            QCC_LogError(ER_FAIL, ("Could not create certificate generator or proxy object manager !"));
            status = ER_FAIL;
            delete ecc;
            break;
        }

        // TODO For now only ALLJOYN_ECDHE_NULL sessions are enabled on the bus
        status = ba->EnablePeerSecurity(ECDHE_KEYX, new ECDHEKeyXListener(),
                                        AJNKEY_STORE, true);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Failed to enable security on the security manager bus attachment."));
            break;
        }

        if (storageCfg.settings.find(STORAGE_PATH_KEY)
            != storageCfg.settings.end()) {
            if (storageCfg.settings.at(STORAGE_PATH_KEY).empty()) {
                storageCfg.settings[STORAGE_PATH_KEY] = storagePath;                 /* Maybe we can we make this dependent on username + salted hash of password ? */
            }
        } else {
            storageCfg.settings[STORAGE_PATH_KEY] = storagePath;             /* Maybe we can we make this dependent on username + salted hash of password ? */
        }

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
            info.claimState = ApplicationClaimState::CLAIMED;
            info.runningState = ApplicationRunningState::UNKNOWN_RUNNING_STATE;
            info.publicKey = it->publicKey;
            info.userDefinedName = it->userDefinedName;
            info.busName = "";             // To be filled in when discovering the app is online.
            info.appID = it->appID;
            info.deviceName = it->deviceName;
            info.appName = it->appName;
            qcc::String manifest = it->manifest;
            info.manifest.Deserialize(manifest);

            if (!it->policy.empty()) {
                PermissionPolicy tmpPolicy;
                tmpPolicy.Import(*busAttachment, (const uint8_t*)it->policy.data(), it->policy.size());
                info.policy = tmpPolicy.ToString();
                qcc::String dbgPrint = "Warm Start - Application " + info.appName + " has this stored policy: \n";
                dbgPrint += info.policy;
                QCC_DbgHLPrintf((dbgPrint.c_str()));
            }

            appsMutex.Lock(__FILE__, __LINE__);
            applications[info.publicKey] = info;
            appsMutex.Unlock(__FILE__, __LINE__);
        }

        status = CreateInterface(busAttachment, serverintf);
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
    ajn::services::AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *this, NULL, 0);

    delete CertificateGen;
    delete proxyObjMgr;
    delete appMonitor;
    delete storage;
}

QStatus SecurityManagerImpl::CreateInterface(ajn::BusAttachment* bus,
                                             ajn::InterfaceDescription*& intf)
{
    QStatus status = bus->CreateInterface(MNGT_INTF_NAME, intf,
                                          AJ_IFC_SECURITY_REQUIRED);
    if (ER_OK != status) {
        QCC_LogError(status,
                     ("Failed to create interface '%s' on security manager bus attachment", MNGT_INTF_NAME));
        return status;
    }
    intf->AddMethod("Claim", "(ayay)", "(ayay)", "rotPublicKey,appPublicKey", 0);
    intf->AddMethod("InstallIdentity", "s", "b", "cert,result", 0);
    intf->AddMethod("InstallMembership", "ay", NULL, "cert", 0);
    intf->AddMethod("RemoveMembership", "ay", NULL, "guildID", 0);
    intf->AddMethod("GetManifest", NULL, "a{sa{sy}}", "manifest", 0);
    intf->AddMethod("InstallAuthorizationData", "a{sa{sy}}", NULL, "authData", 0);
    intf->AddMethod("InstallPolicy", "(yv)",  NULL, "authorization");
    intf->AddMethod("GetPolicy", NULL, "(yv)", "authorization");
    intf->Activate();

    return ER_OK;
}

QStatus SecurityManagerImpl::GetStatus() const
{
    return status;
}

QStatus SecurityManagerImpl::EstablishPSKSession(const ApplicationInfo& app,
                                                 uint8_t* bytes, size_t size)
{
    return ER_FAIL;
}

QStatus SecurityManagerImpl::ClaimApplication(const ApplicationInfo& appInfo, AcceptManifestCB amcb)
{
    ManagedApplicationInfo managedApplicationInfo;
    managedApplicationInfo.appName = "TestApp";     //TODO get the right name.
    ajn::ProxyBusObject* remoteObj = NULL;
    uint8_t* pubKeyX;
    size_t pubKeyXnumOfBytes;
    uint8_t* pubKeyY;
    size_t pubKeyYnumOfBytes;
    qcc::ECCPublicKey eccAppPubKey;
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

        /*===========================================================
         * Step 1: Setup a session and get a proxy object to the remote application
         */

        funcStatus = proxyObjMgr->GetProxyObject(app, ProxyObjectManager::SessionType::ECDHE_NULL, &remoteObj);
        if (funcStatus != ER_OK) {
            QCC_DbgRemoteError(
                ("Could not create a ProxyBusObject to remote application"));
            break;
        }

        /*===========================================================
         * Step 2: On ProxyBusObject call the "claim" method
         *         and provide our public key
         */

        ajn::Message reply(*busAttachment);
        ajn::MsgArg inputs[1];

        QCC_DbgPrintf(
            ("\nSending RoT public Key: %s", PubKeyToString(&rot.GetPublicKey()).c_str()));

        const uint8_t(&keyXBytes)[qcc::ECC_COORDINATE_SZ] =
            rot.GetPublicKey().x;
        const uint8_t(&keyYBytes)[qcc::ECC_COORDINATE_SZ] =
            rot.GetPublicKey().y;
        inputs[0].Set("(ayay)", qcc::ECC_COORDINATE_SZ, keyXBytes, qcc::ECC_COORDINATE_SZ, keyYBytes);

        funcStatus = remoteObj->MethodCall(MNGT_INTF_NAME, "Claim", inputs, 1, reply,
                                           5000);
        if (ER_OK != funcStatus) {
            QCC_DbgRemoteError(
                ("Remote app returned an error or timed out when calling the \"Claim\" function"));
            break;
        }
        const ajn::MsgArg* result = reply->GetArg(0);
        if (NULL == result) {
            QCC_DbgRemoteError(
                ("Marshalling error when returning from the call of the \"Claim\" function"));
            funcStatus = ER_FAIL;
            break;
        }

        if (ER_OK !=
            (funcStatus = result->Get("(ayay)", &pubKeyXnumOfBytes, &pubKeyX, &pubKeyYnumOfBytes, &pubKeyY))) {
            QCC_DbgRemoteError(
                ("Marshalling error: could not extract public key of remote application"));
            break;
        }
        // Verify key
        if ((qcc::ECC_COORDINATE_SZ != pubKeyXnumOfBytes) || (ECC_COORDINATE_SZ != pubKeyYnumOfBytes)) {
            QCC_DbgRemoteError(
                ("Remote error: remote application public key has invalid number of bytes"));
            funcStatus = ER_FAIL;
            break;
        }

        QCC_DbgPrintf(("Claim: Public key exchange was SUCCESSFUL !!"));

        /*===========================================================
         * Step 2b: Retrieve manifest and call accept manifest
         *   callback.
         */

        QCC_DbgPrintf(("Retrieving manifest of remote app ...\n"));

        ajn::Message mfMsg(*busAttachment);
        funcStatus = remoteObj->MethodCall(MNGT_INTF_NAME, "GetManifest",
                                           NULL, 0, mfMsg, 1000);
        if (ER_OK != funcStatus) {
            QCC_DbgPrintf(("Remote app returned error: %d\n", funcStatus));
            break;
        }

        const ajn::MsgArg* mfMa = mfMsg->GetArg(0);
        AuthorizationData manifest;
        manifest.Unmarshal(*mfMa);

        if (!amcb(manifest)) {
            break;
        }

        /*===========================================================
         * Step 3: Get About data from app
         *         Generate Identity certificate (TODO: get the right certificate)
         */

        memcpy(eccAppPubKey.x, pubKeyX, pubKeyXnumOfBytes);
        memcpy(eccAppPubKey.y, pubKeyY, pubKeyYnumOfBytes);

        qcc::IdentityCertificate idCertificate;
        idCertificate.SetAlias(qcc::String("TODO Alias"));
        idCertificate.SetApplicationID(app.appID);
        qcc::String serialNumber;

        storageMutex.Lock(__FILE__, __LINE__);
        funcStatus = storage->GetNewSerialNumber(serialNumber);
        storageMutex.Unlock(__FILE__, __LINE__);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to get a new serial number"));
            break;
        }

        idCertificate.SetSerialNumber(serialNumber);
        qcc::Certificate::ValidPeriod period;
        period.validFrom = time(NULL) - 3600;
        period.validTo = period.validFrom + 3600 +  3153600;     // valid for 365 days
        idCertificate.SetValidity(&period);
        idCertificate.SetSubject(&eccAppPubKey);
        //TODO are all fields set properly?
        if (ER_OK != (funcStatus = CertificateGen->GetIdentityCertificate(idCertificate))) {
            QCC_LogError(funcStatus, ("Failed to get identity certificate"));
            break;
        }

        QCC_DbgPrintf(
            ("Sending pem of identity certificate: %s", idCertificate.GetPEM().c_str()));

        /*===========================================================
         * Step 4: On session: call InstallIdentity with Identity
         *         certificate and wait on result
         */
        ajn::Message pemReply(*busAttachment);
        ajn::MsgArg pemInput[1];
        pemInput[0].Set("s", idCertificate.GetPEM().c_str());

        funcStatus = remoteObj->MethodCall(MNGT_INTF_NAME, "InstallIdentity",
                                           pemInput, 1, pemReply, 5000);
        if (ER_OK != funcStatus) {
            QCC_DbgRemoteError(
                ("Remote app returned an error or timed out when calling the \"InstallIdentity\" function"));
            break;
        }
        if (false == pemReply->GetArg(0)->v_bool) {
            QCC_DbgRemoteError(
                ("Identity certificate could not be installed on claimed application"));
            funcStatus = ER_FAIL;
            break;
        }

        /*===========================================================
         * Step 5: store claimed app data into the storage
         *         wait on result
         */
        memcpy(managedApplicationInfo.publicKey.x, eccAppPubKey.x,
               qcc::ECC_COORDINATE_SZ);
        memcpy(managedApplicationInfo.publicKey.y, eccAppPubKey.y,
               qcc::ECC_COORDINATE_SZ);

        managedApplicationInfo.appName = app.appName;
        busAttachment->GetPeerGUID(app.busName.c_str(), managedApplicationInfo.appID);
        managedApplicationInfo.deviceName = app.deviceName;
        managedApplicationInfo.userDefinedName = app.userDefinedName;
        manifest.Serialize(managedApplicationInfo.manifest);

        storageMutex.Lock(__FILE__, __LINE__);
        funcStatus = storage->StoreApplication(managedApplicationInfo);
        storageMutex.Unlock(__FILE__, __LINE__);

        if (ER_FAIL == funcStatus) {
            QCC_LogError(ER_FAIL,
                         ("Failed to store claimed application info !"));
            break;
        }

        appsMutex.Lock(__FILE__, __LINE__);
        appItr = applications.find(appInfo.publicKey);

        if (appItr == applications.end()) {
            appsMutex.Unlock(__FILE__, __LINE__);
            break;
        }

        app = appItr->second;
        app.manifest = manifest;
        applications[app.publicKey] =  app;
        appsMutex.Unlock(__FILE__, __LINE__);

        if (ER_OK != storage->GetManagedApplication(managedApplicationInfo)) {
            QCC_LogError(ER_FAIL,
                         ("Claimed application cannot be managed !"));
            funcStatus = ER_FAIL;
            break;
        }

        QCC_DbgPrintf(("Application is now persistently managed"));

        /*===========================================================
         * Step 6: store claimed app generated cert into the storage
         *         wait on result
         */
        storageMutex.Lock(__FILE__, __LINE__);
        funcStatus = storage->StoreCertificate(idCertificate);
        storageMutex.Unlock(__FILE__, __LINE__);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to store identity certificate of claimed application !"));
            break;
        }
#if 0
        //The maniffest should not be stored as associated data of the id certificate.
        qcc::String manifestStr;
        qcc::String cprStr;
        if (ER_OK == (funcStatus = manifest.Serialize(manifestStr))) {
            storageMutex.Lock(__FILE__, __LINE__);
            funcStatus = storage->GetAssociatedData(idCertificate, cprStr);
            storageMutex.Unlock(__FILE__, __LINE__);

            if (ER_OK != funcStatus) {
                storageMutex.Lock(__FILE__, __LINE__);
                funcStatus = storage->StoreAssociatedData(idCertificate, manifestStr);
                storageMutex.Unlock(__FILE__, __LINE__);
                if (ER_OK != funcStatus) {
                    QCC_LogError(funcStatus,
                                 ("Failed to store identity certificate associated data of claimed application !"));
                    break;
                }
            }

            storageMutex.Lock(__FILE__, __LINE__);
            funcStatus = storage->GetAssociatedData(idCertificate, cprStr);
            storageMutex.Unlock(__FILE__, __LINE__);

            if (ER_OK == funcStatus) {
                assert(cprStr.compare(manifestStr) == 0);
            }
        } else {
            QCC_LogError(funcStatus, ("Failed to serialize manifest"));
            break;
        }
#endif
        funcStatus = ER_OK;
    } while (0);

    /*===========================================================
     * Step 7: close session
     */
    if (remoteObj) {
        if (ER_OK != proxyObjMgr->ReleaseProxyObject(remoteObj)) {
            QCC_DbgRemoteError(("Error: could not close session"));
        }
    }

    return funcStatus;
}

QStatus SecurityManagerImpl::InstallMembership(const ApplicationInfo& appInfo,
                                               const GuildInfo& guildInfo,
                                               const AuthorizationData* authorizationData)
{
    //Argument checking.
    GuildInfo gi = guildInfo;

    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = storage->GetGuild(gi);
    storageMutex.Unlock(__FILE__, __LINE__);

    if (funcStatus != ER_OK) {     //The guild must exist!
        return funcStatus;
    }

    //Sanity check. Make sure we use our internal collected data. Don't trust what is passed to us.
    ApplicationInfo app;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);

    if (!exist) {
        QCC_LogError(ER_FAIL, ("App does not exist."));
        return ER_FAIL;
    }
    app = appItr->second;

    qcc::MemberShipCertificate cert;
    cert.SetGuildId(gi.guid.ToString());
    cert.SetSubject(&app.publicKey);
    AuthorizationData data;
    storageMutex.Lock(__FILE__, __LINE__);
    if (ER_OK != storage->GetCertificate(cert)) {
        // Generate Membership Certificate
        cert.SetGuildId(guildInfo.guid.ToString());
        cert.SetDelegate(false);
        cert.SetApplicationID(app.appID);

        qcc::String serialNumber;

        funcStatus = storage->GetNewSerialNumber(serialNumber);

        if (funcStatus != ER_OK) {
            QCC_LogError(ER_FAIL, ("Could not get a serial number."));
            storageMutex.Unlock(__FILE__, __LINE__);
            return funcStatus;
        }

        cert.SetSerialNumber(serialNumber);
        qcc::Certificate::ValidPeriod period;
        period.validFrom = time(NULL) - 3600;
        period.validTo = period.validFrom + 3600 +  3153600;     // valid for 365 days
        cert.SetValidity(&period);

        qcc::String authData;
        data = authorizationData ? *authorizationData : app.manifest;
        data.Serialize(authData);

        qcc::Crypto_SHA256 digest;
        digest.Init();
        digest.Update(authData);
        uint8_t bytes[32];
        digest.GetDigest(bytes, false);
        cert.SetDataDigest(qcc::String((const char*)bytes, sizeof(bytes)));
        funcStatus = storage->StoreCertificate(cert);
        if (funcStatus == ER_OK) {
            funcStatus = storage->StoreAssociatedData(cert, authData, true);     //update if already exists.
        }
        storageMutex.Unlock(__FILE__, __LINE__);
        if (funcStatus != ER_OK) {
            QCC_LogError(ER_FAIL, ("Failed to store membership certificate or authorization data"));
            return funcStatus;
        }
    } else {
        //The certificate is stored. Load the associated data as well.
        qcc::String authData;
        funcStatus = storage->GetAssociatedData(cert, authData);
        storageMutex.Unlock(__FILE__, __LINE__);
        if (funcStatus != ER_OK) {
            QCC_LogError(ER_FAIL, ("Failed to fetch associated data for membership certificate"));
            return funcStatus;
        }
        data.Deserialize(authData);
    }

    funcStatus = CertificateGen->GenerateMembershipCertificate(cert);

    if (funcStatus != ER_OK) {
        QCC_DbgRemoteError(
            ("Failed to generate membership certificate"));
        return funcStatus;
    }

    ajn::ProxyBusObject* remoteObj = NULL;
    funcStatus = proxyObjMgr->GetProxyObject(app, ProxyObjectManager::SessionType::ECDHE_DSA, &remoteObj);
    if (funcStatus != ER_OK) {
        QCC_DbgRemoteError(
            ("Could not create a ProxyBusObject to remote application"));
    } else {
        ajn::MsgArg inputs[1];
        qcc::String pem = cert.GetPEM();
        inputs[0].Set("ay", pem.length(), pem.data());
        Message replyMsg(*busAttachment);
        funcStatus = remoteObj->MethodCall(MNGT_INTF_NAME, "InstallMembership", inputs, 1, replyMsg, 5000);
        if (funcStatus != ER_OK) {
            QCC_DbgRemoteError(
                ("Could not call 'InstallMembership' on ProxyBusObject to remote application"));
        } else {
            ajn::MsgArg outArg;
            data.Marshal(outArg);
            Message replyMsg1(*busAttachment);
            funcStatus = remoteObj->MethodCall(MNGT_INTF_NAME,
                                               "InstallAuthorizationData",
                                               &outArg,
                                               1,
                                               replyMsg1,
                                               5000);
            if (funcStatus != ER_OK) {
                QCC_DbgRemoteError(
                    ("Could not call 'InstallAuthorizationData' on ProxyBusObject to remote application"));
            }
        }

        proxyObjMgr->ReleaseProxyObject(remoteObj);
    }
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

    qcc::MemberShipCertificate cert;
    cert.SetGuildId(guildInfo.guid.ToString());
    qcc::ECCPublicKey eccAppPubKey;

    memcpy(eccAppPubKey.x, appInfo.publicKey.x,
           sizeof(eccAppPubKey.x));
    memcpy(eccAppPubKey.y, appInfo.publicKey.y,
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

    ajn::ProxyBusObject* remoteObj = NULL;
    funcStatus = proxyObjMgr->GetProxyObject(app, ProxyObjectManager::SessionType::ECDHE_DSA, &remoteObj);
    if (funcStatus != ER_OK) {
        QCC_DbgRemoteError(
            ("Could not create a ProxyBusObject to remote application"));
    } else {
        ajn::MsgArg inputs[1];
        GUID128 guildid = guildInfo.guid;
        inputs[0].Set("ay", GUID128::SIZE, guildid.GetBytes());

        Message replyMsg(*busAttachment);
        funcStatus = remoteObj->MethodCall(MNGT_INTF_NAME, "RemoveMembership", inputs, 1, replyMsg, 5000);
        if (funcStatus != ER_OK) {
            QCC_DbgRemoteError(
                ("Could not call 'RemoveMembership' on ProxyBusObject to remote application"));
        }

        proxyObjMgr->ReleaseProxyObject(remoteObj);
    }
    return funcStatus;
}

QStatus SecurityManagerImpl::InstallPolicy(const ApplicationInfo& appInfo,
                                           PermissionPolicy& policy)
{
    QStatus status = ER_OK;
    uint8_t* policyData = NULL;
    size_t policySize;
    bool exist;
    ApplicationInfoMap::iterator appItr = SafeAppExist(appInfo.publicKey, exist);
    if (!exist) {
        QCC_LogError(ER_FAIL, ("Unkown application."));
        return ER_FAIL;
    }
    ApplicationInfo app = appItr->second;

    // Persisting the policy in all cases
    storageMutex.Lock(__FILE__, __LINE__);
    ManagedApplicationInfo managedAppInfo;
    managedAppInfo.publicKey = appItr->first;
    if (ER_OK == (status = storage->GetManagedApplication(managedAppInfo))) {
        if (ER_OK != (status = policy.Export(*busAttachment, &policyData, &policySize))) {
            QCC_LogError(status, ("Could not export policy from origin."));
            return status;
        }

        managedAppInfo.policy = qcc::String((const char*)policyData, policySize);

        if (ER_OK == (status = storage->StoreApplication(managedAppInfo, true))) {         //update flag set
            qcc::String print = "Persisted policy : \n";
            print += policy.ToString();
            QCC_DbgHLPrintf((print.c_str()));
        } else {
            QCC_LogError(status, ("Could not persist policy !"));
            return status;
        }
        delete[] policyData;
    } else {
        QCC_LogError(status,
                     ("Trying to persist a policy for an unmanaged application."));
        return status;
    }
    storageMutex.Unlock(__FILE__, __LINE__);

    MsgArg msgArg;
    status = policy.Export(msgArg);
    if (ER_OK != status) {
        QCC_LogError(status,
                     ("Failed to GeneratePolicyArgs."));
        return status;
    }

    ProxyBusObject* remoteObj;
    status = proxyObjMgr->GetProxyObject(app, ProxyObjectManager::SessionType::ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status,
                     ("Could not create a ProxyBusObject to remote application."));
    } else {
        Message replyMsg(*busAttachment);
        status = remoteObj->MethodCall(MNGT_INTF_NAME, "InstallPolicy", &msgArg, 1, replyMsg, 5000);
        if (ER_OK != status) {
            QCC_DbgRemoteError(
                ("Could not call 'InstallPolicy' on ProxyBusObject to remote application."));
        }
        proxyObjMgr->ReleaseProxyObject(remoteObj);
    }

    return status;
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
        ProxyBusObject* remoteObj;
        Message replyMsg(*busAttachment);

        status = proxyObjMgr->GetProxyObject(app, ProxyObjectManager::SessionType::ECDHE_DSA, &remoteObj);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Could not create a ProxyBusObject to remote application."));
            return status;
        }

        QStatus methodStatus;
        methodStatus = remoteObj->MethodCall(MNGT_INTF_NAME, "GetPolicy", NULL, 0, replyMsg, 5000);
        if (ER_OK != methodStatus) {
            QCC_DbgRemoteError(
                ("Could not call 'GetPolicy' on ProxyBusObject to remote application."));
        }

        status = proxyObjMgr->ReleaseProxyObject(remoteObj);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Could not release ProxyBusObject to remote application."));
            return status;
        }

        if (ER_OK != methodStatus) {
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
        if (ER_OK != (status = storage->GetManagedApplication(mgdAppInfo))) {
            QCC_LogError(status, ("Could not find a persisted policy."));
            return status;
        }

        if (!mgdAppInfo.policy.empty()) {
            if (ER_OK !=
                (status =
                     policy.Import(*busAttachment, (const uint8_t*)mgdAppInfo.policy.data(),
                                   mgdAppInfo.policy.size()))) {
                QCC_LogError(status, ("Could not import policy to target."));
            }
        } else {
            QCC_DbgHLPrintf(("Empty policy"));
        }
    }
    return status;
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
        /* We already know this application */
        ApplicationInfo old(foundAppItr->second);
        foundAppItr->second.busName = newSecInfo->busName;
        foundAppItr->second.runningState = newSecInfo->runningState;
        foundAppItr->second.claimState = newSecInfo->claimState;
        foundAppItr->second.rootOfTrustList = newSecInfo->rotList;

        aboutCacheMutex.Lock(__FILE__, __LINE__);
        appCachedAboutInfoItr = aboutCache.find(newSecInfo->busName);
        if (appCachedAboutInfoItr != aboutCache.end()) {
            foundAppItr->second.appName = appCachedAboutInfoItr->second.appName;
            foundAppItr->second.appID = appCachedAboutInfoItr->second.appID;
            foundAppItr->second.deviceName =
                appCachedAboutInfoItr->second.deviceName;
            aboutCache.erase(newSecInfo->busName);
        }
        aboutCacheMutex.Unlock(__FILE__, __LINE__);
        for (ApplicationListener* listener : listeners) {
            listener->OnApplicationStateChange(&old, &foundAppItr->second);
        }
    } else {         /* New application */
        ApplicationInfo old;
        old.busName = oldSecInfo->busName;
        old.runningState = oldSecInfo->runningState;
        old.claimState = oldSecInfo->claimState;
        old.publicKey = oldSecInfo->publicKey;
        old.userDefinedName = "";
        old.appID = "";
        old.appName = "";
        old.deviceName = "";

        ApplicationInfo info;
        info.busName = newSecInfo->busName;
        info.runningState = newSecInfo->runningState;
        info.claimState = newSecInfo->claimState;
        info.publicKey = newSecInfo->publicKey;
        info.userDefinedName = "";
        info.appID = "";
        info.appName = "";
        info.deviceName = "";
        info.rootOfTrustList = newSecInfo->rotList;

        aboutCacheMutex.Lock(__FILE__, __LINE__);
        appCachedAboutInfoItr = aboutCache.find(newSecInfo->busName);
        if (appCachedAboutInfoItr != aboutCache.end()) {
            info.appName = appCachedAboutInfoItr->second.appName;
            info.appID = appCachedAboutInfoItr->second.appID;
            info.deviceName = appCachedAboutInfoItr->second.deviceName;
            aboutCache.erase(newSecInfo->busName);
        }
        aboutCacheMutex.Unlock(__FILE__, __LINE__);

        appsMutex.Lock(__FILE__, __LINE__);
        applications[info.publicKey] = info;
        appsMutex.Unlock(__FILE__, __LINE__);

        for (ApplicationListener* listener : listeners) {
            listener->OnApplicationStateChange(&old, &info);
        }
    }
}

QStatus SecurityManagerImpl::InstallIdentity(const ApplicationInfo& info)
{
    return ER_FAIL;
}

QStatus SecurityManagerImpl::AddRootOfTrust(const ApplicationInfo& app,
                                            const RootOfTrust& rot)
{
    return ER_FAIL;
}

QStatus SecurityManagerImpl::RemoveRootOfTrust(const ApplicationInfo& app,
                                               const RootOfTrust& rot)
{
    return ER_FAIL;
}

const RootOfTrust& SecurityManagerImpl::GetRootOfTrust() const
{
    return rot;
}

std::vector<ApplicationInfo> SecurityManagerImpl::GetApplications(ApplicationClaimState acs) const
{
    std::vector<ApplicationInfo> apps;
    ApplicationInfoMap::const_iterator appItr;

    appsMutex.Lock(__FILE__, __LINE__);

    for (appItr = applications.begin(); appItr != applications.end();
         ++appItr) {
        const ApplicationInfo& appInfo = appItr->second;

        if (ApplicationClaimState::UNKNOWN_CLAIM_STATE != acs) {
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
    QCC_DbgHLPrintf((" ---- Received announcement from '%s'  --------", busName));

    ApplicationInfoMap::const_iterator appItr;

    appsMutex.Lock(__FILE__, __LINE__);

    for (appItr = applications.begin(); appItr != applications.end(); ++appItr) {
        if (appItr->second.busName == busName) {
            ApplicationInfo old = appItr->second;
            ApplicationInfo newInfo = appItr->second;
            newInfo.appName = GetString(APP_NAME, aboutData);
            newInfo.deviceName = GetString(DEVICE_NAME, aboutData);
            newInfo.appID = GetAppId(aboutData);
            newInfo.runningState = ApplicationRunningState::RUNNING;
            for (ApplicationListener* listener : listeners) {
                listener->OnApplicationStateChange(&old, &newInfo);
            }
            appsMutex.Unlock(__FILE__, __LINE__);
            return;
        }
    }

    ApplicationInfo info;
    info.busName = busName;
    info.appName = GetString(APP_NAME, aboutData);
    info.deviceName = GetString(DEVICE_NAME, aboutData);
    info.appID = GetAppId(aboutData);
    info.runningState = ApplicationRunningState::RUNNING;
    info.claimState = ApplicationClaimState::UNKNOWN_CLAIM_STATE;
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

SecurityManagerImpl::ApplicationInfoMap::iterator SecurityManagerImpl::SafeAppExist(const PublicKey& publicKey,
                                                                                    bool& exist)
{
    appsMutex.Lock(__FILE__, __LINE__);
    ApplicationInfoMap::iterator ret = applications.find(publicKey);
    exist = (ret != applications.end());
    appsMutex.Unlock(__FILE__, __LINE__);
    return ret;
}

}
}
#undef QCC_MODULE
