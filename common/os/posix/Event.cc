/**
 * @file
 *
 * Linux implementation of thread event.
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/Event.h>

#if defined(MECHANISM_EVENTFD) && defined(MECHANISM_PIPE)
#error "Cannot specifiy both MECHANISM_EVENTFD and MECHANISM_PIPE"
#endif

#if !defined(MECHANISM_EVENTFD) && !defined(MECHANISM_PIPE)
#error "Must specify MECHANISM_EVENTFD or MECHANISM_PIPE"
#endif

#if defined(MECHANISM_EVENTFD)
#include <sys/eventfd.h>
#endif

#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#if defined(QCC_OS_DARWIN)
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE "EVENT"

#if defined(MECHANISM_PIPE)
static Mutex* pipeLock = NULL;
static vector<pair<int, int> >* freePipeList;
static vector<pair<int, int> >* usedPipeList;
#endif


#if defined(QCC_OS_DARWIN)
QStatus Event::Wait(Event& evt, uint32_t maxWaitMs)
{
    struct timespec tval;
    struct timespec* pTval = NULL;

    Thread* thread = Thread::GetThread();

    int kq = kqueue();
    if (kq == -1) {
        QCC_LogError(ER_OS_ERROR, ("kqueue creation failed with %d (%s)", errno, strerror(errno)));
        return ER_OS_ERROR;
    }
    struct kevent chlist[2];
    struct kevent evlist[2];
    uint32_t processed = 0;

    if (maxWaitMs != WAIT_FOREVER) {
        tval.tv_sec = maxWaitMs / 1000;
        tval.tv_nsec = (maxWaitMs % 1000) * 1000000;
        pTval = &tval;
    }

    if (evt.eventType == TIMED) {
        uint32_t now = GetTimestamp();
        if (evt.timestamp <= now) {
            if (0 < evt.period) {
                evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
            }
            close(kq);
            return ER_OK;
        } else if (!pTval || ((evt.timestamp - now) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_nsec / 1000000))) {
            tval.tv_sec = (evt.timestamp - now) / 1000;
            tval.tv_nsec = 1000000 * ((evt.timestamp - now) % 1000);
            pTval = &tval;
        }
    } else {
        if (0 <= evt.fd) {
            EV_SET(&chlist[processed], evt.fd, ((evt.eventType == IO_WRITE) ? EVFILT_WRITE : EVFILT_READ), EV_ADD, 0, 0, 0);
            processed++;
        } else if (0 <= evt.ioFd) {
            EV_SET(&chlist[processed], evt.ioFd, ((evt.eventType == IO_WRITE) ? EVFILT_WRITE : EVFILT_READ), EV_ADD, 0, 0, 0);
            processed++;
        }
    }

    int stopFd = -1;
    if (thread) {
        stopFd = thread->GetStopEvent().fd;
        EV_SET(&chlist[processed], stopFd, EVFILT_READ, EV_ADD, 0, 0, 0);
        processed++;
    }

    evt.IncrementNumThreads();

    int ret = kevent(kq, chlist, processed, evlist, processed,  pTval);

    evt.DecrementNumThreads();

    if (0 < ret && 0 <= stopFd) {
        for (int n = 0; n < ret; ++n) {
            if ((evlist[n].filter == EVFILT_READ) && evlist[n].ident == stopFd) {
                close(kq);
                return thread->IsStopping() ? ER_STOPPING_THREAD : ER_ALERTED_THREAD;
            }
        }
    }
    if (0 <= ret && evt.eventType == TIMED) {
        uint32_t now = GetTimestamp();
        if (now >= evt.timestamp) {
            if (0 < evt.period) {
                evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
            }
            close(kq);
            return ER_OK;
        } else {
            close(kq);
            return ER_TIMEOUT;
        }
    } else if ((0 < ret) && ((0 <= evt.fd) || (0 <= evt.ioFd))) {
        for (int n = 0; n < ret; ++n) {
            if ((evlist[n].filter == EVFILT_WRITE) && (evt.eventType == IO_WRITE) && (evlist[n].ident == evt.fd || evlist[n].ident == evt.ioFd)) {
                close(kq);
                return ER_OK;
            } else if ((evlist[n].filter == EVFILT_READ) && (evt.eventType == IO_READ || evt.eventType == GEN_PURPOSE) && (evlist[n].ident == evt.fd || evlist[n].ident == evt.ioFd)) {
                close(kq);
                return ER_OK;
            }
        }
        close(kq);
        return ER_TIMEOUT;
    }  else if (0 <= ret) {
        close(kq);
        return ER_TIMEOUT;
    } else {
        close(kq);
        return ER_FAIL;
    }
}
#else
QStatus Event::Wait(Event& evt, uint32_t maxWaitMs)
{
    struct timeval tval;
    struct timeval* pTval = NULL;

    Thread* thread = Thread::GetThread();

#if defined(QCC_OS_LINUX)
    int epollfd = epoll_create1(0);
#elif defined (QCC_OS_ANDROID)
    int epollfd = epoll_create(2);
#endif

    if (epollfd == -1) {
        QCC_LogError(ER_OS_ERROR, ("epoll_create failed with %d (%s)", errno, strerror(errno)));
        return ER_OS_ERROR;
    }
    struct epoll_event ev, events[2];
    if (maxWaitMs != WAIT_FOREVER) {
        tval.tv_sec = maxWaitMs / 1000;
        tval.tv_usec = (maxWaitMs % 1000) * 1000;
        pTval = &tval;
    }

    if (evt.eventType == TIMED) {
        uint32_t now = GetTimestamp();
        if (evt.timestamp <= now) {
            if (0 < evt.period) {
                evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
            }
            close(epollfd);
            return ER_OK;
        } else if (!pTval || ((evt.timestamp - now) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_usec / 1000))) {
            tval.tv_sec = (evt.timestamp - now) / 1000;
            tval.tv_usec = 1000 * ((evt.timestamp - now) % 1000);
            pTval = &tval;
        }
    } else {
        if (0 <= evt.fd) {
            ev.events = (evt.eventType == IO_WRITE) ? EPOLLOUT : EPOLLIN;
            ev.data.fd = evt.fd;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evt.fd, &ev) == -1) {
                if (errno == EEXIST) {
                    QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", evt.fd));
                } else {
                    QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", evt.fd, errno, strerror(errno)));
                    close(epollfd);
                    return ER_OS_ERROR;
                }
            }
        } else if (0 <= evt.ioFd) {
            ev.events = (evt.eventType == IO_WRITE) ? EPOLLOUT : EPOLLIN;
            ev.data.fd = evt.ioFd;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evt.ioFd, &ev) == -1) {
                if (errno == EEXIST) {
                    QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", evt.ioFd));
                } else {
                    QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", evt.ioFd, errno, strerror(errno)));
                    close(epollfd);
                    return ER_OS_ERROR;
                }
            }
        }
    }

    int stopFd = -1;
    if (thread) {
        stopFd = thread->GetStopEvent().fd;
        ev.events = EPOLLIN;
        ev.data.fd = stopFd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, stopFd, &ev) == -1) {
            if (errno == EEXIST) {
                QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", stopFd));
            } else {
                QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", stopFd, errno, strerror(errno)));
                close(epollfd);
                return ER_OS_ERROR;
            }
        }
    }

    evt.IncrementNumThreads();

    int ret = epoll_wait(epollfd, events, 2, pTval ? ((pTval->tv_sec * 1000) + (pTval->tv_usec / 1000)) : -1);

    evt.DecrementNumThreads();

    if (0 < ret && 0 <= stopFd) {
        for (int n = 0; n < ret; ++n) {
            if ((events[n].events & EPOLLIN) && events[n].data.fd == stopFd) {
                close(epollfd);
                return thread->IsStopping() ? ER_STOPPING_THREAD : ER_ALERTED_THREAD;
            }
        }
    }
    if (0 <= ret && evt.eventType == TIMED) {
        uint32_t now = GetTimestamp();
        if (now >= evt.timestamp) {
            if (0 < evt.period) {
                evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
            }
            close(epollfd);
            return ER_OK;
        } else {
            close(epollfd);
            return ER_TIMEOUT;
        }
    } else if ((0 < ret) && ((0 <= evt.fd) || (0 <= evt.ioFd))) {
        for (int n = 0; n < ret; ++n) {
            if ((events[n].events & EPOLLOUT) && evt.eventType == IO_WRITE && (events[n].data.fd == evt.fd || events[n].data.fd == evt.ioFd)) {
                close(epollfd);
                return ER_OK;
            }
            if ((events[n].events & EPOLLIN) && (evt.eventType == IO_READ || evt.eventType == GEN_PURPOSE) && (events[n].data.fd == evt.fd || events[n].data.fd == evt.ioFd)) {
                close(epollfd);
                return ER_OK;
            }
        }
        close(epollfd);
        return ER_TIMEOUT;
    } else if (0 <= ret) {
        close(epollfd);
        return ER_TIMEOUT;
    } else {
        close(epollfd);
        return ER_FAIL;
    }
}
#endif

#if defined(QCC_OS_DARWIN)
QStatus Event::Wait(const vector<Event*>& checkEvents, vector<Event*>& signaledEvents, uint32_t maxWaitMs)
{
    struct timespec tval;
    struct timespec* pTval = NULL;

    if (maxWaitMs != WAIT_FOREVER) {
        tval.tv_sec = maxWaitMs / 1000;
        tval.tv_nsec = (maxWaitMs % 1000) * 1000000;
        pTval = &tval;
    }

    vector<Event*>::const_iterator it;
    uint32_t size = checkEvents.empty() ? 1 : checkEvents.size();

    int kq = kqueue();
    if (kq == -1) {
        QCC_LogError(ER_OS_ERROR, ("kqueue creation failed with %d (%s)", errno, strerror(errno)));
        return ER_OS_ERROR;
    }
    struct kevent chlist[size];
    struct kevent evlist[size];
    uint32_t processed = 0;

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->IncrementNumThreads();
        if ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE)) {
            if (0 <= evt->fd) {
                EV_SET(&chlist[processed], evt->fd, EVFILT_READ, EV_ADD, 0, 0, 0);
                processed++;
            } else if (0 <= evt->ioFd) {
                EV_SET(&chlist[processed], evt->ioFd, EVFILT_READ, EV_ADD, 0, 0, 0);
                processed++;
            }
        } else if (evt->eventType == IO_WRITE) {
            if (0 <= evt->fd) {
                EV_SET(&chlist[processed], evt->fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
                processed++;
            } else if (0 <= evt->ioFd) {
                EV_SET(&chlist[processed], evt->ioFd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
                processed++;
            }
        } else if (evt->eventType == TIMED) {
            uint32_t now = GetTimestamp();
            if (evt->timestamp <= now) {
                tval.tv_sec = 0;
                tval.tv_nsec = 0;
                pTval = &tval;
            } else if (!pTval || ((evt->timestamp - now) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_nsec / 1000000))) {
                tval.tv_sec = (evt->timestamp - now) / 1000;
                tval.tv_nsec = 1000000 * ((evt->timestamp - now) % 1000);
                pTval = &tval;
            }
        }
    }

    int ret = kevent(kq, chlist, processed, evlist, processed,  pTval);

    if (0 <= ret) {
        for (int n = 0; n < ret; ++n) {
            for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
                Event* evt = *it;
                if ((evlist[n].filter == EVFILT_READ) && ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE))) {
                    if (((0 <= evt->fd) && evlist[n].ident == evt->fd) || ((0 <= evt->ioFd)  && evlist[n].ident == evt->ioFd)) {
                        signaledEvents.push_back(evt);
                        break;
                    }
                } else if ((evlist[n].filter == EVFILT_WRITE) && (evt->eventType == IO_WRITE)) {
                    if (((0 <= evt->fd) && evlist[n].ident == evt->fd) || ((0 <= evt->ioFd) && evlist[n].ident == evt->ioFd)) {
                        signaledEvents.push_back(evt);
                        break;
                    }
                }
            }
        }
        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            Event* evt = *it;
            evt->DecrementNumThreads();
            if (evt->eventType == TIMED) {
                uint32_t now = GetTimestamp();
                if (evt->timestamp <= now) {
                    signaledEvents.push_back(evt);
                    if (0 < evt->period) {
                        evt->timestamp += (((now - evt->timestamp) / evt->period) + 1) * evt->period;
                    }
                }
            }
        }
        close(kq);
        return signaledEvents.empty() ? ER_TIMEOUT : ER_OK;
    } else {
        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            (*it)->DecrementNumThreads();
        }
        QCC_LogError(ER_OS_ERROR, ("kevent failed with %d (%s)", errno, strerror(errno)));
        return ER_OS_ERROR;
    }
}
#else
QStatus Event::Wait(const vector<Event*>& checkEvents, vector<Event*>& signaledEvents, uint32_t maxWaitMs)
{
    struct timeval tval;
    struct timeval* pTval = NULL;

    if (maxWaitMs != WAIT_FOREVER) {
        tval.tv_sec = maxWaitMs / 1000;
        tval.tv_usec = (maxWaitMs % 1000) * 1000;
        pTval = &tval;
    }

    vector<Event*>::const_iterator it, jit;
    uint32_t size = checkEvents.empty() ? 1 : checkEvents.size();

#if defined(QCC_OS_LINUX)
    int epollfd = epoll_create1(0);
#elif defined (QCC_OS_ANDROID)
    int epollfd = epoll_create(size);
#endif

    if (epollfd == -1) {
        QCC_LogError(ER_OS_ERROR, ("epoll_create failed with %d (%s)", errno, strerror(errno)));
        return ER_OS_ERROR;
    }
    struct epoll_event ev, events[size];

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->IncrementNumThreads();
        if ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE)) {
            if (0 <= evt->fd) {
                ev.events = EPOLLIN;
                for (jit = checkEvents.begin(); jit != checkEvents.end(); ++jit) {
                    Event* event = *jit;
                    if ((event->fd == evt->fd || event->ioFd == evt->fd) && event->eventType == IO_WRITE) {
                        ev.events |= EPOLLOUT;
                        break;
                    }
                }
                ev.data.fd = evt->fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evt->fd, &ev) == -1) {
                    if (errno == EEXIST) {
                        QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", evt->fd));
                    } else {
                        QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", evt->fd, errno, strerror(errno)));
                        close(epollfd);
                        return ER_OS_ERROR;
                    }
                }
            } else if (0 <= evt->ioFd) {
                ev.events = EPOLLIN;
                for (jit = checkEvents.begin(); jit != checkEvents.end(); ++jit) {
                    Event* event = *jit;
                    if ((event->fd == evt->ioFd || event->ioFd == evt->ioFd) && event->eventType == IO_WRITE) {
                        ev.events |= EPOLLOUT;
                        break;
                    }
                }
                ev.data.fd = evt->ioFd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evt->ioFd, &ev) == -1) {
                    if (errno == EEXIST) {
                        QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", evt->ioFd));
                    } else {
                        QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", evt->ioFd, errno, strerror(errno)));
                        close(epollfd);
                        return ER_OS_ERROR;
                    }
                }
            }
        } else if (evt->eventType == IO_WRITE) {
            if (0 <= evt->fd) {
                ev.events = EPOLLOUT;
                for (jit = checkEvents.begin(); jit != checkEvents.end(); ++jit) {
                    Event* event = *jit;
                    if ((event->fd == evt->fd || event->ioFd == evt->fd) && ((event->eventType == IO_READ) || (event->eventType == GEN_PURPOSE))) {
                        ev.events |= EPOLLIN;
                        break;
                    }
                }
                ev.data.fd = evt->fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evt->fd, &ev) == -1) {
                    if (errno == EEXIST) {
                        QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", evt->fd));
                    } else {
                        QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", evt->fd, errno, strerror(errno)));
                        close(epollfd);
                        return ER_OS_ERROR;
                    }
                }
            } else if (0 <= evt->ioFd) {
                ev.events = EPOLLOUT;
                for (jit = checkEvents.begin(); jit != checkEvents.end(); ++jit) {
                    Event* event = *jit;
                    if ((event->fd == evt->ioFd || event->ioFd == evt->ioFd) && ((event->eventType == IO_READ) || (event->eventType == GEN_PURPOSE))) {
                        ev.events |= EPOLLIN;
                        break;
                    }
                }
                ev.data.fd = evt->ioFd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evt->ioFd, &ev) == -1) {
                    if (errno == EEXIST) {
                        QCC_DbgPrintf(("Duplicate epoll_ctl add for fd %u", evt->ioFd));
                    } else {
                        QCC_LogError(ER_OS_ERROR, ("epoll_ctl add failed for fd %u with %d (%s)", evt->ioFd, errno, strerror(errno)));
                        close(epollfd);
                        return ER_OS_ERROR;
                    }
                }
            }
        } else if (evt->eventType == TIMED) {
            uint32_t now = GetTimestamp();
            if (evt->timestamp <= now) {
                tval.tv_sec = 0;
                tval.tv_usec = 0;
                pTval = &tval;
            } else if (!pTval || ((evt->timestamp - now) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_usec / 1000))) {
                tval.tv_sec = (evt->timestamp - now) / 1000;
                tval.tv_usec = 1000 * ((evt->timestamp - now) % 1000);
                pTval = &tval;
            }
        }
    }

    int ret = epoll_wait(epollfd, events, size, pTval ? ((pTval->tv_sec * 1000) + (pTval->tv_usec / 1000)) : -1);

    if (0 <= ret) {
        for (int n = 0; n < ret; ++n) {
            for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
                Event* evt = *it;
                if ((events[n].events & EPOLLIN) && ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE))) {
                    if (((0 <= evt->fd) && events[n].data.fd == evt->fd) || ((0 <= evt->ioFd)  && events[n].data.fd == evt->ioFd)) {
                        signaledEvents.push_back(evt);
                    }
                } else if ((events[n].events & EPOLLOUT) && (evt->eventType == IO_WRITE)) {
                    if (((0 <= evt->fd) && events[n].data.fd == evt->fd) || ((0 <= evt->ioFd) && events[n].data.fd == evt->ioFd)) {
                        signaledEvents.push_back(evt);
                    }
                }
            }
        }

        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            Event* evt = *it;
            evt->DecrementNumThreads();
            if (evt->eventType == TIMED) {
                uint32_t now = GetTimestamp();
                if (evt->timestamp <= now) {
                    signaledEvents.push_back(evt);
                    if (0 < evt->period) {
                        evt->timestamp += (((now - evt->timestamp) / evt->period) + 1) * evt->period;
                    }
                }
            }
        }
        close(epollfd);
        return signaledEvents.empty() ? ER_TIMEOUT : ER_OK;
    } else {
        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            (*it)->DecrementNumThreads();
        }
        QCC_LogError(ER_OS_ERROR, ("epoll_wait failed with %d  (%s)", errno, strerror(errno)));
        close(epollfd);
        return ER_OS_ERROR;
    }
}
#endif

#if defined(MECHANISM_PIPE)

static void CreateMechanism(int* rdFd, int* wrFd)
{
#ifdef DEBUG_EVENT_LEAKS
    int fds[2];
    int ret = pipe(fds);
    *rdFd = fds[0];
    *wrFd = fds[1];
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
#else
    /* TODO: Potential thread safety issue here */
    if (NULL == pipeLock) {
        pipeLock = new Mutex();
        freePipeList = new vector<pair<int, int> >;
        usedPipeList = new vector<pair<int, int> >;
    }

    /* Check for something on the free pipe list */
    pipeLock->Lock();
    if (!freePipeList->empty()) {
        pair<int, int> fdPair = freePipeList->back();
        usedPipeList->push_back(fdPair);
        freePipeList->pop_back();
        *rdFd = fdPair.first;
        *wrFd = fdPair.second;
    } else {
        /* The free event list is empty so we must allocate a new one */
        int fds[2];
        int ret = pipe(fds);
        if (0 == ret) {
            fcntl(fds[0], F_SETFL, O_NONBLOCK);
            usedPipeList->push_back(pair<int, int>(fds[0], fds[1]));
            *rdFd = fds[0];
            *wrFd = fds[1];
        } else {
            QCC_LogError(ER_FAIL, ("Failed to create pipe. (%d) %s", errno, strerror(errno)));
        }
    }
    pipeLock->Unlock();
