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

#import <UIKit/UIKit.h>
#import "AJNBusListener.h"
#import "AJNSessionPortListener.h"

/** Implementation of the Bus listener. Called by AllJoyn to inform apps of bus related events.
 */
@interface CommonBusListener : NSObject <AJNBusListener, AJNSessionPortListener>

/**
 Init the Value of the SessionPort associated with this SessionPortListener
 @param sessionPort The port of the session
 @return Object ID
 */
- (id)initWithServicePort:(AJNSessionPort)servicePort;

/**
 Set the Value of the SessionPort associated with this SessionPortListener
 @param sessionPort The port of the session
 */
- (void)setSessionPort:(AJNSessionPort)sessionPort;

/**
 Get the SessionPort of the listener
 @return The port of the session
 */
- (AJNSessionPort)sessionPort;
@end