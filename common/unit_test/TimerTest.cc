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

#include <deque>

#include <qcc/Thread.h>
#include <qcc/Timer.h>
#include <Status.h>

using namespace std;
using namespace qcc;

#define __STDC_FORMAT_MACROS

static std::deque<std::pair<QStatus, Alarm> > triggeredAlarms;
static Mutex triggeredAlarmsLock;

static bool testNextAlarm(const Timespec<MonotonicTime>& expectedTime, void* context)
{
    static const int jitter = 100;

    bool ret = false;
    triggeredAlarmsLock.Lock();
    uint64_t startTime = GetTimestamp64();

    // wait up to 20 seconds for an alarm to go off
    while (triggeredAlarms.empty() && (GetTimestamp() < (startTime + 20000))) {
        triggeredAlarmsLock.Unlock();
        qcc::Sleep(5);
        triggeredAlarmsLock.Lock();
    }

    // wait up to 20 seconds!
    if (!triggeredAlarms.empty()) {
        pair<QStatus, Alarm> p = triggeredAlarms.front();
        triggeredAlarms.pop_front();
        Timespec<MonotonicTime> ts;
        GetTimeNow(&ts);
        uint64_t alarmTime = ts.GetMillis();
        uint64_t expectedTimeMs = expectedTime.GetMillis();
        ret = (p.first == ER_OK) && (context == p.second->GetContext()) && (alarmTime + QCC_TIMESTAMP_GRANULARITY >= expectedTimeMs) && (alarmTime < (expectedTimeMs + jitter));
        if (!ret) {
            printf("Failed Triggered Alarm: status=%s, \na.alarmTime=\t%" PRIu64 "\nexpectedTimeMs=\t%" PRIu64 "\ndiff=\t\t%" PRIu64 "\n",
                   QCC_StatusText(p.first), alarmTime, expectedTimeMs, (expectedTimeMs - alarmTime));
        }
    }
    triggeredAlarmsLock.Unlock();
    return ret;
}

class MyAlarmListener : public AlarmListener {
  public:
    MyAlarmListener(uint32_t delay, Timer* callEnableReentrancyInCallback = NULL) :
        AlarmListener(), delay(delay), callEnableReentrancyInCallback(callEnableReentrancyInCallback)
    {
    }
    void AlarmTriggered(const Alarm& alarm, QStatus reason)
    {
        if (callEnableReentrancyInCallback != NULL) {
            callEnableReentrancyInCallback->EnableReentrancy();
        }
        triggeredAlarmsLock.Lock();
        triggeredAlarms.push_back(pair<QStatus, Alarm>(reason, alarm));
        triggeredAlarmsLock.Unlock();
        qcc::Sleep(delay);
    }
  private:
    /* Private assigment operator - does nothing */
    MyAlarmListener operator=(const MyAlarmListener&);
    const uint32_t delay;
    Timer* callEnableReentrancyInCallback;
};

class MyAlarmListener2 : public AlarmListener {
  public:
    MyAlarmListener2(uint32_t delay) : AlarmListener(), delay(delay), first(true)
    {
    }
    void AlarmTriggered(const Alarm& alarm, QStatus reason)
    {
        triggeredAlarmsLock.Lock();
        triggeredAlarms.push_back(pair<QStatus, Alarm>(reason, alarm));
        triggeredAlarmsLock.Unlock();
        if (first == true) {
            qcc::Sleep(delay);
            first = false;
        }
    }
  private:
    /* Private assigment operator - does nothing */
    MyAlarmListener2 operator=(const MyAlarmListener2&);
    const uint32_t delay;
    bool first;
};

TEST(TimerTest, SingleThreaded) {
    Timer t1("testTimer");
    Timespec<MonotonicTime> ts;
    QStatus status = t1.Start();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    MyAlarmListener alarmListener(1);
    AlarmListener* al = &alarmListener;

    /* Simple relative alarm */
    void* context = (void*) 0x12345678;
    uint32_t timeout = 1000;
    uint32_t zero = 0;
    GetTimeNow(&ts);
    Alarm a1(timeout, al, context, zero);

    status = t1.AddAlarm(a1);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    ASSERT_TRUE(testNextAlarm(ts + timeout, context));

    /* Recurring simple alarm */
    void* vptr = NULL;
    GetTimeNow(&ts);
    Alarm a2(timeout, al, vptr, timeout);
    status = t1.AddAlarm(a2);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    ASSERT_TRUE(testNextAlarm(ts + 1000, 0));
    ASSERT_TRUE(testNextAlarm(ts + 2000, 0));
    ASSERT_TRUE(testNextAlarm(ts + 3000, 0));
    ASSERT_TRUE(testNextAlarm(ts + 4000, 0));
    t1.RemoveAlarm(a2);

    /* Stop and Start */
    status = t1.Stop();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    status = t1.Join();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    status = t1.Start();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    status = t1.Stop();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    status = t1.Join();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
}



