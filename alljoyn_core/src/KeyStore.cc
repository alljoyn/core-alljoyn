/**
 * @file
 * The KeyStore class manages the storing and loading of key blobs from
 * external storage. The default implementation stores key blobs in a file.
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
 * Current key store version we will write
 */
static const uint16_t KeyStoreVersion = 0x0103;


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

class DefaultKeyStoreListener : public KeyStoreListener {

  public:

    DefaultKeyStoreListener(const qcc::String& application, const char* fname) {
        if (fname) {
            fileName = GetHomeDir() + "/" + fname;
        } else {
            fileName = GetHomeDir() + "/.alljoyn_keystore/" + application;
        }
    }

    QStatus LoadRequest(KeyStore& keyStore) {
        QStatus status;
        /* Try to load the keystore */
        {
            FileSource source(fileName);
            if (source.IsValid()) {
                source.Lock(true);
                status = keyStore.Pull(source, fileName);
                if (status == ER_OK) {
                    QCC_DbgHLPrintf(("Read key store from %s", fileName.c_str()));
                }
                source.Unlock();
                return status;
            }
        }
        /* Create an empty keystore */
        {
            FileSink sink(fileName, FileSink::PRIVATE);
            if (!sink.IsValid()) {
                status = ER_BUS_WRITE_ERROR;
                QCC_LogError(status, ("Cannot initialize key store %s", fileName.c_str()));
                return status;
            }
        }
        /* Load the empty keystore */
        {
            FileSource source(fileName);
            if (source.IsValid()) {
                source.Lock(true);
                status = keyStore.Pull(source, fileName);
                if (status == ER_OK) {
                    QCC_DbgHLPrintf(("Initialized key store %s", fileName.c_str()));
                } else {
                    QCC_LogError(status, ("Failed to initialize key store %s", fileName.c_str()));
                }
                source.Unlock();
            } else {
                status = ER_BUS_READ_ERROR;
            }
            return status;
        }
    }

    QStatus StoreRequest(KeyStore& keyStore) {
        QStatus status;
        FileSink sink(fileName, FileSink::PRIVATE);
        if (sink.IsValid()) {
            sink.Lock(true);
            status = keyStore.Push(sink);
            if (status == ER_OK) {
                QCC_DbgHLPrintf(("Wrote key store to %s", fileName.c_str()));
            }
            sink.Unlock();
        } else {
            status = ER_BUS_WRITE_ERROR;
            QCC_LogError(status, ("Cannot write key store to %s", fileName.c_str()));
        }
        return status;
    }

  private:

    qcc::String fileName;

};

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
    loaded(NULL),
    keyEventListener(NULL)
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
    delete defaultListener;
    delete listener;
    delete keyStoreKey;
    delete keys;
}

QStatus KeyStore::SetListener(KeyStoreListener& listener)
{
    if (this->listener != NULL) {
        return ER_BUS_LISTENER_ALREADY_SET;
    } else {
        this->listener = new ProtectedKeyStoreListener(&listener);
        return ER_OK;
    }
}

/**
 * Setup the key event listener.
 *
 * @param listener  The listener that will listen to key event.
 */
QStatus KeyStore::SetKeyEventListener(KeyStoreKeyEventListener* listener) {
    keyEventListener = listener;
    return ER_OK;
}

QStatus KeyStore::SetDefaultListener()
{
    this->listener = new ProtectedKeyStoreListener(defaultListener);
    return ER_OK;
}

QStatus KeyStore::Reset()
{
    if (storeState != UNAVAILABLE) {
        QStatus status = Clear();
        storeState = UNAVAILABLE;
        delete listener;
        listener = NULL;
        delete defaultListener;
        defaultListener = NULL;
        shared = false;
        return status;
    } else {
        return ER_FAIL;
    }
}

QStatus KeyStore::Init(const char* fileName, bool isShared)
{
    if (storeState == UNAVAILABLE) {
        if (listener == NULL) {
            defaultListener = new DefaultKeyStoreListener(application, fileName);
            listener = new ProtectedKeyStoreListener(defaultListener);
        }
        shared = isShared;
        return Load();
    } else {
        return ER_FAIL;
    }
}

