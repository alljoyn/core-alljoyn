/**
 * @file
 * This class maintains information about peers connected to the bus.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#include <algorithm>
#include <limits>

#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/time.h>

#include "PeerState.h"
#include "AllJoynCrypto.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {


uint32_t _PeerState::EstimateTimestamp(uint32_t remote)
{
    uint32_t local = qcc::GetTimestamp();
    int32_t delta = static_cast<int32_t>(local - remote);
    int32_t oldOffset = clockOffset;

    /*
     * Clock drift adjustment. Make remote re-confirm minimum occasionally.
     * This will adjust for clock drift that is less than 100 ppm.
     */
    if ((local - lastDriftAdjustTime) > 10000) {
        lastDriftAdjustTime = local;
        ++clockOffset;
    }

    if (((oldOffset - delta) > 0) || firstClockAdjust) {
        QCC_DbgHLPrintf(("Updated clock offset old %d, new %d", clockOffset, delta));
        clockOffset = delta;
        firstClockAdjust = false;
    }

    return remote + static_cast<uint32_t>(clockOffset);
}

#define IN_RANGE(val, start, sz) ((((start) <= ((start) + (sz))) && ((val) >= (start)) && ((val) < ((start) + (sz)))) || \
                                  (((start) > ((start) + (sz))) && !(((val) >= ((start) + (sz))) && ((val) < (start)))))

bool _PeerState::IsValidSerial(uint32_t serial, bool secure, bool unreliable)
{
    bool ret = false;
    /*
     * Serial 0 is always invalid.
     */
    if (serial != 0) {
        const size_t winSize = sizeof(window) / sizeof(window[0]);
        uint32_t* entry = window + (serial % winSize);
        if ((*entry != serial) && IN_RANGE(serial, *entry, numeric_limits<uint32_t>::max() / 2)) {
            *entry = serial;
            ret = true;
        }
    }
    return ret;

}

PeerStateTable::PeerStateTable()
{
    Clear();
}

PeerState PeerStateTable::GetPeerState(const qcc::String& busName)
{
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgHLPrintf(("PeerStateTable::GetPeerState() %s state for %s", peerMap.count(busName) ? "got" : "no", busName.c_str()));
    PeerState result = peerMap[busName];
    lock.Unlock(MUTEX_CONTEXT);

    return result;
}

PeerState PeerStateTable::GetPeerState(const qcc::String& uniqueName, const qcc::String& aliasName)
{
    assert(uniqueName[0] == ':');
    PeerState result;
    lock.Lock(MUTEX_CONTEXT);
    std::map<const qcc::String, PeerState>::iterator iter = peerMap.find(uniqueName);
    if (iter == peerMap.end()) {
        QCC_DbgHLPrintf(("PeerStateTable::GetPeerState() no state stored for %s aka %s", uniqueName.c_str(), aliasName.c_str()));
        result = peerMap[aliasName];
        peerMap[uniqueName] = result;
    } else {
        QCC_DbgHLPrintf(("PeerStateTable::GetPeerState() got state for %s aka %s", uniqueName.c_str(), aliasName.c_str()));
        result = iter->second;
        peerMap[aliasName] = result;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return result;
}

void PeerStateTable::DelPeerState(const qcc::String& busName)
{
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgHLPrintf(("PeerStateTable::DelPeerState() %s for %s", peerMap.count(busName) ? "remove state" : "no state to remove", busName.c_str()));
    peerMap.erase(busName);
    lock.Unlock(MUTEX_CONTEXT);
}

void PeerStateTable::GetGroupKey(qcc::KeyBlob& key)
{
    PeerState groupPeer = GetPeerState("");
    /*
     * The group key is carried by the null-name peer
     */
    groupPeer->GetKey(key, PEER_SESSION_KEY);
    /*
     * Access rights on the group peer always allows signals to be encrypted.
     */
    groupPeer->SetAuthorization(MESSAGE_SIGNAL, _PeerState::ALLOW_SECURE_TX);
}

void PeerStateTable::Clear()
{
    qcc::KeyBlob key;
    lock.Lock(MUTEX_CONTEXT);
    peerMap.clear();
    PeerState nullPeer;
    QCC_DbgHLPrintf(("Allocating group key"));
    key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
    key.SetTag("GroupKey", KeyBlob::NO_ROLE);
    nullPeer->SetKey(key, PEER_SESSION_KEY);
    peerMap[""] = nullPeer;
    lock.Unlock(MUTEX_CONTEXT);
}

PeerStateTable::~PeerStateTable()
{
    lock.Lock(MUTEX_CONTEXT);
    peerMap.clear();
    lock.Unlock(MUTEX_CONTEXT);
}

}
