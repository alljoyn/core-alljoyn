/******************************************************************************
 *
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

#include <qcc/Condition.h>
#include <qcc/IODispatch.h>

using namespace qcc;

class IODispatchStopStreamTest : public testing::Test {
  public:
    class Listener : public IOReadListener, public IOWriteListener, public IOExitListener {
      public:
        Mutex mutex;
        Condition condition;
        bool exitCalled;
        bool returnFromExit;

        Listener() : exitCalled(false), returnFromExit(false) { }
        virtual ~Listener() { }
        virtual QStatus ReadCallback(Source&, bool) { return ER_OK; }
        virtual QStatus WriteCallback(Sink&, bool) { return ER_OK; }
        virtual void ExitCallback() {
            mutex.Lock();
            exitCalled = true;
            condition.Signal();
            mutex.Unlock();

            mutex.Lock();
            while (!returnFromExit) {
                condition.Wait(mutex);
            }
            mutex.Unlock();
        }
        void WaitForExitCallback() {
            mutex.Lock();
            while (!exitCalled) {
                condition.Wait(mutex);
            }
            mutex.Unlock();
        }
        void ReturnFromExitCallback() {
            mutex.Lock();
            returnFromExit = true;
            condition.Signal();
            mutex.Unlock();
        }
    };

    Stream s;
    Listener l;
    IODispatch io;

    IODispatchStopStreamTest() : io("IODispatchStopStreamTest", 4) { }
};

TEST_F(IODispatchStopStreamTest, WhenStreamIsNotStarted)
{
    EXPECT_EQ(ER_INVALID_STREAM, io.StopStream(&s));
}

TEST_F(IODispatchStopStreamTest, WhenExitCallbackIsRunningOrScheduled)
{
    EXPECT_EQ(ER_OK, io.Start());
    EXPECT_EQ(ER_OK, io.StartStream(&s, &l, &l, &l));
    EXPECT_EQ(ER_OK, io.StopStream(&s));
    l.WaitForExitCallback();

    EXPECT_EQ(ER_FAIL, io.StopStream(&s));
    l.ReturnFromExitCallback();
}

TEST_F(IODispatchStopStreamTest, WhenExitCallbackIsRunningOrScheduled_Stop)
{
    EXPECT_EQ(ER_OK, io.Start());
    EXPECT_EQ(ER_OK, io.StartStream(&s, &l, &l, &l));
    EXPECT_EQ(ER_OK, io.Stop());
    l.WaitForExitCallback();

    EXPECT_EQ(ER_FAIL, io.StopStream(&s));
    l.ReturnFromExitCallback();
}

TEST_F(IODispatchStopStreamTest, WhenStreamIsStarted)
{
    EXPECT_EQ(ER_OK, io.Start());
    EXPECT_EQ(ER_OK, io.StartStream(&s, &l, &l, &l));

    EXPECT_EQ(ER_OK, io.StopStream(&s));
    l.WaitForExitCallback();
    l.ReturnFromExitCallback();
}

TEST_F(IODispatchStopStreamTest, WhenStreamIsStarted_Stop)
{
    EXPECT_EQ(ER_OK, io.Start());
    EXPECT_EQ(ER_OK, io.StartStream(&s, &l, &l, &l));

    EXPECT_EQ(ER_OK, io.Stop()); /* This calls StopStream internally. */
    l.WaitForExitCallback();
    l.ReturnFromExitCallback();
}