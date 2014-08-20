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

static Mutex* pipeLock = NULL;
static vector<pair<int, int> >* freePipeList;
static vector<pair<int, int> >* usedPipeList;

Event Event::alwaysSet(0, 0);

Event Event::neverSet(WAIT_FOREVER, 0);

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

    uint32_t startTime = 0;
    if (pTval) {
        startTime = GetTimestamp();
    }

    evt.IncrementNumThreads();

    int ret = kevent(kq, chlist, processed, evlist, processed,  pTval);
    while (ret < 0 && errno == EINTR) {
        if (pTval) {
            uint32_t now = GetTimestamp();
            if ((now - startTime) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_nsec / 1000000)) {
                tval.tv_sec -= ((now - startTime) / 1000);
                tval.tv_nsec -= (1000000 * ((now - startTime) % 1000));
                pTval = &tval;
            } else {
                tval.tv_sec = 0;
                tval.tv_nsec = 0;
                pTval = &tval;
            }
            startTime = GetTimestamp();
        }
        ret = kevent(kq, chlist, processed, evlist, processed,  pTval);
    }

    evt.DecrementNumThreads();

    if (0 < ret && 0 <= stopFd) {
        for (int n = 0; n < ret; ++n) {
            if ((evlist[n].filter & EVFILT_READ) && evlist[n].ident == stopFd) {
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
            if ((evlist[n].filter & EVFILT_WRITE) && (evt.eventType == IO_WRITE) && (evlist[n].ident == evt.fd || evlist[n].ident == evt.ioFd)) {
                close(kq);
                return ER_OK;
            } else if ((evlist[n].filter & EVFILT_READ) && (evt.eventType == IO_READ || evt.eventType == GEN_PURPOSE) && (evlist[n].ident == evt.fd || evlist[n].ident == evt.ioFd)) {
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

    uint32_t startTime = 0;
    if (pTval) {
        startTime = GetTimestamp();
    }

    evt.IncrementNumThreads();

    int ret = epoll_wait(epollfd, events, 2, pTval ? ((pTval->tv_sec * 1000) + (pTval->tv_usec / 1000)) : -1);
    while (ret < 0 && errno == EINTR) {
        if (pTval) {
            uint32_t now = GetTimestamp();
            if ((now - startTime) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_usec / 1000)) {
                tval.tv_sec -= ((now - startTime) / 1000);
                tval.tv_usec -= (1000 * ((now - startTime) % 1000));
                pTval = &tval;
            } else {
                tval.tv_sec = 0;
                tval.tv_usec = 0;
                pTval = &tval;
            }
            startTime = GetTimestamp();
        }
        ret = epoll_wait(epollfd, events, 2, pTval ? ((pTval->tv_sec * 1000) + (pTval->tv_usec / 1000)) : -1);
    }
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

    uint32_t startTime = 0;
    if (pTval) {
        startTime = GetTimestamp();
    }

    int ret = kevent(kq, chlist, processed, evlist, processed,  pTval);
    while (ret < 0 && errno == EINTR) {
        if (pTval) {
            uint32_t now = GetTimestamp();
            if ((now - startTime) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_nsec / 1000000)) {
                tval.tv_sec -= ((now - startTime) / 1000);
                tval.tv_nsec -= (1000000 * ((now - startTime) % 1000));
                pTval = &tval;
            } else {
                tval.tv_sec = 0;
                tval.tv_nsec = 0;
                pTval = &tval;
            }
            startTime = GetTimestamp();
        }
        ret = kevent(kq, chlist, processed, evlist, processed,  pTval);
    }

    if (0 <= ret) {
        for (int n = 0; n < ret; ++n) {
            for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
                Event* evt = *it;
                if ((evlist[n].filter & EVFILT_READ) && ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE))) {
                    if (((0 <= evt->fd) && evlist[n].ident == evt->fd) || ((0 <= evt->ioFd)  && evlist[n].ident == evt->ioFd)) {
                        signaledEvents.push_back(evt);
                        break;
                    }
                } else if ((evlist[n].filter & EVFILT_WRITE) && (evt->eventType == IO_WRITE)) {
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

    vector<Event*>::const_iterator it;
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

    uint32_t startTime = 0;
    if (pTval) {
        startTime = GetTimestamp();
    }

    int ret = epoll_wait(epollfd, events, size, pTval ? ((pTval->tv_sec * 1000) + (pTval->tv_usec / 1000)) : -1);
    while (ret < 0 && errno == EINTR) {
        if (pTval) {
            uint32_t now = GetTimestamp();
            if ((now - startTime) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_usec / 1000)) {
                tval.tv_sec -= ((now - startTime) / 1000);
                tval.tv_usec -= (1000 * ((now - startTime) % 1000));
                pTval = &tval;
            } else {
                tval.tv_sec = 0;
                tval.tv_usec = 0;
                pTval = &tval;
            }
            startTime = GetTimestamp();
        }
        ret = epoll_wait(epollfd, events, size, pTval ? ((pTval->tv_sec * 1000) + (pTval->tv_usec / 1000)) : -1);
    }

    if (0 <= ret) {
        for (int n = 0; n < ret; ++n) {
            for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
                Event* evt = *it;
                if ((events[n].events & EPOLLIN) && ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE))) {
                    if (((0 <= evt->fd) && events[n].data.fd == evt->fd) || ((0 <= evt->ioFd)  && events[n].data.fd == evt->ioFd)) {
                        signaledEvents.push_back(evt);
                        break;
                    }
                } else if ((events[n].events & EPOLLOUT) && (evt->eventType == IO_WRITE)) {
                    if (((0 <= evt->fd) && events[n].data.fd == evt->fd) || ((0 <= evt->ioFd) && events[n].data.fd == evt->ioFd)) {
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

static void createPipe(int* rdFd, int* wrFd)
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

static void destroyPipe(int rdFd, int wrFd)
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

Event::Event() : fd(-1), signalFd(-1), ioFd(-1), eventType(GEN_PURPOSE), numThreads(0)
{
    createPipe(&fd, &signalFd);
}

Event::Event(SocketFd ioFd, EventType eventType)
    : fd(-1), signalFd(-1), ioFd(ioFd), eventType(eventType), timestamp(0), period(0), numThreads(0)
{
}

Event::Event(Event& event, EventType eventType, bool genPurpose)
    : fd(-1), signalFd(-1), ioFd(event.ioFd), eventType(eventType), timestamp(0), period(0), numThreads(0)
{
    if (genPurpose) {
        createPipe(&fd, &signalFd);
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
        destroyPipe(fd, signalFd);
    }
}

QStatus Event::SetEvent()
{
    QStatus status;

    if (GEN_PURPOSE == eventType) {
        char val = 's';
        fd_set rdSet;
        struct timeval tv;
        tv.tv_sec = tv.tv_usec = 0;
        FD_ZERO(&rdSet);
        FD_SET(fd, &rdSet);
        int ret = select(fd + 1, &rdSet, NULL, NULL, &tv);
        if (ret == 0) {
            ret = write(signalFd, &val, sizeof(val));
        }
        status = (ret == 1) ? ER_OK : ER_FAIL;
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
        char buf[32];
        int ret = sizeof(buf);
        while (sizeof(buf) == ret) {
            ret = read(fd, buf, sizeof(buf));
        }
        status = ((0 < ret) || ((-1 == ret) && (EAGAIN == errno))) ? ER_OK : ER_FAIL;
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
