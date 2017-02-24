////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNPingListener.h"
#import "AJNObject.h"
#import "AJNBusAttachment.h"

/**
 * AutoPinger class
 */
@interface AJNAutoPinger : AJNObject

/**
 * Create instance of autopinger
 *
 * @param busAttachment reference to the BusAttachment associated with this
 *                      autopinger.
 *
 */
- (id)initWithBusAttachment:(AJNBusAttachment*)busAttachment;

/**
 * Destructor.
 *
 * Do not destroy an AutoPinger instance from within a PingListener
 * callback. This will cause a deadlock.
 */
- (void)dealloc;

/**
 * Pause all ping actions
 */
- (void)pause;

/**
 * Resume ping actions
 */
- (void)resume;

/**
 * Define new ping group
 *
 * @param  group Ping group name
 * @param  listener Listener called when a change was detected in the reachability of a destination
 * @param  pingInterval Ping interval in seconds
 */
- (void)addPingGroup:(NSString*)group withDelegate:(id<AJNPingListener>)listener withInterval:(uint32_t)pingInterval;

/**
 * Remove complete ping group, including all destinations
 *
 * Do not invoke this method from within a PingListener callback. This will
 * cause a deadlock.
 *
 * @param  group Ping group name
 */
- (void)removePingGroup:(NSString*)group;

/**
 * Set ping interval of the specified group
 *
 * @param  group Ping group name
 * @param  pingInterval Ping interval in seconds
 * @return
 *  - ER_OK: Interval updated
 *  - ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
 */
- (QStatus)setPingInterval:(NSString*)group withInterval:(uint32_t)pingInterval;

/**
 * Add a destination to the specified ping group
 * Destinations are refcounted and must be removed N times if they were added N times
 *
 * @param  group Ping group name
 * @param  destination Destination name to be pinged
 * @return
 *  - ER_OK: destination added
 *  - ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
 */
- (QStatus)addDestination:(NSString*)group forDestination:(NSString*)destination;

/**
 * Remove a destination from the specified ping group
 * This will lower the refcount by one and only remove the destination when the refcount reaches zero
 *
 * @param  group Ping group name
 * @param  destination Destination name to be removed
 * @param  removeAll Rather than decrementing the refcount by one, set refcount to zero and remove
 * @return
 *  - ER_OK: destination removed or was not present
 *  - ER_BUS_PING_GROUP_NOT_FOUND: group did not exist
 */
- (QStatus)removeDestination:(NSString*)group forDestination:(NSString*)destination shouldRemoveAll:(BOOL)removeAll;

@end