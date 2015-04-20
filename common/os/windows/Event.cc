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
    QCC_UNUSED(row);
    QCC_UNUSED(notificationType);

    Event* event = (Event*) arg;
    QCC_DbgHLPrintf(("Received network interface event type %u", notificationType));
    if (!::SetEvent(event->GetHandle())) {
        QCC_LogError(ER_OS_ERROR, ("Setting network interface event failed"));
    }
}

/* This class allows a thread to wait for more than 64 handles at a time. */
class SuperWaiter {
  public:
    SuperWaiter() :
        m_shutdownEvent(nullptr),
        m_cleanupGroup(nullptr),
        m_signaled(false),
        m_indexSignaled(MAXUINT)
    {
    }

    ~SuperWaiter()
    {
        /* Wait until all thread pool workers exit */
        CloseThreadpoolCleanupGroupMembers(m_cleanupGroup, FALSE, nullptr);
        CloseThreadpoolCleanupGroup(m_cleanupGroup);
        m_cleanupGroup = nullptr;

        DestroyThreadpoolEnvironment(&m_environment);

        if (m_shutdownEvent != nullptr) {
            CloseHandle(m_shutdownEvent);
        }
    }

    QStatus Initialize()
    {
        /* Use a manual-reset event to abort the wait for all groups */
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

        m_shutdownEvent = shutdown;
        m_cleanupGroup = cleanupGroup;

        InitializeThreadpoolEnvironment(&m_environment);
        SetThreadpoolCallbackCleanupGroup(&m_environment, m_cleanupGroup, nullptr);
        return ER_OK;
    }

    void Shutdown()
    {
        SetEvent(m_shutdownEvent);
    }

