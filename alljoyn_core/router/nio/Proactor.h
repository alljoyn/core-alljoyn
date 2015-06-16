/*
 * Proactor.h
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
#ifndef NIO_PROACTOR_H_
#define NIO_PROACTOR_H_

#include "ActiveObject.h"
#include "Reactor.h"
#include "DispatcherBase.h"

#include <thread>

namespace nio {

class EventNotifier;
class SocketReadableEvent;
class SocketWriteableEvent;
class TimerEvent;

/**
 * The Proactor owns a Reactor and dispatches callbacks to an ActiveObject.
 * The Reactor uses a single thread to wait for events and makes the callbacks that
 * have been associated with those events.
 *
 * Those callbacks are intercepted by the Proactor and placed into a queue.  The callbacks
 * are then picked up by the ActiveObject's thread pool and executed.  Callbacks registered
 * with the Proactor are not guaranteed to happen on the same thread so protection
 * must be used.
 */
class Proactor : public DispatcherBase {
  public:
    Proactor(uint32_t num_threads);
    virtual ~Proactor();

    /**
     * As with Reactor, Run will block until Stop() is called
     */
    void Run();

    /**
     * Stop running.
     */
    void Stop();

    /**
     * Register an notifier event.  This is used to signal the reactor thread
     * to wake up and do something.  That "something" is specified by the
     * callback in EventNotifier.
     */
    void Register(std::shared_ptr<EventNotifier> notifier);
    void Cancel(std::shared_ptr<EventNotifier> notifier);

    /**
     * Execute a callback when a socket becomes readable
     */
    void Register(std::shared_ptr<SocketReadableEvent> sockevent);
    void Cancel(std::shared_ptr<SocketReadableEvent> sockevent);

    /**
     * Execute a callback when a socket becomes writeable
     */
    void Register(std::shared_ptr<SocketWriteableEvent> sockevent);
    void Cancel(std::shared_ptr<SocketWriteableEvent> sockevent);

    /**
     * Execute a callback after a specified amount of time.
     */
    void Register(std::shared_ptr<TimerEvent> event);
    void Cancel(std::shared_ptr<TimerEvent> event);


    /**
     * Dispatch function F to the ActiveObject.
     * This will bypass the Reactor.
     */
    void Dispatch(Function f) override;

  private:

    Proactor(const Proactor&) = delete;
    Proactor& operator=(const Proactor&) = delete;

    void ReRegisterRead(qcc::SocketFd fd);
    void ReRegisterWrite(qcc::SocketFd fd);

    Reactor reactor;
    ActiveObject activeObject;

    std::map<qcc::SocketFd, std::shared_ptr<SocketReadableEvent> > readEvents;
    std::mutex readEventsLock;

    std::map<qcc::SocketFd, std::shared_ptr<SocketWriteableEvent> > writeEvents;
    std::mutex writeEventsLock;
};

}

#endif /* NIO_PROACTOR_H_ */
