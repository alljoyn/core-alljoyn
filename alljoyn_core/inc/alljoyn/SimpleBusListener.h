/**
 * @file
 * SimpleBusListener is a synchronous bus listener that fits the need of applications that can
 * handled all bus events from the main thread.
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
#ifndef _ALLJOYN_SIMPLEBUSLISTENER_H
#define _ALLJOYN_SIMPLEBUSLISTENER_H

#ifndef __cplusplus
#error Only include SimpleBusListener.h in C++ code.
#endif

#include <alljoyn/Session.h>
#include <alljoyn/BusListener.h>

namespace ajn {

/**
 * @name Bus Events
 * The various bus events.
 */
// @{
/** Event indicating an advertised name was found */
static const uint32_t BUS_EVENT_FOUND_ADVERTISED_NAME = 0x0001;
/** Event indicating a previously found name has been lost*/
static const uint32_t BUS_EVENT_LOST_ADVERTISED_NAME  = 0x0002;
/** Event indicating that the ownership of a bus name has changed. */
static const uint32_t BUS_EVENT_NAME_OWNER_CHANGED    = 0x0004;
/** Bit mask that can be used filter Bus Events */
static const uint32_t BUS_EVENT_ALL                   = 0x00FF;
/** Bit mask that can be used filter Bus Events */
static const uint32_t BUS_EVENT_NONE                  = 0x0000;
// @}
/**
 * Helper class that provides a blocking API that allows application threads to wait for bus events.
 */
class SimpleBusListener : public BusListener {
  public:

    /**
     * Indicator used when calling the #WaitForEvent method to indicate that the
     * listener should wait forever for the specified BusEvent.
     */
    static const uint32_t FOREVER = (uint32_t)-1;

    /**
     * Constructor that initializes a bus listener with specific events enabled.
     *
     * @param enabled   A logical OR of the bus events to be enabled for this listener.
     */
    SimpleBusListener(uint32_t enabled = 0);

    /**
     * Set an event filter. This overrides the events enabled by the constructor. Any queued events
     * that are not enabled are discarded.
     *
     * @param enabled  A logical OR of the bus events to be enabled for this listener.
     */
    void SetFilter(uint32_t enabled);

    /**
     * This union is used to return busEvent information for the bus events.
     */
    class BusEvent {
        friend class SimpleBusListener;
      public:
        uint32_t eventType;             ///< The busEvent type identifies which variant from the union applies.
        union {
            struct {
                const char* name;               ///< well known name that the remote bus is advertising that is of interest to this attachment.
                TransportMask transport;        ///< Transport that received the advertisement.
                const char* namePrefix;         ///< The well-known name prefix used in call to FindAdvertisedName that triggered the busEvent.
            } foundAdvertisedName;
            struct {
                const char* name;       ///< A well known name that the remote bus is advertising that is of interest to this attachment.
                TransportMask transport;        ///< Transport that received the advertisement.
                const char* namePrefix; ///< The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.
            } lostAdvertisedName;
            struct {
                const char* busName;       ///< The well-known name that has changed.
                const char* previousOwner; ///< The unique name that previously owned the name or NULL if there was no previous owner.
                const char* newOwner;      ///< The unique name that now owns the name or NULL if there is no new owner.
            } nameOwnerChanged;
        };

        /**
         * Constructor
         */
        BusEvent() : eventType(0) { }

        /**
         * Copy constructor.
         */
        BusEvent(const BusEvent& other) { *this = other; }

        /**
         * Assignment operator.
         */
        BusEvent& operator=(const BusEvent& other);

      private:

        /**
         * @internal  Storage for the BusEvent strings.
         */
        qcc::String strings[3];
    };

    /**
     * Wait for a bus event.
     *
     * @param busEvent Returns the busEvent type and related information.
     * @param timeout  A timeout in milliseconds to wait for the busEvent, 0 means don't wait just
     *                 check for an busEvent and return, FOREVER (-1) means wait forever.
     *
     * @return
     *         - #ER_OK if an even was received.
     *         - #ER_TIMEOUT if the wait timed out.
     *         - #ER_ALERTED_THREAD if the wait unblocked due to a signal
     */
    QStatus WaitForEvent(BusEvent& busEvent, uint32_t timeout = FOREVER);

    /**
     * Destructor.
     */
    ~SimpleBusListener();

  private:

    /**
     * Copy constructor.
     * @param other   Object to copy from.
     */
    SimpleBusListener(const SimpleBusListener& other);

    /**
     * Assignment operator.
     * @param other  RHS of assignment.
     */
    SimpleBusListener& operator=(const SimpleBusListener& other);

    /**
     * Bit mask of events enabled for this listener.
     */
    uint32_t enabled;

    void ListenerRegistered(BusAttachment* bus);
    void ListenerUnregistered();
    void BusStopping();

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);

    /**
     * @internal
     * Internal storage for this class.
     */
    class Internal;
    Internal& internal;

};

}

#endif
