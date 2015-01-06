/**
 * @file
 *
 * AutoPingerInternalInternal
 */

/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#ifndef _ALLJOYN_AUTOPINGERINTERNAL_H_
#define _ALLJOYN_AUTOPINGERINTERNAL_H_

#ifndef __cplusplus
#error Only include AutoPingerInternal.h in C++ code.
#endif

#include <map>
#include <qcc/Timer.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Debug.h>
#include <alljoyn/Status.h>
#include <alljoyn/PingListener.h>

namespace ajn {
/// @cond ALLJOYN_DEV
/** @internal Forward references */
struct PingGroup;
class BusAttachment;
/// @endcond

/**
 * AutoPingerInternal class
 */
class AutoPingerInternal : public qcc::AlarmListener {
  public:
    static void Init();
    static void Cleanup();

    /**
     * Create instance of autopinger
     *
     */
    AutoPingerInternal(BusAttachment& busAttachment);

    /**
     * Destructor
     */
    virtual ~AutoPingerInternal();

    /**
     * Pause all ping actions
     */
    void Pause();

    /**
     * Resume ping actions
     */
    void Resume();

    /**
     * Define new ping group
     *
     * @param  group Ping group name
     * @param  listener Listener called when a change was detected in the reachability of a destination
     * @param  pingInterval Ping interval in seconds
     */
    void AddPingGroup(const qcc::String& group, PingListener& listener, uint32_t pingInterval = 5);

    /**
     * Remove complete ping group, including all destinations
     *
     * @param  group Ping group name
     */
    void RemovePingGroup(const qcc::String& group);

    /**
     * Set ping interval of the specified group
     *
     * @param  group Ping group name
     * @param  pingInterval Ping interval in seconds
     * @return
     *  - #ER_OK: Interval updated
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    QStatus SetPingInterval(const qcc::String& group, uint32_t pingInterval);

    /**
     * Add a destination to the specified ping group
     * Destinations are refcounted and must be removed N times if they were added N times
     *
     * @param  group Ping group name
     * @param  destination Destination name to be pinged
     * @return
     *  - #ER_OK: destination added
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    QStatus AddDestination(const qcc::String& group, const qcc::String& destination);

    /**
     * Remove a destination from the specified ping group
     * This will lower the refcount by one and only remove the destination when the refcount reaches zero
     *
     * @param  group Ping group name
     * @param  destination Destination name to be removed
     * @param  removeAll Rather than decrementing the refcount by one, set refcount to zero and remove
     * @return
     *  - #ER_OK: destination removed or was not present
     *  - #ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
     */
    QStatus RemoveDestination(const qcc::String& group, const qcc::String& destination, bool removeAll = false);

  private:
    friend class AutoPingAsyncCB;
    friend struct Destination;
    friend class PingAsyncContext;

    enum PingState {
        UNKNOWN,
        LOST,
        AVAILABLE
    };

    AutoPingerInternal(const AutoPingerInternal&);
    void operator=(const AutoPingerInternal&);

    bool UpdatePingStateOfDestination(const qcc::String& group, const qcc::String& destination, const AutoPingerInternal::PingState state);
    void PingGroupDestinations(const qcc::String& group);
    bool IsRunning();
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    qcc::Timer timer; /* Single Timerthread */
    BusAttachment& busAttachment;
    qcc::Mutex pingerMutex;
    std::map<qcc::String, PingGroup*> pingGroups;

    bool pausing;
};
}
#endif /* AUTOPINGERINTERNAL_H_ */
