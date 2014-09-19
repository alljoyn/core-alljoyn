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
static const char* ECDHE_KEYX = "ALLJOYN_ECDHE_NULL"; //"ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA";

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
    id(id), privKey(privKey), rot(pubKey), storageCfg(_storageCfg), status(
        ER_FAIL), am(ApplicationMonitor::GetApplicationMonitor(ba)), busAttachment(
        ba)
{
    status = ER_OK;
    SessionOpts opts;
    InterfaceDescription* serverintf;
    qcc::String storagePath(DEFAULT_STORAGE_PATH);

    qcc::Crypto_ECC* ecc = new qcc::Crypto_ECC();
    ecc->SetDSAPrivateKey(&privKey);
    ecc->SetDSAPublicKey(&pubKey);

    std::unique_ptr<CertificateGenerator> generator(new CertificateGenerator(
                                                        busAttachment->GetGlobalGUIDString(), ecc));
    CertificateGen = std::move(generator);
    do {
        // TODO For now only ALLJOYN_ECDHE_NULL sessions are enabled on the bus
        status = ba->EnablePeerSecurity(ECDHE_KEYX, new ECDHEKeyXListener(),
                                        AJNKEY_STORE, true);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Failed to enable security on the security manager bus attachment"));
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
        std::unique_ptr<Storage> store(sf.GetStorage(storageCfg));
        if (store == nullptr) {
            status = ER_FAIL;
            break;
        }

        status = store->GetStatus();
        storage = std::move(store);
        if (ER_OK != status) {
            break;
        }

        std::vector<ClaimedApplicationInfo> claimedApplications;
        storage->GetClaimedApplications(&claimedApplications);
        std::vector<ClaimedApplicationInfo>::const_iterator it =
            claimedApplications.begin();
        for (; it != claimedApplications.end(); ++it) {
            ApplicationInfo info;
            info.claimState = ApplicationClaimState::CLAIMED;
            info.runningState = ApplicationRunningState::UNKNOWN;
            std::string printableRoTKey = "";
            for (int i = 0; i < (int)qcc::ECC_PUBLIC_KEY_SZ; ++i) {
                char buff[4];
                sprintf(buff, "%02x", (unsigned char)(it->publicKey.data[i]));
                printableRoTKey = printableRoTKey + buff;
            }
            info.publicKey = qcc::String(printableRoTKey.data());
            info.userDefinedName = it->userDefinedName;
            info.busName = "";             // To be filled in when discovering the app is online.
            info.appID = it->appID;
            info.deviceName = it->deviceName;
            info.appName = it->appName;
            applications[info.publicKey] = info;
        }

        status = CreateInterface(busAttachment, serverintf);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create security interface"));
            break;
        }

        if (am == nullptr) {
            QCC_LogError(status, ("NULL Application Monitor"));
            status = ER_FAIL;
            break;
        }
        am->RegisterSecurityInfoListener(this);

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
    Storage* strg = storage.release();
    delete strg;
    ApplicationMonitor* a = am.release();
    delete a;
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
    intf->AddMethod("Claim", "ay", "ay", "rotPublicKey,appPublicKey", 0);
    intf->AddMethod("InstallIdentity", "s", "b", "PEMofIdentityCert,result", 0);
    intf->Activate();

    return ER_OK;
}

QStatus SecurityManagerImpl::GetStatus() const
{
    return status;
}

void SecurityManagerImpl::JoinSessionCB(QStatus status, SessionId id,
                                        const SessionOpts& opts, void* context)
{
}

void SecurityManagerImpl::SessionLost(ajn::SessionId sessionId,
                                      SessionLostReason reason)
{
    QCC_DbgPrintf(("Lost session %lu", (unsigned long)sessionId));
}

QStatus SecurityManagerImpl::EstablishPSKSession(const ApplicationInfo& app,
                                                 uint8_t* bytes, size_t size)
{
    return ER_FAIL;
}

