/**
 * @file
 * The KeyStore class manages the storing and loading of key blobs from
 * external storage. The default implementation stores key blobs in a file.
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

#include <map>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Crypto.h>
#include <qcc/Environ.h>
#include <qcc/FileStream.h>
#include <qcc/KeyBlob.h>
#include <qcc/Util.h>
#include <qcc/StringSource.h>
#include <qcc/StringSink.h>
#include <qcc/Thread.h>

#include <alljoyn/KeyStoreListener.h>

#include "PeerState.h"
#include "KeyStore.h"
#include "BusUtil.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {


/*
 * Lowest version number we can read
 */
static const uint16_t LowStoreVersion = 0x0102;

/*
 * key store version (or any other older) that did not account for guid in secret
 * In the key store version x103 or order, there are some occurrences where the
 * GUID string used in the encryption key for the keystore can be empty
 * because the use of the GetGuid().
 */
static const uint16_t MixupGuidKeyStoreVersion = 0x0103;

/**
 * the key store version where the key is a composite key instead of just
 * the GUID.
 */
static const uint16_t CompositeKeyKeyStoreVersion = 0x0104;

/*
 * Current key store version we will write
 */
static const uint16_t KeyStoreVersion = 0x0104;

QStatus KeyStoreListener::PutKeys(KeyStore& keyStore, const qcc::String& source, const qcc::String& password)
{
    StringSource stringSource(source);
    return keyStore.Pull(stringSource, password);
}

QStatus KeyStoreListener::GetKeys(KeyStore& keyStore, qcc::String& sink)
{
    StringSink stringSink;
    QStatus status = keyStore.Push(stringSink);
    if (status == ER_OK) {
        sink = stringSink.GetString();
    }
    return status;
}

KeyStore::KeyStore(const qcc::String& application) :
    application(application),
    storeState(UNAVAILABLE),
    keys(new KeyMap),
    defaultListener(NULL),
    listener(NULL),
    thisGuid(),
    keyStoreKey(NULL),
    shared(false),
    stored(NULL),
    storedRefCount(0),
    loaded(NULL),
    loadedRefCount(0),
    keyEventListener(NULL),
    useDefaultListener(false),
    guidSetEvent(NULL),
    guidSet(false),
    guidSetRefCount(0)
{
}

KeyStore::~KeyStore()
{
    /* Unblock thread that might be waiting for a store to complete */
    lock.Lock(MUTEX_CONTEXT);
    if (stored) {
        stored->SetEvent();
        lock.Unlock(MUTEX_CONTEXT);
        while (stored) {
            qcc::Sleep(1);
        }
        lock.Lock(MUTEX_CONTEXT);
    }
    /* Unblock thread that might be waiting for a load to complete */
    if (loaded) {
        loaded->SetEvent();
        lock.Unlock(MUTEX_CONTEXT);
        while (loaded) {
            qcc::Sleep(1);
        }
        lock.Lock(MUTEX_CONTEXT);
    }
    lock.Unlock(MUTEX_CONTEXT);
    /* Unblock thread that might be waiting for a guid set to complete */
    if (guidSetEvent) {
        guidSetEvent->SetEvent();
        while (guidSetEvent) {
            qcc::Sleep(1);
        }
    }
    delete defaultListener;
    delete listener;
    delete keyStoreKey;
    delete keys;
}

QStatus KeyStore::SetListener(KeyStoreListener& keyStoreListener)
{
    /**
     * only allow to set new listener when
     *    listener was not previously set
     *    listener was set with the default listener
     */

    lock.Lock(MUTEX_CONTEXT);
    bool setIt = false;
    if (this->listener != NULL) {
        if (useDefaultListener) {
            setIt = true;
        }
    } else {
        setIt = true;
    }
    if (setIt) {
        delete this->listener;
        this->listener = new ProtectedKeyStoreListener(&keyStoreListener);
        useDefaultListener = false;
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return ER_BUS_LISTENER_ALREADY_SET;
}

/**
 * Setup the key event listener.
 *
 * @param listener  The listener that will listen to key event.
 */
QStatus KeyStore::SetKeyEventListener(KeyStoreKeyEventListener* eventListener) {
    keyEventListener = eventListener;
    return ER_OK;
}

QStatus KeyStore::SetDefaultListener()
{
    lock.Lock(MUTEX_CONTEXT);
    delete this->listener;
    this->listener = new ProtectedKeyStoreListener(defaultListener);
    useDefaultListener = true;
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

QStatus KeyStore::Reset()
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState != UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        /* clear the keys first */
        QStatus status = Clear();
        if (ER_OK != status) {
            return status;
        }
        lock.Lock(MUTEX_CONTEXT);
        storeState = UNAVAILABLE;
        delete listener;
        listener = NULL;
        delete defaultListener;
        defaultListener = NULL;
        shared = false;
        useDefaultListener = false;
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    } else {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_FAIL;
    }
}

