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

#include "SecurityAgentImpl.h"

#include <qcc/Debug.h>
#include <qcc/CertificateECC.h>
#include <qcc/KeyInfoECC.h>

#include <alljoyn/version.h>
#include <alljoyn/Session.h>
#include <alljoyn/AllJoynStd.h>

#include <CredentialAccessor.h> // Still in alljoyn_core/src!
#include <PermissionManager.h> // Still in alljoyn_core/src!
#include <PermissionMgmtObj.h> // Still in alljoyn_core/src!

#include <alljoyn/securitymgr/Util.h>

#include "ApplicationUpdater.h"

#define QCC_MODULE "SECMGR_AGENT"

using namespace qcc;
using namespace std;

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

QStatus SecurityAgentImpl::ClaimSelf()
{
    QStatus status = ER_FAIL;
    // Manifest
    size_t manifestRuleCount = 1;
    PermissionPolicy::Rule manifestRules;
    manifestRules.SetInterfaceName("*");
    PermissionPolicy::Rule::Member* mfPrms = new PermissionPolicy::Rule::Member[1];
    mfPrms[0].SetMemberName("*");
    mfPrms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                            PermissionPolicy::Rule::Member::ACTION_MODIFY |
                            PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    manifestRules.SetMembers(1, mfPrms);
    Manifest mf;
    mf.SetFromRules(&manifestRules, manifestRuleCount);

    // Policy
    PermissionPolicy policy;
    policy.SetVersion(1);

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[1];
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];

    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
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
    acls[0].SetRules(1, rules);

    policy.SetAcls(1, acls);
    // Get public key, identity and membership certificates
    CredentialAccessor ca(*busAttachment);
    ECCPublicKey ownPublicKey;
    status = ca.GetDSAPublicKey(ownPublicKey);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to get Public key"));
        return status;
    }

    vector<IdentityCertificate> idCerts;
    vector<MembershipCertificateChain> memberships;

    GroupInfo adminGroup;
    KeyInfoNISTP256 agentKeyInfo;
    agentKeyInfo.SetPublicKey(&ownPublicKey);
    String ownPubKeyID;
    if (ER_OK != (status = CertificateX509::GenerateAuthorityKeyId(&ownPublicKey, ownPubKeyID))) {
        QCC_LogError(status, ("Failed to generate public key ID."));
        return status;
    }
    // Obliged to cast bcz of GenerateAuthorityKeyId String key ID output.
    agentKeyInfo.SetKeyId((const uint8_t*)ownPubKeyID.c_str(), ownPubKeyID.size());

    status = caStorage->RegisterAgent(agentKeyInfo, mf, adminGroup, idCerts, memberships);

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register agent"));
        return status;
    }

    // Claim
    string ownBusName = busAttachment->GetUniqueName().c_str();
    OnlineApplication ownAppInfo;
    ownAppInfo.busName = ownBusName;
    status = proxyObjectManager->Claim(ownAppInfo, publicKeyInfo,
                                       adminGroup, &idCerts.front(), 1, mf);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to Claim"));
        return status;
    }

    // Store policy
    uint8_t* pByteArray = nullptr;
    size_t pSize;
    Util::GetPolicyByteArray(policy, &pByteArray, &pSize);
    KeyStore::Key pKey;
    pKey.SetGUID(GUID128("F5CB9E723D7D4F1CFF985F4DD0D5E388"));
    KeyBlob pKeyBlob((uint8_t*)pByteArray, pSize, KeyBlob::GENERIC);
    delete[] pByteArray;
    status = ca.StoreKey(pKey, pKeyBlob);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store policy"));
        return status;
    }

    // Store membership certificate
    MembershipCertificateChain mcChain = *memberships.begin();
    MembershipCertificate mCert = *(mcChain.begin());
    GUID128 mGUID;
    KeyStore::Key mKey(KeyStore::Key::LOCAL, mGUID);
    KeyBlob mKeyBlob(mCert.GetEncoded(), mCert.GetEncodedLen(), KeyBlob::GENERIC);
    mKeyBlob.SetTag(String((const char*)mCert.GetSerial(), mCert.GetSerialLen()));
    KeyStore::Key mHead;
    mHead.SetGUID(GUID128("42B0C7F35695A3220A46B3938771E965"));
    KeyBlob mHeaderBlob;
    uint8_t mNumEntries = 1;
    mHeaderBlob.Set(&mNumEntries, 1, KeyBlob::GENERIC);
    status = ca.StoreKey(mHead, mHeaderBlob);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store membership header"));
        return status;
    }
    status = ca.AddAssociatedKey(mHead, mKey, mKeyBlob);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to store membership certificate"));
        return status;
    }

    return status;
}

