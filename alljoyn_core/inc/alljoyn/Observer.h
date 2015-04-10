#ifndef _ALLJOYN_OBSERVER_H
#define _ALLJOYN_OBSERVER_H
/**
 * @file
 * This file defines the class Observer.
 * An Observer takes care of discovery, session management and ProxyBusObject
 * creation for bus objects that implement a specific set of interfaces.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Status.h>

namespace ajn {

/**
 * ObjectId is a simple encapsulation of a bus object's unique name and object path.
 *
 * It represents the unique identity of any object on the AllJoyn bus.
 */
struct ObjectId {
    /**
     * Unique bus name (never a well-known name!) of the peer hosting this object.
     */
    qcc::String uniqueBusName;

    /**
     * Path of the object.
     */
    qcc::String objectPath;

    /**
     * Default constructor.
     *
     * Creates an invalid object id.
     */
    ObjectId();

    /**
     * Constructor.
     *
     * @param busname Unique(!) bus name of the peer hosting the object.
     * @param path    Object path
     */
    ObjectId(const qcc::String& busname, const qcc::String& path);

    /**
     * Constructor.
     *
     * @param ppbo ProxyBusObject* for which to construct an object id
     */
    ObjectId(const ProxyBusObject* ppbo);

    /**
     * Constructor.
     *
     * @param pbo ProxyBusObject for which to construct an object id
     */
    ObjectId(const ProxyBusObject& pbo);

    /**
     * Copy Constructor.
     *
     * @param other the ObjectId to copy
     */
    ObjectId(const ObjectId& other);

    /**
     * The assign operator for ObjectId
     *
     * @param other the ObjectId to assign to this ObjectId
     *
     * @return a reference to this ObjectId.
     */
    ObjectId& operator=(const ObjectId& other);

    /**
     * Equals operator for the ObjectId
     *
     * @param other the ObjectId to check for equality
     *
     * @return true it the two objects are equal
     */
    bool operator==(const ObjectId& other) const;

    /**
     * less than operator for the ObjectId
     *
     * @param other the ObjectId to check it this object is less than
     *
     * @returns true if this object is less than other
     */
    bool operator<(const ObjectId& other) const;

    /**
     * Check validity of the object path
     */
    bool IsValid() const;
};

/**
 * An Observer takes care of discovery, session management and ProxyBusObject
 * creation for bus objects that implement a specific set of interfaces.
 *
 * The Observer monitors About announcements, and automatically sets up sessions
 * with all peers that offer objects of interest (i.e. objects that implement at
 * least the set of mandatory interfaces for this Observer). The Observer creates
 * a proxy bus object for each discovered object. The Observer::Listener class
 * is used to inform the application about the discovery of new objects, and about
 * the disappearance of objects from the bus.
 *
 * Objects are considered lost in the following cases:
 * - they are un-announced via About
 * - the hosting peer has closed the session
 * - the hosting peer stopped responding to Ping requests
 */
class Observer {
  public:
    /**
     * Constructor.
     *
     * @param bus                    Bus attachment to which the Observer is attached.
     * @param mandatoryInterfaces    Set of interface names that a bus object
     *                               MUST implement to be discoverable by this Observer.
     * @param numMandatoryInterfaces number of elements in the mandatoryInterfaces array.
     *
     * Some things to take into account:
     *   - the Observer will only discover objects that are announced through About.
     *   - the interface names in mandatoryInterfaces must correspond with
     *     InterfaceDescriptions that have been registered with the bus
     *     attachment before creation of the Observer.
     *   - mandatoryInterfaces must not be empty or NULL
     */
    Observer(BusAttachment& bus,
             const char* mandatoryInterfaces[], size_t numMandatoryInterfaces);

    /**
     * Destructor.
     */
    ~Observer();

    /**
     * Abstract base class implemented by the application and called by the
     * Observer to inform the application about Observer-related events.
     */
    class Listener {
      public:
        virtual ~Listener() { }
        /**
         * A new object has been discovered.
         *
         * @param object a proxy bus object supporting announced in the About signal
         */
        virtual void ObjectDiscovered(ProxyBusObject& object) { QCC_UNUSED(object); };

