/**
 * @file
 *
 * This file implements qcc::Stream.
 */

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

#include <qcc/platform.h>

#include <qcc/String.h>
#include <qcc/Stream.h>
#include <qcc/time.h>

#include <Status.h>

#define QCC_MODULE "STREAM"

using namespace std;
using namespace qcc;

Source Source::nullSource;

volatile int32_t Stream::s_instanceCount = 0;
volatile uint64_t Stream::s_lastDestroyTimestamp = 0;

QStatus Source::GetLine(qcc::String& outStr, uint32_t timeout)
{
    QStatus status;
    uint8_t c;
    size_t actual;
    bool hasBytes = false;

    for (;;) {
        status = PullBytes(&c, 1, actual, timeout);
        if (ER_OK != status) {
            break;
        }
        hasBytes = true;
        if ('\r' == c) {
            continue;
        } else if ('\n' == c) {
            break;
        } else {
            outStr.push_back(c);
        }
    }
    return ((status == ER_EOF) && hasBytes) ? ER_OK : status;
}

void Stream::UpdateIdleInformation(bool isStarting)
{
    QCC_DbgPrintf(("UpdateIdleInformation(%u)", (uint32_t)isStarting));

    if (isStarting) {
        QCC_VERIFY(IncrementAndFetch(&s_instanceCount) > 0);
    } else {
        s_lastDestroyTimestamp = GetTimestamp64();
        QCC_VERIFY(DecrementAndFetch(&s_instanceCount) >= 0);
    }
}

bool AJ_CALL Stream::IsIdle(uint64_t minTime)
{
    /* Stream activity is considered idle if there are no stream objects
     * and no stream object has been destroyed during the minTime period.
     * Note that the idle state can transition while this method is running,
     * so the caller of this method has to be mindful about that race
     * condition.
     */
    if (s_instanceCount == 0) {
        uint64_t currentTimestamp = GetTimestamp64();
        uint64_t previousTimestamp = s_lastDestroyTimestamp;

        if (currentTimestamp >= previousTimestamp) {
            currentTimestamp -= previousTimestamp;

            if (currentTimestamp >= minTime) {
                return true;
            }
        }
    }

    return false;
}
