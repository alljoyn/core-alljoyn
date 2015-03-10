////////////////////////////////////////////////////////////////////////////////
// Copyright (c) AllSeen Alliance. All rights reserved.
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
#import "AJNProxyBusObject.h"
#import "AJNObject.h"
#import "AJNObserverListener.h"

/** An Alljoyn Observer takes care of discovery, session management and
 * ProxyBusObject creation for bus objects that implement a specific set of
 * interfaces.
 *
 * The Observer monitors About announcements, and automatically sets up
 * sessions with all peers that offer objects of interest (i.e. objects that
 * implement at least the set of mandatory interfaces for this Observer). The
 * Observer creates a proxy bus object for each discovered object.
 * AJNObserverListener objects are used to inform the application about
 * the discovery of new objects, and about the disappearance of objects from
 * the bus.
 *
 * Objects are considered lost in the following cases:
 *
 * - they are un-announced via About
 * - the hosting peer has closed the session
 * - the hosting peer stopped responding to Ping requests
 */
@interface AJNObserver : AJNObject

/** Object initializer
 *
 * Some things to take into account:
 *
 * - the Observer will only discover objects that are announced through About.
 * - the interface names in mandatoryInterfaces must correspond with
 *   InterfaceDescriptions that have been registered with the bus attachment
 *   before creation of the Observer.
 * - mandatoryInterfaces must not be empty or nil
 *
 * @param proxyType Class type of the proxy objects
 * @param bus Bus attachment to which the Observer is attached.
 * @param mandatoryInterfaces List of interface names (typeof NSString) that a bus object MUST implement to be discoverable by this Observer.
 * @return the newly created Observer.
 */
- (instancetype)initWithProxyType:(Class)proxyType busAttachment:(AJNBusAttachment*)bus mandatoryInterfaces:(NSArray *)interfaces;

/** Registers the given listener to the observer
 *  @param listener Listener, which captures observer related events
 *  @param triggerOnExisting trigger didDiscoverObject:forObserver: callbacks for already-discovered objects
 */
- (void)registerObserverListener:(id<AJNObserverListener>)listener triggerOnExisting:(BOOL)triggerOnExisting;

/** Removes the specified listener from the observer
 *  @param listener Listener to be removed from the observer
 */
- (void)unregisterObserverListener:(id<AJNObserverListener>)listener;

/** Removes all registered listeners from the observer */
- (void)unregisterAllObserverListeners;

/** Retrieves a proxy object from the Observer cache identified with the service name and object path.
 *
 * If the requested object is not tracked by this observer, nil is returned instead.
 *
 * @param uniqueName The unique bus name.
 * @param path The absolute (non-relative) object path for the remote object.
 * @return Proxy bus object or nil
 */
- (AJNProxyBusObject*)getProxyForUniqueName:(NSString *)uniqueName objectPath:(NSString *)path;

/** Retrieve the first proxy object from the Observer cache
 *
 * The getfirst/getnext pair of functions is useful for iterating over all discovered
 * objects. The iteration is over when either call returns nil.
 *
 * @return Proxy bus object or nil
 */
- (AJNProxyBusObject *)getFirstProxy;

/** Retrieve the next proxy object immediately following the given one
 *
 * The getfirst/getnext pair of functions is useful for iterating over all discovered
 * objects. The iteration is over when either call returns nil.
 *
 * @param previousObject Previous object in the cache.
 * @return Proxy bus object or nil
 */
- (AJNProxyBusObject *)getProxyFollowing:(AJNProxyBusObject *)previousObject;

@end