#endif
}

static void DestroyMechanism(int rdFd, int wrFd)
{
#ifdef DEBUG_EVENT_LEAKS
    close(rdFd);
    close(wrFd);
#else
    pipeLock->Lock();

    /*
     * Delete the pipe (permanently) if the number of pipes on the free list is twice as many as
     * on the used list.
     */
    bool closePipe = (freePipeList->size() >= (2 * (usedPipeList->size() - 1)));

    /* Look for pipe on usedPipeList */
    vector<pair<int, int> >::iterator it = usedPipeList->begin();
    bool foundPipe = false;
    while (it != usedPipeList->end()) {
        if (it->first == rdFd) {
            if (closePipe) {
                close(rdFd);
                close(wrFd);
            } else {
                freePipeList->push_back(*it);
            }
            usedPipeList->erase(it);
            foundPipe = true;
            break;
        }
        ++it;
    }

    if (foundPipe) {
        if (usedPipeList->size() == 0) {
            /* Empty the free list if this was the last pipe in use */
            vector<pair<int, int> >::iterator it = freePipeList->begin();
            while (it != freePipeList->end()) {
                close(it->first);
                close(it->second);
                it = freePipeList->erase(it);
            }
        } else if (closePipe) {
            /* Trim freeList down to 2*used pipe */
            while (freePipeList->size() > (2 * usedPipeList->size())) {
                pair<int, int> fdPair = freePipeList->back();
                close(fdPair.first);
                close(fdPair.second);
                freePipeList->pop_back();
            }
        } else {
            /* Make sure pipe is empty if reusing */
            char buf[32];
            int ret = sizeof(buf);
            while (sizeof(buf) == ret) {
                ret = read(rdFd, buf, sizeof(buf));
            }
        }
        pipeLock->Unlock();
    } else {
        pipeLock->Unlock();
    }
#endif
}

