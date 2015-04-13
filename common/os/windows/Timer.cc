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
    bool IsTimerCallbackThread() const;
    const qcc::String& GetName() const;

  private:
    class _TimerContext {
        friend class TimerImpl;
      public:
        _TimerContext();
        bool operator==(const _TimerContext& other) const;
        bool operator<(const _TimerContext& other) const;
        void StartTimer();

      private:
        bool active;
        PTP_TIMER ptpTimer;
        Alarm alarm;
        DWORD threadId;     /* Thread ID currently servicing this timer context; 0 for scheduled timers */
    };
    typedef ManagedObj<_TimerContext> TimerContext;

    static void CALLBACK s_OnTimeout(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER ptpTimer);
    void OnTimeout(PTP_CALLBACK_INSTANCE instance, PTP_TIMER ptpTimer);

    /* Use these lock and unlock functions to serialize access to the lists */
    void Lock() const;
    void Unlock(bool validateStates = true) const;  /* Note: 'validateStates' parameter is used by debug build only */

    /* Assume timerLock is held on entrance and exit */
    bool RemoveTimerByAlarm(const Alarm& alarm, bool releaseSemaphore);
    /* Assume timerLock is held on entrance and exit */
    QStatus WaitForAvailableAlarm(bool canBlock);

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

    Mutex reentrantLock;                                        /* This must not be acquired while timerLock is held (or deadlock) */
    DWORD threadHoldingReentrantLock;                           /* Thread ID currently holding the reentrant lock; 0 if the lock is not currently held */

    Mutex joinLock;                                             /* Make sure that Join is thread safe */
    volatile LONG joinCount;                                    /* Number of threads waiting in Join() */

    /* Private assignment operator - does nothing */
    TimerImpl& operator=(const TimerImpl&);

    mutable Mutex timerLock;                                    /* Lock for timers and inFlightTimers; declared mutable because HasAlarm is const */
    std::set<TimerContext> timers;    /* Active timers (ordered) */
    std::list<TimerContext> inFlightTimers;                     /* Timers that are currently servicing an alarm */

#ifndef NDEBUG
    mutable DWORD debugThreadHoldingTimerLock;
    int debugSemaphore;
    size_t debugNumExitsExpected;
    TimerContext debugTimerInprogress;
#endif
};

} /* namespace qcc */

TimerImpl::_TimerContext::_TimerContext() : active(true), ptpTimer(nullptr), threadId(0)
{
}

void TimerImpl::_TimerContext::StartTimer()
{
    assert(ptpTimer != nullptr);
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
    SetThreadpoolTimer(ptpTimer, &fileTime, 0, 0);
}

bool TimerImpl::_TimerContext::operator==(const TimerImpl::_TimerContext& other) const
{
    return (alarm == other.alarm);
}

bool TimerImpl::_TimerContext::operator<(const TimerImpl::_TimerContext& other) const
{
    return (alarm < other.alarm);
}

TimerImpl::TimerImpl(qcc::String name, bool expireOnExit, uint32_t concurrency, bool preventReentrancy, uint32_t maxAlarms) :
    nameStr(name)
    , expireOnExit(expireOnExit)
    , maxThreads(concurrency)
    , ptpPool(nullptr)
    , ptpCleanupGroup(nullptr)
    , preventReentrancy(preventReentrancy)
    , maxAlarms(maxAlarms)
    , running(false)
    , timerStoppedEvent(nullptr)
    , alarmSemaphore(nullptr)
    , threadHoldingReentrantLock(0)
    , joinCount(0)
#ifndef NDEBUG
    , debugThreadHoldingTimerLock(0)
    , debugSemaphore(0)
    , debugNumExitsExpected(0)
#endif
{
    InitializeThreadpoolEnvironment(&environment);
}

