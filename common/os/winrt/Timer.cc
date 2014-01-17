/**
 * @file
 *
 * Timer thread
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#include <qcc/CountDownLatch.h>
#include <Status.h>
#include <list>
#include <map>
#include <queue>

#define QCC_MODULE  "TIMER"

#define WORKER_IDLE_TIMEOUT_MS  20
#define FALLBEHIND_WARNING_MS   500
#define HUNDRED_NANOSECONDS_PER_MILLISECOND 10000
#define MILLISECONDS_PER_SECOND 1000

using namespace std;
using namespace qcc;
using namespace Windows::System::Threading;

int32_t qcc::_Alarm::nextId = 0;

namespace qcc {

_Alarm::_Alarm() : listener(NULL), periodMs(0), context(NULL), id(IncrementAndFetch(&nextId))
{
}

_Alarm::_Alarm(Timespec absoluteTime, AlarmListener* listener, void* context, uint32_t periodMs)
    : alarmTime(absoluteTime), listener(listener), periodMs(periodMs), context(context), id(IncrementAndFetch(&nextId))
{
    UpdateComputedTime(alarmTime);
}

_Alarm::_Alarm(uint32_t relativeTime, AlarmListener* listener, void* context, uint32_t periodMs)
    : alarmTime(), listener(listener), periodMs(periodMs), context(context), id(IncrementAndFetch(&nextId))
{
    if (relativeTime == WAIT_FOREVER) {
        alarmTime = END_OF_TIME;
    } else {
        GetTimeNow(&alarmTime);
        alarmTime += relativeTime;
    }
    UpdateComputedTime(alarmTime);
}

_Alarm::_Alarm(AlarmListener* listener, void* context)
    : alarmTime(0, TIME_RELATIVE), listener(listener), periodMs(0), context(context), id(IncrementAndFetch(&nextId))
{
    UpdateComputedTime(alarmTime);
}

void* _Alarm::GetContext(void) const
{
    return context;
}

void _Alarm::SetContext(void* c) const
{
    context = c;
}

uint64_t _Alarm::GetAlarmTime() const
{
    return alarmTime.GetAbsoluteMillis();
}

bool _Alarm::operator<(const _Alarm& other) const
{
    return (id < other.id);
}

bool _Alarm::operator==(const _Alarm& other) const
{
    return (id == other.id);
}

void OSAlarm::UpdateComputedTime(Timespec absoluteTime)
{
    // Get the current timestamp
    uint64_t now = GetTimestamp64();
    // Calculate relative interval
    computedTimeMillis = absoluteTime.GetAbsoluteMillis() - now;
    if (computedTimeMillis > now) {
        // For under flow, relative time is 0
        computedTimeMillis = 0;
    }
}

Timer::Timer(const char* name, bool expireOnExit, uint32_t concurency, bool preventReentrancy, uint32_t maxAlarms)
    : nameStr(name), expireOnExit(expireOnExit), timerThreads(concurency), isRunning(false), controllerIdx(0),
    preventReentrancy(preventReentrancy), OSTimer(this), maxAlarms(maxAlarms)
{
}

// WARNING: There are all types of problems with timers if you don't ensure the parent has done the below steps in the destructor
// StopInternal and Join are already too late in many cases, but done here anyway
Timer::~Timer()
{
    // Ensure all timers have exited
    StopInternal(false);
    // Wait for all timers to complete their exit
    Join();
}

QStatus Timer::Start()
{
    QStatus status = ER_OK;
    // Grab the timer lock
    lock.Lock();
    // While stop operations are pending, wait
    while (NULL != _stopTask) {
        concurrency::task<void>* stopTask = _stopTask;
        // Release the timer lock
        lock.Unlock();
        // Wait for stop to complete
        stopTask->wait();
        // Grab the timer lock
        lock.Lock();
        // Terminal condition for stop tasks
        if (_stopTask == stopTask) {
            _stopTask = NULL;
        }
    }
    // If between all that lock shuffling, we are already in a start case, ignore the request to Start
    if (!isRunning) {
        // Iterate the set of alarms
        for (set<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
            Alarm a = (Alarm) * it;
            try {
                // Create TimeSpan based on computedTimeMillis
                Windows::Foundation::TimeSpan ts = { a->computedTimeMillis * HUNDRED_NANOSECONDS_PER_MILLISECOND };
                // Start the timer
                ThreadPoolTimer ^ tpt = ThreadPoolTimer::CreateTimer(ref new TimerElapsedHandler([&, a] (ThreadPoolTimer ^ timer) {
                                                                                                     OSTimer::TimerCallback(timer, (Alarm)a);
                                                                                                 }), ts);
                // Store the alarm associated with the ThreadPoolTimer that was just created
                a->_timer = tpt;
                _timerMap[(void*)tpt] = a;
                // Track the number of pending operations
                _timersCountdownLatch.Increment();
            } catch (...) {
                status = ER_FAIL;
                break;
            }
        }
        isRunning = (status == ER_OK);
    }
    // Release the timer lock
    lock.Unlock();
    return status;
}

void Timer::TimerCallback(void* context)
{
    /*
     * Most of the time, this dance of putting work on and off the priority_queue would not be needed.
     * However, these timer callbacks appear to arrive out of order occasionally, especially if the
     * call to AlarmTriggered takes long enough.
     *
     * To preserve the ordering of signals, the alarm is pulled out of the calling context, inserted into the
     * priority queue, then the timer lock is acquired. Once the timer lock is taken, the lock owner takes the
     * first alarm in the work queue and works with that alarm.
     */

    // push the alarm onto the queue
    _workQueueLock.Lock();
    std::pair<void*, Alarm&>* inputPair = (std::pair<void*, Alarm&>*)context;
    _timerWorkQueue.push(inputPair->second);
    _workQueueLock.Unlock();

    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    bool alarmFound = false;
    qcc::Alarm alarm;
    // Grab the reentrancy lock
    if (this->preventReentrancy) {
        reentrancyLock.Lock();
    }

    // Grab the timer lock
    lock.Lock();
    // take the highest priority alarm off the queue
    _workQueueLock.Lock();
    if (!_timerWorkQueue.empty()) {
        alarmFound = true;
        alarm = _timerWorkQueue.top();
        _timerWorkQueue.pop();
    }
    _workQueueLock.Unlock();

    // Check if timer is still in the map
    if (alarmFound) {
        if (this->preventReentrancy) {
            //Mark ownership of the reentrancy lock.
            _reentrancyLockOwner = timerThreadHandle;
        }
        // Increase the latch value for pending work
        alarm->_latch->Increment();
        // Ensure a single alarm can only be once in the map
        if (alarm->periodMs == 0) {
            // Single shot timer exiting
            RemoveAlarm(alarm, false);
        } else {
            qcc::Alarm newAlarm(alarm);
            // Schedule based on the period. Don't bother chasing the nanoseconds.
            ReplaceAlarm(alarm, newAlarm, false);
        }

        lock.Unlock();
        // Call the alarm listener associated with this alarm
        alarm->listener->AlarmTriggered(alarm, ER_OK);

        // Decrement the latch value associated with this alarm (work complete)
        alarm->_latch->Decrement();
        // Re-acquire the API lock
        lock.Lock();
        // If thread still owns the reentrancyLock, release the reentrancy lock
        if (_reentrancyLockOwner == timerThreadHandle) {
            _reentrancyLockOwner = NULL;
            // Release the API lock before releasing the reentrancy lock to avoid deadlock.
            lock.Unlock();
            reentrancyLock.Unlock();
        } else {
            // Release the API lock
            lock.Unlock();
        }
    } else {
        // Release the API lock
        lock.Unlock();
        // Release the reentrancy lock since no alarm was found after all
        if (this->preventReentrancy) {
            reentrancyLock.Unlock();
        }
    }
}

