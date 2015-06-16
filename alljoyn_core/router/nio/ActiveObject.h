/*
 * ActiveObject.h
 *
 *  Created on: May 14, 2015
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
#ifndef NIO_ACTIVEOBJECT_H_
#define NIO_ACTIVEOBJECT_H_

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

#include "DispatcherBase.h"

namespace nio {

class ActiveObject : public DispatcherBase {
  public:

    ActiveObject(uint32_t num_threads = 1);
    virtual ~ActiveObject();

    ActiveObject(const ActiveObject&) = delete;
    ActiveObject& operator=(const ActiveObject&) = delete;

    void Dispatch(Function f) override;

    void Stop();

  private:

    void Run();

    std::atomic<bool> m_running;
    std::vector<std::thread> m_threads;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<Function> m_closures;
};

} /* namespace nio */

#endif /* NIO_ACTIVEOBJECT_H_ */
