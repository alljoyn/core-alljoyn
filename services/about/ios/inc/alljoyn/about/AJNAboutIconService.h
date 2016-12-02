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
#import "AJNBusAttachment.h"
#import "AJNBusObject.h"

/**
 AJNAboutIconService is an AJNBusObject that implements the org.alljoyn.Icon standard interface.
 Applications that provide AllJoyn IoE services to receive info about the Icon of the service.
 */
__deprecated
@interface AJNAboutIconService : AJNBusObject

/**
 Designated initializer
 Create an AJNAboutIconService Object using the passed parameters.
 @param bus A reference to the AJNBusAttachment.
 @param mimetype The mimetype of the icon.
 @param url The url of the icon.
 @param content The content of the icon.
 @param csize The size of the content in bytes.
 @return AJNAboutIconService if successful.
 */
- (id)initWithBus:(AJNBusAttachment *)bus mimeType:(NSString *)mimetype url:(NSString *)url content:(uint8_t *)content csize:(size_t)csize __deprecated;

/**
 Register the AJNAboutIconService  on the AllJoyn bus.
 @return ER_OK if successful.
 */
- (QStatus)registerAboutIconService __deprecated;

@end