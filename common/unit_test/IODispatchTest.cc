/******************************************************************************
 *
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
