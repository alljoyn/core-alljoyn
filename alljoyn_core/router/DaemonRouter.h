/**
 * @file
 * DaemonRouter is a "full-featured" router responsible for routing Bus messages
 * between one or more remote endpoints and a single local endpoint.
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
#ifndef _ALLJOYN_DAEMONROUTER_H
#define _ALLJOYN_DAEMONROUTER_H

#include <qcc/platform.h>

#include <vector>

#include <qcc/Thread.h>

#include "Transport.h"

#include <alljoyn/Status.h>

#include "LocalTransport.h"
#include "Router.h"
#include "NameTable.h"
#include "RuleTable.h"

#define ENABLE_OLD_PUSHMESSAGE_COMPATABILITY

namespace ajn {


/**
 * @internal Forward delcarations
 */
class BusController;
class AllJoynObj;
class SessionlessObj;

/**
 * DaemonRouter is a "full-featured" router responsible for routing Bus messages
 * between one or more remote endpoints and a single local endpoint.
 */
class DaemonRouter : public Router {

    friend class _LocalEndpoint;

  public:
    /**
     * Constructor
     */
    DaemonRouter();

    /**
     * Destructor
     */
    ~DaemonRouter();

    /**
     * Set the AllJoynObj associated with this router.
     *
     * @param alljoynObj   The bus controller.
     */
    void SetAllJoynObj(AllJoynObj* alljoynObj) { this->alljoynObj = alljoynObj; }

    /**
     * Set the SessionlessObj associated with this router.
     *
     * @param sessionlessObj   The bus controller.
     */
    void SetSessionlessObj(SessionlessObj* sessionlessObj) { this->sessionlessObj = sessionlessObj; }

    /**
     * Set the busController associated with this router.
     *
     * @param busController   The bus controller.
     */
    void SetBusController(BusController* busController) { this->busController = busController; }

    /**
     * Get the bus controller associated with this router
     */
    BusController* GetBusController() { return busController; }

    /**
     * Add a bus name listener.
     *
     * @param listener    Pointer to object that implements AllJoynNameListerer
     */
    void AddBusNameListener(NameListener* listener) { nameTable.AddListener(listener); }

    /**
     * Remote a bus name listener.
     *
     * @param listener    Pointer to object that implements AllJoynNameListerer
     */
    void RemoveBusNameListener(NameListener* listener) { nameTable.RemoveListener(listener); }

    /**
     * Set GUID of the bus.
     *
     * @param guid   GUID of bus associated with this router.
     */
    void SetGlobalGUID(const qcc::GUID128& guid) { nameTable.SetGUID(guid); }

    /**
     * Generate a unique endpoint name.
     *
     * @return A unique bus name that can be assigned to a (server-side) endpoint.
     */
    qcc::String GenerateUniqueName(void) { return nameTable.GenerateUniqueName(); }

    /**
     * Return whether this is a unique name of a locally connected endpoint.
     *
     * @param uniqueName   Unique name to check.
     * @return  true if a locally connected endpoint has this unique name.
     */
    bool IsValidLocalUniqueName(qcc::String uniqueName) { return nameTable.IsValidLocalUniqueName(uniqueName); }
    /**
     * Add a well-known (alias) bus name.
     *
     * @param aliasName    Alias (well-known) name of bus.
     * @param uniqueName   Unique name of endpoint attempting to own aliasName.
     * @param flags        AddAlias flags from NameTable.
     * @param disposition  [OUT] Outcome of add alias operation. Valid if return code is ER_OK.
     * @param listener     Optional listener whose AddAliasComplete method will be called if return code is ER_OK.
     * @param context      Optional context passed to listener.
     * @return  ER_OK if successful;
     */
    QStatus AddAlias(const qcc::String& aliasName,
                     const qcc::String& uniqueName,
                     uint32_t flags,
                     uint32_t& disposition,
                     NameListener* listener = NULL,
                     void* context = NULL)
    {
        return nameTable.AddAlias(aliasName, uniqueName, flags, disposition, listener, context);
    }

