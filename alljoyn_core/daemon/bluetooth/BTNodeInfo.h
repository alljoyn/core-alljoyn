/**
 * @file
 * Bluetooth device information class definition.
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
#ifndef _ALLJOYN_BTNODEINFO_H
#define _ALLJOYN_BTNODEINFO_H

#include <qcc/platform.h>

#include <limits>
#include <set>

#include <qcc/GUID.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/Session.h>

#include "BTBusAddress.h"


namespace ajn {

/** Convenience typedef for a set of name strings. */
typedef std::set<qcc::String> NameSet;

/** Forward declaration of _BTNodeInfo. */
class _BTNodeInfo;

/** Typedef for BTNodeInfo ManagedObj. */
typedef qcc::ManagedObj<_BTNodeInfo> BTNodeInfo;

/** Class containing information about Bluetooth nodes. */
class _BTNodeInfo {

  public:
    enum NodeRelationships {
        UNAFFILIATED,
        SELF,
        DIRECT_MINION,
        INDIRECT_MINION,
        MASTER
    };

    enum SessionState {
        NO_SESSION,
        JOINING_SESSION,
        SESSION_UP
    };

    #define BTNODEINFO_INVALID_GUID "dd464c6f2163464db492d8e5180519b9"

    /**
     * Default constructor.
     */
    _BTNodeInfo() :
        guid(BTNODEINFO_INVALID_GUID),
        uniqueName(),
        nodeAddr(),
        relationship(UNAFFILIATED),
        connectProxyNode(NULL),
        uuidRev(bt::INVALID_UUIDREV),
        expireTime(std::numeric_limits<uint64_t>::max()),
        eirCapable(false),
        connectionCount(0),
        sessionID(0),
        sessionState(NO_SESSION)
    { }

    /**
     * Construct that initializes certain information.
     *
     * @param nodeAddr      Bus address of the node
     */
    _BTNodeInfo(const BTBusAddress& nodeAddr) :
        guid(BTNODEINFO_INVALID_GUID),
        uniqueName(),
        nodeAddr(nodeAddr),
        relationship(UNAFFILIATED),
        connectProxyNode(NULL),
        uuidRev(bt::INVALID_UUIDREV),
        expireTime(std::numeric_limits<uint64_t>::max()),
        eirCapable(false),
        connectionCount(0),
        sessionID(0),
        sessionState(NO_SESSION)
    { }

    /**
     * Construct that initializes certain information.
     *
     * @param nodeAddr      Bus address of the node
     * @param uniqueName    Unique bus name of the daemon on the node
     */
    _BTNodeInfo(const BTBusAddress& nodeAddr, const qcc::String& uniqueName) :
        guid(BTNODEINFO_INVALID_GUID),
        uniqueName(uniqueName),
        nodeAddr(nodeAddr),
        relationship(UNAFFILIATED),
        connectProxyNode(NULL),
        uuidRev(bt::INVALID_UUIDREV),
        expireTime(std::numeric_limits<uint64_t>::max()),
        eirCapable(false),
        connectionCount(0),
        sessionID(0),
        sessionState(NO_SESSION)
    { }

    /**
     * Construct that initializes certain information.
     *
     * @param nodeAddr      Bus address of the node
     * @param uniqueName    Unique bus name of the daemon on the node
     * @param guid          Bus GUID of the node
     */
    _BTNodeInfo(const BTBusAddress& nodeAddr, const qcc::String& uniqueName, const qcc::GUID128& guid) :
        guid(guid),
        uniqueName(uniqueName),
        nodeAddr(nodeAddr),
        relationship(UNAFFILIATED),
        connectProxyNode(NULL),
        uuidRev(bt::INVALID_UUIDREV),
        expireTime(std::numeric_limits<uint64_t>::max()),
        eirCapable(false),
        connectionCount(0),
        sessionID(0),
        sessionState(NO_SESSION)
    { }

    /**
     * Destructor.
     */
    ~_BTNodeInfo() { if (connectProxyNode) { delete connectProxyNode; } }

    /**
     * Check is the node information is valid.
     *
     * @return  true if the node information valid, false otherwise
     */
    bool IsValid() const { return nodeAddr.IsValid(); }

