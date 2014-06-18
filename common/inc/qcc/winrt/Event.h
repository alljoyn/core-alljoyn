/**
 * @file
 *
 * Platform independent event implementation
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <windows.h>
#include <vector>

#include <qcc/atomic.h>
#include <qcc/Mutex.h>

#include <Status.h>


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
    static Event alwaysSet;

    /** Singleton never set Event */
    static Event neverSet;

    /** Indicate how to select on file descriptor */
    typedef enum {
        GEN_PURPOSE,   /**< General purpose Windows event backed event */
        IO_READ,       /**< IO read event */
        IO_WRITE,      /**< IO write event */
        TIMED          /**< Event is automatically set based on time */
    } EventType;

    /**
     * Create a general purpose Event.
     * General purpose events are manually set and and reset.
     */
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
     * Create an event from an existing event.
     * This constructor is typically used to create an IO_WRITE type event from an IO_READ one.
     * (or vice-versa). Some platforms (windows) do not allow creation of two independent events
     * from the same socket descriptor. Thus, this constructor is used for creating the second
     * Event (sink or source) rather than using the one that takes a file descriptor as an argument twice.
     *
     * @param event       Event whose underlying source is used a basis for a new event.
     * @param genPurpose  true if event should act as both an I/O event and a gen purpose event.
     */
    Event(Event& event, EventType eventType, bool genPurpose);

    /**
     * Constructor used by Windows specific I/O sources/sinks
     * (This constructor should only be used within Windows platform specific code.)
     *
     * @param fd          Socket descriptor associated with this event.
     * @param eventType   Type of event (IO_READ or IO_WRITE) associated with fd.
     * @param genPurpose  true if event should act as both an I/O event and a gen purpose event.
     */
    Event(qcc::SocketFd fd, EventType eventType, bool genPurpose);

    /** Destructor */
    ~Event();

    /**
     * Shutdown the event and unblock any waiters
     */
    void Close();

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
     * @return ER_OK if successful, ER_TIMEOUT if one if the checkEvents times out.
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
     * Indicate whether the event is currently set.
     *
     * @return true iff event is set.
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
     * Get the underlying file descriptor for I/O backed events.
     * This returns -1 if there is no underlying file descriptor.
     *
     * @return  The underlying file descriptor or -1.
     */
    qcc::SocketFd GetFD() const { return ioFd; }

    /**
     * Get the underlying Windows' event handle.  Use of this function is not
     * portable and should only be used in platform specific code.
     *
     * @return  The underlying event handle.
     */
    HANDLE GetHandle() const
    {
        return ((eventType == GEN_PURPOSE) ? handle : (eventType != TIMED) ? ioHandle : INVALID_HANDLE_VALUE);
    }

    /**
     * Return the type of this event.
     */
    EventType GetEventType() const { return eventType; }

    /**
     * Get the number of threads that are currently blocked waiting for this event
     *
     * @return The number of blocked threads
     */
    uint32_t GetNumBlockedThreads() { return numThreads; }

  private:

    HANDLE handle;          /**< General purpose event handle */
    HANDLE ioHandle;        /**< I/O event handle */
    EventType eventType;    /**< Type of event */
    uint32_t timestamp;     /**< time for next triggering of TIMED Event */
    uint32_t period;        /**< Number of milliseconds between periodic timed events */
    qcc::SocketFd ioFd;     /**< Socket descriptor or -1 if not socket based IO */
    int32_t numThreads;     /**< Number of threads currently waiting on this event */

    /**
     * Protected copy constructor.
     * Events cannot be safely copied because they contain events handles.
     */
    Event& operator=(const Event& evt) { return *this; }

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
