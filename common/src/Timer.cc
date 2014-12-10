/**
 * @file
 *
 * Timer thread
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#define QCC_MODULE  "TIMER"

#define WORKER_IDLE_TIMEOUT_MS  20
#define FALLBEHIND_WARNING_MS   500

#define TIMER_IS_DEAD_ALERTCODE  1
#define FORCEREMOVEALARM_ALERTCODE  2

using namespace std;
using namespace qcc;

int32_t qcc::_Alarm::nextId = 0;

namespace qcc {

class TimerThread : public Thread {
  public:

    enum {
        STOPPED,    /**< Thread must be started via Start() */
        STARTING,   /**< Thread has been Started but is not ready to service requests */
        IDLE,       /**< Thrad is sleeping. Waiting to be alerted via Alert() */
        RUNNING,    /**< Thread is servicing an AlarmTriggered callback */
        STOPPING    /**< Thread is stopping due to extended idle time. Not ready for Start or Alert */
    } state;

    TimerThread(const String& name, int index, Timer* timer) :
        Thread(name),
        state(STOPPED),
        hasTimerLock(false),
        index(index),
        timer(timer),
        currentAlarm(NULL)
    { }

    virtual ~TimerThread() { }

    bool hasTimerLock;

    QStatus Start(void* arg, ThreadListener* listener);

    const Alarm* GetCurrentAlarm() const { return currentAlarm; }
    void SetCurrentAlarm(const Alarm* alarm) { currentAlarm = alarm; }

    int GetIndex() const { return index; }

  protected:
    virtual ThreadReturn STDCALL Run(void* arg);

  private:
    const int index;
    Timer* timer;
    const Alarm* currentAlarm;
};

}

_Alarm::_Alarm() : listener(NULL), periodMs(0), context(NULL), id(IncrementAndFetch(&nextId))
{
}

_Alarm::_Alarm(Timespec absoluteTime, AlarmListener* listener, void* context, uint32_t periodMs)
    : alarmTime(absoluteTime), listener(listener), periodMs(periodMs), context(context), id(IncrementAndFetch(&nextId))
{
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
}

_Alarm::_Alarm(AlarmListener* listener, void* context)
    : alarmTime(0, TIME_RELATIVE), listener(listener), periodMs(0), context(context), id(IncrementAndFetch(&nextId))
{
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
    return (alarmTime < other.alarmTime) || ((alarmTime == other.alarmTime) && (id < other.id));
}

bool _Alarm::operator==(const _Alarm& other) const
{
    return (alarmTime == other.alarmTime) && (id == other.id);
}

Timer::Timer(String name, bool expireOnExit, uint32_t concurency, bool preventReentrancy, uint32_t maxAlarms) :
    OSTimer(this),
    currentAlarm(NULL),
    expireOnExit(expireOnExit),
    timerThreads(concurency),
    isRunning(false),
    controllerIdx(0),
    preventReentrancy(preventReentrancy),
    nameStr(name),
    maxAlarms(maxAlarms)
{
    /* Timer thread objects will be created when required */
}

Timer::~Timer()
{
    Stop();
    Join();
    for (uint32_t i = 0; i < timerThreads.size(); ++i) {
        if (timerThreads[i] != NULL) {
            delete timerThreads[i];
            timerThreads[i] = NULL;
        }
    }
}

QStatus Timer::Start()
{
    QStatus status = ER_OK;
    lock.Lock();
    if (!isRunning) {
        controllerIdx = 0;
        isRunning = true;
        if (timerThreads[0] == NULL) {
            String threadName = nameStr + "_" + U32ToString(0);
            timerThreads[0] = new TimerThread(threadName, 0, this);
        }
        status = timerThreads[0]->Start(NULL, this);
        isRunning = false;
        if (status == ER_OK) {
            uint64_t startTs = GetTimestamp64();
            while (timerThreads[0] && (timerThreads[0]->state != TimerThread::IDLE)) {
                if ((startTs + 5000) < GetTimestamp64()) {
                    status = ER_FAIL;
                    break;
                } else {
                    lock.Unlock();
                    Sleep(2);
                    lock.Lock();
                }
            }
        }
        isRunning = (status == ER_OK);
    }
    lock.Unlock();
    return status;
}

