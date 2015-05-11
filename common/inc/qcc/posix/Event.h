/**
 * @file
 *
 * Platform independent event implementation
 */

/******************************************************************************
 *
 *
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

#ifndef _OS_QCC_EVENT_H
#define _OS_QCC_EVENT_H

#include <qcc/platform.h>

#include <vector>

#include <qcc/atomic.h>
#include <qcc/Mutex.h>

#include <Status.h>

/*
 * Must choose either pipes (MECHANISM_PIPE) or eventfd (MECHANISM_EVENTFD) as
 * the underlying OS mechanism for events.  We prefer eventfd as it is more
 * efficient, but we let users decide by pre-defining the mechanism.
 */
#if !defined(MECHANISM_EVENTFD) && !defined(MECHANISM_PIPE)
// #define MECHANISM_PIPE
#define MECHANISM_EVENTFD
#endif

/** @internal */
namespace qcc {

/** @internal Forward Reference */
class Source;

/**
 * Events are used to send signals between threads.
 */
class Event {
  public:

    /** Cause Wait to have no timeout */
    static const uint32_t WAIT_FOREVER = static_cast<uint32_t>(-1);

    /** Singleton always set Event */
    static Event& alwaysSet;

    /** Singleton never set Event */
    static Event& neverSet;

    /** Type of event */
    typedef enum {
        GEN_PURPOSE,     /**< General purpose pipe backed event */
        IO_READ,         /**< IO Read event */
        IO_WRITE,        /**< IO Write event */
        TIMED            /**< Event fires automatically when limit time is reached */
    } EventType;

    /** Create a general purpose Event. */
    Event();

    /**
     * Create a timed event.
     * Timed events cannot be manually set and reset.
     *
     * @param delay    Number of milliseconds to delay before Event is automatically set.
     * @param period   Number of milliseconds between auto-set events or 0 to indicate no repeat.
     */
    Event(uint32_t delay, uint32_t period = 0);

    /**
     * Constructor used by Linux specific I/O sources/sinks
     * (This constructor should only be used within Linux platform specific code.)
     *
     * @param event       Event whose underlying file descriptor is used as basis for new event
     * @param eventType   Type of event.
     * @param genPurpose  true if event should act as both an I/O event and a gen purpose event.
     */
    Event(Event& event, EventType eventType, bool genPurpose);

    /**
     * Constructor used by I/O sources/sinks
     *
     * @param ioFd        I/O file descriptor associated with this event.
     */
    Event(SocketFd ioFd, EventType eventType);

    /** Destructor */
    ~Event();

    /**
     * Wait on a group of events.
     * The call to Wait will return when any of the events on the list is signaled.
     *
     * @warning There is a subtle difference between Windows and Posix
     * implementations of this method.  In the Windows case, the return value of
     * this method inherits ER_TIMEOUT from any checkEvents that time out; but
     * in the posix case ER_OK is returned if one of the checkEvents times out.
     * For portable uses of this method consider ER_OK and ER_TIMEOUT as
     * indicating success.
     *
     * @param checkEvents    Vector of event object references to wait on.
     * @param signaledEvents Vector of event object references from checkEvents that are signaled.
     * @param maxMs          Max number of milliseconds to wait or WAIT_FOREVER to wait forever.
     * @return ER_OK if successful.
     */
    static QStatus Wait(const std::vector<Event*>& checkEvents, std::vector<Event*>& signaledEvents, uint32_t maxMs = WAIT_FOREVER);

    /**
     * Wait on a single event.
     * The call to Wait will return when the event is signaled.
     *
     * @param event   Event to wait on.
     * @param maxMs   Max number of milliseconds to wait or WAIT_FOREVER to wait forever.
     * @return ER_OK if successful.
     */
    static QStatus Wait(Event& event, uint32_t maxMs = WAIT_FOREVER);

    /**
     * Release a lock and then wait on a single event.
     * The call to Wait will return when the event is signaled.
     *
     * @param event   Event to wait on.
     * @param lock    The lock to release after incrementing numThreads
     * @param maxMs   Max number of milliseconds to wait or WAIT_FOREVER to wait forever.
     * @return ER_OK if successful.
     */
    static QStatus Wait(Event& event, qcc::Mutex& lock, uint32_t maxMs = WAIT_FOREVER)
    {
        event.IncrementNumThreads();
        lock.Unlock();
        QStatus status = Wait(event, maxMs);
        event.DecrementNumThreads();
        return status;
    }

    /**
     * Set the event to the signaled state.
     * All threads that are waiting on the event will become runnable.
     * Calling SetEvent when the state is already signaled has no effect.
     *
     * @return ER_OK if successful.
     */
    QStatus SetEvent();

    /**
     * Reset the event to the non-signaled state.
     * Threads that call wait() will block until the event state becomes signaled.
     * Calling ResetEvent() when the state of the event is non-signaled has no effect.
     *
     * @return ER_OK if successful.
     */
    QStatus ResetEvent();

    /**
     * Indicate whether the event is currently signaled.
     *
     * @return true iff event is signaled.
     */
    bool IsSet();

    /**
     * Reset TIMED event and set next auto-set delay and period.
     *
     * @param delay    Number of milliseconds to delay before Event is automatically set.
     * @param period   Number of milliseconds between auto-set events or 0 to indicate no repeat.
     */
    void ResetTime(uint32_t delay, uint32_t period);

    /**
     * Get the underlying file descriptor for I/O events.
     * This returns INVALID_SOCKET_FD if there is no underlying file descriptor.
     *
     * @return  The underlying file descriptor or INVALID_SOCKET_FD.
     */
    SocketFd GetFD() { return ioFd; }

    /**
     * Get the underlying event type.
     *
     * @return  The underlying event type.
     */
    EventType GetEventType() { return eventType; }

    /**
     * Get the number of threads that are currently blocked waiting for this event
     *
     * @return The number of blocked threads
     */
    uint32_t GetNumBlockedThreads() { return numThreads; }

  private:

    static void Init();
    static void Shutdown();
    friend class StaticGlobals;

    int fd;                 /**< File descriptor linked to general purpose event or -1 */
    int signalFd;           /**< File descriptor used by GEN_PURPOSE events to manually set/reset event */
    SocketFd ioFd;          /**< I/O File descriptor associated with event or -1 */
    EventType eventType;    /**< Indicates type of event */
    uint32_t timestamp;     /**< time for next triggering of TIMED Event */
    uint32_t period;        /**< Number of milliseconds between periodic timed events */
    volatile int32_t numThreads; /**< Number of threads currently waiting on this event */

    /**
     * Protected copy constructor.
     * Events on some platforms (windows) cannot be safely copied because they contain events handles.
     */
    Event& operator=(const Event& evt) { QCC_UNUSED(evt); return *this; }

    /**
     * Increment the count of threads blocked on this event
     */
    void IncrementNumThreads() { IncrementAndFetch(&numThreads); }

    /**
     * Decrement the count of threads blocked on this event
     */
    void DecrementNumThreads() { DecrementAndFetch(&numThreads); }

};

}  /* namespace */

#endif
