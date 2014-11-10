////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
