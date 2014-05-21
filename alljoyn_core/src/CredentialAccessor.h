#ifndef _ALLJOYN_CREDENTIAL_ACCESSOR_H
#define _ALLJOYN_CREDENTIAL_ACCESSOR_H
/**
 * @file
 * This file defines the CredentialAccessor class that provides the interface to
 * access and manage the set credentials used in the authentication
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include PasswordManager.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/GUID.h>
#include <qcc/KeyBlob.h>
#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>

namespace ajn {


/**
 * Class to allow the application to get access the authentication credentials.
 */

class CredentialAccessor {
  public:


    /**
     * Constructor
     *
     * @param keyStore  the key store
     */
    CredentialAccessor(BusAttachment& bus);

    /**
     * virtual destructor
     */
    virtual ~CredentialAccessor() { }

    /**
     * Return the Auth GUID
     * @param[out] guid object holding the auth GUID for this application
     * @return
     *      - ER_OK on success
     *      - ER_BUS_KEY_STORE_NOT_LOADED if the GUID is not available
     */
    QStatus GetGuid(qcc::GUID128& guid);

    /**
     * Return the Peer Auth GUID
     * @param peerName the peer bus name
     * @param[out] guid object holding the peerAuth GUID
     * @return
     *      - ER_OK on success
     *      - ER_BUS_NO_PEER_GUID if the GUID is not available
     */
    QStatus GetPeerGuid(qcc::String& peerName, qcc::GUID128& guid);

    /**
     * Get the guid to index a local key blob.
     *
     * @param keyType     The key blob type
     * @param[out] guid   The matching guid
     * @return
     *      - ER_OK if successful
     *      - ER_CRYPTO_KEY_UNAVAILABLE if key is unavailable
     */
    QStatus GetLocalGUID(qcc::KeyBlob::Type keyType, qcc::GUID128& guid);

    /**
     * Get a key blob
     *
     * @param guid  The unique identifier for the key
     * @param[out] key   The key blob to get
     * @return
     *      - ER_OK if successful
     *      - ER_CRYPTO_KEY_UNAVAILABLE if key is unavailable
     */
    QStatus GetKey(const qcc::GUID128& guid, qcc::KeyBlob& key);

    /**
     * Delete a key blob
     *
     * @param guid  The unique identifier for the key
     * @return
     *      - ER_OK if successful if the key is deleted or not found
     *      - ER_FAIL if the operation fails
     */
    QStatus DeleteKey(const qcc::GUID128& guid);

    /**
     * Add an associated key.
     *
     * @param headerGuid   The header guid to associate with
     * @param guid         The guid to store the key
     * @param key          The key blob to store
     * @return
     *      - ER_OK if successful
       m range
     *      - ER_FAIL if operation fails
     */
    QStatus AddAssociatedKey(qcc::GUID128& headerGuid, qcc::GUID128& guid, qcc::KeyBlob& key);

    /**
     * Get the list of keys associated with the given header GUID.
     * @param headerGuid  The header guid where the keys are associated with
     * @param[out] list  The output list of guids.  This list must be deallocated after used.
     * @param[out] numItems The output size of the list
     * @return
     *      - ER_OK if successful
     *      - ER_FAIL if operation fails
     */
    QStatus GetKeys(qcc::GUID128& headerGuid, qcc::GUID128** list, size_t* numItems);

    /**
     * store a key blob.
     * @param guid         The guid to index
     * @param key          The key blob to store
     * @return
     *      - ER_OK if successful
       m range
     *      - ER_FAIL if operation fails
     */
    QStatus StoreKey(qcc::GUID128& guid, qcc::KeyBlob& key);

  private:
    BusAttachment& bus;

};

}
#endif
