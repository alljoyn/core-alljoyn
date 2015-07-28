/**
 * @file
 *
 * Alarm declaration
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
#ifndef _QCC_ALARM_H
#define _QCC_ALARM_H

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/atomic.h>

#include <qcc/String.h>
#include <qcc/time.h>
#include <qcc/ManagedObj.h>

#include <Status.h>

namespace qcc {

/** @internal Forward declaration */
class Timer;
class _Alarm;
class TimerImpl;
class TimerThread;

typedef ManagedObj<_Alarm> Alarm;

/**
 * An alarm listener is capable of receiving alarm callbacks
 */
class AlarmListener {
    friend class TimerImpl;
    friend class TimerThread;

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

class _Alarm {
    friend class TimerImpl;
    friend class TimerThread;

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
     * @param limited         Whether this alarm should be counted towards the Timer's max alarm limit
     */
    _Alarm(Timespec absoluteTime, AlarmListener* listener, void* context = NULL, uint32_t periodMs = 0, bool limited = true);

    /**
     * Create an alarm that can be added to a Timer.
     *
     * @param relativeTimed   Number of ms from now that alarm will trigger.
     * @param listener        Object to call when alarm is triggered.
     * @param context         Opaque context passed to listener callback.
     * @param periodMs        Periodicity of alarm in ms or 0 for no repeat.
     * @param limited         Whether this alarm should be counted towards the Timer's max alarm limit
     */
    _Alarm(uint32_t relativeTime, AlarmListener* listener, void* context = NULL, uint32_t periodMs = 0, bool limited = true);

    /**
     * Create an alarm that immediately calls a listener.
     *
     * @param listener        Object to call
     * @param context         Opaque context passed to listener callback.
     * @param limited         Whether this alarm should be counted towards the Timer's max alarm limit
     */
    _Alarm(AlarmListener* listener, void* context = NULL, bool limited = true);

    /**
     * Get context associated with alarm.
     *
     * @return User defined context.
     */
    void* GetContext(void) const;

    /**
     * Set context associated with alarm.
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

    static volatile int32_t nextId;
    Timespec alarmTime;
    AlarmListener* listener;
    uint32_t periodMs;
    mutable void* context;
    int32_t id;
    const bool limitable;               /*< Whether this alarm needs to be counted towards the Timer's max alarm limit */

    _Alarm& operator=(const _Alarm& other);
};

}

#endif
