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

#include <gtest/gtest.h>
#include "TestUtil.h"
#include "AgentStorageWrapper.h"

#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

/** @file UpdateFromSecMgrTests.cc */

namespace secmgr_tests {
class UpdatesFromSecMgrWrapper :
    public AgentStorageWrapper {
  public:
    UpdatesFromSecMgrWrapper(shared_ptr<AgentCAStorage>& _ca) :
        AgentStorageWrapper(_ca)
    {
    }

    QStatus StartUpdates(Application& app, uint64_t& updateID)
    {
        lock.Lock();
        appsStartedUpdating[app]++;
        lock.Unlock();
        return ca->StartUpdates(app, updateID);
    }

    QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        lock.Lock();
        appsWithUpdatesCompleted[app]++;
        lock.Unlock();
        return ca->UpdatesCompleted(app, updateID);
    }

  public:
    map<Application, size_t> appsStartedUpdating;
    map<Application, size_t> appsWithUpdatesCompleted;

  private:
    UpdatesFromSecMgrWrapper& operator=(const UpdatesFromSecMgrWrapper);

    qcc::Mutex lock;
};

class UpdateFromSecmgrTest :
    public ClaimedTest {
  public:
    UpdateFromSecmgrTest() : wrappedCa(nullptr)
    {
    }

    enum UpdateStage { UPDATE_STATRED, UPDATE_COMPLETED };
    void TearDown()
    {
        ClaimedTest::TearDown();
    }

    shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        wrappedCa = shared_ptr<UpdatesFromSecMgrWrapper>(new UpdatesFromSecMgrWrapper(ca));
        ca = wrappedCa;
        return ca;
    }

    bool WaitForStageUpdates(UpdateStage stage, size_t numOfApps, size_t until)
    {
        lock.Lock();
        bool returnVal = true;
        map<Application, size_t>::const_iterator itr;
        QStatus status;
        do {
            switch (stage) {
            case UPDATE_STATRED:
                itr = wrappedCa->appsStartedUpdating.begin();
                while (itr != wrappedCa->appsStartedUpdating.end()) {
                    returnVal = returnVal && (itr->second == until);
                    itr++;
                }
                if (numOfApps != wrappedCa->appsStartedUpdating.size() || !returnVal) {
                    status = sem.TimedWait(lock, 10000);
                    if (ER_OK != status) {
                        printf("Timeout- failing test - %i\n", status);
                        goto Exit;
                    }
                } else {
                    lock.Unlock();
                    return returnVal;
                }
                break;

            case UPDATE_COMPLETED:
                itr = wrappedCa->appsWithUpdatesCompleted.begin();
                while (itr != wrappedCa->appsWithUpdatesCompleted.end()) {
                    returnVal = returnVal && (itr->second == until);
                    itr++;
                }
                if (numOfApps != wrappedCa->appsWithUpdatesCompleted.size() || !returnVal) {
                    status = sem.TimedWait(lock, 10000);
                    if (ER_OK != status) {
                        printf("Timeout- failing test - %i\n", status);
                        goto Exit;
                    }
                } else {
                    lock.Unlock();
                    return returnVal;
                }
                break;

            default:
                break;
            }
        } while (true);

    Exit:
        lock.Unlock();
        return false;
    }

  public:
    shared_ptr<UpdatesFromSecMgrWrapper> wrappedCa;
    qcc::Condition sem;
    qcc::Mutex lock;
};

/**
 * @test Ensure that an explicit update applications call from the security
 *       agent will trigger the correct logic in the application updater.
 *       -# Claim an application.
 *       -# Trigger update applications from the security agent for all apps.
 *       -# Ensure that the number of times update started and completed
 *          is correct.
 *       -# Trigger update applications from the security agent using a vector
 *          containing the claimed app.
 *       -# Ensure that the number of times update started and completed
 *          have been incremented.
 **/
TEST_F(UpdateFromSecmgrTest, BasicUpdateFromSecMgr) {
    secMgr->UpdateApplications();

    ASSERT_TRUE(WaitForStageUpdates(UPDATE_STATRED, 1, 2)); // once by the auto-updater
    ASSERT_TRUE(WaitForStageUpdates(UPDATE_COMPLETED, 1, 1));

    vector<OnlineApplication> appV;
    OnlineApplication app;
    app.keyInfo = wrappedCa->appsWithUpdatesCompleted.begin()->first.keyInfo;
    appV.push_back(app);
    secMgr->UpdateApplications(&appV);

    ASSERT_TRUE(WaitForStageUpdates(UPDATE_STATRED, 1, 3));
    ASSERT_TRUE(WaitForStageUpdates(UPDATE_COMPLETED, 1, 2));
}
}
