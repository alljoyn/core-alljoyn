/**
 * @file
 *
 * Linux implementation of thread event.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE "EVENT"

static Mutex* pipeLock = NULL;
static vector<pair<int, int> >* freePipeList;
static vector<pair<int, int> >* usedPipeList;

Event Event::alwaysSet(0, 0);

Event Event::neverSet(WAIT_FOREVER, 0);

QStatus Event::Wait(Event& evt, uint32_t maxWaitMs)
{
    fd_set set;
    fd_set stopSet;
    int maxFd = -1;
    struct timeval tval;
    struct timeval* pTval = NULL;

    Thread* thread = Thread::GetThread();

    FD_ZERO(&set);
    FD_ZERO(&stopSet);

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
            return ER_OK;
        } else if (!pTval || ((evt.timestamp - now) < (uint32_t) (tval.tv_sec * 1000 + tval.tv_usec / 1000))) {
            tval.tv_sec = (evt.timestamp - now) / 1000;
            tval.tv_usec = 1000 * ((evt.timestamp - now) % 1000);
            pTval = &tval;
        }
    } else {
        if (0 <= evt.fd) {
            FD_SET(evt.fd, &set);
            maxFd = max(maxFd, evt.fd);
        }
        if (0 <= evt.ioFd) {
            FD_SET(evt.ioFd, &set);
            maxFd = max(maxFd, evt.ioFd);
        }
    }

    int stopFd = -1;
    if (thread) {
        stopFd = thread->GetStopEvent().fd;
        if (evt.eventType == IO_WRITE) {
            FD_SET(stopFd, &stopSet);
        } else {
            FD_SET(stopFd, &set);
        }
        maxFd = max(maxFd, stopFd);
    }

    evt.IncrementNumThreads();

    int ret = select(maxFd + 1,
                     (evt.eventType == IO_WRITE) ? &stopSet : &set,
                     (evt.eventType == IO_WRITE) ? &set : NULL,
                     NULL,
                     pTval);

    evt.DecrementNumThreads();

    if ((0 <= stopFd) && (FD_ISSET(stopFd, &set) || FD_ISSET(stopFd, &stopSet))) {
        return thread->IsStopping() ? ER_STOPPING_THREAD : ER_ALERTED_THREAD;
    } else if (evt.eventType == TIMED) {
        uint32_t now = GetTimestamp();
        if (now >= evt.timestamp) {
            if (0 < evt.period) {
                evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
            }
            return ER_OK;
        } else {
            return ER_TIMEOUT;
        }
    } else if ((0 < ret) && (((0 <= evt.fd) && FD_ISSET(evt.fd, &set)) || ((0 <= evt.ioFd) && FD_ISSET(evt.ioFd, &set)))) {
        return ER_OK;
    } else if (0 <= ret) {
        return ER_TIMEOUT;
    } else {
        return ER_FAIL;
    }
}

QStatus Event::Wait(const vector<Event*>& checkEvents, vector<Event*>& signaledEvents, uint32_t maxWaitMs)
{
    fd_set rdset;
    fd_set wrset;
    struct timeval tval;
    struct timeval* pTval = NULL;
    bool rdSetEmpty = true;
    bool wrSetEmpty = true;

    if (maxWaitMs != WAIT_FOREVER) {
        tval.tv_sec = maxWaitMs / 1000;
        tval.tv_usec = (maxWaitMs % 1000) * 1000;
        pTval = &tval;
    }

    FD_ZERO(&rdset);
    FD_ZERO(&wrset);
    int maxFd = 0;
    vector<Event*>::const_iterator it;

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->IncrementNumThreads();
        if ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE)) {
            if (0 <= evt->fd) {
                FD_SET(evt->fd, &rdset);
                maxFd = std::max(maxFd, evt->fd);
                rdSetEmpty = false;
            }
            if (0 <= evt->ioFd) {
                FD_SET(evt->ioFd, &rdset);
                maxFd = std::max(maxFd, evt->ioFd);
                rdSetEmpty = false;
            }
        } else if (evt->eventType == IO_WRITE) {
            if (0 <= evt->fd) {
                FD_SET(evt->fd, &wrset);
                wrSetEmpty = false;
                maxFd = std::max(maxFd, evt->fd);
            }
            if (0 <= evt->ioFd) {
                FD_SET(evt->ioFd, &wrset);
                wrSetEmpty = false;
                maxFd = std::max(maxFd, evt->ioFd);
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

    int ret = select(maxFd + 1, rdSetEmpty ? NULL : &rdset, wrSetEmpty ? NULL : &wrset, NULL, pTval);

    if (0 <= ret) {
        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            Event* evt = *it;
            evt->DecrementNumThreads();
            if (!rdSetEmpty && ((evt->eventType == IO_READ) || (evt->eventType == GEN_PURPOSE))) {
                if (((0 <= evt->fd) && FD_ISSET(evt->fd, &rdset)) || ((0 <= evt->ioFd) && FD_ISSET(evt->ioFd, &rdset))) {
                    signaledEvents.push_back(evt);
                }
            } else if (!wrSetEmpty && (evt->eventType == IO_WRITE)) {
                if (((0 <= evt->fd) && FD_ISSET(evt->fd, &wrset)) || ((0 <= evt->ioFd) && FD_ISSET(evt->ioFd, &wrset))) {
                    signaledEvents.push_back(evt);
                }
            } else if (evt->eventType == TIMED) {
                uint32_t now = GetTimestamp();
                if (evt->timestamp <= now) {
                    signaledEvents.push_back(evt);
                    if (0 < evt->period) {
                        evt->timestamp += (((now - evt->timestamp) / evt->period) + 1) * evt->period;
                    }
                }
            }
        }
        return signaledEvents.empty() ? ER_TIMEOUT : ER_OK;
    } else {
        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            (*it)->DecrementNumThreads();
        }
        QCC_LogError(ER_FAIL, ("select failed with %d (%s)", errno, strerror(errno)));
        return ER_FAIL;
    }
}

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

Event::Event(int ioFd, EventType eventType, bool genPurpose)
    : fd(-1), signalFd(-1), ioFd(ioFd), eventType(eventType), timestamp(0), period(0), numThreads(0)
{
    if (genPurpose) {
        createPipe(&fd, &signalFd);
    }
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
