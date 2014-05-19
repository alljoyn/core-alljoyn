/**
 * @file
 * The KeyStore class manages the storing and loading of key blobs from external storage.
 */

/******************************************************************************
 * Copyright (c) 2010-2012, 2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_KEYSTORE_H
#define _ALLJOYN_KEYSTORE_H

#ifndef __cplusplus
#error Only include KeyStore.h in C++ code.
#endif

#include <map>
#include <set>

#include <qcc/platform.h>

#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/KeyBlob.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/Event.h>
#include <qcc/time.h>

#include <alljoyn/KeyStoreListener.h>
#include "ProtectedKeyStoreListener.h"

#include <alljoyn/Status.h>


namespace ajn {

/* forward decl */
class KeyStoreKeyEventListener;

/**
 * The %KeyStore class manages the storing and loading of key blobs from
 * external storage.
 */
class KeyStore {
  public:

    /**
     * KeyStore constructor
     */
    KeyStore(const qcc::String& application);

    /**
     * KeyStore destructor
     */
    ~KeyStore();

    /**
     * Initialize the key store. The function can only be called once.
     *
     * @param fileName  The filename to be used by the default key store if the default key store is being used.
     *                  This overrides the value in the applicationName parameter passed into the constructor.
     *
     * @param isShared  Indicates if the key store is being shared between multiple applications.
     */
    QStatus Init(const char* fileName, bool isShared);

    /**
     * Requests the key store listener to store the contents of the key store
     */
    QStatus Store();

    /**
     * Re-read keys from the key store. This is a no-op unless the key store is shared.
     * If the key store is shared the key store is reloaded merging any changes made by
     * other applications with changes made by the calling application.
     *
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     *
     */
    QStatus Reload();

    /**
     * Get a key blob from the key store
     *
     * @param guid         The unique identifier for the key
     * @param key          The key blob to get
     * @param accessRights The access rights associated with the key
     * @return
     *      - ER_OK if successful
     *      - ER_BUS_KEY_UNAVAILABLE if key is unavailable
     *      - ER_BUS_KEY_EXPIRED if the requested key has expired
     */
    QStatus GetKey(const qcc::GUID128 & guid, qcc::KeyBlob & key, uint8_t accessRights[4]);

    /**
     * Get a key blob from the key store
     *
     * @param guid     The unique identifier for the key
     * @param key      The key blob to get
     * @return
     *      - ER_OK if successful
     *      - ER_BUS_KEY_UNAVAILABLE if key is unavailable
     *      - ER_BUS_KEY_EXPIRED if the requested key has expired
     */
    QStatus GetKey(const qcc::GUID128& guid, qcc::KeyBlob& key) {
        uint8_t accessRights[4];
        return GetKey(guid, key, accessRights);
    }

    /**
     * Add a key blob to the key store
     *
     * @param guid         The unique identifier for the key
     * @param key          The key blob to add
     * @param accessRights The access rights associated with the key
     * @return ER_OK
     */
    QStatus AddKey(const qcc::GUID128& guid, const qcc::KeyBlob& key, const uint8_t accessRights[4]);

    /**
     * Add a key blob to the key store
     *
     * @param guid     The unique identifier for the key
     * @param key      The key blob to add
     * @return ER_OK
     */
    QStatus AddKey(const qcc::GUID128& guid, const qcc::KeyBlob& key) {
        const uint8_t accessRights[4] = { 0, 0, 0, 0 };
        return AddKey(guid, key, accessRights);
    }

    /**
     * Remove a key blob from the key store
     *
     * @param guid  The unique identifier for the key
     * return ER_OK
     */
    QStatus DelKey(const qcc::GUID128& guid);

    /**
     * Set expiration time on a key blob from the key store
     *
     * @param guid        The unique identifier for the key
     * @param expiration  Time to expire the key
     * return ER_OK
     */
    QStatus SetKeyExpiration(const qcc::GUID128& guid, const qcc::Timespec& expiration);

    /**
     * Get expiration time on a key blob from the key store
     *
     * @param guid        The unique identifier for the key
     * @param expiration  Time the key will expire
     * return ER_OK
     */
    QStatus GetKeyExpiration(const qcc::GUID128& guid, qcc::Timespec& expiration);

    /**
     * Test is there is a requested key blob in the key store
     *
     * @param guid  The unique identifier for the key
     * @return      Returns true if the requested key is in the key store.
     */
    bool HasKey(const qcc::GUID128& guid);

    /**
     * Return the GUID for this key store
     * @param[out] guid return the GUID for this key store
     * @return
     *      - ER_OK on success
     *      - ER_BUS_KEY_STORE_NOT_LOADED if the GUID is not available
     */
    QStatus GetGuid(qcc::GUID128& guid)
    {
        if (storeState == UNAVAILABLE) {
            return ER_BUS_KEY_STORE_NOT_LOADED;
        } else {
            guid = thisGuid;
            return ER_OK;
        }
    }

