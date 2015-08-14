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
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <thread>
#include <cstdio>
#include <condition_variable>
#include <sys/wait.h>

#include <alljoyn/securitymgr/SecurityManagerFactory.h>
#include <alljoyn/securitymgr/GuildInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/PolicyGenerator.h>
#include <alljoyn/securitymgr/sqlstorage/SQLStorageFactory.h>

#include <qcc/Environ.h>

#include "PermissionMgmt.h"
#include "Stub.h"
#include <alljoyn/PermissionPolicy.h>

using namespace ajn::securitymgr;
using namespace std;

class AutoAccepter :
    public ClaimListener {
    bool ApproveManifest(const Application& app,
                         const PermissionPolicy::Rule* manifestRules,
                         const size_t manifestRulesCount)
    {
        return true;
    }
};

class TestClaimListener :
    public ClaimListener {
  public:
    TestClaimListener(bool& _claimAnswer) :
        claimAnswer(_claimAnswer), claimed(false), pemIdentityCertificate("") { }

    void WaitForClaimed(Stub& stub)
    {
        unique_lock<mutex> lk(m);
        cv_claimed.wait(lk, [this] { return claimed; });
        stub.SetDSASecurity(true);
        cout << "waitforclaimed --> ok " << getpid() << endl;
    }

    void WaitForIdentityCertificate()
    {
        unique_lock<mutex> lk(m);
        cv_id.wait(lk, [this] { return pemIdentityCertificate != ""; });
        cout << "waitforidentity --> ok " << getpid() << endl;
    }

    void WaitForMembershipCertificate()
    {
        unique_lock<mutex> lk(m);
        cv_memb.wait(lk, [this] { return pemMembershipCertificates.size() > 0; });
        cout << "waitformembership --> ok " << getpid() << endl;
    }

    void WaitForAuthData()
    {
        unique_lock<mutex> lk(m);
        cv_auth.wait(lk, [this] { return authData.size() > 0; });
        cout << "waitforauthdata --> ok " << getpid() << endl;
    }

    void WaitForPolicy()
    {
        unique_lock<mutex> lk(m);
        cv_pol.wait(lk, [this] { return policy != ""; });
        cout << "waitforpolicy --> ok " << getpid() << endl;
    }

  private:
    bool& claimAnswer;
    bool claimed;
    mutex m;
    condition_variable cv_claimed;
    condition_variable cv_id;
    condition_variable cv_memb;
    condition_variable cv_auth;
    condition_variable cv_pol;
    string pemIdentityCertificate;
    vector<string> pemMembershipCertificates;
    vector<PermissionPolicy*> authData;
    string policy;

    bool OnClaimRequest(const ECCPublicKey* pubKeyRot,
                        void* ctx)
    {
        assert(pubKeyRot != nullptr);
        return claimAnswer;
    }

    /* This function is called when the claiming process has completed successfully */
    void OnClaimed(void* ctx)
    {
        unique_lock<mutex> lk(m);
        claimed = true;
        cv_claimed.notify_one();
        cout << "on claimed " << getpid() << endl;
    }

    virtual void OnAuthData(const PermissionPolicy* data)
    {
        unique_lock<mutex> lk(m);
        authData.push_back(const_cast<PermissionPolicy*>(data));
        cv_auth.notify_one();
        cout << "on Authorization Data " << getpid() << endl;
    }

    virtual void OnIdentityInstalled(const string& _pemIdentityCertificate)
    {
        unique_lock<mutex> lk(m);
        assert(_pemIdentityCertificate != "");
        pemIdentityCertificate = _pemIdentityCertificate;
        cv_id.notify_one();
        cout << "on identity installed " << getpid() << endl;
    }

    virtual void OnMembershipInstalled(const string& _pemMembershipCertificate)
    {
        unique_lock<mutex> lk(m);
        assert(_pemMembershipCertificate != "");
        pemMembershipCertificates.push_back(_pemMembershipCertificate);
        cv_memb.notify_one();
        cout << "on membership installed " << getpid() << endl;
    }

    virtual void OnPolicyInstalled(PermissionPolicy& _policy)
    {
        unique_lock<mutex> lk(m);
        assert(_policy.GetTermsSize() > 0);
        policy = _policy.ToString();
        cv_pol.notify_one();
        cout << "on policy installed " << getpid() << endl;
    }
};