/*
 * The semantics of the general purpose event is that we only set and reset the
 * event.  We use select to decide if the event has become signalled via a call
 * to SetMechanism.  There are no P() or V() operations, so the event just
 * remains signalled until a ResetMechanism is done.  In order to signal an
 * event, we write something into the underlying pipe which makes the read side
 * appear signalled.  In order to reset it we simply read from the pipe to
 * remove the data.  As long as there are bits in the pipe, a select will find
 * the readFd readable and we consider the event signaled.
 */
static QStatus SetMechanism(int signalFd)
{
    char val = 's';
    /*
     * In order to signal our event, we write a byte into the pipe, which then
     * becomes ready and the event becomes signaled.  Multiple writes into the
     * pipe are possible if multiple calls to SetEvent() are made.  We don't
     * attempt to put together a thread-safe way to limit writes to exactly one
     * so ResetMechanism() will have to read until the pipe becomes empty.
     */
    int ret = write(signalFd, &val, sizeof(val));
    return ret >= 0 ? ER_OK : ER_FAIL;
}

static QStatus ResetMechanism(int fd)
{
    char buf[32];
    int ret = sizeof(buf);

    /*
     * In order to reset our event, we read from the pipe until there are no
     * longer any bytes in it which then makes the associated fd not ready and
     * the event becomes signaled.  Since SetMechanism() doesn't guarantee only
     * one write to the pipe, we have to read all of the bytes that may be
     * there.
     */
    while (sizeof(buf) == ret) {
        ret = read(fd, buf, sizeof(buf));
    }

    /*
     * If we successfully read, the eventfd is reset, which is okay.
     */
    if (ret >= 0) {
        return ER_OK;
    }

    /*
     * If we get EAGAIN or EWOULDBLOCK, the eventfd was already reset, which is
     * okay.
     */
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return ER_OK;
    }

    /*
     * This is a real error;
     */
    return ER_FAIL;
}

