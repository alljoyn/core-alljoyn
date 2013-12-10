/**
 * @file
 *
 * Implements the BT node database.
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

#include <qcc/platform.h>

#include <list>
#include <set>
#include <vector>

#include <qcc/String.h>

#include <alljoyn/MsgArg.h>

#include "BDAddress.h"
#include "BTNodeDB.h"

#define QCC_MODULE "ALLJOYN_BTC"


using namespace std;
using namespace qcc;


namespace ajn {


static set<BTNodeInfo>::const_iterator findNode(const set<BTNodeInfo>& nodes, const BTBusAddress& addr)
{
    set<BTNodeInfo>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->GetBusAddress() == addr) {
            break;
        }
    }
    return it;
}


static set<BTNodeInfo>::iterator findNode(set<BTNodeInfo>& nodes, const BTBusAddress& addr)
{
    set<BTNodeInfo>::iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->GetBusAddress() == addr) {
            break;
        }
    }
    return it;
}


static set<BTNodeInfo>::const_iterator findNode(const set<BTNodeInfo>& nodes, const BDAddress& addr)
{
    set<BTNodeInfo>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->GetBusAddress().addr == addr) {
            break;
        }
    }
    return it;
}


static set<BTNodeInfo>::const_iterator findNode(const set<BTNodeInfo>& nodes, const String& uniqueName)
{
    set<BTNodeInfo>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it) {
        if (!(*it)->GetUniqueName().empty() && ((*it)->GetUniqueName() == uniqueName)) {
            break;
        }
    }
    return it;
}


const BTNodeInfo BTNodeDB::FindNode(const BTBusAddress& addr) const
{
    BTNodeInfo node;
    Lock(MUTEX_CONTEXT);
    set<BTNodeInfo>::const_iterator it = findNode(nodes, addr);
    if (it != nodes.end()) {
        node = *it;
    }
    Unlock(MUTEX_CONTEXT);
    return node;
}


const BTNodeInfo BTNodeDB::FindNode(const BDAddress& addr) const
{
    BTNodeInfo node;
    Lock(MUTEX_CONTEXT);
    set<BTNodeInfo>::const_iterator it = findNode(nodes, addr);
    if (it != nodes.end()) {
        node = *it;
    }
    Unlock(MUTEX_CONTEXT);
    return node;
}


const BTNodeInfo BTNodeDB::FindNode(const String& uniqueName) const
{
    BTNodeInfo node;
    Lock(MUTEX_CONTEXT);
    set<BTNodeInfo>::const_iterator it = findNode(nodes, uniqueName);
    if (it != nodes.end()) {
        node = *it;
    }
    Unlock(MUTEX_CONTEXT);
    return node;
}


BTNodeInfo BTNodeDB::FindDelegateMinion(const BTNodeInfo& start, const BTNodeInfo& skip, bool eirCapable) const
{
    Lock(MUTEX_CONTEXT);
    const_iterator next = nodes.find(start);
    const_iterator traditional = nodes.end();
#ifndef NDEBUG
    if (next == End()) {
        String s("Failed to find: " + start->GetBusAddress().addr.ToString());
        DumpTable(s.c_str());
    }
#endif
    assert(next != End());
    do {
        ++next;
        if (next == End()) {
            next = Begin();
        }

        if (!(*next)->IsEIRCapable() && (traditional == nodes.end()) && (*next != skip)) {
            traditional = next;
        }



    } while ((*next != start) && (!(*next)->IsMinion() || (*next == skip) || !(*next)->IsEIRCapable()));
    Unlock(MUTEX_CONTEXT);

    if (!eirCapable) {
        next = (*next == start) ? traditional : next;
    }

    return *next;
}


void BTNodeDB::AddNode(const BTNodeInfo& node)
{
    Lock(MUTEX_CONTEXT);
    assert(node->IsValid());
    RemoveNode(node);  // remove the old one (if it exists) before adding the new one with updated info

    // Add to the master set
    nodes.insert(node);

    Unlock(MUTEX_CONTEXT);
}


void BTNodeDB::RemoveNode(const BTNodeInfo& node)
{
    Lock(MUTEX_CONTEXT);
    set<BTNodeInfo>::iterator it = findNode(nodes, node->GetBusAddress());
    if (it != nodes.end()) {
        // Remove from the master set
        nodes.erase(it);
    }

    Unlock(MUTEX_CONTEXT);
}


void BTNodeDB::Diff(const BTNodeDB& other, BTNodeDB* added, BTNodeDB* removed) const
{
    Lock(MUTEX_CONTEXT);
    other.Lock(MUTEX_CONTEXT);
    if (added) {
        added->Lock(MUTEX_CONTEXT);
    }
    if (removed) {
        removed->Lock(MUTEX_CONTEXT);
    }

    const_iterator nodeit;
    set<BTNodeInfo>::const_iterator addrit;

    // Find removed names/nodes
    if (removed) {
        for (nodeit = Begin(); nodeit != End(); ++nodeit) {
            const BTNodeInfo& node = *nodeit;
            addrit = findNode(other.nodes, node->GetBusAddress());
            if (addrit == other.nodes.end()) {
                removed->AddNode(node);
            } else {
                BTNodeInfo diffNode = node->Clone();
                bool include = false;
                const BTNodeInfo& onode = *addrit;
                NameSet::const_iterator nameit;
                NameSet::const_iterator onameit;
                for (nameit = node->GetAdvertiseNamesBegin(); nameit != node->GetAdvertiseNamesEnd(); ++nameit) {
                    const String& name = *nameit;
                    onameit = onode->FindAdvertiseName(name);
                    if (onameit == onode->GetAdvertiseNamesEnd()) {
                        diffNode->AddAdvertiseName(name);
                        include = true;
                    }
                }
                if (include) {
                    removed->AddNode(diffNode);
                }
            }
        }
    }

    // Find added names/nodes
    if (added) {
        for (nodeit = other.Begin(); nodeit != other.End(); ++nodeit) {
            const BTNodeInfo& onode = *nodeit;
            addrit = findNode(nodes, onode->GetBusAddress());
            if (addrit == nodes.end()) {
                added->AddNode(onode);
            } else {
                BTNodeInfo diffNode = onode->Clone();
                bool include = false;
                const BTNodeInfo& node = *addrit;
                NameSet::const_iterator nameit;
                NameSet::const_iterator onameit;
                for (onameit = onode->GetAdvertiseNamesBegin(); onameit != onode->GetAdvertiseNamesEnd(); ++onameit) {
                    const String& oname = *onameit;
                    nameit = node->FindAdvertiseName(oname);
                    if (nameit == node->GetAdvertiseNamesEnd()) {
                        diffNode->AddAdvertiseName(oname);
                        include = true;
                    }
                }
                if (include) {
                    added->AddNode(diffNode);
                }
            }
        }
    }

    if (removed) {
        removed->Unlock(MUTEX_CONTEXT);
    }
    if (added) {
        added->Unlock(MUTEX_CONTEXT);
    }
    other.Unlock(MUTEX_CONTEXT);
    Unlock(MUTEX_CONTEXT);
}


void BTNodeDB::NodeDiff(const BTNodeDB& other, BTNodeDB* added, BTNodeDB* removed) const
{
    Lock(MUTEX_CONTEXT);
    other.Lock(MUTEX_CONTEXT);
    if (added) {
        added->Lock(MUTEX_CONTEXT);
    }
    if (removed) {
        removed->Lock(MUTEX_CONTEXT);
    }

    const_iterator nodeit;
    set<BTNodeInfo>::const_iterator addrit;

    // Find removed names/nodes
    if (removed) {
        for (nodeit = Begin(); nodeit != End(); ++nodeit) {
            const BTNodeInfo& node = *nodeit;
            addrit = findNode(other.nodes, node->GetBusAddress());
            if (addrit == other.nodes.end()) {
                removed->AddNode(node);
            }
        }
    }

    // Find added names/nodes
    if (added) {
        for (nodeit = other.Begin(); nodeit != other.End(); ++nodeit) {
            const BTNodeInfo& onode = *nodeit;
            addrit = findNode(nodes, onode->GetBusAddress());
            if (addrit == nodes.end()) {
                added->AddNode(onode);
            }
        }
    }

    if (removed) {
        removed->Unlock(MUTEX_CONTEXT);
    }
    if (added) {
        added->Unlock(MUTEX_CONTEXT);
    }
    other.Unlock(MUTEX_CONTEXT);
    Unlock(MUTEX_CONTEXT);
}


void BTNodeDB::UpdateDB(const BTNodeDB* added, const BTNodeDB* removed, bool removeNodes)
{
    // Remove names/nodes
    Lock(MUTEX_CONTEXT);
    if (removed) {
        const_iterator rit;
        for (rit = removed->Begin(); rit != removed->End(); ++rit) {
            BTNodeInfo rnode = *rit;
            set<BTNodeInfo>::const_iterator it = findNode(nodes, rnode->GetBusAddress());
            if (it != nodes.end()) {
                // Remove names from node
                BTNodeInfo node = *it;
                if (&(*node) == &(*rnode)) {
                    // The exact same instance of node is in the removed DB so
                    // just remove the node so that the names don't get
                    // corrupted in the removed DB.
                    RemoveNode(node);

                } else {
                    // node and rnode are different instances so there is no
                    // chance of corrupting the list of names in rnode.
                    NameSet::const_iterator rnameit;
                    for (rnameit = rnode->GetAdvertiseNamesBegin(); rnameit != rnode->GetAdvertiseNamesEnd(); ++rnameit) {
                        const String& rname = *rnameit;
                        node->RemoveAdvertiseName(rname);
                    }
                    if (removeNodes && (node->AdvertiseNamesEmpty())) {
                        // Remove node with no advertise names
                        RemoveNode(node);
                    }
                }
            } // else not in DB so ignore it.
        }
    }

    if (added) {
        // Add names/nodes
        const_iterator ait;
        for (ait = added->Begin(); ait != added->End(); ++ait) {
            BTNodeInfo anode = *ait;
            set<BTNodeInfo>::const_iterator it = findNode(nodes, anode->GetBusAddress());
            if (it == nodes.end()) {
                // New node
                BTNodeInfo connNode = FindNode(anode->GetConnectNode()->GetBusAddress());
                if (connNode->IsValid()) {
                    anode->SetConnectNode(connNode);
                }
                assert(anode->GetConnectNode()->IsValid());
                AddNode(anode);
            } else {
                // Add names to existing node
                BTNodeInfo node = *it;
                NameSet::const_iterator anameit;
                for (anameit = anode->GetAdvertiseNamesBegin(); anameit != anode->GetAdvertiseNamesEnd(); ++anameit) {
                    const String& aname = *anameit;
                    node->AddAdvertiseName(aname);
                }
                BTNodeInfo connNode = FindNode(anode->GetConnectNode()->GetBusAddress());
                if (!connNode->IsValid()) {
                    connNode = added->FindNode(anode->GetConnectNode()->GetBusAddress());
                }
                assert(connNode->IsValid());
                node->SetConnectNode(connNode);
                // Update the UUIDRev
                node->SetUUIDRev(anode->GetUUIDRev());
                if (useExpirations) {
                    // Update the expire time
                    node->SetExpireTime(anode->GetExpireTime());
                }
                if ((node->GetUniqueName() != anode->GetUniqueName()) && !anode->GetUniqueName().empty()) {
                    node->SetUniqueName(anode->GetUniqueName());
                }
            }
        }
    }

    Unlock(MUTEX_CONTEXT);
}


void BTNodeDB::RemoveExpiration()
{
    if (useExpirations) {
        Lock(MUTEX_CONTEXT);
        uint64_t expireTime = numeric_limits<uint64_t>::max();
        iterator it = nodes.begin();
        while (it != nodes.end()) {
            BTNodeInfo node = *it;
            node->SetExpireTime(expireTime);
            ++it;
        }
        Unlock(MUTEX_CONTEXT);
    } else {
        QCC_LogError(ER_FAIL, ("Called RemoveExpiration on BTNodeDB instance initialized without expiration support."));
        assert(false);
    }
}


void BTNodeDB::RefreshExpiration(uint32_t expireDelta)
{
    if (useExpirations) {
        Lock(MUTEX_CONTEXT);
        Timespec now;
        GetTimeNow(&now);
        uint64_t expireTime = now.GetAbsoluteMillis() + expireDelta;
        iterator it = nodes.begin();
        while (it != nodes.end()) {
            BTNodeInfo node = *it;
            node->SetExpireTime(expireTime);
            ++it;
        }
        Unlock(MUTEX_CONTEXT);
    } else {
        QCC_LogError(ER_FAIL, ("Called RefreshExpiration on BTNodeDB instance initialized without expiration support."));
        assert(false);
    }
}


void BTNodeDB::RefreshExpiration(const BTNodeInfo& connNode, uint32_t expireDelta)
{
    if (useExpirations) {
        Lock(MUTEX_CONTEXT);

        Timespec now;
        GetTimeNow(&now);
        uint64_t expireTime = now.GetAbsoluteMillis() + expireDelta;

        for (set<BTNodeInfo>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ((*it)->GetConnectNode() == connNode) {
                BTNodeInfo node = *it;
                node->SetExpireTime(expireTime);
                node->SetUUIDRev(connNode->GetUUIDRev());
            }
        }

        Unlock(MUTEX_CONTEXT);
    } else {
        QCC_LogError(ER_FAIL, ("Called RefreshExpiration on BTNodeDB instance initialized without expiration support."));
        assert(false);
    }
}


void BTNodeDB::NodeSessionLost(SessionId sessionID)
{
    Lock(MUTEX_CONTEXT);
    set<BTNodeInfo>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->GetSessionID() && ((*it)->GetSessionID() == sessionID)) {
            break;
        }
    }
    if (it != nodes.end()) {
        BTNodeInfo lnode = *it;

        lnode->SetSessionID(0);
        lnode->SetSessionState(_BTNodeInfo::NO_SESSION);
    }
    Unlock(MUTEX_CONTEXT);
}


void BTNodeDB::UpdateNodeSessionID(SessionId sessionID, const BTNodeInfo& node)
{
    Lock(MUTEX_CONTEXT);
    set<BTNodeInfo>::const_iterator it = findNode(nodes, node->GetBusAddress());
    if (it != nodes.end()) {
        BTNodeInfo lnode = *it;

        lnode->SetSessionID(sessionID);
        lnode->SetSessionState(_BTNodeInfo::SESSION_UP);
    }
    Unlock(MUTEX_CONTEXT);
}


#ifndef NDEBUG
void BTNodeDB::DumpTable(const char* info) const
{
    Lock(MUTEX_CONTEXT);
    const_iterator nodeit;
    QCC_DbgPrintf(("Node DB (%s):", info));
    for (nodeit = Begin(); nodeit != End(); ++nodeit) {
        const BTNodeInfo& node = *nodeit;
        NameSet::const_iterator nameit;
        String expireTime;
        if (node->GetExpireTime() == numeric_limits<uint64_t>::max()) {
            expireTime = "<infinite>";
        } else {
            Timespec now;
            GetTimeNow(&now);
            int64_t delta = node->GetExpireTime() - now.GetAbsoluteMillis();
            expireTime = I64ToString(delta, 10, (delta < 0) ? 5 : 4, '0');
            expireTime = expireTime.substr(0, expireTime.size() - 3) + '.' + expireTime.substr(expireTime.size() - 3);
        }
        QCC_DbgPrintf(("    %s (connect addr: %s  unique name: \"%s\"  uuidRev: %08x  direct: %s  expire time: %s):",
                       node->ToString().c_str(),
                       node->GetConnectNode()->ToString().c_str(),
                       node->GetUniqueName().c_str(),
                       node->GetUUIDRev(),
                       node->IsDirectMinion() ? "true" : "false",
                       expireTime.c_str()));
        QCC_DbgPrintf(("         Advertise names:"));
        for (nameit = node->GetAdvertiseNamesBegin(); nameit != node->GetAdvertiseNamesEnd(); ++nameit) {
            QCC_DbgPrintf(("            %s", nameit->c_str()));
        }
        QCC_DbgPrintf(("         Find names:"));
        for (nameit = node->GetFindNamesBegin(); nameit != node->GetFindNamesEnd(); ++nameit) {
            QCC_DbgPrintf(("            %s", nameit->c_str()));
        }
    }
    Unlock(MUTEX_CONTEXT);
}
#endif

}
