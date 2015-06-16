/*
 * Reactor.h
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

#ifndef NIO_REACTOR_H_
#define NIO_REACTOR_H_

#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include <list>
#include <memory>

#include <qcc/Socket.h>

#include "TimerManager.h"
#include "DispatcherBase.h"

namespace nio {

class EventNotifier;
class SocketReadableEvent;
class SocketWriteableEvent;
class TimerEvent;

class Reactor : public DispatcherBase {

    friend class Proactor;

  public:

    Reactor();
    virtual ~Reactor();

    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    /**
     * Run the reactor.  This function will block the calling thread until StopReactor is called.
     */
    void Run();

    /**
     * Stop the reactor.  Make RunReactor return.  This can be called from any thread.
     */
    void Stop();

    /**
     * Register an notifier event
     */
    void Register(std::shared_ptr<EventNotifier> notifier);
    void Cancel(std::shared_ptr<EventNotifier> notifier);

    /**
     * Register a socket readable event
     */
    void Register(std::shared_ptr<SocketReadableEvent> sockevent);
    void Cancel(std::shared_ptr<SocketReadableEvent> sockevent);

    /**
     * Register a socket writeable event
     */
    void Register(std::shared_ptr<SocketWriteableEvent> sockevent);
    void Cancel(std::shared_ptr<SocketWriteableEvent> sockevent);

    /**
     * Register a timer event
     */
    void Register(std::shared_ptr<TimerEvent> event);
    void Cancel(std::shared_ptr<TimerEvent> event);


  private:
    /**
     * A callback when something happens with the socket
     */
    typedef std::function<void (QStatus)> SocketFunction;

    /**
     * Make the callback cb when the socket sock becomes readable
     */
    void AddReadHandler(qcc::SocketFd sock, SocketFunction cb);

    /**
     * Cancel the read handler for socket sock
     */
    void RemoveReadHandler(qcc::SocketFd sock);

    /**
     * Make the callback cb when the socket sock becomes writeable
     */
    void AddWriteHandler(qcc::SocketFd sock, SocketFunction cb);

    /**
     * Cancel the write handler for socket sock
     */
    void RemoveWriteHandler(qcc::SocketFd sock);


    void Dispatch(Function f) override;

    /**
     * Wake up the looper
     */
    void SignalReactor();

    void AddReadHandlerInternal(qcc::SocketFd sock, SocketFunction cb);
    void RemoveReadHandlerInternal(qcc::SocketFd sock);
    void AddWriteHandlerInternal(qcc::SocketFd sock, SocketFunction cb);
    void RemoveWriteHandlerInternal(qcc::SocketFd sock);

    void RunDispatchEvents();

    std::atomic<bool> m_running;
    std::thread::id reactor_thread;

    std::list<Function> dispatch_list;
    std::mutex dispatch_list_lock;

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    int epoll_fd;
    int event_fd;
#elif defined QCC_OS_DARWIN
    int kqueue_fd;
    int event_fds[2];
#elif defined QCC_OS_GROUP_WINDOWS
#endif

    TimerManager timerManager;

    static const uint32_t EVENT_None = 0x00;
    static const uint32_t EVENT_Read = 0x01;
    static const uint32_t EVENT_Write = 0x02;

    void DispatchEvent(qcc::SocketFd sock, uint32_t cb, bool error);

    struct SocketInfo {
        SocketInfo() : cb_types(EVENT_None) { }

        SocketFunction onRead;
        SocketFunction onWrite;

        uint32_t cb_types;
    };

    // this should only ever be touched by the reactor thread so no lock is necessary
    std::map<qcc::SocketFd, SocketInfo> socketMap;
};

}

#endif /* NIO_REACTOR_H_ */
