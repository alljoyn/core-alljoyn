/*
 * @file
 *
 * WinRT implementation of event
 */

/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include <qcc/platform.h>

#include <map>
#include <list>
#include <windows.h>
#include <errno.h>
#include <time.h>

#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/winrt/SocketWrapper.h>

using namespace std;
using namespace Windows::Foundation;

/** @internal */
#define QCC_MODULE "EVENT"

namespace qcc {

class IoEventMonitor {
  public:

    struct EventList {
        std::list<Event*> events;
        EventRegistrationToken eventRegToken;
    };

    qcc::Mutex lock;

    /*
     * Mapping from socket handles to Event registrations
     */
    std::map<void*, EventList*> eventMap;

    void IoEventCallback(Platform::Object ^ sender, int events)
    {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>(sender);
        void* sockfd = (void*)socket;

        lock.Lock();

        std::map<void*, EventList*>::iterator iter = eventMap.find(sockfd);
        if (iter != eventMap.end()) {
            IoEventMonitor::EventList* eventList = iter->second;

            std::list<Event*>::iterator evit = eventList->events.begin();
            if (evit == eventList->events.end()) {
                QCC_LogError(ER_OS_ERROR, ("Event list was empty"));
            }

            while (evit != eventList->events.end()) {
                Event* ev = (*evit);
                bool isSet = false;
                if ((events & (int)qcc::winrt::Events::Write) && (ev->GetEventType() == Event::IO_WRITE)) {
                    isSet = true;
                    QCC_DbgHLPrintf(("Setting write event %d", ev->GetHandle()));
                }
                if ((events & (int)qcc::winrt::Events::Read) && (ev->GetEventType() == Event::IO_READ)) {
                    isSet = true;
                    QCC_DbgHLPrintf(("Setting read event %d", ev->GetHandle()));
                }
                if (events & (int)qcc::winrt::Events::Exception) {
                    isSet = true;
                    QCC_DbgHLPrintf(("Setting event %d for exception state", ev->GetHandle()));
                }
                if (isSet) {
                    BOOL ret = ::SetEvent(ev->GetHandle());
                    if (!ret) {
                        QCC_LogError(ER_OS_ERROR, ("SetEvent returned %d", ret));
                    }
                }
                ++evit;
            }
        }

        lock.Unlock();
    }

    void RegisterEvent(Event* event) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)event->GetFD());
        void* sockfd = (void*)socket;

        QCC_DbgHLPrintf(("RegisterEvent %s for fd %d (ioHandle=%d)", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", sockfd, event->GetHandle()));
        assert((event->GetEventType() == Event::IO_READ) || (event->GetEventType() == Event::IO_WRITE));

        lock.Lock();
        std::map<void*, EventList*>::iterator iter = eventMap.find(sockfd);
        EventList* eventList;
        if (iter == eventMap.end()) {
            /* new IO */
            eventList = new EventList();
            eventMap[sockfd] = eventList;

            // AddRef the socket to fixup ref counting in map
            __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(socket);
            pUnk->__abi_AddRef();

            // Register callback
            eventList->eventRegToken = socket->SocketEventsChanged::add(
                ref new qcc::winrt::SocketWrapperEventsChangedHandler([&](Platform::Object ^ source, int events) {
                                                                          IoEventCallback(source, events);
                                                                      }));
        } else {
            eventList = iter->second;
        }
        /*
         * Add the event to the list of events for this socket
         */
        eventList->events.push_back(event);
        lock.Unlock();
    }

    void DeregisterEvent(Event* event) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)event->GetFD());
        void* sockfd = (void*)socket;

        QCC_DbgPrintf(("DeregisterEvent %s for fd %d", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", sockfd));
        assert((event->GetEventType() == Event::IO_READ) || (event->GetEventType() == Event::IO_WRITE));

        lock.Lock();
        std::map<void*, EventList*>::iterator iter = eventMap.find(sockfd);
        if (iter != eventMap.end()) {
            EventList*eventList = iter->second;
            /*
             * Remove this event from the event list
             */
            eventList->events.remove(event);
            /*
             * Remove the eventList and clean up if all the events are gone.
             */
            if (eventList->events.empty()) {
                // Unregister callback
                socket->SocketEventsChanged::remove(eventList->eventRegToken);

                eventMap.erase(iter);

                // Release the socket to fixup ref counting in map
                __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(socket);
                pUnk->__abi_Release();

                delete eventList;
            }
        } else {
            QCC_LogError(ER_OS_ERROR, ("eventList for fd %d missing from event map", event->GetFD()));
        }
        lock.Unlock();
    }
};

static IoEventMonitor IoMonitor;

Event Event::alwaysSet(0, 0);

Event Event::neverSet(WAIT_FOREVER, 0);

