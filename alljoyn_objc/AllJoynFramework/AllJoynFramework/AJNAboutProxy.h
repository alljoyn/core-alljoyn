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
#import "AJNSessionOptions.h"
#import "AJNObject.h"
#import "AJNBusObject.h"
#import "AJNStatus.h"
#import "AJNAboutDataListener.h"
#import "AJNMessageArgument.h"



@interface AJNAboutProxy : AJNObject


/**
 * AboutProxy Constructor
 *
 * @param  bus reference to BusAttachment
 * @param[in] busName Unique or well-known name of AllJoyn bus
 * @param[in] sessionId the session received after joining AllJoyn session
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment busName:(NSString *)serviceName sessionId:(AJNSessionId)sessionId;

/**
 * Get the ObjectDescription array for specified bus name.
 *
 * @param[out] objectDescs  objectDescs  Description of busName's remote objects.
 *
 * @return
 *   - ER_OK if successful.
 *   - ER_BUS_REPLY_IS_ERROR_MESSAGE on unknown failure.
 */

- (QStatus)getObjectDescriptionUsingMsgArg:(AJNMessageArgument *)objectDesc;

/**
 * Get the AboutData  for specified bus name.
 *
 * @param[in] languageTag is the language used to request the AboutData.
 * @param[out] data is reference of AboutData that is filled by the function
 *
 * @return
 *    - ER_OK if successful.
 *    - ER_LANGUAGE_NOT_SUPPORTED if the language specified is not supported
 *    - ER_BUS_REPLY_IS_ERROR_MESSAGE on unknown failure
 */
- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)data;

/**
 * GetVersion get the About version
 *
 * @param[out] version of the service.
 *
 * @return ER_OK on success
 */

- (QStatus)getVersion:(uint16_t *)version;

@end