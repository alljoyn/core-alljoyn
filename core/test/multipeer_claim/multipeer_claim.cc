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
#include <alljoyn/securitymgr/ApplicationInfo.h>
#include <alljoyn/securitymgr/PolicyGenerator.h>

#include <alljoyn/securitymgr/sqlstorage/SQLStorageFactory.h>

#include "PermissionMgmt.h"
#include "Stub.h"
#include <alljoyn/PermissionPolicy.h>

using namespace ajn::securitymgr;
using namespace std;

class AutoAccepter :
    public ManifestListener {
    bool ApproveManifest(const ApplicationInfo& appInfo,
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
        std::unique_lock<std::mutex> lk(m);
        cv_claimed.wait(lk, [this] { return claimed; });
        stub.SetDSASecurity(true);
        cout << "waitforclaimed --> ok " << getpid() << endl;
    }

    void WaitForIdentityCertificate()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_id.wait(lk, [this] { return pemIdentityCertificate != ""; });
        cout << "waitforidentity --> ok " << getpid() << endl;
    }

    void WaitForMembershipCertificate()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_memb.wait(lk, [this] { return pemMembershipCertificates.size() > 0; });
        cout << "waitformembership --> ok " << getpid() << endl;
    }

    void WaitForAuthData()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_auth.wait(lk, [this] { return authData.size() > 0; });
        cout << "waitforauthdata --> ok " << getpid() << endl;
    }

    void WaitForPolicy()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_pol.wait(lk, [this] { return policy != ""; });
        cout << "waitforpolicy --> ok " << getpid() << endl;
    }

  private:
    bool& claimAnswer;
    bool claimed;
    std::mutex m;
    std::condition_variable cv_claimed;
    std::condition_variable cv_id;
    std::condition_variable cv_memb;
    std::condition_variable cv_auth;
    std::condition_variable cv_pol;
    qcc::String pemIdentityCertificate;
    std::vector<qcc::String> pemMembershipCertificates;
    std::vector<PermissionPolicy*> authData;
    qcc::String policy;

    bool OnClaimRequest(const qcc::ECCPublicKey* pubKeyRot,
                        void* ctx)
    {
        assert(pubKeyRot != NULL);
        return claimAnswer;
    }

    /* This function is called when the claiming process has completed successfully */
    void OnClaimed(void* ctx)
    {
        std::unique_lock<std::mutex> lk(m);
        claimed = true;
        cv_claimed.notify_one();
        cout << "on claimed " << getpid() << endl;
    }

    virtual void OnAuthData(const PermissionPolicy* data)
    {
        std::unique_lock<std::mutex> lk(m);
        authData.push_back(const_cast<PermissionPolicy*>(data));
        cv_auth.notify_one();
        cout << "on Authorization Data " << getpid() << endl;
    }

    virtual void OnIdentityInstalled(const qcc::String& _pemIdentityCertificate)
    {
        std::unique_lock<std::mutex> lk(m);
        assert(_pemIdentityCertificate != "");
        pemIdentityCertificate = _pemIdentityCertificate;
        cv_id.notify_one();
        cout << "on identity installed " << getpid() << endl;
    }

    virtual void OnMembershipInstalled(const qcc::String& _pemMembershipCertificate)
    {
        std::unique_lock<std::mutex> lk(m);
        assert(_pemMembershipCertificate != "");
        pemMembershipCertificates.push_back(_pemMembershipCertificate);
        cv_memb.notify_one();
        cout << "on membership installed " << getpid() << endl;
    }

    virtual void OnPolicyInstalled(PermissionPolicy& _policy)
    {
        std::unique_lock<std::mutex> lk(m);
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
        srand(time(NULL) + (int)getpid());
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

        std::map<GUID128, qcc::String> membershipCertificates = stub.GetMembershipCertificates();
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
                 ajn::PermissionConfigurator::ClaimableState claimState,
                 size_t count)
    {
        std::unique_lock<std::mutex> lk(m);

        cout << "[Boss] Waiting for " << count << " peers."  << endl;
        cv.wait(lk, [ = ] { return CheckPredicateLocked(runningState, claimState, count); });

        return true;
    }

  private:
    map<qcc::String, ApplicationInfo> appInfo;
    std::mutex m;
    std::condition_variable cv;

    bool CheckPredicateLocked(ApplicationRunningState runningState,
                              ajn::PermissionConfigurator::ClaimableState claimState,
                              size_t count) const
    {
        if (appInfo.size() != count) {
            cout << "Not enough peers" << appInfo.size() << " != " << count << endl;
            return false;
        }

        for (pair<qcc::String, ApplicationInfo> businfo : appInfo) {
            if (businfo.second.runningState != runningState || businfo.second.claimState != claimState) {
                cout << "Wrong states for " << businfo.second.busName << businfo.second.runningState << " != " <<
                runningState << ", " <<  businfo.second.claimState << " != " << claimState << endl;
                return false;
            }
        }

        return true;
    }

    void OnApplicationStateChange(const ApplicationInfo* old,
                                  const ApplicationInfo* updated)
    {
        ApplicationListener::PrintStateChangeEvent(old, updated);
        const ApplicationInfo* info = updated ? updated : old;
        std::unique_lock<std::mutex> lk(m);
        appInfo[updated->busName] = *info;
        cout << "[Boss] Event peer count = " << appInfo.size() << endl;
        cv.notify_one();
    }
};

static int be_secmgr(size_t peers)
{
    QStatus status;
    bool retval = false;
    const char* storage_path = NULL;
    if ((storage_path = getenv("STORAGE_PATH")) == NULL) {
        storage_path = "/tmp/secmgr.db";
    }
    remove(storage_path);

    TestApplicationListener tal;
    BusAttachment ba("test", true);
    SecurityManager* secMgr = NULL;
    Storage* storage = NULL;
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

        SQLStorageFactory& sf = SQLStorageFactory::GetInstance();
        storage = sf.GetStorage();
        SecurityManager* secMgr = secFac.GetSecurityManager(storage, NULL);
        if (secMgr == NULL) {
            cerr << "No security manager" << endl;
            break;
        }
        secMgr->SetManifestListener(&aa);

        secMgr->RegisterApplicationListener(&tal);

        cout << "Waiting for peers to become claimable " << endl;
        if (tal.WaitFor(STATE_RUNNING, ajn::PermissionConfigurator::STATE_CLAIMABLE, peers) == false) {
            break;
        }

        sleep(2);

        vector<ApplicationInfo> apps = secMgr->GetApplications();
        bool breakhit = false;
        for (ApplicationInfo app : apps) {
            if (app.runningState == STATE_RUNNING && app.claimState ==
                ajn::PermissionConfigurator::STATE_CLAIMABLE) {
                cout << "Trying to claim " << app.busName.c_str() << endl;
                IdentityInfo idInfo;
                idInfo.guid = app.peerID;
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
        if (tal.WaitFor(STATE_RUNNING, ajn::PermissionConfigurator::STATE_CLAIMED, peers) == false) {
            break;
        }

        if (secMgr->GetApplications(ajn::PermissionConfigurator::STATE_CLAIMED).size() != peers) {
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
        for (ApplicationInfo app : apps) {
            if (app.runningState == STATE_RUNNING && app.claimState ==
                ajn::PermissionConfigurator::STATE_CLAIMED) {
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

int main(int argc, char** argv)
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
                std::chrono::milliseconds dura(1000);
                std::this_thread::sleep_for(dura);
                newArgs[1] = (char*)"p";
                newArgs[2] = (char*)"10";
            }
            newArgs[3] = NULL;
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
