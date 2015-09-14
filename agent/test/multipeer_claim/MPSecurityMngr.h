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

#ifndef ALLJOYN_SECMGR_MPSECURITYMNGR_H_
#define ALLJOYN_SECMGR_MPSECURITYMNGR_H_

#include <string>
#include <map>

#include <qcc/Thread.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/securitymgr/Manifest.h>
#include <alljoyn/securitymgr/ClaimListener.h>
#include <alljoyn/securitymgr/PolicyGenerator.h>
#include <alljoyn/securitymgr/SecurityAgentFactory.h>
#include <alljoyn/securitymgr/storage/StorageFactory.h>
#include "ProxyObjectManager.h"

using namespace std;
using namespace ajn;
using namespace ajn::securitymgr;
using namespace qcc;

namespace secmgr_tests {
class AutoAccepter :
    public ClaimListener {
    QStatus ApproveManifestAndSelectSessionType(ClaimContext& claimContext)
    {
        claimContext.ApproveManifest();
        claimContext.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL);
        return ER_OK;
    }
};

class MPSecurityMngr :
    ApplicationListener {
  public:
    /**
     * Creates a new MPSecurityMngr.
     */
    MPSecurityMngr();

    /**
     * Starts this MPSecurityMngr.
     */
    QStatus Start(int nrOfPeers);

    QStatus WaitUntilFinished();

    /*
     * Stops this MPSecurityMngr.
     */
    QStatus Stop();

    ~MPSecurityMngr();

    void OnApplicationStateChange(const OnlineApplication* old,
                                  const OnlineApplication* updated);

    void OnSyncError(const SyncError* syncError)
    {
        cout << "OnSyncError " << syncError->app.ToString() << ", type = " << syncError->type << ", status = " <<
            syncError->status << endl;
        errorFound = true;
    }

    void OnManifestUpdate(const ManifestUpdate* mf)
    {
        QCC_UNUSED(mf);
    }

    void ClaimApplications();

    void CheckApplicationUpdated(const OnlineApplication& app);

    //The appstate enum describes the states a peer has to go through
    typedef enum {
        DISCOVERED, //The application is discovered, waiting to start the test.
        CLAIMING, //Any new peer should signal itself in claimable state.
                  //When discovered the peer is put CLAIMING state and a Claim call will to it will be scheduled.
        CLAIMED, //Transition state: An application reports it is claimed and was still in claiming state.
                 //No Action taken. There is no guarantee a peer will enter this state.
        UPDATE_PENDING, //After the peer notices it has been claimed it should set it state to NEED_UPDATE.
                        //When the test sees a peer with NEED_UPDATE set in CLAIMED or CLAIMING state, then
                        // it should schedule an update task and move the state to UPDATE_PENDING
        UPDATING, //The agent code will send and event SYNC_PENDING. If the app is in UPDATE_PENDING state,
                  //Then we migrate to the UPDATING state. No action needed
        CHECK_UPDATE, //When SYNC_OK event is received and in UPDATING state, then we migrate to CHECK_UPDATE.
                      //An asynchronous task is started to validate the updates were indeed successful
        RESETTING, //When the asynchronous update validation task finishes, it will remove the peer.
                   //The state is updated to RESETTING
        DONE //When a event from a known application is received in RESETTING state, saying it is claimable,
             //then we consider the test done for that peer. The state is set to DONE.
    } AppState;

    const char* ToString(AppState st)
    {
        switch (st) {
        case DISCOVERED: return "Discovered";

        case CLAIMING: return "Claiming";

        case CLAIMED: return "Claimed";

        case UPDATE_PENDING: return "Update pending";

        case UPDATING: return "Updating";

        case CHECK_UPDATE: return "Checking updates";

        case RESETTING: return "Reseting";

        case DONE: return "Done";

        default:
            return "Not Known";
        }
    }

  private:
    void Reset();

    void UpdateApplication(const OnlineApplication& app);

    void DumpState();

    BusAttachment busAttachment;
    DefaultECDHEAuthListener authListener;
    shared_ptr<SecurityAgent> secMgr;
    shared_ptr<UIStorage> storage;
    shared_ptr<AgentCAStorage> agentCa;
    shared_ptr<PolicyGenerator> generator;
    shared_ptr<ProxyObjectManager> pomngr;
    Mutex appslock;
    Mutex actionLock;
    AutoAccepter aa;
    int peers;
    int peersFound;
    int peersClaimed;
    int peersReady;
    IdentityInfo idInfo;
    GroupInfo group;
    bool errorFound;
    map<OnlineApplication, AppState> apps;
};

class ASyncTask :
    public Thread {
  public:

    ASyncTask(const OnlineApplication& _app, MPSecurityMngr* _mgr, bool _do_claim) : app(_app), mgr(_mgr), do_claim(
            _do_claim)
    {
        Start();
    }

    virtual ThreadReturn STDCALL Run(void* arg)
    {
        QCC_UNUSED(arg);
        if (do_claim) {
            mgr->ClaimApplications();
        } else {
            mgr->CheckApplicationUpdated(app);
        }
        return nullptr;
    }

  private:
    OnlineApplication app;
    MPSecurityMngr* mgr;
    bool do_claim;
};
} // namespace
#endif //ALLJOYN_SECMGR_MPSECURITYMNGR_H_
