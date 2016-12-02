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
#include <gtest/gtest.h>

#include <qcc/Thread.h>

using namespace std;
using namespace qcc;

static const uint32_t s_testThreadWaitTimeMs = 1000;
static const int32_t s_multipleThreadsCount = 8;

#if defined(QCC_OS_GROUP_POSIX)
static void* ExternalThread(void*) {
#elif defined(QCC_OS_GROUP_WINDOWS)
static unsigned __stdcall ExternalThread(void*) {
#endif
    Thread* thread = Thread::GetThread();
    QCC_UNUSED(thread);
    return NULL;
}

class ThreadsSingleTest : public::testing::Test {
  public:
    bool taskFinished;
    Thread* threadUnderTest;

    ThreadsSingleTest() :
        taskFinished(false),
        threadUnderTest(nullptr)
    { }

    virtual void SetUp()
    {
        threadUnderTest = new Thread("threadTestFunction", WaitAndSetFlagTask);
    }

    virtual void TearDown()
    {
        delete threadUnderTest;
    }

  private:

    static ThreadReturn WaitAndSetFlagTask(void* arg)
    {
        bool* taskFinished = (bool*)arg;

        Sleep(s_testThreadWaitTimeMs);
        if (nullptr != taskFinished) {
            *taskFinished = true;
        }

        return nullptr;
    }
};

class ThreadsMultipleTest : public ThreadsSingleTest {
  public:
    ThreadsMultipleTest() : ThreadsSingleTest()
    { }

    virtual void SetUp()
    {
        ThreadsSingleTest::SetUp();

        ASSERT_GT(s_multipleThreadsCount, 1);
        m_multipleThreadsFinished = 0;
        for (int32_t i = 0; i < s_multipleThreadsCount; i++) {
            threadsCollections.push_back(new Thread("joinAnotherThreadTask", JoinAnotherThreadTask));
        }
    }

    virtual void TearDown()
    {
        for (Thread* singleThread : threadsCollections) {
            delete singleThread;
        }

        ThreadsSingleTest::TearDown();
    }

    void JoinFromMultipleThreads() {
        for (Thread* singleThread : threadsCollections) {
            ASSERT_EQ(ER_OK, singleThread->Start(threadUnderTest));
        }

        for (Thread* singleThread : threadsCollections) {
            ASSERT_EQ(ER_OK, singleThread->Join());
        }

        ASSERT_EQ(s_multipleThreadsCount, m_multipleThreadsFinished);
    }

  private:

    static volatile int32_t m_multipleThreadsFinished;

    static ThreadReturn JoinAnotherThreadTask(void* arg)
    {
        Thread* anotherThread = (Thread*)arg;

        EXPECT_NE(nullptr, anotherThread);
        EXPECT_EQ(ER_OK, anotherThread->Join());

        IncrementAndFetch(&m_multipleThreadsFinished);
        return nullptr;
    }

    vector<Thread*> threadsCollections;
};

volatile int32_t ThreadsMultipleTest::m_multipleThreadsFinished = 0;

/*
 * Test this with valgrind to see that no memory leaks occur with
 * external thread objects.
 */
TEST(ThreadTest, CleanExternalThread) {
#if defined(QCC_OS_GROUP_POSIX)
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, NULL, ExternalThread, NULL));
    ASSERT_EQ(0, pthread_join(tid, NULL));
#elif defined(QCC_OS_GROUP_WINDOWS)
    uintptr_t handle = _beginthreadex(NULL, 0, ExternalThread, NULL, 0, NULL);
    ASSERT_NE((uintptr_t)0, handle);
    ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(reinterpret_cast<HANDLE>(handle), INFINITE));
    ASSERT_NE(0, CloseHandle(reinterpret_cast<HANDLE>(handle)));
#endif
}

TEST_F(ThreadsSingleTest, shouldStartThreadWithoutErrors)
{
    EXPECT_EQ(ER_OK, threadUnderTest->Start());
}

TEST_F(ThreadsSingleTest, shouldJoinThreadWithoutErrors)
{
    ASSERT_EQ(ER_OK, threadUnderTest->Start());

    EXPECT_EQ(ER_OK, threadUnderTest->Join());
}

TEST_F(ThreadsSingleTest, shouldFinishTaskAfterJoinFromOneThread)
{
    ASSERT_FALSE(taskFinished);

    ASSERT_EQ(ER_OK, threadUnderTest->Start(&taskFinished));
    ASSERT_EQ(ER_OK, threadUnderTest->Join());

    EXPECT_TRUE(taskFinished);
}

TEST_F(ThreadsSingleTest, shouldNullifyHandleAfterJoinFromOneThread)
{
    ASSERT_EQ(ER_OK, threadUnderTest->Start());
    ASSERT_NE((ThreadHandle)nullptr, threadUnderTest->GetHandle());

    ASSERT_EQ(ER_OK, threadUnderTest->Join());

    EXPECT_EQ((ThreadHandle)nullptr, threadUnderTest->GetHandle());
}

TEST_F(ThreadsMultipleTest, shouldFinishTaskAfterJoinFromMultipleThreads)
{
    ASSERT_FALSE(taskFinished);

    ASSERT_EQ(ER_OK, threadUnderTest->Start(&taskFinished));
    JoinFromMultipleThreads();

    EXPECT_TRUE(taskFinished);
}

TEST_F(ThreadsMultipleTest, shouldNullifyHandleAfterJoinFromMultipleThreads)
{
    ASSERT_EQ(ER_OK, threadUnderTest->Start());
    ASSERT_NE((ThreadHandle)nullptr, threadUnderTest->GetHandle());

    JoinFromMultipleThreads();

    EXPECT_EQ((ThreadHandle)nullptr, threadUnderTest->GetHandle());
}