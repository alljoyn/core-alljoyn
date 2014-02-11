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

/**
 AJNAnnouncementListener is a helper protocol used by AnnounceHandlerAdapter to receive AboutService signal notification.
 The user of the class need to implement the protocol method.
 */
@protocol AJNAnnouncementListener <NSObject>

/**
 Called by the AnnounceHandlerAdapter when a new announcement received. This gives the listener implementation the opportunity to save a reference to announcemet parameters.
 @param version The version of the AboutService.
 @param port The port used by the AboutService
 @param busName Unique or well-known name of AllJoyn bus.
 @param objectDescs Map of ObjectDescriptions using NSMutableDictionary, describing interfaces
 @param aboutData Map of AboutData using NSMutableDictionary, describing message args
 */
- (void)announceWithVersion:(uint16_t)version port:(uint16_t)port busName:(NSString *)busName objectDescriptions:(NSMutableDictionary *)objectDescs aboutData:(NSMutableDictionary **)aboutData;
@end