    /**
     * Get the begin iterator for the advertise name set.
     *
     * @return  const_iterator pointing to the begining of the advertise name set
     */
    NameSet::const_iterator GetAdvertiseNamesBegin() const { return adNames.begin(); }

    /**
     * Get the end iterator for the advertise name set.
     *
     * @return  const_iterator pointing to the end of the advertise name set
     */
    NameSet::const_iterator GetAdvertiseNamesEnd() const { return adNames.end(); }

    /**
     * Get the iterator pointing to the specified name for the advertise name set.
     *
     * @param name  Name of the advertise name to find
     *
     * @return  const_iterator pointing to the specified name or the end of the advertise name set
     */
    NameSet::const_iterator FindAdvertiseName(const qcc::String& name) const { return adNames.find(name); }

    /**
     * Get the begin iterator for the advertise name set.
     *
     * @return  const_iterator pointing to the begining of the advertise name set
     */
    NameSet::iterator GetAdvertiseNamesBegin() { return adNames.begin(); }

    /**
     * Get the end iterator for the advertise name set.
     *
     * @return  const_iterator pointing to the end of the advertise name set
     */
    NameSet::iterator GetAdvertiseNamesEnd() { return adNames.end(); }

    /**
     * Get the iterator pointing to the specified name for the advertise name set.
     *
     * @param name  Name of the advertise name to find
     *
     * @return  const_iterator pointing to the specified name or the end of the advertise name set
     */
    NameSet::iterator FindAdvertiseName(const qcc::String& name) { return adNames.find(name); }

    /**
     * Get the number of entries in the advertise name set.
     *
     * @return  the number of entries in the advertise name set
     */
    size_t AdvertiseNamesSize() const { return adNames.size(); }

    /**
     * Check if the advertise name set is empty
     *
     * @return  true if the advertise name set is empty, false otherwise
     */
    bool AdvertiseNamesEmpty() const { return adNames.empty(); }

    /**
     * Add a name to the advertise name set.
     *
     * @param name  Name to add to the advertise name set
     */
    void AddAdvertiseName(const qcc::String& name) { adNames.insert(name); }

    /**
     * Remove a name from the advertise name set.
     *
     * @param name  Name to remove from the advertise name set
     */
    void RemoveAdvertiseName(const qcc::String& name) { adNames.erase(name); }

    /**
     * Remove a name from the advertise name set referenced by an iterator.
     *
     * @param it    Reference to the name to remove from the advertise name set
     */
    void RemoveAdvertiseName(NameSet::iterator& it) { adNames.erase(it); }


    /**
     * Get the begin iterator for the find name set.
     *
     * @return  const_iterator pointing to the begining of the find name set
     */
    NameSet::const_iterator GetFindNamesBegin() const { return findNames.begin(); }

    /**
     * Get the end iterator for the find name set.
     *
     * @return  const_iterator pointing to the end of the find name set
     */
    NameSet::const_iterator GetFindNamesEnd() const { return findNames.end(); }

    /**
     * Get the iterator pointing to the specified name for the find name set.
     *
     * @param name  Name of the find name to find
     *
     * @return  const_iterator pointing to the specified name or the end of the find name set
     */
    NameSet::const_iterator FindFindName(const qcc::String& name) const { return findNames.find(name); }

    /**
     * Get the begin iterator for the find name set.
     *
     * @return  const_iterator pointing to the begining of the find name set
     */
    NameSet::iterator GetFindNamesBegin() { return findNames.begin(); }

    /**
     * Get the end iterator for the find name set.
     *
     * @return  const_iterator pointing to the end of the find name set
     */
    NameSet::iterator GetFindNamesEnd() { return findNames.end(); }

    /**
     * Get the iterator pointing to the specified name for the find name set.
     *
     * @param name  Name of the find name to find
     *
     * @return  const_iterator pointing to the specified name or the end of the find name set
     */
    NameSet::iterator FindFindName(const qcc::String& name) { return findNames.find(name); }

    /**
     * Get the number of entries in the findadvertise name set.
     *
     * @return  the number of entries in the find name set
     */
    size_t FindNamesSize() const { return findNames.size(); }

    /**
     * Check if the find name set is empty
     *
     * @return  true if the find name set is empty, false otherwise
     */
    bool FindNamesEmpty() const { return findNames.empty(); }

