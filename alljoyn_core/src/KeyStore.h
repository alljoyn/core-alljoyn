/**
 * @file
 * The KeyStore class manages the storing and loading of key blobs from external storage.
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
     * the key store index.
     */
    class Key {
      public:
        /**
         *  the key type
         */
        typedef enum {
            REMOTE,  /**< remote source */
            LOCAL    /**< local source */
        } KeyType;

        /* constructor */
        Key() : type(Key::LOCAL), guid(0)
        {
        }

        /* constructor */
        Key(const KeyType type) : type(type), guid(0)
        {
        }
        /* constructor */
        Key(const KeyType type, const qcc::GUID128& guid) : type(type), guid(guid)
        {
        }

        /* copy constructor */
        Key(const Key& other) : type(other.type), guid(other.guid)
        {
        }

        /**
         * Equals operator for the key
         *
         * @param[in] other the other key to compare
         *
         * @return true if both keys are equal
         */
        bool operator==(const Key& other) const
        {
            if (type != other.type) {
                return false;
            }
            return guid == other.guid;
        }

        /**
         * Less than operator for the key
         *
         * @param[in] other the other key to compare
         *
         * @return true if this key is less than the other key
         */
        bool operator<(const Key& other) const
        {
            if (guid < other.guid) {
                return true;
            } else if (guid == other.guid) {
                return (type < other.type);
            }
            return false;
        }

        void SetType(const KeyType type)
        {
            this->type = type;
        }

        const KeyType& GetType() const
        {
            return type;
        }

        void SetGUID(const qcc::GUID128& guid)
        {
            this->guid = guid;
        }

        const qcc::GUID128& GetGUID() const
        {
            return guid;
        }

        /**
         * ToString
         */

        qcc::String ToString() const
        {
            qcc::String str;
            if (type == LOCAL) {
                str = "L:";
            } else if (type == REMOTE) {
                str = "R:";
            }
            str += guid.ToString();
            return str;
        }

      private:
        KeyType type;
        qcc::GUID128 guid;
    };

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
     * @param key         The unique identifier for the key
     * @param keyBlob          The key blob to get
     * @param accessRights The access rights associated with the key
     * @return
     *      - ER_OK if successful
     *      - ER_BUS_KEY_UNAVAILABLE if key is unavailable
     *      - ER_BUS_KEY_EXPIRED if the requested key has expired
     */
    QStatus GetKey(const Key &key, qcc::KeyBlob & keyBlob, uint8_t accessRights[4]);

    /**
     * Get a key blob from the key store
     *
     * @param key         The unique identifier for the key
     * @param keyBlob     The key blob to get
     * @return
     *      - ER_OK if successful
     *      - ER_BUS_KEY_UNAVAILABLE if key is unavailable
     *      - ER_BUS_KEY_EXPIRED if the requested key has expired
     */
    QStatus GetKey(const Key& key, qcc::KeyBlob& keyBlob)
    {
        uint8_t accessRights[4];
        return GetKey(key, keyBlob, accessRights);
    }

    /**
     * Add a key blob to the key store
     *
     * @param key         The unique identifier for the key
     * @param keyBlob     The key blob to add
     * @param accessRights The access rights associated with the key
     * @return ER_OK
     */
    QStatus AddKey(const Key& key, const qcc::KeyBlob& keyBlob, const uint8_t accessRights[4]);

    /**
     * Add a key blob to the key store
     *
     * @param key         The unique identifier for the key
     * @param keyBlob     The key blob to add
     * @return ER_OK
     */
    QStatus AddKey(const Key& key, const qcc::KeyBlob& keyBlob)
    {
        const uint8_t accessRights[4] = { 0, 0, 0, 0 };
        return AddKey(key, keyBlob, accessRights);
    }

    /**
     * Remove a key blob from the key store
     *
     * @param key  The unique identifier for the key
     * return ER_OK
     */
    QStatus DelKey(const Key& key);

    /**
     * Set expiration time on a key blob from the key store
     *
     * @param key        The unique identifier for the key
     * @param expiration  Time to expire the key
     * return ER_OK
     */
    QStatus SetKeyExpiration(const Key& key, const qcc::Timespec& expiration);

    /**
     * Get expiration time on a key blob from the key store
     *
     * @param key        The unique identifier for the key
     * @param expiration  Time the key will expire
     * return ER_OK
     */
    QStatus GetKeyExpiration(const Key& key, qcc::Timespec& expiration);

    /**
     * Test is there is a requested key blob in the key store
     *
     * @param key  The unique identifier for the key
     * @return      Returns true if the requested key is in the key store.
     */
    bool HasKey(const Key& key);

    /**
     * Return the GUID for this key store
     * @param[out] guid return the GUID for this key store
     * @return
     *      - ER_OK on success
     *      - ER_BUS_KEY_STORE_NOT_LOADED if the GUID is not available
     */
    QStatus GetGuid(qcc::GUID128& guid)
    {
        WaitForGuidSet();
        if (!guidSet) {
            return ER_KEY_STORE_ID_NOT_YET_SET;
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
    qcc::String GetGuid()
    {
        WaitForGuidSet();
        return !guidSet ? "" : thisGuid.ToString();
    }

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
     * Clear keys with tag with prefix match.
     * @param tagPrefixPattern the tag prefix pattern to compare
     *
     * @return  ER_OK if the operation is successful
     */
    QStatus Clear(const qcc::String& tagPrefixPattern);

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
     * Search for associated keys with the given key
     * @param key  The header key
     * @param list  The output list of associated keys.  This list must be deallocated after used.
     * @param numItems The output size of the list
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus SearchAssociatedKeys(const Key& key, Key** list, size_t* numItems);

  private:

    class KeyStoreEncryptionKey : public qcc::KeyBlob {
      public:
        KeyStoreEncryptionKey() : qcc::KeyBlob()
        {
        }
        virtual ~KeyStoreEncryptionKey()
        {
        }

        /**
         * Set the password
         */
        void SetPassword(const qcc::String& password)
        {
            this->password = password;
        }

        /**
         * Set the guid
         */
        void SetGuidString(const qcc::String& guidString)
        {
            this->guidString = guidString;
        }

        /**
         * build the encryption key
         */
        void Build();

        /**
         * build the encryption key
         * @param password the password
         * @param guid the guid
         */
        void Build(const qcc::String& password, const qcc::String& guidString);

      private:
        /**
         * Assignment not allowed
         */
        KeyStoreEncryptionKey& operator=(const KeyStoreEncryptionKey& other);

        /**
         * Copy constructor not allowed
         */
        KeyStoreEncryptionKey(const KeyStoreEncryptionKey& other);

        qcc::String password;
        qcc::String guidString;
    };

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
     * wait for the guid to set
     */
    QStatus WaitForGuidSet();

    /**
     * mark that the guid is set so all the waiters can retrieve the GUID.
     */
    void MarkGuidSet();

    /**
     * synchronized method to invoke the listener load request
     */
    QStatus SendLoadRequest(bool waitFor);

    /**
     * Delete the key from the internal structure.  The lock must be acquired prior to calling this method.
     *
     * @param key  The unique identifier for the key
     * return ER_OK
     */
    QStatus DeleteKey(const Key& key);

    /**
     * synchronized method to invoke the listener store request
     */
    QStatus SendStoreRequest(bool waitFor);

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
        qcc::KeyBlob keyBlob;        ///< The key blob for the key
        uint8_t accessRights[4]; ///< Access rights associated with this record (see PeerState)
    };

    /**
     * Type for a key map
     */
    typedef std::map<Key, KeyRecord> KeyMap;

    /**
     * In memory copy of the key store
     */
    KeyMap* keys;

    /**
     * GUID for keys that have been deleted
     */
    std::set<Key> deletions;

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
    KeyStoreEncryptionKey* keyStoreKey;

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
     * ref count for the stored event
     */
    uint8_t storedRefCount;

    /**
     * Event for synchronizing load requests
     */
    qcc::Event* loaded;
    /**
     * ref count for the loaded event
     */
    uint8_t loadedRefCount;

    /* the key event listener */
    KeyStoreKeyEventListener* keyEventListener;

    /**
     * The default listener is activated.
     */
    bool useDefaultListener;

    /**
     * Event for synchronizing GetGuid requests
     */
    qcc::Event* guidSetEvent;
    /**
     * Key store guid is set
     */
    bool guidSet;

    /**
     * Mutex to protect guidSetEvent and its data
     */
    qcc::Mutex guidSetEventLock;

    /**
     * guidSet ref count
     */
    uint8_t guidSetRefCount;

};

class KeyStoreKeyEventListener {
  public:
    KeyStoreKeyEventListener() { }
    virtual ~KeyStoreKeyEventListener() { }

    /**
     * When the keystore auto delete a key because of expiration, this listener will be notified.
     * @param holder the keystore
     * @param key the effected key
     * @return true if there are other guids affected; false, if only this guid is effected.
     */

    virtual bool NotifyAutoDelete(KeyStore* holder, const KeyStore::Key& key);
};

class KeyStoreListenerFactory {

  public:
    static KeyStoreListener* CreateInstance(const qcc::String& application, const char* fname);

};

}

#endif
