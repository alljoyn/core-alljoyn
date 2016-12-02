/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include "SecurityAgentImpl.h"

#include <qcc/Debug.h>
#include <qcc/CertificateECC.h>
#include <qcc/KeyInfoECC.h>

#include <alljoyn/version.h>
#include <alljoyn/Session.h>
#include <alljoyn/AllJoynStd.h>

#include <alljoyn/securitymgr/Util.h>

#include "ApplicationUpdater.h"

#define QCC_MODULE "SECMGR_AGENT"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {

static AJ_PCSTR s_selfClaimManifestTemplateXml =
    "<manifest>"
    "<node>"
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

const PermissionConfigurator::ClaimCapabilities ClaimContext::CLAIM_TYPE_NOT_SET = 0;

class ClaimContextImpl :
    public ClaimContext, public DefaultECDHEAuthListener {
  public:
    ClaimContextImpl(const OnlineApplication& application,
                     const Manifest& manifest,
                     const PermissionConfigurator::ClaimCapabilities _capabilities,
                     const PermissionConfigurator::ClaimCapabilityAdditionalInfo _capInfo) :
        ClaimContext(application, manifest, _capabilities, _capInfo)
    {
    }

    QStatus SetPreSharedKey(const uint8_t* psk, size_t pskSize)
    {
        /*
         * This function suppresses compiler warnings when calling SetPSK, which is deprecated.
         * ECHDE_PSK is deprecated as of 16.04, but we still test it, per the Alliance deprecation policy.
         * ASACORE-2762 tracks removal of the ECDHE_PSK tests (and this function can be removed as a part of that work).
         * https://jira.allseenalliance.org/browse/ASACORE-2762
         */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

        return SetPSK(psk, pskSize);

#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    QStatus SetSharedPassword(const uint8_t* password, size_t passwordSize)
    {
        return SetPassword(password, passwordSize);
    }

    ProxyObjectManager::SessionType GetSessionType()
    {
        switch (GetClaimType()) {
        case PermissionConfigurator::CAPABLE_ECDHE_NULL:
            return ProxyObjectManager::ECDHE_NULL;

        case PermissionConfigurator::CAPABLE_ECDHE_PSK:
            return ProxyObjectManager::ECDHE_PSK;

        case PermissionConfigurator::CAPABLE_ECDHE_SPEKE:
            return ProxyObjectManager::ECDHE_SPEKE;

        case PermissionConfigurator::CAPABLE_ECDHE_ECDSA:
            return ProxyObjectManager::ECDHE_DSA;

        default:
            return ProxyObjectManager::ECDHE_SPEKE;
        }
    }
};

QStatus SecurityAgentImpl::ClaimSelf()
{
    QStatus status;
    IdentityCertificateChain idCerts;
    vector<MembershipCertificateChain> memberships;
    GroupInfo adminGroup;
    KeyInfoNISTP256 agentKeyInfo;
    String ownPubKeyID;
    string signedManifestXml;
    AJ_PCSTR signedManifestXmlC;
    PermissionConfigurator& configurator = busAttachment->GetPermissionConfigurator();

    // Get public key, identity and membership certificates
    configurator.GetSigningPublicKey(agentKeyInfo);
    status = caStorage->RegisterAgent(agentKeyInfo,
                                      s_selfClaimManifestTemplateXml,
                                      adminGroup,
                                      idCerts,
                                      signedManifestXml,
                                      memberships);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register agent"));
        return status;
    }
    signedManifestXmlC = signedManifestXml.c_str();

    // Go into claimable state by setting up a manifest.
    status = configurator.SetManifestTemplateFromXml(s_selfClaimManifestTemplateXml);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to set the Manifest"));
        return status;
    }

    // Claim
    status = configurator.Claim(publicKeyInfo,
                                adminGroup.guid,
                                adminGroup.authority,
                                idCerts.data(),
                                idCerts.size(),
                                &signedManifestXmlC,
                                1);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to Claim"));
        return status;
    }

    for (size_t index = 0; index < memberships.size(); index++) {
        status = configurator.InstallMembership(memberships[index].data(), memberships[index].size());
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to install membership certificate chain[%u]", index));
            return status;
        }
    }

    // Update policy