QStatus KeyStore::Init(const char* fileName, bool isShared)
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        if (listener == NULL) {
            defaultListener = KeyStoreListenerFactory::CreateInstance(application, fileName);
            listener = new ProtectedKeyStoreListener(defaultListener);
        }
        shared = isShared;
        lock.Unlock(MUTEX_CONTEXT);
        return Load();
    } else {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_KEY_STORE_ALREADY_INITIALIZED;
    }
}

QStatus KeyStore::Store()
{
    QStatus status = ER_OK;

    lock.Lock(MUTEX_CONTEXT);
    /* Cannot store if never loaded */
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    /* Don't store if not modified */
    if (storeState != MODIFIED) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }
    EraseExpiredKeys();

    /* Reload to merge keystore changes before storing */
    if (revision > 0) {
        lock.Unlock(MUTEX_CONTEXT);
        status = Reload();
        lock.Lock(MUTEX_CONTEXT);
    }
    if (status == ER_OK) {
        if (storedRefCount == 0) {
            stored = new Event();
        }
        storedRefCount++;
        lock.Unlock(MUTEX_CONTEXT);
        status = listener->StoreRequest(*this);
        if (status == ER_OK) {
            status = Event::Wait(*stored);
        }
        lock.Lock(MUTEX_CONTEXT);
        storedRefCount--;
        if (storedRefCount == 0) {
            delete stored;
            stored = NULL;
        }
        /* Done tracking deletions */
        deletions.clear();
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::Load()
{
    QStatus status;
    lock.Lock(MUTEX_CONTEXT);
    keys->clear();
    storeState = UNAVAILABLE;
    if (loadedRefCount == 0) {
        loaded = new Event();
    }
    loadedRefCount++;
    lock.Unlock(MUTEX_CONTEXT);
    status = listener->LoadRequest(*this);
    if (status == ER_OK) {
        status = Event::Wait(*loaded);
    }
    lock.Lock(MUTEX_CONTEXT);
    loadedRefCount--;
    if (loadedRefCount == 0) {
        delete loaded;
        loaded = NULL;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

/* private method assumes lock is already acquired by the caller. */
size_t KeyStore::EraseExpiredKeys()
{
    size_t count = 0;
    bool dirty = true;
    while (dirty) {
        dirty = false;
        KeyMap::iterator it = keys->begin();
        while (it != keys->end()) {
            KeyMap::iterator current = it++;
            if (current->second.keyBlob.HasExpired()) {
                QCC_DbgPrintf(("Deleting expired key for GUID %s", current->first.ToString().c_str()));
                bool affected = false;
                if (keyEventListener) {
                    affected = keyEventListener->NotifyAutoDelete(this, current->first);
                }
                keys->erase(current);
                ++count;
                dirty = true;
                if (affected) {
                    break;  /* need to refresh the iterator because the list members may have changed by the NotifyAutoDelete call */
                }
            }
        }
    } /* clean sweep */

    return count;
}

QStatus KeyStore::Pull(Source& source, const qcc::String& password)
{
    QCC_DbgPrintf(("KeyStore::Pull"));


    /* Don't load if already loaded */
    lock.Lock(MUTEX_CONTEXT);
    if (storeState != UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }

    uint8_t guidBuf[qcc::GUID128::SIZE];
    size_t pulled;
    size_t len = 0;
    uint16_t version;

    /* Pull and check the key store version */
    QStatus status = source.PullBytes(&version, sizeof(version), pulled);
    if ((status == ER_OK) && ((version > KeyStoreVersion) || (version < LowStoreVersion))) {
        status = ER_BUS_KEYSTORE_VERSION_MISMATCH;
        QCC_LogError(status, ("Keystore has wrong version expected %d got %d", KeyStoreVersion, version));
    }
    /* Pull the revision number */
    if (status == ER_OK) {
        status = source.PullBytes(&revision, sizeof(revision), pulled);
    }
    /* Pull the application GUID */
    if (status == ER_OK) {
        status = source.PullBytes(guidBuf, qcc::GUID128::SIZE, pulled);
        thisGuid.SetBytes(guidBuf);
        MarkGuidSet();
    }

    /* This is the only chance to generate the key store key */
    if (!keyStoreKey) {
        keyStoreKey = new KeyStoreEncryptionKey();
    }
    keyStoreKey->Build(password, thisGuid.ToString());


    /* Allow for an uninitialized (empty) key store */
    if (status == ER_EOF) {
        keys->clear();
        storeState = MODIFIED;
        revision = 0;
        MarkGuidSet();  /* make thisGuid the keystore guid */
        status = ER_OK;
        goto ExitPull;
    }
    if (status != ER_OK) {
        goto ExitPull;
    }
    QCC_DbgPrintf(("KeyStore::Pull (revision %d)", revision));
    /* Get length of the encrypted keys */
    status = source.PullBytes(&len, sizeof(len), pulled);
    if (status != ER_OK) {
        goto ExitPull;
    }
    /* Sanity check on the length */
    if (len > 64000) {
        status = ER_BUS_CORRUPT_KEYSTORE;
        goto ExitPull;
    }
    if (len > 0) {
        uint8_t* data = NULL;
        /*
         * Pull the encrypted keys.
         */
        data = new uint8_t[len];
        status = source.PullBytes(data, len, pulled);
        if (pulled != len) {
            status = ER_BUS_CORRUPT_KEYSTORE;
        }
        if (status == ER_OK) {
            /*
             * Decrypt the key store.
             */
            /**
             * In the key store version 0x103 or older, the encryption secret was
             * supposed to be the password + the key store guid.  However, the call
             * the retrieve the guid may return empty (when the keystore is in
             * loading mode) so it used just the password.
             * However, the non-empty guid string is actually written to the listener
             * so the decryption will fail because of key mismatch.
             */
            bool mayUseAlternateKey = (version <= MixupGuidKeyStoreVersion);
            uint8_t* altData = NULL;
            size_t altLen = len;
            if (mayUseAlternateKey) {
                altData = new uint8_t[altLen];
                /* copy the data in case to decrypt with an alternate key.  The data needs to be copied since the call aes.Decrypt_CCM below overwrites the data buffer */
                memcpy(altData, data, altLen);
            }
            KeyBlob nonce((uint8_t*)&revision, sizeof(revision), KeyBlob::GENERIC);
            Crypto_AES aes(*keyStoreKey, Crypto_AES::CCM);
            status = aes.Decrypt_CCM(data, data, len, nonce, NULL, 0, 16);
            if ((status != ER_OK) && mayUseAlternateKey) {
                KeyBlob altKey;
                altKey.Derive(password, Crypto_AES::AES128_SIZE, KeyBlob::AES);
                Crypto_AES altAES(altKey, Crypto_AES::CCM);
                status = altAES.Decrypt_CCM(altData, data, altLen, nonce, NULL, 0, 16);
            }
            delete [] altData;

            /*
             * Unpack the guid/key pairs from an intermediate string source.
             */
            StringSource strSource(data, len);
            while (status == ER_OK) {
                uint32_t rev;
                status = strSource.PullBytes(&rev, sizeof(rev), pulled);
                Key::KeyType keyType = Key::REMOTE;
                if ((status == ER_OK) && (version >= CompositeKeyKeyStoreVersion)) {
                    status = strSource.PullBytes(&keyType, sizeof(keyType), pulled);
                }
                if (status == ER_OK) {
                    status = strSource.PullBytes(guidBuf, qcc::GUID128::SIZE, pulled);
                }
                if (status == ER_OK) {
                    qcc::GUID128 guid(0);
                    guid.SetBytes(guidBuf);
                    Key key(keyType, guid);
                    KeyRecord& keyRec = (*keys)[key];
                    keyRec.revision = rev;
                    status = keyRec.keyBlob.Load(strSource);
                    if (status == ER_OK) {
                        if (version > LowStoreVersion) {
                            status = strSource.PullBytes(&keyRec.accessRights, sizeof(keyRec.accessRights), pulled);
                        } else {
                            /*
                             * Maintain backwards compatibility with an older key store
                             */
                            for (size_t i = 0; i < ArraySize(keyRec.accessRights); ++i) {
                                keyRec.accessRights[i] = _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX;
                            }
                        }
                    }
                    QCC_DbgPrintf(("KeyStore::Pull rev:%d GUID %s %s", rev, QCC_StatusText(status), guid.ToString().c_str()));
                }
            }
            if (status == ER_EOF) {
                status = ER_OK;
            }
        }
        delete [] data;
    }
    if (status != ER_OK) {
        goto ExitPull;
    }
    if (EraseExpiredKeys()) {
        storeState = MODIFIED;
    } else {
        storeState = LOADED;
    }

ExitPull:

    if (status != ER_OK) {
        keys->clear();
        storeState = MODIFIED;
    }
    if (loaded) {
        loaded->SetEvent();
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::Clear()
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    keys->clear();
    storeState = MODIFIED;
    revision = 0;
    deletions.clear();
    lock.Unlock(MUTEX_CONTEXT);
    listener->StoreRequest(*this);
    return ER_OK;
}

static bool MatchesPrefix(const String& str, const String& prefixPattern)
{
    return !WildcardMatch(str, prefixPattern);
}

QStatus KeyStore::Clear(const qcc::String& tagPrefixPattern)
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    bool dirty = true;
    QStatus status = ER_OK;
    while (dirty) {
        dirty = false;
        for (KeyMap::iterator it = keys->begin(); it != keys->end(); it++) {
            if (it->second.keyBlob.GetTag().empty()) {
                continue;  /* skip the untag */
            }
            if (MatchesPrefix(it->second.keyBlob.GetTag(), tagPrefixPattern)) {
                status = DeleteKey(it->first);
                if (ER_OK != status) {
                    break;
                }
                dirty = true;
                break;  /* redo the loop since the iterator is out-of-sync */
            }
        }
        if (ER_OK != status) {
            break;
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
    if (ER_OK == status) {
        listener->StoreRequest(*this);
    }
    return status;
}

QStatus KeyStore::Reload()
{
    QCC_DbgHLPrintf(("KeyStore::Reload"));

    /*
     * Cannot reload if the key store has never been loaded
     */
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    /*
     * Reload is defined to be a no-op for non-shared key stores
     */
    if (!shared) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }

    QStatus status;
    uint32_t currentRevision = revision;
    KeyMap* currentKeys = keys;
    keys = new KeyMap();

    /*
     * Load the keys so we can check for changes and merge if needed
     */
    lock.Unlock(MUTEX_CONTEXT);
    status = Load();
    lock.Lock(MUTEX_CONTEXT);

    /*
     * Check if key store has been changed since we last touched it.
     */
    if ((status == ER_OK) && (revision > currentRevision)) {
        QCC_DbgHLPrintf(("KeyStore::Reload merging changes"));
        KeyMap::iterator it;
        /*
         * Handle deletions
         */
        std::set<Key>::iterator itDel;
        for (itDel = deletions.begin(); itDel != deletions.end(); ++itDel) {
            it = keys->find(*itDel);
            if ((it != keys->end()) && (it->second.revision <= currentRevision)) {
                QCC_DbgPrintf(("KeyStore::Reload deleting %s", itDel->ToString().c_str()));
                keys->erase(*itDel);
            }
        }
        /*
         * Handle additions and updates
         */
        for (it = currentKeys->begin(); it != currentKeys->end(); ++it) {
            if (it->second.revision > currentRevision) {
                QCC_DbgPrintf(("KeyStore::Reload added rev:%d %s", it->second.revision, it->first.ToString().c_str()));
                if ((*keys)[it->first].revision > currentRevision) {
                    /*
                     * In case of a merge conflict go with the key that is currently stored
                     */
                    QCC_DbgPrintf(("KeyStore::Reload merge conflict rev:%d %s", it->second.revision, it->first.ToString().c_str()));
                } else {
                    (*keys)[it->first] = it->second;
                    QCC_DbgPrintf(("KeyStore::Reload merging %s", it->first.ToString().c_str()));
                }
            }
        }
        delete currentKeys;
        EraseExpiredKeys();
    } else {
        /*
         * Restore state
         */
        KeyMap* goner = keys;
        keys = currentKeys;
        delete goner;
        revision = currentRevision;
    }

    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::Push(Sink& sink)
{
    size_t pushed;
    QStatus status = ER_OK;

    QCC_DbgHLPrintf(("KeyStore::Push (revision %d)", revision + 1));
    lock.Lock(MUTEX_CONTEXT);

    /*
     * Pack the keys into an intermediate string sink.
     */
    StringSink strSink;
    KeyMap::iterator it;
    for (it = keys->begin(); it != keys->end(); ++it) {
        strSink.PushBytes(&it->second.revision, sizeof(revision), pushed);
        Key::KeyType keyType = it->first.GetType();
        strSink.PushBytes(&keyType, sizeof(keyType), pushed);
        strSink.PushBytes(it->first.GetGUID().GetBytes(), qcc::GUID128::SIZE, pushed);
        it->second.keyBlob.Store(strSink);

        strSink.PushBytes(&it->second.accessRights, sizeof(it->second.accessRights), pushed);
        QCC_DbgPrintf(("KeyStore::Push rev:%d key %s", it->second.revision, it->first.ToString().c_str()));
    }
    size_t keysLen = strSink.GetString().size();
    /*
     * First two bytes are the version number.
     */
    status = sink.PushBytes(&KeyStoreVersion, sizeof(KeyStoreVersion), pushed);
    if (status != ER_OK) {
        goto ExitPush;
    }
    /*
     * Second two bytes are the key store revision number. The revision number is incremented each
     * time the key store is stored.
     */
    ++revision;
    status = sink.PushBytes(&revision, sizeof(revision), pushed);
    if (status != ER_OK) {
        goto ExitPush;
    }
    /*
     * Store the GUID
     */
    if (status == ER_OK) {
        status = sink.PushBytes(thisGuid.GetBytes(), qcc::GUID128::SIZE, pushed);
        MarkGuidSet(); /* now thisGuid is active */
    }
    if (status != ER_OK) {
        goto ExitPush;
    }

    if (keysLen > 0) {
        /*
         * Encrypt keys.
         */
        KeyBlob nonce((uint8_t*)&revision, sizeof(revision), KeyBlob::GENERIC);
        uint8_t* keysData = new uint8_t[keysLen + 16];
        Crypto_AES aes(*keyStoreKey, Crypto_AES::CCM);
        status = aes.Encrypt_CCM(strSink.GetString().data(), keysData, keysLen, nonce, NULL, 0, 16);
        /* Store the length of the encrypted keys */
        if (status == ER_OK) {
            status = sink.PushBytes(&keysLen, sizeof(keysLen), pushed);
        }
        /* Store the encrypted keys */
        if (status == ER_OK) {
            status = sink.PushBytes(keysData, keysLen, pushed);
        }
        delete [] keysData;
    } else {
        status = sink.PushBytes(&keysLen, sizeof(keysLen), pushed);
    }
    if (status != ER_OK) {
        goto ExitPush;
    }
    storeState = LOADED;

ExitPush:

    if (stored) {
        stored->SetEvent();
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::GetKey(const Key& key, KeyBlob& keyBlob, uint8_t accessRights[4])
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QStatus status;
    QCC_DbgPrintf(("KeyStore::GetKey %s", key.ToString().c_str()));
    if (keys->find(key) != keys->end()) {
        KeyRecord& keyRec = (*keys)[key];
        keyBlob = keyRec.keyBlob;
        memcpy(accessRights, &keyRec.accessRights, sizeof(uint8_t) * 4);
        QCC_DbgPrintf(("AccessRights %1x%1x%1x%1x", accessRights[0], accessRights[1], accessRights[2], accessRights[3]));
        status = ER_OK;
    } else {
        status = ER_BUS_KEY_UNAVAILABLE;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

bool KeyStore::HasKey(const Key& key)
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return false;
    }
    bool hasKey = keys->count(key) != 0;
    lock.Unlock(MUTEX_CONTEXT);
    return hasKey;
}

QStatus KeyStore::AddKey(const Key& key, const KeyBlob& keyBlob, const uint8_t accessRights[4])
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_DbgPrintf(("KeyStore::AddKey %s", key.ToString().c_str()));
    KeyRecord& keyRec = (*keys)[key];
    keyRec.revision = revision + 1;
    keyRec.keyBlob = keyBlob;
    QCC_DbgPrintf(("AccessRights %1x%1x%1x%1x", accessRights[0], accessRights[1], accessRights[2], accessRights[3]));
    memcpy(&keyRec.accessRights, accessRights, sizeof(uint8_t) * 4);
    storeState = MODIFIED;
    deletions.erase(key);
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

/**
 * This internal method deletes the key assuming the lock is already acquired.
 */
QStatus KeyStore::DeleteKey(const Key& key)
{
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_DbgPrintf(("KeyStore::DeleleKey %s", key.ToString().c_str()));
    /* Use a local copy because erase might destroy the key */
    Key keyCopy(key);
    keys->erase(key);
    storeState = MODIFIED;
    deletions.insert(keyCopy);
    return ER_OK;
}

QStatus KeyStore::DelKey(const Key& key)
{
    lock.Lock(MUTEX_CONTEXT);
    QStatus status = DeleteKey(key);
    lock.Unlock(MUTEX_CONTEXT);
    if (ER_OK != status) {
        return status;
    }
    listener->StoreRequest(*this);
    return ER_OK;
}

QStatus KeyStore::SetKeyExpiration(const Key& key, const Timespec<qcc::EpochTime>& expiration)
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QStatus status = ER_OK;
    QCC_DbgPrintf(("KeyStore::SetExpiration %s", key.ToString().c_str()));
    if (keys->count(key) != 0) {
        (*keys)[key].keyBlob.SetExpiration(expiration);
        storeState = MODIFIED;
    } else {
        status = ER_BUS_KEY_UNAVAILABLE;
    }
    lock.Unlock(MUTEX_CONTEXT);
    if (status == ER_OK) {
        listener->StoreRequest(*this);
    }
    return status;
}

QStatus KeyStore::GetKeyExpiration(const Key& key, Timespec<EpochTime>& expiration)
{
    lock.Lock(MUTEX_CONTEXT);
    if (storeState == UNAVAILABLE) {
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    lock.Unlock(MUTEX_CONTEXT);
    /*
     * For shared key stores we may need to do a reload before checking for key expiration.
     */
    QStatus status = Reload();
    if (status == ER_OK) {
        lock.Lock(MUTEX_CONTEXT);
        QCC_DbgPrintf(("KeyStore::GetExpiration %s", key.ToString().c_str()));
        if (keys->count(key) != 0) {
            (*keys)[key].keyBlob.GetExpiration(expiration);
        } else {
            status = ER_BUS_KEY_UNAVAILABLE;
        }
        lock.Unlock(MUTEX_CONTEXT);
    }
    return status;
}

QStatus KeyStore::SearchAssociatedKeys(const Key& key, Key** list, size_t* numItems)
{
    size_t count = 0;
    lock.Lock(MUTEX_CONTEXT);
    for (KeyMap::iterator it = keys->begin(); it != keys->end(); ++it) {
        if ((it->second.keyBlob.GetAssociationMode() != KeyBlob::ASSOCIATE_MEMBER)
            && (it->second.keyBlob.GetAssociationMode() != KeyBlob::ASSOCIATE_BOTH)) {
            continue;
        }
        if (it->second.keyBlob.GetAssociation() == key.GetGUID()) {
            count++;
        }
    }
    if (count == 0) {
        *numItems = count;
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }

    Key* keyList = new Key[count];
    size_t idx = 0;
    for (KeyMap::iterator it = keys->begin(); it != keys->end(); ++it) {
        if ((it->second.keyBlob.GetAssociationMode() != KeyBlob::ASSOCIATE_MEMBER)
            && (it->second.keyBlob.GetAssociationMode() != KeyBlob::ASSOCIATE_BOTH)) {
            continue;
        }
        if (it->second.keyBlob.GetAssociation() == key.GetGUID()) {
            if (idx >= count) { /* bound check */
                delete [] keyList;
                lock.Unlock(MUTEX_CONTEXT);
                return ER_FAIL;
            }
            keyList[idx++] = it->first;
        }
    }
    *numItems = count;
    *list = keyList;
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

QStatus KeyStore::WaitForGuidSet()
{
    if (guidSet) {
        return ER_OK;
    }
    guidSetEventLock.Lock(MUTEX_CONTEXT);
    if (guidSetRefCount == 0) {
        guidSetEvent = new Event();
    }
    guidSetRefCount++;
    guidSetEventLock.Unlock(MUTEX_CONTEXT);
    Event::Wait(*guidSetEvent);
    guidSetEventLock.Lock(MUTEX_CONTEXT);
    guidSetRefCount--;
    if (guidSetRefCount == 0) {
        delete guidSetEvent;
        guidSetEvent = NULL;
    }
    guidSetEventLock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

void KeyStore::MarkGuidSet()
{
    guidSet = true;
    if (guidSetEvent) {
        guidSetEvent->SetEvent();
    }
}

void KeyStore::KeyStoreEncryptionKey::Build(const qcc::String& password, const qcc::String& guidString)
{
    SetPassword(password);
    SetGuidString(guidString);
    Build();
}

void KeyStore::KeyStoreEncryptionKey::Build()
{
    Derive(password + guidString, Crypto_AES::AES128_SIZE, KeyBlob::AES);
}

}
