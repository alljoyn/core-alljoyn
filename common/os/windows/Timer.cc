/**
 * @file
 *
 * Implement Timer object
 */

/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include <qcc/Debug.h>
#include <qcc/Timer.h>
#include <qcc/StringUtil.h>
#include <Status.h>
#include <algorithm>
#include <list>

#define QCC_MODULE  "TIMER"

using namespace qcc;

namespace qcc {

class TimerImpl {
  public:
    TimerImpl(qcc::String name, bool expireOnExit, uint32_t concurrency);
    ~TimerImpl();

    QStatus Start();
    QStatus Stop();
    QStatus Join();
    QStatus AddAlarm(const Alarm& alarm);
    bool RemoveAlarm(const Alarm& alarm, bool blockIfTriggered = true);
    void RemoveAlarmsWithListener(const AlarmListener& listener);
    bool HasAlarm(const Alarm& alarm) const;
    bool IsRunning() const;
    const qcc::String& GetName() const;

  private:
    class _TimerContext {
        friend class TimerImpl;
      public:
        _TimerContext();
        _TimerContext(PTP_TIMER ptpTimer, const Alarm& alarm);
        bool operator==(const _TimerContext& other) const;
      private:
        PTP_TIMER ptpTimer;
        Alarm alarm;
    };
    typedef ManagedObj<_TimerContext> TimerContext;

    static void CALLBACK OnTimeout(
        _Inout_ PTP_CALLBACK_INSTANCE instance,
        _Inout_opt_ PVOID context,
        _Inout_ PTP_TIMER ptpTimer);

    String nameStr;
    const bool expireOnExit;
    const DWORD maxThreads;

    TP_CALLBACK_ENVIRON environment;
    PTP_POOL ptpPool;
    PTP_CLEANUP_GROUP ptpCleanupGroup;

    mutable Mutex timersLock;
    std::list<TimerContext> timers;
};

}

TimerImpl::_TimerContext::_TimerContext() : ptpTimer(NULL)
{
}

TimerImpl::_TimerContext::_TimerContext(PTP_TIMER ptpTimer, const Alarm& alarm) : ptpTimer(ptpTimer), alarm(alarm)
{
}

bool TimerImpl::_TimerContext::operator==(const TimerImpl::_TimerContext& other) const
{
    return (ptpTimer == other.ptpTimer);
}

TimerImpl::TimerImpl(qcc::String name, bool expireOnExit, uint32_t concurrency) :
    nameStr(name), expireOnExit(expireOnExit), maxThreads(concurrency), ptpPool(NULL), ptpCleanupGroup(NULL)
{
    InitializeThreadpoolEnvironment(&environment);
}

TimerImpl::~TimerImpl()
{
    Stop();
    DestroyThreadpoolEnvironment(&environment);
}

QStatus TimerImpl::Start()
{
    PTP_POOL ptpPoolLocal = CreateThreadpool(NULL);
    if (ptpPoolLocal == NULL) {
        QCC_LogError(ER_OS_ERROR, ("CreateThreadpool failed with OS error %d", GetLastError()));
        return ER_OS_ERROR;
    }
    PTP_CLEANUP_GROUP ptpCleanupGroupLocal = CreateThreadpoolCleanupGroup();
    if (ptpCleanupGroupLocal == NULL) {
        QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolCleanupGroup failed with OS error %d", GetLastError()));
        CloseThreadpool(ptpPoolLocal);
        return ER_OS_ERROR;
    }
    SetThreadpoolThreadMaximum(ptpPoolLocal, maxThreads);
    SetThreadpoolCallbackPool(&environment, ptpPoolLocal);
    SetThreadpoolCallbackCleanupGroup(&environment, ptpCleanupGroupLocal, NULL);
    ptpPool = ptpPoolLocal;
    ptpCleanupGroup = ptpCleanupGroupLocal;
    return ER_OK;
}

QStatus TimerImpl::Stop()
{
    if (ptpCleanupGroup != NULL) {
        CloseThreadpoolCleanupGroupMembers(ptpCleanupGroup, TRUE, NULL);
        CloseThreadpoolCleanupGroup(ptpCleanupGroup);
        ptpCleanupGroup = NULL;
    }
    if (ptpPool != NULL) {
        CloseThreadpool(ptpPool);
        ptpPool = NULL;
    }
    if (expireOnExit) {
        /* Copy the outstanding timers locally and fire the alarm */
        timersLock.Lock();
        std::list<TimerContext> timersLocal = timers;
        timers.clear();
        timersLock.Unlock();
        for (auto& timerLocal : timersLocal) {
            (timerLocal->alarm->listener->AlarmTriggered)(timerLocal->alarm, ER_TIMER_EXITING);
        }
        timersLocal.clear();
    }
    timers.clear();
    return ER_OK;
}

QStatus TimerImpl::Join()
{
    return ER_OK;
}