QStatus Timer::Stop()
{
    QStatus status = ER_OK;
    lock.Lock();
    isRunning = false;
    for (size_t i = 0; i < timerThreads.size(); ++i) {
        if (timerThreads[i] != NULL) {
            QStatus tStatus = timerThreads[i]->Stop();
            status = (status == ER_OK) ? tStatus : status;
        }
    }

    deque<Thread*>::iterator it = addWaitQueue.begin();
    while (it != addWaitQueue.end()) {
        (*it++)->Alert(TIMER_IS_DEAD_ALERTCODE);
    }
    lock.Unlock();
    return status;
}

QStatus Timer::Join()
{
    QStatus status = ER_OK;
    lock.Lock();
    for (size_t i = 0; i < timerThreads.size(); ++i) {
        if (timerThreads[i] != NULL) {
            lock.Unlock();
            QStatus tStatus = timerThreads[i]->Join();
            lock.Lock();
            status = (status == ER_OK) ? tStatus : status;
        }


    }
    lock.Unlock();
    return status;
}

QStatus Timer::AddAlarm(const Alarm& alarm)
{
    QStatus status = ER_OK;
    lock.Lock();
    if (isRunning) {
        /* Don't allow an infinite number of alarms to exist on this timer */
        while (maxAlarms && (alarms.size() >= maxAlarms) && isRunning) {
            Thread* thread = Thread::GetThread();
            assert(thread);
            addWaitQueue.push_front(thread);
            lock.Unlock(MUTEX_CONTEXT);
            QStatus status1 = Event::Wait(Event::neverSet, Event::WAIT_FOREVER);
            lock.Lock(MUTEX_CONTEXT);
            deque<Thread*>::iterator eit = find(addWaitQueue.begin(), addWaitQueue.end(), thread);
            if (eit != addWaitQueue.end()) {
                addWaitQueue.erase(eit);
            }
            /* Reset alert status */
            if (ER_ALERTED_THREAD == status1) {
                thread->GetStopEvent().ResetEvent();
                if (thread->GetAlertCode() == FORCEREMOVEALARM_ALERTCODE) {
                    lock.Unlock(MUTEX_CONTEXT);
                    return ER_TIMER_EXITING;
                }
            }
        }

        /* Ensure timer is still running */
        if (isRunning) {
            /* Insert the alarm and alert the Timer thread if necessary */
            bool alertThread = alarms.empty() || (alarm < *alarms.begin());
            alarms.insert(alarm);

            if (alertThread && (controllerIdx >= 0)) {
                TimerThread* tt = timerThreads[controllerIdx];
                if (tt->state == TimerThread::IDLE) {
                    status = tt->Alert();
                }
            }
        } else {
            status = ER_TIMER_EXITING;
        }

    } else {
        status = ER_TIMER_EXITING;
    }

    lock.Unlock();
    return status;
}

QStatus Timer::AddAlarmNonBlocking(const Alarm& alarm)
{
    QStatus status = ER_OK;
    lock.Lock();
    if (isRunning) {
        /* Don't allow an infinite number of alarms to exist on this timer */
        if (maxAlarms && (alarms.size() >= maxAlarms)) {
            lock.Unlock();
            return ER_TIMER_FULL;
        }

        /* Insert the alarm and alert the Timer thread if necessary */
        bool alertThread = alarms.empty() || (alarm < *alarms.begin());
        alarms.insert(alarm);

        if (alertThread && (controllerIdx >= 0)) {
            TimerThread* tt = timerThreads[controllerIdx];
            if (tt->state == TimerThread::IDLE) {
                status = tt->Alert();
            }
        }
    } else {
        status = ER_TIMER_EXITING;
    }

    lock.Unlock();
    return status;
}