// Callback is executed when a timer has successfully been called or cancelled
void Timer::TimerCleanupCallback(void* context)
{
    // Decrement the outstanding requests on this timer
    _timersCountdownLatch.Decrement();
}

QStatus Timer::Stop()
{
    QStatus status = ER_OK;
    // Grab the timer lock
    lock.Lock();
    // Ignore stop requests if timer isn't running
    if (isRunning) {
        // Create an async operation to stop
        Windows::Foundation::IAsyncAction ^ stopAction = concurrency::create_async([this](concurrency::cancellation_token ct) {
                                                                                       StopInternal(true);
                                                                                   });
        if (nullptr != stopAction) {
            // Don't leak async operations here if we're being pulsed
            if (NULL != _stopTask) {
                delete _stopTask;
                _stopTask = nullptr;
            }
            // Create task
            _stopTask = new concurrency::task<void>(stopAction);
            // Check for allocation error
            if (NULL == _stopTask) {
                status = ER_OUT_OF_MEMORY;
            }
        } else {
            status = ER_OS_ERROR;
        }
        isRunning = !(status == ER_OK);
    }
    // Release the timer lock
    lock.Unlock();
    return status;
}

QStatus Timer::Join()
{
    QStatus status = ER_OK;
    // Grab the timer lock
    lock.Lock();
    // Wait for any pending stop to complete
    if (NULL != _stopTask) {
        concurrency::task<void>* stopTask = _stopTask;
        // Release the timer lock
        lock.Unlock();
        // Wait for stop
        stopTask->wait();
        // Re-acquire the timer lock
        lock.Lock();
    }
    // Wait for all alarms to exit
    while (_timersCountdownLatch.Current() != 0) {
        // Release the timer lock
        lock.Unlock();
        status = _timersCountdownLatch.Wait();
        // Re-acquire the timer lock
        lock.Lock();
    }
    // Release the timer lock
    lock.Unlock();
    return status;
}

