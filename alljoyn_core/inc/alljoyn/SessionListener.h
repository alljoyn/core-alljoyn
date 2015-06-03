/**
 * @file
 * SessionListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to receive sessions related event information.
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
#ifndef _ALLJOYN_SESSIONLISTENER_H
#define _ALLJOYN_SESSIONLISTENER_H

#ifndef __cplusplus
#error Only include SessionListener.h in C++ code.
#endif

#include <alljoyn/Session.h>

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of session related events.
 */
class SessionListener {
  public:

    /** Reason for the session being lost */
    typedef enum {
        ALLJOYN_SESSIONLOST_INVALID                      = 0x00, /**< Invalid */
        ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION      = 0x01, /**< Remote end called LeaveSession */
        ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY   = 0x02, /**< Remote end closed abruptly */
        ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER            = 0x03, /**< Session binder removed this endpoint by calling RemoveSessionMember */
        ALLJOYN_SESSIONLOST_LINK_TIMEOUT                 = 0x04, /**< Link was timed-out */
        ALLJOYN_SESSIONLOST_REASON_OTHER                 = 0x05, /**< Unspecified reason for session loss */
        ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER_SELF       = 0x06, /**< Session binder removed its joiner part by calling RemoveSessionMember (selfjoin only) */
    } SessionLostReason;

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~SessionListener() { }

    /**
     * Called by the bus when an existing session becomes disconnected.
     *
     * @param sessionId     Id of session that was lost.
     * @param reason        The reason for the session being lost
     */
    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_UNUSED(sessionId);
        QCC_UNUSED(reason);
    }

    /**
     * Called by the bus when a member of a multipoint session is added.
     *
     * @param sessionId     Id of session whose member(s) changed.
     * @param uniqueName    Unique name of member who was added.
     */
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        QCC_UNUSED(sessionId);
        QCC_UNUSED(uniqueName);
    }

    /**
     * Called by the bus when a member of a multipoint session is removed.
     *
     * @param sessionId     Id of session whose member(s) changed.
     * @param uniqueName    Unique name of member who was removed.
     */
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        QCC_UNUSED(sessionId);
        QCC_UNUSED(uniqueName);
    }
};

}

#endif
