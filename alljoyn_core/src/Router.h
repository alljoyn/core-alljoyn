/**
 * @file
 * Router is responsible for routing Bus messages between one or more AllJoynTransports(s)
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
#ifndef _ALLJOYN_ROUTER_H
#define _ALLJOYN_ROUTER_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include "BusEndpoint.h"

namespace ajn {

/**
 * %Router defines an interface that describes how to route messages between two
 * or more endpoints.
 */
class Router {
  public:

    /**
     * Destructor
     */
    virtual ~Router() { }

    /**
     * Route an incoming Message Bus Message from an endpoint.
     *
     * @param sender  Endpoint that is sending the message
     * @param msg     Message to be processed.
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise.
     */
    virtual QStatus PushMessage(Message& msg, BusEndpoint& sender) = 0;

    /**
     * Register an endpoint.
     * This method must be called by an endpoint before attempting to use the router.
     *
     * @param endpoint   Endpoint being registered.
     * @return ER_OK if successful.
     */
    virtual QStatus RegisterEndpoint(BusEndpoint& endpoint) = 0;

    /**
     * Un-register an endpoint.
     * This method must be called by an endpoint before the endpoint is deallocted.
     *
     * @param epName   Name of endpoint being registered.
     */
    virtual void UnregisterEndpoint(const qcc::String& epName, EndpointType epType) = 0;

    /**
     * Find the endpoint that owns the given unique or well-known name.
     *
     * @param busname    Unique or well-known bus name
     *
     * @return  Returns the requested endpoint or an invalid endpoint if the
     *          endpoint was not found.
     */
    virtual BusEndpoint FindEndpoint(const qcc::String& busname) = 0;

    /**
     * Generate a unique endpoint name.
     * This method is not used by non-daemon instnces of the router.
     * An empty string is returned if called in this case.
     *
     * @return A unique bus name that can be assigned to a (server-side) endpoint.
     */
    virtual qcc::String GenerateUniqueName(void) = 0;

    /**
     * Return true if this router is in contact with a bus (either locally or remotely)
     * This method can be used to determine whether messages sent to "the bus" will be routed.
     *
     * @return @b true if the messages can be routed currently.
     */
    virtual bool IsBusRunning(void) const = 0;

    /**
     * Determine whether this is an AllJoyn daemon process.
     *
     * @return true iff this bus instance is an AllJoyn daemon process.
     */
    virtual bool IsDaemon() const = 0;

    /**
     * Set the global GUID of the bus.
     *
     * @param guid   GUID of bus associated with this router.
     */
    virtual void SetGlobalGUID(const qcc::GUID128& guid) = 0;
};

}

#endif
