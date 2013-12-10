/**
 * @file
 * Implements a pool for Packet objects
 */

/******************************************************************************
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/Mutex.h>

#include "PacketPool.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "PACKET"

namespace ajn {

PacketPool::PacketPool() : mtu(0), usedCount(0)
{
}

QStatus PacketPool::Start(size_t mtu)
{
    this->mtu = mtu;
    return ER_OK;
}

QStatus PacketPool::Stop()
{
    return ER_OK;
}

PacketPool::~PacketPool()
{
    lock.Lock();
    std::vector<Packet*>::iterator it = freeList.begin();
    for (; it != freeList.end(); ++it) {
        Packet* p = *it;
        delete p;
    }
    freeList.clear();
    lock.Unlock();
}

Packet* PacketPool::GetPacket() {
    Packet* p = NULL;
#ifdef PACKET_LEAK_DEBUG
    p = new Packet(mtu);
#else
    lock.Lock();
    usedCount++;
    if (freeList.size() > 0) {
        p = freeList.back();
        freeList.pop_back();
        lock.Unlock();
    } else {
        lock.Unlock();
        p = new Packet(mtu);
    }
#endif
    return p;
}

void PacketPool::ReturnPacket(Packet* p) {
#ifdef PACKET_LEAK_DEBUG
    delete p;
#else
    lock.Lock();
    --usedCount;
    if ((freeList.size() * 2) > usedCount) {
        lock.Unlock();
        delete p;
    } else {
        p->Clean();
        freeList.push_back(p);
        lock.Unlock();
    }
#endif
}

}