bool Timer::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    bool foundAlarm = false;
    lock.Lock();
    if (isRunning || expireOnExit) {
        if (alarm->periodMs) {
            set<Alarm>::iterator it = alarms.begin();
            while (it != alarms.end()) {
                if ((*it)->id == alarm->id) {
                    foundAlarm = true;
                    alarms.erase(it);
                    break;
                }
                ++it;
            }
        } else {
            set<Alarm>::iterator it = alarms.find(alarm);
            if (it != alarms.end()) {
                foundAlarm = true;
                alarms.erase(it);
            }
        }
        if (blockIfTriggered && !foundAlarm) {
            /*
             * There might be a call in progress to the alarm that is being removed.
             * RemoveAlarm must not return until this alarm is finished.
             */
            for (size_t i = 0; i < timerThreads.size(); ++i) {
                if ((timerThreads[i] == NULL) || (timerThreads[i] == Thread::GetThread())) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (curAlarm && (*curAlarm == alarm)) {
                    lock.Unlock();
                    qcc::Sleep(2);
                    lock.Lock();
                    if (timerThreads[i] == NULL) {
                        break;
                    }
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
    return foundAlarm;
}

bool Timer::ForceRemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    bool foundAlarm = false;
    lock.Lock();
    if (isRunning || expireOnExit) {
        if (alarm->periodMs) {
            set<Alarm>::iterator it = alarms.begin();
            while (it != alarms.end()) {
                if ((*it)->id == alarm->id) {
                    foundAlarm = true;
                    alarms.erase(it);
                    break;
                }
                ++it;
            }
        } else {
            set<Alarm>::iterator it = alarms.find(alarm);
            if (it != alarms.end()) {
                foundAlarm = true;
                alarms.erase(it);
            }
        }
        if (blockIfTriggered && !foundAlarm) {
            /*
             * There might be a call in progress to the alarm that is being removed.
             * RemoveAlarm must not return until this alarm is finished.
             */
            for (size_t i = 0; i < timerThreads.size(); ++i) {
                if ((timerThreads[i] == NULL) || (timerThreads[i] == Thread::GetThread())) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (curAlarm && (*curAlarm == alarm)) {
                    timerThreads[i]->Alert(FORCEREMOVEALARM_ALERTCODE);
                    lock.Unlock();
                    qcc::Sleep(2);
                    lock.Lock();
                    if (timerThreads[i] == NULL) {
                        break;
                    }
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
    return foundAlarm;
}

QStatus Timer::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
{
    QStatus status = ER_NO_SUCH_ALARM;
    lock.Lock();
    if (isRunning) {
        set<Alarm>::iterator it = alarms.find(origAlarm);
        if (it != alarms.end()) {
            alarms.erase(it);
            status = AddAlarm(newAlarm);
        } else if (blockIfTriggered) {
            /*
             * There might be a call in progress to origAlarm.
             * RemoveAlarm must not return until this alarm is finished.
             */
            for (size_t i = 0; i < timerThreads.size(); ++i) {
                if ((timerThreads[i] == NULL) || (timerThreads[i] == Thread::GetThread())) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (curAlarm && (*curAlarm == origAlarm)) {
                    lock.Unlock();
                    qcc::Sleep(2);
                    lock.Lock();
                    if (timerThreads[i] == NULL) {
                        break;
                    }
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
    return status;
}

bool Timer::RemoveAlarm(const AlarmListener& listener, Alarm& alarm)
{
    bool removedOne = false;
    lock.Lock();
    if (isRunning || expireOnExit) {
        for (set<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
            if ((*it)->listener == &listener) {
                alarm = *it;
                alarms.erase(it);
                removedOne = true;
                break;
            }
        }
        /*
         * This function is most likely being called because the listener is about to be freed. If there
         * are no alarms remaining check that we are not currently servicing an alarm for this listener.
         * If we are, wait until the listener returns.
         */
        if (!removedOne) {
            for (size_t i = 0; i < timerThreads.size(); ++i) {
                if ((timerThreads[i] == NULL) || (timerThreads[i] == Thread::GetThread())) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (curAlarm && ((*curAlarm)->listener == &listener)) {
                    lock.Unlock();
                    qcc::Sleep(5);
                    lock.Lock();
                    if (timerThreads[i] == NULL) {
                        break;
                    }
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
    return removedOne;
}

void Timer::RemoveAlarmsWithListener(const AlarmListener& listener)
{
    Alarm a;
    while (RemoveAlarm(listener, a)) {
    }
}

bool Timer::HasAlarm(const Alarm& alarm)
{
    bool ret = false;
    lock.Lock();
    if (isRunning) {
        ret = alarms.count(alarm) != 0;
    }
    lock.Unlock();
    return ret;
}

QStatus TimerThread::Start(void* arg, ThreadListener* listener)
{
    QStatus status = ER_OK;
    timer->lock.Lock();
    if (timer->isRunning) {
        state = TimerThread::STARTING;
        status = Thread::Start(arg, listener);
    }
    timer->lock.Unlock();
    return status;
}

void Timer::TimerCallback(void* context)
{
}

void Timer::TimerCleanupCallback(void* context)
{
}

ThreadReturn STDCALL TimerThread::Run(void* arg)
{
    QCC_DbgPrintf(("TimerThread::Run()"));

    /*
     * Enter the main loop with the timer lock held.
     */
    timer->lock.Lock();

    while (!IsStopping()) {
        QCC_DbgPrintf(("TimerThread::Run(): Looping."));
        Timespec now;
        GetTimeNow(&now);
        bool isController = (timer->controllerIdx == index);

        QCC_DbgPrintf(("TimerThread::Run(): isController == %d", isController));
        QCC_DbgPrintf(("TimerThread::Run(): controllerIdx == %d", timer->controllerIdx));

        /*
         * If the controller has relinquished its role and is off executing a
         * handler, the first thread back assumes the role of controller.  The
         * controller ensured that some thread would be awakened to come back
         * and do this if there was one idle or stopped.  If all threads were
         * off executing alarms, the first one back will assume the controller
         * role.
         */
        if (!isController && (timer->controllerIdx == -1)) {
            timer->controllerIdx = index;
            isController = true;
            QCC_DbgPrintf(("TimerThread::Run(): Assuming controller role, idx == %d", timer->controllerIdx));
        }

        /*
         * Check for something to do, either now or at some (alarm) time in the
         * future.
         */
        if (!timer->alarms.empty()) {
            QCC_DbgPrintf(("TimerThread::Run(): Alarms pending"));
            const Alarm topAlarm = *(timer->alarms.begin());
            int64_t delay = topAlarm->alarmTime - now;

            /*
             * There is an alarm waiting to go off, but there is some delay
             * until the next alarm is scheduled to pop, so we might want to
             * sleep.  If there is a delay (the alarm is not due now) we sleep
             * if we're the controller thread or if we're a worker and the delay
             * time is low enough to make it worthwhile for the worker not to
             * stop.
             */
            if ((delay > 0) && (isController || (delay < WORKER_IDLE_TIMEOUT_MS))) {
                QCC_DbgPrintf(("TimerThread::Run(): Next alarm delay == %d", delay));
                state = IDLE;

                QStatus status = ER_TIMEOUT;
                if (isController) {
                    /* Since there is delay for the alarm, the controller will first wait for the other
                     * threads to exit and delete their objects.
                     * If a new alarm is added, the controller thread will be alerted and the status
                     * from Event::Wait will be ER_ALERTED_THREAD, causing the loop to be exited.
                     */
                    for (size_t i = 0; i < timer->timerThreads.size(); ++i) {
                        if (i != static_cast<size_t>(index) && timer->timerThreads[i] != NULL) {

                            while ((timer->timerThreads[i]->state != TimerThread::STOPPED || timer->timerThreads[i]->IsRunning()) && timer->isRunning && status == ER_TIMEOUT && delay > WORKER_IDLE_TIMEOUT_MS) {
                                timer->lock.Unlock();
                                status = Event::Wait(Event::neverSet, WORKER_IDLE_TIMEOUT_MS);
                                timer->lock.Lock();
                                GetTimeNow(&now);
                                delay = topAlarm->alarmTime - now;
                            }

                            if (status == ER_ALERTED_THREAD || status == ER_STOPPING_THREAD || !timer->isRunning || delay <= WORKER_IDLE_TIMEOUT_MS) {
                                break;
                            }
                            if (timer->timerThreads[i]->state == TimerThread::STOPPED && !timer->timerThreads[i]->IsRunning()) {
                                delete timer->timerThreads[i];
                                timer->timerThreads[i] = NULL;
                                QCC_DbgPrintf(("TimerThread::Run(): Deleted unused worker thread %d", i));
                            }
                        }
                    }

                }
                if ((status == ER_TIMEOUT) && (delay > 0)) {
                    timer->lock.Unlock();
                    Event evt(static_cast<uint32_t>(delay), 0);
                    Event::Wait(evt);
                    timer->lock.Lock();
                }
                stopEvent.ResetEvent();
            } else if (isController || (delay <= 0)) {
                QCC_DbgPrintf(("TimerThread::Run(): Next alarm is due now"));
                /*
                 * There is an alarm waiting to go off.  We are either the
                 * controller or the alarm is past due.  If the alarm is past
                 * due, we want to print an error message if we are getting too
                 * far behind.
                 *
                 * Note that this does not necessarily mean that something bad
                 * is happening.  In the case of a threadpool, for example,
                 * since all threads are dispatched "now," corresponding alarms
                 * will always be late.  It is the case, however, that a
                 * generally busy system would begin to fall behind, and it
                 * would be useful to know this.  Therefore we do log a message
                 * if the system gets too far behind.  We define "too far" by
                 * the constant FALLBEHIND_WARNING_MS.
                 */
                if (delay < 0 && std::abs((long)delay) > FALLBEHIND_WARNING_MS) {
                    QCC_DbgHLPrintf(("TimerThread::Run(): Timer \"%s\" alarm is late by %ld ms",
                                     Thread::GetThreadName(), std::abs((long)delay)));
                }

                state = RUNNING;
                stopEvent.ResetEvent();
                timer->lock.Unlock();

                /* Get the reentrancy lock if necessary */
                hasTimerLock = timer->preventReentrancy;
                if (hasTimerLock) {
                    timer->reentrancyLock.Lock();
                }

                /*
                 * There is an alarm due to be executed now, and we are either
                 * the controller thread or a worker thread executing now.  in
                 * either case, we are going to handle the alarm at the head of
                 * the list.
                 */
                timer->lock.Lock();


                TimerThread* tt = NULL;
                int nullIdx = -1;
                /*
                 * There may be several threads wandering through this code.
                 * One of them is acting as the controller, whose job it is to
                 * wake up or spin up threads to replace it when it goes off to
                 * execute an alarm.  If there are no more alarms to execute,
                 * the controller goes idle, but worker threads stop and exit.
                 *
                 * At this point, we know we have an alarm to execute.  Ideally
                 * we just want to directly execute the alarm without doing a
                 * context switch, so whatever thread (worker or controller) is
                 * executing should handle the alarm.
                 *
                 * Since the alarm may take an arbitrary length of time to
                 * complete, if we are the controller, we need to make sure that
                 * there is another thread that can assume the controller role
                 * if we are off executing an alarm.  An idle thread means it is
                 * waiting for an alarm to become due.  A stopped thread means
                 * it has exited since it found no work to do.
                 */
                if (isController) {
                    QCC_DbgPrintf(("TimerThread::Run(): Controller looking for worker"));

                    /*
                     * Look for an idle or stopped worker to execute alarm callback for us.
                     */
                    while (!tt && timer->isRunning && (timer->timerThreads.size() > 1)) {
                        /* Whether all other timer threads are running/starting. */
                        bool allOtherThreadsRunning = true;

                        for (size_t i = 0; i < timer->timerThreads.size(); ++i) {
                            if (i != static_cast<size_t>(index)) {
                                if (timer->timerThreads[i] == NULL) {
                                    if (nullIdx == -1) {
                                        nullIdx = i;
                                    }
                                    allOtherThreadsRunning = false;
                                    continue;
                                }
                                if (timer->timerThreads[i]->state != TimerThread::RUNNING && timer->timerThreads[i]->state != TimerThread::STARTING) {
                                    allOtherThreadsRunning = false;
                                }
                                if (timer->timerThreads[i]->state == TimerThread::IDLE) {
                                    tt = timer->timerThreads[i];
                                    QCC_DbgPrintf(("TimerThread::Run(): Found idle worker at index %d", i));
                                    break;
                                } else if (timer->timerThreads[i]->state == TimerThread::STOPPED && !timer->timerThreads[i]->IsRunning()) {
                                    tt = timer->timerThreads[i];
                                    QCC_DbgPrintf(("TimerThread::Run(): Found stopped worker at index %d", i));
                                }
                            }
                        }
                        if (tt || !timer->isRunning || allOtherThreadsRunning || nullIdx != -1) {
                            /* Break out of loop if one of the following is true:
                             * 1. Found an idle/stopped worker.
                             * 2. A NULL entry was found
                             * 3. Timer has been stopped.
                             * 4. All other timer threads are currently starting/running(probably processing other alarms).
                             */
                            break;
                        }
                        timer->lock.Unlock();
                        Sleep(2);
                        timer->lock.Lock();
                    }


                    if (timer->isRunning) {
                        if (!tt && nullIdx != -1) {
                            /* If <tt> is NULL and we have located an index in the timerThreads vector
                             * that is NULL, allocate memory so we can start this thread
                             */
                            String threadName = timer->nameStr + "_" + U32ToString(nullIdx);
                            timer->timerThreads[nullIdx] = new TimerThread(threadName, nullIdx, timer);
                            tt = timer->timerThreads[nullIdx];
                            QCC_DbgPrintf(("TimerThread::Run(): Created timer thread %d", nullIdx));
                        }
                        if (tt) {
                            /*
                             * If <tt> is non-NULL, then we have located a thread that
                             * will be able to take over the controller role if
                             * required, so either wake it up or start it depending on
                             * its current state. Also, ensure that the timer has not been
                             * stopped.
                             */
                            QCC_DbgPrintf(("TimerThread::Run(): Have timer thread (tt)"));
                            if (tt->state == TimerThread::IDLE) {
                                QCC_DbgPrintf(("TimerThread::Run(): Alert()ing idle timer thread (tt)"));
                                QStatus status = tt->Alert();
                                if (status != ER_OK) {
                                    QCC_LogError(status, ("Error alerting timer thread %s", tt->GetName()));
                                }
                            } else if (tt->state == TimerThread::STOPPED) {
                                QCC_DbgPrintf(("TimerThread::Run(): Start()ing stopped timer thread (tt)"));
                                QStatus status = tt->Start(NULL, timer);
                                if (status != ER_OK) {
                                    QCC_LogError(status, ("Error starting timer thread %s", tt->GetName()));
                                }
                            }
                        }
                    }

                    /*
                     * If we are the controller, then we are going to have to yield
                     * our role since the alarm may take an arbitrary length of time
                     * to execute.  The next thread that wends its way through this
                     * run loop will assume the role.
                     */
                    timer->controllerIdx = -1;
                    GetTimeNow(&timer->yieldControllerTime);
                    QCC_DbgPrintf(("TimerThread::Run(): Yielding controller role"));
                    isController = false;
                }
                /* Make sure the alarm has not been serviced yet.
                 * If it has already been serviced by another thread, just ignore
                 * and go back to the top of the loop.
                 */
                set<Alarm>::iterator it = timer->alarms.find(topAlarm);
                if (it != timer->alarms.end()) {
                    Alarm top = *it;
                    timer->alarms.erase(it);
                    currentAlarm = &top;
                    if (0 < timer->addWaitQueue.size()) {
                        Thread* wakeMe = timer->addWaitQueue.back();
                        timer->addWaitQueue.pop_back();
                        QStatus status = wakeMe->Alert();
                        if (ER_OK != status) {
                            QCC_LogError(status, ("Failed to alert thread blocked on full tx queue"));
                        }
                    }
                    timer->lock.Unlock();

                    QCC_DbgPrintf(("TimerThread::Run(): ******** AlarmTriggered()"));
                    (top->listener->AlarmTriggered)(top, ER_OK);
                    if (hasTimerLock) {
                        hasTimerLock = false;
                        timer->reentrancyLock.Unlock();
                    }
                    timer->lock.Lock();
                    /* If ForceRemoveAlarm has been called for this alarm, we need to reset the alert code.
                     * Note that this must be atomic with setting the currentAlarm to NULL.
                     */
                    Thread* thread = Thread::GetThread();
                    if (thread->GetAlertCode() == FORCEREMOVEALARM_ALERTCODE) {
                        thread->ResetAlertCode();
                    }
                    currentAlarm = NULL;

                    if (0 != top->periodMs) {
                        top->alarmTime += top->periodMs;
                        if (top->alarmTime < now) {
                            top->alarmTime = now;
                        }
                        QCC_DbgPrintf(("TimerThread::Run(): Adding back periodic alarm"));
                        timer->AddAlarm(top);
                    }
                } else {
                    if (hasTimerLock) {
                        hasTimerLock = false;
                        timer->reentrancyLock.Unlock();
                    }
                }
            } else {
                /*
                 * This is a worker (non-controller) thread with nothing to do
                 * immediately, so we idle for WORKER_IDLE_TIMEOUT_MS and then
                 * stop it until we have a need for it to be consuming resources.
                 */
                state = IDLE;
                QCC_DbgPrintf(("TimerThread::Run(): Worker with nothing to do"));
                timer->lock.Unlock();
                QStatus status = Event::Wait(Event::neverSet, WORKER_IDLE_TIMEOUT_MS);
                timer->lock.Lock();
                if (status == ER_TIMEOUT && timer->controllerIdx != -1) {
                    QCC_DbgPrintf(("TimerThread::Run(): Worker with nothing to do stopping"));
                    state = STOPPING;
                    break;
                }
                stopEvent.ResetEvent();
            }
        } else {
            /*
             * The alarm list is empty, so we only have a need to have a single
             * controller thread running.  If we are that controller, we wait
             * until there is something to do.  If we are not that controller,
             * we idle for WORKER_IDLE_TIMEOUT_MS and then stop running so
             * we don't consume resources.
             */
            QCC_DbgPrintf(("TimerThread::Run(): Alarm list is empty"));
            if (isController) {
                /* Since there are no alarms, the controller will first wait for the other
                 * threads to exit and delete their objects.
                 * If a new alarm is added, the controller thread will be alerted and the status
                 * from Event::Wait will be ER_ALERTED_THREAD, causing the loop to be exited.
                 */
                state = IDLE;
                QStatus status = ER_TIMEOUT;
                for (size_t i = 0; i < timer->timerThreads.size(); ++i) {
                    if (i != static_cast<size_t>(index) && timer->timerThreads[i] != NULL) {

                        while ((timer->timerThreads[i]->state != TimerThread::STOPPED || timer->timerThreads[i]->IsRunning()) && timer->isRunning && status == ER_TIMEOUT) {
                            timer->lock.Unlock();
                            status = Event::Wait(Event::neverSet, WORKER_IDLE_TIMEOUT_MS);
                            timer->lock.Lock();
                        }
                        if (status == ER_ALERTED_THREAD || status == ER_STOPPING_THREAD || !timer->isRunning) {
                            break;
                        }
                        if (timer->timerThreads[i]->state == TimerThread::STOPPED && !timer->timerThreads[i]->IsRunning()) {
                            delete timer->timerThreads[i];
                            timer->timerThreads[i] = NULL;
                            QCC_DbgPrintf(("TimerThread::Run(): Deleted unused worker thread %d", i));
                        }

                    }
                }

                QCC_DbgPrintf(("TimerThread::Run(): Controller going idle"));
                if (status == ER_TIMEOUT) {
                    /* The controller has successfully deleted objects of all other worker threads.
                     * and has not been alerted/stopped.
                     */
                    timer->lock.Unlock();
                    Event::Wait(Event::neverSet);
                    timer->lock.Lock();
                }
                stopEvent.ResetEvent();
            } else {
                QCC_DbgPrintf(("TimerThread::Run(): non-Controller idling"));
                state = IDLE;
                timer->lock.Unlock();
                QStatus status = Event::Wait(Event::neverSet, WORKER_IDLE_TIMEOUT_MS);
                timer->lock.Lock();
                if (status == ER_TIMEOUT && timer->controllerIdx != -1) {
                    QCC_DbgPrintf(("TimerThread::Run(): non-Controller stopping"));
                    state = STOPPING;
                    break;
                }
                stopEvent.ResetEvent();
            }
        }
    }

    /*
     * We entered the main loop with the lock taken, so we need to give it here.
     */
    state = STOPPING;
    timer->lock.Unlock();
    return (ThreadReturn) 0;
}

void Timer::ThreadExit(Thread* thread)
{
    TimerThread* tt = static_cast<TimerThread*>(thread);
    lock.Lock();
    if ((!isRunning) && expireOnExit) {
        /* Call all alarms */
        while (!alarms.empty()) {
            /*
             * Note it is possible that the callback will call RemoveAlarm()
             */
            set<Alarm>::iterator it = alarms.begin();
            Alarm alarm = *it;
            alarms.erase(it);
            tt->SetCurrentAlarm(&alarm);
            lock.Unlock();
            tt->hasTimerLock = preventReentrancy;
            if (tt->hasTimerLock) {
                reentrancyLock.Lock();
            }
            alarm->listener->AlarmTriggered(alarm, ER_TIMER_EXITING);
            if (tt->hasTimerLock) {
                tt->hasTimerLock = false;
                reentrancyLock.Unlock();
            }
            lock.Lock();
            tt->SetCurrentAlarm(NULL);
        }
    }
    tt->state = TimerThread::STOPPED;
    lock.Unlock();
    tt->Join();
}

void Timer::EnableReentrancy()
{
    Thread* thread = Thread::GetThread();
    lock.Lock();
    bool allowed = false;
    for (uint32_t i = 0; i < timerThreads.size(); ++i) {
        if ((timerThreads[i] != NULL) && (static_cast<Thread*>(timerThreads[i]) == thread)) {
            allowed = true;
            break;
        }
    }
    lock.Unlock();
    if (allowed) {
        TimerThread* tt = static_cast<TimerThread*>(thread);
        if (tt->hasTimerLock) {
            tt->hasTimerLock = false;
            reentrancyLock.Unlock();
        }
    } else {
        QCC_LogError(ER_TIMER_NOT_ALLOWED, ("Invalid call to Timer::EnableReentrancy from thread %s", Thread::GetThreadName()));
    }
}

bool Timer::ThreadHoldsLock()
{
    Thread* thread = Thread::GetThread();
    lock.Lock();
    bool allowed = false;
    for (uint32_t i = 0; i < timerThreads.size(); ++i) {
        if ((timerThreads[i] != NULL) && (static_cast<Thread*>(timerThreads[i]) == thread)) {
            allowed = true;
            break;
        }
    }
    lock.Unlock();
    if (allowed) {
        TimerThread* tt = static_cast<TimerThread*>(thread);
        return tt->hasTimerLock;
    }

    return false;
}

OSTimer::OSTimer(qcc::Timer* timer) : _timer(timer)
{
}