QStatus SecurityManagerImpl::ClaimApplication(const ApplicationInfo& appInfo)
{
    if (ER_OK != status) {
        return status;
    }
    //Sanity check. Make sure we use our internal collected data. Don't trust what is passed to us.
    std::map<qcc::String, ApplicationInfo>::iterator it = applications.find(appInfo.publicKey);

    if (it == applications.end()) {
        return ER_FAIL;
    }
    ApplicationInfo app = it->second;

    ClaimedApplicationInfo claimedApplicationInfo;
    claimedApplicationInfo.appName = "TestApp";
    bool registeredClaimed = false;
    ajn::SessionId sessionId = 0;
    ajn::ProxyBusObject remoteObj;
    const InterfaceDescription* remoteIntf;
    uint8_t* pubKey;
    size_t pubKeynumOfBytes;
    qcc::ECCPublicKey eccAppPubKey;
    //  qcc::String pemIdentityCert;
    qcc::String Certificate;
    // qcc::CertificateType1 identyCertficate;     //TODO Figure out the correct cert type
    QStatus status = ER_FAIL;

    /*===========================================================
     * Step 1: Setup a session with remote application
     *   A. setup session options and join the session
     */
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = busAttachment->JoinSession(app.busName.c_str(), MNGT_SERVICE_PORT,
                                        this, sessionId, opts);
    if (ER_OK != status) {
        return status;
    }

    do {
        /* B. Setup ProxyBusObject:
         *     - Create a ProxyBusObject from remote application info and session
         *     - Get the interface description from the bus based on the remote interface name
         *     - Extend the ProxyBusObject with the interface description
         */

        remoteObj = ajn::ProxyBusObject(*busAttachment, app.busName.c_str(),
                                        MNGT_SERVICE_PATH, sessionId);
        remoteIntf = busAttachment->GetInterface(MNGT_INTF_NAME);
        if (NULL == remoteIntf) {
            QCC_DbgRemoteError(("No remote interface found of app to claim"));
            status = ER_FAIL;
            break;
        }
        remoteObj.AddInterface(*remoteIntf);

        /*===========================================================
         * Step 2: On ProxyBusObject call the "claim" method
         *         and provide our public key
         */

        ajn::Message reply(*busAttachment);
        ajn::MsgArg inputs[1];

        QCC_DbgPrintf(
            ("\nSending RoT public Key: %s", PubKeyToString(&rot.GetPublicKey()).c_str()));

        const uint8_t(&keyBytes)[qcc::ECC_PUBLIC_KEY_SZ] =
            rot.GetPublicKey().data;
        inputs[0].Set("ay", qcc::ECC_PUBLIC_KEY_SZ, keyBytes);

        status = remoteObj.MethodCall(MNGT_INTF_NAME, "Claim", inputs, 1, reply,
                                      5000);
        if (ER_OK != status) {
            QCC_DbgRemoteError(
                ("Remote app returned an error or timed out when calling the \"Claim\" function"));
            break;
        }
        const ajn::MsgArg* result = reply->GetArg(0);
        if (NULL == result) {
            QCC_DbgRemoteError(
                ("Marshalling error when returning from the call of the \"Claim\" function"));
            status = ER_FAIL;
            break;
        }

        if (ER_OK != (status = result->Get("ay", &pubKeynumOfBytes, &pubKey))) {
            QCC_DbgRemoteError(
                ("Marshalling error: could not extract public key of remote application"));
            break;
        }
        // Verify key
        if (qcc::ECC_PUBLIC_KEY_SZ != pubKeynumOfBytes) {
            QCC_DbgRemoteError(
                ("Remote error: remote application public key has invalid number of bytes"));
            status = ER_FAIL;
            break;
        }

        QCC_DbgPrintf(("Claim: Public key exchange was SUCCESSFUL !!"));

        /*===========================================================
         * Step 3: Get About data from app
         *         Generate Identity certificate (TODO: get the right certificate)
         */

        CertificateGen->GetIdentityCertificate(app.appID,
                                               app.publicKey,
                                               Certificate);
        memcpy((void*)&eccAppPubKey.data, (void*)pubKey,
               qcc::ECC_PUBLIC_KEY_SZ);

        QCC_DbgPrintf(
            ("\nReceived application public Key: %s \n", PubKeyToString(&eccAppPubKey).c_str()));
#if 0
        identyCertficate = qcc::CertificateType1(&rot.GetPublicKey(), &eccAppPubKey);
        pemIdentityCert = identyCertficate.GetPEM();
        QCC_DbgPrintf(
            ("Sending pem of identity certificate: %s", pemIdentityCert.c_str()));
#endif
        /*===========================================================
         * Step 4: On session: call InstallIdentity with Identity
         *         certificate and wait on result
         */
        ajn::Message pemReply(*busAttachment);
        ajn::MsgArg pemInput[1];
        pemInput[0].Set("s", Certificate.c_str());

        status = remoteObj.MethodCall(MNGT_INTF_NAME, "InstallIdentity",
                                      pemInput, 1, pemReply, 5000);
        if (ER_OK != status) {
            QCC_DbgRemoteError(
                ("Remote app returned an error or timed out when calling the \"InstallIdentity\" function"));
            break;
        }
        if (false == pemReply->GetArg(0)->v_bool) {
            QCC_DbgRemoteError(
                ("Identity certificate could not be installed on claimed application"));
            status = ER_FAIL;
            break;
        }

        /*===========================================================
         * Step 5: store claimed app data into the storage
         *         wait on result
         */
        memcpy(claimedApplicationInfo.publicKey.data, pubKey,
               qcc::ECC_PUBLIC_KEY_SZ);
        claimedApplicationInfo.appName = app.appName;
        claimedApplicationInfo.appID = app.appID;
        claimedApplicationInfo.deviceName = app.deviceName;
        claimedApplicationInfo.userDefinedName = app.userDefinedName;
        if (ER_FAIL
            == storage->TrackClaimedApplication(claimedApplicationInfo)) {
            QCC_LogError(ER_FAIL,
                         ("Failed to store claimed application's key !"));
            status = ER_FAIL;
            break;
        }

        if (ER_FAIL
            != storage->ClaimedApplication(eccAppPubKey,
                                           &registeredClaimed)) {
            if (!registeredClaimed) {
                QCC_LogError(ER_FAIL,
                             ("Failed to track a claimed application !"));
            }
            QCC_DbgPrintf(("Application was indeed registered as claimed"));
        } else {
            QCC_LogError(ER_FAIL,
                         ("Failed to check for claimed application !"));
            status = ER_FAIL;
            break;
        }

#if 0
        /*===========================================================
         * Step 6: store claimed app generated cert into the storage
         *         wait on result
         */

        if (ER_FAIL == (status = storage->StoreCertificate(eccAppPubKey, identyCertficate))) {
            QCC_LogError(status, ("Failed to store identity certificate of claimed application !"));
            break;
        }
#endif
        status = ER_OK;
    } while (0);

    /*===========================================================
     * Step 7: close session
     */
    if (ER_OK != busAttachment->LeaveSession(sessionId)) {
        QCC_DbgRemoteError(("Error: could not close session"));
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
    std::map<qcc::String, ApplicationInfo>::iterator foundAppItr;
    foundAppItr = applications.find(newSecInfo->publicKey);

    if (foundAppItr != applications.end()) {
        /* We already know this application */
        ApplicationInfo old = foundAppItr->second;
        foundAppItr->second.busName = newSecInfo->busName;
        foundAppItr->second.runningState = newSecInfo->runningState;
        foundAppItr->second.claimState = newSecInfo->claimState;
        foundAppItr->second.rootOfTrustList = newSecInfo->rotList;

        appCachedAboutInfoItr = aboutCache.find(newSecInfo->busName);
        if (appCachedAboutInfoItr != aboutCache.end()) {
            foundAppItr->second.appName = appCachedAboutInfoItr->second.appName;
            foundAppItr->second.appID = appCachedAboutInfoItr->second.appID;
            foundAppItr->second.deviceName =
                appCachedAboutInfoItr->second.deviceName;

            aboutCache.erase(newSecInfo->busName);
        }
        for (ApplicationListener* listener : listeners) {
            listener->OnApplicationStateChange(&old, &foundAppItr->second);
        }
    } else {     /* New application */
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

        appCachedAboutInfoItr = aboutCache.find(newSecInfo->busName);
        if (appCachedAboutInfoItr != aboutCache.end()) {
            info.appName = appCachedAboutInfoItr->second.appName;
            info.appID = appCachedAboutInfoItr->second.appID;
            info.deviceName = appCachedAboutInfoItr->second.deviceName;
            aboutCache.erase(newSecInfo->busName);
        }
        applications[info.publicKey] = info;
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
    std::map<qcc::String, ApplicationInfo>::const_iterator appItr;

    for (appItr = applications.begin(); appItr != applications.end();
         ++appItr) {
        const ApplicationInfo& appInfo = appItr->second;

        if (ApplicationClaimState::UNKNOWN != acs) {
            if (appItr->second.claimState == acs) {
                apps.push_back(appInfo);
            }
        } else {
            apps.push_back(appInfo);
        }
    }
    return apps;
}

void SecurityManagerImpl::RegisterApplicationListener(ApplicationListener* al)
{
    if (NULL != al) {
        listeners.push_back(al);
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

ApplicationInfo* SecurityManagerImpl::GetApplication(qcc::String& busName) const
{
    std::vector<ApplicationInfo> apps;
    std::map<qcc::String, ApplicationInfo>::const_iterator appItr;

    for (appItr = applications.begin(); appItr != applications.end();
         ++appItr) {
        if (busName == appItr->second.busName) {
            return new ApplicationInfo(appItr->second);
        }
    }
    return NULL;
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

    std::map<qcc::String, ApplicationInfo>::const_iterator appItr;

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
            return;
        }
    }

    ApplicationInfo info;
    info.busName = busName;
    info.appName = GetString(APP_NAME, aboutData);
    info.deviceName = GetString(DEVICE_NAME, aboutData);
    info.appID = GetAppId(aboutData);
    info.runningState = ApplicationRunningState::RUNNING;
    info.claimState = ApplicationClaimState::UNKNOWN;
    info.publicKey = "";

    info.userDefinedName = "";

    aboutCache[busName] = info;
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
}
}
#undef QCC_MODULE
