#ifndef _ICEMANAGER_H
#define _ICEMANAGER_H

/**
 * @file ICEManager.h
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
#include "ICESession.h"
#include "ICESessionListener.h"
#include <alljoyn/Status.h>
#include "RendezvousServerInterface.h"

using namespace qcc;

namespace ajn {

// Forward Declaration
class ICESessionListener;

/**
 * ICEManager is an active singleton class that provides the external interface to ICE.
 * It is responsible for executing and coordinating ICE related network operations.
 */
class ICEManager {
  public:

    /** constructor */
    ICEManager();

    /**
     * Destructor.  This will deallocate sessions and terminate
     * the keep-alive thread.
     *
     * @sa ICEManager:DeallocateSession
     */
    ~ICEManager(void);

    /**
     * Allocate a Session.
     *
     * Perform the following sequence:
     *  1) If addHostCandidates is true, add host candidates for ALL known local network
     *     interfaces.
     *  2) Allocate local network resources.
     *  3) If addRelayedCandidates is true, reserve TURN resource(s) from TURN server
     *  4) Determine server reflexive ICE candidates via STUN.
     *
     * Local network resources and TURN resource reservation(s) will remain in effect until
     * the session is deallocate.
     *
     * @param addHostCandidates     If true, all known local network adapters will be added for
     *                              allocation.
     *
     * @param addRelayedCandidates  If true, TURN server resources will be allocated .
     *
     * @param enableIpv6            Use IPv6 interfaces also as candidates.
     *
     * @param session               Handle for session.
     *
     * @param stunInfo              STUN server information
     *
     * @param onDemandAddress       IP address of the interface over which the on demand connection
     *                                                          has been set up with the Rendezvous Server.
     *
     * @param persistentAddress     IP address of the interface over which the persistent connection
     *                                                          has been set up with the Rendezvous Server.
     */
    QStatus AllocateSession(bool addHostCandidates,
                            bool addRelayedCandidates,
                            bool enableIpv6,
                            ICESessionListener* listener,
                            ICESession*& session,
                            STUNServerInfo stunInfo,
                            IPAddress onDemandAddress,
                            IPAddress persistentAddress);


    /**
     * Deallocate an ICESession.
     * Deallocate all local network resources and TURN reservations associated with a given ICESession.
     *
     * @param session   ICESession to deallocate.
     */
    QStatus DeallocateSession(ICESession*& session);

  private:

    list<ICESession*> sessions;     ///< List of allocated ICESessions.

    Mutex lock;                    ///< Synchronizes multiple threads

    /** Private copy constructor */
    ICEManager(const ICEManager&);

    /** Private assignment operator */
    ICEManager& operator=(const ICEManager&);

};

} //namespace ajn

#endif