QStatus Event::Wait(Event& evt, uint32_t maxWaitMs)
{
    HANDLE handles[3];
    int numHandles = 0;

    /*
     * The IO event is necessarily an auto-reset event. Calling select with a zero timeout to check
     * the I/O status before blocking ensures that Event::Wait is idempotent.
     */
    if ((evt.eventType == IO_READ) || (evt.eventType == IO_WRITE)) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)evt.GetFD());
        int events = socket->GetEvents();
        bool isSet = false;
        if (evt.eventType == IO_READ && (events & (int)qcc::winrt::Events::Read) != 0) {
            isSet = true;
        }
        if (evt.eventType == IO_WRITE && (events & (int)qcc::winrt::Events::Write) != 0) {
            isSet = true;
        }
        if ((events & (int)qcc::winrt::Events::Exception) != 0) {
            isSet = true;
        }
        if (isSet) {
            ::SetEvent(evt.GetHandle());
        }
    }

    Thread* thread = Thread::GetThread();

    Event* stopEvent = &thread->GetStopEvent();
    handles[numHandles++] = stopEvent->handle;

    if (INVALID_HANDLE_VALUE != evt.handle) {
        handles[numHandles++] = evt.handle;
    }
    if (INVALID_HANDLE_VALUE != evt.ioHandle) {
        handles[numHandles++] = evt.ioHandle;
    }
    if (TIMED == evt.eventType) {
        uint32_t now = GetTimestamp();
        if (evt.timestamp <= now) {
            if (0 < evt.period) {
                evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
            }
            return ER_OK;
        } else if (WAIT_FOREVER == maxWaitMs || ((evt.timestamp - now) < maxWaitMs)) {
            maxWaitMs = evt.timestamp - now;
        }
    }

    QStatus status = ER_OK;

    evt.IncrementNumThreads();
    DWORD ret = WaitForMultipleObjectsEx(numHandles, handles, FALSE, maxWaitMs, FALSE);
    evt.DecrementNumThreads();

    if ((ret >= WAIT_OBJECT_0) && (ret < (WAIT_OBJECT_0 + numHandles))) {
        if (handles[ret - WAIT_OBJECT_0] == stopEvent->handle) {
            status = (thread && thread->IsStopping()) ? ER_STOPPING_THREAD : ER_ALERTED_THREAD;
        } else {
            status = ER_OK;
        }
    } else if (WAIT_TIMEOUT == ret) {
        if (TIMED == evt.eventType) {
            uint32_t now = GetTimestamp();
            if (now >= evt.timestamp) {
                if (0 < evt.period) {
                    evt.timestamp += (((now - evt.timestamp) / evt.period) + 1) * evt.period;
                }
            } else {
                status = ER_TIMEOUT;
            }
        } else {
            QCC_DbgPrintf(("WaitForMultipleObjectsEx timeout %d", maxWaitMs));
            status = ER_TIMEOUT;
        }
    } else {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("WaitForMultipleObjectsEx returned 0x%x.", ret));
        if (WAIT_FAILED == ret) {
            QCC_LogError(status, ("GetLastError=%d", ::GetLastError()));
            QCC_LogError(status, ("numHandles=%d, maxWaitMs=%d, Handles: ", numHandles, maxWaitMs));
            for (int i = 0; i < numHandles; ++i) {
                QCC_LogError(status, ("  0x%x", handles[i]));
            }
        }
    }
    return status;
}


QStatus Event::Wait(const vector<Event*>& checkEvents, vector<Event*>& signaledEvents, uint32_t maxWaitMs)
{
    const int MAX_HANDLES = 64;

    int numHandles = 0;
    HANDLE handles[MAX_HANDLES];
    vector<Event*>::const_iterator it;

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->IncrementNumThreads();
        if (INVALID_HANDLE_VALUE != evt->handle) {
            handles[numHandles++] = evt->handle;
            if (MAX_HANDLES <= numHandles) {
                break;
            }
        }
        if (INVALID_HANDLE_VALUE != evt->ioHandle) {
            handles[numHandles++] = evt->ioHandle;
            if (MAX_HANDLES <= numHandles) {
                break;
            }
        }
        if (evt->eventType == TIMED) {
            uint32_t now = GetTimestamp();
            if (evt->timestamp <= now) {
                maxWaitMs = 0;
            } else if ((WAIT_FOREVER == maxWaitMs) || ((evt->timestamp - now) < maxWaitMs)) {
                maxWaitMs = evt->timestamp - now;
            }
        }

        if ((evt->eventType == IO_READ) || (evt->eventType == IO_WRITE)) {
            qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)evt->GetFD());
            int events = socket->GetEvents();
            bool isSet = false;
            if (evt->eventType == IO_READ && (events & (int)qcc::winrt::Events::Read) != 0) {
                isSet = true;
            }
            if (evt->eventType == IO_WRITE && (events & (int)qcc::winrt::Events::Write) != 0) {
                isSet = true;
            }
            if ((events & (int)qcc::winrt::Events::Exception) != 0) {
                isSet = true;
            }
            if (isSet) {
                ::SetEvent(evt->GetHandle());
            }
        }
    }
    /* Restore thread counts if we are not going to block */
    if (numHandles >= MAX_HANDLES) {
        while (true) {
            Event* evt = *it;
            evt->DecrementNumThreads();
            if (it == checkEvents.begin()) {
                break;
            }
            --it;
        }
        QCC_LogError(ER_FAIL, ("Event::Wait: Maximum number of HANDLES reached"));
        return ER_FAIL;
    }

    DWORD ret = WaitForMultipleObjectsEx(numHandles, handles, FALSE, maxWaitMs, FALSE);

    bool somethingSet = (ret >= WAIT_OBJECT_0) && (ret < (WAIT_OBJECT_0 + numHandles));

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->DecrementNumThreads();
        if (TIMED == evt->eventType) {
            uint32_t now = GetTimestamp();
            if (now >= evt->timestamp) {
                if (0 < evt->period) {
                    evt->timestamp += (((now - evt->timestamp) / evt->period) + 1) * evt->period;
                }
                signaledEvents.push_back(evt);
            }
        } else {
            if (somethingSet && evt->IsSet()) {
                signaledEvents.push_back(evt);
            }
        }
    }

    QStatus status = ER_OK;
    if (somethingSet || (WAIT_TIMEOUT == ret)) {
        status = signaledEvents.empty() ? ER_TIMEOUT : ER_OK;
    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("WaitForMultipleObjectsEx(2) returned 0x%x.", ret));
        if (WAIT_FAILED == ret) {
            QCC_LogError(status, ("GetLastError=%d", ::GetLastError()));
            QCC_LogError(status, ("numHandles=%d, maxWaitMs=%d, Handles: ", numHandles, maxWaitMs));
            for (int i = 0; i < numHandles; ++i) {
                QCC_LogError(status, ("  0x%x", handles[i]));
            }
        }
    }
    return status;
}