QStatus Timer::AddAlarm(const Alarm& alarm)
{
    QStatus status = ER_OK;
    // Grab the timer lock
    lock.Lock();
    // Check if timer is running
    if (isRunning) {
        /* Don't allow an infinite number of alarms to exist on this timer */
        while (maxAlarms && (alarms.size() >= maxAlarms) && isRunning) {
            lock.Unlock();
            qcc::Sleep(2);
            lock.Lock();
        }
        /* Ensure timer is still running */
        if (isRunning) {
            try {
                // Create the TimeSpan
                Windows::Foundation::TimeSpan ts = { alarm->computedTimeMillis * HUNDRED_NANOSECONDS_PER_MILLISECOND };

                Alarm a = (Alarm)alarm;
                // Create the timer
                ThreadPoolTimer ^ tpt = ThreadPoolTimer::CreateTimer(ref new TimerElapsedHandler([&, a] (ThreadPoolTimer ^ timer) {
                                                                                                     OSTimer::TimerCallback(timer, (Alarm)a);
                                                                                                 }),
                                                                     ts,
                                                                     ref new TimerDestroyedHandler([&](ThreadPoolTimer ^ timer) {
                                                                                                       OSTimer::TimerCleanupCallback(timer);
                                                                                                   }));
                a->_timer = tpt;
                // Store the alarm associated with the new created timer
                _timerMap[(void*)tpt] = a;
                // Increment the outstanding timers
                _timersCountdownLatch.Increment();
                // Add the alarm to the list
                alarms.insert(a);
            } catch (...) {
                status = ER_FAIL;
            }
        } else {
            status = ER_TIMER_EXITING;
        }
    } else {
        status = ER_TIMER_EXITING;
    }
    // Release the timer lock
    lock.Unlock();
    return status;
}

