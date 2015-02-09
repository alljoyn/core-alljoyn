/**
 * @file
 * The KeyStoreListener class handled requests to load or store the key store.
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
#ifndef _ALLJOYN_KEYSTORE_LISTENER_H
#define _ALLJOYN_KEYSTORE_LISTENER_H

#ifndef __cplusplus
#error Only include KeyStoreListener.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>


namespace ajn {

/**
 * Forward declaration.
 */
class KeyStore;

/**
 * An application can provide a key store listener to override the default key store Load and Store
 * behavior. This will override the default key store behavior.
 */
class KeyStoreListener {

  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~KeyStoreListener() { }

    /**
     * This method is called when a key store needs to be loaded.
     * @remark The application must call <tt>#PutKeys</tt> to put the new key store data into the
     * internal key store.
     *
     * @param keyStore   Reference to the KeyStore to be loaded.
     *
     * @return
     *      - #ER_OK if the load request was satisfied
     *      - An error status otherwise
     *
     */
    virtual QStatus LoadRequest(KeyStore& keyStore) = 0;

    /**
     * Put keys into the key store from an encrypted byte string.
     *
     * @param keyStore  The keyStore to put to. This is the keystore indicated in the LoadRequest call.
     * @param source    The byte string containing the encrypted key store contents.
     * @param password  The password required to decrypt the key data
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     *
     */
    QStatus PutKeys(KeyStore& keyStore, const qcc::String& source, const qcc::String& password);

    /**
     * This method is called when a key store needs to be stored.
     * @remark The application must call <tt>#GetKeys</tt> to obtain the key data to be stored.
     *
     * @param keyStore   Reference to the KeyStore to be stored.
     *
     * @return
     *      - #ER_OK if the store request was satisfied
     *      - An error status otherwise
     */
    virtual QStatus StoreRequest(KeyStore& keyStore) = 0;

    /**
     * Get the current keys from the key store as an encrypted byte string.
     *
     * @param keyStore  The keyStore to get from. This is the keystore indicated in the StoreRequest call.
     * @param sink      The byte string to write the keys to.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus GetKeys(KeyStore& keyStore, qcc::String& sink);

};

}

#endif
