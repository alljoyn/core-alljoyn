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
#import "AJNSessionOptions.h"

/**
 AJNAboutIconClient enables the user of the class to interact with the remote AboutServiceIcon instance.
 */

@interface AJNAboutIconClient : NSObject
/**
 Designated initializer.
 Create an AboutIconClient Object using the passed AJNBusAttachment.
 @param bus A reference to the AJNBusAttachment.
 */
- (id)initWithBus:(AJNBusAttachment *)bus;

/**
 Populate a given parameter with the icon url for a specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param url The url of the icon[out].
 @return ER_OK if successful.
 */
- (QStatus)urlFromBusName:(NSString *)busName url:(NSString **)url;

/**
 Populate a given parameter with the icon url for a specified bus name and session id.
 @param busName Unique or well-known name of AllJoyn bus.
 @param url The url of the icon[out].
 @param sessionId The session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)urlFromBusName:(NSString *)busName url:(NSString **)url sessionId:(AJNSessionId)sessionId;

/**
 Populate a given parameter with the icon content and the content size for a specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param content The retrieved content of the icon payload [out].
 @param contentSize The size of the content payload [out].
 @return ER_OK if successful.
 */
- (QStatus)contentFromBusName:(NSString *)busName content:(uint8_t **)content contentSize:(size_t&)contentSize;

/**
 Populate a given parameter with the icon content and the content size for a specified bus name and session id.
 @param busName Unique or well-known name of AllJoyn bus.
 @param content The retrieved content of the icon payload [out].
 @param contentSize The size of the content payload [out].
 @param sessionId The session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)contentFromBusName:(NSString *)busName content:(uint8_t **)content contentSize:(size_t&)contentSize sessionId:(AJNSessionId)sessionId;

/**
 Populate a given parameters with the version of the AboutIcontClient for a specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param version The AboutIcontClient version[out].
 @return ER_OK if successful.
 */
- (QStatus)versionFromBusName:(NSString *)busName version:(int&)version;

/**
 Populate a given parameters with the version of the AboutIcontClient for a specified bus name and session id.
 @param busName Unique or well-known name of AllJoyn bus.
 @param version The AboutIcontClient version[out].
 @param sessionId the session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)versionFromBusName:(NSString *)busName version:(int&)version sessionId:(AJNSessionId)sessionId;

/**
 Populate a given parameter with the icon mime type for a specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param mimeType The icon's mime type [out].
 @return ER_OK if successful.
 */
- (QStatus)mimeTypeFromBusName:(NSString *)busName mimeType:(NSString **)mimeType;

/**
 Populate a given parameter with the icon mime type for a specified bus name and session id.
 @param busName Unique or well-known name of AllJoyn bus.
 @param mimeType The icon's mime type [out].
 @param sessionId The session received after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)mimeTypeFromBusName:(NSString *)busName mimeType:(NSString **)mimeType sessionId:(AJNSessionId)sessionId;

/**
 Populate a given parameter with the icon size for a specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param size The size of the icon [out].
 @return ER_OK if successful.
 */
- (QStatus)sizeFromBusName:(NSString *)busName size:(size_t&)size;

/**
 Populate a given parameter with the icon size for a specified bus name and session id.
 @param busName Unique or well-known name of AllJoyn bus.
 @param size The size of the icon [out].
 @param sessionId The session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)sizeFromBusName:(NSString *)busName size:(size_t&)size sessionId:(AJNSessionId)sessionId;

@end
