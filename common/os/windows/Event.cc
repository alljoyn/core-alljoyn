/*
 * @file
 *
 * Windows implementation of event.
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
#include <qcc/platform.h>

#include <map>
#include <list>
#include <winsock2.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <windows.h>
#include <errno.h>
#include <time.h>

#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#if (_WIN32_WINNT > 0x0603)
#include <MSAJTransport.h>
#endif

using namespace std;

/** @internal */
#define QCC_MODULE "EVENT"

namespace qcc {

static const long READ_SET = FD_READ | FD_CLOSE | FD_ACCEPT;
static const long WRITE_SET = FD_WRITE | FD_CLOSE | FD_CONNECT;

#if (_WIN32_WINNT > 0x0603)
/** Read set and Write set for Named Pipe IO events */
static const long NP_READ_SET = ALLJOYN_READ_READY | ALLJOYN_DISCONNECTED;
static const long NP_WRITE_SET = ALLJOYN_WRITE_READY | ALLJOYN_DISCONNECTED;
VOID CALLBACK NamedPipeIoEventCallback(PVOID arg, BOOLEAN TimerOrWaitFired);
#endif

VOID WINAPI IpInterfaceChangeCallback(PVOID arg, PMIB_IPINTERFACE_ROW row, MIB_NOTIFICATION_TYPE notificationType);

VOID CALLBACK IoEventCallback(PVOID arg, BOOLEAN TimerOrWaitFired);


void WINAPI IpInterfaceChangeCallback(PVOID arg, PMIB_IPINTERFACE_ROW row, MIB_NOTIFICATION_TYPE notificationType)
{
    UNREFERENCED_PARAMETER(row);
    UNREFERENCED_PARAMETER(notificationType);

    Event* event = (Event*) arg;
    QCC_DbgHLPrintf(("Received network interface event type %u", notificationType));
    if (!::SetEvent(event->GetHandle())) {
        QCC_LogError(ER_OS_ERROR, ("Setting network interface event failed"));
    }
}

/* This class allows thread to wait more than 64 handles at a time. */
class SuperWaiter {
  public:
    SuperWaiter() :
        m_shutdown(INVALID_HANDLE_VALUE),
        m_cleanupGroup(nullptr),
        m_indexSignaled(-1)
    {
    }

    ~SuperWaiter()
    {
        /* This blocks until all threads exit */
        CloseThreadpoolCleanupGroupMembers(m_cleanupGroup, FALSE, nullptr);
        CloseThreadpoolCleanupGroup(m_cleanupGroup);
        m_cleanupGroup = nullptr;

        DestroyThreadpoolEnvironment(&m_environment);

        if (m_shutdown) {
            CloseHandle(m_shutdown);
        }
    }

    QStatus Initialize()
    {
        HANDLE shutdown = CreateEvent(nullptr, true, false, nullptr);
        if (NULL == shutdown) {
            QCC_LogError(ER_OUT_OF_MEMORY, ("CreateEvent failed."));
            return ER_OUT_OF_MEMORY;
        }

        PTP_CLEANUP_GROUP cleanupGroup = CreateThreadpoolCleanupGroup();
        if (nullptr == cleanupGroup) {
            QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolCleanupGroup failed with error %u.", GetLastError()));
            CloseHandle(shutdown);
            return ER_OS_ERROR;
        }

        m_shutdown = shutdown;
        m_cleanupGroup = cleanupGroup;

        InitializeThreadpoolEnvironment(&m_environment);
        SetThreadpoolCallbackCleanupGroup(&m_environment, m_cleanupGroup, nullptr);
        return ER_OK;
    }

    void Shutdown()
    {
        SetEvent(m_shutdown);
    }

