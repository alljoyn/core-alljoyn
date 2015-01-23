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
    TimerImpl(qcc::String name, bool expireOnExit, uint32_t concurrency, bool preventReentrancy, uint32_t maxAlarms);
    ~TimerImpl();

    QStatus Start();
    QStatus Stop();
    QStatus Join();
    QStatus AddAlarm(const Alarm& alarm, bool canBlock);
    bool RemoveAlarm(const Alarm& alarm, bool blockIfTriggered = true);
    void RemoveAlarmsWithListener(const AlarmListener& listener);
    bool HasAlarm(const Alarm& alarm) const;
    bool IsRunning() const;
    void EnableReentrancy();
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

    void AddTimerInternal(TimerContext timer);
    void RemoveTimerInternal(TimerContext timer);

    String nameStr;
    const bool expireOnExit;
    const DWORD maxThreads;
    const uint32_t maxAlarms;
    bool preventReentrancy;

    HANDLE timerStoppedEvent;
    HANDLE lightLoadEvent;

    TP_CALLBACK_ENVIRON environment;
    PTP_POOL ptpPool;
    PTP_CLEANUP_GROUP ptpCleanupGroup;

    Mutex reentrantLock;        /* This must not be acquired while timersLock is held (deadlock) */
    mutable Mutex timersLock;   /* Declared mutable because HasAlarm is const */
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

TimerImpl::TimerImpl(qcc::String name, bool expireOnExit, uint32_t concurrency, bool preventReentrancy, uint32_t maxAlarms) :
    nameStr(name), expireOnExit(expireOnExit), maxThreads(concurrency), ptpPool(NULL), ptpCleanupGroup(NULL), preventReentrancy(preventReentrancy), maxAlarms(maxAlarms),
    timerStoppedEvent(NULL), lightLoadEvent(NULL)
{
    InitializeThreadpoolEnvironment(&environment);
}

TimerImpl::~TimerImpl()
{
    Stop();
    DestroyThreadpoolEnvironment(&environment);
}

/* This function requires lock held by caller */
void TimerImpl::AddTimerInternal(TimerContext timer) {
    timers.push_back(timer);
    if ((lightLoadEvent != NULL) && (timers.size() >= maxAlarms)) {
        ResetEvent(lightLoadEvent);
    }
    /* This assert is to ensure maxAlarms is never exceeded (if it's specified) */
    assert((maxAlarms == 0) || (timers.size() <= maxAlarms));
}

/* This function requires lock held by caller */
void TimerImpl::RemoveTimerInternal(TimerContext timer)
{
    timers.remove(timer);
    if ((lightLoadEvent != NULL) && (timers.size() < maxAlarms)) {
        SetEvent(lightLoadEvent);
    }
}

QStatus TimerImpl::Start()
{
    if (IsRunning()) {
        Stop();
        assert(!IsRunning());
    }

    assert(timerStoppedEvent == NULL);
    timerStoppedEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
    if (timerStoppedEvent == NULL) {
        QCC_LogError(ER_OS_ERROR, ("CreateEvent failed with OS error %d", GetLastError()));
        return ER_OS_ERROR;
    }
    if (maxAlarms > 0) {
        assert(lightLoadEvent == NULL);
        lightLoadEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
        if (lightLoadEvent == NULL) {
            QCC_LogError(ER_OS_ERROR, ("CreateEvent failed with OS error %d", GetLastError()));
            return ER_OS_ERROR;
        }
    }

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

    ResetEvent(timerStoppedEvent);
    return ER_OK;
}

QStatus TimerImpl::Stop()
{
    if (timerStoppedEvent != NULL) {
        SetEvent(timerStoppedEvent);
    }
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

    if (lightLoadEvent != NULL) {
        CloseHandle(lightLoadEvent);
        lightLoadEvent = NULL;
    }
    if (timerStoppedEvent != NULL) {
        CloseHandle(timerStoppedEvent);
        timerStoppedEvent = NULL;
    }
    return ER_OK;
}

QStatus TimerImpl::Join()
{
    return ER_OK;
}