Event::Event() :
    handle(CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS)),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(GEN_PURPOSE),
    timestamp(0),
    period(0),
    ioFd(-1),
    numThreads(0)
{
}

Event::Event(Event& event, EventType eventType, bool genPurpose) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(eventType),
    timestamp(0),
    period(0),
    ioFd(event.ioFd),
    numThreads(0)
{
    /* Create an auto reset event for the socket fd */
    if (ioFd > 0) {
        assert((eventType == IO_READ) || (eventType == IO_WRITE));
        ioHandle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
        IoMonitor.RegisterEvent(this);
    }
    if (genPurpose) {
        handle = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    }
}

Event::Event(qcc::SocketFd ioFd, EventType eventType, bool genPurpose) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(eventType),
    timestamp(0),
    period(0),
    ioFd(ioFd),
    numThreads(0)
{
    /* Create an auto reset event for the socket fd */
    if (ioFd > 0) {
        assert((eventType == IO_READ) || (eventType == IO_WRITE));
        ioHandle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
        IoMonitor.RegisterEvent(this);
    }
    if (genPurpose) {
        handle = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    }
}

Event::Event(uint32_t timestamp, uint32_t period) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(TIMED),
    timestamp(WAIT_FOREVER == timestamp ? WAIT_FOREVER : GetTimestamp() + timestamp),
    period(period),
    ioFd(-1),
    numThreads(0)
{
}

Event::~Event()
{
    Close();
}

void Event::Close()
{
    /* Threads should not be waiting on this event */
    if ((handle != INVALID_HANDLE_VALUE) && !::SetEvent(handle)) {
        QCC_LogError(ER_FAIL, ("SetEvent failed with %d", ::GetLastError()));
    }
    if (TIMED == eventType) {
        timestamp = 0;
    }

    /* No longer monitoring I/O for this event */
    if (ioHandle != INVALID_HANDLE_VALUE) {
        IoMonitor.DeregisterEvent(this);
        CloseHandle(ioHandle);
        ioHandle = INVALID_HANDLE_VALUE;
    }

    /* Close the handles */
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

QStatus Event::SetEvent()
{
    QStatus status = ER_OK;

    if (INVALID_HANDLE_VALUE != handle) {
        if (!::SetEvent(handle)) {
            status = ER_FAIL;
            QCC_LogError(status, ("SetEvent failed with %d", ::GetLastError()));
        }
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
        /* Not a general purpose or timed event */
        status = ER_FAIL;
        QCC_LogError(status, ("Attempt to manually set an I/O event"));
    }
    return status;
}

QStatus Event::ResetEvent()
{
    QStatus status = ER_OK;

    if (INVALID_HANDLE_VALUE != handle) {
        if (!::ResetEvent(handle)) {
            status = ER_FAIL;
            QCC_LogError(status, ("ResetEvent failed with %d", ::GetLastError()));
        }
    } else if (TIMED == eventType) {
        if (0 < period) {
            uint32_t now = GetTimestamp();
            timestamp += (((now - timestamp) / period) + 1) * period;
        } else {
            timestamp = static_cast<uint32_t>(-1);
        }
    } else {
        /* Not a general purpose or timed event */
        status = ER_FAIL;
        QCC_LogError(status, ("Attempt to manually reset an I/O event"));
    }
    return status;
}

bool Event::IsSet()
{
    return (ER_TIMEOUT == Wait(*this, 0)) ? false : true;
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

}  /* namespace */