#endif // defined(MECHANISM_PIPE)

#if defined(MECHANISM_EVENTFD)

/*
 * Create an underlying mechanism for events if using the eventfd event
 * notification mechanism.
 */
static void CreateMechanism(int* readFd, int* writeFd)
{
    QCC_DbgTrace(("CreateMechanism()"));
    int efd = eventfd(0, O_NONBLOCK); // EFD_NONBLOCK poorly defined in openWRT, so go right to the source
    if (efd < 0) {
        QCC_LogError(ER_FAIL, ("CreateMechanism(): Unable to create eventfd (%d:\"%s\")", errno, strerror(errno)));
    }
    *readFd = *writeFd = efd;
}

/*
 * Destroy an existing underlying mechanism for events using the eventfd event
 * notification mechanism.
 */
static void DestroyMechanism(int readFd, int writeFd)
{
    QCC_DbgTrace(("DestroyMechanism()"));
    assert(readFd == writeFd && "destroyMechanism(): expect readFd == writeFd for eventfd mechanism");
    close(readFd);
}

/*
 * The semantics of the general purpose event is that we only set and reset the
 * event.  We use select to decide if the event has become signalled via a call
 * to SetMechanism.  There are no P() or V() operations, so the event just
 * remains signalled until a ResetMechanism is done.  In order to signal an
 * event, we write a positive 64-bit integer to the FD.  In order to reset it,
 * we write a zero.  As long as the contents of the FD are non-zero, a select
 * will find the eventfd readable and we consider the event signaled.
 */
