/******************************************************************************
 * Copyright (c) 2011,2014 AllSeen Alliance. All rights reserved.
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

#include <qcc/Timer.h>
#include <Status.h>

using namespace std;
using namespace qcc;

#define __STDC_FORMAT_MACROS

static std::deque<std::pair<QStatus, Alarm> > triggeredAlarms;
static Mutex triggeredAlarmsLock;

static bool testNextAlarm(const Timespec& expectedTime, void* context)
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
        Timespec ts;
        GetTimeNow(&ts);
        uint64_t alarmTime = ts.GetAbsoluteMillis();
        uint64_t expectedTimeMs = expectedTime.GetAbsoluteMillis();
        ret = (p.first == ER_OK) && (context == p.second->GetContext()) && (alarmTime >= expectedTimeMs) && (alarmTime < (expectedTimeMs + jitter));
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
    MyAlarmListener(uint32_t delay) : AlarmListener(), delay(delay)
    {
    }
    void AlarmTriggered(const Alarm& alarm, QStatus reason)
    {
        triggeredAlarmsLock.Lock();
        triggeredAlarms.push_back(pair<QStatus, Alarm>(reason, alarm));
        triggeredAlarmsLock.Unlock();
        qcc::Sleep(delay);
    }
  private:
    const uint32_t delay;
};


TEST(TimerTest, SingleThreaded) {
    Timer t1("testTimer");
    Timespec ts;
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
    Timespec ts;
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
    Timespec ts;
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
