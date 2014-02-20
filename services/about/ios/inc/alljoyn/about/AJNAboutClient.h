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

/**
 AJNAboutClient is a helper class used by an AllJoyn IoE client application to discover services being offered by nearby AllJoyn
 IoE service applications. AJNAboutClient enables the user of the class to interact with the remote AboutService instance.
 */

@interface AJNAboutClient : NSObject

/**
 Designated initializer
 Create an AboutClient Object using the passed AJNBusAttachment
 @param bus A reference to the AJNBusAttachment.
 @return AboutClient if successful.
 */
- (id)initWithBus:(AJNBusAttachment *)bus;

/**
 Populate a given dictionary with the object Description(s) for a specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param objectDescs Description of busName's remote objects [in,out].
 @param sessionId The session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)objectDescriptionsWithBusName:(NSString *)busName andObjectDescriptions:(NSMutableDictionary **)objectDescs andSessionId:(uint32_t)sessionId;

/**
 Populate a given dictionary with the AboutData for specified bus name.
 @param busName Unique or well-known name of AllJoyn bus.
 @param languageTag The language used to request the AboutData.
 @param data A reference of AboutData that is filled by the function [in,out].
 @param sessionId The session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)aboutDataWithBusName:(NSString *)busName andLanguageTag:(NSString *)languageTag andAboutData:(NSMutableDictionary **)data andSessionId:(uint32_t)sessionId;

/**
 Populate a given parameter with the About version
 @param busName Unique or well-known name of AllJoyn bus.
 @param version The service version.
 @param sessionId The session received  after joining AllJoyn session.
 @return ER_OK if successful.
 */
- (QStatus)versionWithBusName:(NSString *)busName andVersion:(int)version andSessionId:(AJNSessionId)sessionId;

@end
