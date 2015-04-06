/**
 * @file
 *
 * Timer declaration
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
#ifndef _QCC_TIMER_H
#define _QCC_TIMER_H

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/atomic.h>
#include <set>
#include <deque>

#include <qcc/Mutex.h>
#include <qcc/time.h>
#include <qcc/String.h>
#include <qcc/ManagedObj.h>
#include <qcc/Alarm.h>

namespace qcc {

/** @internal Forward declaration */
class Timer;
class _Alarm;
class TimerImpl;
class TimerThread;

class Timer {

  public:

    /**
     * Constructor
     *
     * @param name               Name for the thread.
     * @param expireOnExit       If true call all pending alarms when this thread exits.
     * @param concurrency         Dispatch up to this number of alarms concurently (using multiple threads).
     * @param prevenReentrancy   Prevent re-entrant call of AlarmTriggered.
     * @param maxAlarms          Maximum number of outstanding alarms allowed before blocking calls to AddAlarm or 0 for infinite.
     */
    Timer(qcc::String name, bool expireOnExit = false, uint32_t concurrency = 1, bool preventReentrancy = false, uint32_t maxAlarms = 0);

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
    bool IsRunning() const;

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
    bool HasAlarm(const Alarm& alarm) const;

    /**
     * Allow the currently executing AlarmTriggered callback to be reentered if another alarm is triggered.
     * Calling this method has no effect if timer was created with preventReentrancy == false;
     * Calling this method can only be made from within the AlarmTriggered timer callback.
     */
    void EnableReentrancy();

    /**
     * Check whether the current object is holding the lock
     *
     * @return true if the current Timer is holding the reentrancy lock
     */
    bool IsHoldingReentrantLock() const;

    /**
     * Check whether or not the current thread belongs to this timer instance.
     *
     * @return true if the current thread is a timer thread from this instance
     */
    bool IsTimerCallbackThread() const;

    /**
     * Get the name of the Timer thread pool
     *
     * @return the name of the timer thread(s)
     */
    const qcc::String& GetName() const;

  private:
    /*
     * private copy constructor
     */
    Timer(const Timer&);

    /*
     * private assignment operator
     */
    Timer& operator=(const Timer& src);

    TimerImpl* timerImpl;
};

}

#endif