SecurityAgentImpl::SecurityAgentImpl(const shared_ptr<AgentCAStorage>& _caStorage, BusAttachment* ba) :
    publicKeyInfo(),
    appMonitor(ApplicationMonitor::GetApplicationMonitor(ba)),
    busAttachment(ba),
    caStorage(_caStorage),
    queue(TaskQueue<AppListenerEvent*, SecurityAgentImpl>(this)), mfListener(nullptr)
{
    proxyObjectManager = nullptr;
    applicationUpdater = nullptr;
}

QStatus SecurityAgentImpl::Init()
{
    SessionOpts opts;
    QStatus status = ER_OK;

    do {
        if (nullptr == caStorage) {
            status = ER_FAIL;
            QCC_LogError(status, ("Invalid caStorage means."));
            break;
        }

        if (nullptr == busAttachment) {
            status = ER_FAIL;
            QCC_LogError(status, ("Null bus attachment."));
            break;
        }

        status = Util::Init(busAttachment);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to initialize Util"));
        }

        ProxyObjectManager::listener = new ECDHEKeyXListener();
        if (ProxyObjectManager::listener == nullptr) {
            status = ER_FAIL;
            QCC_LogError(status, ("Failed to allocate ECDHEKeyXListener"));
            break;
        }

        status = busAttachment->EnablePeerSecurity(KEYX_ECDHE_NULL, ProxyObjectManager::listener,
                                                   AJNKEY_STORE, true);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Failed to enable security on the security agent bus attachment."));
            break;
        }

        status = caStorage->GetCaPublicKeyInfo(publicKeyInfo);
        if (ER_OK != status || publicKeyInfo.empty()) {
            QCC_LogError(status, ("publicKeyInfo.GetPublicKey()->empty() = %i", publicKeyInfo.empty()));
        }

        proxyObjectManager = make_shared<ProxyObjectManager>(busAttachment);

        if (nullptr == proxyObjectManager) {
            QCC_LogError(ER_FAIL,
                         ("Could not create proxyObjectManager!"));
            status = ER_FAIL;
            break;
        }

        PermissionConfigurator::ApplicationState applicationState;
        status = busAttachment->GetPermissionConfigurator().GetApplicationState(applicationState);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to read claim local state."));
            break;
        }
        if (PermissionConfigurator::CLAIMED != applicationState) {
            status = ClaimSelf();
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to claim self"));
                break;
            }
        }

        applicationUpdater = make_shared<ApplicationUpdater>(busAttachment,
                                                             caStorage,
                                                             proxyObjectManager,
                                                             appMonitor,
                                                             this);
        if (nullptr == applicationUpdater) {
            status = ER_FAIL;
            QCC_LogError(status, ("Failed to initialize application updater."));
            break;
        }

        caStorage->RegisterStorageListener(this);

        if (nullptr == appMonitor) {
            QCC_LogError(status, ("nullptr Application Monitor"));
            status = ER_FAIL;
            break;
        }
        appMonitor->RegisterSecurityInfoListener(this);
    } while (0);

    return status;
}

SecurityAgentImpl::~SecurityAgentImpl()
{
    caStorage->UnRegisterStorageListener(this);

    appMonitor->UnregisterSecurityInfoListener(this);

    applicationUpdater = nullptr;

    queue.Stop();

    Util::Fini();

    proxyObjectManager = nullptr;

    delete ProxyObjectManager::listener;
    ProxyObjectManager::listener = nullptr;

    // empty string as authMechanism to avoid resetting keyStore
    QStatus status = busAttachment->EnablePeerSecurity("", nullptr, nullptr, true);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to disable security on busAttachment at destruction"));
    }
}

void SecurityAgentImpl::SetManifestListener(ManifestListener* mfl)
{
    mfListener = mfl;
}

QStatus SecurityAgentImpl::SetUpdatesPending(const OnlineApplication& app, bool updatesPending)
{
    appsMutex.Lock(__FILE__, __LINE__);

    bool exist;
    OnlineApplicationMap::iterator it = SafeAppExist(app.keyInfo, exist);
    if (!exist) {
        appsMutex.Unlock(__FILE__, __LINE__);
        QCC_LogError(ER_FAIL, ("Application does not exist !"));
        return ER_FAIL;
    }

    OnlineApplication oldApp = it->second;
    if (oldApp.updatesPending != updatesPending) {
        it->second.updatesPending = updatesPending;
        NotifyApplicationListeners(&oldApp, &(it->second));
    }

    appsMutex.Unlock(__FILE__, __LINE__);
    return ER_OK;
}

