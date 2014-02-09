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
#import "AJNBusObject.h"

/**
 AJNAboutIconService is an AJNBusObject that implements the org.alljoyn.Icon standard interface.
 Applications that provide AllJoyn IoE services to receive info about the Icon of the service.
 */
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
- (id)initWithBus:(AJNBusAttachment *)bus mimeType:(NSString *)mimetype url:(NSString *)url content:(uint8_t *)content csize:(size_t)csize;

/**
 Register the AJNAboutIconService  on the AllJoyn bus.
 @return ER_OK if successful.
 */
- (QStatus)registerAboutIconService;

@end
