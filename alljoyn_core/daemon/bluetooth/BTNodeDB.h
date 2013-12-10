/**
 * @file
 * BusObject responsible for controlling/handling Bluetooth delegations.
 */

/******************************************************************************
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
#ifndef _ALLJOYN_BTNODEDB_H
#define _ALLJOYN_BTNODEDB_H

#include <qcc/platform.h>

#include <limits>
#include <set>
#include <vector>

#include <qcc/ManagedObj.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/time.h>

#include "BDAddress.h"
#include "BTBusAddress.h"
#include "BTNodeInfo.h"


namespace ajn {

/** Bluetooth Node Database */
class BTNodeDB {
  public:
    /** Convenience iterator typedef. */
    typedef std::set<BTNodeInfo>::iterator iterator;

    /** Convenience const_iterator typedef. */
    typedef std::set<BTNodeInfo>::const_iterator const_iterator;


    BTNodeDB(bool useExpirations = false) : useExpirations(useExpirations) { }

    /**
     * Find a node given a Bluetooth device address and a PSM.
     *
     * @param addr  Bluetooth device address
     * @param psm   L2CAP PSM for the AllJoyn service
     *
     * @return  BTNodeInfo of the found node (BTNodeInfo::IsValid() will return false if not found)
     */
    const BTNodeInfo FindNode(const BDAddress& addr, uint16_t psm) const { BTBusAddress busAddr(addr, psm); return FindNode(busAddr); }

    /**
     * Find a node given a bus address.
     *
     * @param addr  bus address
     *
     * @return  BTNodeInfo of the found node (BTNodeInfo::IsValid() will return false if not found)
     */
    const BTNodeInfo FindNode(const BTBusAddress& addr) const;

    /**
     * Find a node given a unique name of the daemon running on a node.
     *
     * @param uniqueName    unique name of the daemon running on a node
     *
     * @return  BTNodeInfo of the found node (BTNodeInfo::IsValid() will return false if not found)
     */
    const BTNodeInfo FindNode(const qcc::String& uniqueName) const;

    /**
     * Find the first node with given a Bluetooth device address.  (Generally,
     * the bluetooth device address should be unique, but it is not completely
     * impossible for 2 instances for AllJoyn to be running on one physical
     * device with the same Bluetooth device address but with different PSMs.)
     *
     * @param addr  Bluetooth device address
     *
     * @return  BTNodeInfo of the found node (BTNodeInfo::IsValid() will return false if not found)
     */
    const BTNodeInfo FindNode(const BDAddress& addr) const;

    void FindNodes(const BDAddress& addr, const_iterator& begin, const_iterator& end)
    {
        BTBusAddress lower(addr, 0x0000);
        BTBusAddress upper(addr, 0xffff);
        Lock(MUTEX_CONTEXT);
        begin = nodes.lower_bound(lower);
        end = nodes.upper_bound(upper);
        Unlock(MUTEX_CONTEXT);
    }


    /**
     * Find a minion starting with the specified start node in the set of
     * nodes, and skipping over the skip node.  If any nodes (beside start and
     * skip) are EIR capable, those nodes will be selected.  Non-EIR capable
     * nodes will only be considered if eirCapable is false and there are no
     * EIR capable nodes beyond start and skip.
     *
     * @param start         Node in the DB to use as a starting point for the search
     * @param skip          Node in the DB to skip over if next in line
     * @param eirCapable    Flag to indicate if only EIR capable nodes should be considered
     *
     * @return  BTNodeInfo of the next delegate minion.  Will be the same as start if none found.
     */
    BTNodeInfo FindDelegateMinion(const BTNodeInfo& start, const BTNodeInfo& skip, bool eirCapable) const;

    /**
     * Add a node to the DB with no expiration time.
     *
     * @param node  Node to be added to the DB.
     */
    void AddNode(const BTNodeInfo& node);

    /**
     * Remove a node from the DB.
     *
     * @param node  Node to be removed from the DB.
     */
    void RemoveNode(const BTNodeInfo& node);

    /**
     * Determine the difference between this DB and another DB.  Nodes that
     * appear in only one or the other DB will be copied (i.e., share the same
     * referenced data) to the added/removed DBs as appropriate.  Nodes that
     * appear in both this and the other DB but have differences in their set
     * of advertised names will result in fully independed copies of the node
     * information with only the appropriate name changes being put in the
     * added/removed DBs as appropriate.  It is possible for a node to appear
     * in both the added DB and removed DB if that node had advertised names
     * both added and removed.
     *
     * @param other         Other DB for comparision
     * @param added[out]    If non-null, the set of nodes (and names) found in
     *                      other but not in us
     * @param removed[out]  If non-null, the set of nodes (and names) found in
     *                      us but not in other
     */
    void Diff(const BTNodeDB& other, BTNodeDB* added, BTNodeDB* removed) const;