QStatus TimerImpl::AddAlarm(const Alarm& alarm, bool canBlock)
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

    if (canBlock && (lightLoadEvent != NULL)) {
        assert(maxAlarms > 0);
        bool timerAdded = false;
        /* Block until we can add a new timer (when the alarm count is lower than maxAlarms) */
        while (!timerAdded) {
            HANDLE handles[] = { timerStoppedEvent, lightLoadEvent };
            DWORD waitResult = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
            switch (waitResult) {
            case WAIT_OBJECT_0:
                /* Timer stopped */
                CloseThreadpoolTimer(ptpTimerLocal);
                return ER_TIMER_EXITING;

            case WAIT_OBJECT_0 + 1:
                /* Got signal that it's OK to add another alarm, attempt to add under lock */
                timersLock.Lock();
                if (timers.size() < maxAlarms) {
                    TimerContext ctx(ptpTimerLocal, alarm);
                    AddTimerInternal(ctx);
                    timerAdded = true;
                }
                timersLock.Unlock();
                break;

            case WAIT_FAILED:
                QCC_LogError(ER_OS_ERROR, ("WaitForMultipleObjects failed with OS error %d", GetLastError()));
                CloseThreadpoolTimer(ptpTimerLocal);
                return ER_OS_ERROR;

            default:
                QCC_LogError(ER_OS_ERROR, ("WaitForMultipleObjects failed with waitResult %d", waitResult));
                CloseThreadpoolTimer(ptpTimerLocal);
                return ER_OS_ERROR;
            }
        }
        if (!timerAdded) {
            CloseThreadpoolTimer(ptpTimerLocal);
            return ER_TIMER_EXITING;
        }
    } else {
        /* Cannot block */
        bool timerAdded = false;
        timersLock.Lock();
        if ((maxAlarms == 0) || (timers.size() < maxAlarms)) {
            TimerContext ctx(ptpTimerLocal, alarm);
            AddTimerInternal(ctx);
            timerAdded = true;
        }
        timersLock.Unlock();
        if (!timerAdded) {
            CloseThreadpoolTimer(ptpTimerLocal);
            return ER_TIMER_FULL;
        }
    }

    /* Calculate the delay; this must be done after the wait */
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
    bool preventReentrancy = timerImpl->preventReentrancy;
    for (auto& timer : timerImpl->timers) {
        if (timer->ptpTimer == ptpTimerLocal) {
            found = true;
            timerLocal = timer;
            /* If this is a one-shot timer, remove it out of the master list */
            if (timer->alarm->periodMs == 0) {
                timerImpl->RemoveTimerInternal(timer);
            }
            break;
        }
    }
    timerImpl->timersLock.Unlock();
    if (found) {
        bool unlockReentrantLock = false;
        if (preventReentrancy) {
            /* User wants callback made serially; block this thread until other callback is complete */
            unlockReentrantLock = true;
            timerImpl->reentrantLock.Lock();
        } else {
            /* Ensure the callback for the next alarm is speedy */
            CallbackMayRunLong(instance);
        }
        /* Trigger the alarm but only if the timer is still running */
        if (timerImpl->IsRunning()) {
            (timerLocal->alarm->listener->AlarmTriggered)(timerLocal->alarm, ER_OK);
        }
        /* Release the reentrant lock if acquired */
        if (unlockReentrantLock) {
            timerImpl->reentrantLock.Unlock();
        }
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
                RemoveTimerInternal(timer);
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
                RemoveTimerInternal(timer);
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
    if (timerStoppedEvent == NULL) {
        return false;
    }
    if (WaitForSingleObject(timerStoppedEvent, 0) == WAIT_OBJECT_0) {
        return false;
    }
    return true;
}

void TimerImpl::EnableReentrancy()
{
    preventReentrancy = false;
}

const qcc::String& TimerImpl::GetName() const
{
    return nameStr;
}

Timer::Timer(String name, bool expireOnExit, uint32_t concurrency, bool preventReentrancy, uint32_t maxAlarms) :
    timerImpl(new TimerImpl(name, expireOnExit, concurrency, preventReentrancy, maxAlarms))
{
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
    return timerImpl->AddAlarm(alarm, true);
}

QStatus Timer::AddAlarmNonBlocking(const Alarm& alarm)
{
    return timerImpl->AddAlarm(alarm, false);
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
        status = timerImpl->AddAlarm(newAlarm, blockIfTriggered);
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
    timerImpl->EnableReentrancy();
}

bool Timer::IsHoldingReentrantLock() const
{
    return false;
}