    QStatus WaitForMultipleObjects(_In_ uint32_t totalHandles, _In_reads_(totalHandles) const HANDLE* handles, _In_ uint32_t timeoutMsec, _Out_ uint32_t* indexSignaled)
    {
        assert(totalHandles != 0);
        m_signaled = false;
        *indexSignaled = m_indexSignaled = MAXUINT;

        /* Subtract 1 from MAXIMUM_WAIT_OBJECTS to accomodate the shutdown event */
        static const uint32_t maximumGroupHandles = MAXIMUM_WAIT_OBJECTS - 1;
        uint32_t startIndex = 0;

        while (startIndex < totalHandles) {
            uint32_t numHandles = totalHandles - startIndex;
            if (numHandles > maximumGroupHandles) {
                numHandles = maximumGroupHandles;
            }

            WaitGroup* group = new WaitGroup(this, &handles[startIndex], numHandles, startIndex, timeoutMsec);
            startIndex += numHandles;

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

        /* Wait until all thread pool workers exit */
        CloseThreadpoolCleanupGroupMembers(m_cleanupGroup, FALSE, nullptr);

        if (!m_signaled) {
            return ER_TIMEOUT;
        }

        *indexSignaled = m_indexSignaled;
        return ER_OK;
    }

  private:
    HANDLE m_shutdownEvent;
    TP_CALLBACK_ENVIRON m_environment;
    PTP_CLEANUP_GROUP m_cleanupGroup;
    volatile bool m_signaled;
    volatile uint32_t m_indexSignaled;

    struct WaitGroup {
        WaitGroup(_In_ SuperWaiter* waiter, _In_reads_(numHandles) const HANDLE* handles, _In_ uint32_t numHandles, _In_ uint32_t groupIndex, _In_ uint32_t timeoutMsec) :
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
        const uint32_t m_numHandles;
        const uint32_t m_groupIndex;
        const uint32_t m_timeoutMsec;
      private:
        /* Private assigment operator - does nothing */
        WaitGroup operator=(const WaitGroup&);
    };

    static VOID CALLBACK s_WaitThread(_Inout_ PTP_CALLBACK_INSTANCE instance, _Inout_opt_ PVOID context, _Inout_ PTP_WORK work)
    {
        QCC_UNUSED(work);
        WaitGroup* group = reinterpret_cast<WaitGroup*>(context);

        /* Make sure we have another thread available for the next wait group */
        CallbackMayRunLong(instance);

        uint32_t numHandles = group->m_numHandles + 1;
        HANDLE* handles = new HANDLE[numHandles];
        handles[0] = group->m_waiter->m_shutdownEvent;
        if (memcpy_s(&handles[1], (sizeof(HANDLE) * group->m_numHandles), group->m_handles, (sizeof(HANDLE) * group->m_numHandles))) {
            assert(!"memcpy_s failed");
            delete [] handles;
            delete group;
            return;
        }

        DWORD waitResult = ::WaitForMultipleObjectsEx(numHandles, handles, false, group->m_timeoutMsec, false);
        delete[] handles;

        switch (waitResult) {
        case WAIT_FAILED:
        case WAIT_ABANDONED:
            assert(false);
            break;

        case WAIT_TIMEOUT:
            break;

        case WAIT_OBJECT_0:
            /* One of the other groups has found a signaled event already */
            break;

        default:
            /* Calculate the true wait result, subtract 1 because the first index is for shutdown purpose */
            uint32_t indexSignaled = group->m_groupIndex + ((waitResult - WAIT_OBJECT_0) - 1);
            group->m_waiter->m_indexSignaled = indexSignaled;
            group->m_waiter->m_signaled = true;

            /* Ask the other groups to finish waiting */
            group->m_waiter->Shutdown();
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
        QCC_DbgHLPrintf(("RegisterEvent %s for fd %d (ioHandle=%p)", event->GetEventType() == Event::IO_READ ? "IO_READ" : "IO_WRITE", sock, event->GetHandle()));
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
        QCC_UNUSED(event);
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
        QCC_UNUSED(event);
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
    QCC_UNUSED(TimerOrWaitFired);

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
                        QCC_DbgHLPrintf(("Setting write event %p", ev->GetHandle()));
                    }
                    if ((ioEvents.lNetworkEvents & READ_SET) && (ev->GetEventType() == Event::IO_READ)) {
                        isSet = true;
                        QCC_DbgHLPrintf(("Setting read event %p", ev->GetHandle()));
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
                        QCC_DbgHLPrintf(("Setting write event %p", ev->GetHandle()));
                    }
                    if ((eventMask & NP_READ_SET) && (ev->GetEventType() == Event::IO_READ)) {
                        isSet = true;
                        QCC_DbgHLPrintf(("Setting read event %p", ev->GetHandle()));
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

QStatus AJ_CALL Event::Wait(Event& evt, uint32_t maxWaitMs)
{
    HANDLE handles[3];
    uint32_t numHandles = 0;

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
    if (nullptr != evt.ioHandle) {
        handles[numHandles++] = evt.ioHandle;
    }
    if (nullptr != evt.handle) {
        handles[numHandles++] = evt.handle;
    }
    if (nullptr != evt.timerHandle) {
        assert(TIMED == evt.eventType);
        handles[numHandles++] = evt.timerHandle;
    }

    Thread* thread = Thread::GetThread();
    Event* stopEvent = &thread->GetStopEvent();
    handles[numHandles++] = stopEvent->handle;
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
        QCC_DbgPrintf(("WaitForMultipleObjectsEx timeout %u", maxWaitMs));
        status = ER_TIMEOUT;
    } else {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("WaitForMultipleObjectsEx returned 0x%x.", ret));
        if (WAIT_FAILED == ret) {
            QCC_LogError(status, ("GetLastError=%u", GetLastError()));
            QCC_LogError(status, ("numHandles=%u, maxWaitMs=%u, Handles: ", numHandles, maxWaitMs));
            for (uint32_t i = 0; i < numHandles; ++i) {
                QCC_LogError(status, ("  0x%p", handles[i]));
            }
        }
    }
    return status;
}


QStatus AJ_CALL Event::Wait(const vector<Event*>& checkEvents, vector<Event*>& signaledEvents, uint32_t maxWaitMs)
{
    QStatus status = ER_OK;
    uint32_t numHandles = 0;
    vector<HANDLE> handles;
    vector<Event*>::const_iterator it;

    for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
        Event* evt = *it;
        evt->IncrementNumThreads();
        if (evt->handle != nullptr) {
            handles.push_back(evt->handle);
            numHandles++;
        }
        if (evt->ioHandle != nullptr) {
            handles.push_back(evt->ioHandle);
            numHandles++;
        }
        if (evt->timerHandle != nullptr) {
            assert(evt->eventType == TIMED);
            handles.push_back(evt->timerHandle);
            numHandles++;
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

    if (numHandles == 0) {
        QCC_LogError(status, ("Wait: incorrect checkEvents vector."));
        assert(false);

        /* Preserve the behavior of the Posix implementation */
        Sleep(maxWaitMs);
        return ER_TIMEOUT;
    }

    bool somethingSet = true;

    while ((status == ER_OK) && signaledEvents.empty() && somethingSet) {
        uint32_t orig = GetTimestamp();
        HANDLE signaledHandle = nullptr;

        if (numHandles <= MAXIMUM_WAIT_OBJECTS) {
            DWORD ret = WaitForMultipleObjectsEx(numHandles, handles.data(), FALSE, maxWaitMs, FALSE);

            if ((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + numHandles)) {
                /* One of the handles was signaled */
                uint32_t signaledIndex = ret - WAIT_OBJECT_0;
                signaledHandle = handles[signaledIndex];
            } else {
                somethingSet = false;

                if (ret != WAIT_TIMEOUT) {
                    status = ER_OS_ERROR;
                    if (ret == WAIT_FAILED) {
                        QCC_LogError(status, ("WaitForMultipleObjectsEx(2) returned 0x%x, error=%d.", ret, GetLastError()));
                    } else if (ret == WAIT_ABANDONED) {
                        QCC_LogError(status, ("WaitForMultipleObjectsEx(2) returned WAIT_ABANDONED."));
                    }
                }
            }
        } else {
            SuperWaiter waiter;
            status = waiter.Initialize();
            if (status == ER_OK) {
                uint32_t signaledIndex;
                status = waiter.WaitForMultipleObjects(numHandles, handles.data(), maxWaitMs, &signaledIndex);
                if (status != ER_OK) {
                    somethingSet = false;
                } else {
                    signaledHandle = handles[signaledIndex];
                }
            }
        }

        for (it = checkEvents.begin(); it != checkEvents.end(); ++it) {
            Event* evt = *it;
            evt->DecrementNumThreads();

            if (somethingSet) {
                if (evt->eventType == TIMED) {
                    /*
                     * Avoid calling IsSet() because that will call Wait with zero timeout. The Wait above already
                     * switched this timer to non-signaled state, so IsSet() would incorrectly return false.
                     */
                    if ((evt->timerHandle != nullptr) && (evt->timerHandle == signaledHandle)) {
                        signaledEvents.push_back(evt);
                    }
                } else if (evt->IsSet()) {
                    signaledEvents.push_back(evt);
                }
            }
        }

        uint32_t now = GetTimestamp();
        /* Adjust the maxWaitMs by the time elapsed, in case we loop back up and call WaitForMultipleObjectsEx again. */
        maxWaitMs -= now - orig;
        /*
         * If somethingSet is true, signaledEvents must not be empty here. But there are situations when WaitForMultipleObjectsEx
         * can return even though the event is not set. So, if somethingSet is true, but signaledEvents is empty,
         * we call WaitForMultipleObjectsEx again.
         */
    }

    if ((status == ER_OK) && signaledEvents.empty()) {
        status = ER_TIMEOUT;
    }

    /* Log everything but OK or TIME_OUT before returning a generic error */
    if ((ER_OK != status) && (ER_TIMEOUT != status)) {
        QCC_LogError(status, ("Wait failed, numHandles=%u, maxWaitMs=%u", numHandles, maxWaitMs));
        QCC_DbgHLPrintf(("Wait failed, handles:"));
        for (uint32_t i = 0; i < numHandles; ++i) {
            QCC_DbgHLPrintf(("  0x%p", handles[i]));
        }
        status = ER_FAIL;
    }
    return status;
}

Event::Event() :
    handle(CreateEvent(NULL, TRUE, FALSE, NULL)),
    ioHandle(nullptr),
    eventType(GEN_PURPOSE),
    period(0),
    timerHandle(nullptr),
    ioFd(INVALID_SOCKET_FD),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(nullptr),
    isSocket(false)
{
}

Event::Event(bool networkIfaceEvent) :
    handle(CreateEvent(NULL, TRUE, FALSE, NULL)),
    ioHandle(nullptr),
    eventType(GEN_PURPOSE),
    period(0),
    timerHandle(nullptr),
    ioFd(INVALID_SOCKET_FD),
    numThreads(0),
    networkIfaceEvent(networkIfaceEvent),
    networkIfaceHandle(nullptr),
    isSocket(false)
{
    if (networkIfaceEvent) {
        NotifyIpInterfaceChange(AF_UNSPEC, (PIPINTERFACE_CHANGE_CALLBACK)IpInterfaceChangeCallback, this, false, &networkIfaceHandle);
    }
}

Event::Event(Event& event, EventType eventType, bool genPurpose) :
    handle(nullptr),
    ioHandle(nullptr),
    eventType(eventType),
    period(0),
    timerHandle(nullptr),
    ioFd(event.ioFd),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(nullptr),
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
    handle(nullptr),
    ioHandle(nullptr),
    eventType(eventType),
    period(0),
    timerHandle(nullptr),
    ioFd(ioFd),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(nullptr),
    isSocket(true)
{
    /* Create an auto reset event for the socket fd */
    if (ioFd != INVALID_SOCKET_FD) {
        assert((eventType == IO_READ) || (eventType == IO_WRITE));
        ioHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
        IoMonitor->RegisterEvent(this);
    }
}

Event::Event(uint32_t delay, uint32_t period) :
    handle(nullptr),
    ioHandle(nullptr),
    eventType(TIMED),
    period(period),
    timerHandle(nullptr),
    ioFd(INVALID_SOCKET_FD),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(nullptr),
    isSocket(false)
{
    timerHandle = CreateWaitableTimerW(NULL, FALSE, NULL);

    if (timerHandle != nullptr) {
        ResetTime(delay, period);
    }
}

Event::Event(HANDLE busHandle, EventType eventType) :
    handle(nullptr),
    ioHandle(nullptr),
    eventType(eventType),
    period(0),
    timerHandle(nullptr),
    ioFd(PtrToInt(busHandle)),
    numThreads(0),
    networkIfaceEvent(false),
    networkIfaceHandle(nullptr),
    isSocket(false)
{
    assert((eventType == IO_READ) || (eventType == IO_WRITE));
    ioHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    IoMonitor->RegisterEvent(this);
}

Event::~Event()
{
    /* Threads should not be waiting on this event */
    if ((handle != nullptr) && !::SetEvent(handle)) {
        QCC_LogError(ER_FAIL, ("SetEvent failed with %d", GetLastError()));
    }

    /* No longer monitoring I/O for this event */
    if (ioHandle != nullptr) {
        IoMonitor->DeregisterEvent(this);
        CloseHandle(ioHandle);
    }

    /* Close the handles */
    if (handle != nullptr) {
        CloseHandle(handle);
    }

    /* Cancel network change notification */
    if (networkIfaceHandle != nullptr) {
        CancelMibChangeNotify2(networkIfaceHandle);
    }

    if (timerHandle != nullptr) {
        assert(eventType == TIMED);
        ::CancelWaitableTimer(timerHandle);
        ::CloseHandle(timerHandle);
    }
}

QStatus Event::SetEvent()
{
    QStatus status = ER_OK;

    if (handle != nullptr) {
        if (!::SetEvent(handle)) {
            status = ER_FAIL;
            QCC_LogError(status, ("SetEvent failed with %d", GetLastError()));
        }
    } else if (TIMED == eventType) {
        ResetTime(0, period);
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

    if (handle != nullptr) {
        if (!::ResetEvent(handle)) {
            status = ER_FAIL;
            QCC_LogError(status, ("ResetEvent failed with %d", GetLastError()));
        }
    } else if (TIMED == eventType) {
        if (0 < period) {
            ResetTime(period, period);
        } else {
            ResetTime(WAIT_FOREVER, period);
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
    if (ioHandle != nullptr) {
        /* Waiting for an I/O event can be interrupted by ER_STOPPING_THREAD or
           ER_ALERTED_THREAD, but the I/O event is set only in the ER_OK case.
         */
        return (ER_OK == status) ? true : false;
    }
    return (ER_TIMEOUT != status) ? true : false;
}

void Event::ResetTime(uint32_t delay, uint32_t period)
{
    this->period = period;

    if (delay == WAIT_FOREVER) {
        ::CancelWaitableTimer(timerHandle);
    } else {
        LARGE_INTEGER timerDelay = { 0 };
        timerDelay.QuadPart -= (int64_t)delay;
        timerDelay.QuadPart *= 10000i64;
        ::SetWaitableTimer(timerHandle, &timerDelay, (int32_t)period, NULL, NULL, FALSE);
    }
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