static int be_peer()
{
    int retval = false;
    bool claimAnswer = true;

    do {
        srand(time(nullptr) + (int)getpid());
        TestClaimListener tcl(claimAnswer);
        Stub stub(&tcl);
        if (stub.GetInstalledIdentityCertificate() != "") {
            retval = false;
            break;
        }

        if (stub.GetRoTKeys().size() != 0) {
            retval = false;
            break;
        }

        // if (stub.OpenClaimWindow() != ER_OK) {
        //    retval = false;
        //    break;
        // }

        cout << "Waiting to be claimed " << getpid() << endl;
        tcl.WaitForClaimed(stub);
        cout << "Waiting identity certificate " << getpid() << endl;
        tcl.WaitForIdentityCertificate();
        cout << "Waiting membership certificate " << getpid() << endl;
        tcl.WaitForMembershipCertificate();
        cout << "Waiting for Authorization data " << getpid() << endl;
        tcl.WaitForAuthData();
        cout << "Waiting for policy " << getpid() << endl;
        tcl.WaitForPolicy();

        if (stub.GetInstalledIdentityCertificate() == "") {
            cerr << "Identity certificate not installed" << endl;
            retval = false;
            break;
        }

        map<GUID128, string> membershipCertificates = stub.GetMembershipCertificates();
        if (membershipCertificates.size() != 1) {
            cerr << "Membership certificate not installed" << endl;
            retval = false;
            break;
        }

        if (stub.GetRoTKeys().size() == 0) {
            cerr << "No RoT" << endl;
            retval = false;
            break;
        }

        retval = true;
    } while (0);

    cout << "Peer " << getpid()  << " finished " << retval << endl;

    return retval;
}

class TestApplicationListener :
    public ApplicationListener {
  public:
    TestApplicationListener()
    { }

    ~TestApplicationListener()
    {
    }

    bool WaitFor(ApplicationRunningState runningState,
                 PermissionConfigurator::ApplicationState claimState,
                 size_t count)
    {
        unique_lock<mutex> lk(m);

        cout << "[Boss] Waiting for " << count << " peers."  << endl;
        cv.wait(lk, [ = ] { return CheckPredicateLocked(runningState, claimState, count); });

        return true;
    }

  private:
    map<string, Application> app;
    mutex m;
    condition_variable cv;

    bool CheckPredicateLocked(ApplicationRunningState runningState,
                              PermissionConfigurator::ApplicationState claimState,
                              size_t count) const
    {
        if (app.size() != count) {
            cout << "Not enough peers" << app.size() << " != " << count << endl;
            return false;
        }

        for (pair<string, Application> businfo : app) {
            if (businfo.second.runningState != runningState || businfo.second.claimState != claimState) {
                cout << "Wrong states for " << businfo.second.busName << businfo.second.runningState << " != " <<
                    runningState << ", " <<  businfo.second.claimState << " != " << claimState << endl;
                return false;
            }
        }

        return true;
    }

    void OnApplicationStateChange(const OnlineApplication* old,
                                  const OnlineApplication* updated)
    {
        ApplicationListener::PrintStateChangeEvent(old, updated);
        const Application* info = updated ? updated : old;
        unique_lock<mutex> lk(m);
        app[updated->busName] = *info;
        cout << "[Boss] Event peer count = " << app.size() << endl;
        cv.notify_one();
    }

    void OnSyncError(const SyncError* syncError)
    {
        QCC_UNUSED(syncError);
    }
};

