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
#import "QASAboutService.h"
#import "QASAboutPropertyStoreImpl.h"

/**
 QASAboutServiceApi is wrapper class that encapsulates the QASAboutService using a shared instance.
 */
@interface QASAboutServiceApi : QASAboutService

/**
 Destroy the shared instance.
 */
- (void)destroyInstance;

/**
 * Create an AboutServiceApi Shared instance.
 * @return AboutServiceApi instance(created only once).
 */
+ (id)sharedInstance;

/**
 Start teh service using a given AJNBusAttachment and PropertyStore.
 @param bus A reference to the AJNBusAttachment.
 @param store A reference to a property store.
 */
- (void)startWithBus:(AJNBusAttachment *)bus andPropertyStore:(QASAboutPropertyStoreImpl *)store;

/**
 Return a reference to the property store.
 */
- (ajn ::services ::AboutPropertyStoreImpl *)getPropertyStore;

@end
