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

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNPropertyStore.h"

/**
 AJNAboutService is a wrapper for AboutService which uses AllJoyn BusObject and implements the org.alljoyn.About standard interface.
 Applications that provide AllJoyn IoE services use an instance of this class to announce
 their capabilities and other identifying details of the services being provided.
 */
@interface AJNAboutService : NSObject

/**
 A flag that indicate the service mode.
 */
@property bool isServiceStarted;

/**
 Register the bus and the property store to be used by AJNAboutService and set the isServiceStarted to true.
 @param bus A reference to the AJNBusAttachment.
 @param store A reference to a property store.
 */
- (void)registerBus:(AJNBusAttachment *)bus andPropertystore:(id <AJNPropertyStore> )store;

/**
 Register the AJNAboutService on the AllJoyn bus passing the port to be announced - only if isServiceStarted is true.
 @param port used to bind the session.
 @return ER_OK if successful.
 */
- (QStatus)registerPort:(AJNSessionPort)port;

/**
 * Unregister the AJNAboutService  from the bus.
 */
- (void)unregister;

/**
 Add object Descriptions to the AJNAboutService announcement.
 @param path The path of the interface.
 @param interfaceNames The name of the interface.
 @return ER_OK if successful.
 */
- (QStatus)addObjectDescriptionWithPath:(NSString *)path andInterfaceNames:(NSMutableArray *)interfaceNames;

/**
 Remove object Descriptions from the AboutService announcement.
 @param path The path of the interface.
 @param interfaceNames The name of the interface.
 @return ER_OK if successful.
 */
- (QStatus)removeObjectDescriptionWithPath:(NSString *)path andInterfaceNames:(NSMutableArray *)interfaceNames;

/**
 Send or replace the org.alljoyn.About.Announce sessionless signal.
 </br>Validate store and object announcements and emit the announce signal.
 @return ER_MANDATORY_FIELD_MISSING: Logs an error with specific field that has a problem.
 */
- (QStatus)announce;

@end
