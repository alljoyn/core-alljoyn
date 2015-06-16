/*
 * TimerEvent.h
 *
 *  Created on: May 18, 2015
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
#ifndef NIO_TIMEREVENT_H_
#define NIO_TIMEREVENT_H_

#include "EventBase.h"
#include "TimerManager.h"

#include <chrono>
#include <functional>

namespace nio {

class TimerEvent final : public EventBase {
  public:

    typedef std::function<void ()> TimerCallback;

    /**
     * Build a new TimerEvent
     *
     * @param when  How long until the timer fires.
     * @param cb    The callback to execute.
     * @param repeat    The repeat time.  Zero for one-shot.
     */
    TimerEvent(
        std::chrono::milliseconds when,
        TimerCallback cb,
        std::chrono::milliseconds repeat = std::chrono::milliseconds::zero());

    virtual ~TimerEvent();

    TimerEvent(const TimerEvent&) = delete;
    TimerEvent& operator=(const TimerEvent&) = delete;

    virtual void ExecuteInternal() override;

    std::chrono::milliseconds GetFirst() const;

    std::chrono::milliseconds GetRepeat() const;

    void SetId(TimerManager::TimerId id);

    TimerManager::TimerId GetId() const;

  private:
    std::chrono::milliseconds when;
    TimerCallback cb;
    std::chrono::milliseconds repeat;
    TimerManager::TimerId id;
};

} /* namespace nio */

#endif /* NIO_TIMEREVENT_H_ */