static QStatus SetMechanism(int efd)
{
    QCC_DbgTrace(("SetMechanism()"));

    /*
     * You signal an eventfd simply by writing a positive value which is added
     * to its internal count.  If that value does not overflow, it goes to
     * signaled.
     */
    uint64_t val = 1;
    ssize_t ret = write(efd, &val, sizeof(val));
    return ret >= 0 ? ER_OK : ER_FAIL;
}

static QStatus ResetMechanism(int efd)
{
    QCC_DbgTrace(("ResetMechanism()"));
    uint64_t val;

    /*
     * You reset an eventfd simply by reading from it, which will reset its
     * associated count to zero and make it not signaled.
     */
    ssize_t ret = read(efd, &val, sizeof(val));

    /*
     * If we successfully read, the eventfd is reset, which is okay.
     */
    if (ret >= 0) {
        return ER_OK;
    }

    /*
     * If we get EAGAIN or EWOULDBLOCK, the eventfd was already reset, which is
     * okay.
     */
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return ER_OK;
    }

    /*
     * This is a real error;
     */
    return ER_FAIL;
}

#endif // defined(MECHANISM_EVENTFD)

Event::Event() : fd(-1), signalFd(-1), ioFd(-1), eventType(GEN_PURPOSE), numThreads(0)
{
    CreateMechanism(&fd, &signalFd);
}