static int be_secmgr(size_t peers)
{
    QStatus status;
    bool retval = false;
    string storage_path;

    storage_path = Environ::GetAppEnviron()->Find("STORAGE_PATH", "/tmp/secmgr.db");

    remove(storage_path.c_str());

    TestApplicationListener tal;
    BusAttachment ba("test", true);
    SecurityManager* secMgr = nullptr;
    Storage* storage = nullptr;
    AutoAccepter aa;

    do {
        status = ba.Start();
        if (status != ER_OK) {
            cerr << "Could not start busattachment" << endl;
            break;
        }

        status = ba.Connect();
        if (status != ER_OK) {
            cerr << "Could not connect busattachment" << endl;
            break;
        }

        SecurityManagerFactory& secFac = SecurityManagerFactory::GetInstance();

        StorageFactory& sf = StorageFactory::GetInstance();
        storage = sf.GetStorage();
        SecurityManager* secMgr = secFac.GetSecurityManager(storage, nullptr);
        if (secMgr == nullptr) {
            cerr << "No security agent" << endl;
            break;
        }
        secMgr->SetManifestListener(&aa);

        secMgr->RegisterApplicationListener(&tal);

        cout << "Waiting for peers to become claimable " << endl;
        if (tal.WaitFor(STATE_RUNNING, PermissionConfigurator::CLAIMABLE, peers) == false) {
            break;
        }

        sleep(2);

        vector<Application> apps = secMgr->GetApplications();
        bool breakhit = false;
        for (Application app : apps) {
            if (app.runningState == STATE_RUNNING && app.claimState ==
                PermissionConfigurator::CLAIMABLE) {
                cout << "Trying to claim " << app.busName.c_str() << endl;
                IdentityInfo idInfo;
                idInfo.guid = GUID128(app.aki);
                idInfo.name = "MyTestName";
                if (secMgr->StoreIdentity(idInfo) != ER_OK) {
                    cerr << "Could not store identity " << endl;
                    breakhit = true;
                    break;
                }

                if (secMgr->Claim(app, idInfo) != ER_OK) {
                    cerr << "Could not claim application " << app.busName.c_str() << endl;
                    breakhit = true;
                    break;
                }
            }
        }
        if (breakhit) {
            break;
        }

        cout << "Waiting for peers to become claimed " << endl;
        if (tal.WaitFor(STATE_RUNNING, PermissionConfigurator::CLAIMED, peers) == false) {
            break;
        }

        if (secMgr->GetApplications(PermissionConfigurator::CLAIMED).size() != peers) {
            cerr << "Expected: " << peers << " claimed applications but only have " <<
                secMgr->GetApplications().size() << endl;
            break;
        }

        GuildInfo guild;
        guild.guid = GUID128("E4DD81F54E7DB918EA5B2CE79D72200E");
        secMgr->StoreGuild(guild);

        PermissionPolicy policy;
        vector<GuildInfo> guilds;
        guilds.push_back(guild);
        if (ER_OK != PolicyGenerator::DefaultPolicy(guilds, policy)) {
            cerr << "Failed to generate policy." << endl;
            break;
        }

        apps = secMgr->GetApplications();
        for (Application app : apps) {
            if (app.runningState == STATE_RUNNING && app.claimState ==
                PermissionConfigurator::CLAIMED) {
                cout << "Trying to install membership certificate on " << app.busName.c_str() << endl;
                if (secMgr->InstallMembership(app, guild) != ER_OK) {
                    cerr << "Could not install membership certificate on " << app.busName.c_str() << endl;
                    breakhit = true;
                    break;
                }
                if (secMgr->UpdatePolicy(app, policy) != ER_OK) {
                    cerr << "Could not install policy on " << app.busName.c_str() << endl;
                    breakhit = true;
                    break;
                }
            }
        }
        if (breakhit) {
            break;
        }

        retval = true;
    } while (0);

    secMgr->UnregisterApplicationListener(&tal);

    delete secMgr;
    ba.Disconnect();
    ba.Stop();
    ba.Join();
    delete storage;
    cout << "Secmgr " << getpid()  << " finished " << retval << endl;
    return retval;
}

int CDECL_CALL main(int argc, char** argv)
{
    int status;
    int peers;
    if (argc == 1) {
        peers = 4;
    } else if (argc == 2) {
        peers = atoi(argv[1]);
    } else if (argc == 3) {
        if (0 == strcmp(argv[1], "p")) {
            return be_peer();
        } else {
            peers = atoi(argv[2]);
            return be_secmgr(peers);
        }
    }

    int secmgrs = 1;

    vector<pid_t> children(peers + secmgrs);

    for (size_t i = 0; i < children.size(); ++i) {
        if ((children[i] = fork()) == 0) {
            cout << "pid = " << getpid() << endl;
            char* newArgs[4];
            newArgs[0] = argv[0];
            char buf[20];
            if (i < (size_t)secmgrs) {
                cout << "[MAIN] SecMgr needs " << peers << " peers." << endl;
                newArgs[1] = (char*)"mgr";
                snprintf(buf, 20, "%d", peers);
                newArgs[2] = buf;
            } else {
                /* TODO: REMOVE THIS WHEN RACE-CONDITION WITH APPLICATIONINFO LISTENER REGISTRATION IS REMOVED */
                chrono::milliseconds dura(1000);
                this_thread::sleep_for(dura);
                newArgs[1] = (char*)"p";
                newArgs[2] = (char*)"10";
            }
            newArgs[3] = nullptr;
            execv(argv[0], newArgs);
            cout << "[MAIN] Exec fails." <<  endl;
            return EXIT_FAILURE;
        } else if (children[i] == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }
    }

    for (size_t i = 0; i < children.size(); ++i) {
        if (waitpid(children[i], &status, 0) < 0) {
            cerr << "could not wait for PID" << children[i] << endl;
            perror("waitpid");
            status = false;
            break;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) == false) {
            cerr << "PID = " << children[i] << endl;
            break;
        }
    }

    if (status == false) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