/* Cannot install policy See ASACORE-2543
    PermissionPolicy policy;
    policy.SetVersion(1);

    PermissionPolicy::Acl acls[2];
    PermissionPolicy::Peer rot[1];
    PermissionPolicy::Peer peers[1];
    PermissionPolicy::Rule rules[1];
    PermissionPolicy::Rule::Member prms[3];

    rot[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
    rot[0].SetKeyInfo(&caPublicKey);
    acls[0].SetPeers(1, rot);

    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[1].SetPeers(1, peers);
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
    acls[1].SetRules(1, rules);
    policy.SetAcls(2, acls);

    me.UpdatePolicy(policy);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to update policy"));
        return status;
    }
 */
    return status;
}

SecurityAgentImpl::SecurityAgentImpl(const shared_ptr<AgentCAStorage>& _caStorage, BusAttachment* ba) :
    publicKeyInfo(),
    appMonitor(nullptr),
    ownBa(false),
    caStorage(_caStorage),
    queue(this),
    claimListener(nullptr)
{
    proxyObjectManager = nullptr;
    applicationUpdater = nullptr;

    QStatus status = ER_FAIL;
    if (ba == nullptr) {
        ba = new BusAttachment("SecurityAgent", true);
        ownBa = true;
    }

    do {
        if (!ba->IsStarted()) {
            status = ba->Start();
            if (ER_OK != status) {
                QCC_LogError(status, ("Failed to start bus attachment"));
                break;
            }
        }

        if (!ba->IsConnected()) {
            status = ba->Connect();
            if (ER_OK != status) {
                QCC_LogError(status, ("Failed to connect bus attachment"));
                break;
            }
        }
    } while (0);

    busAttachment = ba;
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

        status = caStorage->GetCaPublicKeyInfo(publicKeyInfo);
        if (ER_OK != status) {
            QCC_LogError(status, ("CA is inaccessible"));
            break;
        }

        if (publicKeyInfo.empty()) {
            status = ER_FAIL;
            QCC_LogError(status, ("Public key of CA is empty"));
            break;
        }

        status = Util::Init(busAttachment);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to initialize Util"));
        }

        proxyObjectManager = make_shared<ProxyObjectManager>(busAttachment);

        status = busAttachment->EnablePeerSecurity(KEYX_ECDHE_SPEKE, &proxyObjectManager->listener);
        if (ER_OK != status) {
            QCC_LogError(status,
                         ("Failed to enable security on the security agent bus attachment."));
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

        appMonitor = shared_ptr<ApplicationMonitor>(new ApplicationMonitor(busAttachment));
        if (nullptr == appMonitor) {
            QCC_LogError(status, ("nullptr Application Monitor"));
            status = ER_FAIL;
            break;
        }
        appMonitor->RegisterSecurityInfoListener(this);

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
    } while (0);

    if (ER_OK != status) {
        Util::Fini();
    }
    return status;
}

SecurityAgentImpl::~SecurityAgentImpl()
{
    caStorage->UnRegisterStorageListener(this);

    if (appMonitor != nullptr) {
        appMonitor->UnregisterSecurityInfoListener(this);
        appMonitor = nullptr;
    }

    applicationUpdater = nullptr;

    queue.Stop();

    Util::Fini();

    proxyObjectManager = nullptr;

    if (ownBa) {
        busAttachment->Disconnect();
        busAttachment->Stop();
        busAttachment->Join();
        delete busAttachment;
        busAttachment = nullptr;
    }
}

void SecurityAgentImpl::SetClaimListener(ClaimListener* cl)
{
    claimListener = cl;
}

