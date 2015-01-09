/**
 * @file
 * NameTable is a thread-safe mapping between unique/well-known
 * bus names and the Transport that these names exist on.
 */

/******************************************************************************
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_NAMETABLE_H
#define _ALLJOYN_NAMETABLE_H

#include <qcc/platform.h>

#include <deque>
#include <vector>
#include <set>

#include <qcc/Mutex.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>

#include <alljoyn/Status.h>

#include "BusEndpoint.h"
#include "VirtualEndpoint.h"

#include <qcc/STLContainer.h>

namespace ajn {

/** @internal Forward reference */
class NameListener;

/**
 * NameTable is a thread-safe mapping between unique/well-known
 * bus names and the BusEndpoint that these names exist on.
 * This mapping is many (names) to one (endpoint). Every endpoint has
 * exactly one unique name and zero or more well-known names.
 */
class NameTable {
  public:

    /**
     * AddAlias complete callback
     */
    typedef void (*AddAliasComplete)(qcc::String& busName, uint32_t disposition, void* context);

    /**
     * RemoveAlias complete callback
     */
    typedef void (*RemoveAliasComplete)(qcc::String& busName, uint32_t disposition, void* context);

    /**
     * Constructor
     */
    NameTable() : uniqueId(0), uniquePrefix(":1.") { }

    /**
     * Set the GUID of the bus.
     * This unique bus names are assigned using the bus guid as a prefix. This
     * ensures that AllJoyn endpoints are globally unique.
     *
     * @param guid   The bus guid.
     */
    void SetGUID(const qcc::GUID128& guid);

    /**
     * Register a listener that will be called whenever ownership of a bus name
     * (unique or well-known) changes.
     *
     * @param listener   Listener to be notified
     */
    void AddListener(NameListener* listener);

    /**
     * Un-Register a listener that was previously registered with AddListener.
     *
     * @param listener   Listener to be notified
     */
    void RemoveListener(NameListener* listener);

    /**
     * Generate a unique bus name.
     *
     * @return A unique name string that can be assigned to a msgbus endpoint.
     */
    qcc::String GenerateUniqueName(void);

    /**
     * Add an endpoint and it's unique name to the name table.
     *
     * @param endpoint      Endpoint whose unique name will be added to the name table.
     */
    void AddUniqueName(BusEndpoint& endpoint);

    /**
     * Release a unique name and any well-known names associated with the endpoint.
     *
     * @param uniqueName    Unique name.
     */
    void RemoveUniqueName(const qcc::String& uniqueName);

    /**
     * Add a well-known (alias) bus name.
     *
     * @param aliasName      Alias (well-known) name of bus.
     * @param uniqueName     Unique name of endpoint attempting to own aliasName.
     * @param flags          AliasFlags associated with alias request.
     * @param disposition    [OUT] Outcome of add alias. Only valid if return status is ER_OK.
     * @param listener       Optional listener whose AddAliasComplete callback will be called if return code is ER_OK.
     * @param context        Optional context passed to AddAliasComplete callback.
     * @return  ER_OK if successful;
     */
    QStatus AddAlias(const qcc::String& aliasName,
                     const qcc::String& uniqueName,
                     uint32_t flags,
                     uint32_t& disposition,
                     NameListener* listener = NULL,
                     void* context = NULL);

    /**
     * Remove a well-known bus name.
     *
     * @param aliasName    Well-known name to remove.
     * @param ownerName    Unique name of busName owner.
     * @param disposition  [OUT] Outcome of remove alias operation. Only valid if return is ER_OK.
     * @param listener     Optional listener whose RemoveAliasComplete callback will be called if return code is ER_OK.
     * @param context      Optional context passed to RemoveAliasComplete callback.
     */
    void RemoveAlias(const qcc::String& aliasName,
                     const qcc::String& ownerName,
                     uint32_t& disposition,
                     NameListener* listener = NULL,
                     void* context = NULL);

    /**
     * Set a virtual alias.
     * A virtual alias is a well-known bus name for a virtual endpoint.
     * Virtual aliases differ from regular aliases in that the local bus controller
     * does not handle name queueing. It is up to the remote endpoint to manange
     * the queueing for such aliases.
     *
     * @param alias               The virtual alias being modified.
     * @param newownerEp          The VirtualEndpoint that is the new owner of alias or NULL if none.
     * @param requestingEndpoint  The VirtualEndpoint that is requesting the change
     * @return  true if this request caused changes to the name table.
     */
    bool SetVirtualAlias(const qcc::String& alias,
                         VirtualEndpoint* newOwnerEp,
                         VirtualEndpoint& requestingEndpoint);

    /**
     * Remove well-known names associated with a virtual endpoint.
     *
     * @param uniqueName  UniqueName of virtual endpoint whose well-known names are to be removed.
     */
    void RemoveVirtualAliases(const qcc::String& uniqueName);

    /**
     * Update propagation info of names associated with a virtual endpoint.
     *
     * @param uniqueName  UniqueName of virtual endpoint whose propagation info is to be updated.
     */
    void UpdateVirtualAliases(const qcc::String& uniqueName);

    /**
     * Find an endpoint for a given unique or alias bus name.
     *
     * @param busName   Name of bus.
     * @return  Returns the endpoint if it was found or an invalid endpoint if not found
     */
    BusEndpoint FindEndpoint(const qcc::String& busName) const;

