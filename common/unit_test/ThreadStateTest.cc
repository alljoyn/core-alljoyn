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

#include <qcc/ThreadState.h>
#include <qcc/Thread.h>
#include <unistd.h>

using namespace std;
using namespace qcc;

static volatile int32_t ReturnOk = 0;
static volatile int32_t ReturnAlreadyHandled = 0;

class ThreadsStateTest : public::testing::Test {
  public:
    ThreadState* threadStateUnderTest;

    ThreadsStateTest() :
        threadStateUnderTest(nullptr)
    { }

    virtual void SetUp()
    {
        threadStateUnderTest = new ThreadState();
    }

    virtual void TearDown()
    {
        delete threadStateUnderTest;

        while (ReturnOk != 0) {
            ReturnOk = DecrementAndFetch(&ReturnOk);
        }
        while (ReturnAlreadyHandled != 0) {
            ReturnAlreadyHandled = DecrementAndFetch(&ReturnAlreadyHandled);
        }
    }

  private:

};

TEST_F(ThreadsStateTest, goIntoErrorStateTest)
{
    EXPECT_EQ(ThreadState::INITIAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->error());
    EXPECT_EQ(ThreadState::CRITICALERROR, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Error, threadStateUnderTest->start());
}

TEST_F(ThreadsStateTest, stateChangeTest)
{
    /** initial state **/
    EXPECT_EQ(ThreadState::INITIAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Error, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::InInitialState, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::Error, threadStateUnderTest->stopped());
    EXPECT_EQ(ThreadState::Error, threadStateUnderTest->join());
    EXPECT_EQ(ThreadState::Error, threadStateUnderTest->joined());

    /** trigger start **/
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::AlreadyRunning, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::STARTING, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::AlreadyRunning, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::AlreadyRunning, threadStateUnderTest->started());

    /** started/runing state **/
    EXPECT_EQ(ThreadState::RUNNING, threadStateUnderTest->getCurrentState());

    /** trigger stop  **/
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::STOPPING, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->stopped());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->stopped());

    /** check stopped state *   */
    EXPECT_EQ(ThreadState::STOPPED, threadStateUnderTest->getCurrentState());

    /** trigger join  **/
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->join());
    EXPECT_EQ(ThreadState::JOINING, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->stopped());

    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->joined());
    EXPECT_EQ(ThreadState::DEAD, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->stopped());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->join());
    EXPECT_EQ(ThreadState::AlreadyJoined, threadStateUnderTest->joined());

    /** trigger restart from dead**/
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->start());
    /** check starting state**/
    EXPECT_EQ(ThreadState::STARTING, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->started());
    /** check whether restarted again **/
    EXPECT_EQ(ThreadState::RUNNING, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->stopped());

    /** trigger restart from stop **/
    /** expected result :: please check QStatus Thread::Start(void* arg, ThreadListener* listener)*/
    EXPECT_EQ(ThreadState::AlreadyStopped, threadStateUnderTest->start());
}

static void handleReturnValues(ThreadState::Rc rc)
{
    if (rc == ThreadState::StopAlreadyHandled ||
        rc == ThreadState::JoinAlreadyHandled) {
        IncrementAndFetch(&ReturnAlreadyHandled);
    } else if (rc == ThreadState::Ok) {
        IncrementAndFetch(&ReturnOk);
    }
}

ThreadReturn delayedStarter(void* arg)
{
    usleep(10 * 1000);
    if (arg != NULL) {
        ThreadState* threadState(static_cast<ThreadState*>(arg));
        threadState->started();
    }
    return NULL;
}

ThreadReturn stopperMain(void* arg)
{
    if (arg != NULL) {
        ThreadState* threadState(static_cast<ThreadState*>(arg));
        handleReturnValues(threadState->stop());
    }
    return NULL;
}

TEST_F(ThreadsStateTest, callStopWhenStarting)
{
    EXPECT_EQ(ThreadState::INITIAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::STARTING, threadStateUnderTest->getCurrentState());

    qcc::Thread t2("stopperMain", stopperMain);
    t2.Start(threadStateUnderTest);

    qcc::Thread t1("delayedStarter", delayedStarter);
    t1.Start(threadStateUnderTest);

    ThreadState::Rc rc = threadStateUnderTest->stop();
    handleReturnValues(rc);

    t1.Stop();
    t1.Join();
    t2.Stop();
    t2.Join();

    EXPECT_EQ(1, ReturnAlreadyHandled);
    EXPECT_EQ(1, ReturnOk);
}

ThreadReturn delayedStartStopper(void* arg)
{
    usleep(10 * 1000);
    if (arg != NULL) {
        ThreadState* threadState(static_cast<ThreadState*>(arg));
        handleReturnValues(threadState->started());
        handleReturnValues(threadState->stop());
        handleReturnValues(threadState->stopped());
    }
    return NULL;
}

ThreadReturn joinerMain(void* arg)
{
    if (arg != NULL) {
        ThreadState* threadState(static_cast<ThreadState*>(arg));
        handleReturnValues(threadState->join());
    }
    return NULL;
}

TEST_F(ThreadsStateTest, callJoinWhenStarting)
{
    EXPECT_EQ(ThreadState::INITIAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::STARTING, threadStateUnderTest->getCurrentState());

    qcc::Thread t2("joinerMain", joinerMain);
    t2.Start(threadStateUnderTest);

    qcc::Thread t1("delayedStartStopper", delayedStartStopper);
    t1.Start(threadStateUnderTest);

    ThreadState::Rc rc = threadStateUnderTest->join();
    handleReturnValues(rc);

    t1.Stop();
    t1.Join();
    t2.Stop();
    t2.Join();

    EXPECT_EQ(1, ReturnAlreadyHandled);
    EXPECT_EQ(4, ReturnOk);
}

class ExternalThreadsStateTest : public::testing::Test {
  public:
    ThreadState* threadStateUnderTest;

    ExternalThreadsStateTest() :
        threadStateUnderTest(nullptr)
    { }

    virtual void SetUp()
    {
        threadStateUnderTest = new ThreadState(true);
    }

    virtual void TearDown()
    {
        delete threadStateUnderTest;
    }
};

TEST_F(ExternalThreadsStateTest, stateChangeTest)
{
    EXPECT_EQ(ThreadState::EXTERNAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::IsExternalThread, threadStateUnderTest->start());
    EXPECT_EQ(ThreadState::EXTERNAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::IsExternalThread, threadStateUnderTest->started());
    EXPECT_EQ(ThreadState::IsExternalThread, threadStateUnderTest->stop());
    EXPECT_EQ(ThreadState::IsExternalThread, threadStateUnderTest->stopped());
    EXPECT_EQ(ThreadState::Error, threadStateUnderTest->joined());
    EXPECT_EQ(ThreadState::EXTERNAL, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->join());
    EXPECT_EQ(ThreadState::EXTERNALJOINING, threadStateUnderTest->getCurrentState());
    EXPECT_EQ(ThreadState::Ok, threadStateUnderTest->joined());
    EXPECT_EQ(ThreadState::EXTERNALJOINED, threadStateUnderTest->getCurrentState());
}

