////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNObject.h"
#import "AJNStatus.h"

@interface AJNInit : NSObject

/**
 * This must be called prior to instantiating or using any AllJoyn
 * functionality.
 *
 * alljoynShutdown must be called for each invocation of alljoynInit.
 */
+ (QStatus)alljoynInit;

/**
 * Call this to release any resources acquired in AllJoynInit().  No AllJoyn
 * functionality may be used after calling this.
 *
 * alljoynShutdown must be called for each invocation of alljoynInit.
 * alljoynShutdown must not be called without a prior alljoynInit call.
 *
 */
+ (QStatus)alljoynShutdown;

/**
 * This must be called before using any AllJoyn router functionality.
 *
 * alljoynRouterShutdown must be called for each invocation of alljoynRouterInit.
 *
 * For an application that is a routing node (either standalone or bundled), the
 * complete initialization sequence is:
 * @code
 * [AJNInit alljoynInit];
 * [AJNInit alljoynRouterInit];
 * @endcode
 */
+ (QStatus)alljoynRouterInit;

/**
 * Call this to release any resources acquired in AllJoynRouterInit().  \
 *
 * alljoynRouterShutdown must be called for each invocation of alljoynRouterInit.
 * alljoynRouterShutdown must not be called without a prior alljoynRouterInit call.
 *
 * For an application that is a routing node (either standalone or bundled), the
 * complete shutdown sequence is:
 * @code
 * [AJNInit alljoynRouterShutdown];
 * [AJNInit alljoynShutdown];
 * @endcode
 */
+ (QStatus)alljoynRouterShutdown;
@end