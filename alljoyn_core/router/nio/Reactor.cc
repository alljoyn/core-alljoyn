/*
 * Reactor.cc
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

#include "Reactor.h"
#include "EventNotifier.h"
#include "SocketReadableEvent.h"
#include "SocketWriteableEvent.h"
#include "TimerEvent.h"

#include <assert.h>

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
#   include <sys/epoll.h>
#   include <sys/eventfd.h>
#   include <unistd.h>
#elif defined QCC_OS_DARWIN
#   include <sys/event.h>
#elif defined QCC_OS_GROUP_WINDOWS
#   include <Winsock2.h>
#endif

using namespace nio;

Reactor::Reactor() : m_running(false), reactor_thread(),
#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    epoll_fd(qcc::INVALID_SOCKET_FD), event_fd(qcc::INVALID_SOCKET_FD),
#elif QCC_OS_DARWIN
    kqueue_fd(qcc::INVALID_SOCKET_FD),
#elif defined QCC_OS_GROUP_WINDOWS

#endif
    timerManager()
{

#ifdef QCC_OS_DARWIN
    event_fds[0] = event_fds[1] = qcc::INVALID_SOCKET_FD;
#endif
}

Reactor::~Reactor()
{
}

void Reactor::Register(std::shared_ptr<EventNotifier> notifier)
{
    notifier->SetDispatcher(this);
}

void Reactor::Cancel(std::shared_ptr<EventNotifier> notifier)
{
    notifier->SetEnabled(false);
    notifier->SetDispatcher(nullptr);
}


void Reactor::Register(std::shared_ptr<SocketReadableEvent> sockevent)
{
    qcc::SocketFd fd = sockevent->GetSocket();
    auto func = [sockevent] (QStatus status) {
                    sockevent->SetStatus(status); sockevent->Execute();
                };
    AddReadHandler(fd, func);
}

void Reactor::Cancel(std::shared_ptr<SocketReadableEvent> sockevent)
{
    // we don't want to make the user callback if we are in the process of canceling!
    sockevent->SetEnabled(false);
    RemoveReadHandler(sockevent->GetSocket());
}


void Reactor::Register(std::shared_ptr<SocketWriteableEvent> sockevent)
{
    qcc::SocketFd fd = sockevent->GetSocket();
    auto func = [sockevent] (QStatus status) {
                    sockevent->SetStatus(status); sockevent->Execute();
                };
    AddWriteHandler(fd, func);
}

void Reactor::Cancel(std::shared_ptr<SocketWriteableEvent> sockevent)
{
    // we don't want to make the user callback if we are in the process of canceling
    // because the FD will *NOT* be removed from the epoll set until possibly
    // some time after Reactor::RemoveWriteHandler returns!
    sockevent->SetEnabled(false);
    RemoveWriteHandler(sockevent->GetSocket());
}


void Reactor::Register(std::shared_ptr<TimerEvent> event)
{
    auto cb = [event] (TimerManager::TimerId) {
                  event->Execute();
              };
    TimerManager::TimerId id = timerManager.AddTimer(event->GetFirst(), cb, event->GetRepeat());
    // we need to hold on to the id so we can cancel!
    event->SetId(id);
}

void Reactor::Cancel(std::shared_ptr<TimerEvent> event)
{
    TimerManager::TimerId id = event->GetId();
    event->SetEnabled(false);
    timerManager.CancelTimer(id);
}

void Reactor::Dispatch(Function f)
{
    // run F on the reactor thread the next time we wake up
    {
        std::lock_guard<std::mutex> guard(dispatch_list_lock);
        dispatch_list.push_back(f);
    }

    SignalReactor();
}

void Reactor::AddWriteHandler(qcc::SocketFd sock, SocketFunction cb)
{
    if (reactor_thread != std::this_thread::get_id()) {
        Dispatch(std::bind(&Reactor::AddWriteHandlerInternal, this, sock, cb));
    } else {
        AddWriteHandlerInternal(sock, cb);
    }
}

void Reactor::RemoveWriteHandler(qcc::SocketFd sock)
{
    if (reactor_thread != std::this_thread::get_id()) {
        Dispatch(std::bind(&Reactor::RemoveWriteHandlerInternal, this, sock));
    } else {
        RemoveWriteHandlerInternal(sock);
    }
}

void Reactor::AddWriteHandlerInternal(qcc::SocketFd sock, SocketFunction cb)
{
    SocketInfo& sockinfo = socketMap[sock];
    const bool exists = sockinfo.cb_types & EVENT_Write;
    sockinfo.cb_types |= EVENT_Write;
    // overwrite existing callback
    sockinfo.onWrite = cb;

    // no need to make the syscalls again
    if (exists) {
        return;
    }

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    struct::epoll_event listen_event;
    listen_event.data.fd = sock;
    // if we want readable AND writeable
    if (sockinfo.cb_types & EVENT_Read) {
        listen_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock, &listen_event);
    } else {
        listen_event.events = EPOLLOUT | EPOLLET;
        ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &listen_event);
    }
#elif defined QCC_OS_DARWIN
    struct kevent ev;
    EV_SET(&ev, sock, EVFILT_READ, EV_ADD, 0, 0, 0);
    kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);
#elif defined QCC_OS_GROUP_WINDOWS

#endif
}

void Reactor::RemoveWriteHandlerInternal(qcc::SocketFd sock)
{
    auto it = socketMap.find(sock);
    if (it == socketMap.end()) {
        return;
    }

    SocketInfo& sockinfo = it->second;
    sockinfo.cb_types &= ~EVENT_Write;
    // do this to clear out any references being held by the existing onWrite function
    sockinfo.onWrite = [] (QStatus) {
                       };

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    // if we still want to be writeable
    if (sockinfo.cb_types & EVENT_Read) {
        struct::epoll_event listen_event;
        listen_event.data.fd = sock;
        listen_event.events = EPOLLIN | EPOLLET;
        ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock, &listen_event);
    } else {
        ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
    }
#elif defined QCC_OS_DARWIN
    struct kevent ev;
    EV_SET(&ev, sock, EVFILT_READ, EV_DELETE, 0, 0, 0);
    kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);
#elif defined QCC_OS_GROUP_WINDOWS

#endif

    // if we don't care about this event anymore
    if (sockinfo.cb_types == EVENT_None) {
        socketMap.erase(it);
    }
}

void Reactor::AddReadHandler(qcc::SocketFd sock, SocketFunction cb)
{
    if (reactor_thread != std::this_thread::get_id()) {
        Dispatch(std::bind(&Reactor::AddReadHandlerInternal, this, sock, cb));
    } else {
        AddReadHandlerInternal(sock, cb);
    }
}

void Reactor::RemoveReadHandler(qcc::SocketFd sock)
{
    if (reactor_thread != std::this_thread::get_id()) {
        Dispatch(std::bind(&Reactor::RemoveReadHandlerInternal, this, sock));
    } else {
        RemoveReadHandlerInternal(sock);
    }
}

void Reactor::AddReadHandlerInternal(qcc::SocketFd sock, SocketFunction cb)
{
    SocketInfo& sockinfo = socketMap[sock];
    const bool exists = sockinfo.cb_types & EVENT_Read;
    sockinfo.cb_types |= EVENT_Read;
    sockinfo.onRead = cb;

    // no need to make the syscalls again
    if (exists) {
        return;
    }

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    // if we want readable AND writeable
    struct::epoll_event listen_event;
    listen_event.data.fd = sock;

    if (sockinfo.cb_types & EVENT_Write) {
        listen_event.events = EPOLLOUT | EPOLLIN | EPOLLET;
        ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock, &listen_event);
    } else {
        listen_event.events = EPOLLIN | EPOLLET;
        ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &listen_event);
    }
#elif defined QCC_OS_DARWIN
    struct kevent ev;
    EV_SET(&ev, sock, EVFILT_WRITE, EV_ADD, 0, 0, 0);
    kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);
#elif defined QCC_OS_GROUP_WINDOWS

#endif
}

void Reactor::RemoveReadHandlerInternal(qcc::SocketFd sock)
{
    auto it = socketMap.find(sock);
    if (it == socketMap.end()) {
        return;
    }

    SocketInfo& sockinfo = it->second;
    sockinfo.cb_types &= ~EVENT_Read;

    // do this to clear out any shared_ptr references being held by the existing onRead function
    sockinfo.onRead = [] (QStatus) {
                      };

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    // if we still want to be writeable
    if (sockinfo.cb_types & EVENT_Write) {
        struct::epoll_event listen_event;
        listen_event.data.fd = sock;
        listen_event.events = EPOLLOUT | EPOLLET;
        ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock, &listen_event);
    } else {
        ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
    }
#elif defined QCC_OS_DARWIN
    struct kevent ev;
    EV_SET(&ev, sock, EVFILT_WRITE, EV_ADD, 0, 0, 0);
    kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);
#elif defined QCC_OS_GROUP_WINDOWS

#endif

    if (sockinfo.cb_types == EVENT_None) {
        socketMap.erase(it);
    }
}

void Reactor::SignalReactor()
{
    if (m_running && reactor_thread != std::this_thread::get_id()) {
#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
        uint64_t u64;
        ssize_t rc = ::write(event_fd, &u64, sizeof(u64));
        QCC_UNUSED(rc);
#elif defined QCC_OS_DARWIN
        uint64_t u64;
        ::write(event_fds[1], &u64, sizeof(u64));
#elif defined QCC_OS_GROUP_WINDOWS
        // TODO: how do this thing?
#endif
    }
}

void Reactor::Stop()
{
    if (m_running.exchange(false)) {
        SignalReactor();
    }
}

void Reactor::DispatchEvent(qcc::SocketFd fd, uint32_t cb, bool error)
{
    auto it = socketMap.find(fd);
    if (it != socketMap.end()) {
        SocketInfo& sock = it->second;
        if (sock.cb_types & cb) {
            switch (cb) {
            case EVENT_Read:
                sock.onRead(error ? ER_OS_ERROR : ER_OK);
                break;

            case EVENT_Write:
                sock.onWrite(error ? ER_OS_ERROR : ER_OK);
                break;

            default:
                break;
            }
        }
    }
}

void Reactor::RunDispatchEvents()
{
    // swap out the list while locked
    std::list<Function> events;
    {
        std::lock_guard<std::mutex> guard(dispatch_list_lock);
        events.swap(dispatch_list);
    }

    for (Function f : events) {
        f();
    }
}

void Reactor::Run()
{
    reactor_thread = std::this_thread::get_id();
    m_running = true;

#if defined QCC_OS_LINUX || defined QCC_OS_ANDROID
    epoll_fd = ::epoll_create(1);
    event_fd = ::eventfd(0, 0);

    // need to listen for INPUT on event_fd
    struct::epoll_event listen_event;
    listen_event.events = EPOLLIN;
    listen_event.data.fd = event_fd;
    ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &listen_event);

    while (m_running == true) {
        // first handle the dispatched events
        RunDispatchEvents();

        if (!m_running) {
            break;
        }

        // then the timers
        std::chrono::milliseconds timeout = timerManager.RunTimerCallbacks();

        if (!m_running) {
            break;
        }

        const int MAXEVENTS = 64;
        struct::epoll_event events[MAXEVENTS];
        const int numEvents = ::epoll_wait(epoll_fd, events, MAXEVENTS, timeout != std::chrono::milliseconds::zero() ? timeout.count() : -1);

        if (!m_running) {
            break;
        }

        for (int i = 0; i < numEvents; ++i) {
            struct::epoll_event* event = events + i;
            // handle event
            if (event->data.fd == event_fd) {
                uint64_t u64;
                ssize_t rc = ::read(event_fd, &u64, sizeof(u64));
                QCC_UNUSED(rc);
                continue;
            }

            const bool error = (event->events & EPOLLERR) || (event->events & EPOLLHUP);
            if ((event->events & EPOLLIN) || (event->events & EPOLLPRI)) {
                DispatchEvent(event->data.fd, EVENT_Read, error);
            }

            if (event->events & EPOLLOUT) {
                DispatchEvent(event->data.fd, EVENT_Write, error);
            }

            if (error) {
                // error or disconnect.
                // do not close the socket; the program owns that.
                // however, we can't do anything else with it
                ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event->data.fd, NULL);

                auto it = socketMap.find(event->data.fd);
                if (it != socketMap.end()) {
                    SocketInfo& sock = it->second;

                    // call ONE of the callbacks.  qcc::Send or qcc::Recv will return an error
                    // and the program should know to close the socket
                    if (sock.cb_types & EVENT_Read) {
                        RemoveReadHandler(event->data.fd);
                    }

                    if (sock.cb_types & EVENT_Write) {
                        RemoveWriteHandler(event->data.fd);
                    }

                    socketMap.erase(it);
                }
            }
        }
    }

    ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);

    ::close(event_fd);
    event_fd = -1;

    ::close(epoll_fd);
    epoll_fd = -1;
#elif QCC_OS_DARWIN
    kqueue_fd = kqueue();
    ::pipe(event_fds);

    struct kevent ev;
    EV_SET(&ev, event_fds[0], EVFILT_READ, EV_ADD, 0, 0, 0);
    kevent(kqueue_fd, &ev, 1, NULL, 0, NULL);

    std::vector<struct kevent> events;

    while (m_running == true) {
        RunDispatchEvents();

        if (!m_running) {
            break;
        }

        // always handle timers first
        std::chrono::milliseconds timeout = timerManager.RunTimerCallbacks();

        if (!m_running) {
            break;
        }

        events.resize((2 * socketMap.size()) + 1);
        struct::timespec ts;
        ts.tv_sec = timeout.count() / 1000;
        ts.tv_nsec = 1000 * (timeout.count() % 1000);

        const int numEvents = ::kevent(kqueue_fd, NULL, 0, &events[0], events.size(), timeout != std::chrono::milliseconds::zero() ? &ts : NULL);

        if (!m_running) {
            break;
        }

        for (int i = 0; i < numEvents; ++i) {
            struct kevent* ev = &events[i];
            if (ev->ident == event_fds[0]) {
                // signaled from another thread; nothing to handle
                continue;
            }

            switch (ev->filter) {
            case EVFILT_READ:
                DispatchEvent(ev->ident, EVENT_Read, false);
                break;

            case EVFILT_WRITE:
                DispatchEvent(ev->ident, EVENT_Write, false);
                break;

            default:
                break;
            }
        }
    }

    ::close(event_fds[0]);
    ::close(event_fds[1]);
    ::close(kqueue_fd);
    kqueue_fd = -1;
#elif defined QCC_OS_GROUP_WINDOWS

    while (m_running == true) {
        RunDispatchEvents();
        if (!m_running) {
            break;
        }

        std::chrono::milliseconds timeout = timerManager.RunTimerCallbacks();
        if (!m_running) {
            break;
        }

        struct::timeval tv;
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = 1000 * (timeout.count() % 1000);
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        for (auto it : socketMap) {
            const qcc::SocketFd sock = it.first;
            const SocketInfo& sockinfo = it.second;

            if (sockinfo.cb_types & EVENT_Read) {
                FD_SET(sock, &read_fds);
            }

            if (sockinfo.cb_types & EVENT_Write) {
                FD_SET(sock, &write_fds);
            }
        }

        // TODO: there is probably a much better way to do this on Windows
        int rc = ::select(0, &read_fds, &write_fds, NULL, timeout != std::chrono::milliseconds::zero() ? &tv : NULL);
        if (rc < 0) {
            // ERROR!!!
            m_running = false;
            break;
        }

        for (auto it : socketMap) {
            const qcc::SocketFd sock = it.first;

            // how to detect socket errors?
            bool error = false;

            if (FD_ISSET(sock, &read_fds)) {
                DispatchEvent(sock, EVENT_Read, error);
            }

            if (FD_ISSET(sock, &write_fds)) {
                DispatchEvent(sock, EVENT_Write, error);
            }
        }
    }

#endif

    socketMap.clear();
}
