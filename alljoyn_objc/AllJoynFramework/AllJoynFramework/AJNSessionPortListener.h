////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
 * app of session related events.
 */
@protocol AJNSessionPortListener <NSObject>

@required

/**
 * Accept or reject an incoming JoinSession request. The session does not exist until this
 * after this function returns.
 *
 * This callback is only used by session creators. Therefore it is only called on listeners
 * passed to BusAttachment::BindSessionPort.
 *
 * @param joiner            Unique name of potential joiner.
 * @param sessionPort       Session port that was joined.
 * @param options           Session options requested by the joiner.
 * @return   Return true if JoinSession request is accepted. false if rejected.
 */
- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options;

@optional

/**
 * Called by the bus when a session has been successfully joined. The session is now fully up.
 *
 * This callback is only used by session creators. Therefore it is only called on listeners
 * passed to AJNBusAttachment::bindSessionOnPort.
 *
 * @param joiner         Unique name of the joiner.
 * @param sessionId      Id of session.
 * @param sessionPort    Session port that was joined.
 */
- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort;

@end