    /**
     * Remove a well-known bus name.
     *
     * @param aliasName     Well-known name to be removed.
     * @param ownerName     Unique name of owner of aliasName.
     * @param disposition   Outcome of remove alias operation. Valid only if return code is ER_OK.
     * @param listener      Optional listener whose RemoveAliasComplete method will be called if return code is ER_OK.
     * @param context       Optional context passed to listener.
     */
    void RemoveAlias(const qcc::String& aliasName,
                     const qcc::String& ownerName,
                     uint32_t& disposition,
                     NameListener* listener = NULL,
                     void* context = NULL)
    {
        nameTable.RemoveAlias(aliasName, ownerName, disposition, listener, context);
    }

    /**
     * Get a list of bus names.
     *
     * @param names  OUT Parameter: Vector of bus names.
     */
    void GetBusNames(std::vector<qcc::String>& names) const;

    /**
     * Find the endpoint that owns the given unique or well-known name.
     *
     * @param busname    Unique or well-known bus name
     * @return Returns either the bus endpoint or an invalid bus endpoint with
     */
    BusEndpoint FindEndpoint(const qcc::String& busname);

    /**
     * Find the remote or bus-to-bus endpoint that owns the given unique or well-known name.
     *
     * @param busname    Unique or well-known bus name
     * @param endpoint   Returns the bus endpoint
     *
     * @return  Returns true if the endpoint was found, false if it was not found.
     */
    bool FindEndpoint(const qcc::String& busname, RemoteEndpoint& endpoint) {
        BusEndpoint ep = FindEndpoint(busname);
        if ((ep->GetEndpointType() == ENDPOINT_TYPE_REMOTE) || (ep->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS)) {
            endpoint = RemoteEndpoint::cast(ep);
            return true;
        } else {
            endpoint->Invalidate();
            return false;
        }
    }

