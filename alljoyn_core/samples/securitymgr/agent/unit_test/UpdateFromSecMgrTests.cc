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
enum UpdateStage { UPDATE_STARTED, UPDATE_COMPLETED };

class UpdatesFromSecMgrWrapper :
    public AgentStorageWrapper {
  public:
    UpdatesFromSecMgrWrapper(shared_ptr<AgentCAStorage>& _ca) :
        AgentStorageWrapper(_ca)
    {
    }

    bool WaitForStageUpdates(UpdateStage stage, OnlineApplication& app, size_t until)
    {
        lock.Lock();
        bool returnVal = true;
        do {
            switch (stage) {
            case UPDATE_STARTED: {
                    auto itr = appsStartedUpdating.find(app);
                    while (itr == appsStartedUpdating.end()) {
                        QStatus status = sem.TimedWait(lock, 10000);
                        if (ER_OK != status) {
                            printf("Timeout- failing test - %i\n", status);
                            goto Exit;
                        }
                        itr = appsStartedUpdating.find(app);
                    }
                    while (itr->second != until) {
                        QStatus status = sem.TimedWait(lock, 10000);
                        if (ER_OK != status) {
                            printf("Timeout- failing test - %i\n", status);
                            goto Exit;
                        }
                    }
                    itr = appsStartedUpdating.find(app);
                    lock.Unlock();
                    return returnVal;
                }

            case UPDATE_COMPLETED: {
                    auto itr = appsWithUpdatesCompleted.find(app);
                    while (itr == appsWithUpdatesCompleted.end()) {
                        QStatus status = sem.TimedWait(lock, 10000);
                        if (ER_OK != status) {
                            printf("Timeout- failing test - %i\n", status);
                            goto Exit;
                        }
                        itr = appsWithUpdatesCompleted.find(app);
                    }
                    while (itr->second != until) {
                        QStatus status = sem.TimedWait(lock, 10000);
                        if (ER_OK != status) {
                            printf("Timeout- failing test - %i\n", status);
                            goto Exit;
                        }
                    }
                    itr = appsWithUpdatesCompleted.find(app);
                    lock.Unlock();
                    return returnVal;
                }

            default:
                break;
            }
        } while (true);

    Exit:
        lock.Unlock();
        return false;
    }

    QStatus StartUpdates(Application& app, uint64_t& updateID)
    {
        lock.Lock();
        appsStartedUpdating[app]++;
        sem.Signal();
        lock.Unlock();
        return ca->StartUpdates(app, updateID);
    }

    QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        lock.Lock();
        appsWithUpdatesCompleted[app]++;
        sem.Signal();
        lock.Unlock();
        return ca->UpdatesCompleted(app, updateID);
    }

  public:
    map<Application, size_t> appsStartedUpdating;
    map<Application, size_t> appsWithUpdatesCompleted;

  private:
    UpdatesFromSecMgrWrapper& operator=(const UpdatesFromSecMgrWrapper);

    qcc::Mutex lock;
    qcc::Condition sem;
};

class UpdateFromSecmgrTest :
    public ClaimedTest {
  public:
    UpdateFromSecmgrTest() : wrappedCa(nullptr)
    {
    }

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

  public:
    shared_ptr<UpdatesFromSecMgrWrapper> wrappedCa;
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

    ASSERT_TRUE(wrappedCa->WaitForStageUpdates(UPDATE_STARTED, testAppInfo, 2)); // once by the auto-updater
    ASSERT_TRUE(wrappedCa->WaitForStageUpdates(UPDATE_COMPLETED, testAppInfo, 1));

    vector<OnlineApplication> appV;
    appV.push_back(testAppInfo);
    secMgr->UpdateApplications(&appV);

    ASSERT_TRUE(wrappedCa->WaitForStageUpdates(UPDATE_STARTED, testAppInfo, 3));
    ASSERT_TRUE(wrappedCa->WaitForStageUpdates(UPDATE_COMPLETED, testAppInfo, 2));
}
}
