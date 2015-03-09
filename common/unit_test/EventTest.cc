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

void RunEventTest(uint32_t instances, uint32_t signalIndex, uint32_t delayMs, uint32_t timeoutMs)
{
    if (signalIndex < instances) {
        /* Because timed event doesn't have a real OS event handle underneath, add 1 to account for it in our test */
        instances++;
    }

    Timespec ts1;
    GetTimeNow(&ts1);

    std::vector<Event*> checkEvents;
    for (uint32_t i = 0; i < instances; ++i) {
        Event* event = nullptr;
        if (i == signalIndex) {
            /* Timed event */
            event = new Event(delayMs);
        } else {
            event = new Event();
        }
        checkEvents.push_back(event);
    }

    std::vector<Event*> signalEvents;
    QStatus status = Event::Wait(checkEvents, signalEvents, timeoutMs);

    Timespec ts2;
    GetTimeNow(&ts2);
    uint32_t waitReturnTimeMs = ts2.GetAbsoluteMillis() - ts1.GetAbsoluteMillis();

    if (timeoutMs < delayMs) {
        /* Expecting timeout */
        ASSERT_EQ(ER_TIMEOUT, status);
        ASSERT_EQ(0U, signalEvents.size());
        ASSERT_LE(timeoutMs, waitReturnTimeMs);
    } else {
        /* Expecting an event */
        ASSERT_EQ(ER_OK, status);
        ASSERT_EQ(1U, signalEvents.size());
        ASSERT_EQ(checkEvents[signalIndex], signalEvents[0]);
        ASSERT_LE(delayMs, waitReturnTimeMs);
        ASSERT_GT(timeoutMs, waitReturnTimeMs);
    }

    /* Clean up */
    for (auto event : checkEvents) {
        delete event;
    }
}

const uint32_t T1 = 1000;
const uint32_t T2 = 2000;

TEST(EventTest, Below64Handles)
{
    RunEventTest(1, 0, T1, T2);
    RunEventTest(63, 62, T1, T2);
}

TEST(EventTest, Exactly64Handles)
{
    RunEventTest(64, 63, T1, T2);
}

TEST(EventTest, Above64Handles)
{
    RunEventTest(1000, 999, T1, T2);
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
    RunEventTest(1000, 1, T2, T1);
}