    /**
     * Add a name to the find name set.
     *
     * @param name  Name to add to the find name set
     */
    void AddFindName(const qcc::String& name) { findNames.insert(name); }

    /**
     * Remove a name from the find name set.
     *
     * @param name  Name to remove from the find name set
     */
    void RemoveFindName(const qcc::String& name) { findNames.erase(name); }

    /**
     * Remove a name from the find name set referenced by an iterator.
     *
     * @param it    Reference to the name to remove from the find name set
     */
    void RemoveFindName(NameSet::iterator& it) { findNames.erase(it); }

    /**
     * Get the bus GUID.
     *
     * @return  String representation of the bus GUID.
     */
    const qcc::GUID128& GetGUID() const { return guid; }

    /**
     * Set the bus GUID.
     *
     * @param guid  String representation of the bus GUID.
     */
    void SetGUID(const qcc::String& guid) { this->guid = qcc::GUID128(guid); }
    void SetGUID(const qcc::GUID128& guid) { this->guid = guid; }

    /**
     * Get the unique name of the AllJoyn controller object.
     *
     * @return  The unique name of the AllJoyn controller object.
     */
    const qcc::String& GetUniqueName() const { return uniqueName; }

    /**
     * Set the unique name of the AllJoyn controller object.  Care must be
     * taken when setting this.  It is used as a lookup key in BTNodeDB and
     * setting this for a node contained by BTNodeDB will _NOT_ update that
     * index.
     *
     * @param name  The unique name of the AllJoyn controller object.
     */
    void SetUniqueName(const qcc::String& name) { uniqueName = name; }

    /**
     * Get the Bluetooth bus address.
     *
     * @return  The Bluetooth bus address.
     */
    const BTBusAddress& GetBusAddress() const { return nodeAddr; }

    /**
     * Set the Bluetooth bus address.  Care must be taken when setting this.
     * It is used as a lookup key in BTNodeDB and setting this for a node
     * contained by BTNodeDB will _NOT_ update that index.
     *
     * @param addr  The Bluetooth bus address.
     */
    void SetBusAddress(const BTBusAddress& addr) { nodeAddr = addr; }

    /**
     * Get whether this node is a direct minion or not.
     *
     * @return  'true' if the node is a direct minion, 'false' otherwise.
     */
    bool IsDirectMinion() const { return relationship == DIRECT_MINION; }

    /**
     * Get whether this node is a minion (direct or indirect) or not.
     *
     * @return  'true' if the node is a minion, 'false' otherwise.
     */
    bool IsMinion() const { return (relationship == DIRECT_MINION) || (relationship == INDIRECT_MINION); }

    /**
     * Set whether this node is a direct minion or not.
     *
     * @param val   'true' if the node is a direct minion, 'false' otherwise.
     */
    void SetRelationship(NodeRelationships relationship) { this->relationship = relationship; }

    /**
     * Get the bus address that is accepting connections for us.
     *
     * @return  Bus address accepting connections for us
     */
    BTNodeInfo GetConnectNode() const
    {
        BTNodeInfo next = BTNodeInfo::wrap((_BTNodeInfo*)this);
        while (next->connectProxyNode) {
            next = *(next->connectProxyNode);
        }
        return next;
    }

    /**
     * Set the node that accepts connections for us.  It may actually have
     * node that accepts connections for it, thus producing a chain.  Care
     * must be taken when setting this.  It is used as a lookup key in
     * BTNodeDB and setting this for a node contained by BTNodeDB will _NOT_
     * update that index.
     *
     * @param node  Node that will accept connections for us.
     */
    void SetConnectNode(const BTNodeInfo& node)
    {
        if (*node == *this) {
            if (connectProxyNode) {
                delete connectProxyNode;
                connectProxyNode = NULL;
            } // connectProxyNode == NULL -- nothing to do
        } else {
            if (connectProxyNode) {
                *connectProxyNode = node;
            } else {
                connectProxyNode = new BTNodeInfo(node);
            }
        }
    }

    /**
     * Get the UUID revision of the advertisement this node was discovered in.
     *
     * @return  The UUID revision.
     */
    uint32_t GetUUIDRev() const { return uuidRev; }

