/**
 * @file IceManager.cc
 *
 * IceManager is responsible for executing and coordinating ICE related network operations.
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#include <list>
#include <qcc/Mutex.h>
#include <qcc/Debug.h>
#include <ICESession.h>
#include <ICESessionListener.h>
#include <ICEManager.h>
#include "RendezvousServerInterface.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICEMANAGER"

namespace ajn {

ICEManager::ICEManager()
{
}

ICEManager::~ICEManager(void)
{
    // Reclaim memory consumed by the sessions.
    lock.Lock();
    while (!sessions.empty()) {
        ICESession* session = sessions.front();
        delete session;
        sessions.pop_front();
    }
    lock.Unlock();
}



QStatus ICEManager::AllocateSession(bool addHostCandidates,
                                    bool addRelayedCandidates,
                                    bool enableIpv6,
                                    ICESessionListener* listener,
                                    ICESession*& session,
                                    STUNServerInfo stunInfo,
                                    IPAddress onDemandAddress,
                                    IPAddress persistentAddress)
{
    QStatus status = ER_OK;

    session = new ICESession(addHostCandidates, addRelayedCandidates, listener,
                             stunInfo, onDemandAddress, persistentAddress, enableIpv6);

    status = session->Init();

    if (ER_OK == status) {
        lock.Lock();                // Synch with another thread potentially calling destructor.
                                    // Not likely because this is a singleton, but...
        sessions.push_back(session);
        lock.Unlock();
    } else {
        QCC_LogError(status, ("session->Init"));
        delete session;
        session = NULL;
    }

    return status;
}



QStatus ICEManager::DeallocateSession(ICESession*& session)
{
    QStatus status = ER_OK;

    assert(session != NULL);

    if (session != NULL) {
        // remove from list
        lock.Lock();                // Synch with another thread potentially calling destructor.
                                    // Not likely because this is a singleton, but...
        sessions.remove(session);
        lock.Unlock();

        delete session;
    }

    return status;
}

} //namespace ajn
