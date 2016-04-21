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
#include <qcc/Event.h>
#include <qcc/FileStream.h>
#include <qcc/KeyBlob.h>
#include <qcc/LockLevel.h>
#include <qcc/Util.h>
#include <qcc/StringSource.h>
#include <qcc/StringSink.h>
#include <qcc/Thread.h>

#include <alljoyn/KeyStoreListener.h>

#include "PeerState.h"
#include "KeyStore.h"
#include "BusUtil.h"
#include "ProtectedKeyStoreListener.h"

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

/*
 * This is a process-wide lock to protect keystore files. The current implementation has one
 * exclusive lock per process even though there could be multiple keystore files (multiple
 * BusAttachments) used. It is primarily for code simplicity.
 */
static qcc::Mutex* s_exclusiveLock = nullptr;


KeyStoreListener::~KeyStoreListener()
{
}

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

/* This function is used by the listener to implement exclusive lock across proceeses. */
QStatus KeyStoreListener::AcquireExclusiveLock(const char* file, uint32_t line)
{
    /* Cross-process file locking is platform dependent and it is implemented by the derived class. */
    QCC_UNUSED(file);
    QCC_UNUSED(line);
    return ER_OK;
}

/* This function is used by the listener to implement exclusive lock across proceeses. */
void KeyStoreListener::ReleaseExclusiveLock(const char* file, uint32_t line)
{
    /* Cross-process file locking is platform dependent and it is implemented by the derived class. */
    QCC_UNUSED(file);
    QCC_UNUSED(line);
}


void KeyStore::Init()
{
    s_exclusiveLock = new qcc::Mutex(qcc::LOCK_LEVEL_KEYSTORE_EXCLUSIVELOCK);
}

void KeyStore::Shutdown()
{
    delete s_exclusiveLock;
}

KeyStore::KeyStore(const qcc::String& application) :
    application(application),
    storeState(UNAVAILABLE),
    keys(new KeyMap),
    persistentKeys(new KeyMap),
    defaultListener(NULL),
    listener(NULL),
    thisGuid(),
    lock(LOCK_LEVEL_KEYSTORE_LOCK),
    keyStoreKey(NULL),
    revision(0),
    persistentRevision(0),
    stored(NULL),
    storedRefCount(0),
    loaded(NULL),
    loadedRefCount(0),
    keyEventListener(NULL),
    useDefaultListener(false),
    guidSetEvent(NULL),
    guidSet(false),
    guidSetEventLock(LOCK_LEVEL_KEYSTORE_GUIDSETEVENTLOCK),
    guidSetRefCount(0),
    exclusiveLockRefreshState(ExclusiveLockNotHeld)
{
}

KeyStore::~KeyStore()
{
    /* Unblock thread that might be waiting for a store to complete */
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (stored) {
        stored->SetEvent();
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        while (stored) {
            qcc::Sleep(1);
        }
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    }
    /* Unblock thread that might be waiting for a load to complete */
    if (loaded) {
        loaded->SetEvent();
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        while (loaded) {
            qcc::Sleep(1);
        }
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    /* Unblock thread that might be waiting for a guid set to complete */
    if (guidSetEvent) {
        guidSetEvent->SetEvent();
        while (guidSetEvent) {
            qcc::Sleep(1);
        }
    }
    delete defaultListener;
    if (listener != nullptr) {
        listener->DelRef();
        listener = nullptr;
    }
    delete keyStoreKey;
    if (keys) {
        keys->clear();
    }
    delete keys;
    if (persistentKeys) {
        persistentKeys->clear();
    }
    delete persistentKeys;
}

QStatus KeyStore::SetListener(KeyStoreListener& keyStoreListener)
{
    /**
     * We need to acquire the exclusive lock here to ensure that the listener
     * doesn't get removed while someone else is still holding the lock.
     * Calling release on the same listener is necessary here to ensure that
     * the listener's exclusive lock doesn't get orphaned.
     */
    QCC_VERIFY(ER_OK == s_exclusiveLock->Lock(MUTEX_CONTEXT));

    /**
     * only allow to set new listener when
     *    listener was not previously set
     *    listener was set with the default listener
     */

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    bool setIt = false;
    if (this->listener != NULL) {
        if (useDefaultListener) {
            setIt = true;
        }
    } else {
        setIt = true;
    }
    if (setIt) {
        if (listener) {
            listener->DelRef();
            listener = nullptr;
        }
        this->listener = new ProtectedKeyStoreListener(&keyStoreListener);
        useDefaultListener = false;
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(MUTEX_CONTEXT));
        return ER_OK;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(MUTEX_CONTEXT));
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
    /**
     * We need to acquire the exclusive lock here to ensure that the listener
     * doesn't get removed while someone else is still holding the lock.
     * Calling release on the same listener is necessary here to ensure that
     * the listener's exclusive lock doesn't get orphaned.
     */
    QCC_VERIFY(ER_OK == s_exclusiveLock->Lock(MUTEX_CONTEXT));
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (listener) {
        listener->DelRef();
        listener = nullptr;
    }
    this->listener = new ProtectedKeyStoreListener(defaultListener);
    useDefaultListener = true;
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(MUTEX_CONTEXT));
    return ER_OK;
}

