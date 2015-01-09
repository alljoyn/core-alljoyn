////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNSessionOptions.h"

/**
 * Protocol implemented by AllJoyn apps and called by AllJoyn to inform
 * the app of session related events.
 */
@protocol AJNSessionListener <NSObject>

/** Reason for the session being lost */
typedef enum{
    ALLJOYN_SESSIONLOST_INVALID                      = 0x00, /**< Invalid */
    ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION      = 0x01, /**< Remote end called LeaveSession
                                                              */
    ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY   = 0x02, /**< Remote end closed abruptly */
    ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER            = 0x03, /**< Session binder removed this en
                                                              dpoint by calling RemoveSessionMember */
    ALLJOYN_SESSIONLOST_LINK_TIMEOUT                 = 0x04, /**< Link was timed-out */
    ALLJOYN_SESSIONLOST_REASON_OTHER                 = 0x05, /**< Unspecified reason for session loss */
    ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER_SELF       = 0x06  /**< Session binder removed its joiner part 
                                                              by calling RemoveSessionMember (selfjoin only) */
}AJNSessionLostReason;

@optional

/**
 * Called by the bus when an existing session becomes disconnected.
 *
 * @param sessionId     Id of session that was lost.
 */
- (void)sessionWasLost:(AJNSessionId)sessionId;

/**
* Called by the bus when an existing session becomes disconnected.
*
* @param sessionId     Id of session that was lost.
* @param reason        The reason for the session being lost
*
*/
- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason;

/**
 * Called by the bus when a member of a multipoint session is added.
 *
 * @param memberName    Unique name of member who was added.
 * @param sessionId     Id of session whose member(s) changed.
 */
- (void)didAddMemberNamed:(NSString *)memberName toSession:(AJNSessionId)sessionId;

/**
 * Called by the bus when a member of a multipoint session is removed.
 *
 * @param memberName    Unique name of member who was added.
 * @param sessionId     Id of session whose member(s) changed.
 */
- (void)didRemoveMemberNamed:(NSString *)memberName fromSession:(AJNSessionId)sessionId;

@end