TimerImpl::~TimerImpl()
{
    QStatus status = ER_OK;
    if (status == ER_OK) {
        status = Stop();
        assert(status == ER_OK);
    }
    if (status == ER_OK) {
        status = Join();
        assert(status == ER_OK);
    }
    if (status == ER_OK) {
        /*
         * Make sure that all threads have Join()'d before proceeding.
         * Using Sleep here instead of a synchronization object to avoid wasting resource on
         * an unlikely event that the caller forgets to call Stop & Join before tearing down.
         */
        while (joinCount > 0) {
            Sleep(1);
        }
        if (alarmSemaphore != nullptr) {
            CloseHandle(alarmSemaphore);
        }
        if (timerStoppedEvent != nullptr) {
            CloseHandle(timerStoppedEvent);
        }
        DestroyThreadpoolEnvironment(&environment);
    }
}

void TimerImpl::Lock() const
{
    timerLock.Lock();
#ifndef NDEBUG
    debugThreadHoldingTimerLock = GetCurrentThreadId();
#endif
}

void TimerImpl::Unlock(bool validateStates) const
{
#ifndef NDEBUG
    if (validateStates) {
        /* Make sure that the scheduled timer is the first one */
        if (running && !timers.empty()) {
            TimerContext first = *(timers.begin());
            if (debugTimerInprogress->ptpTimer != first->ptpTimer) {
                bool found = false;
                for (auto& timer : inFlightTimers) {
                    if (debugTimerInprogress->ptpTimer == timer->ptpTimer) {
                        found = true;
                        break;
                    }
                }
                assert(found);
            }
        }
    }
    debugThreadHoldingTimerLock = 0;
#else
    UNREFERENCED_PARAMETER(validateStates);
#endif
    timerLock.Unlock();
}

/* Assume timerLock is held on entrance and exit */
bool TimerImpl::RemoveTimerByAlarm(const Alarm& alarm, bool releaseSemaphore)
{
    assert(debugThreadHoldingTimerLock == GetCurrentThreadId());

    bool found = false;
    for (auto it = timers.begin(); it != timers.end(); it++) {
        if ((*it)->alarm == alarm) {
            found = true;
            timers.erase(it);
            break;
        }
    }
    if (releaseSemaphore && (alarmSemaphore != nullptr) && found) {
        debugSemaphore--;
        QCC_VERIFY(ReleaseSemaphore(alarmSemaphore, 1, nullptr));
    }

    assert(debugThreadHoldingTimerLock == GetCurrentThreadId());
    return found;
}

QStatus TimerImpl::Start()
{
    if (IsRunning()) {
        return ER_OK;
    }

    if (timerStoppedEvent == nullptr) {
        timerStoppedEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
        if (timerStoppedEvent == nullptr) {
            QCC_LogError(ER_OS_ERROR, ("CreateEvent failed with OS error %d", GetLastError()));
            return ER_OS_ERROR;
        }
    }
    if ((maxAlarms > 0) && (alarmSemaphore == nullptr)) {
        alarmSemaphore = CreateSemaphore(nullptr, maxAlarms, maxAlarms, nullptr);
        if (alarmSemaphore == nullptr) {
            QCC_LogError(ER_OS_ERROR, ("CreateSemaphore failed with OS error %d", GetLastError()));
            return ER_OS_ERROR;
        }
    }

    PTP_POOL ptpPoolLocal = CreateThreadpool(nullptr);
    if (ptpPoolLocal == nullptr) {
        QCC_LogError(ER_OS_ERROR, ("CreateThreadpool failed with OS error %d", GetLastError()));
        return ER_OS_ERROR;
    }
    PTP_CLEANUP_GROUP ptpCleanupGroupLocal = CreateThreadpoolCleanupGroup();
    if (ptpCleanupGroupLocal == nullptr) {
        QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolCleanupGroup failed with OS error %d", GetLastError()));
        CloseThreadpool(ptpPoolLocal);
        return ER_OS_ERROR;
    }
    SetThreadpoolThreadMaximum(ptpPoolLocal, maxThreads);
    SetThreadpoolCallbackPool(&environment, ptpPoolLocal);
    SetThreadpoolCallbackCleanupGroup(&environment, ptpCleanupGroupLocal, nullptr);
    ptpPool = ptpPoolLocal;
    ptpCleanupGroup = ptpCleanupGroupLocal;

    ResetEvent(timerStoppedEvent);
    running = true;
    return ER_OK;
}