QStatus KeyStore::Reset()
{
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState != UNAVAILABLE) {
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        /* clear the keys first */
        QStatus status = Clear();
        if (ER_OK != status) {
            return status;
        }
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
        storeState = UNAVAILABLE;
        if (listener != nullptr) {
            listener->DelRef();
            listener = nullptr;
        }
        delete defaultListener;
        defaultListener = NULL;
        useDefaultListener = false;
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        return status;
    } else {
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        return ER_FAIL;
    }
}

QStatus KeyStore::Init(const char* fileName, bool isShared)
{
    QCC_UNUSED(isShared);
    if (storeState == UNAVAILABLE) {
        if (listener == NULL) {
            defaultListener = KeyStoreListenerFactory::CreateInstance(application, fileName);
            listener = new ProtectedKeyStoreListener(defaultListener);
        }
        return Load();
    } else {
        return ER_KEY_STORE_ALREADY_INITIALIZED;
    }
}

QStatus KeyStore::StoreInternal(std::vector<Key>& expiredKeys)
{
    /* This method assumes exclusive lock is already acquired by the caller. */
    s_exclusiveLock->AssertOwnedByCurrentThread();

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    QCC_ASSERT(exclusiveLockRefreshState != ExclusiveLockNotHeld);
    exclusiveLockRefreshState = ExclusiveLockHeld_Clean;
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    QCC_ASSERT(storeState != UNAVAILABLE);

    /* Don't store if not modified */
    if (storeState != MODIFIED) {
        return ER_OK;
    }

    /* Find out which keys are expired */
    expiredKeys.clear();
    if (keys != nullptr) {
        for (auto& key : *keys) {
            if (key.second.keyBlob.HasExpired()) {
                expiredKeys.push_back(key.first);
            }
        }
    }

    if (storedRefCount == 0) {
        stored = new Event();
    }
    storedRefCount++;

    QStatus status = listener->StoreRequest(*this);
    if (status == ER_OK) {
        status = Event::Wait(*stored);
    }
    storedRefCount--;
    if (storedRefCount == 0) {
        delete stored;
        stored = NULL;
    }

    return status;
}