QStatus Timer::AddAlarmNonBlocking(const Alarm& alarm)
{
    QStatus status = ER_OK;
    // Grab the timer lock
    lock.Lock();
    // Check if timer is running
    if (isRunning) {
        /* Don't allow an infinite number of alarms to exist on this timer */
        if (maxAlarms && (alarms.size() >= maxAlarms)) {
            lock.Unlock();
            return ER_TIMER_FULL;
        }
        try {
            // Create the TimeSpan
            Windows::Foundation::TimeSpan ts = { alarm->computedTimeMillis * HUNDRED_NANOSECONDS_PER_MILLISECOND };

            Alarm a = (Alarm)alarm;
            // Create the timer
            ThreadPoolTimer ^ tpt = ThreadPoolTimer::CreateTimer(ref new TimerElapsedHandler([&, a] (ThreadPoolTimer ^ timer) {
                                                                                                 OSTimer::TimerCallback(timer, (Alarm)a);
                                                                                             }),
                                                                 ts,
                                                                 ref new TimerDestroyedHandler([&](ThreadPoolTimer ^ timer) {
                                                                                                   OSTimer::TimerCleanupCallback(timer);
                                                                                               }));
            a->_timer = tpt;
            // Store the alarm associated with the new created timer
            _timerMap[(void*)tpt] = a;
            // Increment the outstanding timers
            _timersCountdownLatch.Increment();
            // Add the alarm to the list
            alarms.insert(a);
        } catch (...) {
            status = ER_FAIL;
        }
    } else {
        status = ER_TIMER_EXITING;
    }
    // Release the timer lock
    lock.Unlock();
    return status;
}

bool Timer::ForceRemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    //TODO: AJCORE332: Implement forceRemoveAlarm and addWaitQueue as implemented in windows/posix.
    return false;
}
bool Timer::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    bool removed = false;
    qcc::Event evt;
    // Grab the timer lock
    lock.Lock();
    if (alarms.size() > 0) {
        set<Alarm>::iterator it = alarms.find(alarm);
        // Check if alarm is the list
        if (it != alarms.end()) {
            Alarm a = (Alarm) * it;
            // Remove the lookaside
            std::map<void*, qcc::Alarm>::iterator itTimerMap = _timerMap.find((void*)a->_timer);
            if (itTimerMap != _timerMap.end()) {
                _timerMap.erase(itTimerMap);
            }
            // Cancel the firing (if possible)
            if (nullptr != a->_timer) {
                try {
                    a->_timer->Cancel();
                } catch (...) {
                    // Don't bubble OS exceptions out
                }
            }
            // Wait for triggering to finish if specified
            while (blockIfTriggered && a->_latch->Current() != 0) {
                lock.Unlock();
                a->_latch->Wait();
                lock.Lock();
            }
            // Clear out the alarm if someone didn't erase it before re-acquiring the lock
            it = alarms.find(a);
            if (it != alarms.end()) {
                alarms.erase(it);
                removed = true;
            }
        }
    }
    // Release the timer lock
    lock.Unlock();
    return removed;
}

QStatus Timer::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
{
    QStatus status = ER_NO_SUCH_ALARM;
    qcc::Event evt;
    // Get the timer lock
    lock.Lock();
    // Check if alarm is the list
    set<Alarm>::iterator it = alarms.find(origAlarm);
    if (it != alarms.end()) {
        Alarm a = (Alarm) * it;
        // Remove the lookaside
        std::map<void*, qcc::Alarm>::iterator itTimerMap = _timerMap.find((void*)a->_timer);
        if (itTimerMap != _timerMap.end()) {
            _timerMap.erase(itTimerMap);
        }
        // Cancel the firing (if possible)
        if (nullptr != a->_timer) {
            try {
                a->_timer->Cancel();
            } catch (...) {
                // Don't bubble OS exceptions out
            }
        }
        // Wait for triggering to finish if specified
        while (blockIfTriggered && a->_latch->Current() != 0) {
            lock.Unlock();
            a->_latch->Wait();
            lock.Lock();
        }
        // Clear out the alarm if someone didn't erase it before re-acquiring the lock
        it = alarms.find(a);
        if (it != alarms.end()) {
            alarms.erase(it);
        }
        // Ensure the ids are the same as this is replace
        Alarm na = (Alarm)newAlarm;
        na->id = origAlarm->id;
        status = AddAlarm(na);
    }
    // Release the timer lock
    lock.Unlock();
    return status;
}