QStatus TimerImpl::Stop()
{
    Lock();
    running = false;
    if (timerStoppedEvent != nullptr) {
        SetEvent(timerStoppedEvent);
    }
#ifndef NDEBUG
    debugNumExitsExpected = timers.size();
#endif
    if (!timers.empty()) {
        if (expireOnExit) {
            TimerContext first = *(timers.begin());
            /* Fire immediately */
            FILETIME immediate = { 0 };
            SetThreadpoolTimer(first->ptpTimer, &immediate, 0, 0);
        } else {
            for (auto timer : timers) {
                /* Cancel */
                SetThreadpoolTimer(timer->ptpTimer, nullptr, 0, 0);
            }
        }
    }
    Unlock(false);
    return ER_OK;
}

QStatus TimerImpl::Join()
{
    InterlockedIncrement(&joinCount);
    if (timerStoppedEvent != nullptr) {
        /* Block forever until Stop is called */
        DWORD waitResult = WaitForSingleObject(timerStoppedEvent, INFINITE);
        if (waitResult != WAIT_OBJECT_0) {
            QCC_LogError(ER_OS_ERROR, ("WaitForSingleObject failed with OS error %d", GetLastError()));
            assert(false);
            InterlockedDecrement(&joinCount);
            return ER_OS_ERROR;
        }
    }

    joinLock.Lock();
    if (ptpCleanupGroup != nullptr) {
        if (expireOnExit) {
            /* Wait for all timers to complete; timerLock can be acquired here because it is never held when the timer is triggered */
            for (bool done = false; !done;) {
                Lock();
                done = (timers.size() == 0);
                Unlock(false);
                if (!done) {
                    Sleep(2);
                }
            }
            assert(debugNumExitsExpected == 0);
        }
        CloseThreadpoolCleanupGroupMembers(ptpCleanupGroup, TRUE, nullptr);
        CloseThreadpoolCleanupGroup(ptpCleanupGroup);
        ptpCleanupGroup = nullptr;
    }
#ifndef NDEBUG
    debugTimerInprogress = TimerContext();
#endif
    inFlightTimers.clear();
    timers.clear();
    if (ptpPool != nullptr) {
        CloseThreadpool(ptpPool);
        ptpPool = nullptr;
    }

    joinLock.Unlock();
    InterlockedDecrement(&joinCount);
    return ER_OK;
}

/* Assume timerLock is held on entrance and exit */
QStatus TimerImpl::WaitForAvailableAlarm(bool canBlock)
{
    assert(debugThreadHoldingTimerLock == GetCurrentThreadId());

    QStatus status;
    assert(maxAlarms > 0);

    Unlock();
    HANDLE handles[] = { timerStoppedEvent, alarmSemaphore };
    DWORD timeout = canBlock ? INFINITE : 0;
    /* Block until we can add a new timer (when the alarm count is lower than maxAlarms) */
    DWORD waitResult = WaitForMultipleObjects(_countof(handles), handles, FALSE, timeout);
    Lock();
    switch (waitResult) {
    case WAIT_OBJECT_0:
        /* Timer stopped */
        status = ER_TIMER_EXITING;
        break;

    case WAIT_OBJECT_0 + 1:
        debugSemaphore++;
        if (running) {
            assert(timers.size() < maxAlarms);
            status = ER_OK;
        } else {
            debugSemaphore--;
            QCC_VERIFY(ReleaseSemaphore(alarmSemaphore, 1, nullptr));
            status = ER_TIMER_EXITING;
        }
        break;

    case WAIT_TIMEOUT:
        assert(!canBlock);
        status = ER_TIMER_FULL;
        break;

    case WAIT_FAILED:
        QCC_LogError(ER_OS_ERROR, ("WaitForMultipleObjects failed with OS error %d", GetLastError()));
        status = ER_OS_ERROR;
        break;

    default:
        QCC_LogError(ER_OS_ERROR, ("WaitForMultipleObjects failed with waitResult %d", waitResult));
        status = ER_OS_ERROR;
        break;
    }

    assert(debugThreadHoldingTimerLock == GetCurrentThreadId());
    return status;
}