QStatus KeyStore::LoadPersistentKeys()
{
    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    bool callingLoadRequest = false;
    if (loadedRefCount == 0) {
        loaded = new Event();
        callingLoadRequest = true;
        persistentKeys->clear();
    }
    loadedRefCount++;
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    if (callingLoadRequest) {
        status = listener->LoadRequest(*this);
    } else {
        status = ER_OK;
    }
    if (status == ER_OK) {
        status = Event::Wait(*loaded);
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    loadedRefCount--;
    if (loadedRefCount == 0) {
        delete loaded;
        loaded = NULL;
    }
    if ((status == ER_OK) && (exclusiveLockRefreshState == ExclusiveLockHeld_Dirty)) {
        exclusiveLockRefreshState = ExclusiveLockHeld_Clean;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    return status;
}

/**
 * An instance of the KeyStore has a cached copy in memory and the persistent
 * storage.  The persistent storage can be shared with other processes.
 * Therefore, the Load function loads the data from the persistent storage and
 * merges with its cache.  The following merging rules apply.
 * 1. If the entry is deleted from the cache then it will not be merged.
 * 2. If the entry is in the cache but not in the persistent storage:
 *     a. If the entry is a brand new entry (never persisted before), it will be
 *          merged.
 *     b. The entry is an existing entry (persisted before), it may be deleted
 *          by another process, its revision will be checked against the
 *          persistent storage revision.  The entry be merged only if its
 *          revision is not older than the persistent storage revision.
 */
QStatus KeyStore::Load()
{
    QCC_DbgHLPrintf(("KeyStore::Load"));

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    ExclusiveLockRefreshState refreshState = exclusiveLockRefreshState;
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    /*
     * Having exclusive lock held + "Clean" means there's no need to reload the keystore.
     */
    if (refreshState == ExclusiveLockHeld_Clean) {
        return ER_OK;
    }

    /*
     * Load the keys so we can check for changes and merge if needed.
     * This should really be guarded by a shared lock instead of exclusive lock,
     * but we're just using exclusive lock for now since shared lock is not available.
     */
    QCC_VERIFY(ER_OK == s_exclusiveLock->Lock(MUTEX_CONTEXT));
    QStatus status = LoadPersistentKeys();
    if (ER_OK != status) {
        QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(MUTEX_CONTEXT));
        return status;
    }

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    storeState = UNAVAILABLE;

    /*
     * Check if key store has been changed since we last touched it.
     */
    bool needToMerge = ((persistentRevision > revision) || (!deletions.empty()));

    if (needToMerge) {
        QCC_DbgHLPrintf(("KeyStore::Load merging changes"));
        bool dirty = false;
        /*
         * Handle deletions
         */
        for (std::set<Key>::iterator itDel = deletions.begin(); itDel != deletions.end(); ++itDel) {
            KeyMap::iterator it = persistentKeys->find(*itDel);
            if ((it != persistentKeys->end()) && (it->second.revision <= revision)) {
                QCC_DbgPrintf(("KeyStore::Load deleting %s", itDel->ToString().c_str()));
                persistentKeys->erase(*itDel);
                dirty = true;
            }
        }
        deletions.clear();
        /*
         * Handle additions and updates
         */
        for (KeyMap::iterator it = keys->begin(); it != keys->end(); ++it) {
            bool addIt = false;
            /* take the newer entries */
            if (persistentKeys->find(it->first) == persistentKeys->end()) {
                /* in the current in-memory map but not in the persistent store */
                if (!it->second.persisted) {
                    /* brand new entry that has never been persisted */
                    addIt = true;
                } else {
                    /* the entry could have been deleted from persistent
                     * storage by other process.
                     */
                    if (it->second.revision >= persistentRevision) {
                        /* the in-memory entry may have been updated */
                        addIt = true;
                    }
                }
            } else if ((*persistentKeys)[it->first].revision < it->second.revision) {
                /* persistent entry is older then the in-memory entry.  Need update */
                addIt = true;
            }
            if (addIt) {
                (*persistentKeys)[it->first] = it->second;
                QCC_DbgPrintf(("KeyStore::Load merging %s", it->first.ToString().c_str()));
                dirty = true;
            }
        }
        keys->clear();
        delete keys;
        keys = persistentKeys;
        /* the cache revision is now the same as the persistent storage revision */
        revision = persistentRevision;
        persistentKeys = new KeyMap();
        if (dirty) {
            storeState = MODIFIED;
        } else {
            storeState = LOADED;
        }
    } else {
        /*
         * nothing changes
         */
        persistentKeys->clear();
        storeState = LOADED;
    }

    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(MUTEX_CONTEXT));
    return status;
}

