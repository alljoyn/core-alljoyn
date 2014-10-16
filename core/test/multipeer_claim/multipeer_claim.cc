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
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <thread>
#include <cstdio>
#include <condition_variable>
#include <sys/wait.h>

#include "SecurityManagerFactory.h"
#include "PermissionMgmt.h"
#include "Common.h"
#include "Stub.h"
#include "AuthorizationData.h"
#include "AppGuildInfo.h"

#include <PolicyGenerator.h>
#include <alljoyn/PermissionPolicy.h>

using namespace ajn::securitymgr;
using namespace std;

class TestClaimListener :
    public ClaimListener {
  public:
    TestClaimListener(bool& _claimAnswer) :
        claimAnswer(_claimAnswer), claimed(false), pemIdentityCertificate("") { }

    void WaitForClaimed()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_claimed.wait(lk, [this] { return claimed; });
        cout << "waitforclaimed --> ok" << endl;
    }

    void WaitForIdentityCertificate()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_id.wait(lk, [this] { return pemIdentityCertificate != ""; });
        cout << "waitforidentity --> ok" << endl;
    }

    void WaitForMembershipCertificate()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_memb.wait(lk, [this] { return pemMembershipCertificates.size() > 0; });
        cout << "waitformembership --> ok" << endl;
    }

    void WaitForAuthData()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_auth.wait(lk, [this] { return authData.size() > 0; });
        cout << "waitforauthdata --> ok" << endl;
    }

    void WaitForPolicy()
    {
        std::unique_lock<std::mutex> lk(m);
        cv_pol.wait(lk, [this] { return policy != ""; });
        cout << "waitforpolicy --> ok" << endl;
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
    std::vector<AuthorizationData> authData;
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

    virtual void OnAuthData(const AuthorizationData& data)
    {
        std::unique_lock<std::mutex> lk(m);
        authData.push_back(data);
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

        if (stub.OpenClaimWindow() != ER_OK) {
            retval = false;
            break;
        }

        cout << "Waiting to be claimed " << getpid() << endl;
        tcl.WaitForClaimed();
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

    bool WaitFor(ApplicationRunningState runningState, ApplicationClaimState claimState, size_t count)
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [ = ] { return CheckPredicateLocked(runningState, claimState, count); });

        return true;
    }

  private:
    map<qcc::String, ApplicationInfo> appInfo;
    std::mutex m;
    std::condition_variable cv;

    bool CheckPredicateLocked(ApplicationRunningState runningState, ApplicationClaimState claimState,
                              size_t count) const
    {
        if (appInfo.size() != count) {
            return false;
        }

        for (pair<qcc::String, ApplicationInfo> businfo : appInfo) {
            if (businfo.second.runningState != runningState || businfo.second.claimState != claimState) {
                return false;
            }
        }

        return true;
    }

    void OnApplicationStateChange(const ApplicationInfo* old,
                                  const ApplicationInfo* updated)
    {
#if 1
        cout << "  Application updated:" << endl;
        cout << "  ====================" << endl;
        cout << "  Application name :" << updated->appName << endl;
        cout << "  Hostname            :" << updated->deviceName << endl;
        cout << "  Busname            :" << updated->busName << endl;
        cout << "  - claim state     :" << ToString(old->claimState) << " --> " <<
        ToString(updated->claimState) <<
        endl;
        cout << "  - running state     :" << ToString(old->runningState) << " --> " <<
        ToString(updated->runningState) <<
        endl << "> " << flush;
#endif

        std::unique_lock<std::mutex> lk(m);
        appInfo[updated->busName] = *updated;
        cv.notify_one();
    }
};

static bool AutoAcceptManifest(const AuthorizationData& manifest)
{
    return true;
}

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

        ajn::securitymgr::SecurityManagerFactory& secFac = ajn::securitymgr::SecurityManagerFactory::GetInstance();

        ajn::securitymgr::StorageConfig sc;
        sc.settings["STORAGE_PATH"] = qcc::String(storage_path);
        assert(sc.settings.at("STORAGE_PATH").compare(storage_path) == 0);
        SecurityManager* secMgr = secFac.GetSecurityManager("hello", "world", sc, NULL);
        if (secMgr == NULL) {
            cerr << "No security manager" << endl;
            break;
        }

        secMgr->RegisterApplicationListener(&tal);

        cout << "Waiting for peers to become claimable " << endl;
        if (tal.WaitFor(ApplicationRunningState::RUNNING, ApplicationClaimState::CLAIMABLE, peers) == false) {
            break;
        }

        vector<ApplicationInfo> apps = secMgr->GetApplications();
        bool breakhit = false;
        for (ApplicationInfo app : apps) {
            if (app.runningState == ApplicationRunningState::RUNNING && app.claimState ==
                ApplicationClaimState::CLAIMABLE) {
                cout << "Trying to claim " << app.busName.c_str() << endl;
                if (secMgr->ClaimApplication(app, &AutoAcceptManifest) != ER_OK) {
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
        if (tal.WaitFor(ApplicationRunningState::RUNNING, ApplicationClaimState::CLAIMED, peers) == false) {
            break;
        }

        if (secMgr->GetApplications(ApplicationClaimState::CLAIMED).size() != peers) {
            cerr << "Expected: " << peers << " claimed applications but only have " <<
            secMgr->GetApplications().size() << endl;
            break;
        }

        GuildInfo guild;
        guild.guid = GUID128("E4DD81F54E7DB918EA5B2CE79D72200E");
        secMgr->StoreGuild(guild);

        PermissionPolicy policy;
        vector<GUID128> guildIds;
        guildIds.push_back(guild.guid);
        if (ER_OK != PolicyGenerator::DefaultPolicy(guildIds, policy)) {
            cerr << "Failed to generate policy." << endl;
            break;
        }

        apps = secMgr->GetApplications();
        for (ApplicationInfo app : apps) {
            if (app.runningState == ApplicationRunningState::RUNNING && app.claimState ==
                ApplicationClaimState::CLAIMED) {
                cout << "Trying to install membership certificate on " << app.busName.c_str() << endl;
                if (secMgr->InstallMembership(app, guild) != ER_OK) {
                    cerr << "Could not install membership certificate on " << app.busName.c_str() << endl;
                    breakhit = true;
                    break;
                }
                if (secMgr->InstallPolicy(app, policy) != ER_OK) {
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

    delete secMgr;
    ba.Disconnect();
    ba.Stop();
    ba.Join();

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
    }
    int secmgrs = 1;

    vector<pid_t> children(peers + secmgrs);

    for (size_t i = 0; i < children.size(); ++i) {
        if ((children[i] = fork()) == 0) {
            cout << "pid = " << getpid() << endl;
            if (i < (size_t)secmgrs) {
                return be_secmgr(peers);
            } else {
                /* TODO: REMOVE THIS WHEN RACE-CONDITION WITH APPLICATIONINFO LISTENER REGISTRATION IS REMOVED */
                std::chrono::milliseconds dura(1000);
                std::this_thread::sleep_for(dura);
                return be_peer();
            }
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