QStatus TimerImpl::AddAlarm(const Alarm& alarm, bool canBlock)
{
    bool releaseSemaphore = false;
    Lock();
    QStatus status = ER_OK;
    if (!running) {
        status = ER_TIMER_EXITING;
    }
    /* Ensure the number of outstanding timers is lower than maxAlarm */
    if ((status == ER_OK) && (alarmSemaphore != nullptr)) {
        status = WaitForAvailableAlarm(canBlock);
        if (status == ER_OK) {
            releaseSemaphore = true;
        }
    }
    /* Prevent adding a new alarm if there's a limit imposed and we've reached it */
    if ((status == ER_OK) && (maxAlarms > 0) && (timers.size() >= maxAlarms)) {
        /* ER_TIMER_FULL should never be returned if the caller says the API can be blocked */
        assert(!canBlock);
        assert((alarmSemaphore == nullptr) || releaseSemaphore);
        status = ER_TIMER_FULL;
    }
    /* Create a new OS timer */
    PTP_TIMER ptpTimerLocal = nullptr;
    if (status == ER_OK) {
        ptpTimerLocal = CreateThreadpoolTimer(s_OnTimeout, this, &environment);
        if (ptpTimerLocal == nullptr) {
            QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolTimer failed with OS error %d", GetLastError()));
            assert((alarmSemaphore == nullptr) || releaseSemaphore);
            status = ER_OS_ERROR;
        }
    }
    /* Insert the new timer into the list */
    if (status == ER_OK) {
        assert((maxAlarms == 0) || (timers.size() < maxAlarms));
        TimerContext newTimer;
        newTimer->alarm = alarm;
        newTimer->ptpTimer = ptpTimerLocal;
        ptpTimerLocal = nullptr;

        TimerContext previous;
        if (!timers.empty()) {
            previous = *(timers.begin());
        }
        size_t sizeBefore = timers.size();
        timers.insert(newTimer);
        if (sizeBefore == timers.size()) {
            /* Failed to insert (duplicate), ensure semaphore is released */
            assert((alarmSemaphore == nullptr) || releaseSemaphore);
            status = ER_FAIL;
        } else {
            /* The new timer added successfully */
            releaseSemaphore = false;
            /* See if the timer just added is in the first slot */
            TimerContext first = *(timers.begin());
            if (first->alarm == newTimer->alarm) {
                /* Invalidate & cancel the previous timer */
                if (previous->ptpTimer != nullptr) {
                    previous->active = false;
                    SetThreadpoolTimer(previous->ptpTimer, nullptr, 0, 0);
                }
                /* Schedule the new timer */
#ifndef NDEBUG
                debugTimerInprogress = newTimer;
#endif
                newTimer->StartTimer();
                /* Sanity check */
                assert(first->alarm == newTimer->alarm);
                assert(first->alarm != previous->alarm);
            }
        }
    }
    Unlock();
    if (releaseSemaphore) {
        debugSemaphore--;
        QCC_VERIFY(ReleaseSemaphore(alarmSemaphore, 1, nullptr));
    }
    return status;
}

void TimerImpl::s_OnTimeout(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER ptpTimerLocal)
{
    TimerImpl* timerImpl = reinterpret_cast<TimerImpl*>(context);
    timerImpl->OnTimeout(instance, ptpTimerLocal);
}