Event::Event(SocketFd ioFd, EventType eventType)
    : fd(-1), signalFd(-1), ioFd(ioFd), eventType(eventType), timestamp(0), period(0), numThreads(0)
{
}

Event::Event(Event& event, EventType eventType, bool genPurpose)
    : fd(-1), signalFd(-1), ioFd(event.ioFd), eventType(eventType), timestamp(0), period(0), numThreads(0)
{
    if (genPurpose) {
        CreateMechanism(&fd, &signalFd);
    }
}

Event::Event(uint32_t timestamp, uint32_t period)
    : fd(-1),
    signalFd(-1),
    ioFd(-1),
    eventType(TIMED),
    timestamp(WAIT_FOREVER == timestamp ? WAIT_FOREVER : GetTimestamp() + timestamp),
    period(period),
    numThreads(0)
{
}

Event::~Event()
{
    /* Threads should not be waiting */
    if ((GEN_PURPOSE == eventType) || (TIMED == eventType)) {
        SetEvent();
    }

    /* Destroy eventfd if one was created */
    if (GEN_PURPOSE == eventType) {
        DestroyMechanism(fd, signalFd);
    }
}

QStatus Event::SetEvent()
{
    QStatus status;

    if (GEN_PURPOSE == eventType) {
        status = SetMechanism(signalFd);
    } else if (TIMED == eventType) {
        uint32_t now = GetTimestamp();
        if (now < timestamp) {
            if (0 < period) {
                timestamp -= (((now - timestamp) / period) + 1) * period;
            } else {
                timestamp = now;
            }
        }
        status = ER_OK;
    } else {
        /* Not a general purpose event */
        status = ER_FAIL;
        QCC_LogError(status, ("Attempt to manually set an I/O event"));
    }
    return status;
}

QStatus Event::ResetEvent()
{
    QStatus status = ER_OK;

    if (GEN_PURPOSE == eventType) {
        status = ResetMechanism(fd);
        if (ER_OK != status) {
            QCC_LogError(status, ("pipe read failed with %d (%s)", errno, strerror(errno)));
        }
    } else if (TIMED == eventType) {
        if (0 < period) {
            uint32_t now = GetTimestamp();
            if (now >= timestamp) {
                timestamp += (((now - timestamp) / period) + 1) * period;
            }
        } else {
            timestamp = static_cast<uint32_t>(-1);
        }
    } else {
        /* Not a general purpose event */
        status = ER_FAIL;
        QCC_LogError(status, ("Attempt to manually reset an I/O event"));
    }
    return status;
}

bool Event::IsSet()
{
    QStatus status(Wait(*this, 0));
    return (status == ER_OK || status == ER_ALERTED_THREAD);
}

void Event::ResetTime(uint32_t delay, uint32_t period)
{
    if (delay == WAIT_FOREVER) {
        this->timestamp = WAIT_FOREVER;
    } else {
        this->timestamp = GetTimestamp() + delay;
    }
    this->period = period;
}