QStatus SecurityAgentImpl::SetSyncState(const Application& app,
                                        const ApplicationSyncState syncState)
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
    if (oldApp.syncState != syncState) {
        it->second.syncState = syncState;
        NotifyApplicationListeners(&oldApp, &(it->second));
    }

    appsMutex.Unlock(__FILE__, __LINE__);
    return ER_OK;
}

QStatus SecurityAgentImpl::Claim(const OnlineApplication& app, const IdentityInfo& identityInfo)
{
    QStatus status;

    // Check ClaimListener
    if (claimListener == nullptr) {
        status = ER_FAIL;
        QCC_LogError(status, ("No ClaimListener set"));
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

    PendingClaim pc(_app, &pendingClaims, &appsMutex);
    status = pc.Init();
    if (ER_OK != status) {
        QCC_LogError(status, ("Cannot concurrent claim a single application"));
        return status;
    }

    /*===========================================================
     * Step 1: Select Session type  & Accept manifest
     */
    Manifest manifest;
    PermissionConfigurator::ClaimCapabilities claimCapabilities;
    PermissionConfigurator::ClaimCapabilityAdditionalInfo claimCapInfo;
    {   // Open scope limit the lifetime of the proxy object
        ProxyObjectManager::ManagedProxyObject mngdProxy(_app);
        status = proxyObjectManager->GetProxyObject(mngdProxy, ProxyObjectManager::ECDHE_NULL);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not connect to the remote application"));
            return status;
        }
        status = mngdProxy.GetManifestTemplate(manifest);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not retrieve manifest template"));
            return status;
        }

        status = mngdProxy.GetClaimCapabilities(claimCapabilities, claimCapInfo);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not retrieve ClaimCapabilities"));
            return status;
        }
    }
    ClaimContextImpl ctx(_app, manifest, claimCapabilities, claimCapInfo);

    status = claimListener->ApproveManifestAndSelectSessionType(ctx);
    if (ER_OK != status) {
        return status;
    }

    if (!ctx.IsManifestApproved()) {
        return ER_MANIFEST_REJECTED;
    }

    if (ctx.GetClaimType() == ClaimContext::CLAIM_TYPE_NOT_SET) {
        status = ER_FAIL;
        QCC_LogError(status, ("No ClaimType selected by ClaimListener"));
        return status;
    }

    /*===========================================================
     * Step 2: Claim
     */

    KeyInfoNISTP256 CAKeyInfo;
    status = caStorage->GetCaPublicKeyInfo(CAKeyInfo);
    if (status != ER_OK) {
        QCC_LogError(status, ("CA is not available"));
        return status;
    }

    IdentityCertificateChain idCertificate;

    GroupInfo adminGroup;
    ajn::Manifest signedManifest;
    status = caStorage->StartApplicationClaiming(_app, identityInfo, manifest, adminGroup, idCertificate, signedManifest);
    if (status != ER_OK) {
        return status;
    }

    {   // Open scope limit the lifetime of the proxy object
        ProxyObjectManager::ManagedProxyObject mngdProxy(_app);
        status = proxyObjectManager->GetProxyObject(mngdProxy, ctx.GetSessionType(), &ctx);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not connect to application"));
        }

        if (ER_OK == status) {
            status = mngdProxy.Claim(CAKeyInfo, adminGroup, idCertificate, signedManifest);
            if (ER_OK != status) {
                QCC_LogError(status, ("Could not claim application"));
            }
        }
    }

    if (ER_OK == status) {
        ProxyObjectManager::ManagedProxyObject mngdProxy(_app);
        status = proxyObjectManager->GetProxyObject(mngdProxy, ProxyObjectManager::SessionType::ECDHE_DSA, &ctx);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not connect to application"));
        }

        status = busAttachment->SecureConnection(_app.busName.c_str(), true);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not secure connection"));
        }

        if (ER_OK == status) {
            /* "StartManagement" already called inside "Claim". */
            status = mngdProxy.EndManagement();
            if (ER_OK != status) {
                QCC_LogError(status, ("Could not end management after claiming"));
            }
        }
    }

    QStatus finiStatus = caStorage->FinishApplicationClaiming(_app, status);
    if (ER_OK != finiStatus) {
        QCC_LogError(finiStatus, ("Failed to finalize claiming attempt"));
        if (ER_OK == status) {
            ProxyObjectManager::ManagedProxyObject mngdProxy(_app);
            QStatus resetStatus = proxyObjectManager->GetProxyObject(mngdProxy);
            if (ER_OK == resetStatus) {
                resetStatus = mngdProxy.Reset();
            }
            if (ER_OK != resetStatus) {
                QCC_LogError(resetStatus, ("Failed to reset application after storage failure"));
            }
        }
    }
    return ER_OK != finiStatus ? finiStatus : status;
}

