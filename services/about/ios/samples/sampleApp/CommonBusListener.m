/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#import "CommonBusListener.h"
#import "AJNSessionOptions.h"

@interface CommonBusListener ()
@property AJNSessionPort servicePort;
@end

@implementation CommonBusListener

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

- (id)initWithServicePort:(AJNSessionPort)servicePort
{
	self = [super init];
	if (self) {
		self.servicePort = servicePort;
	}
	return self;
}

/**
 * Set the Value of the SessionPort associated with this SessionPortListener
 * @param sessionPort
 */
- (void)setSessionPort:(AJNSessionPort)sessionPort
{
	self.servicePort = sessionPort;
}

/**
 * Get the SessionPort of the listener
 * @return
 */
- (AJNSessionPort)sessionPort
{
	return self.servicePort;
}

//  protocol method
- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
	if (sessionPort != self.servicePort) {
		NSLog(@"Rejecting join attempt on unexpected session port %hu.", sessionPort);
		return false;
	}
	else {
		NSLog(@"Accepting join session request from %@ (proximity=%c, traffic=%u, transports=%hu).\n", joiner, options.proximity, options.trafficType, options.transports);
		return true;
	}
}

@end