void TimerImpl::OnTimeout(PTP_CALLBACK_INSTANCE instance, PTP_TIMER ptpTimerLocal)
{
    Timespec now;
    GetTimeNow(&now);

    /* The first thing to do is to look for the timer to trigger and take it out from the list */
    TimerContext trigger;
    bool releaseSemaphoreOnExit = false;
    Lock();
    if (!timers.empty()) {
        TimerContext first = *(timers.begin());
        if (first->ptpTimer == ptpTimerLocal) {
            /* Prepare to fire the alarm by moving it into the in-flight list */
            assert(first->ptpTimer != nullptr);
            trigger = first;
            inFlightTimers.push_back(trigger);
            QCC_VERIFY(RemoveTimerByAlarm(trigger->alarm, false));
            releaseSemaphoreOnExit = true;
        } else {
            /*
             * Here we make sure that we're indeed dealing with the first timer as it is possible that the
             * previous (first) timer started just before it was pushed out to another slot. For example,
             * 1. Timer1 fires but has not taken the lock yet.
             * 2. AddAlarm is called, takes the lock, cancels Timer1 and inserts Timer0 to the front.
             * 3. Timer1 is now in the second slot but is already in transit, thus this else clause does nothing.
             */
            assert(trigger->ptpTimer == nullptr);
            assert(!releaseSemaphoreOnExit);
        }
    }
#ifndef NDEBUG
    if (timers.empty()) {
        debugTimerInprogress = TimerContext();
    }
    if (!IsRunning() && (trigger->ptpTimer != nullptr)) {
        debugNumExitsExpected--;
    }
#endif
    Unlock(false);

    /* If we did not find the timer to trigger, there's nothing else to do */
    if (trigger->ptpTimer == nullptr) {
        assert(!releaseSemaphoreOnExit);
        return;
    }

    /* Ensure there's at least one idle thread in the pool to pick up the next alarm immediately */
    CallbackMayRunLong(instance);

    /* User wants callback made serially; block this thread until other callback is complete */
    if (IsRunning() && preventReentrancy) {
        reentrantLock.Lock();
        threadHoldingReentrantLock = GetCurrentThreadId();
    }

    /* Schedule the next timer under reentrancy lock to ensure in-order delivery */
    Lock();
    if (!timers.empty()) {
        TimerContext nextTimer = *(timers.begin());
        assert(nextTimer->ptpTimer != nullptr);
        assert(nextTimer->ptpTimer != trigger->ptpTimer);
        nextTimer->active = true;
#ifndef NDEBUG
        debugTimerInprogress = nextTimer;
#endif
        if (!IsRunning()) {
            nextTimer->alarm->alarmTime = now;
        }
        nextTimer->StartTimer();
    }
    Unlock();

    /* Trigger the alarm under reentrancy lock but outside timers lock */
    assert(trigger->alarm->listener != nullptr);
    if (IsRunning()) {
        (trigger->alarm->listener->AlarmTriggered)(trigger->alarm, ER_OK);
    } else if (expireOnExit) {
        (trigger->alarm->listener->AlarmTriggered)(trigger->alarm, ER_TIMER_EXITING);
    }

    /*
     * Release the reentrant lock if acquired.
     * NOTE: The AlarmTriggered callback above could have released the reentrant lock already
     * by calling EnableReentrancy, but the thread ID check below should take care of that.
     */
    if ((preventReentrancy) && (GetCurrentThreadId() == threadHoldingReentrantLock)) {
        threadHoldingReentrantLock = 0;
        reentrantLock.Unlock();
    }

    Lock();
    /* Add the triggered timer back if it is periodic (move it back from the in-flight list) */
    bool closeTimerOnExit = IsRunning();
    bool removed = false;
    for (auto& timer : inFlightTimers) {
        if (timer->ptpTimer == trigger->ptpTimer) {
            inFlightTimers.remove(timer);
            removed = true;
            break;
        }
    }
    if (IsRunning() && removed) {
        if (!trigger->active || (trigger->ptpTimer == nullptr)) {
            /* Timer has been invalidated (eg. by RemoveAlarm); it cannot be closed as someone could be waiting for it */
            closeTimerOnExit = false;
        } else {
            uint32_t periodMs = trigger->alarm->periodMs;
            if (periodMs > 0) {
                if (!IsRunning()) {
                    trigger->alarm->alarmTime = now;
                } else {
                    trigger->alarm->alarmTime += periodMs;
                    if (trigger->alarm->alarmTime < now) {
                        trigger->alarm->alarmTime = now;
                    }
                }
                /* We are adding back the periodic timer, so keep the semaphore */
                closeTimerOnExit = false;
                releaseSemaphoreOnExit = false;
                bool isFirst;
                if (timers.empty()) {
                    isFirst = true;
                } else {
                    isFirst = false;
                    TimerContext first = *(timers.begin());
                    assert(first->ptpTimer != nullptr);
                    if (trigger->alarm < first->alarm) {
                        isFirst = true;
                        /* Invalidate and cancel the previous timer */
                        first->active = false;
                        SetThreadpoolTimer(first->ptpTimer, nullptr, 0, 0);
                    }
                }
                timers.insert(trigger);
                if (isFirst) {
#ifndef NDEBUG
                    debugTimerInprogress = trigger;
#endif
                    trigger->StartTimer();
                    /* The rest is sanity check */
                    TimerContext first = *(timers.begin());
                    assert(first->ptpTimer == trigger->ptpTimer);
                    assert(first->alarm == trigger->alarm);
                }
            }
        }
    }
    trigger->threadId = 0;
    Unlock();

    /* Release the semaphore in the event that this function removed the timer context from the list. */
    if (releaseSemaphoreOnExit && (alarmSemaphore != nullptr)) {
        debugSemaphore--;
        QCC_VERIFY(ReleaseSemaphore(alarmSemaphore, 1, nullptr));
    }
    /* Clean up thread resources now since there nobody is referencing it anymore */
    if (closeTimerOnExit) {
        CloseThreadpoolTimer(ptpTimerLocal);
    }
}

