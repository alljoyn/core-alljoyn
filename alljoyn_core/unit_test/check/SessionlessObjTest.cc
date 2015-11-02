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

#include "SessionlessObj.h"

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "../ajTestCommon.h"

using namespace std;
using namespace qcc;
using namespace ajn;

namespace qcc {
void PrintTo(const Timespec<MonotonicTime>& ts, ::std::ostream* os) {
    *os << ts.seconds << '.' << setfill('0') << setw(3) << ts.mseconds;
}
};

namespace ajn {
void PrintTo(const SessionlessObj::BackoffLimits& p, ::std::ostream* os) {
    *os << "T=" << p.periodMs << ",k=" << p.linear << ",c=" << p.exponential << ",R=" << p.maxSecs;
}
void PrintTo(const SessionlessObj::WorkType& work, ::std::ostream* os) {
    static const char* workString[] = { "NONE", "APPLY_NEW_RULES", "REQUEST_NEW_SIGNALS" };
    *os << workString[work];
}
};

class SessionlessBackoffAlgorithmTest : public testing::TestWithParam<SessionlessObj::BackoffLimits> {
  public:
    static const bool doInitialBackoff = true;
};

TEST_P(SessionlessBackoffAlgorithmTest, Backoff)
{
    const SessionlessObj::BackoffLimits backoff = GetParam();
    uint32_t T = backoff.periodMs;
    uint32_t k = backoff.linear;
    uint32_t c = backoff.exponential;
    uint32_t R = backoff.maxSecs;

    Timespec<MonotonicTime> first, next;
    uint32_t i = 0, j;
    Timespec<MonotonicTime> lo, hi;

    /* Initial backoff (T) */
    SessionlessObj::GetNextJoinTime(backoff, doInitialBackoff, i++, first, next);
    ASSERT_LE(first, next); ASSERT_LT(next, first + T);

    /* Begin linear backoff (k) */
    while (i <= k) {
        lo = first + T;
        for (j = 1; j < i; ++j) {
            lo += j * T;
        }
        hi = lo + i * T;
        SessionlessObj::GetNextJoinTime(backoff, doInitialBackoff, i++, first, next);
        ASSERT_LE(lo, next); ASSERT_LT(next, hi);
    }

    /* Continue with exponential backoff (c) */
    for (j = k; j < c; j *= 2) {
        lo = hi;
        hi += j * 2 * T;
        SessionlessObj::GetNextJoinTime(backoff, doInitialBackoff, i++, first, next);
        ASSERT_LE(lo, next); ASSERT_LT(next, hi);
    }

    /* Constant retry period until R reached */
    while (SessionlessObj::GetNextJoinTime(backoff, doInitialBackoff, i++, first, next) == ER_OK) {
        lo = hi;
        hi += c * T;
        ASSERT_LE(lo, next); ASSERT_LT(next, hi);
    }

    /* Done after R sec */
    ASSERT_GT(next - first, R * 1000);
}

INSTANTIATE_TEST_CASE_P(SessionlessBackoff, SessionlessBackoffAlgorithmTest,
                        ::testing::Values(SessionlessObj::BackoffLimits(1500, 4, 32, 120),
                                          SessionlessObj::BackoffLimits(1500, 5, 32, 120),
                                          SessionlessObj::BackoffLimits(1500, 2, 16, 120)));

#if GTEST_HAS_COMBINE

typedef::testing::TestWithParam<tuple<bool, bool, bool, bool, bool> > TestParamTuple;

class SessionlessPendingWorkTest : public TestParamTuple {
  public:
    virtual void SetUp()
    {
        haveReceived = get<0>(GetParam());
        haveNewRule = get<1>(GetParam());
        haveNewChangeId = get<2>(GetParam());
        newRuleMatchesInterface = get<3>(GetParam());
        oldRuleMatchesInterface = get<4>(GetParam());
    }
    bool haveReceived;
    bool haveNewRule;
    bool haveNewChangeId;
    bool newRuleMatchesInterface;
    bool oldRuleMatchesInterface;
};

TEST_P(SessionlessPendingWorkTest, PendingWork)
{
    SessionlessObj::TimestampedRules rules;
    uint32_t nextRulesId = 0;
    Rule oldRule(oldRuleMatchesInterface ? "interface='org.alljoyn.About'" : "interface='org.oldRule'");
    rules.insert(std::pair<String, SessionlessObj::TimestampedRule>
                     (":test.2", SessionlessObj::TimestampedRule(oldRule, nextRulesId++)));
    if (haveNewRule) {
        Rule newRule(newRuleMatchesInterface ? "interface='org.alljoyn.About'" : "interface='org.newRule'");
        rules.insert(std::pair<String, SessionlessObj::TimestampedRule>
                         (":test.2", SessionlessObj::TimestampedRule(newRule, nextRulesId++)));
    }

    SessionlessObj::RemoteCache cache("org.alljoyn.sl.y2VZ0CWRc.x0", 1 /* version */, "2VZ0CWRc",
                                      "org.alljoyn.About", 0 /* changeId */, TRANSPORT_UDP);
    if (haveReceived) {
        cache.haveReceived = haveReceived;
        cache.receivedChangeId = cache.changeId;
        cache.appliedRulesId = 0;
    }
    if (haveNewChangeId) {
        ++cache.changeId;
    }

    SessionlessObj::WorkType expectedWork = SessionlessObj::NONE;
    if (haveReceived && haveNewRule && newRuleMatchesInterface) {
        expectedWork = SessionlessObj::APPLY_NEW_RULES;
    } else if ((!haveReceived || haveNewChangeId) && ((haveNewRule && newRuleMatchesInterface) || oldRuleMatchesInterface)) {
        expectedWork = SessionlessObj::REQUEST_NEW_SIGNALS;
    }
    EXPECT_EQ(expectedWork, SessionlessObj::PendingWork(cache, rules, nextRulesId));
}

INSTANTIATE_TEST_CASE_P(SessionlessPendingWork,
                        SessionlessPendingWorkTest,
                        ::testing::Combine(::testing::Bool(),   /* haveReceived */
                                           ::testing::Bool(),   /* haveNewRule */
                                           ::testing::Bool(),   /* haveNewChangeId */
                                           ::testing::Bool(),   /* newRuleMatchesInterface */
                                           ::testing::Bool())); /* oldRuleMatchesInterface */

#endif /* GTEST_HAS_COMBINE */