    /**
     * Return the GUID for this key store as a hex-encoded string.
     *
     * @return  Returns the hex-encode string for the GUID or an empty string if the key store is not loaded.
     */
    qcc::String GetGuid() {  return (storeState == UNAVAILABLE) ? "" : thisGuid.ToString(); }

    /**
     * Override the default listener so the application can provide the load and store
     *
     * @param listener  The listener that will override the default listener. This function
     *                  must be called before the key store is initialized.
     */
    QStatus SetListener(KeyStoreListener& listener);

    /**
     * Setup the key event listener.
     *
     * @param listener  The listener that will listen to key event.
     */
    QStatus SetKeyEventListener(KeyStoreKeyEventListener* listener);

    /**
     * Restores the default listener
     *
     */
    QStatus SetDefaultListener();

    /**
     * Get the name of the application that owns this key store.
     *
     * @return The application name.
     */
    const qcc::String& GetApplication() { return application; }

    /**
     * Clear all keys from the key store.
     *
     * @return  ER_OK if the key store was cleared.
     */
    QStatus Clear();

    /**
     * Reset the state of
     *
     * @return  ER_OK if the state was cleared
     */
    QStatus Reset();

    /**
     * Pull keys into the key store from a source.
     *
     * @param source    The source to read the keys from.
     * @param password  The password required to decrypt the key store
     *
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     *
     */
    QStatus Pull(qcc::Source& source, const qcc::String& password);

    /**
     * Push the current keys from the key store into a sink
     *
     * @param sink The sink to write the keys to.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Push(qcc::Sink& sink);

    /**
     * Indicates if this is a shared key store.
     *
     * @return  Returns true if the key store is shared between multiple applications.
     */
    bool IsShared() { return shared; }

    /**
     * Search for associated keys with the given guid
     * @param guid  The header guid
     * @param list  The output list of associated guids.  This list must be deallocated after used.
     * @param numItems The output size of the list
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus SearchAssociatedKeys(const qcc::GUID128& guid, qcc::GUID128** list, size_t* numItems);

  private:

    /**
     * Assignment not allowed
     */
    KeyStore& operator=(const KeyStore& other);

    /**
     * Copy constructor not allowed
     */
    KeyStore(const KeyStore& other);

    /**
     * Default constructor not allowed
     */
    KeyStore();

    /**
     * Internal function to erase expired keys
     */
    size_t EraseExpiredKeys();

    /**
     * Internal Load function
     */
    QStatus Load();

    /**
     * The application that owns this key store. If the key store is shared this will be the name
     * of a suite of applications.
     */
    qcc::String application;

    /**
     * State of the key store
     */
    enum {
        UNAVAILABLE, /**< Key store has not been loaded */
        LOADED,      /**< Key store is loaded */
        MODIFIED     /**< Key store has been modified since it was loaded */
    } storeState;

    /**
     *  Type for a key record
     */
    class KeyRecord {
      public:
        KeyRecord() : revision(0) { }
        uint32_t revision;       ///< Revision number when this key was added
        qcc::KeyBlob key;        ///< The key blob for the key
        uint8_t accessRights[4]; ///< Access rights associated with this record (see PeerState)
    };

    /**
     * Type for a key map
     */
    typedef std::map<qcc::GUID128, KeyRecord> KeyMap;

    /**
     * In memory copy of the key store
     */
    KeyMap* keys;

    /**
     * GUID for keys that have been deleted
     */
    std::set<qcc::GUID128> deletions;

    /**
     * Default listener for handling load/store requests
     */
    KeyStoreListener* defaultListener;

    /**
     * Listener for handling load/store requests
     */
    ProtectedKeyStoreListener* listener;

    /**
     * The guid for this key store
     */
    qcc::GUID128 thisGuid;

    /**
     * Mutex to protect key store
     */
    qcc::Mutex lock;

    /**
     * Key for encrypting/decrypting the key store.
     */
    qcc::KeyBlob* keyStoreKey;

    /**
     * Revision number for the key store
     */
    uint32_t revision;

    /**
     * Indicates if the key store is shared between multiple applications.
     */
    bool shared;

    /**
     * Event for synchronizing store requests
     */
    qcc::Event* stored;

    /**
     * Event for synchronizing load requests
     */
    qcc::Event* loaded;

    /* the key event listener */
    KeyStoreKeyEventListener* keyEventListener;

};

class KeyStoreKeyEventListener {
  public:
    KeyStoreKeyEventListener() { }
    virtual ~KeyStoreKeyEventListener() { }

    /**
     * When the keystore auto delete a key because of expiration, this listener will be notified.
     * @param holder the keystore
     * @param guid the effected guid
     * @return true if there are other guids affected; false, if only this guid is effected.
     */

    virtual bool NotifyAutoDelete(KeyStore* holder, const qcc::GUID128& guid);
};

}

#endif