    /**
     * Return whether this is a unique name of a locally connected endpoint.
     *
     * @param uniqueName   Unique name to check.
     * @return  true if a locally connected endpoint has this unique name.
     */
    bool IsValidLocalUniqueName(const qcc::String& uniqueName) const;

    /**
     * Get all bus names from name table.
     *
     * @param[out] names Vector of names.
     */
    void GetBusNames(std::vector<qcc::String>& names) const;

    /**
     * Get all unique names and their alias (well-known) names.
     *
     * @param[out]  nameVec   Vector of (uniqueName, aliases) pairs where aliases is a vector of alias names.
     */
    void GetUniqueNamesAndAliases(std::vector<std::pair<qcc::String, std::vector<qcc::String> > >& nameVec) const;

    /**
     * Get all the unique names that are in queue for the same alias (well-known) name
     *
     * @param[in] busName (well-known) name
     * @param[out] names vecter of uniqueNames in queue for the
     */
    void GetQueuedNames(const qcc::String& busName, std::vector<qcc::String>& names);

    /**
     * Lock table.
     */
    void Lock() { lock.Lock(MUTEX_CONTEXT); }

    /**
     * Lock table.
     */
    void Unlock() { lock.Unlock(MUTEX_CONTEXT); }

  private:
    typedef struct {
        BusEndpoint endpoint;
        SessionOpts::NameTransferType nameTransfer;
    }  UniqueNameEntry;

    typedef struct {
        qcc::String endpointName;
        uint32_t flags;
    } NameQueueEntry;

    typedef struct {
        VirtualEndpoint endpoint;
        SessionOpts::NameTransferType nameTransfer;
    }  VirtualAliasEntry;

    /**
     * Hash functor
     */
    struct Hash {
        inline size_t operator()(const qcc::String& s) const {
            return qcc::hash_string(s.c_str());
        }
    };

    struct Equal {
        inline bool operator()(const qcc::String& s1, const qcc::String& s2) const {
            return s1 == s2;
        }
    };

    mutable qcc::Mutex lock;                                             /**< Lock protecting name tables */
    std::unordered_map<qcc::String, UniqueNameEntry, Hash, Equal> uniqueNames;   /**< Unique name table */
    std::unordered_map<qcc::String, std::deque<NameQueueEntry>, Hash, Equal> aliasNames;  /**< Alias name table */
    uint32_t uniqueId;
    qcc::String uniquePrefix;

    typedef qcc::ManagedObj<NameListener*> ProtectedNameListener;
    std::set<ProtectedNameListener> listeners;                         /**< Listeners regsitered with name table */
    std::map<qcc::StringMapKey, VirtualAliasEntry> virtualAliasNames;    /**< map of virtual aliases to virtual endpts */

    /**
     * Returns the minimum name transfer value for sessions with the endpoint.
     *
     * @param ep the endpoint
     */
    SessionOpts::NameTransferType GetNameTransfer(BusEndpoint& ep);

    /**
     * Returns the minimum name transfer value for sessions with the endpoint.
     *
     * @param vep the virtual endpoint
     */
    SessionOpts::NameTransferType GetNameTransfer(const VirtualEndpoint& vep);

    /**
     * Helper used to call the listners
     *
     * @param alias Well-known bus name now owned by listener.
     * @param oldOwner Unique name of old owner of alias or NULL if none existed.
     * @param oldOwnerNameTransfer Whether the old owner should be propagated (ALL_NAMES) or not (DAEMON_NAMES).
     * @param newOwner Unique name of new owner of alias or NULL if none (now) exists.
     * @param newOwnerNameTransfer Whether the new owner should be propagated (ALL_NAMES) or not (DAEMON_NAMES).
     */
    void CallListeners(const qcc::String& aliasName,
                       const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                       const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);
};

/**
 * AllJoynNameListeners are notified by the NameTable when message
 * bus name events interest occur.
 */
class NameListener {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~NameListener() { }

    /**
     * Called when a bus name changes ownership.
     *
     * @param alias Well-known bus name now owned by listener.
     * @param oldOwner Unique name of old owner of alias or NULL if none existed.
     * @param oldOwnerNameTransfer Whether the old owner should be propagated (ALL_NAMES) or not (DAEMON_NAMES).
     * @param newOwner Unique name of new owner of alias or NULL if none (now) exists.
     * @param newOwnerNameTransfer Whether the new owner should be propagated (ALL_NAMES) or not (DAEMON_NAMES).
     */
    virtual void NameOwnerChanged(const qcc::String& alias,
                                  const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                                  const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer) = 0;

    /**
     * Called upon completion of AddAlias call.
     * This method is guaranteed to be called BEFORE any "owner changed" or other asynchronous callback
     * that was triggered by the AddAlias call.
     *
     * @param aliasName    Name of alias
     * @param disposition  Disposition of aliasName as a result of AddAlias call.
     * @param context      Context passed to AddAlias.
     */
    virtual void AddAliasComplete(const qcc::String& aliasName,
                                  uint32_t disposition,
                                  void* context) { }

    /**
     * Called upon completion of RemoveAlias call.
     * This method is guaranteed to be called BEFORE any "owner changed" or other asynchronous callback
     * that was triggered by the RemoveAlias call.
     *
     * @param aliasName    Name of alias
     * @param disposition  Disposition of aliasName as a result of AddAlias call.
     * @param context      Context passed to AddAlias.
     */
    virtual void RemoveAliasComplete(const qcc::String& aliasName,
                                     uint32_t disposition,
                                     void* context) { }
};

}

#endif