QStatus TimerImpl::AddAlarm(const Alarm& alarm)
{
    if (!IsRunning()) {
        return ER_TIMER_EXITING;
    }

    /* Create a new timer */
    PTP_TIMER ptpTimerLocal = CreateThreadpoolTimer(OnTimeout, this, &environment);
    if (ptpTimerLocal == NULL) {
        QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolTimer failed with OS error %d", GetLastError()));
        return ER_OS_ERROR;
    }

    /* Calculate the delay */
    Timespec now;
    GetTimeNow(&now);
    FILETIME fileTime = { 0 };
    int64_t delay = alarm->alarmTime.GetAbsoluteMillis() - now.GetAbsoluteMillis();
    if (delay > 0) {
        LARGE_INTEGER howLongToWait;
        howLongToWait.QuadPart = -10000i64 * delay;
        fileTime.dwLowDateTime = howLongToWait.LowPart;
        fileTime.dwHighDateTime = howLongToWait.HighPart;
    }

    /* Add the timer into the list */
    timersLock.Lock();
    timers.push_back(TimerContext(ptpTimerLocal, alarm));
    timersLock.Unlock();

    /* Schedule the timer */
    SetThreadpoolTimer(ptpTimerLocal, &fileTime, alarm->periodMs, 0);
    return ER_OK;
}

void TimerImpl::OnTimeout(
    _Inout_ PTP_CALLBACK_INSTANCE instance,
    _Inout_opt_ PVOID context,
    _Inout_ PTP_TIMER ptpTimerLocal)
{
    TimerImpl* timerImpl = reinterpret_cast<TimerImpl*>(context);
    /* Find the timer object corresponding to this callback */
    bool found = false;
    TimerContext timerLocal;
    timerImpl->timersLock.Lock();
    for (auto& timer : timerImpl->timers) {
        if (timer->ptpTimer == ptpTimerLocal) {
            found = true;
            timerLocal = timer;
            /* If this is a one-shot timer, remove it out of the master list */
            if (timer->alarm->periodMs == 0) {
                timerImpl->timers.remove(timer);
            }
            break;
        }
    }
    timerImpl->timersLock.Unlock();
    if (found) {
        /* Ensure the callback for the next alarm is speedy */
        CallbackMayRunLong(instance);
        (timerLocal->alarm->listener->AlarmTriggered)(timerLocal->alarm, ER_OK);
    }
}

bool TimerImpl::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    bool found = false;
    TimerContext timerLocal;
    if (IsRunning() || expireOnExit) {
        timersLock.Lock();
        for (auto& timer : timers) {
            if (timer->alarm == alarm) {
                found = true;
                timerLocal = timer;
                timers.remove(timer);
                break;
            }
        }
        timersLock.Unlock();
    }
    if (found) {
        SetThreadpoolTimer(timerLocal->ptpTimer, NULL, 0, 0);
        if (blockIfTriggered) {
            WaitForThreadpoolTimerCallbacks(timerLocal->ptpTimer, FALSE);
            CloseThreadpoolTimer(timerLocal->ptpTimer);
        }
    }
    return found;
}

void TimerImpl::RemoveAlarmsWithListener(const AlarmListener& listener)
{
    if (IsRunning() || expireOnExit) {
        bool found = false;
        TimerContext timerLocal;
        timersLock.Lock();
        for (auto& timer : timers) {
            if (timer->alarm->listener == &listener) {
                found = true;
                timerLocal = timer;
                timers.remove(timer);
                break;
            }
        }
        timersLock.Unlock();
        if (found) {
            SetThreadpoolTimer(timerLocal->ptpTimer, NULL, 0, 0);
        }
    }
}

bool TimerImpl::HasAlarm(const Alarm& alarm) const
{
    bool found = false;
    if (IsRunning()) {
        timersLock.Lock();
        for (auto& timer : timers) {
            if (timer->alarm == alarm) {
                found = true;
                break;
            }
        }
        timersLock.Unlock();
    }
    return found;
}

bool TimerImpl::IsRunning() const
{
    return (ptpPool != NULL);
}

const qcc::String& TimerImpl::GetName() const
{
    return nameStr;
}

Timer::Timer(String name, bool expireOnExit, uint32_t concurrency, bool preventReentrancy, uint32_t maxAlarms) :
    timerImpl(new TimerImpl(name, expireOnExit, concurrency))
{
    /* Timer thread objects will be created when required */
}

Timer::~Timer()
{
    Stop();
    delete timerImpl;
}

QStatus Timer::Start()
{
    return timerImpl->Start();
}

QStatus Timer::Stop()
{
    return timerImpl->Stop();
}

QStatus Timer::Join()
{
    return timerImpl->Join();
}

QStatus Timer::AddAlarm(const Alarm& alarm)
{
    return timerImpl->AddAlarm(alarm);
}

QStatus Timer::AddAlarmNonBlocking(const Alarm& alarm)
{
    return timerImpl->AddAlarm(alarm);
}

bool Timer::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    return timerImpl->RemoveAlarm(alarm, blockIfTriggered);
}

void Timer::RemoveAlarmsWithListener(const AlarmListener& listener)
{
    timerImpl->RemoveAlarmsWithListener(listener);
}

bool Timer::ForceRemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    return timerImpl->RemoveAlarm(alarm, blockIfTriggered);
}

QStatus Timer::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
{
    QStatus status = ER_NO_SUCH_ALARM;
    if (timerImpl->RemoveAlarm(origAlarm, blockIfTriggered)) {
        status = timerImpl->AddAlarm(newAlarm);
    }
    return status;
}

bool Timer::HasAlarm(const Alarm& alarm) const
{
    return timerImpl->HasAlarm(alarm);
}

bool Timer::IsRunning() const
{
    return timerImpl->IsRunning();
}

void Timer::EnableReentrancy()
{
}

bool Timer::IsHoldingReentrantLock() const
{
    return false;
}