QStatus KeyStore::Pull(Source& source, const qcc::String& password)
{
    QCC_DbgPrintf(("KeyStore::Pull"));

    uint32_t persistentRevisionLocalBuffer = 0;
    uint8_t guidBuf[qcc::GUID128::SIZE] = { };
    size_t pulled = 0;
    size_t len = 0;
    uint16_t version = 0;
    KeyMap pulledKeyRecords;

    /* Pull and check the key store version */
    QStatus status = source.PullBytes(&version, sizeof(version), pulled);
    if ((status == ER_OK) && ((version > KeyStoreVersion) || (version < LowStoreVersion))) {
        status = ER_BUS_KEYSTORE_VERSION_MISMATCH;
        QCC_LogError(status, ("Keystore has wrong version expected %u got %u", KeyStoreVersion, version));
    }
    /* Pull the persistent storage revision number */
    if (status == ER_OK) {
        status = source.PullBytes(&persistentRevisionLocalBuffer, sizeof(persistentRevisionLocalBuffer), pulled);
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

    if (status != ER_OK) {
        goto ExitPull;
    }
    QCC_DbgPrintf(("KeyStore::Pull (revision %u)", persistentRevisionLocalBuffer));
    /* Get length of the encrypted keys */
    status = source.PullBytes(&len, sizeof(len), pulled);
    if (status != ER_OK) {
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
            KeyBlob nonce((uint8_t*)&persistentRevisionLocalBuffer, sizeof(persistentRevisionLocalBuffer), KeyBlob::GENERIC);
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
                    KeyRecord& keyRec = pulledKeyRecords[key];
                    keyRec.persisted = true;
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

ExitPull:

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (status == ER_OK) {
        persistentRevision = persistentRevisionLocalBuffer;
        /* Populate the keys we've just pulled */
        for (auto& pulledKeyRec : pulledKeyRecords) {
            KeyRecord& keyRecord = pulledKeyRec.second;
            (*persistentKeys)[pulledKeyRec.first] = keyRecord;
        }
    } else {
        persistentKeys->clear();
        /* Allow for an uninitialized (empty) key store */
        if (status == ER_EOF) {
            persistentRevision = 0;
            MarkGuidSet();  /* make thisGuid the keystore guid */
            status = ER_OK;
        }
    }
    if (loaded) {
        loaded->SetEvent();
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    return status;
}

void KeyStore::LoadFromEmptyFile(const qcc::String& password)
{
    QCC_DbgPrintf(("KeyStore::PullEmptyKeyStore"));

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));

    if (!keyStoreKey) {
        keyStoreKey = new KeyStoreEncryptionKey();
    }
    keyStoreKey->Build(password, thisGuid.ToString());

    persistentKeys->clear();
    persistentRevision = 0;
    MarkGuidSet();  /* make thisGuid the keystore guid */

    if (loaded) {
        loaded->SetEvent();
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
}

QStatus KeyStore::Clear()
{
    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    if (status != ER_OK) {
        return status;
    }

    status = AcquireExclusiveLock(MUTEX_CONTEXT);
    if (status != ER_OK) {
        return status;
    }

    keys->clear();
    storeState = MODIFIED;
    deletions.clear();

    ReleaseExclusiveLock(MUTEX_CONTEXT);
    return status;
}

static bool MatchesPrefix(const String& str, const String& prefixPattern)
{
    return !WildcardMatch(str, prefixPattern);
}

QStatus KeyStore::Clear(const qcc::String& tagPrefixPattern)
{
    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    if (status != ER_OK) {
        return status;
    }

    status = AcquireExclusiveLock(MUTEX_CONTEXT);
    if (status != ER_OK) {
        return status;
    }

    bool dirty = true;
    while (dirty) {
        dirty = false;
        for (KeyMap::iterator it = keys->begin(); it != keys->end(); it++) {
            if (it->second.keyBlob.GetTag().empty()) {
                continue;  /* skip the untag */
            }
            if (MatchesPrefix(it->second.keyBlob.GetTag(), tagPrefixPattern)) {
                DelKeyInternal(it->first);
                dirty = true;
                break;  /* redo the loop since the iterator is out-of-sync */
            }
        }
        if (ER_OK != status) {
            break;
        }
    }

    /* Release the lock, which also commits to the listener on success. */
    ReleaseExclusiveLock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::Reload()
{
    QCC_DbgHLPrintf(("KeyStore::Reload"));
    /*
     * KeyStore "isShared" feature is removed in 16.04.
     * All instances of KeyStore that point to the same file are now always shared.
     */
    return Load();
}

QStatus KeyStore::Push(Sink& sink)
{
    QCC_DbgHLPrintf(("KeyStore::Push (revision %u)", revision + 1));

    /* Refresh the keystore. */
    (void)Load();

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));

    /*
     * Pack the keys into an intermediate string sink.
     */
    size_t pushed;
    StringSink strSink;
    KeyMap::iterator it;
    for (it = keys->begin(); it != keys->end(); ++it) {
        strSink.PushBytes(&it->second.revision, sizeof(revision), pushed);
        Key::KeyType keyType = it->first.GetType();
        strSink.PushBytes(&keyType, sizeof(keyType), pushed);
        strSink.PushBytes(it->first.GetGUID().GetBytes(), qcc::GUID128::SIZE, pushed);
        it->second.keyBlob.Store(strSink);

        strSink.PushBytes(&it->second.accessRights, sizeof(it->second.accessRights), pushed);
        QCC_DbgPrintf(("KeyStore::Push rev:%u key %s", it->second.revision, it->first.ToString().c_str()));
    }
    size_t keysLen = strSink.GetString().size();
    /*
     * First two bytes are the version number.
     */
    QStatus status = sink.PushBytes(&KeyStoreVersion, sizeof(KeyStoreVersion), pushed);
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
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    return status;
}

QStatus KeyStore::GetKey(const Key& key, KeyBlob& keyBlob, uint8_t accessRights[4])
{
    QCC_DbgPrintf(("KeyStore::GetKey %s", key.ToString().c_str()));

    /* Refresh the keystore. */
    (void)Load();

    KeyRecord* keyRec = nullptr;
    KeyBlob* kb = nullptr;

    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
        goto Exit;
    }
    if (keys->find(key) == keys->end()) {
        status = ER_BUS_KEY_UNAVAILABLE;
        goto Exit;
    }

    keyRec = &(*keys)[key];
    kb = &keyRec->keyBlob;
    if (kb->HasExpired()) {
        status = ER_BUS_KEY_EXPIRED;
        goto Exit;
    }

    /* See if the key's association has expired. */
    if ((kb->GetAssociationMode() == KeyBlob::ASSOCIATE_MEMBER) ||
        (kb->GetAssociationMode() == KeyBlob::ASSOCIATE_BOTH)) {
        Key head(key.GetType(), kb->GetAssociation());
        if ((keys->find(head) != keys->end()) &&
            ((*keys)[head].keyBlob.HasExpired())) {
            /* The key's association (its head) has expired. */
            status = ER_BUS_KEY_EXPIRED;
            goto Exit;
        }
    }

    /* Copy the access rights. */
    memcpy(accessRights, &keyRec->accessRights, sizeof(keyRec->accessRights));
    QCC_DbgPrintf(("AccessRights %1x%1x%1x%1x", accessRights[0], accessRights[1], accessRights[2], accessRights[3]));
    keyBlob = *kb;

Exit:
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    return status;
}

