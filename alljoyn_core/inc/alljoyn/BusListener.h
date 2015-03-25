/**
 * @file
 * BusListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to asynchronously receive bus  related event information.
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
#ifndef _ALLJOYN_BUSLISTENER_H
#define _ALLJOYN_BUSLISTENER_H

#ifndef __cplusplus
#error Only include BusListener.h in C++ code.
#endif

#include <alljoyn/TransportMask.h>

namespace ajn {

/**
 * Foward declaration.
 */
class BusAttachment;
class MsgArg;

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of bus related events.
 */
class BusListener {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~BusListener() { }

    /**
     * Called by the bus when the listener is registered. This gives the listener implementation the
     * opportunity to save a reference to the bus.
     *
     * @param bus  The bus the listener is registered with.
     */
    virtual void ListenerRegistered(BusAttachment* bus) {
        QCC_UNUSED(bus);
    }

    /**
     * Called by the bus when the listener is unregistered.
     */
    virtual void ListenerUnregistered() { }

    /**
     * Called by the bus when an external bus is discovered that is advertising a well-known name
     * that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
     *
     * @param name         A well known name that the remote bus is advertising.
     * @param transport    Transport that received the advertisement.
     * @param namePrefix   The well-known name prefix used in call to FindAdvertisedName that triggered this callback.
     */
    virtual void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        QCC_UNUSED(name);
        QCC_UNUSED(transport);
        QCC_UNUSED(namePrefix);
    }

    /**
     * Called by the bus when an advertisement previously reported through FoundName has become unavailable.
     *
     * @param name         A well known name that the remote bus is advertising that is of interest to this attachment.
     * @param transport    Transport that stopped receiving the given advertised name.
     * @param namePrefix   The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.
     */
    virtual void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        QCC_UNUSED(name);
        QCC_UNUSED(transport);
        QCC_UNUSED(namePrefix);
    }

    /**
     * Called by the bus when the ownership of any well-known name changes.
     *
     * @param busName        The well-known name that has changed.
     * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
     * @param newOwner       The unique name that now owns the name or NULL if there is no new owner.
     */
    virtual void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner) {
        QCC_UNUSED(busName);
        QCC_UNUSED(previousOwner);
        QCC_UNUSED(newOwner);
    }

    /**
     * This has been deprecated.  It will not be called any more. Use the ProxyBusObject property change handling mechanism instead.
     * Called by the bus when the value of a property changes if that property has annotation.
     *
     * @param propName       The well-known name that has changed.
     * @param propValue      The new value of the property; NULL if not present
     */
    QCC_DEPRECATED(virtual void PropertyChanged(const char* propName, const MsgArg * propValue)) {
        QCC_UNUSED(propName);
        QCC_UNUSED(propValue);
    }

    /**
     * Called when a BusAttachment this listener is registered with is stopping.
     */
    virtual void BusStopping() { }

    /**
     * Called when a BusAttachment this listener is registered with has become disconnected from
     * the bus.
     */
    virtual void BusDisconnected() { }

};

}

#endif
