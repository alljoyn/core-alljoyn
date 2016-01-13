/**
 * @file
 *
 * Lock level definitions used to detect out-of-order Mutex object acquires.
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
#ifndef _QCC_LOCKLEVEL_H
#define _QCC_LOCKLEVEL_H

namespace qcc {

/*
 * Most locks should be acquired in a well-defined order, to avoid potential deadlocks.
 * Assign the level values below to each lock, and LockChecker will verify that locks
 * with lower level don't get acquired while current thread owns a lock having a higher
 * level.
 */
typedef enum {
    /*
     * LOCK_LEVEL_CHECKING_DISABLED locks are completely ignored by the LockChecker.
     *
     * Typical LOCK_LEVEL_CHECKING_DISABLED locks are:
     *
     * - Those locks that are used by the LockChecker internally, and therefore cannot
     *   be reliably verified.
     *
     * - Locks that are held while calling back from the SCL to the app code, because
     *   specifying a lock level from an apps is currently unsupported.
     *
     * - Locks that are involved in a known potential deadlock, as a workaround until
     *   that deadlock gets fixed.
     *
     * - Locks that result in a false positive assertion failed from the LockChecker.
     *   No such false positives are currently known in AllJoyn.
     *
     * Example of a pattern that would result in false positives, in the current
     * LockChecker implementation:
     *
     * - Path1: Lock A, lock B, then lock C
     * - Path2: Lock A, lock C, then lock B
     *
     * If that pattern will ever be used in AllJoyn, we'll have to either fix LockChecker
     * for it, or mark lock B or C as LOCK_LEVEL_CHECKING_DISABLED.
     */
    LOCK_LEVEL_CHECKING_DISABLED = -1,

    /*
     * LOCK_LEVEL_NOT_SPECIFIED are those locks that have not been opted-in yet for
     * LockChecker verification. Those locks are being ignored by the LockChecker unless
     * they are being acquired while owning one of the verified locks. Ignoring the
     * unspecified lock lock level could result in missing bugs that the owner of the
     * verified locks wanted to expose, and therefore LockChecker complains about that
     * situation. To eliminate that complaint, a valid level should be assigned to each
     * of these locks.
     */
    LOCK_LEVEL_NOT_SPECIFIED = 0,

    /* Bus.cc */
    LOCK_LEVEL_BUS_LISTENERSLOCK = 1000,

    /* ObserverManager.cc */
    LOCK_LEVEL_OBSERVERMANAGER_WQLOCK = 3000,

    /* AutoPingerInternal.cc */
    LOCK_LEVEL_AUTOPINGERINTERNAL_GLOBALPINGERLOCK = 4000,

    /* PeerState.cc */
    LOCK_LEVEL_PEERSTATE_INITIATORHASHLOCK = 5000,

    /* Observer.cc */
    LOCK_LEVEL_OBSERVER_LISTENERSLOCK = 6000,
    LOCK_LEVEL_OBSERVER_PROXIESLOCK = 6100,

    /* ProxyBusObject.cc */
    LOCK_LEVEL_PROXYBUSOBJECT_INTERNAL_LOCK = 8000,
    LOCK_LEVEL_PROXYBUSOBJECT_CACHEDPROPS_LOCK = 8100,

    /* NameTable.cc */
    LOCK_LEVEL_NAMETABLE_LOCK = 10000,

    /* SessionlessObj.cc */
    LOCK_LEVEL_SESSIONLESSOBJ_LOCK = 11000,

    /* VirtualEndpoint.cc */
    LOCK_LEVEL_VIRTUALENDPOINT_MB2BENDPOINTSLOCK = 12000,

    /* UDPTransport.cc */
    LOCK_LEVEL_UDPTRANSPORT_MESSAGEPUMP_LOCK = 13000,
    LOCK_LEVEL_UDPTRANSPORT_EXITWORKERCOMMANDQUEUELOCK = 13020,
    LOCK_LEVEL_UDPTRANSPORT_CBLOCK = 13030,
    LOCK_LEVEL_UDPTRANSPORT_UDPENDPOINT_STATELOCK = 13040,
    LOCK_LEVEL_UDPTRANSPORT_ARDPSTREAM_LOCK = 13050,
    LOCK_LEVEL_UDPTRANSPORT_ARDPLOCK = 13060,
    LOCK_LEVEL_UDPTRANSPORT_CONNLOCK = 13070,
    LOCK_LEVEL_UDPTRANSPORT_PRELISTLOCK = 13075,
    LOCK_LEVEL_UDPTRANSPORT_WORKERCOMMANDQUEUELOCK = 13090,
    LOCK_LEVEL_UDPTRANSPORT_MLISTENREQUESTSLOCK = 13100,
    LOCK_LEVEL_UDPTRANSPORT_LISTENFDSLOCK = 13110,

    /* RemoteEndpoint.cc */
    LOCK_LEVEL_REMOTEENDPOINT_INTERNAL_LOCK = 14000,

    /* PeerState.cc */
    LOCK_LEVEL_PEERSTATE_LOCK = 14500,

    /* IODispatch.cc */
    LOCK_LEVEL_IODISPATCH_LOCK = 15000,

    /* Thread.cc */
    LOCK_LEVEL_THREAD_AUXLISTENERSLOCK = 16000,

    /* LocalTransport.cc */
    LOCK_LEVEL_LOCALTRANSPORT_LOCALENDPOINT_DISPATCHER_WORKLOCK = 17000,
    LOCK_LEVEL_LOCALTRANSPORT_LOCALENDPOINT_OBJECTSLOCK = 17100,
    LOCK_LEVEL_LOCALTRANSPORT_LOCALENDPOINT_HANDLERTHREADSLOCK = 17200,
    LOCK_LEVEL_LOCALTRANSPORT_LOCALENDPOINT_REPLYMAPLOCK = 17300,

    /* SignalTable.cc */
    LOCK_LEVEL_SIGNALTABLE_LOCK = 18000,

    /* RuleTable.cc */
    LOCK_LEVEL_RULETABLE_LOCK = 19000,

    /* TCPTransport.cc */
    LOCK_LEVEL_TCPTRANSPORT_MLISTENREQUESTSLOCK = 20000,
    LOCK_LEVEL_TCPTRANSPORT_MLISTENFDSLOCK = 20100,

    /* KeyStore.cc */
    LOCK_LEVEL_KEYSTORE_GUIDSETEVENTLOCK = 21200,

    /* ProtectedKeyStoreListener.cc */
    LOCK_LEVEL_PROTECTEDKEYSTORELISTENER_LOCK = 22000,

    /* BusAttachment.cc */
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_LISTENERSLOCK = 25000,
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_SESSIONPORTLISTENERSLOCK = 25100,
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_SESSIONSLOCK = 25200,
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_JOINLOCK = 25300,
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_ABOUTLISTENERSLOCK = 25400,
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_PERMISSIONCONFIGURATIONLISTENERLOCK = 25500,
    LOCK_LEVEL_BUSATTACHMENT_INTERNAL_APPLICATIONSTATELISTENERSLOCK = 25600,

    /* DaemonRouter.cc */
    LOCK_LEVEL_DAEMONROUTER_MLOCK = 27000,

    /* MethodTable.cc */
    LOCK_LEVEL_METHODTABLE_LOCK = 28000,

    /* AllJoynObj.cc */
    LOCK_LEVEL_ALLJOYNOBJ_JOINSESSIONTHREADSLOCK = 29000,

    /* AboutObjectDescription.cc */
    LOCK_LEVEL_ABOUTOBJECTDESCRIPTION_INTERNAL_ANNOUNCEOBJECTSMAPLOCK = 30000,

    /* PermissionMgmtObj.cc */
    LOCK_LEVEL_PERMISSIONMGMTOBJ_LOCK = 31000,

    /* BusObject.cc */
    LOCK_LEVEL_BUSOBJECT_COMPONENTS_COUNTERLOCK = 32000,

    /* ProtectedAuthListener.h */
    LOCK_LEVEL_PROTECTEDAUTHLISTENER_LOCK = 33000,

    /* IpNameServiceImpl.cc */
    LOCK_LEVEL_IPNAMESERVICEIMPL_MUTEX = 34000,

    /* Event.cc */
    LOCK_LEVEL_EVENT_IOEVENTMONITOR_LOCK = 35000,

    /* Timer.cc */
    LOCK_LEVEL_TIMERIMPL_LOCK = 36000,

    /* OpenSsl.cc */
    LOCK_LEVEL_OPENSSL_LOCK = 37000,

    /* Thread.cc */
    LOCK_LEVEL_THREAD_WAITLOCK = 38000,
    LOCK_LEVEL_THREAD_HBJMUTEX = 38100,

} LockLevel;

} /* namespace */

#endif  /* _QCC_LOCKLEVEL_H */