bool KeyStore::HasKey(const Key& key)
{
    /* Refresh the keystore. */
    (void)Load();

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    bool hasKey = false;
    if (storeState != UNAVAILABLE) {
        hasKey = keys->count(key) != 0;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    return hasKey;
}

QStatus KeyStore::AddKey(const Key& key, const KeyBlob& keyBlob, const uint8_t accessRights[4])
{
    qcc::String guid = key.ToString();
    QCC_DbgPrintf(("KeyStore::AddKey %s", guid.c_str()));

    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    if (status != ER_OK) {
        return status;
    }

    status = AcquireExclusiveLock(MUTEX_CONTEXT);
    if (status != ER_OK) {
        return status;
    }

    /* Perform necessary work on the local copy. */
    KeyRecord& keyRec = (*keys)[key];
    keyRec.revision = revision + 1;
    keyRec.keyBlob = keyBlob;
    QCC_DbgPrintf(("AccessRights %1x%1x%1x%1x", accessRights[0], accessRights[1], accessRights[2], accessRights[3]));
    memcpy(&keyRec.accessRights, accessRights, sizeof(uint8_t) * 4);
    storeState = MODIFIED;
    deletions.erase(key);

    /* Release the lock, which also commits to the listener. */
    ReleaseExclusiveLock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::DelKey(const Key& key, bool includeAssociatedKeys, bool exclusiveLockAlreadyHeld)
{
    qcc::String guid = key.ToString();
    QCC_DbgPrintf(("KeyStore::DelKey %s", guid.c_str()));

    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    if (status != ER_OK) {
        return status;
    }

    if (!exclusiveLockAlreadyHeld) {
        status = AcquireExclusiveLock(MUTEX_CONTEXT);
        if (status != ER_OK) {
            return status;
        }
    }

    if (keys->find(key) == keys->end()) {
        /* This function must return success if the key doesn't exist (and not ER_BUS_KEY_UNAVAILABLE). */
        status = ER_OK;
        goto Exit;
    }

    if (includeAssociatedKeys) {
        /*
         * Delete the associated keys. Do not call DelKeyInternal directly since it
         * is neccessary to handle the associated deletes for each member. If a delete
         * fails, we save the status and continue trying to delete other associated keys.
         */
        QStatus savedStatus = ER_OK;
        KeyBlob& kb = (*keys)[key].keyBlob;
        if ((kb.GetAssociationMode() == KeyBlob::ASSOCIATE_HEAD) ||
            (kb.GetAssociationMode() == KeyBlob::ASSOCIATE_BOTH)) {
            KeyStore::Key* list = nullptr;
            size_t numItems = 0;
            if (SearchAssociatedKeys(key, &list, &numItems) == ER_OK) {
                for (size_t cnt = 0; cnt < numItems; cnt++) {
                    status = DelKey(list[cnt], true, true);
                    if (status != ER_OK) {
                        savedStatus = status;
                    }
                }
            }
            delete [] list;
        }
        status = savedStatus;
    }


    DelKeyInternal(key);

Exit:
    if (!exclusiveLockAlreadyHeld) {
        /* Release the lock, which also commits to the listener. */
        ReleaseExclusiveLock(MUTEX_CONTEXT);
    }
    return status;
}

/* ExclusiveLock must be held when calling this function. */
void KeyStore::DelKeyInternal(const Key& key)
{
    /* Perform necessary work on the local copy. */
    /* Use a local copy because erase might destroy the key */
    Key keyCopy(key);
    keys->erase(key);
    storeState = MODIFIED;
    deletions.insert(keyCopy);
}

QStatus KeyStore::SetKeyExpiration(const Key& key, const Timespec<qcc::EpochTime>& expiration)
{
    qcc::String guid = key.ToString();
    QCC_DbgPrintf(("KeyStore::SetExpiration %s", guid.c_str()));

    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    if (status != ER_OK) {
        return status;
    }

    status = AcquireExclusiveLock(MUTEX_CONTEXT);
    if (status != ER_OK) {
        return status;
    }

    /* Perform necessary work on the local copy. */
    if (keys->count(key) != 0) {
        (*keys)[key].keyBlob.SetExpiration(expiration);
        storeState = MODIFIED;
    } else {
        ReleaseExclusiveLock(MUTEX_CONTEXT);
        return ER_BUS_KEY_UNAVAILABLE;
    }

    ReleaseExclusiveLock(MUTEX_CONTEXT);
    return status;
}

QStatus KeyStore::GetKeyExpiration(const Key& key, Timespec<EpochTime>& expiration)
{
    qcc::String guid = key.ToString();
    QCC_DbgPrintf(("KeyStore::GetExpiration %s", guid.c_str()));

    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (storeState == UNAVAILABLE) {
        status = ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    if (status != ER_OK) {
        return status;
    }

    /* Refresh the keystore. */
    (void)Load();

    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    if (keys->count(key) != 0) {
        (*keys)[key].keyBlob.GetExpiration(expiration);
    } else {
        status = ER_BUS_KEY_UNAVAILABLE;
    }
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));

    return status;
}

QStatus KeyStore::SearchAssociatedKeys(const Key& key, Key** list, size_t* numItems)
{
    /* Refresh the keystore. */
    (void)Load();

    size_t count = 0;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
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
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
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
                QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
                return ER_FAIL;
            }
            keyList[idx++] = it->first;
        }
    }
    *numItems = count;
    *list = keyList;
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
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