QStatus TimerImpl::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
{
    if (!IsRunning()) {
        return ER_TIMER_EXITING;
    }

    /* Create a new timer */
    PTP_TIMER ptpTimerLocal = CreateThreadpoolTimer(s_OnTimeout, this, &environment);
    if (ptpTimerLocal == nullptr) {
        QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolTimer failed with OS error %d", GetLastError()));
        return ER_OS_ERROR;
    }

    bool found = false;
    Lock();
    if (running && !timers.empty()) {
        TimerContext first = *(timers.begin());
        if (first->alarm == origAlarm) {
            /* This is the first alarm schedule to fire, replace the OS timer */
            found = true;
            assert(first->ptpTimer != nullptr);
            SetThreadpoolTimer(first->ptpTimer, nullptr, 0, 0);
            first->ptpTimer = ptpTimerLocal;
            first->alarm = newAlarm;
#ifndef NDEBUG
            debugTimerInprogress = first;
#endif
            first->StartTimer();
        } else {
            for (auto timer : timers) {
                if (timer->alarm == origAlarm) {
                    found = true;
                    timer->alarm = newAlarm;
                    break;
                }
            }
        }
    }
    /*
     * The alarm may be in-flight (and not in the main 'timers' list); if this is the case, we'll need to block this thread
     * if it isn't the thread that executing the alarm (as the alarm handler itself can call this function).
     * Here we'll invalidate the timer context in the in-flight list and wait on it outside the lock.
     */
    PTP_TIMER wait = nullptr;
    if (blockIfTriggered && !found) {
        for (auto timer : inFlightTimers) {
            if (timer->alarm == origAlarm) {
                if (timer->threadId != GetCurrentThreadId()) {
                    wait = timer->ptpTimer;
                    /* Invalidate this timer context */
                    timer->active = false;
                }
                break;
            }
        }
    }
    Unlock();
    if (!found) {
        return ER_NO_SUCH_ALARM;
    }
    /* Block the execution until the alarm completes. */
    if (wait != nullptr) {
        SetThreadpoolTimer(wait, nullptr, 0, 0);
        if (!IsTimerCallbackThread()) {
            WaitForThreadpoolTimerCallbacks(wait, TRUE);
            /* Timer is no longer referenced, close it now */
            CloseThreadpoolTimer(wait);
        }
    }
    return ER_OK;
}

