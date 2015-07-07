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
#include <qcc/Thread.h>

#include "ajTestCommon.h"

using namespace ajn;
using namespace qcc;

/* ASACORE-2144 */
class TestSessionListener : public SessionListener {
  public:
    bool sessionLost;
    TestSessionListener() : sessionLost(false) { }
    virtual void SessionLost(SessionId, SessionLostReason) {
        sessionLost = true;
    }
};

TEST(SessionlessObjTest, SessionLostIfRequestRangeNotSent) {
    TestSessionListener listener;
    BusAttachment bus("SessionlessObjTest", true);
    EXPECT_EQ(ER_OK, bus.Start());
    EXPECT_EQ(ER_OK, bus.Connect(getConnectArg().c_str()));

    String name = bus.GetUniqueName();
    name = name.substr(0, name.find_last_of('.')) + ".1";
    SessionId sid;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, bus.JoinSession(name.c_str(), 100, &listener, sid, opts));

    for (int i = 0; i < 5000 / 10; ++i) {
        if (listener.sessionLost) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(listener.sessionLost);

    EXPECT_EQ(ER_OK, bus.Stop());
    EXPECT_EQ(ER_OK, bus.Join());
}