QStatus SecurityAgentImpl::Claim(const OnlineApplication& app, const IdentityInfo& identityInfo)
{
    QStatus status;

    // Check ManifestListener
    if (mfListener == nullptr) {
        status = ER_FAIL;
        QCC_LogError(status, ("No ManifestListener set"));
        return status;
    }

    // Check app
    bool exist;
    OnlineApplicationMap::iterator appItr = SafeAppExist(app.keyInfo, exist);
    if (!exist) {
        status = ER_FAIL;
        QCC_LogError(status, ("Unknown application"));
        return status;
    }
    OnlineApplication _app = appItr->second;

    /*===========================================================
     * Step 1: Accept manifest
     */
    Manifest manifest;
    status = proxyObjectManager->GetManifestTemplate(_app, manifest);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not retrieve manifest"));
        return status;
    }

    if (!mfListener->ApproveManifest(_app, manifest)) {
        return ER_MANIFEST_REJECTED;
    }

    /*===========================================================
     * Step 2: Claim
     */

    KeyInfoNISTP256 CAKeyInfo;
    status = caStorage->GetCaPublicKeyInfo(CAKeyInfo);

    IdentityCertificate idCertificate;

    GroupInfo adminGroup;
    status = caStorage->StartApplicationClaiming(_app, identityInfo, manifest, adminGroup, idCertificate);
    if (status != ER_OK) {
        return status;
    }
    status = proxyObjectManager->Claim(_app, CAKeyInfo, adminGroup, &idCertificate, 1, manifest);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not claim application"));
    }
    status = caStorage->FinishApplicationClaiming(_app, (status == ER_OK));
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to notify Application got claimed"));
        //TODO: should we unclaim?
    }

    return status;
}

void SecurityAgentImpl::AddSecurityInfo(OnlineApplication& app, const SecurityInfo& si)
{
    app.busName = si.busName;
    app.applicationState = si.applicationState;
    app.keyInfo = si.keyInfo;
}

void SecurityAgentImpl::RemoveSecurityInfo(OnlineApplication& app, const SecurityInfo& si)
{
    // Update online app if the busName is still relevant.
    if (app.busName == si.busName) {
        app.busName = "";
    }
}

void SecurityAgentImpl::OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                              const SecurityInfo* newSecInfo)
{
    if ((nullptr == newSecInfo) && (nullptr == oldSecInfo)) {
        QCC_LogError(ER_FAIL, ("Both OnSecurityStateChange args are nullptr!"));
        return;
    }

    KeyInfoNISTP256 pubKeyInfo =
        (nullptr != newSecInfo) ? newSecInfo->keyInfo : oldSecInfo->keyInfo;
    bool exist;
    OnlineApplicationMap::iterator foundAppItr = SafeAppExist(pubKeyInfo, exist);

    if (exist) {
        OnlineApplication old(foundAppItr->second);
        if (nullptr != newSecInfo) {
            // update of known application
            AddSecurityInfo(foundAppItr->second, *newSecInfo);
            NotifyApplicationListeners(&old, &foundAppItr->second);
        } else {
            // removal of known application
            RemoveSecurityInfo(foundAppItr->second, *oldSecInfo);
            NotifyApplicationListeners(&old, &foundAppItr->second);
        }
    } else {
        if (nullptr == newSecInfo) {
            // removal of unknown application
            return;
        }
        // add new application
        OnlineApplication app;
        AddSecurityInfo(app, *newSecInfo);

        QStatus status = caStorage->GetManagedApplication(app); //Ignore the status
        if (status != ER_OK && status != ER_END_OF_DATA) {
            //We expect ER_OK for known apps and ER_END_OF_DATA for new ones.
            //All other codes mean an error: we print a log statement and continue.
            QCC_LogError(status, ("Failed to retrieve info from storage; continuing"));
        }
        appsMutex.Lock(__FILE__, __LINE__);
        applications[app.keyInfo] = app;
        appsMutex.Unlock(__FILE__, __LINE__);

        NotifyApplicationListeners(nullptr, &app);
    }
}

const KeyInfoNISTP256& SecurityAgentImpl::GetPublicKeyInfo() const
{
    return publicKeyInfo;
}

QStatus SecurityAgentImpl::GetApplication(OnlineApplication& _application) const
{
    QStatus status = ER_END_OF_DATA;
    appsMutex.Lock(__FILE__, __LINE__);

    OnlineApplicationMap::const_iterator ret = applications.find(_application.keyInfo);
    if (ret != applications.end()) {
        status = ER_OK;
        _application = ret->second;
    }
    appsMutex.Unlock(__FILE__, __LINE__);

    return status;
}

QStatus SecurityAgentImpl::GetApplications(vector<OnlineApplication>& apps,
                                           const PermissionConfigurator::ApplicationState applicationState)
