/**
 * @file
 *
 * Implement Timer object
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
    QStatus ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered);
    bool RemoveAlarm(const Alarm& alarm, bool blockIfTriggered = true);
    void RemoveAlarmsWithListener(const AlarmListener& listener);
    bool HasAlarm(const Alarm& alarm) const;
    bool IsRunning() const;
    void EnableReentrancy();
    bool IsHoldingReentrantLock() const;
    const qcc::String& GetName() const;

  private:
    class _TimerContext {
        friend class TimerImpl;
      public:
        _TimerContext();
        bool operator==(const _TimerContext& other) const;
        void StartTimer(PTP_TIMER newTimer, const Alarm& newAlarm);
      private:
        PTP_TIMER ptpTimer;
        Alarm alarm;
    };
    typedef ManagedObj<_TimerContext> TimerContext;

    static void CALLBACK OnTimeout(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER ptpTimer);

    void AddTimerInternal(TimerContext timer);
    void RemoveTimerInternal(TimerContext timer);

    String nameStr;
    const bool expireOnExit;
    const DWORD maxThreads;
    const uint32_t maxAlarms;
    const bool preventReentrancy;
    volatile bool running;

    HANDLE timerStoppedEvent;
    HANDLE alarmSemaphore;

    TP_CALLBACK_ENVIRON environment;
    PTP_POOL ptpPool;
    PTP_CLEANUP_GROUP ptpCleanupGroup;

    DWORD threadHoldingReentrantLock;   /* Thread ID currently holding the reentrant lock; 0 if the lock is not currently held */
    Mutex reentrantLock;                /* This must not be acquired while timersLock is held (deadlock) */
    mutable Mutex timersLock;           /* Declared mutable because HasAlarm is const */
    std::list<TimerContext> timers;
};

}

TimerImpl::_TimerContext::_TimerContext() : ptpTimer(NULL)
{
}

void TimerImpl::_TimerContext::StartTimer(PTP_TIMER newTimer, const Alarm& newAlarm)
{
    if (ptpTimer != NULL) {
        /* Cancel the old timer */
        SetThreadpoolTimer(ptpTimer, NULL, 0, 0);
        ptpTimer = NULL;
    }

    ptpTimer = newTimer;
    alarm = newAlarm;

    if (ptpTimer != NULL) {
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

        /* Schedule the timer */
        SetThreadpoolTimer(newTimer, &fileTime, alarm->periodMs, 0);
    }
}

bool TimerImpl::_TimerContext::operator==(const TimerImpl::_TimerContext& other) const
{
    return (ptpTimer == other.ptpTimer);
}

TimerImpl::TimerImpl(qcc::String name, bool expireOnExit, uint32_t concurrency, bool preventReentrancy, uint32_t maxAlarms) :
    nameStr(name), expireOnExit(expireOnExit), maxThreads(concurrency), ptpPool(NULL), ptpCleanupGroup(NULL),
    preventReentrancy(preventReentrancy), maxAlarms(maxAlarms), running(false),
    timerStoppedEvent(NULL), alarmSemaphore(NULL), threadHoldingReentrantLock(0)
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
    /* This assert is to ensure maxAlarms is never exceeded (if it's specified) */
    assert((maxAlarms == 0) || (timers.size() <= maxAlarms));
}

