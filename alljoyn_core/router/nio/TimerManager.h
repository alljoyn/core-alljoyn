/*
 * TimerManager.h
 *
 *  Created on: Apr 3, 2015
 *      Author: erongo
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
#ifndef NIO_TIMERMANAGER_H_
#define NIO_TIMERMANAGER_H_

#include <chrono>
#include <map>
#include <functional>
#include <mutex>

namespace nio {

class TimerManager {
  public:

    typedef uint64_t TimerId;

    TimerManager();
    virtual ~TimerManager();

    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;

    typedef std::function<void (TimerId)> TimerCallback;

    /**
     * Start a timer based on relative time 'when'
     *
     * @param when  The time when the callback happens, relative to now
     * @param cb    The function to call
     * @param repeat    How often to repeat after the initial time
     *
     * @return      A unique identifier representing the time to use
     */
    TimerId AddTimer(
        std::chrono::milliseconds when,
        TimerCallback cb,
        std::chrono::milliseconds repeat = std::chrono::milliseconds::zero());

    /**
     * Run all expired timers
     *
     * @return  the number of MILLISECONDS until the next timer expires
     */
    std::chrono::milliseconds RunTimerCallbacks();

    /**
     * Cancel the timer specified
     *
     * @param id    The id returned by AddTimer
     */
    void CancelTimer(TimerId id);

  private:

    static TimerId NextTimer;

    void RunInternalCallback(TimerCallback cb, std::chrono::milliseconds repeat, TimerId id);

    // map of callbacks, sorted by absolute expiration time
    std::multimap<std::chrono::steady_clock::time_point, TimerId> m_timeouts;

    // mapping of id to user callback
    std::map<TimerId, TimerCallback> m_callbacks;

    std::mutex m_lock;
};

}

#endif /* NIO_TIMERMANAGER_H_ */
