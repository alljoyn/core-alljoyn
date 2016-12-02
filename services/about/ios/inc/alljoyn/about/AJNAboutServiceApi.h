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

#import <Foundation/Foundation.h>
#import "AJNAboutService.h"
#import "AJNAboutPropertyStoreImpl.h"

/**
 AJNAboutServiceApi is wrapper class that encapsulates the AJNAboutService using a shared instance.
 */
__deprecated
@interface AJNAboutServiceApi : AJNAboutService

/**
 Destroy the shared instance.
 */
- (void)destroyInstance __deprecated;

/**
 * Create an AboutServiceApi Shared instance.
 * @return AboutServiceApi instance(created only once).
 */
+ (id)sharedInstance __deprecated;

/**
 Start teh service using a given AJNBusAttachment and PropertyStore.
 @param bus A reference to the AJNBusAttachment.
 @param store A reference to a property store.
 */
- (void)startWithBus:(AJNBusAttachment *)bus andPropertyStore:(AJNAboutPropertyStoreImpl *)store __deprecated;

/**
 Return a reference to the property store.
 */
- (ajn ::services ::AboutPropertyStoreImpl *)getPropertyStore __deprecated;

@end