/* This function requires lock held by caller */
void TimerImpl::RemoveTimerInternal(TimerContext timer)
{
    timers.remove(timer);
    if (alarmSemaphore != NULL) {
        ReleaseSemaphore(alarmSemaphore, 1, nullptr);
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
        assert(alarmSemaphore == NULL);
        alarmSemaphore = CreateSemaphore(nullptr, maxAlarms, maxAlarms, nullptr);
        if (alarmSemaphore == NULL) {
            QCC_LogError(ER_OS_ERROR, ("CreateSemaphore failed with OS error %d", GetLastError()));
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
    running = true;
    return ER_OK;
}

QStatus TimerImpl::Stop()
{
    if (timerStoppedEvent != NULL) {
        SetEvent(timerStoppedEvent);
    }
    running = false;
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

    if (alarmSemaphore != NULL) {
        CloseHandle(alarmSemaphore);
        alarmSemaphore = NULL;
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

    TimerContext timerToAdd;
    if (canBlock && (alarmSemaphore != NULL)) {
        assert(maxAlarms > 0);
        /* Block until we can add a new timer (when the alarm count is lower than maxAlarms) */
        HANDLE handles[] = { timerStoppedEvent, alarmSemaphore };
        DWORD waitResult = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
        switch (waitResult) {
        case WAIT_OBJECT_0:
            /* Timer stopped */
            return ER_TIMER_EXITING;

        case WAIT_OBJECT_0 + 1:
            /* Got signal that it's OK to add another alarm, add under lock */
            timersLock.Lock();
            AddTimerInternal(timerToAdd);
            timersLock.Unlock();
            timerToAdd->StartTimer(ptpTimerLocal, alarm);
            break;

        case WAIT_FAILED:
            QCC_LogError(ER_OS_ERROR, ("WaitForMultipleObjects failed with OS error %d", GetLastError()));
            return ER_OS_ERROR;

        default:
            QCC_LogError(ER_OS_ERROR, ("WaitForMultipleObjects failed with waitResult %d", waitResult));
            return ER_OS_ERROR;
        }
    } else {
        /* Cannot block */
        bool timerAdded = false;
        timersLock.Lock();
        if ((maxAlarms == 0) || (timers.size() < maxAlarms)) {
            AddTimerInternal(timerToAdd);
            timerAdded = true;
        }
        timersLock.Unlock();
        if (timerAdded) {
            timerToAdd->StartTimer(ptpTimerLocal, alarm);
        } else {
            return ER_TIMER_FULL;
        }
    }
    return ER_OK;
}

void TimerImpl::OnTimeout(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER ptpTimerLocal)
{
    TimerImpl* timerImpl = reinterpret_cast<TimerImpl*>(context);
    /* Find the timer object corresponding to this callback */
    bool found = false;
    bool remove = false;
    TimerContext timerLocal;
    timerImpl->timersLock.Lock();
    bool preventReentrancy = timerImpl->preventReentrancy;
    for (auto& timer : timerImpl->timers) {
        if (timer->ptpTimer == ptpTimerLocal) {
            found = true;
            timerLocal = timer;
            /* If this is a one-shot timer, schedule a removal */
            if (timer->alarm->periodMs == 0) {
                remove = true;
            }
            break;
        }
    }
    timerImpl->timersLock.Unlock();
    if (found) {
        /* Ensure there's at least one idle thread in the pool to pick up the next alarm immediately */
        CallbackMayRunLong(instance);
        if (preventReentrancy) {
            /* User wants callback made serially; block this thread until other callback is complete */
            timerImpl->reentrantLock.Lock();
            timerImpl->threadHoldingReentrantLock = GetCurrentThreadId();
        }
        /* Trigger the alarm but only if the timer is still running */
        if (timerImpl->IsRunning()) {
            (timerLocal->alarm->listener->AlarmTriggered)(timerLocal->alarm, ER_OK);
        }

        /*
         * Release the reentrant lock if acquired.
         * NOTE: The AlarmTriggered callback above could have released the reentrant lock already
         * by calling EnableReentrancy, but the thread ID check below should take care of that.
         */
        if (preventReentrancy && (GetCurrentThreadId() == timerImpl->threadHoldingReentrantLock)) {
            timerImpl->threadHoldingReentrantLock = 0;
            timerImpl->reentrantLock.Unlock();
        }
        /* Remove the timer if needed; because we left the lock during the callback, we need to iterate the list again */
        if (remove) {
            timerImpl->timersLock.Lock();
            for (auto& timer : timerImpl->timers) {
                if (timer->ptpTimer == ptpTimerLocal) {
                    timerImpl->RemoveTimerInternal(timer);
                    break;
                }
            }
            timerImpl->timersLock.Unlock();
        }
    }
}

QStatus TimerImpl::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
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
    PTP_TIMER timerFound = NULL;
    timersLock.Lock();
    for (auto& timer : timers) {
        if (timer->alarm == origAlarm) {
            /* Replace the old timer with the new one under lock */
            timerFound = timer->ptpTimer;
            timer->StartTimer(ptpTimerLocal, newAlarm);
        }
    }
    timersLock.Unlock();
    if (timerFound == NULL) {
        return ER_NO_SUCH_ALARM;
    }
    if (blockIfTriggered) {
        WaitForThreadpoolTimerCallbacks(timerFound, FALSE);
    }
    return ER_OK;
}

bool TimerImpl::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    bool found = false;
    if (IsRunning() || expireOnExit) {
        TimerContext timerLocal;
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
        if (found) {
            SetThreadpoolTimer(timerLocal->ptpTimer, NULL, 0, 0);
            if (blockIfTriggered) {
                WaitForThreadpoolTimerCallbacks(timerLocal->ptpTimer, FALSE);
            }
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
    if (!running) {
        return false;
    }
    if (timerStoppedEvent == NULL) {
        return false;
    }
    return true;
}

void TimerImpl::EnableReentrancy()
{
    if (IsHoldingReentrantLock()) {
        threadHoldingReentrantLock = 0;
        reentrantLock.Unlock();
    }
}

bool TimerImpl::IsHoldingReentrantLock() const
{
    if (preventReentrancy) {
        DWORD currentThreadId = GetCurrentThreadId();
        return (currentThreadId == threadHoldingReentrantLock);
    }
    return false;
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
    return timerImpl->ReplaceAlarm(origAlarm, newAlarm, blockIfTriggered);
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
    return timerImpl->IsHoldingReentrantLock();
}