    QStatus WaitForMultipleObjects(_In_ size_t totalHandles, _In_reads_(totalHandles) const HANDLE* handles, _In_ size_t timeoutMsec, _Out_ int32_t* indexSignaled)
    {
        assert(0 != totalHandles);
        assert(nullptr != handles);
        assert(nullptr != indexSignaled);
        assert(nullptr != m_cleanupGroup);
        assert(INVALID_HANDLE_VALUE != m_shutdown);

        *indexSignaled = -1;

        for (size_t i = 0; i < totalHandles; ++i) {
            /* Subtract 1 from MAXIMUM_WAIT_OBJECTS to accomodate the shutdown event */
            size_t numHandles = totalHandles - i;
            if (MAXIMUM_WAIT_OBJECTS - 1 < numHandles) {
                numHandles = MAXIMUM_WAIT_OBJECTS - 1;
            }

            if (i == 0) {
                /* Only the first group gets the true timeout to avoid unnecessary race */
            } else {
                timeoutMsec = INFINITE;
            }

            WaitGroup* group = new WaitGroup(this, &handles[i], numHandles, i, timeoutMsec);
            i += numHandles;

            PTP_WORK work = CreateThreadpoolWork(s_WaitThread, group, &m_environment);
            if (nullptr == work) {
                QCC_LogError(ER_OS_ERROR, ("CreateThreadpoolCleanupGroup failed"));
                Shutdown();
                delete group;
                return ER_OS_ERROR;
            }

            /* Threadpool work is now responsible for deleting the group object */
            SubmitThreadpoolWork(work);
        }

        WaitForSingleObject(m_shutdown, INFINITE);

        LONG result = InterlockedCompareExchange(&m_indexSignaled, -1, -1);
        if (result < 0) {
            return ER_TIMEOUT;
        }

        *indexSignaled = static_cast<int32_t>(result);
        return ER_OK;
    }

  private:
    HANDLE m_shutdown;
    TP_CALLBACK_ENVIRON m_environment;
    PTP_CLEANUP_GROUP m_cleanupGroup;
    volatile LONG m_indexSignaled;

    struct WaitGroup {
        WaitGroup(_In_ SuperWaiter* waiter, _In_reads_(numHandles) const HANDLE* handles, _In_ size_t numHandles, _In_ size_t groupIndex, _In_ size_t timeoutMsec) :
            m_waiter(waiter),
            m_handles(handles),
            m_numHandles(numHandles),
            m_groupIndex(groupIndex),
            m_timeoutMsec(timeoutMsec)
        {
            /* The number of handles must be MAXIMUM_WAIT_OBJECTS - 1 or fewer per group */
            assert(numHandles < MAXIMUM_WAIT_OBJECTS);
        }

        SuperWaiter* m_waiter;
        const HANDLE* m_handles;
        const size_t m_numHandles;
        const size_t m_groupIndex;
        const size_t m_timeoutMsec;
      private:
        WaitGroup operator=(const WaitGroup&) { return *this; };
    };

    static VOID CALLBACK s_WaitThread(_Inout_ PTP_CALLBACK_INSTANCE instance, _Inout_opt_ PVOID context, _Inout_ PTP_WORK work)
    {
        UNREFERENCED_PARAMETER(work);

        WaitGroup* group = reinterpret_cast<WaitGroup*>(context);

        /* Make sure we have another thread available for the next wait group */
        CallbackMayRunLong(instance);

        size_t numHandles = group->m_numHandles + 1;
        HANDLE* handles = new HANDLE[numHandles];
        handles[0] = group->m_waiter->m_shutdown;
        if (memcpy_s(&handles[1], (sizeof(HANDLE) * group->m_numHandles), group->m_handles, (sizeof(HANDLE) * group->m_numHandles))) {
            assert(!"memcpy_s failed");
            delete [] handles;
            delete group;
            return;
        }

        DWORD waitResult = WaitForMultipleObjectsEx(numHandles, handles, false, group->m_timeoutMsec, false);
        delete[] handles;

        switch (waitResult) {
        case WAIT_FAILED:
        case WAIT_ABANDONED:
            assert(false);

        /* Fallthrough */
        case WAIT_TIMEOUT:
            group->m_waiter->Shutdown();
            break;

        case WAIT_OBJECT_0:
            /* Shutdown requested, nothing report */
            break;

        default:
            {
                /* Calculate the true wait result, subtract 1 because the first index is for shutdown purpose */
                LONG indexSignaled = group->m_groupIndex + ((waitResult - WAIT_OBJECT_0) - 1);
                InterlockedExchange(&(group->m_waiter->m_indexSignaled), indexSignaled);
                group->m_waiter->Shutdown();
            }
            break;
        }

        delete group;
    }
};

class IoEventMonitor {
  public:

    struct EventList {
        std::list<Event*> events;
        uint32_t fdSet;
        HANDLE ioEvent;
        HANDLE waitObject;
    };

    qcc::Mutex lock;

    /*
     * Mapping from socket handles to Event registrations
     */
    std::map<SocketFd, EventList*> eventMap;

#if (_WIN32_WINNT > 0x0603)
    /*
     * Mapping from pipe handles to Event registrations
     */
    std::map<HANDLE, EventList*> namedPipeEventMap;

#endif

    void RegisterEvent(Event* event)
    {
        if (event->IsSocket()) {
            RegisterSocketEvent(event);
        } else {
            RegisterNamedPipeEvent(event);
        }
    }

    void DeregisterEvent(Event* event)
    {
        if (event->IsSocket()) {
            DeregisterSocketEvent(event);
        } else {
            DeregisterNamedPipeEvent(event);
        }
    }

    void RegisterSocketEvent(Event* event)
    {
        SocketFd sock = event->GetFD();
        QCC_DbgHLPrintf(("RegisterEvent %s for fd %d (ioHandle=%d)", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", sock, event->GetHandle()));
        assert((event->GetEventType() == Event::IO_READ) || (event->GetEventType() == Event::IO_WRITE));

        lock.Lock();
        std::map<SocketFd, EventList*>::iterator iter = eventMap.find(sock);
        EventList*eventList;
        if (iter == eventMap.end()) {
            /* new IO */
            eventList = new EventList();
            eventList->ioEvent = WSACreateEvent();
            eventList->fdSet = 0;
            eventMap[sock] = eventList;
            if (RegisterWaitForSingleObject(&eventList->waitObject, eventList->ioEvent, IoEventCallback, (void*)sock, INFINITE, WT_EXECUTEINWAITTHREAD)) {
                QCC_DbgHLPrintf(("RegisterWaitForSingleObject %d", eventList->waitObject));
            } else {
                QCC_LogError(ER_OS_ERROR, ("RegisterWaitForSingleObject failed"));
            }
        } else {
            eventList = iter->second;
        }
        /*
         * Add the event to the list of events for this socket
         */
        eventList->events.push_back(event);
        /*
         * Check if the set of IO conditions being monitored has changed.
         */
        uint32_t fdSet = eventList->fdSet | ((event->GetEventType() == Event::IO_READ) ? READ_SET : WRITE_SET);
        if (eventList->fdSet != fdSet) {
            eventList->fdSet = fdSet;
            QCC_DbgHLPrintf(("WSAEventSelect %x", fdSet));
            WSAEventSelect(sock, eventList->ioEvent, fdSet);
        }
        lock.Unlock();
    }


    void DeregisterSocketEvent(Event* event)
    {
        SocketFd sock = event->GetFD();

        QCC_DbgPrintf(("DeregisterEvent %s for fd %d", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", sock));
        assert((event->GetEventType() == Event::IO_READ) || (event->GetEventType() == Event::IO_WRITE));

        lock.Lock();
        std::map<SocketFd, EventList*>::iterator iter = eventMap.find(sock);
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
                eventMap.erase(iter);
                WSAEventSelect(event->GetFD(), eventList->ioEvent, 0);
                /*
                 * Make sure event is not in a set state
                 */
                WSAResetEvent(eventList->ioEvent);
                /*
                 * Cannot be holding the lock while unregistering the wait because this can cause a
                 * deadlock if the wait callback is blocked waiting for the lock.
                 */
                QCC_DbgHLPrintf(("UnregisterWait %d", eventList->waitObject));
                lock.Unlock();
                UnregisterWait(eventList->waitObject);
                lock.Lock();
                WSACloseEvent(eventList->ioEvent);
                delete eventList;
            }
        } else {
            QCC_LogError(ER_OS_ERROR, ("eventList for fd %d missing from event map", event->GetFD()));
        }
        lock.Unlock();
    }


    void RegisterNamedPipeEvent(Event* event)
    {
#if (_WIN32_WINNT > 0x0603)
        HANDLE pipe = LongToHandle((ULONG)event->GetFD());
        QCC_DbgHLPrintf(("RegisterEvent %s for fd %d (ioHandle=%p)", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", pipe, event->GetHandle()));
        assert((event->GetEventType() == Event::IO_READ) || (event->GetEventType() == Event::IO_WRITE));

        lock.Lock();
        std::map<HANDLE, EventList*>::iterator iter = namedPipeEventMap.find(pipe);
        EventList*eventList;
        if (iter == namedPipeEventMap.end()) {
            /* new IO */
            eventList = new EventList();
            eventList->ioEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            eventList->fdSet = 0;
            namedPipeEventMap[pipe] = eventList;
            if (RegisterWaitForSingleObject(&eventList->waitObject, eventList->ioEvent, NamedPipeIoEventCallback, (void*)pipe, INFINITE, WT_EXECUTEINWAITTHREAD)) {
                QCC_DbgHLPrintf(("RegisterWaitForSingleObject %d", eventList->waitObject));
            } else {
                QCC_LogError(ER_OS_ERROR, ("RegisterWaitForSingleObject failed"));
            }
        } else {
            eventList = iter->second;
        }
        /*
         * Add the event to the list of events for this end of pipe
         */
        eventList->events.push_back(event);
        /*
         * Check if the set of IO conditions being monitored have changed.
         */
        uint32_t fdSet = eventList->fdSet | ((event->GetEventType() == Event::IO_READ) ? NP_READ_SET : NP_WRITE_SET);
        if (eventList->fdSet != fdSet) {
            eventList->fdSet = fdSet;
            QCC_DbgHLPrintf(("NamedPipeEventSelect %x", fdSet));
            AllJoynEventSelect(pipe, eventList->ioEvent, fdSet);
        }
        lock.Unlock();
#else
        UNREFERENCED_PARAMETER(event);
#endif
    }

    void DeregisterNamedPipeEvent(Event* event)
    {
#if (_WIN32_WINNT > 0x0603)
        HANDLE pipe = LongToHandle((ULONG)event->GetFD());
        QCC_DbgPrintf(("DeregisterEvent %s for pipe %p", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", pipe));
        assert((event->GetEventType() == Event::IO_READ) || (event->GetEventType() == Event::IO_WRITE));

        lock.Lock();
        std::map<HANDLE, EventList*>::iterator iter = namedPipeEventMap.find(pipe);
        if (iter != namedPipeEventMap.end()) {
            EventList*eventList = iter->second;
            /*
             * Remove this event from the event list
             */
            eventList->events.remove(event);
            /*
             * Remove the eventList and clean up if all the events are gone.
             */
            if (eventList->events.empty()) {
                namedPipeEventMap.erase(iter);
                AllJoynEventSelect(pipe, eventList->ioEvent, 0);
                /*
                 * Make sure event is not in a set state
                 */
                ::ResetEvent(eventList->ioEvent);
                /*
                 * Cannot be holding the lock while unregistering the wait because this can cause a
                 * deadlock if the wait callback is blocked waiting for the lock.
                 */
                QCC_DbgHLPrintf(("UnregisterWait %d", eventList->waitObject));
                lock.Unlock();
                UnregisterWait(eventList->waitObject);
                lock.Lock();
                ::CloseHandle(eventList->ioEvent);
                delete eventList;
            }
        } else {
            QCC_LogError(ER_OS_ERROR, ("eventList for fd %d missing from event map", event->GetFD()));
        }
        lock.Unlock();
#else
        UNREFERENCED_PARAMETER(event);
#endif
    }

};

static uint64_t _alwaysSet[RequiredArrayLength(sizeof(Event), uint64_t)];
static uint64_t _neverSet[RequiredArrayLength(sizeof(Event), uint64_t)];
static IoEventMonitor* IoMonitor = NULL;
static bool initialized = false;

Event& Event::alwaysSet = (Event&)_alwaysSet;
Event& Event::neverSet = (Event&)_neverSet;

void Event::Init()
{
    if (!initialized) {
        new (&alwaysSet)Event(0, 0);
        new (&neverSet)Event(Event::WAIT_FOREVER, 0);
        IoMonitor = new IoEventMonitor;
        initialized = true;
    }
}

void Event::Shutdown()
{
    if (initialized) {
        delete IoMonitor;
        IoMonitor = NULL;
        neverSet.~Event();
        alwaysSet.~Event();
        initialized = false;
    }
}

VOID CALLBACK IoEventCallback(PVOID arg, BOOLEAN TimerOrWaitFired)
{
    UNREFERENCED_PARAMETER(TimerOrWaitFired);

    SocketFd sock = (SocketFd)arg;
    IoMonitor->lock.Lock();
    std::map<SocketFd, IoEventMonitor::EventList*>::iterator iter = IoMonitor->eventMap.find(sock);
    if (iter != IoMonitor->eventMap.end()) {
        IoEventMonitor::EventList*eventList = iter->second;
        WSANETWORKEVENTS ioEvents;
        int ret = WSAEnumNetworkEvents(sock, eventList->ioEvent, &ioEvents);
        if (ret != 0) {
            QCC_LogError(ER_OS_ERROR, ("WSAEnumNetworkEvents returned %d", ret));
        } else {
            QCC_DbgHLPrintf(("IoEventCallback %x", ioEvents.lNetworkEvents));
            if (ioEvents.lNetworkEvents) {
                std::list<Event*>::iterator evit = eventList->events.begin();
                if (evit == eventList->events.end()) {
                    QCC_LogError(ER_OS_ERROR, ("Event list was empty"));
                }
                while (evit != eventList->events.end()) {
                    Event* ev = (*evit);
                    bool isSet = false;

                    if ((ioEvents.lNetworkEvents & WRITE_SET) && (ev->GetEventType() == Event::IO_WRITE)) {
                        isSet = true;
                        QCC_DbgHLPrintf(("Setting write event %d", ev->GetHandle()));
                    }
                    if ((ioEvents.lNetworkEvents & READ_SET) && (ev->GetEventType() == Event::IO_READ)) {
                        isSet = true;
                        QCC_DbgHLPrintf(("Setting read event %d", ev->GetHandle()));
                    }
                    if (isSet) {
                        if (!::SetEvent(ev->GetHandle())) {
                            QCC_LogError(ER_OS_ERROR, ("SetEvent returned %d", ret));
                        }
                    }
                    ++evit;
                }
            }
        }
    }
    IoMonitor->lock.Unlock();
}

#if (_WIN32_WINNT > 0x0603)
VOID CALLBACK NamedPipeIoEventCallback(PVOID arg, BOOLEAN TimerOrWaitFired)
{
    HANDLE pipe = (HANDLE)arg;
    BOOL ret;
    DWORD eventMask;
    IoMonitor->lock.Lock();
    std::map<HANDLE, IoEventMonitor::EventList*>::iterator iter = IoMonitor->namedPipeEventMap.find(pipe);
    if (iter != IoMonitor->namedPipeEventMap.end()) {
        IoEventMonitor::EventList*eventList = iter->second;

        ret = AllJoynEnumEvents(pipe, eventList->ioEvent, &eventMask);
        if (ret != TRUE) {
            QCC_LogError(ER_OS_ERROR, ("NamedPipeEventEnum returned %d, GLE = %u", ret, ::GetLastError()));
        } else {
            QCC_DbgHLPrintf(("NamedPipeIoEventCallback %x", eventMask));
            if (eventMask) {
                std::list<Event*>::iterator evit = eventList->events.begin();
                if (evit == eventList->events.end()) {
                    QCC_LogError(ER_OS_ERROR, ("Event list was empty"));
                }
                while (evit != eventList->events.end()) {
                    Event* ev = (*evit);
                    bool isSet = false;

                    if ((eventMask & NP_WRITE_SET) && (ev->GetEventType() == Event::IO_WRITE)) {
                        isSet = true;
                        QCC_DbgHLPrintf(("Setting write event %d", ev->GetHandle()));
                    }
                    if ((eventMask & NP_READ_SET) && (ev->GetEventType() == Event::IO_READ)) {
                        isSet = true;
                        QCC_DbgHLPrintf(("Setting read event %d", ev->GetHandle()));
                    }
                    if (isSet) {
                        if (!::SetEvent(ev->GetHandle())) {
                            QCC_LogError(ER_OS_ERROR, ("SetEvent returned %d", ret));
                        }
                    }
                    ++evit;
                }
            }
        }
    }
    IoMonitor->lock.Unlock();
}
#endif

QStatus Event::Wait(Event& evt, uint32_t maxWaitMs)
{
    HANDLE handles[3];
    int numHandles = 0;

    /*
     * The IO event is necessarily an auto-reset event. Calling select with a zero timeout to check
     * the I/O status before blocking ensures that Event::Wait is idempotent.
     */
    if ((evt.eventType == IO_READ) || (evt.eventType == IO_WRITE)) {
        if (evt.IsNetworkEventSet()) {
            ::SetEvent(evt.ioHandle);
        }
    }

    /*
     * The order of handle being added here is important because we want to prioritize the I/O handle.
     * In the event that multiple handles get set during the wait, we would get the prioritized one.
     * For example, if both the IO and stopEvent handles are set, this function would return ER_OK
     * instead of ER_ALERTED_THREAD.
     */
    if (INVALID_HANDLE_VALUE != evt.ioHandle) {
        handles[numHandles++] = evt.ioHandle;
    }
    if (INVALID_HANDLE_VALUE != evt.handle) {
        handles[numHandles++] = evt.handle;
    }

    Thread* thread = Thread::GetThread();
    Event* stopEvent = &thread->GetStopEvent();
    handles[numHandles++] = stopEvent->handle;

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
        if (thread && thread->IsStopping()) {
            /* If there's a stop during the wait, prioritize the return value and ignore what was signaled */
            status = ER_STOPPING_THREAD;
        } else {
            if (handles[ret - WAIT_OBJECT_0] == stopEvent->handle) {
                /* The wait returned due to stopEvent */
                status = ER_ALERTED_THREAD;
            } else {
                /* The wait returned due to the source event */
                status = ER_OK;
            }
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
            QCC_LogError(status, ("GetLastError=%d", GetLastError()));
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
    QStatus status = ER_OK;
    int numHandles = 0;
    vector<HANDLE> handles;
    vector<Event*>::const_iterator it;

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->IncrementNumThreads();
        if (INVALID_HANDLE_VALUE != evt->handle) {
            handles.push_back(evt->handle);
            numHandles++;
        }
        if (INVALID_HANDLE_VALUE != evt->ioHandle) {
            handles.push_back(evt->ioHandle);
            numHandles++;
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
            if (evt->IsNetworkEventSet()) {
                ::SetEvent(evt->ioHandle);
            } else {
                /*
                 * FD_READ network event is level-triggered but not edge-triggered, This means that if the relevant network condition (data is available)
                 * is still valid after the application calls recv(), the network event is recorded and the associated event object is set.
                 * This expects the application routine to wait for the event object before it calls recv(). Unfortunately AllJoyn does not
                 * behave in that way: it may try to pull data one byte at a time; it also does not wait on the event object until recv()
                 * returns ER_WOULDBLOCK. This causes the problem that sometimes the event handle is signaled, but in fact the socket file
                 * descriptor is not in signaled state (FD_READ).
                 */
                ::ResetEvent(evt->ioHandle);
            }
        }
    }


    bool somethingSet = true;
    bool timedOut = false;

    while (signaledEvents.empty() && somethingSet) {
        uint32_t orig = GetTimestamp();

        if (numHandles <= MAXIMUM_WAIT_OBJECTS) {
            assert(numHandles > 0);
            DWORD ret = WaitForMultipleObjectsEx(numHandles, handles.data(), FALSE, maxWaitMs, FALSE);

            /*
             * somethingSet will be true if the return value is between WAIT_OBJECT_0 and WAIT_OBJECT_0 + numHandles.
             * i.e. ret indicates that one of the handles passed in the array "handles" to WaitForMultipleObjectsEx
             * caused the function to return.
             */
            somethingSet = (ret >= WAIT_OBJECT_0) && (ret < (WAIT_OBJECT_0 + numHandles));
            timedOut = (WAIT_TIMEOUT == ret);

            if (WAIT_FAILED == ret) {
                QCC_LogError(ER_FAIL, ("WaitForMultipleObjectsEx(2) returned 0x%x, error=%d.", ret, GetLastError()));
            } else if (WAIT_ABANDONED == ret) {
                QCC_LogError(ER_FAIL, ("WaitForMultipleObjectsEx(2) returned WAIT_ABANDONED."));
            }
        } else {
            int32_t indexSignaled = -1;
            SuperWaiter waiter;
            status = waiter.Initialize();
            if (ER_OK == status) {
                status = waiter.WaitForMultipleObjects(numHandles, handles.data(), maxWaitMs, &indexSignaled);
                if (ER_OK == status) {
                    somethingSet = true;
                    timedOut = false;
                } else if (ER_TIMEOUT == status) {
                    somethingSet = false;
                    timedOut = true;
                }
            }
            if ((ER_OK != status) && (!timedOut)) {
                /* Restore thread counts if we did not block */
                for (;;) {
                    Event* evt = *it;
                    evt->DecrementNumThreads();
                    if (it == checkEvents.begin()) {
                        break;
                    }
                    --it;
                }
                QCC_LogError(status, ("SuperWaiter::WaitForMultipleObjects: failed."));
                return ER_FAIL;
            }
        }

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

        uint32_t now = GetTimestamp();
        /* Adjust the maxWaitMs by the time elapsed, in case we loop back up and call WaitForMultipleObjectsEx again. */
        maxWaitMs -= now - orig;
        /* If somethingSet is true, signaledEvents must not be empty here. But there are situations when WaitForMultipleObjectsEx
           can return even though the event is not set. So, if somethingSet is true, but signaledEvents is empty,
           we call WaitForMultipleObjectsEx again.
         */
    }

    if (somethingSet || timedOut) {
        status = signaledEvents.empty() ? ER_TIMEOUT : ER_OK;
    }

    /* Log everything but OK or TIME_OUT before returning a generic error */
    if ((ER_OK != status) && (ER_TIMEOUT != status)) {
        QCC_LogError(status, ("WaitForMultipleObjects(2) failed, numHandles=%d, maxWaitMs=%d", numHandles, maxWaitMs));
        QCC_DbgHLPrintf(("WaitForMultipleObjects(2) failed, Handles: "));
        for (int i = 0; i < numHandles; ++i) {
            QCC_DbgHLPrintf(("  0x%x", handles[i]));
        }
        status = ER_FAIL;
    }
    return status;
}

Event::Event() :
    handle(CreateEvent(NULL, TRUE, FALSE, NULL)),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(GEN_PURPOSE),
    timestamp(0),
    period(0),
    ioFd(INVALID_SOCKET_FD),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(INVALID_HANDLE_VALUE),
    isSocket(false)
{
}

Event::Event(bool networkIfaceEvent) :
    handle(CreateEvent(NULL, TRUE, FALSE, NULL)),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(GEN_PURPOSE),
    timestamp(0),
    period(0),
    ioFd(INVALID_SOCKET_FD),
    numThreads(0),
    networkIfaceEvent(networkIfaceEvent),
    networkIfaceHandle(INVALID_HANDLE_VALUE),
    isSocket(false)
{
    if (networkIfaceEvent) {
        NotifyIpInterfaceChange(AF_UNSPEC, (PIPINTERFACE_CHANGE_CALLBACK)IpInterfaceChangeCallback, this, false, &networkIfaceHandle);
    }
}

Event::Event(Event& event, EventType eventType, bool genPurpose) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(eventType),
    timestamp(0),
    period(0),
    ioFd(event.ioFd),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(INVALID_HANDLE_VALUE),
    isSocket(event.isSocket)
{
    /* Create an auto reset event for the socket fd */
    if (ioFd != INVALID_SOCKET_FD) {
        assert((eventType == IO_READ) || (eventType == IO_WRITE));
        ioHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
        IoMonitor->RegisterEvent(this);
    }
    if (genPurpose) {
        handle = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
}

Event::Event(SocketFd ioFd, EventType eventType) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(eventType),
    timestamp(0),
    period(0),
    ioFd(ioFd),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(INVALID_HANDLE_VALUE),
    isSocket(true)
{
    /* Create an auto reset event for the socket fd */
    if (ioFd != INVALID_SOCKET_FD) {
        assert((eventType == IO_READ) || (eventType == IO_WRITE));
        ioHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
        IoMonitor->RegisterEvent(this);
    }
}

Event::Event(uint32_t timestamp, uint32_t period) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(TIMED),
    timestamp(WAIT_FOREVER == timestamp ? WAIT_FOREVER : GetTimestamp() + timestamp),
    period(period),
    ioFd(INVALID_SOCKET_FD),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(INVALID_HANDLE_VALUE),
    isSocket(false)
{
}

Event::Event(HANDLE busHandle, EventType eventType) :
    handle(INVALID_HANDLE_VALUE),
    ioHandle(INVALID_HANDLE_VALUE),
    eventType(eventType),
    timestamp(0),
    period(0),
    ioFd(PtrToInt(busHandle)),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(INVALID_HANDLE_VALUE),
    isSocket(false)
{
    assert((eventType == IO_READ) || (eventType == IO_WRITE));
    ioHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    IoMonitor->RegisterEvent(this);
}

Event::~Event()
{
    /* Threads should not be waiting on this event */
    if ((handle != INVALID_HANDLE_VALUE) && !::SetEvent(handle)) {
        QCC_LogError(ER_FAIL, ("SetEvent failed with %d (%s)", GetLastError()));
    }
    if (TIMED == eventType) {
        timestamp = 0;
    }

    /* No longer monitoring I/O for this event */
    if (ioHandle != INVALID_HANDLE_VALUE) {
        IoMonitor->DeregisterEvent(this);
        CloseHandle(ioHandle);
    }

    /* Close the handles */
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }

    /* Cancel network change notification */
    if (networkIfaceEvent && networkIfaceHandle != INVALID_HANDLE_VALUE) {
        CancelMibChangeNotify2(networkIfaceHandle);
    }
}

QStatus Event::SetEvent()
{
    QStatus status = ER_OK;

    if (INVALID_HANDLE_VALUE != handle) {
        if (!::SetEvent(handle)) {
            status = ER_FAIL;
            QCC_LogError(status, ("SetEvent failed with %d", GetLastError()));
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
            QCC_LogError(status, ("ResetEvent failed with %d", GetLastError()));
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
    QStatus status = Wait(*this, 0);
    if (ioHandle != INVALID_HANDLE_VALUE) {
        /* Waiting for an I/O event can be interrupted by ER_STOPPING_THREAD or
           ER_ALERTED_THREAD, but the I/O event is set only in the ER_OK case.
         */
        return (ER_OK == status) ? true : false;
    }
    return (ER_TIMEOUT != status) ? true : false;
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

bool Event::IsNetworkEventSet()
{
    bool ret = false;
    if (this->IsSocket()) {
        static const timeval toZero = { 0, 0 };
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(this->ioFd, &fds);
        select(1, this->eventType == IO_READ ? &fds : NULL, this->eventType == IO_WRITE ? &fds : NULL, NULL, &toZero);
        ret = FD_ISSET(this->ioFd, &fds);
    }
#if (_WIN32_WINNT > 0x0603)
    else {
        DWORD eventMask;
        HANDLE pipe = LongToHandle((ULONG) this->ioFd);
        BOOL success = AllJoynEnumEvents(pipe, NULL, &eventMask);
        assert(success);
        if ((eventMask & NP_WRITE_SET) && (this->eventType == Event::IO_WRITE)) {
            ret = true;
        }
        if ((eventMask & NP_READ_SET) && (this->eventType == Event::IO_READ)) {
            ret = true;
        }
    }
#endif
    return ret;
}

}  /* namespace */

