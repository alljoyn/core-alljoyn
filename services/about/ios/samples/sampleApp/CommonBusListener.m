/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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