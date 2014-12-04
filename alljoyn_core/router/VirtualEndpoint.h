/**
 * @file
 * A VirtualEndpoint is a representation of an AllJoyn endpoint that exists behind a remote
 * AllJoyn daemon.
 */

/******************************************************************************
 * Copyright (c) 2009-2012,2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_VIRTUALENDPOINT_H
#define _ALLJOYN_VIRTUALENDPOINT_H

#include <qcc/platform.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>

#include "BusEndpoint.h"
#include "RemoteEndpoint.h"

#include <alljoyn/Message.h>

#include <alljoyn/Status.h>

namespace ajn {

class _VirtualEndpoint;

/**
 * %VirtualEndpoint is an alias for a remote bus connection that exists
 * behind a remote AllJoyn daemon.
 */
typedef qcc::ManagedObj<_VirtualEndpoint> VirtualEndpoint;


/**
 * %_VirtualEndpoint is managed class that implements the virtual endpoint functionality.
 */
class _VirtualEndpoint : public _BusEndpoint {
  public:

    /**
     * Default constructor initializes an invalid endpoint. This allows for the declaration of uninitialized VirtualEndpoint variables.
     */
    _VirtualEndpoint() : m_epState(EP_ILLEGAL) { }

    /**
     * Constructor
     *
     * @param uniqueName      Unique name for this endpoint.
     * @param b2bEp           Initial Bus-to-bus endpoint for this virtual endpoint.
     */
    _VirtualEndpoint(const qcc::String& uniqueName, RemoteEndpoint& b2bEp);

    ~_VirtualEndpoint() { }
    /**
     * Send an outgoing message.
     *
     * @param msg   Message to be sent.
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus PushMessage(Message& msg);

    /**
     * Send an outgoing message over a specific session.
     *
     * @param msg   Message to be sent.
     * @param id    SessionId to use for outgoing message.
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus PushMessage(Message& msg, SessionId id);

    /**
     * Get unique bus name.
     *
     * @return
     *      - unique bus name
     *      - empty if server has not yet assigned one (client-side).
     */
    const qcc::String& GetUniqueName() const { return m_uniqueName; }

    /**
     * Get the BusToBus endpoint associated with this virtual endpoint.
     *
     * @param sessionId   Id of session between src and dest.
     * @param b2bCount    [OUT] Number of b2bEps that can route for given session. May be NULL.
     * @return The current (top of queue) bus-to-bus endpoint or an invalid endpoint
     *         (endpoint->IsValid() == false) if there is no such endpoint.
     */
    RemoteEndpoint GetBusToBusEndpoint(SessionId sessionId = 0, int* b2bCount = NULL) const;

    /**
     * Gets the BusToBus endpoints associated with this virtual endpoint.
     *
     * @return The set of BusToBus endpoints that can route for this virtual endpoint.
     */
    std::multimap<SessionId, RemoteEndpoint> GetBusToBusEndpoints() const;

    /**
     * Add an alternate bus-to-bus endpoint that can route for this endpoint.
     *
     * @param endpoint   A bus-to-bus endpoint that can route to this virutual endpoint.
     * @return  true if endpoint was added.
     */
    bool AddBusToBusEndpoint(RemoteEndpoint& endpoint);

    /**
     * Remove a bus-to-bus endpoint that can route for thie virtual endpoint.
     *
     * @param endpoint   Bus-to-bus endpoint to remove from list of routes
     * @return  true iff virtual endpoint has no bus-to-bus endpoint and should be removed.
     */
    bool RemoveBusToBusEndpoint(RemoteEndpoint& endpoint);

    /**
     * Map a session id to one of this VirtualEndpoint's B2B endpoints.
     *
     * @param sessionId  The session id.
     * @param b2bEp      The bus-to-bus endpoint for the session.
     * @return  ER_OK if successful.
     */
    QStatus AddSessionRef(SessionId sessionId, RemoteEndpoint& b2bEp);

    /**
     * Map a session id to the best of this VirtualEndpoint's B2B endpoints that match session opts.
     *
     * @param sessionId  The session id.
     * @param opts       Qualifying session opts for B2B endpoint or NULL to indicate no constraints.
     * @param b2bEp      [OUT] Written with B2B chosen for session.
     * @return  ER_OK if successful.
     */
    QStatus AddSessionRef(SessionId sessionId, SessionOpts* opts, RemoteEndpoint& b2bEp);

    /**
     * Remove (counted) mapping of sessionId to B2B endpoint.
     *
     * @param sessionId  The session id.
     */
    void RemoveSessionRef(SessionId sessionId);

    /**
     * Return true iff the given bus-to-bus endpoint can potentially be used to route
     * messages for this virtual endpoint.
     *
     * @param b2bEndpoint   B2B endpoint being checked for suitability as a route for this virtual endpoint.
     * @return true iff the B2B endpoint can be used to route messages for this virtual endpoint.
     */
    bool CanUseRoute(const RemoteEndpoint& b2bEndpoint) const;

    /**
     * Return true iff any of the B2B eps named in the set can be used to route messages for this virtual endpoint.
     *
     * @param b2bNames   Set of unique-names of b2b endpoints to be tested.
     * @return true iff any of the B2B endpoints can be used to route messages for this virtual endpoint.
     */
    bool CanUseRoutes(const std::multiset<qcc::String>& b2bNames) const;

    /**
     * Return true iff the virtual endpoint can route to destination without the aid of the
     * daemon identified by guid.
     *
     * @param guid     GUID of daemon that should be ignored when determing whether vep can route to dest.
     * @return true iff the vep can route to its dest without the aid of daemon identified by guid.
     */
    bool CanRouteWithout(const qcc::GUID128& guid) const;

    /**
     * Get the set of sessionIds that route through a given bus-to-bus endpoint.
     *
     * @param[IN]   b2bEndpoint   B2B endpoint.
     * @param[OUT]  set of sessionIds that route through the given endpoint.
     */
    void GetSessionIdsForB2B(RemoteEndpoint& endpoint, std::set<SessionId>& sessionIds);

    /**
     * Indicate whether this endpoint is allowed to receive messages from remote devices.
     * VirtualEndpoints are always allowed to receive remote messages.
     *
     * @return true
     */
    bool AllowRemoteMessages() { return true; }

    enum EndpointState {
        EP_ILLEGAL = 0,      /**< This is an invalid endpoint. i.e. constructed with the default constructor. */
        EP_STARTED,          /**< The endpoint has at least one bus-to-bus endpoint */
        EP_STOPPING,         /**< The endpoint is being stopped. */
    };

    /**
     * Indicates whether the virtual endpoint is being stopped.
     *
     * @return true iff the endpoint is in the process of being stopped.
     */
    bool IsStopping(void) { return m_epState == EP_STOPPING; }
  private:

    const qcc::String m_uniqueName;                             /**< The unique name for this endpoint */
    std::multimap<SessionId, RemoteEndpoint> m_b2bEndpoints;    /**< Set of b2bs that can route for this virtual ep */

    /** B2BInfo is a data container that holds B2B endpoint selection criteria */
    struct B2BInfo {
        SessionOpts opts;     /**< Session options for B2BEndpoint */
        uint32_t hops;        /**< Currently unused hop count from local daemon to final destination */
    };
    mutable qcc::Mutex m_b2bEndpointsLock;      /**< Lock that protects m_b2bEndpoints */
    bool m_hasRefs;
    EndpointState m_epState;                                    /**< The state of the virtual endpoint */

};

}

#endif
