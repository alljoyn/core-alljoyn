/**
 * @file
 * DaemonRouter is a "full-featured" router responsible for routing Bus messages
 * between one or more remote endpoints and a single local endpoint.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/Thread.h>

#include "Transport.h"

#include <alljoyn/Status.h>

#include "LocalTransport.h"
#include "Router.h"
#include "NameTable.h"
#include "RuleTable.h"

namespace ajn {


/**
 * @internal Forward delcarations
 */
class BusController;

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
    bool IsBusRunning(void) const { return localEndpoint->IsValid(); }

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
     * @param  optsHint    Optional session options constraint for selection of destB2bEp if not explicitly specified.
     * @return  ER_OK if successful.
     */
    QStatus AddSessionRoute(SessionId id, BusEndpoint& srcEp, RemoteEndpoint* srcB2bEp, BusEndpoint& destEp,
                            RemoteEndpoint& destB2bEp, SessionOpts* optsHint = NULL);

    /**
     * Remove existing session routes.
     * This method removes routes that involve uniqueName as a source or as a destination for a particular session id.
     * When sessionId is 0, all routes that involved uniqueName are removed.
     *
     * @param  uniqueName  Unique name.
     * @param  id          Session id or 0 to indicate "all sessions".
     */
    void RemoveSessionRoutes(const char* uniqueName, SessionId id);

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
    LocalEndpoint localEndpoint;    /**< The local endpoint */
    RuleTable ruleTable;            /**< Routing rule table */
    NameTable nameTable;            /**< BusName to transport lookupl table */
    BusController* busController;   /**< The bus controller used with this router */

    std::set<RemoteEndpoint> m_b2bEndpoints; /**< Collection of Bus-to-bus endpoints */
    qcc::Mutex m_b2bEndpointsLock;           /**< Lock that protects m_b2bEndpoints */

    /** Session multicast destination map */
    struct SessionCastEntry {
        SessionId id;
        qcc::String src;
        RemoteEndpoint b2bEp;
        BusEndpoint destEp;

        SessionCastEntry(SessionId id, const qcc::String& src) :
            id(id), src(src) { }

        SessionCastEntry(SessionId id, const qcc::String& src, RemoteEndpoint& b2bEp, BusEndpoint& destEp) :
            id(id), src(src), b2bEp(b2bEp), destEp(destEp) { }

        bool operator<(const SessionCastEntry& other) const {
            /* The order of comparison of src and id has been reversed, so that upper_bound can be
             * used with (desiredID -1) to obtain the first entry that has the desired src and id.
             */
            return (src < other.src) || ((src == other.src) && ((id < other.id) || ((id == other.id) && ((b2bEp < other.b2bEp) || ((b2bEp == other.b2bEp) && (destEp < other.destEp))))));

        }

        bool operator==(const SessionCastEntry& other) const {
            return (id == other.id)  && (src == other.src) && (b2bEp == other.b2bEp) && (destEp == other.destEp);
        }

        qcc::String ToString() const {
            char idbuf[16];
            char ptrbuf[16];
            qcc::String str;
            str.append("id: ");
            snprintf(idbuf, sizeof(idbuf), "%u", id);
            str.append(idbuf);

            str.append(",src: ");
            str.append(src);

            str.append(", remote: ");
            snprintf(ptrbuf, sizeof(ptrbuf), "%p", b2bEp.unwrap());
            str.append(ptrbuf);
            str.append("(");
            str.append(b2bEp->GetUniqueName());
            str.append(",");
            str.append(b2bEp->GetRemoteName());
            str.append(")");

            str.append(", dest: ");
            snprintf(ptrbuf, sizeof(ptrbuf), "%p", destEp.unwrap());
            str.append(ptrbuf);
            str.append(destEp->GetUniqueName());

            return str;
        }
    };

    std::set<SessionCastEntry> sessionCastSet; /**< Session multicast set */
    qcc::Mutex sessionCastSetLock;             /**< Lock that protects sessionCastSet */
};

}

#endif
