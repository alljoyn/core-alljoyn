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
#ifndef _QCC_LOCKCHECKERLEVEL_H
#define _QCC_LOCKCHECKERLEVEL_H

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

    /* UDPTransport.cc */
    LOCK_LEVEL_UDPTRANSPORT_MESSAGEPUMP_LOCK = 1000,
    LOCK_LEVEL_UDPTRANSPORT_ENDPOINTLISTLOCK = 1010,
    LOCK_LEVEL_UDPTRANSPORT_EXITWORKERCOMMANDQUEUELOCK = 1020,
    LOCK_LEVEL_UDPTRANSPORT_CBLOCK = 1030,
    LOCK_LEVEL_UDPTRANSPORT_UDPENDPOINT_STATELOCK = 1040,
    LOCK_LEVEL_UDPTRANSPORT_ARDPSTREAM_LOCK = 1050,
    LOCK_LEVEL_UDPTRANSPORT_ARDPLOCK = 1060,
    LOCK_LEVEL_UDPTRANSPORT_CONNLOCK = 1070,
    LOCK_LEVEL_UDPTRANSPORT_PRELISTLOCK = 1080,
    LOCK_LEVEL_UDPTRANSPORT_WORKERCOMMANDQUEUELOCK = 1090,
    LOCK_LEVEL_UDPTRANSPORT_LISTENFDSLOCK = 1100,

    /* IpNameServiceImpl.cc */
    LOCK_LEVEL_IPNAMESERVICEIMPL_MUTEX = 9000,

    /* PeerState.cc */
    LOCK_LEVEL_PEERSTATE_LOCK = 9100,

    /* Event.cc locks */
    LOCK_LEVEL_EVENT_IOEVENTMONITOR_LOCK = 10000,

    /* OpenSsl.cc locks */
    LOCK_LEVEL_OPENSSL_LOCK = 20000,

} LockCheckerLevel;


} /* namespace */

#endif  /* _QCC_LOCKCHECKERLEVEL_H */