bool Timer::RemoveAlarm(const AlarmListener& listener, Alarm& alarm)
{
    bool foundOne = false;
    // Grab the timer lock
    lock.Lock();
    // Iterate the alarms
    for (set<Alarm>::iterator it = alarms.begin(); it != alarms.end();) {
        // Check listener for a match
        if ((*it)->listener == &listener) {
            alarm = *it;
            // Remove the alarm
            RemoveAlarm(alarm, false);
            foundOne = true;
            // Reset the iterator
            it = alarms.begin();
        } else {
            ++it;
        }
    }
    // Release the timer lock
    lock.Unlock();
    return foundOne;
}

void Timer::RemoveAlarmsWithListener(const AlarmListener& listener)
{
    Alarm a;
    // Remove all alarms with listener
    while (RemoveAlarm(listener, a)) {
    }
}

bool Timer::HasAlarm(const Alarm& alarm)
{
    bool ret = false;
    // Grab the timer lock
    lock.Lock();
    // No timers if not running
    if (isRunning) {
        ret = alarms.count(alarm) != 0;
    }
    // Release the timer lock
    lock.Unlock();
    return ret;
}

void Timer::ThreadExit(Thread* thread)
{
    // never called
}

void Timer::EnableReentrancy()
{
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    // Grab the timer lock
    lock.Lock();
    // Check for thread handle in ownership map
    if (_reentrancyLockOwner == timerThreadHandle) {
        // Does thread own lock?
        // Unlock the reentrant lock
        reentrancyLock.Unlock();
        // Mark thread as not owning the lock
        _reentrancyLockOwner = NULL;
    } else {
        QCC_DbgPrintf(("Invalid call to Timer::EnableReentrancy from thread %s; only allowed from %s", Thread::GetThreadName(), nameStr.c_str()));
    }
    // Release the timer lock
    lock.Unlock();
}

bool Timer::ThreadHoldsLock()
{
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    // Grab timer lock
    lock.Lock();
    bool retVal = false;
    // Does the thread own the lock?
    if (_reentrancyLockOwner == timerThreadHandle) {
        // Store ownership state
        retVal = true;
    }
    // Release the timer lock
    lock.Unlock();

    return retVal;
}

OSTimer::OSTimer(qcc::Timer* timer) : _timer(timer), _reentrancyLockOwner(NULL), _stopTask(NULL)
{
}

OSTimer::~OSTimer()
{
    // Delete any allocated stop tasks
    if (NULL != _stopTask) {
        delete _stopTask;
        _stopTask = NULL;
    }
}

void OSTimer::TimerCallback(Windows::System::Threading::ThreadPoolTimer ^ timer, Alarm& a)
{
    // Proxy the timer fire callback back into timer
    _timer->TimerCallback((void*)&(std::pair<void*, Alarm&>((void*)timer, a)));  /// create a pair to hand back to the callback
}

void OSTimer::TimerCleanupCallback(Windows::System::Threading::ThreadPoolTimer ^ timer)
{
    // Proxy the timer cleanup callback back into timer
    _timer->TimerCleanupCallback((void*)timer);
}

void OSTimer::StopInternal(bool timerExiting)
{
    // Grab the timer lock
    _timer->lock.Lock();
    // Iterate the alarms in timer
    for (set<Alarm>::iterator it = _timer->alarms.begin(); it != _timer->alarms.end();) {
        const Alarm alarm = *it;
        // *it may be removed by AlarmTriggered() callback, so increase it here
        ++it;
        if (nullptr != alarm->_timer) {
            try {
                // Cancel the timer
                alarm->_timer->Cancel();
            } catch (...) {
                // don't bubble OS exceptions out
            }
        }
        // Check if notification should be sent
        if (_timer->expireOnExit && timerExiting) {
            // Execution here is a sequential flush to notify listeners of exit
            alarm->listener->AlarmTriggered(alarm, ER_TIMER_EXITING);
        }
    }
    // Release the timer lock
    _timer->lock.Unlock();
}

OSAlarm::OSAlarm() : _timer(nullptr)
{
}

/// compare Alarm objects for storage in a priority queue.
bool CompareAlarm::operator()(const Alarm& a1, const Alarm& a2) {
    if (a2->alarmTime < a1->alarmTime) {
        return true;
    }
    if (a1->alarmTime == a2->alarmTime && a1->id > a2->id) {
        return true;
    }
    return false;
};

}
