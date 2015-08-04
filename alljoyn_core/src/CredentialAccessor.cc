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
#include <qcc/platform.h>

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

QStatus CredentialAccessor::GetDSAPublicKey(qcc::ECCPublicKey& publicKey)
{
    KeyStore::Key key;
    qcc::KeyBlob kb;
    GetLocalKey(qcc::KeyBlob::DSA_PUBLIC, key);
    QStatus status = GetKey(key, kb);
    if (status != ER_OK) {
        return status;
    }
    memcpy(&publicKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}

QStatus CredentialAccessor::GetDSAPrivateKey(qcc::ECCPrivateKey& privateKey)
{
    KeyStore::Key key;
    qcc::KeyBlob kb;
    GetLocalKey(qcc::KeyBlob::DSA_PRIVATE, key);
    QStatus status = GetKey(key, kb);
    if (status != ER_OK) {
        return status;
    }
    memcpy(&privateKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}



QStatus CredentialAccessor::GetLocalKey(qcc::KeyBlob::Type keyType, KeyStore::Key& key)
{
    key.SetType(KeyStore::Key::LOCAL);
    /* each local key will be indexed by an hardcode randomly generated GUID.
       This method is similar to that used by the RSA Key exchange to store
       the private key and cert chain */
    if (keyType == KeyBlob::PRIVATE) {
        key.SetGUID(GUID128(qcc::String("a62655061e8295e2462794065f2a1c95")));
        return ER_OK;
    }
    if (keyType == KeyBlob::AES) {
        key.SetGUID(GUID128(qcc::String("b4dc47954ce6e94f6669f31b343b91d8")));
        return ER_OK;
    }
    if (keyType == KeyBlob::PEM) {
        key.SetGUID(GUID128(qcc::String("29ebe36c0ac308c8eb808cfdf1f36953")));
        return ER_OK;
    }
    if (keyType == KeyBlob::PUBLIC) {
        key.SetGUID(GUID128(qcc::String("48b020fc3a65c6bc5ac22b949a869dab")));
        return ER_OK;
    }
    if (keyType == KeyBlob::SPKI_CERT) {
        key.SetGUID(GUID128(qcc::String("9ddf8d784fef4b57d5103e3bef656067")));
        return ER_OK;
    }
    if (keyType == KeyBlob::DSA_PRIVATE) {
        key.SetGUID(GUID128(qcc::String("d1b60ce37ba71ea4b870d73b6cd676f5")));
        return ER_OK;
    }
    if (keyType == KeyBlob::DSA_PUBLIC) {
        key.SetGUID(GUID128(qcc::String("19409269762da560d7812cb8a542f024")));
        return ER_OK;
    }
    return ER_CRYPTO_KEY_UNAVAILABLE;      /* not available */
}

QStatus CredentialAccessor::GetKey(const KeyStore::Key& key, qcc::KeyBlob& keyBlob)
{
    return bus.GetInternal().GetKeyStore().GetKey(key, keyBlob);
}

QStatus CredentialAccessor::DeleteKey(const KeyStore::Key& key)
{
    KeyBlob kb;
    QStatus status = bus.GetInternal().GetKeyStore().GetKey(key, kb);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return ER_OK;     /* not found */
    } else if (status != ER_OK) {
        return status;
    }
    status = bus.GetInternal().GetKeyStore().DelKey(key);
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
    KeyStore::Key* list;
    size_t numItems;
    status = bus.GetInternal().GetKeyStore().SearchAssociatedKeys(key, &list, &numItems);
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

QStatus CredentialAccessor::StoreKey(KeyStore::Key& key, qcc::KeyBlob& keyBlob)
{
    KeyStore& ks = bus.GetInternal().GetKeyStore();
    QStatus status = ks.AddKey(key, keyBlob);
    if (status != ER_OK) {
        return status;
    }
    /* persist the changes */
    return ks.Store();
}

QStatus CredentialAccessor::GetKeys(const KeyStore::Key& headerKey, KeyStore::Key** list, size_t* numItems)
{
    KeyBlob headerKb;
    QStatus status = GetKey(headerKey, headerKb);
    if (status != ER_OK) {
        return status;
    }
    return bus.GetInternal().GetKeyStore().SearchAssociatedKeys(headerKey, list, numItems);
}

QStatus CredentialAccessor::AddAssociatedKey(KeyStore::Key& headerKey, KeyStore::Key& key, qcc::KeyBlob& keyBlob)
{
    if (headerKey == key) {
        return StoreKey(headerKey, keyBlob);
    }
    KeyBlob headerKb;
    QStatus status = GetKey(headerKey, headerKb);
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
        status = StoreKey(headerKey, headerKb);
        if (status != ER_OK) {
            return status;
        }
    }
    keyBlob.SetAssociation(headerKey.GetGUID());
    return StoreKey(key, keyBlob);
}
}