    /**
     * Determine the difference between this DB and another DB in terms of
     * nodes only.  In other words, nodes in this DB that do not appear in the
     * other DB will be copied into the removed DB while nodes in the other DB
     * that do not appear in this DB will be copied to the added DB.
     * Differences in names will be ignored.
     *
     * @param other         Other DB for comparision
     * @param added[out]    If non-null, the set of nodes found in other but not in us
     * @param removed[out]  If non-null, the set of nodes found in us but not in other
     */
    void NodeDiff(const BTNodeDB& other, BTNodeDB* added, BTNodeDB* removed) const;

    /**
     * Applies the differences found in BTNodeDB::Diff to us.
     *
     * @param added         If non-null, nodes (and names) to add
     * @param removed       If non-null, name to remove from nodes
     * @param removeNodes   Optional parameter that defaults to true
     *                      - true: remove nodes that become empty due to all
     *                              names being removed
     *                      - false: keep empty nodes
     */
    void UpdateDB(const BTNodeDB* added, const BTNodeDB* removed, bool removeNodes = true);

    /**
     * Removes the expiration time of all nodes (sets expiration to end-of-time).
     */
    void RemoveExpiration();

    /**
     * Updates the expiration time of all nodes.
     *
     * @param expireDelta   Number of milliseconds from now to set the
     *                      expiration time.
     */
    void RefreshExpiration(uint32_t expireDelta);

    /**
     * Updates the expiration time of all nodes that may be connected to via
     * connAddr.
     *
     * @param connNode      Node accepting connections on behalf of other nodes.
     * @param expireDelta   Number of milliseconds from now to set the
     *                      expiration time.
     */
    void RefreshExpiration(const BTNodeInfo& connNode, uint32_t expireDelta);

    /**
     * Fills a BTNodeDB with the set of nodes that are connectable via
     * connNode.
     *
     * @param connNode  BTNodeInfo of the device accepting connections on
     *                  behalf of other nodes.
     * @param subDB     Sub-set BTNodeDB to store the found nodes in.
     */
    void GetNodesFromConnectNode(const BTNodeInfo& connNode, BTNodeDB& subDB) const
    {
        Lock(MUTEX_CONTEXT);
        for (std::set<BTNodeInfo>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ((*it)->GetConnectNode() == connNode) {
                subDB.AddNode(*it);
            }
        }
        Unlock(MUTEX_CONTEXT);
    }

    void PopExpiredNodes(BTNodeDB& expiredDB)
    {
        Lock(MUTEX_CONTEXT);
        qcc::Timespec now;
        qcc::GetTimeNow(&now);
        std::set<BTNodeInfo>::iterator it = nodes.begin();
        while (it != nodes.end()) {
            BTNodeInfo node = *it;
            if (node->GetExpireTime() <= now.GetAbsoluteMillis()) {
                nodes.erase(node);
                expiredDB.AddNode(node);
                it = nodes.begin();
            } else {
                ++it;
            }
        }
        Unlock(MUTEX_CONTEXT);
    }

    uint64_t NextNodeExpiration()
    {
        uint64_t next = std::numeric_limits<uint64_t>::max();
        for (std::set<BTNodeInfo>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            BTNodeInfo node = *it;
            if (node->GetExpireTime() < next) {
                next = node->GetExpireTime();
            }
        }
        return next;
    }


    void NodeSessionLost(SessionId sessionID);
    void UpdateNodeSessionID(SessionId sessionID, const BTNodeInfo& node);

    /**
     * Lock the mutex that protects the database from unsafe access.
     */
    void Lock(const char* file, uint32_t line) const { lock.Lock(file, line); }
    void Lock() const { lock.Lock(MUTEX_CONTEXT); }

    /**
     * Release the the mutex that protects the database from unsafe access.
     */
    void Unlock(const char* file, uint32_t line) const { lock.Unlock(file, line); }
    void Unlock() const { lock.Unlock(MUTEX_CONTEXT); }

    /**
     * Get the begin iterator for the set of nodes.
     *
     * @return  const_iterator pointing to the first node
     */
    const_iterator Begin() const { return nodes.begin(); }

    /**
     * Get the end iterator for the set of nodes.
     *
     * @return  const_iterator pointing to one past the last node
     */
    const_iterator End() const { return nodes.end(); }

    /**
     * Get the number of entries in the node DB.
     *
     * @return  the number of entries in the node DB
     */
    size_t Size() const
    {
        Lock(MUTEX_CONTEXT);
        size_t size = nodes.size();
        Unlock(MUTEX_CONTEXT);
        return size;
    }

    /**
     * Clear out the DB.
     */
    void Clear() { nodes.clear(); }

#ifndef NDEBUG
    void DumpTable(const char* info) const;
#else
    void DumpTable(const char* info) const { }
#endif


  private:

    BTNodeDB(const BTNodeDB& other) : useExpirations(false) { }
    BTNodeDB& operator=(const BTNodeDB& other) { return *this; }

    std::set<BTNodeInfo> nodes;     /**< The node DB storage. */

    mutable qcc::Mutex lock;        /**< Mutext to protect the DB. */

    const bool useExpirations;
};

} // namespace ajn

#endif
