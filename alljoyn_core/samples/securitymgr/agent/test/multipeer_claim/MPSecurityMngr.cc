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

#include "MPSecurityMngr.h"

#include <qcc/Thread.h>

using namespace std;

namespace secmgr_tests {
MPSecurityMngr::MPSecurityMngr() :
    managerBus("mpsecmgr", true), validatorBus("localmpsecmgr", true), authListener(), secMgr(nullptr), storage(nullptr),
    agentCa(nullptr), generator(nullptr), pomngr(nullptr), aa(), peers(0), peersFound(0),
    peersClaimed(0), peersReady(0), errorFound(false), localstate(LS_UNCLAIMED)
{
}

QStatus MPSecurityMngr::Start(int nrOfPeers)
{
    QStatus status = ER_OK;

    peers = nrOfPeers;

    status = validatorBus.Start();
    if (status != ER_OK) {
        cerr << "Could not start local busattachment" << endl;
        return status;
    }

    status = validatorBus.Connect();
    if (status != ER_OK) {
        cerr << "Could not connect local busattachment" << endl;
        return status;
    }

    status = managerBus.Start();
    if (status != ER_OK) {
        cerr << "Could not start busattachment" << endl;
        return status;
    }

    status = managerBus.Connect();
    if (status != ER_OK) {
        cerr << "Could not connect busattachment" << endl;
        return status;
    }

    StorageFactory& sf = StorageFactory::GetInstance();
    status = sf.GetStorage("mpr", storage);
    if (ER_OK != status || nullptr == storage) {
        cerr << "Failed to created storage component" << endl;
        return status;
    }
    status = storage->GetCaStorage(agentCa);
    if (ER_OK != status || nullptr == agentCa) {
        cerr << "Failed to Get the AgentCaStorage" << endl;
        return status;
    }
    idInfo.name = "MPAppuser";
    status = storage->StoreIdentity(idInfo);
    if (status != ER_OK) {
        cerr << "Fail to create Idenity" << endl;
        return status;
    }

    group.name = "test group";
    status = storage->StoreGroup(group);
    if (status != ER_OK) {
        cerr << "Fail to create group" << endl;
        return status;
    }

    SecurityAgentFactory::GetInstance().GetSecurityAgent(agentCa, secMgr, &managerBus);
    if (secMgr == nullptr) {
        cerr << "No security agent" << endl;
        return status;
    }
    status = storage->GetAdminGroup(adminGroup);
    if (status != ER_OK) {
        cerr << "Fail to get admin group" << endl;
        return status;
    }

    status = storage->StoreGroup(adminGroup);
    if (status != ER_OK) {
        cerr << "Fail to create admin group" << endl;
        return status;
    }

    pomngr = shared_ptr<ProxyObjectManager>(new ProxyObjectManager(&validatorBus));
    status =
        validatorBus.EnablePeerSecurity(KEYX_ECDHE_NULL " " ECDHE_KEYX, &pomngr->listener, nullptr, true);
    if (status != ER_OK) {
        cerr << "Failed to enable Peer security on local bus" << endl;
        return status;
    }
    PermissionPolicy::Rule rule;
    rule.SetInterfaceName("*");
    PermissionPolicy::Rule::Member member;
    member.SetMemberName("*");
    member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    member.SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_PROVIDE |
        PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rule.SetMembers(1, &member);
    validatorBus.GetPermissionConfigurator().SetPermissionManifest(&rule, 1);

    generator = shared_ptr<PolicyGenerator>(new PolicyGenerator(adminGroup));

    secMgr->SetClaimListener(&aa);

    secMgr->RegisterApplicationListener(this);

    cout << "Waiting for peers to become claimable " << endl;
    return status;
}

void MPSecurityMngr::ClaimLocalBus(const OnlineApplication& localApp)
{
    QStatus status = secMgr->Claim(localApp, idInfo);
    if (ER_OK != status) {
        cerr << "Failed to claim local app: " << QCC_StatusText(status) << endl;
        errorFound = true;
        return;
    }
    status = storage->InstallMembership(localApp, adminGroup);
    if (ER_OK != status) {
        cerr << "Failed to install membership: " << QCC_StatusText(status) << endl;
        errorFound = true;
        return;
    }
    PermissionPolicy::Rule rule;
    rule.SetInterfaceName("*");
    PermissionPolicy::Rule::Member member;
    member.SetMemberName("*");
    member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    member.SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_PROVIDE |
        PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rule.SetMembers(1, &member);
    PermissionPolicy p;
    PermissionPolicy::Acl pacl;
    PermissionPolicy::Peer peers[2];
    peers[1].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
    shared_ptr<AgentCAStorage> ca;
    storage->GetCaStorage(ca);
    KeyInfoNISTP256 pk;
    ca->GetCaPublicKeyInfo(pk);
    peers[0].SetKeyInfo(&pk);
    pacl.SetPeers(2, peers);
    pacl.SetRules(1, &rule);
    p.SetAcls(1, &pacl);
    OnlineApplication app = localApp;
    status = storage->UpdatePolicy(app, p);
    if (ER_OK != status) {
        cerr << "Failed to claim local app:" << QCC_StatusText(status) << endl;
        errorFound = true;
        return;
    }
}

QStatus MPSecurityMngr::Stop()
{
    QStatus status = StopBus(managerBus);
    if (ER_OK != status) {
        return status;
    }
    return StopBus(validatorBus);
}

QStatus MPSecurityMngr::StopBus(BusAttachment& bus)
{
    QStatus status = bus.EnablePeerSecurity("", nullptr, nullptr, true);
    if (ER_OK != status) {
        return status;
    }

    status = bus.Disconnect();
    if (ER_OK != status) {
        return status;
    }

    status = bus.Stop();
    if (ER_OK != status) {
        return status;
    }

    status = bus.Join();
    if (ER_OK != status) {
        return status;
    }

    return status;
}

void MPSecurityMngr::CheckApplicationUpdated(const OnlineApplication& app)
{
    while (localstate != LS_INITIALIZED) {
        qcc::Sleep(250);
    }

    cout << "Secmgr[DoCheckApplicationUpdated]: checking application '" << app.busName << "'" << endl;
    vector<MembershipSummary> summaries;
    ProxyObjectManager::ManagedProxyObject mngdProxy(app);

    QStatus status = pomngr->GetProxyObject(mngdProxy);
    if (ER_OK != status) {
        cerr << "Secmgr[DoCheckApplicationUpdated]: Failed to connect to application" << app.busName << ". Got " <<
            status << endl;
        errorFound = true;
        return;
    }

    status = mngdProxy.GetMembershipSummaries(summaries);
    if (ER_OK != status) {
        cerr << "Secmgr[DoCheckApplicationUpdated]: Failed to get membership summaries " << app.busName << ". Got " <<
            status << endl;
        errorFound = true;
        return;
    }
    if (1 != summaries.size()) {
        cerr << "Secmgr[DoCheckApplicationUpdated]: Got wrong  membership summaries size for " << app.busName <<
            ". Got " <<
            summaries.size() << endl;
        errorFound = true;
        return;
    }
    uint32_t version = 0;
    status = mngdProxy.GetPolicyVersion(version);
    if (ER_OK != status) {
        cerr << "Secmgr[DoCheckApplicationUpdated]: Failed to get policy version " << app.busName << ". Got " <<
            status << endl;
        errorFound = true;
        return;
    }
    if (1 != summaries.size()) {
        cerr << "Secmgr[DoCheckApplicationUpdated]: Got wrong  policy version for " << app.busName << ". Got " <<
            version << endl;
        errorFound = true;
        return;
    }

    appslock.Lock();
    apps[app] = RESETTING;
    OnlineApplication _app = app;
    status = storage->ResetApplication(_app);
    if (ER_OK != status) {
        errorFound = true;
    }
    appslock.Unlock();
}

void MPSecurityMngr::ClaimApplications()
{
    appslock.Lock();
    auto _apps = apps;
    appslock.Unlock();

    for (auto it = _apps.begin(); it != _apps.end(); it++) {
        OnlineApplication app = it->first;
        appslock.Lock();
        auto _it = apps.find(app);
        if (_it != apps.end()) {
            _it->second = CLAIMING;
        } else {
            cout << "MPSecurityMngr::ClaimApplications: Cannot Find app " << app.ToString() << endl;
            errorFound = true;
        }
        appslock.Unlock();

        cout << "Secmgr[DoClaimApplication]: Claiming application '" << app.busName << "'" << endl;
        actionLock.Lock();
        QStatus status = secMgr->Claim(app, idInfo);
        actionLock.Unlock();
        if (ER_OK != status) {
            cerr << "Secmgr[DoClaimApplication]: Failed to claim application " << app.busName << ". Got " <<
                status << endl;
            errorFound = true;
            break;
        } else {
            peersClaimed++;
        }
    }
}

void MPSecurityMngr::UpdateApplication(const OnlineApplication& app)
{
    actionLock.Lock();
    QStatus status = storage->InstallMembership(app, group);
    if (ER_OK != status) {
        errorFound = true;
        cerr << "Could not install membership for" << app.ToString() << endl;
        actionLock.Unlock();
        return;
    }
    vector<GroupInfo> groups;
    groups.push_back(group);
    PermissionPolicy policy;
    generator->DefaultPolicy(groups, policy);
    OnlineApplication _app = app;
    status = storage->UpdatePolicy(_app, policy);
    if (ER_OK != status) {
        errorFound = true;
        cerr << "Could not update policy for" << app.ToString() << endl;
    }
    actionLock.Unlock();
}

void MPSecurityMngr::OnApplicationStateChange(const OnlineApplication* old,
                                              const OnlineApplication* updated)
{
    const OnlineApplication* info = updated ? updated : old;
    cout << "Secmgr[OnApplicationStateChange]: " << info->ToString() << endl;
    appslock.Lock();
    if (info->busName == validatorBus.GetUniqueName().c_str()) {
        if (LS_UNCLAIMED == localstate) {
            new ASyncTask(*info, this, CLAIM_LOCALBUS);
            localstate = LS_CLAIMING;
        } else {
            if (SYNC_PENDING == info->syncState) {
                localstate = LS_SYNC_INPROGRESS;
            } else if (LS_SYNC_INPROGRESS == localstate && SYNC_OK == info->syncState) {
                localstate = LS_INITIALIZED;
            }
        }
        appslock.Unlock();
        return;
    }
    if (PermissionConfigurator::CLAIMABLE == info->applicationState) {
        if (apps.find(*info) == apps.end()) {
            apps[*info] = DISCOVERED;
            if (++peersFound >= peers) {
                new ASyncTask(*info, this, CLAIM_APPS);//TODO worry about mem cleanUp later
            }
        } else if (RESETTING == apps.find(*info)->second) {
            apps[*info] = DONE;
            peersReady++;
        } else {
            //duplicate event (ignore) or no action needed.
        }
    } else {
        if (apps.find(*info) == apps.end()) {
            cerr << "Invalid state. Found claimed application not managed by us." << endl;
            appslock.Unlock();
            return;
        }
        if (PermissionConfigurator::CLAIMED == info->applicationState) {
            switch (apps.find(*info)->second) {
            case CLAIMING:
                apps[*info] = CLAIMED;
                break;

            default:
                //ignore event;
                break;
            }
        } else if (PermissionConfigurator::NEED_UPDATE == info->applicationState) {
            switch (apps.find(*info)->second) {
            case CLAIMING: //The CLAIMED state can be missed.
            case CLAIMED:
                UpdateApplication(*info);
                apps[*info] = UPDATE_PENDING;
                break;

            case UPDATE_PENDING:
                if (SYNC_PENDING == info->syncState) {
                    apps[*info] = UPDATING;
                }
                break;

            case UPDATING:
                if (SYNC_OK == info->syncState) {
                    new ASyncTask(*info, this, CHECK_UPDATES);//TODO worry about mem cleanUp later
                    apps[*info] = CHECK_UPDATE;
                }
                break;

            default:
                //ignore event;
                break;
            }
        }
    }
    appslock.Unlock();
}

void MPSecurityMngr::DumpState()
{
    appslock.Lock();
    cout << "MPSecurityMngr: Checking " << peers << "; found " << apps.size() << endl;

    for (map<OnlineApplication, AppState>::iterator it = apps.begin(); it != apps.end(); it++) {
        cout << "  " << it->first.ToString() << " state: " << string(ToString(it->second)) << endl;
    }

    appslock.Unlock();
}

QStatus MPSecurityMngr::WaitUntilFinished()
{
    int i = 0;
    map<OnlineApplication, AppState> lastChecked;
    int diffCount = 0;
    do {
        qcc::Sleep(1000);
        appslock.Lock();
        cout << "MPSecurityMngr::DumpState: peersNeeded = " << peers << ", peersFound = " <<  apps.size() <<
            ", peersClaimed = " << peersClaimed <<
            ", PeersReady = " << peersReady << endl;

        if (0 == (i++ % 10)) {
            DumpState();
            if (lastChecked == apps) {
                if (2 == diffCount++) {
                    errorFound = true;
                }
            } else {
                lastChecked = apps;
            }
        }
        appslock.Unlock();
        //Wait until all peers went through the test or an error occurs
    } while (peersReady < peers && (!errorFound));
    qcc::Sleep(1000);
    cout << "MPSecurityMngr::WaitUntilFinished: finished peers = " << peers << ", peersClaimed = " << peersClaimed <<
        ", PeersReady = " << peersReady << endl;
    return errorFound ? ER_FAIL : ER_OK;
}

void MPSecurityMngr::Reset()
{
    managerBus.ClearKeyStore();
    validatorBus.ClearKeyStore();
}

MPSecurityMngr::~MPSecurityMngr()
{
    Reset();
    Stop();
}
} // namespace
