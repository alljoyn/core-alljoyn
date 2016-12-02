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
#import "AJNAboutIcon.h"
#import "AJNSessionOptions.h"
#import "AJNObject.h"
#import "AJNAboutIcon.h"
#import "AJNBusAttachment.h"

@interface AJNAboutIconProxy : AJNObject

/**
 * Construct an AboutIconProxy.
 * @param bus reference to BusAttachment
 * @param[in] busName Unique or well-known name of AllJoyn bus
 * @param[in] sessionId the session received  after joining AllJoyn sessio
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment busName:(NSString *)busName sessionId:(AJNSessionId)sessionId;

/**
 * @param[out] icon class that holds icon content
 * @return ER_OK if successful
 */
- (QStatus)getIcon:(AJNAboutIcon *)icon;

/**
 * Get the version of the About Icon Interface
 *
 * @param[out] version of the AboutIcontClient
 *
 * @return ER_OK if successful
 */
- (QStatus)getVersion:(uint16_t *)version;

@end