////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

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