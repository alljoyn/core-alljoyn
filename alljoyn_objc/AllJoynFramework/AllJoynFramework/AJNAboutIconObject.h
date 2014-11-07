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
#import "AJNBusObject.h"
#import "AJNBusAttachment.h"
#import "AJNAboutIcon.h"

@interface AJNAboutIconObject : AJNBusObject
/**
 * Construct an About Icon BusObject.
 *
 * @param[in] bus  BusAttachment instance associated with this AboutService
 * @param[in] icon instance of an AboutIcon which holds the icon image
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment aboutIcon:(AJNAboutIcon *)aboutIcon;

/**
 * version of the org.alljoyn.Icon interface
 */
+ (uint16_t)version;

@end