QStatus KeyStore::Store()
{
    QStatus status = ER_OK;

    /* Cannot store if never loaded */
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    /* Don't store if not modified */
    if (storeState == MODIFIED) {

        lock.Lock(MUTEX_CONTEXT);
        EraseExpiredKeys();

        /* Reload to merge keystore changes before storing */
        if (revision > 0) {
            lock.Unlock(MUTEX_CONTEXT);
            status = Reload();
            lock.Lock(MUTEX_CONTEXT);
        }
        if (status == ER_OK) {
            stored = new Event();
            lock.Unlock(MUTEX_CONTEXT);
            status = listener->StoreRequest(*this);
            if (status == ER_OK) {
                status = Event::Wait(*stored);
            }
            lock.Lock(MUTEX_CONTEXT);
            delete stored;
            stored = NULL;
            /* Done tracking deletions */
            deletions.clear();
        }
        lock.Unlock(MUTEX_CONTEXT);
    }
    return status;
}

QStatus KeyStore::Load()
{
    QStatus status;
    lock.Lock(MUTEX_CONTEXT);
    keys->clear();
    storeState = UNAVAILABLE;
    loaded = new Event();
    lock.Unlock(MUTEX_CONTEXT);
    status = listener->LoadRequest(*this);
    if (status == ER_OK) {
        status = Event::Wait(*loaded);
    }
    lock.Lock(MUTEX_CONTEXT);
    delete loaded;
    loaded = NULL;
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

size_t KeyStore::EraseExpiredKeys()
{
    size_t count = 0;
    bool dirty = true;
    while (dirty) {
        dirty = false;
        KeyMap::iterator it = keys->begin();
        while (it != keys->end()) {
            KeyMap::iterator current = it++;
            if (current->second.key.HasExpired()) {
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
    if (storeState != UNAVAILABLE) {
        return ER_OK;
    }

    lock.Lock(MUTEX_CONTEXT);

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
    }

    /* This is the only chance to generate the key store key */
    if (!keyStoreKey) {
        keyStoreKey = new KeyBlob;
    }
    keyStoreKey->Derive(password + GetGuid(), Crypto_AES::AES128_SIZE, KeyBlob::AES);

    /* Allow for an uninitialized (empty) key store */
    if (status == ER_EOF) {
        keys->clear();
        storeState = MODIFIED;
        revision = 0;
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
            KeyBlob nonce((uint8_t*)&revision, sizeof(revision), KeyBlob::GENERIC);
            Crypto_AES aes(*keyStoreKey, Crypto_AES::CCM);
            status = aes.Decrypt_CCM(data, data, len, nonce, NULL, 0, 16);
            /*
             * Unpack the guid/key pairs from an intermediate string source.
             */
            StringSource strSource(data, len);
            while (status == ER_OK) {
                uint32_t rev;
                status = strSource.PullBytes(&rev, sizeof(rev), pulled);
                if (status == ER_OK) {
                    status = strSource.PullBytes(guidBuf, qcc::GUID128::SIZE, pulled);
                }
                if (status == ER_OK) {
                    qcc::GUID128 guid;
                    guid.SetBytes(guidBuf);
                    KeyRecord& keyRec = (*keys)[guid];
                    keyRec.revision = rev;
                    status = keyRec.key.Load(strSource);
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
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    lock.Lock(MUTEX_CONTEXT);
    keys->clear();
    storeState = MODIFIED;
    revision = 0;
    deletions.clear();
    lock.Unlock(MUTEX_CONTEXT);
    listener->StoreRequest(*this);
    return ER_OK;
}

QStatus KeyStore::Reload()
{
    QCC_DbgHLPrintf(("KeyStore::Reload"));

    /*
     * Cannot reload if the key store has never been loaded
     */
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    /*
     * Reload is defined to be a no-op for non-shared key stores
     */
    if (!shared) {
        return ER_OK;
    }

    lock.Lock(MUTEX_CONTEXT);
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
        std::set<qcc::GUID128>::iterator itDel;
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
        strSink.PushBytes(it->first.GetBytes(), qcc::GUID128::SIZE, pushed);
        it->second.key.Store(strSink);
        strSink.PushBytes(&it->second.accessRights, sizeof(it->second.accessRights), pushed);
        QCC_DbgPrintf(("KeyStore::Push rev:%d GUID %s", it->second.revision, it->first.ToString().c_str()));
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

QStatus KeyStore::GetKey(const qcc::GUID128& guid, KeyBlob& key, uint8_t accessRights[4])
{
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QStatus status;
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("KeyStore::GetKey %s", guid.ToString().c_str()));
    if (keys->find(guid) != keys->end()) {
        KeyRecord& keyRec = (*keys)[guid];
        key = keyRec.key;
        memcpy(accessRights, &keyRec.accessRights, sizeof(uint8_t) * 4);
        QCC_DbgPrintf(("AccessRights %1x%1x%1x%1x", accessRights[0], accessRights[1], accessRights[2], accessRights[3]));
        status = ER_OK;
    } else {
        status = ER_BUS_KEY_UNAVAILABLE;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

bool KeyStore::HasKey(const qcc::GUID128& guid)
{
    if (storeState == UNAVAILABLE) {
        return false;
    }
    bool hasKey;
    lock.Lock(MUTEX_CONTEXT);
    hasKey = keys->count(guid) != 0;
    lock.Unlock(MUTEX_CONTEXT);
    return hasKey;
}

QStatus KeyStore::AddKey(const qcc::GUID128& guid, const KeyBlob& key, const uint8_t accessRights[4])
{
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("KeyStore::AddKey %s", guid.ToString().c_str()));
    KeyRecord& keyRec = (*keys)[guid];
    keyRec.revision = revision + 1;
    keyRec.key = key;
    QCC_DbgPrintf(("AccessRights %1x%1x%1x%1x", accessRights[0], accessRights[1], accessRights[2], accessRights[3]));
    memcpy(&keyRec.accessRights, accessRights, sizeof(uint8_t) * 4);
    storeState = MODIFIED;
    deletions.erase(guid);
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

QStatus KeyStore::DelKey(const qcc::GUID128& guid)
{
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("KeyStore::DelKey %s", guid.ToString().c_str()));
    keys->erase(guid);
    storeState = MODIFIED;
    deletions.insert(guid);
    lock.Unlock(MUTEX_CONTEXT);
    listener->StoreRequest(*this);
    return ER_OK;
}

QStatus KeyStore::SetKeyExpiration(const qcc::GUID128& guid, const Timespec& expiration)
{
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    QStatus status = ER_OK;
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("KeyStore::SetExpiration %s", guid.ToString().c_str()));
    if (keys->count(guid) != 0) {
        (*keys)[guid].key.SetExpiration(expiration);
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

QStatus KeyStore::GetKeyExpiration(const qcc::GUID128& guid, Timespec& expiration)
{
    if (storeState == UNAVAILABLE) {
        return ER_BUS_KEYSTORE_NOT_LOADED;
    }
    /*
     * For shared key stores we may need to do a reload before checking for key expiration.
     */
    QStatus status = Reload();
    if (status == ER_OK) {
        lock.Lock(MUTEX_CONTEXT);
        QCC_DbgPrintf(("KeyStore::GetExpiration %s", guid.ToString().c_str()));
        if (keys->count(guid) != 0) {
            (*keys)[guid].key.GetExpiration(expiration);
        } else {
            status = ER_BUS_KEY_UNAVAILABLE;
        }
        lock.Unlock(MUTEX_CONTEXT);
    }
    return status;
}

QStatus KeyStore::SearchAssociatedKeys(const qcc::GUID128& guid, qcc::GUID128** list, size_t* numItems) {
    size_t count = 0;
    lock.Lock(MUTEX_CONTEXT);
    for (KeyMap::iterator it = keys->begin(); it != keys->end(); ++it) {
        if ((it->second.key.GetAssociationMode() != KeyBlob::ASSOCIATE_MEMBER)
            && (it->second.key.GetAssociationMode() != KeyBlob::ASSOCIATE_BOTH)) {
            continue;
        }
        if (it->second.key.GetAssociation() == guid) {
            count++;
        }
    }
    if (count == 0) {
        *numItems = count;
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }

    qcc::GUID128*guids = new qcc::GUID128[count];
    size_t idx = 0;
    for (KeyMap::iterator it = keys->begin(); it != keys->end(); ++it) {
        if ((it->second.key.GetAssociationMode() != KeyBlob::ASSOCIATE_MEMBER)
            && (it->second.key.GetAssociationMode() != KeyBlob::ASSOCIATE_BOTH)) {
            continue;
        }
        if (it->second.key.GetAssociation() == guid) {
            if (idx >= count) { /* bound check */
                delete [] guids;
                lock.Unlock(MUTEX_CONTEXT);
                return ER_FAIL;
            }
            guids[idx++] = it->first;
        }
    }
    *numItems = count;
    *list = guids;
    lock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}
}