const
{
    QStatus status = ER_FAIL;
    OnlineApplicationMap::const_iterator appItr;

    appsMutex.Lock(__FILE__, __LINE__);

    if (applications.empty()) {
        appsMutex.Unlock(__FILE__, __LINE__);
        return ER_END_OF_DATA;
    }

    for (appItr = applications.begin(); appItr != applications.end();
         ++appItr) {
        const OnlineApplication& app = appItr->second;
        if (appItr->second.applicationState == applicationState) {
            apps.push_back(app);
        }
    }

    appsMutex.Unlock(__FILE__, __LINE__);

    status = (apps.empty() ? ER_END_OF_DATA : ER_OK);

    return status;
}

void SecurityAgentImpl::RegisterApplicationListener(ApplicationListener* al)
{
    if (nullptr != al) {
        applicationListenersMutex.Lock(__FILE__, __LINE__);
        listeners.push_back(al);
        applicationListenersMutex.Unlock(__FILE__, __LINE__);
    }
}

void SecurityAgentImpl::UnregisterApplicationListener(ApplicationListener* al)
{
    applicationListenersMutex.Lock(__FILE__, __LINE__);
    vector<ApplicationListener*>::iterator it = find(
        listeners.begin(), listeners.end(), al);
    if (listeners.end() != it) {
        listeners.erase(it);
    }
    applicationListenersMutex.Unlock(__FILE__, __LINE__);
}

SecurityAgentImpl::OnlineApplicationMap::iterator SecurityAgentImpl::SafeAppExist(const KeyInfoNISTP256 key,
                                                                                  bool& exist)
{
    appsMutex.Lock(__FILE__, __LINE__);
    OnlineApplicationMap::iterator ret = applications.find(key);
    exist = (ret != applications.end());
    appsMutex.Unlock(__FILE__, __LINE__);
    return ret;
}

void SecurityAgentImpl::NotifyApplicationListeners(const SyncError* error)
{
    queue.AddTask(new AppListenerEvent(nullptr, nullptr, error));
}

void SecurityAgentImpl::OnPendingChanges(vector<Application>& apps)
{
    OnPendingChangesCompleted(apps);
}

void SecurityAgentImpl::OnPendingChangesCompleted(vector<Application>& apps)
{
    for (size_t i = 0; i < apps.size(); i++) {
        OnlineApplication old;
        old.keyInfo = apps[i].keyInfo;
        if (ER_OK == GetApplication(old)) {
            OnlineApplication app = old;
            app.updatesPending = apps[i].updatesPending;
            appsMutex.Lock();
            applications[app.keyInfo] = app;
            appsMutex.Unlock();
            queue.AddTask(new AppListenerEvent(new OnlineApplication(old), new OnlineApplication(app), nullptr));
        }
    }
}

void SecurityAgentImpl::NotifyApplicationListeners(const OnlineApplication* oldApp,
                                                   const OnlineApplication* newApp)
{
    queue.AddTask(new AppListenerEvent(oldApp ? new OnlineApplication(*oldApp) : nullptr,
                                       newApp ? new OnlineApplication(*newApp) : nullptr,
                                       nullptr));
}

void SecurityAgentImpl::HandleTask(AppListenerEvent* event)
{
    applicationListenersMutex.Lock(__FILE__, __LINE__);
    if (event->syncError) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnSyncError(event->syncError);
        }
    } else {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnApplicationStateChange(event->oldApp, event->newApp);
        }
    }
    applicationListenersMutex.Unlock(__FILE__, __LINE__);
}

QStatus SecurityAgentImpl::GetApplicationSecInfo(SecurityInfo& secInfo) const
{
    return appMonitor->GetApplication(secInfo);
}

void SecurityAgentImpl::UpdateApplications(const vector<OnlineApplication>* apps)
{
    bool syncAll = (apps == nullptr);
    OnlineApplicationMap::const_iterator appMapItr;

    if (syncAll) {
        for (appMapItr = applications.begin(); appMapItr != applications.end();
             ++appMapItr) {
            const OnlineApplication& app = appMapItr->second;
            if (app.applicationState == PermissionConfigurator::CLAIMED) {
                applicationUpdater->UpdateApplication(app);
            }
        }
    } else {
        vector<OnlineApplication>::const_iterator appItr = apps->begin();
        while (appItr != apps->end()) {
            if ((appMapItr = applications.find(appItr->keyInfo)) != applications.end()) {
                const OnlineApplication& app = appMapItr->second;
                if (app.applicationState == PermissionConfigurator::CLAIMED) {
                    applicationUpdater->UpdateApplication(app);
                }
            }
            appMapItr++;
        }
    }
}
}
}
#undef QCC_MODULE