bool TimerImpl::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    if (!IsRunning()) {
        return false;
    }
    bool found = false;
    Lock();
    if (running && !timers.empty()) {
        TimerContext first = *(timers.begin());
        if (first->alarm == alarm) {
            /* Cancel the timer */
            assert(first->ptpTimer != nullptr);
            first->active = false;
            SetThreadpoolTimer(first->ptpTimer, nullptr, 0, 0);
            found = RemoveTimerByAlarm(alarm, true);
            assert(found);
            /* Start the next one */
            if (!timers.empty()) {
                first = *(timers.begin());
                assert(first->ptpTimer != nullptr);
                first->active = true;
#ifndef NDEBUG
                debugTimerInprogress = first;
#endif
                first->StartTimer();
            }
        }

        if (!found) {
            found = RemoveTimerByAlarm(alarm, true);
        }
    }
    /*
     * The alarm may be in-flight (and not in the main 'timers' list); if this is the case, we'll need to block this thread
     * if it isn't the thread that executing the alarm (as the alarm handler itself can call this function).
     * Here we'll invalidate the timer context in the in-flight list and wait on it outside the lock.
     */
    PTP_TIMER wait = nullptr;
    if (blockIfTriggered && !found) {
        for (auto& timer : inFlightTimers) {
            if (timer->alarm == alarm) {
                if (timer->threadId != GetCurrentThreadId()) {
                    wait = timer->ptpTimer;
                    /* Invalidate this timer context */
                    timer->active = false;
                }
                break;
            }
        }
    }
    Unlock();
    /* Block the execution until the alarm completes. */
    if (wait != nullptr) {
        SetThreadpoolTimer(wait, nullptr, 0, 0);
        if (!IsTimerCallbackThread()) {
            WaitForThreadpoolTimerCallbacks(wait, TRUE);
            /* Timer is no longer referenced, close it now */
            CloseThreadpoolTimer(wait);
        }
    }
    return found;
}

void TimerImpl::RemoveAlarmsWithListener(const AlarmListener& listener)
{
    Lock();
    if (running || expireOnExit) {
        int i = 0;
        for (auto timer : timers) {
            if (timer->alarm->listener == &listener) {
                /* Invalidate and cancel the timer */
                timer->active = false;
                SetThreadpoolTimer(timer->ptpTimer, nullptr, 0, 0);
                QCC_VERIFY(RemoveTimerByAlarm(timer->alarm, true));
                /* Start the next one if we've removed the first entry */
                if ((i == 0) && (!timers.empty())) {
                    TimerContext first = *(timers.begin());
                    assert(first->ptpTimer != nullptr);
                    first->active = true;
#ifndef NDEBUG
                    debugTimerInprogress = first;
#endif
                    first->StartTimer();
                }
                break;
            }
            i++;
        }
    }
    Unlock();
}

bool TimerImpl::HasAlarm(const Alarm& alarm) const
{
    bool found = false;
    Lock();
    if (running) {
        for (auto& timer : timers) {
            if (timer->alarm == alarm) {
                found = true;
                break;
            }
        }
    }
    Unlock();
    return found;
}

bool TimerImpl::IsRunning() const
{
    return running;
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

bool TimerImpl::IsTimerCallbackThread() const
{
    bool result = false;
    DWORD currentThreadId = GetCurrentThreadId();
    Lock();
    for (auto timer : inFlightTimers) {
        if (timer->threadId == currentThreadId) {
            result = true;
            break;
        }
    }
    Unlock();
    return result;
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

bool Timer::IsTimerCallbackThread() const
{
    return timerImpl->IsTimerCallbackThread();
}

