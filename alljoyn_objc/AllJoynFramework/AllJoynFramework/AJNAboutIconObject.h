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
#import "AJNBusObject.h"
#import "AJNBusAttachment.h"
#import "AJNAboutIcon.h"

@interface AJNAboutIconObject : AJNBusObject
/**
 * Construct an About Icon BusObject.
 *
 * @param[in] bus  BusAttachment instance associated with this AboutService
 * @param[in] icon instance of an AboutIcon which holds the icon image
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment aboutIcon:(AJNAboutIcon *)aboutIcon;

/**
 * version of the org.alljoyn.Icon interface
 */
+ (uint16_t)version;

@end