TEST(TimerTest, TestMultiThreaded) {
    Timespec<MonotonicTime> ts;
    GetTimeNow(&ts);
    QStatus status;
    MyAlarmListener alarmListener(5000);
    AlarmListener* al = &alarmListener;

    /* Test concurrency */
    Timer t2("testTimer", true, 3);
    status = t2.Start();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);


    uint32_t one = 1;
    GetTimeNow(&ts);

    Alarm a3(one, al);
    status = t2.AddAlarm(a3);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    Alarm a4(one, al);
    status = t2.AddAlarm(a4);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    Alarm a5(one, al);
    status = t2.AddAlarm(a5);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    Alarm a6(one, al);
    status = t2.AddAlarm(a6);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    Alarm a7(one, al);
    status = t2.AddAlarm(a7);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    Alarm a8(one, al);
    status = t2.AddAlarm(a8);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);


    ASSERT_TRUE(testNextAlarm(ts + 1, 0));
    ASSERT_TRUE(testNextAlarm(ts + 1, 0));
    ASSERT_TRUE(testNextAlarm(ts + 1, 0));

    ASSERT_TRUE(testNextAlarm(ts + 5001, 0));
    ASSERT_TRUE(testNextAlarm(ts + 5001, 0));
    ASSERT_TRUE(testNextAlarm(ts + 5001, 0));
}

TEST(TimerTest, TestReplaceTimer) {
    Timespec<MonotonicTime> ts;
    GetTimeNow(&ts);
    MyAlarmListener alarmListener(1);
    AlarmListener* al = &alarmListener;
    QStatus status;
    Timer t3("testTimer");
    status = t3.Start();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);


    uint32_t timeout = 2000;
    GetTimeNow(&ts);
    Alarm ar1(timeout, al);
    timeout = 5000;
    Alarm ar2(timeout, al);

    status = t3.AddAlarm(ar1);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    status = t3.ReplaceAlarm(ar1, ar2);
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    ASSERT_TRUE(testNextAlarm(ts + 5000, 0));
}

/*
 * This test verifies Timer's ability to stop by validating the number of callbacks made per following:
 *
 * 1. Schedule ten alarms to expire immediately (t0).
 * 2. Schedule ten alarms to expire 10 seconds from now (t2).
 * 3. Wait for 5 seconds (t1) and see how many callbacks are made. The expectation is as follow...
 *     a. ten t0 alarms fired with ER_OK result.
 *     b. ten t2 alarms fired with ER_TIMER_EXITING result (since we're using expireOnExit=true).
 * 4. Wait for additional 2 seconds (t3) to make sure no additional alarms are fired after stopped.
 */
