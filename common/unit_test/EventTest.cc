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

#include <qcc/Event.h>
#include <qcc/time.h>

using namespace std;
using namespace qcc;

void RunEventTest(uint32_t instances, uint32_t signalIndex, uint32_t delayMs, uint32_t timeoutMs, bool useTimedEvent = true)
{
    const uint32_t jitter = 500;
    Timespec<MonotonicTime> ts1;
    GetTimeNow(&ts1);

    std::vector<Event*> checkEvents;
    for (uint32_t i = 0; i < instances; ++i) {
        Event* event = nullptr;
        if (i == signalIndex) {
            if (useTimedEvent) {
                /* Timed event */
                event = new Event(delayMs);
            } else {
                /* General purpose event */
                event = new Event();
                event->SetEvent();
            }
        } else {
            event = new Event();
        }
        checkEvents.push_back(event);
    }

    std::vector<Event*> signalEvents;
    QStatus status = Event::Wait(checkEvents, signalEvents, timeoutMs);

    Timespec<MonotonicTime> ts2;
    GetTimeNow(&ts2);
    uint32_t waitReturnTimeMs = ts2.GetMillis() - ts1.GetMillis();

    if (timeoutMs < delayMs) {
        /* Expecting timeout */
        ASSERT_EQ(ER_TIMEOUT, status);
        ASSERT_EQ(0U, signalEvents.size());
        ASSERT_LE(timeoutMs, waitReturnTimeMs + QCC_TIMESTAMP_GRANULARITY);
    } else {
        /* Expecting an event */
        ASSERT_EQ(ER_OK, status);
        ASSERT_EQ(1U, signalEvents.size());
        ASSERT_EQ(checkEvents[signalIndex], signalEvents[0]);
        ASSERT_LE(delayMs, waitReturnTimeMs + QCC_TIMESTAMP_GRANULARITY);

        if ((timeoutMs > jitter) && (timeoutMs - jitter > delayMs)) {
            ASSERT_GT(timeoutMs, waitReturnTimeMs);
        }
    }

    /* Clean up */
    for (auto event : checkEvents) {
        delete event;
    }
}

const uint32_t T1 = 1000;
const uint32_t T2 = 2000;
/*
 * On darwin platform the number of instances above 256 will cause "Too many open files" error
 * due to number of file descriptors being limited to 256
 */
#if __MACH__
const uint32_t INSTANCES_DARWIN = 100;
const uint32_t SIGNAL_INDEX = 99;
#endif

/*
 * 64, 63 and similar number of events are significant for the Windows implementation.
 * When waiting for more than 64 events, the Windows implementation makes multiple
 * WaitForMultipleObject() calls, for up to 63 events at a time.
 */
TEST(EventTest, Below64Handles1)
{
    RunEventTest(2, 0, T1, T2);
}

TEST(EventTest, Below64Handles2)
{
    RunEventTest(63, 62, T1, T2);
}

TEST(EventTest, Exactly64Handles)
{
    RunEventTest(64, 63, T1, T2);
}

TEST(EventTest, Above64Handles1)
{
    RunEventTest(65, 64, T1, T2, true);
}

TEST(EventTest, Above64Handles2)
{
    RunEventTest(65, 64, 0, 0, false);
}

TEST(EventTest, Above64Handles3)
{
    RunEventTest(65, 64, 0, T1, false);
}

TEST(EventTest, Above64Handles4)
{
    RunEventTest(65, 63, T1, T2, true);
}

TEST(EventTest, Above64Handles5)
{
    RunEventTest(65, 63, 0, 0, false);
}

TEST(EventTest, Above64Handles6)
{
    RunEventTest(65, 63, 0, T1, false);
}

TEST(EventTest, Above64Handles7)
{
#if __MACH__
    RunEventTest(INSTANCES_DARWIN, SIGNAL_INDEX, T1, T2);
#else
    RunEventTest(1000, 999, T1, T2);
#endif
}

TEST(EventTest, Below64HandlesTO)
{
    RunEventTest(60, 0, T2, T1);
}

TEST(EventTest, Exactly64HandlesTO)
{
    RunEventTest(64, 0, T2, T1);
}

TEST(EventTest, Above64HandlesTO)
{
#if __MACH__
    RunEventTest(INSTANCES_DARWIN, 1, T2, T1);
#else
    RunEventTest(1000, 1, T2, T1);
#endif
}
