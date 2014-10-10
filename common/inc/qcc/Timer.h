/**
 * @file
 *
 * Timer thread
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014 AllSeen Alliance. All rights reserved.
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
#ifndef _QCC_TIMER_H
#define _QCC_TIMER_H

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/atomic.h>
#include <set>
#include <deque>

#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/String.h>
#include <qcc/ManagedObj.h>

#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/OSTimer.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/OSTimer.h>
#else
#error No OS GROUP defined.
#endif

namespace qcc {

/** @internal Forward declaration */
class Timer;
class _Alarm;
class TimerThread;

/**
 * An alarm listener is capable of receiving alarm callbacks
 */
class AlarmListener {
    friend class Timer;
    friend class TimerThread;
    friend class OSTimer;

  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AlarmListener() { }

  private:

    /**
     * @param alarm  The alarm that was triggered.
     * @param status The reason the alarm was triggered. This will be either:
     *               #ER_OK               The normal case.
     *               #ER_TIMER_EXITING    The timer thread is exiting.
     */
    virtual void AlarmTriggered(const Alarm& alarm, QStatus reason) = 0;
};

class _Alarm : public OSAlarm {
    friend class Timer;
    friend class TimerThread;
    friend class OSTimer;
    friend class CompareAlarm;

  public:

    /** Disable timeout value */
    static const uint32_t WAIT_FOREVER = static_cast<uint32_t>(-1);

    /**
     * Create a default (unusable) alarm.
     */
    _Alarm();

    /**
     * Create an alarm that can be added to a Timer.
     *
     * @param absoluteTime    Alarm time.
     * @param listener        Object to call when alarm is triggered.
     * @param context         Opaque context passed to listener callback.
     * @param periodMs        Periodicity of alarm in ms or 0 for no repeat.
     */
    _Alarm(Timespec absoluteTime, AlarmListener* listener, void* context = NULL, uint32_t periodMs = 0);

    /**
     * Create an alarm that can be added to a Timer.
     *
     * @param relativeTimed   Number of ms from now that alarm will trigger.
     * @param listener        Object to call when alarm is triggered.
     * @param context         Opaque context passed to listener callback.
     * @param periodMs        Periodicity of alarm in ms or 0 for no repeat.
     */
    _Alarm(uint32_t relativeTime, AlarmListener* listener, void* context = NULL, uint32_t periodMs = 0);

    /**
     * Create an alarm that immediately calls a listener.
     *
     * @param listener        Object to call
     * @param context         Opaque context passed to listener callback.
     */
    _Alarm(AlarmListener* listener, void* context = NULL);

    /**
     * Get context associated with alarm.
     *
     * @return User defined context.
     */
    void* GetContext(void) const;

    /**
     * Set context associated with alarm.
     *
     */
    void SetContext(void* c) const;

    /**
     * Get the absolute alarmTime in milliseconds
     */
    uint64_t GetAlarmTime() const;

    /**
     * Return true if this Alarm's time is less than the passed in alarm's time
     */
    bool operator<(const _Alarm& other) const;

    /**
     * Return true if two alarm instances are equal.
     */
    bool operator==(const _Alarm& other) const;

  private:

    static int32_t nextId;
    Timespec alarmTime;
    AlarmListener* listener;
    uint32_t periodMs;
    mutable void* context;
    int32_t id;
};

/**
 * Alarm is a reference counted (managed) version of _Alarm
 */
typedef qcc::ManagedObj<_Alarm> Alarm;

class Timer : public OSTimer, public ThreadListener {
    friend class TimerThread;
    friend class OSTimer;

  public:

    /**
     * Constructor
     *
     * @param name               Name for the thread.
     * @param expireOnExit       If true call all pending alarms when this thread exits.
     * @param concurency         Dispatch up to this number of alarms concurently (using multiple threads).
     * @param prevenReentrancy   Prevent re-entrant call of AlarmTriggered.
     * @param maxAlarms          Maximum number of outstanding alarms allowed before blocking calls to AddAlarm or 0 for infinite.
     */
    Timer(qcc::String name, bool expireOnExit = false, uint32_t concurency = 1, bool preventReentrancy = false, uint32_t maxAlarms = 0);

    /**
     * Destructor.
     */
    ~Timer();

    /**
     * Start the timer.
     *
     * @return  ER_OK if successful.
     */
    QStatus Start();

    /**
     * Stop the timer (and its associated threads).
     *
     * @return ER_OK if successful.
     */
    QStatus Stop();

    /**
     * Join the timer.
     * Block the caller until all the timer's threads are stopped.
     *
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Return true if Timer is running.
     *
     * @return true iff timer is running.
     */
    bool IsRunning() const { return isRunning; }