    /**
     * Find the virtual endpoint that owns the given unique or well-known name.
     *
     * @param busname    Unique or well-known bus name
     * @param endpoint   Returns the bus endpoint
     *
     * @return  Returns true if the endpoint was found, false if it was not found.
     */
    bool FindEndpoint(const qcc::String& busname, VirtualEndpoint& endpoint) {
        BusEndpoint ep = FindEndpoint(busname);
        if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
            endpoint = VirtualEndpoint::cast(ep);
            return true;
        } else {
            endpoint->Invalidate();
            return false;
        }
    }

    /**
     * Add a rule for an endpoint.
     *
     * @param endpoint   The endpoint that this rule applies to.
     * @param rule       Rule for endpoint
     * @return ER_OK if successful;
     */
    QStatus AddRule(BusEndpoint& endpoint, Rule& rule);

    /**
     * Remove a rule for an endpoint.
     *
     * @param endpoint    Endpoint that rule applies to.
     * @param rule        Rule to remove.
     * @return ER_OK if   successful;
     */
    QStatus RemoveRule(BusEndpoint& endpoint, Rule& rule);

    /**
     * Remove all rules for a given endpoint.
     *
     * @param endpoint    Endpoint whose rules will be removed.
     * @return ER_OK if successful;
     */
    QStatus RemoveAllRules(BusEndpoint& endpoint) { return ruleTable.RemoveAllRules(endpoint); }

    /**
     * Route an incoming Message Bus Message from an endpoint.
     *
     * @param sender  Endpoint that is sending the message
     * @param msg     Message to be processed.
     * @return ER_OK if successful.
     */
    QStatus PushMessage(Message& msg, BusEndpoint& sender);

    /**
     * Register an endpoint.
     * This method must be called by an endpoint before attempting to use the router.
     *
     * @param endpoint   Endpoint being registered.
     */
    QStatus RegisterEndpoint(BusEndpoint& endpoint);

    /**
     * Un-register an endpoint.
     * This method must be called by an endpoint before the endpoint is deallocted.
     *
     * @param epName   Name of endpoint being unregistered.
     */
    void UnregisterEndpoint(const qcc::String& epName, EndpointType epType);

    /**
     * Return true if this router is in contact with a bus (either locally or remotely)
     * This method can be used to determine whether messages sent to "the bus" will be routed.
     *
     * @return true iff the messages can be routed currently.
     */
    bool IsBusRunning(void) const
    {
        localEndpointLock.Lock();
        bool valid = localEndpoint->IsValid();
        localEndpointLock.Unlock();
        return valid;
    }

    /**
     * Indicate that this Bus instance is an AllJoyn daemon
     *
     * @return true since DaemonRouter is always part of an AllJoyn daemon.
     */
    bool IsDaemon() const { return true; }

    /**
     * Lock name table
     */
    void LockNameTable() { nameTable.Lock(); }

    /**
     * Unlock name table
     */
    void UnlockNameTable() { nameTable.Unlock(); }

    /**
     * Get all unique names and their exportable alias (well-known) names.
     *
     * @param  nameVec   Vector of (uniqueName, aliases) pairs where aliases is a vector of alias names.
     */
    void GetUniqueNamesAndAliases(std::vector<std::pair<qcc::String, std::vector<qcc::String> > >& nameVec) const
    {
        nameTable.GetUniqueNamesAndAliases(nameVec);
    }

    /**
     * Get all the unique names that are in queue for the same alias (well-known) name
     *
     * @param[in] busName (well-known) name
     * @param[out] names vecter of uniqueNames in queue for the
     */
    void GetQueuedNames(const qcc::String& busName, std::vector<qcc::String>& names)
    {
        nameTable.GetQueuedNames(busName, names);
    }
    /**
     * Set (or clear) a virtual alias.
     * A virtual alias is a well-known bus name for a virtual endpoint.
     * Virtual aliases differ from regular aliases in that the local bus controller
     * does not handle name queueing. It is up to the remote endpoint to manange
     * the queueing for such aliases.
     *
     * @param alias        The virtual alias being modified.
     * @param newOwnerEp   The VirtualEndpoint that is the new owner of alias or NULL if none.
     * @param requestingEp A Virtual endpoint from the remote daemon that is requesting this change.
     * @return  true iff alias was a change to the name table
     */
    bool SetVirtualAlias(const qcc::String& alias, VirtualEndpoint* newOwnerEp, VirtualEndpoint& requestingEp)
    {
        return nameTable.SetVirtualAlias(alias, newOwnerEp, requestingEp);
    }

    /**
     * Remove well-known names associated with a virtual endpoint.
     *
     * @param uniqueName   UniqueName of virtual endpoint whose well-known names are to be removed.
     */
    void RemoveVirtualAliases(const qcc::String& uniqueName)
    {
        nameTable.RemoveVirtualAliases(uniqueName);
    }

    /**
     * Update propagation info of names associated with a virtual endpoint.
     *
     * @param uniqueName  UniqueName of virtual endpoint whose propagation info is to be updated.
     */
    void UpdateVirtualAliases(const qcc::String& uniqueName)
    {
        nameTable.UpdateVirtualAliases(uniqueName);
    }

    /**
     * Add a session route.
     *
     * @param  id          Session Id.
     * @param  srcEp       Route source endpoint.
     * @param  srcB2bEp    Source B2B endpoint. (NULL if srcEp is not virtual).
     * @param  destEp      BusEndpoint of route destination.
     * @param  destB2bEp   [IN/OUT] If passed in as invalid endpoint type, attempt to use optsHint to choose destB2bEp and return selected ep.
     * @return  ER_OK if successful.
     */
    QStatus AddSessionRoute(SessionId id, BusEndpoint& srcEp, RemoteEndpoint* srcB2bEp, BusEndpoint& destEp,
                            RemoteEndpoint& destB2bEp);

    /**
     * Remove existing session routes.  This method removes routes
     * that involve uniqueName as a source or as a destination for a
     * particular session id.
     *
     * @param  uniqueName  Unique name.
     * @param  id          Session id.
     */
    void RemoveSessionRoutes(const char* uniqueName, SessionId id);

    /**
     * Remove existing session routes.  This method removes routes for
     * all endpoints associated with a particular session id.
     *
     * @param  id          Session id.
     */
    void RemoveSessionRoutes(SessionId id);

    /**
     * Remove existing session routes.
     * This method removes routes that involve endpoint as a source or as a destination for all session ids.
     *
     * @param  endpoint    Endpoint to be removed.
     */
    void RemoveSessionRoutesForEndpoint(BusEndpoint& ep);

    /**
     * Remove self-join related session-route.
     *
     * @param  uniqueName  Unique name.
     * @param  id          Session id or 0 to indicate "all sessions".
     *
     */
    void RemoveSelfJoinSessionRoute(const char* src, SessionId id);
    /**
     * Return the routing rule table.
     *
     * @return the routing rule table.
     */
    RuleTable& GetRuleTable() { return ruleTable; }

  private:
    LocalEndpoint localEndpoint;          /**< The local endpoint */
    mutable qcc::Mutex localEndpointLock; /**< Mutex to protect localEndpoint modification. */
    RuleTable ruleTable;                  /**< Routing rule table */
    NameTable nameTable;                  /**< BusName to transport lookupl table */
    BusController* busController;         /**< The bus controller used with this router */
    AllJoynObj* alljoynObj;               /**< AllJoyn bus object used with this router */
    SessionlessObj* sessionlessObj;       /**< Sessionless bus object used with this router */

    std::set<RemoteEndpoint> m_b2bEndpoints; /**< Collection of Bus-to-bus endpoints */
    qcc::Mutex m_b2bEndpointsLock;           /**< Lock that protects m_b2bEndpoints */

    typedef std::map<BusEndpoint, uint8_t> SessionEps;    /**< Set of endpoints in a session (with a flag bit field) */
    typedef std::map<SessionId, SessionEps> SessionMap;   /**< Map of session IDs to sets endpoints */
    SessionMap sessionMap;      /**< Provide a lookup table of which endpoints are members of which session.*/
    qcc::Mutex sessionMapLock;  /**< Lock that protects the session map. */

    /* Add a session ref to the virtualendpoint with the specified name
     * @param  vepName: Name of virtual endpoint to which a ref needs to be added.
     * @param  id: Id of the session
     * @param  b2bEp: B2b endpoint of the session
     */
    QStatus AddSessionRef(qcc::String vepName, SessionId id, RemoteEndpoint b2bEp);

    /* Remove a session ref to the virtualendpoint with the specified name
     * @param  vepName: Name of virtual endpoint to which a ref needs to be decremented.
     * @param  id: Id of the session
     */
    void RemoveSessionRef(qcc::String vepName, SessionId id);

    /**
     * Helper function to determine if a message can be delivered over a given
     * session from the source to the destination.
     *
     * @param id    Session ID
     * @param src   Source endpoint
     * @param dest  Destination endpoint
     *
     * @return  true iff message can be delivered.
     */
    bool IsSessionDeliverable(SessionId id, BusEndpoint& src, BusEndpoint& dest);