    /**
     * Set the UUID revision of the advertisement this node was discovered in.
     *
     * @param uuidRev   The UUID revision.
     */
    void SetUUIDRev(uint32_t uuidRev) { this->uuidRev = uuidRev; }

    /**
     * Get the absolute expire time in milliseconds.  If value is
     * numeric_limits<uint64_t>::max() then no expiration timeout set.
     *
     * @return  Absolute expiration time in milliseconds
     */
    uint64_t GetExpireTime() const { return expireTime; }

    /**
     * Set the expiration time.  Care must be taken when setting this.  It is
     * used as a lookup key in BTNodeDB and setting this for a node contained
     * by BTNodeDB will _NOT_ update that index.
     *
     * @param expireTime    Absolute expiration time in milliseconds
     */
    void SetExpireTime(uint64_t expireTime)
    {
        this->expireTime = expireTime;
    }

    /**
     * Indicate if the node is EIR capable or not.
     *
     * @return  - true if node is EIR capable
     *          - false if node is not EIR capable
     */
    bool IsEIRCapable() const { return eirCapable; }

    /**
     * Set EIR capability flag.
     *
     * @param eirCapable    Set to true if node is EIR capable
     */
    void SetEIRCapable(bool eirCapable) { this->eirCapable = eirCapable; }

    /**
     * Gets the number of connections to the node.
     *
     * @return  Number of connections
     */
    uint16_t GetConnectionCount() const { return connectionCount; }

    /**
     * Set the number of connections to the node.
     *
     * @param connectionCount   Number of connections
     */
    void SetConnectionCount(uint16_t connectionCount) { this->connectionCount = connectionCount; }

    /**
     * Increment the connection count and return the number of connections.
     *
     * @return  Number of connections after incrementing the count
     */
    uint16_t IncConnCount() { return ++connectionCount; }

    /**
     * Decrement the connection count and return the number of remaining connections.
     *
     * @return  Number of remaining connections after decrementing the count
     */
    uint16_t DecConnCount() { return --connectionCount; }

    /**
     * Get the session ID of the connection to this node.
     *
     * @return  BT topology manager session ID
     */
    SessionId GetSessionID() const { return sessionID; }

    /**
     * Set the session ID of the connection to this node.
     *
     * @param sessionID  BT topology manager session ID
     */
    void SetSessionID(SessionId sessionID) { this->sessionID = sessionID; }

    SessionState GetSessionState() const { return sessionState; }
    void SetSessionState(SessionState state) { sessionState = state; }

    /**
     * Create in a human readable form for the node (includes the memory
     * address of the managed object instance.
     *
     * @return  a string representation of node: "XX:XX:XX:XX:XX:XX-XXXX XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX (0xXXXXXXX)"
     */
    qcc::String ToString() const
    {
        return nodeAddr.ToString() + " " + guid.ToString() + " (0x" + qcc::U64ToString(reinterpret_cast<uint64_t>(&(*this)), 16) + ")";
    }

    /**
     * Equivalence operator.
     *
     * @param other     reference to the rhs of "==" for comparison
     *
     * @return  true if this is == other, false otherwise
     */
    bool operator==(const _BTNodeInfo& other) const
    {
        return (nodeAddr == other.nodeAddr);
    }

    /**
     * Inequality operator.
     *
     * @param other     reference to the rhs of "==" for comparison
     *
     * @return  true if this is != other, false otherwise
     */
    bool operator!=(const _BTNodeInfo& other) const { return !(*this == other); }

    /**
     * Less than operator.
     *
     * @param other     reference to the rhs of "<" for comparison
     *
     * @return  true if this is < other, false otherwise
     */
    bool operator<(const _BTNodeInfo& other) const
    {
        return (nodeAddr < other.nodeAddr);
    }