TEST(TimerTest, TestStopTimer) {
    const uint32_t t0 = 0;
    const uint32_t t1 = 5000;
    const uint32_t t2 = 10000;
    const uint32_t t3 = 12000;
    const uint32_t maxAlarms = 20;

    QStatus status;
    MyAlarmListener alarmListener(0);
    AlarmListener* al = &alarmListener;
    Timer timer("testTimer", true, maxAlarms, false);
    status = timer.Start();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    /* reset the counts, just in case */
    triggeredAlarmsLock.Lock();
    triggeredAlarms.clear();
    triggeredAlarmsLock.Unlock();

    /* t0: add t0 alarms */
    for (uint32_t i = 0; (i < (maxAlarms / 2)); ++i) {
        Alarm alarm(t0, al);
        status = timer.AddAlarm(alarm);
        ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    }

    /* t0: add t2 alarms */
    for (uint32_t i = 0; (i < (maxAlarms / 2)); ++i) {
        Alarm alarm(t2, al);
        status = timer.AddAlarm(alarm);
        ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    }

    /* Wait for t1 to pass */
    qcc::Sleep(t1 - t0);

    /* t1: stop timer; also call Join() to make sure all threads run to completion */
    status = timer.Stop();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);
    status = timer.Join();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    /* Make sure there are ten t0 alarms fired with ER_OK and ten t2 alarms expired with ER_TIMER_EXITING */
    triggeredAlarmsLock.Lock();
    uint32_t alarmsFired = 0;
    uint32_t alarmsExpired = 0;
    while (!triggeredAlarms.empty()) {
        pair<QStatus, Alarm> p = triggeredAlarms.front();
        triggeredAlarms.pop_front();
        if (p.first == ER_OK) {
            ++alarmsFired;
        } else if (p.first == ER_TIMER_EXITING) {
            ++alarmsExpired;
        }
    }
    triggeredAlarms.clear();
    triggeredAlarmsLock.Unlock();
    ASSERT_EQ(maxAlarms / 2, alarmsFired);
    ASSERT_EQ(maxAlarms / 2, alarmsExpired);

    /* Wait for t3 to pass */
    qcc::Sleep(t3 - t1);

    /* t3: triggeredAlarms should still be zero */
    ASSERT_EQ(triggeredAlarms.size(), (size_t)0);
}

/*
 * This test verifies functionality of Timer::EnableReentrancy (called in alarm callback).
 * 1. Alarm (a1) is scheduled to fire immediately (t0) but takes 3s (t3) to complete.
 * 2. Alarm (a2) is scheduled to fire 1s (t1) from now.
 * 3. Alarm (a3) is scheduled to fire >1s (t2) from now.
 * 4. Schedule all 3 alarms together (with unlimited maxAlarms).
 * 5. See how many callbacks are made; expecting 1 so far for (a1).
 * 6. Wait for >1s (>t1) and see how many callbacks are made; expecting all 2 (a1 and a2).
 * 7. Wait for >4s (>t4) and see how many callbacks are made; expecting all 3.
 */
TEST(TimerTest, TestReentrancy) {
    const uint32_t t0 = 0;
    const uint32_t t1 = 1000;
    const uint32_t t2 = 1001;
    const uint32_t t3 = 3000;
    const uint32_t t4 = 4000;
    const uint32_t jitter = 500;
    /* Reset the counts */
    triggeredAlarmsLock.Lock();
    triggeredAlarms.clear();
    triggeredAlarmsLock.Unlock();

    QStatus status;
    Timer timer("testTimer", true, 8, true);
    status = timer.Start();
    ASSERT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status);

    MyAlarmListener listener0(t0, NULL);
    AlarmListener* al0 = &listener0;
    MyAlarmListener listener1(t1, NULL);
    AlarmListener* al1 = &listener1;
    /* listener3 calls Timer::EnableReentrancy in its callback */
    MyAlarmListener listener3(t3, &timer);
    AlarmListener* al3 = &listener3;

    Alarm a1(t0, al3);
    Alarm a2(t1, al1);
    Alarm a3(t2, al0);

    /* Schedule all 3 alarms together. */
    ASSERT_EQ(ER_OK, timer.AddAlarm(a1));
    ASSERT_EQ(ER_OK, timer.AddAlarm(a2));
    ASSERT_EQ(ER_OK, timer.AddAlarm(a3));

    /* Wait a tad bit and see how many callbacks are made; expecting 1 callbacks (sitting in MyAlarmListener callback) */
    qcc::Sleep(jitter);
    triggeredAlarmsLock.Lock();
    ASSERT_EQ(triggeredAlarms.size(), (size_t)1);
    triggeredAlarmsLock.Unlock();

    /* Wait for 1s (t1) and see how many callbacks are made; expecting 2 callbacks (as 3rd alarm should be serialized) */
    qcc::Sleep(t1);
    triggeredAlarmsLock.Lock();
    ASSERT_EQ(triggeredAlarms.size(), (size_t)2);
    triggeredAlarmsLock.Unlock();

    /* Wait for 4s (well after t3) and see how many callbacks are made; expecting all 3 */
    qcc::Sleep(t4 - t1);
    triggeredAlarmsLock.Lock();
    ASSERT_EQ(triggeredAlarms.size(), (size_t)3);
    triggeredAlarmsLock.Unlock();
}

