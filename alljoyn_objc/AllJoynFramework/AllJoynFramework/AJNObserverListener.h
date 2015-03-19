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

@class AJNObserver;

/**
 * Protocol implemented by AllJoyn apps and called by AllJoyn to inform
 * the app of observer related events.
 */
@protocol AJNObserverListener <NSObject>
@optional

/** Called by the Observer when a new object was discovered for which it is intrerested in.
 *
 * @param obj Object that was discovered by the Observer.
 * @param observer Observer for which the event was received.
 */
-(void)didDiscoverObject:(AJNProxyBusObject *)proxy forObserver:(AJNObserver *)observer;

/** Called by the Observer when a previous discovered object got lost.
 *
 * @param obj Object known to the Observer for which it is no longer detected.
 * @param observer Observer for which the event was received.
 */
-(void)didLoseObject:(AJNProxyBusObject *)proxy forObserver:(AJNObserver *)observer;

@end