QStatus KeyStore::AcquireExclusiveLock(const char* file, uint32_t line)
{
    /* First acquire the process lock. */
    if (file == nullptr) {
        /* Release mode */
        QCC_VERIFY(ER_OK == s_exclusiveLock->Lock());
    } else {
        /* Debug mode */
        QCC_VERIFY(ER_OK == s_exclusiveLock->Lock(file, line));
    }

    /* Acquire the file lock. */
    QStatus status = listener->AcquireExclusiveLock(file, line);
    if (status != ER_OK) {
        /* Using MUTEX_CONTEXT here since this unlock isn't called directly by the caller. */
        QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(MUTEX_CONTEXT));
        return status;
    }

    QCC_VERIFY(ER_OK == lock.Lock(file, line));
    /* This assert fires if there is a recursive call to acquire the lock. */
    QCC_ASSERT(exclusiveLockRefreshState == ExclusiveLockNotHeld);
    /* Mark the refresh state to 'dirty' so Load() would refresh the keys. */
    exclusiveLockRefreshState = ExclusiveLockHeld_Dirty;
    QCC_VERIFY(ER_OK == lock.Unlock(file, line));

    /* Refresh the keystore. */
    (void)Load();
    return ER_OK;
}

void KeyStore::ReleaseExclusiveLock(const char* file, uint32_t line)
{
    /* This method assumes exclusive lock is already acquired by the caller. */
    s_exclusiveLock->AssertOwnedByCurrentThread();

    std::vector<Key> expiredKeys;
    QStatus status = StoreInternal(expiredKeys);
    QCC_VERIFY(ER_OK == lock.Lock(file, line));
    if (status == ER_OK) {
        /* This assert makes sure dirty keys have been committed. */
        QCC_ASSERT(exclusiveLockRefreshState == ExclusiveLockHeld_Clean);
    } else {
        QCC_LogError(status, ("KeyStore::StoreInternal returned status=(%#x)", status));
        QCC_ASSERT(exclusiveLockRefreshState != ExclusiveLockNotHeld);
    }
    /* We're about to leave the lock, set the state accordingly. */
    exclusiveLockRefreshState = ExclusiveLockNotHeld;
    QCC_VERIFY(ER_OK == lock.Unlock(file, line));

    /* Release the file lock. */
    listener->ReleaseExclusiveLock(file, line);

    /* Release the process lock. */
    if (file == nullptr) {
        /* Release mode */
        QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock());
    } else {
        /* Debug mode */
        QCC_VERIFY(ER_OK == s_exclusiveLock->Unlock(file, line));
    }

    if (status == ER_OK) {
        /* Notify deletion of expired keys. */
        for (auto& key : expiredKeys) {
            if (keyEventListener) {
                keyEventListener->NotifyAutoDelete(this, key);
            }
        }
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