void SecurityAgentImpl::AddSecurityInfo(OnlineApplication& app, const SecurityInfo& si)
{
    app.busName = si.busName;
    app.applicationState = si.applicationState;
    app.keyInfo = si.keyInfo;
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
            // no internal clean-up is done. See ASACORE-2549
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

        // retrieve syncStatus from storage
        QStatus status = caStorage->GetManagedApplication(app);
        if (ER_END_OF_DATA == status) {
            app.syncState = SYNC_UNMANAGED;
        } else if (ER_OK != status) {
            QCC_LogError(status, ("Error retrieving application from storage"));
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

void SecurityAgentImpl::NotifyApplicationListeners(const ManifestUpdate* manifestUpdate)
{
    queue.AddTask(new AppListenerEvent(manifestUpdate));
}

void SecurityAgentImpl::NotifyApplicationListeners(const SyncError* error)
{
    queue.AddTask(new AppListenerEvent(error));
}

void SecurityAgentImpl::OnPendingChanges(vector<Application>& apps)
{
    for (size_t i = 0; i < apps.size(); i++) {
        SetSyncState(apps[i], apps[i].syncState);
    }
}

void SecurityAgentImpl::OnPendingChangesCompleted(vector<Application>& apps)
{
    for (size_t i = 0; i < apps.size(); i++) {
        SetSyncState(apps[i], SYNC_OK);
    }
}

void SecurityAgentImpl::OnApplicationsAdded(vector<Application>& apps)
{
    for (size_t i = 0; i < apps.size(); i++) {
        SetSyncState(apps[i], SYNC_OK);
    }
}

void SecurityAgentImpl::OnApplicationsRemoved(vector<Application>& apps)
{
    for (size_t i = 0; i < apps.size(); i++) {
        SetSyncState(apps[i], SYNC_UNMANAGED);
    }
}

void SecurityAgentImpl::OnStorageReset()
{
    appsMutex.Lock(__FILE__, __LINE__);

    OnlineApplicationMap::iterator it;
    for (it = applications.begin(); it != applications.end(); ++it) {
        SetSyncState(it->second, SYNC_UNMANAGED);
    }

    appsMutex.Unlock(__FILE__, __LINE__);
}

void SecurityAgentImpl::NotifyApplicationListeners(const OnlineApplication* oldApp,
                                                   const OnlineApplication* newApp)
{
    queue.AddTask(new AppListenerEvent(oldApp ? new OnlineApplication(*oldApp) : nullptr,
                                       newApp ? new OnlineApplication(*newApp) : nullptr));
}

void SecurityAgentImpl::HandleTask(AppListenerEvent* event)
{
    applicationListenersMutex.Lock(__FILE__, __LINE__);
    if (event->syncError) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnSyncError(event->syncError);
        }
    } else if (event->manifestUpdate) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->OnManifestUpdate(event->manifestUpdate);
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
            appItr++;
        }
    }
}
}
}
#undef QCC_MODULE