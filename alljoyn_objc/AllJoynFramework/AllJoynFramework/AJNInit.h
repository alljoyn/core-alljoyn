////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
#import "AJNObject.h"

@interface AJNInit : NSObject

/**
 * This must be called prior to instantiating or using any AllJoyn
 * functionality.
 */
+ (QStatus)alljoynInit;

/**
 * Call this to release any resources acquired in AllJoynInit().  No AllJoyn
 * functionality may be used after calling this.
 */
+ (QStatus)alljoynShutdown;

/**
 * This must be called before using any AllJoyn router functionality.
 *
 * For an application that is a routing node (either standalone or bundled), the
 * complete initialization sequence is:
 * @code
 * [AJNInit alljoynInit];
 * [AJNInit alljoynRouterInit];
 * @endcode
 */
+ (QStatus)alljoynRouterInit;

/**
 * Call this to release any resources acquired in AllJoynRouterInit().
 *
 * For an application that is a routing node (either standalone or bundled), the
 * complete shutdown sequence is:
 * @code
 * [AJNInit alljoynRouterShutdown];
 * [AJNInit alljoynShutdown];
 * @endcode
 */
+ (QStatus)alljoynRouterShutdown;
@end
