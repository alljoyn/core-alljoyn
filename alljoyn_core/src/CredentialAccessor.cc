/**
 * @file
 * CredentialAccessor is the utility that allows access to some of the credential material
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
#include <qcc/platform_cpp.h>

#include <qcc/GUID.h>
#include <qcc/StringUtil.h>
#include "CredentialAccessor.h"
#include "BusInternal.h"
#include "PeerState.h"

#define QCC_MODULE "ALLJOYN"


using namespace std;
using namespace qcc;


namespace ajn {


CredentialAccessor::CredentialAccessor(BusAttachment& bus) :
    bus(bus) {
}

QStatus CredentialAccessor::GetGuid(qcc::GUID128& guid)
{
    return bus.GetInternal().GetKeyStore().GetGuid(guid);
}

QStatus CredentialAccessor::GetPeerGuid(qcc::String& peerName, qcc::GUID128& guid)
{
    PeerStateTable* peerStateTable = bus.GetInternal().GetPeerStateTable();
    if (peerStateTable->IsKnownPeer(peerName)) {
        PeerState peerState = peerStateTable->GetPeerState(peerName);
        guid = peerState->GetGuid();
        return ER_OK;
    }
    return ER_BUS_NO_PEER_GUID;
}

QStatus CredentialAccessor::GetKey(const qcc::GUID128& guid, qcc::KeyBlob& key) {
    return bus.GetInternal().GetKeyStore().GetKey(guid, key);
}

QStatus CredentialAccessor::DeleteKey(const qcc::GUID128& guid)
{
    KeyBlob kb;
    QStatus status = bus.GetInternal().GetKeyStore().GetKey(guid, kb);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return ER_OK;     /* not found */
    } else if (status != ER_OK) {
        return status;
    }
    status = bus.GetInternal().GetKeyStore().DelKey(guid);
    if ((status != ER_OK) && (status != ER_BUS_KEY_UNAVAILABLE)) {
        return status;
    }
    bool deleteAssociates = false;
    if ((kb.GetAssociationMode() == KeyBlob::ASSOCIATE_HEAD) ||
        (kb.GetAssociationMode() == KeyBlob::ASSOCIATE_BOTH)) {
        deleteAssociates = true;
    }
    if (!deleteAssociates) {
        return ER_OK;
    }
    qcc::GUID128* list;
    size_t numItems;
    status = bus.GetInternal().GetKeyStore().SearchAssociatedKeys(guid, &list, &numItems);
    if (status != ER_OK) {
        return ER_OK;
    }
    if (numItems == 0) {
        return ER_OK;
    }
    for (size_t cnt = 0; cnt < numItems; cnt++) {
        status = DeleteKey(list[cnt]);      /* do not call KeyStore::DelKey directly since it is neccesary to handle the associated deletes for each member */
    }
    delete [] list;
    return ER_OK;
}

QStatus CredentialAccessor::StoreKey(qcc::GUID128& guid, qcc::KeyBlob& key)
{
    KeyStore& ks = bus.GetInternal().GetKeyStore();
    QStatus status = ks.AddKey(guid, key);
    if (status != ER_OK) {
        return status;
    }
    /* persist the changes */
    return ks.Store();
}

QStatus CredentialAccessor::GetKeys(qcc::GUID128& headerGuid, qcc::GUID128** list, size_t* numItems)
{
    KeyBlob headerKb;
    QStatus status = GetKey(headerGuid, headerKb);
    if (status != ER_OK) {
        return status;
    }
    return bus.GetInternal().GetKeyStore().SearchAssociatedKeys(headerGuid, list, numItems);
}

QStatus CredentialAccessor::AddAssociatedKey(qcc::GUID128& headerGuid, qcc::GUID128& guid, qcc::KeyBlob& key)
{
    if (headerGuid == guid) {
        return StoreKey(headerGuid, key);
    }
    KeyBlob headerKb;
    QStatus status = GetKey(headerGuid, headerKb);
    if (status != ER_OK) {
        return status;
    }
    bool dirty = false;
    if (headerKb.GetAssociationMode() == KeyBlob::ASSOCIATE_NONE) {
        headerKb.SetAssociationMode(KeyBlob::ASSOCIATE_HEAD);
        dirty = true;
    } else if (headerKb.GetAssociationMode() == KeyBlob::ASSOCIATE_MEMBER) {
        headerKb.SetAssociationMode(KeyBlob::ASSOCIATE_BOTH);
        dirty = true;
    }
    if (dirty) {
        status = StoreKey(headerGuid, headerKb);
        if (status != ER_OK) {
            return status;
        }
    }
    key.SetAssociation(headerGuid);
    return StoreKey(guid, key);
}
}