        /**
         * A previously discovered object has been lost.
         *
         * @param object the proxy bus object representing the object that
         *               has been lost.
         *
         * Note that it is no longer possible to perform method calls (RPCs) on
         * this proxy object. If the object reappears, a new proxy object will
         * be created.
         */
        virtual void ObjectLost(ProxyBusObject& object) { QCC_UNUSED(object); };
    };

    /**
     * Register a listener.
     *
     * @param listener the listener to register
     * @param triggerOnExisting trigger ObjectDiscovered callbacks for
     *                          already-discovered objects
     */
    void RegisterListener(Listener& listener, bool triggerOnExisting = true);

    /**
     * Unregister a listener.
     *
     * @param listener the listener to unregister
     */
    void UnregisterListener(Listener& listener);

    /**
     * Unregister all listeners.
     *
     * There is no real need to unregister all listeners before the Observer
     * is destroyed, but it is considered good style to do so.
     */
    void UnregisterAllListeners();

    /*
     * All methods below that return ProxyBusObjects will return an
     * invalid object if appropriate (if there is no object with this ObjectId
     * or when the iteration is finished). This can be checked with the
     * ProxyBusObject::IsValid() method
     */

    /**
     * Get a proxy object.
     *
     * @param oid The object id of the object you want to retrieve.
     *
     * @return the proxy object with the requested ObjectId
     *
     * This call will always return a ProxyBusObject, even if the Observer
     * has not discovered the object you're looking for. In that case, an invalid
     * ProxyBusObject will be returned. You can check this condition with
     * the ProxyBusObject::IsValid() method.
     */
    ProxyBusObject Get(const ObjectId& oid);

    /**
     * Get a proxy object.
     *
     * @param uniqueBusName The unique bus name of the peer hosting the object
     *                      you want to retrieve.
     * @param objectPath    The path of the object you want to retrieve.
     *
     * @return The discovered proxy object.
     *
     * This call will always return a ProxyBusObject, even if the Observer
     * has not discovered the object you're looking for. In that case, an invalid
     * ProxyBusObject will be returned. You can check this condition with
     * the ProxyBusObject::IsValid() method.
     */
    ProxyBusObject Get(const qcc::String& uniqueBusName, const qcc::String& objectPath) {
        return Get(ObjectId(uniqueBusName, objectPath));
    }

    /**
     * Get the first proxy object.
     *
     * The GetFirst/GetNext pair of methods is useful for iterating over all discovered
     * objects. The iteration is over when the proxy object returned by either call is
     * not valid (see ProxyBusObject::IsValid).
     *
     * @return the first proxy object found in the observer
     */
    ProxyBusObject GetFirst();

    /**
     * Get the next proxy object.
     *
     * The GetFirst/GetNext pair of methods is useful for iterating over all discovered
     * objects. The iteration is over when the proxy object returned by either call is
     * not valid (see ProxyBusObject::IsValid).
     *
     * @param oid This method will return the proxy object immediately following the one
     *            with this object id.
     * @return the proxy object immediately following the one identified by the oid object id
     */
    ProxyBusObject GetNext(const ObjectId& oid);

    /**
     * Get the next proxy object.
     *
     * The GetFirst/GetNext pair of methods is useful for iterating over all discovered
     * objects. The iteration is over when the proxy object returned by either call is
     * not valid (see ProxyBusObject::IsValid).
     *
     * @param mpbo This method will return the proxy object immediately following this one
     *
     * @return the proxy object immediately following the mpbo proxy object
     */
    ProxyBusObject GetNext(const ProxyBusObject& mpbo) {
        return GetNext(ObjectId(mpbo));
    }

    class Internal;
  private:
    Internal* internal;

    /* no copy constructors or assignment */
    Observer(const Observer& other);
    Observer& operator=(const Observer& other);
};

}
#endif