#ifdef ENABLE_OLD_PUSHMESSAGE_COMPATABILITY
    /**
     * This function adapts the decision about which endpoints receive a given
     * message based on the behavior of the previous implementation of
     * PushMessage().  The previous version of PushMessage() exhibited some odd
     * behaviors for certain corner cases.  So rather that burying the special
     * case code in the new version of PushMessage(), the special case code is
     * collected into a separate function to make it easier to find the code to
     * remove when the old behavior is no longer required.
     *
     * @param add               decision to send to dest before override
     * @param src               source endpoint
     * @param dest              destination endpoint
     * @param sessionId         session ID
     * @param isBroadcast       indicates if message is a broadcast message
     * @param isSessionCast     indicates if message is a sessioncast message
     * @param isSessionless     indicates if message is a sessionless message
     * @param isGlobalBroadcast indicates if message is a global broadcast message
     * @param detachId          session ID for DetachSession message (0 otherwise)
     *
     * @return  New valued for add.
     */
    bool AddCompatabilityOverride(bool add,
                                  BusEndpoint& src,
                                  BusEndpoint& dest,
                                  const SessionId sessionId,
                                  const bool isBroadcast,
                                  const bool isSessioncast,
                                  const bool isSessionless,
                                  const bool isGlobalBroadcast,
                                  const SessionId detachId);

    /**
     * This function alters the resulting status code from PushMessage() so that
     * its behavior is closer to that of the previous implementation of
     * PushMessage().  Again, this handle certain corner cases where the
     * original version of PushMessage() exhibited inconsistent behavior.
     *
     * @param status            return status code before override
     * @param src               source endpoint
     * @param isSessionCast     indicates if message is a sessioncast message
     * @param isSessionless     indicates if message is a sessionless message
     * @param policyRejected    indicates if PolicyDB rules prohibit message delivery
     *
     * @return  New return status value.
     */
    QStatus StatusCompatabilityOverride(QStatus status,
                                        BusEndpoint& src,
                                        const bool isSessioncast,
                                        const bool isSessionless,
                                        const bool policyRejected);
#endif
};

}

#endif