    /**
     * Clone this node into a new instance of BTNodeInfo.  Advertise and find
     * names may optionally be included in the clone.  They are excluded by
     * default.  Modifications to the new instance will not magically show up
     * in this instance.
     *
     * @param includeNames  [optional] Include advertise/find names in clone.
     *
     * @return  A new, independent instance of this.
     */
    BTNodeInfo Clone(bool includeNames = false) const
    {
        BTNodeInfo clone(nodeAddr, uniqueName, guid);
        clone->relationship = relationship;
        clone->connectProxyNode = connectProxyNode ? new BTNodeInfo(*connectProxyNode) : NULL;
        if (includeNames) {
            clone->adNames.insert(adNames.begin(), adNames.end());
            clone->findNames.insert(findNames.begin(), findNames.end());
        }
        clone->uuidRev = uuidRev;
        clone->expireTime = expireTime;
        clone->eirCapable = eirCapable;
        clone->connectionCount = connectionCount;
        clone->sessionID = sessionID;
        clone->sessionState = sessionState;

        return clone;
    }

    void Diff(const BTNodeInfo& other, BTNodeInfo* added, BTNodeInfo* removed) const
    {
        NameSet::const_iterator it;
        if (added) {
            (*added) = Clone();
            assert((*added)->adNames.empty() && (*added)->findNames.empty());
            for (it = other->adNames.begin(); it != other->adNames.end(); ++it) {
                if (adNames.find(*it) == adNames.end()) {
                    (*added)->adNames.insert(*it);
                }
            }
            for (it = other->findNames.begin(); it != other->findNames.end(); ++it) {
                if (findNames.find(*it) == findNames.end()) {
                    (*added)->findNames.insert(*it);
                }
            }
            if (!(*added)->adNames.empty() || !(*added)->findNames.empty()) {
                (*added)->nodeAddr = nodeAddr;
            } else {
                (*added)->nodeAddr = BTBusAddress();
            }
        }
        if (removed) {
            (*removed) = Clone();
            assert((*removed)->adNames.empty() && (*removed)->findNames.empty());
            for (it = adNames.begin(); it != adNames.end(); ++it) {
                if (other->adNames.find(*it) == other->adNames.end()) {
                    (*removed)->adNames.insert(*it);
                }
            }
            for (it = findNames.begin(); it != findNames.end(); ++it) {
                if (other->findNames.find(*it) == other->findNames.end()) {
                    (*removed)->findNames.insert(*it);
                }
            }
            if (!(*removed)->adNames.empty() || !(*removed)->findNames.empty()) {
                (*removed)->nodeAddr = nodeAddr;
            } else {
                (*removed)->nodeAddr = BTBusAddress();
            }
        }
    }

    void Update(const BTNodeInfo* added, const BTNodeInfo* removed)
    {
        NameSet::const_iterator it;
        if (removed) {
            for (it = (*removed)->adNames.begin(); it != (*removed)->adNames.end(); ++it) {
                adNames.erase(*it);
            }
            for (it = (*removed)->findNames.begin(); it != (*removed)->findNames.end(); ++it) {
                findNames.erase(*it);
            }
        }
        if (added) {
            for (it = (*added)->adNames.begin(); it != (*added)->adNames.end(); ++it) {
                adNames.insert(*it);
            }
            for (it = (*added)->findNames.begin(); it != (*added)->findNames.end(); ++it) {
                findNames.insert(*it);
            }
        }
    }


  private:
    /**
     * Private copy constructor to catch potential coding errors.
     */
    _BTNodeInfo(const _BTNodeInfo& other) { }

    /**
     * Private assignment operator to catch potential coding errors.
     */
    _BTNodeInfo& operator=(const _BTNodeInfo& other) { return *this; }

    qcc::GUID128 guid;              /**< Bus GUID of the node. */
    qcc::String uniqueName;         /**< Unique bus name of the daemon on the node. */
    BTBusAddress nodeAddr;          /**< Bus address of the node. */
    NodeRelationships relationship; /**< Relationship of node with respect to self. */
    BTNodeInfo* connectProxyNode;   /**< Node that will accept connections for us. */
    NameSet adNames;                /**< Set of advertise names. */
    NameSet findNames;              /**< Set of find names. */
    uint32_t uuidRev;               /**< UUID revision of the advertisement this node was found in. */
    uint64_t expireTime;            /**< Time when advertised information is considered stale. */
    bool eirCapable;                /**< Indicates if device is EIR capable or not. */
    uint16_t connectionCount;       /**< Number of connections with this node. */
    SessionId sessionID;            /**< BT controller session ID. */
    SessionState sessionState;      /**< BT topology manager session state with this node. (Only valid for MASTER and DIRECT_MINION) */
};

}

#endif