    /**
     * Associate an alarm with a timer.
     *
     * @param alarm     Alarm to add.
     * @return ER_OK if alarm was added
     *         ER_TIMER_EXITING if timer is exiting
     */
    QStatus AddAlarm(const Alarm& alarm);

    /**
     * Associate an alarm with a timer.
     * Non-blocking version.
     *
     * @param alarm     Alarm to add.
     * @return ER_OK if alarm was added
     *         ER_TIMER_FULL if timer has maximum allowed alarms
     *         ER_TIMER_EXITING if timer is exiting
     */
    QStatus AddAlarmNonBlocking(const Alarm& alarm);

    /**
     * Disassociate an alarm from a timer.
     *
     * @param alarm             Alarm to remove.
     * @param blockIfTriggered  If alarm has already been triggered, block the caller until AlarmTriggered returns.
     * @return  true iff the given alarm was found and removed.
     */
    bool RemoveAlarm(const Alarm& alarm, bool blockIfTriggered = true);

    bool ForceRemoveAlarm(const Alarm& alarm, bool blockIfTriggered = true);

    /**
     * Remove any alarm for a specific listener returning the alarm. Returns a boolean if an alarm
     * was removed. This function is designed to be called in a loop to remove all alarms for a
     * specific listener. For example this function would be called from the listener's destructor.
     * The alarm is returned so the listener can free and resource referenced by the alarm.
     *
     * @param listener  The specific listener.
     * @param alarm     Alarm that was removed
     * @return  true iff the given alarm was found and removed.
     */
    bool RemoveAlarm(const AlarmListener& listener, Alarm& alarm);

    /**
     * Replace an existing Alarm.
     * Alarms that are  already "in-progress" (meaning they are scheduled for callback) cannot be replaced.
     * In this case, RemoveAlarm will return ER_NO_SUCH_ALARM and may optionally block until the triggered
     * alarm's AlarmTriggered callback has returned.
     *
     * @param origAlarm    Existing alarm to be replaced.
     * @param newAlarm     Alarm that will replace origAlarm if found.
     * @param blockIfTriggered  If alarm has already been triggered, block the caller until AlarmTriggered returns.
     *
     * @return  ER_OK if alarm was replaced
     *          ER_NO_SUCH_ALARM if origAlarm was already triggered or didn't exist.
     *          Any other error indicates that adding newAlarm failed (orig alarm will have been removed in this case).
     */
    QStatus ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered = true);

    /**
     * Remove all pending alarms with a given alarm listener.
     *
     * @param listener   AlarmListener whose alarms will be removed from this timer.
     */
    void RemoveAlarmsWithListener(const AlarmListener& listener);

    /*
     * Test if the specified alarm is associated with this timer.
     *
     * @param alarm     Alarm to check.
     *
     * @return  Returns true if the alarm is associated with this timer, false if not.
     */
    bool HasAlarm(const Alarm& alarm);

    /**
     * Allow the currently executing AlarmTriggered callback to be reentered if another alarm is triggered.
     * Calling this method has no effect if timer was created with preventReentrancy == false;
     * Calling this method can only be made from within the AlarmTriggered timer callback.
     */
    void EnableReentrancy();

    /**
     * Check whether the current TimerThread is holding the lock
     *
     * @return true if the current thread is a timer thread that holds the reentrancy lock
     */
    bool ThreadHoldsLock();

    /**
     * Get the name of the Timer thread pool
     *
     * @return the name of the timer thread(s)
     */
    const qcc::String& GetName() const
    { return nameStr; }

    /**
     * TimerThread ThreadExit callback.
     * For internal use only.
     */
    void ThreadExit(qcc::Thread* thread);

    /**
     * Callback used for asyncronous implementations of timer
     * For internal use only.
     */
    void TimerCallback(void* context);

    /**
     * Callback used for asyncronous implementations of timer
     * For internal use only.
     */
    void TimerCleanupCallback(void* context);

  protected:

    Mutex lock;
    std::set<Alarm, std::less<Alarm> >  alarms;
    Alarm* currentAlarm;
    bool expireOnExit;
    std::vector<TimerThread*> timerThreads;
    bool isRunning;
    int32_t controllerIdx;
    qcc::Timespec yieldControllerTime;
    bool preventReentrancy;
    Mutex reentrancyLock;
    qcc::String nameStr;
    const uint32_t maxAlarms;
    std::deque<qcc::Thread*> addWaitQueue; /**< Threads waiting for alarms set to become not-full */
};

}

#endif
