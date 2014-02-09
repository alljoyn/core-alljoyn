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
#import "AJNAnnouncementListener.h"

/**
 AJNAnnouncementReceiver enable registering an announcement listener to receive org.alljoyn.about Announce signals
 */
@interface AJNAnnouncementReceiver : NSObject

/**
 Designated initializer.
 Create an AJNAnnouncementReceiver Object using the passed announcementListener an an AJNBusAttachment.
 @param announcementListener A reference to an announcement listener.
 @param bus A reference to the AJNBusAttachment.
 @return ER_OK if successful.
 */
- (id)initWithAnnouncementListener:(id <AJNAnnouncementListener> )announcementListener andBus:(AJNBusAttachment *)bus;

/**
 Register the to announcement listener to receive org.alljoyn.about Announce signals.
 @return ER_OK if successful.
 */
- (QStatus)registerAnnouncementReceiver;

/**
 Unregister the announcement listener from receiving org.alljoyn.about Announce signal.
 @return ER_OK if successful.
 */
- (QStatus)unRegisterAnnouncementReceiver;

@end
