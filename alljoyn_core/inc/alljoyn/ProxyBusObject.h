#ifndef _ALLJOYN_REMOTEBUSOBJECT_H
#define _ALLJOYN_REMOTEBUSOBJECT_H
/**
 * @file
 * This file defines the class ProxyBusObject.
 * The ProxyBusObject represents a single object registered  registered on the bus.
 * ProxyBusObjects are used to make method calls on these remotely located DBus objects.
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

#include <qcc/platform.h>
#include <set>
#include <qcc/String.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Message.h>
#include <alljoyn/Session.h>

#include <alljoyn/Status.h>

namespace qcc {
/** @internal Forward references */
class Mutex;
class Condition;
class Thread;
}

namespace ajn {

/** @internal Forward references */
class BusAttachment;

/**
 * Each %ProxyBusObject instance represents a single DBus/AllJoyn object registered
 * somewhere on the bus. ProxyBusObjects are used to make method calls on these
 * remotely located DBus objects.
 */
class ProxyBusObject : public MessageReceiver {
    friend class XmlHelper;
    friend class AllJoynObj;
    friend class MatchRuleTracker;

  public:

    /**
     * The default timeout for method calls (25 seconds)
     */
    static const uint32_t DefaultCallTimeout = 25000;

    /**
     * Base class used as a container for callback typedefs
     *
     * @internal Do not use this pattern for creating new public Async versions of the APIs.  See
     * the PropertiesChangedListener class below for an example of how to add new async APIs.
     */
    class Listener {
      public:
        /**
         * Callback registered with IntrospectRemoteObjectAsync()
         *
         * @param status    ER_OK if successful or an error status indicating the reason for the failure.
         * @param obj       Remote bus object that was introspected
         * @param context   Context passed in IntrospectRemoteObjectAsync()
         */
        typedef void (ProxyBusObject::Listener::* IntrospectCB)(QStatus status, ProxyBusObject* obj, void* context);

        /**
         * Callback registered with GetPropertyAsync()
         *
         * @param status    - ER_OK if the property get request was successfull or:
         *                  - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
         *                  - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
         *                  - Other error status codes indicating the reason the get request failed.
         * @param obj       Remote bus object that was introspected
         * @param value     If status is ER_OK a MsgArg containing the returned property value
         * @param context   Caller provided context passed in to GetPropertyAsync()
         */
        typedef void (ProxyBusObject::Listener::* GetPropertyCB)(QStatus status, ProxyBusObject* obj, const MsgArg& value, void* context);

        /**
         * Callback registered with GetAllPropertiesAsync()
         *
         * @param status    - ER_OK if the get all properties request was successfull or:
         *                  - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
         *                  - Other error status codes indicating the reason the get request failed.
         * @param obj         Remote bus object that was introspected
         * @param[out] values If status is ER_OK an array of dictionary entries, signature "a{sv}" listing the properties.
         * @param context     Caller provided context passed in to GetPropertyAsync()
         */
        typedef void (ProxyBusObject::Listener::* GetAllPropertiesCB)(QStatus status, ProxyBusObject* obj, const MsgArg& values, void* context);

        /**
         * Callback registered with SetPropertyAsync()
         *
         * @param status    - ER_OK if the property was successfully set or:
         *                  - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
         *                  - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
         *                  - Other error status codes indicating the reason the set request failed.
         * @param obj       Remote bus object that was introspected
         * @param context   Caller provided context passed in to SetPropertyAsync()
         */
        typedef void (ProxyBusObject::Listener::* SetPropertyCB)(QStatus status, ProxyBusObject* obj, void* context);
    };

    /**
     * Pure virtual base class for the Properties Changed listener.
     */
    class PropertiesChangedListener {
      public:
        virtual ~PropertiesChangedListener() { }

        /**
         * Callback to receive property changed events.
         *
         * @param obj           Remote bus object that owns the property that changed.
         * @param ifaceName     Name of the interface that defines the property.
         * @param changed       Property values that changed as an array of dictionary entries, signature "a{sv}".
         * @param invalidated   Properties whose values have been invalidated, signature "as".
         * @param context       Caller provided context passed in to RegisterPropertiesChangedListener
         */
        virtual void PropertiesChanged(ProxyBusObject& obj,
                                       const char* ifaceName,
                                       const MsgArg& changed,
                                       const MsgArg& invalidated,
                                       void* context) = 0;
    };

    /**
     * Create a default (unusable) %ProxyBusObject.
     *
     * This constructor exist only to support assignment.
     */
    ProxyBusObject();

    /**
     * Create an empty proxy object that refers to an object at given remote service name. Note
     * that the created proxy object does not contain information about the interfaces that the
     * actual remote object implements with the exception that org.freedesktop.DBus.Peer
     * interface is special-cased (per the DBus spec) and can always be called on any object. Nor
     * does it contain information about the child objects that the actual remote object might
     * contain. The security mode can be specified if known or can be derived from the XML
     * description.
     *
     * To fill in this object with the interfaces and child object names that the actual remote
     * object describes in its introspection data, call IntrospectRemoteObject() or
     * IntrospectRemoteObjectAsync().
     *
     * If the remote service name is set to a unique name, then both
     * GetServiceName() and GetUniqueName() will return the unique name.
     *
     * @param bus        The bus.
     * @param service    The remote service name (well-known or unique).
     * @param path       The absolute (non-relative) object path for the remote object.
     * @param sessionId  The session id the be used for communicating with remote object.
     * @param secure     The security mode for the remote object.
     */
    ProxyBusObject(BusAttachment& bus, const char* service, const char* path, SessionId sessionId, bool secure = false);

    /**
     * Create an empty proxy object that refers to an object at given remote service name. Note
     * that the created proxy object does not contain information about the interfaces that the
     * actual remote object implements with the exception that org.freedesktop.DBus.Peer
     * interface is special-cased (per the DBus spec) and can always be called on any object. Nor
     * does it contain information about the child objects that the actual remote object might
     * contain. The security mode can be specified if known or can be derived from the XML
     * description.
     *
     * To fill in this object with the interfaces and child object names that the actual remote
     * object describes in its introspection data, call IntrospectRemoteObject() or
     * IntrospectRemoteObjectAsync().
     *
     * This version is primarily used during introspection where the
     * unique name is known.  If, when creating the ProxyBusObject,
     * both an alias is known and the unique name is known, then use
     * this constructor.  Note, that the only the service name is used
     * when generating messages.
     *
     * @param bus        The bus.
     * @param service    The remote service name (well-known or unique).
     * @param uniqueName The remote service's unique name.
     * @param path       The absolute (non-relative) object path for the remote object.
     * @param sessionId  The session id the be used for communicating with remote object.
     * @param secure     The security mode for the remote object.
     */
    ProxyBusObject(BusAttachment& bus, const char* service, const char* uniqueName, const char* path, SessionId sessionId, bool secure = false);

    /**
     *  %ProxyBusObject destructor.
     */
    virtual ~ProxyBusObject();

    /**
     * Return the absolute object path for the remote object.
     *
     * @return Object path
     */
    const qcc::String& GetPath(void) const;

    /**
     * Return the remote service name for this object.
     *
     * @return Service name (typically a well-known service name but may be a unique name)
     */
    const qcc::String& GetServiceName(void) const;

    /**
     * Return the remote unique name for this object.
     *
     * @return Service name (typically a well-known service name but may be a unique name)
     */
    const qcc::String& GetUniqueName(void) const;

    /**
     * Return the session Id for this object.
     *
     * @return Session Id
     */
    SessionId GetSessionId(void) const;

    /**
     * Query the remote object on the bus to determine the interfaces and
     * children that exist. Use this information to populate this proxy's
     * interfaces and children.
     *
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus IntrospectRemoteObject(uint32_t timeout = DefaultCallTimeout);

    /**
     * Query the remote object on the bus to determine the interfaces and
     * children that exist. Use this information to populate this object's
     * interfaces and children.
     *
     * This call executes asynchronously. When the introspection response
     * is received from the actual remote object, this ProxyBusObject will
     * be updated and the callback will be called.
     *
     * This call exists primarily to allow introspection of remote objects
     * to be done inside AllJoyn method/signal/reply handlers and ObjectRegistered
     * callbacks.
     *
     * @param listener  Pointer to the object that will receive the callback.
     * @param callback  Method on listener that will be called.
     * @param context   User defined context which will be passed as-is to callback.
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     * @return
     *      - #ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus IntrospectRemoteObjectAsync(ProxyBusObject::Listener* listener, ProxyBusObject::Listener::IntrospectCB callback, void* context, uint32_t timeout = DefaultCallTimeout);

    /**
     * Get a property from an interface on the remote object.
     *
     * @param iface       Name of interface to retrieve property from.
     * @param property    The name of the property to get.
     * @param[out] value  Property value.
     * @param timeout     Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was obtained.
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus GetProperty(const char* iface, const char* property, MsgArg& value, uint32_t timeout = DefaultCallTimeout) const;

    /**
     * Make an asynchronous request to get a property from an interface on the remote object.
     * The property value is passed to the callback function.
     *
     * @param iface     Name of interface to retrieve property from.
     * @param property  The name of the property to get.
     * @param listener  Pointer to the object that will receive the callback.
     * @param callback  Method on listener that will be called.
     * @param context   User defined context which will be passed as-is to callback.
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     * @return
     *      - #ER_OK if the request to get the property was successfully issued .
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
     *      - An error status otherwise
     */
    QStatus GetPropertyAsync(const char* iface,
                             const char* property,
                             ProxyBusObject::Listener* listener,
                             ProxyBusObject::Listener::GetPropertyCB callback,
                             void* context,
                             uint32_t timeout = DefaultCallTimeout);

    /**
     * Get all properties from an interface on the remote object.
     *
     * @param iface       Name of interface to retrieve all properties from.
     * @param[out] values Property values returned as an array of dictionary entries, signature "a{sv}".
     * @param timeout     Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was obtained.
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus GetAllProperties(const char* iface, MsgArg& values, uint32_t timeout = DefaultCallTimeout) const;

    /**
     * Make an asynchronous request to get all properties from an interface on the remote object.
     *
     * @param iface     Name of interface to retrieve property from.
     * @param listener  Pointer to the object that will receive the callback.
     * @param callback  Method on listener that will be called.
     * @param context   User defined context which will be passed as-is to callback.
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     * @return
     *      - #ER_OK if the request to get all properties was successfully issued .
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
     *      - An error status otherwise
     */
    QStatus GetAllPropertiesAsync(const char* iface,
                                  ProxyBusObject::Listener* listener,
                                  ProxyBusObject::Listener::GetPropertyCB callback,
                                  void* context,
                                  uint32_t timeout = DefaultCallTimeout);

    /**
     * Set a property on an interface on the remote object.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param property  The name of the property to set
     * @param value     The value to set
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was set
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus SetProperty(const char* iface, const char* property, MsgArg& value, uint32_t timeout = DefaultCallTimeout) const;

    /**
     * Make an asynchronous request to set a property on an interface on the remote object.
     * A callback function reports the success or failure of ther operation.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param property  The name of the property to set.
     * @param value     The value to set
     * @param listener  Pointer to the object that will receive the callback.
     * @param callback  Method on listener that will be called.
     * @param context   User defined context which will be passed as-is to callback.
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     * @return
     *      - #ER_OK if the request to set the property was successfully issued .
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - An error status otherwise
     */
    QStatus SetPropertyAsync(const char* iface,
                             const char* property,
                             MsgArg& value,
                             ProxyBusObject::Listener* listener,
                             ProxyBusObject::Listener::SetPropertyCB callback,
                             void* context,
                             uint32_t timeout = DefaultCallTimeout);

    /**
     * Helper function to sychronously set a uint32 property on the remote object.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param property  The name of the property to set
     * @param u         The uint32 value to set
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was set
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus SetProperty(const char* iface, const char* property, uint32_t u, uint32_t timeout = DefaultCallTimeout) const {
        MsgArg arg("u", u); return SetProperty(iface, property, arg, timeout);
    }

    /**
     * Helper function to sychronously set an int32 property on the remote object.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param property  The name of the property to set
     * @param i         The int32 value to set
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was set
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus SetProperty(const char* iface, const char* property, int32_t i, uint32_t timeout = DefaultCallTimeout) const {
        MsgArg arg("i", i); return SetProperty(iface, property, arg, timeout);
    }

    /**
     * Helper function to sychronously set string property on the remote object from a C string.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param property  The name of the property to set
     * @param s         The string value to set
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was set
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus SetProperty(const char* iface, const char* property, const char* s, uint32_t timeout = DefaultCallTimeout) const {
        MsgArg arg("s", s); return SetProperty(iface, property, arg, timeout);
    }

    /**
     * Helper function to sychronously set string property on the remote object from a qcc::String.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param property  The name of the property to set
     * @param s         The string value to set
     * @param timeout   Timeout specified in milliseconds to wait for a reply
     *
     * @return
     *      - #ER_OK if the property was set
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus SetProperty(const char* iface, const char* property, const qcc::String& s, uint32_t timeout = DefaultCallTimeout) const {
        MsgArg arg("s", s.c_str()); return SetProperty(iface, property, arg, timeout);
    }

    /**
     * Function to register a handler for property change events.
     * Note that registering the same handler callback for the same
     * interface will overwrite the previous registration.  The same
     * handler callback may be registered for several different
     * interfaces simultaneously.
     *
     * Note that this makes method calls under the hood.  If this is
     * called from a message handler or other AllJoyn callback, you
     * must call BusAttachment::EnableConcurrentCallbacks().
     *
     * @param iface             Remote object's interface on which the property is defined.
     * @param properties        The name of the properties to monitor (NULL for all).
     * @param propertiesSize    Number of properties to monitor.
     * @param listener          Reference to the object that will receive the callback.
     * @param context           User defined context which will be passed as-is to callback.
     *
     * @return
     *      - #ER_OK if the handler was registered successfully
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     *      - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
     */
    QStatus RegisterPropertiesChangedListener(const char* iface,
                                              const char** properties,
                                              size_t propertiesSize,
                                              ProxyBusObject::PropertiesChangedListener& listener,
                                              void* context);

    /**
     * Function to unregister a handler for property change events.
     *
     * @param iface     Remote object's interface on which the property is defined.
     * @param listener  Reference to the object that used to receive the callback.
     *
     * @return
     *      - #ER_OK if the handler was registered successfully
     *      - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
     */
    QStatus UnregisterPropertiesChangedListener(const char* iface, ProxyBusObject::PropertiesChangedListener& listener);

    /**
     * Returns the interfaces implemented by this object. Note that all proxy bus objects
     * automatically inherit the "org.freedesktop.DBus.Peer" which provides the built-in "ping"
     * method, so this method always returns at least that one interface.
     *
     * @param ifaces     A pointer to an InterfaceDescription array to receive the interfaces. Can be NULL in
     *                   which case no interfaces are returned and the return value gives the number
     *                   of interface available.
     * @param numIfaces  The size of the InterfaceDescription array. If this value is smaller than the total
     *                   number of interfaces only numIfaces will be returned.
     *
     * @return  The number of interfaces returned or the total number of interfaces if ifaces is NULL.
     */
    size_t GetInterfaces(const InterfaceDescription** ifaces = NULL, size_t numIfaces = 0) const;

    /**
     * Returns a pointer to an interface description. Returns NULL if the object does not implement
     * the requested interface.
     *
     * @param iface  The name of interface to get.
     *
     * @return
     *      - A pointer to the requested interface description.
     *      - NULL if requested interface is not implemented or not found
     */
    const InterfaceDescription* GetInterface(const char* iface) const;

    /**
     * Tests if this object implements the requested interface.
     *
     * @param iface  The interface to check
     *
     * @return  true if the object implements the requested interface
     */
    bool ImplementsInterface(const char* iface) const { return GetInterface(iface) != NULL; }

    /**
     * Add an interface to this ProxyBusObject.
     *
     * Occasionally, AllJoyn library user may wish to call a method on
     * a %ProxyBusObject that was not reported during introspection of the remote object.
     * When this happens, the InterfaceDescription will have to be registered with the
     * Bus manually and the interface will have to be added to the %ProxyBusObject using this method.
     * @remark
     * The interface added via this call must have been previously registered with the
     * Bus. (i.e. it must have come from a call to Bus::GetInterface()).
     *
     * @param iface    The interface to add to this object. Must come from Bus::GetInterface().
     * @return
     *      - #ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus AddInterface(const InterfaceDescription& iface);

    /**
     * Add an existing interface to this object using the interface's name.
     *
     * @param name   Name of existing interface to add to this object.
     * @return
     *      - #ER_OK if successful.
     *      - An error status otherwise.
     */
    QStatus AddInterface(const char* name);

    /**
     * Returns an array of ProxyBusObject pointers for the children of this
     * %ProxyBusObject.
     *
     * @param children     An array to %ProxyBusObject pointers to receive the
     *                     children.  Can be NULL in which case no children are
     *                     returned and the return value gives the number of
     *                     children available.  Note that the lifetime of the
     *                     pointers is limited to the lifetime of the parent.
     * @param numChildren  The size of the %ProxyBusObject array. If this value
     *                     is smaller than the total number of children only
     *                     numChildren will be returned.
     *
     * @return  The number of children returned or the total number of children
     *          if children is NULL.
     */
    size_t GetChildren(ProxyBusObject** children = NULL, size_t numChildren = 0);

    /**
     * Returns an array of _ProxyBusObjects for the children of this %ProxyBusObject.
     * Unlike the unmanaged version of GetChildren, it is expected the caller will call
     * delete on each _ProxyBusObject in the array returned.
     *
     * @param children     A pointer to a %_ProxyBusObject array to receive the children. Can be NULL in
     *                     which case no children are returned and the return value gives the number
     *                     of children available.
     * @param numChildren  The size of the %_ProxyBusObject array. If this value is smaller than the total
     *                     number of children only numChildren will be returned.
     *
     * @return  The number of children returned or the total number of children if children is NULL.
     */
    QCC_DEPRECATED(size_t GetManagedChildren(void* children = NULL, size_t numChildren = 0));

    /**
     * Get a path descendant ProxyBusObject (child) by its absolute or relative path name.
     *
     * For example, if this ProxyBusObject's path is @c "/foo/bar", then you
     * can retrieve the ProxyBusObject for @c "/foo/bar/bat/baz" by calling
     * @c GetChild("/foo/bar/bat/baz") or @c GetChild("/bat/baz").  The pointer
     * that is returned is owned by the parent instance of ProxyBusObject and
     * will be deleted when the parent is destroyed.
     *
     * @param path the absolute or relative path for the child.
     *
     * @return
     *      - The (potentially deep) descendant ProxyBusObject
     *      - NULL if not found.
     */
    ProxyBusObject* GetChild(const char* path);

    /**
     * Get a path descendant _ProxyBusObject (child) by its absolute or relative path name.
     *
     * For example, if this _ProxyBusObject's path is @c "/foo/bar", then you can retrieve the
     * _ProxyBusObject for @c "/foo/bar/bat/baz" by calling @c GetChild("/foo/bar/bat/baz") or
     * @c GetChild("bat/baz"). Unlike the unmanaged version of GetChild, it is expected the
     * caller will call delete on the _ProxyBusObject returned.
     *
     * @param inPath the absolute or relative path for the child.
     *
     * @return
     *      - The (potentially deep) descendant _ProxyBusObject
     *      - NULL if not found.
     */
    QCC_DEPRECATED(void* GetManagedChild(const char* inPath));

    /**
     * Add a child object (direct or deep object path descendant) to this object.
     * If you add a deep path descendant, this method will create intermediate
     * ProxyBusObject children as needed.
     *
     * @remark
     *  - It is an error to try to add a child that already exists.
     *  - It is an error to try to add a child that has an object path that is not a descendant of this object's path.
     *
     * @param child  Child ProxyBusObject
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_BAD_CHILD_PATH if the path is a bad path
     *      - #ER_BUS_OBJ_ALREADY_EXISTS if the object already exists on the ProxyBusObject
     */
    QStatus AddChild(const ProxyBusObject& child);

    /**
     * Remove a child object and any descendants it may have.
     *
     * @param path   Absolute or relative (to this ProxyBusObject) object path.
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_BAD_CHILD_PATH if the path given was not a valid path
     *      - #ER_BUS_OBJ_NOT_FOUND if the Child object was not found
     *      - #ER_FAIL any other unexpected error.
     */
    QStatus RemoveChild(const char* path);

    /**
     * Make a synchronous method call from this object
     *
     * @param method       Method being invoked.
     * @param args         The arguments for the method call (can be NULL)
     * @param numArgs      The number of arguments
     * @param replyMsg     The reply message received for the method call
     * @param timeout      Timeout specified in milliseconds to wait for a reply
     * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
     *                     - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
     *                     - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
     *
     *
     * @return
     *      - #ER_OK if the method call succeeded and the reply message type is #MESSAGE_METHOD_RET
     *      - #ER_BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is #MESSAGE_ERROR
     */
    QStatus MethodCall(const InterfaceDescription::Member& method,
                       const MsgArg* args,
                       size_t numArgs,
                       Message& replyMsg,
                       uint32_t timeout = DefaultCallTimeout,
                       uint8_t flags = 0) const;

    /**
     * Make a synchronous method call from this object
     *
     * @param ifaceName    Name of interface.
     * @param methodName   Name of method.
     * @param args         The arguments for the method call (can be NULL)
     * @param numArgs      The number of arguments
     * @param replyMsg     The reply message received for the method call
     * @param timeout      Timeout specified in milliseconds to wait for a reply
     * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
     *                     - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
     *                     - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
     *
     * @return
     *      - #ER_OK if the method call succeeded and the reply message type is #MESSAGE_METHOD_RET
     *      - #ER_BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is #MESSAGE_ERROR
     */
    virtual QStatus MethodCall(const char* ifaceName,
                               const char* methodName,
                               const MsgArg* args,
                               size_t numArgs,
                               Message& replyMsg,
                               uint32_t timeout = DefaultCallTimeout,
                               uint8_t flags = 0) const;

    /**
     * Make a fire-and-forget method call from this object. The caller will not be able to tell if
     * the method call was successful or not. This is equivalent to calling MethodCall() with
     * flags == ALLJOYN_FLAG_NO_REPLY_EXPECTED. Because this call doesn't block it can be made from
     * within a signal handler.
     *
     * @param ifaceName    Name of interface.
     * @param methodName   Name of method.
     * @param args         The arguments for the method call (can be NULL)
     * @param numArgs      The number of arguments
     * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
     *                     - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
     *                     - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
     *
     * @return
     *      - #ER_OK if the method call succeeded
     */
    QStatus MethodCall(const char* ifaceName,
                       const char* methodName,
                       const MsgArg* args,
                       size_t numArgs,
                       uint8_t flags = 0) const
    {
        return MethodCallAsync(ifaceName, methodName, NULL, NULL, args, numArgs, NULL, 0, flags |= ALLJOYN_FLAG_NO_REPLY_EXPECTED);
    }

    /**
     * Make a fire-and-forget method call from this object. The caller will not be able to tell if
     * the method call was successful or not. This is equivalent to calling MethodCall() with
     * flags == ALLJOYN_FLAG_NO_REPLY_EXPECTED. Because this call doesn't block it can be made from
     * within a signal handler.
     *
     * @param method       Method being invoked.
     * @param args         The arguments for the method call (can be NULL)
     * @param numArgs      The number of arguments
     * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
     *                     - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
     *                     - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
     *
     * @return
     *      - #ER_OK if the method call succeeded and the reply message type is #MESSAGE_METHOD_RET
     */
    QStatus MethodCall(const InterfaceDescription::Member& method,
                       const MsgArg* args,
                       size_t numArgs,
                       uint8_t flags = 0) const
    {
        return MethodCallAsync(method, NULL, NULL, args, numArgs, NULL, 0, flags |= ALLJOYN_FLAG_NO_REPLY_EXPECTED);
    }

    /**
     * Make an asynchronous method call from this object
     *
     * @param method       Method being invoked.
     * @param receiver     The object to be called when the asych method call completes.
     * @param replyFunc    The function that is called to deliver the reply
     * @param args         The arguments for the method call (can be NULL)
     * @param numArgs      The number of arguments
     * @param receiver     The object to be called when the asych method call completes.
     * @param context      User-defined context that will be returned to the reply handler
     * @param timeout      Timeout specified in milliseconds to wait for a reply
     * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
     *                     - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
     *                     - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus MethodCallAsync(const InterfaceDescription::Member& method,
                            MessageReceiver* receiver,
                            MessageReceiver::ReplyHandler replyFunc,
                            const MsgArg* args = NULL,
                            size_t numArgs = 0,
                            void* context = NULL,
                            uint32_t timeout = DefaultCallTimeout,
                            uint8_t flags = 0) const;

    /**
     * Make an asynchronous method call from this object
     *
     * @param ifaceName    Name of interface for method.
     * @param methodName   Name of method.
     * @param receiver     The object to be called when the asynchronous method call completes.
     * @param replyFunc    The function that is called to deliver the reply
     * @param args         The arguments for the method call (can be NULL)
     * @param numArgs      The number of arguments
     * @param context      User-defined context that will be returned to the reply handler
     * @param timeout      Timeout specified in milliseconds to wait for a reply
     * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
     *                     - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
     *                     - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus MethodCallAsync(const char* ifaceName,
                            const char* methodName,
                            MessageReceiver* receiver,
                            MessageReceiver::ReplyHandler replyFunc,
                            const MsgArg* args = NULL,
                            size_t numArgs = 0,
                            void* context = NULL,
                            uint32_t timeout = DefaultCallTimeout,
                            uint8_t flags = 0) const;

    /**
     * Initialize this proxy object from an XML string. Calling this method does several things:
     *
     *  -# Create and register any new InterfaceDescription(s) that are mentioned in the XML.
     *     (Interfaces that are already registered with the bus are left "as-is".)
     *  -# Add all the interfaces mentioned in the introspection data to this ProxyBusObject.
     *  -# Recursively create any child ProxyBusObject(s) and create/add their associated @n
     *     interfaces as mentioned in the XML. Then add the descendant object(s) to the appropriate
     *     descendant of this ProxyBusObject (in the children collection). If the named child object
     *     already exists as a child of the appropriate ProxyBusObject, then it is updated
     *     to include any new interfaces or children mentioned in the XML.
     *
     * Note that when this method fails during parsing, the return code will be set accordingly.
     * However, any interfaces which were successfully parsed prior to the failure
     * may be registered with the bus. Similarly, any objects that were successfully created
     * before the failure will exist in this object's set of children.
     *
     * @param xml         An XML string in DBus introspection format.
     * @param identifier  An optional identifying string to include in error logging messages.
     *
     * @return
     *      - #ER_OK if parsing is completely successful.
     *      - An error status otherwise.
     */
    QStatus ParseXml(const char* xml, const char* identifier = NULL);

    /**
     * Explicitly secure the connection to the remote peer for this proxy object. Peer-to-peer
     * connections can only be secured if EnablePeerSecurity() was previously called on the bus
     * attachment for this proxy object. If the peer-to-peer connection is already secure this
     * function does nothing. Note that peer-to-peer connections are automatically secured when a
     * method call requiring encryption is sent.
     *
     * This call causes messages to be send on the bus, therefore it cannot be called within AllJoyn
     * callbacks (method/signal/reply handlers or ObjectRegistered callbacks, etc.)
     *
     * @param forceAuth  If true, forces an re-authentication even if the peer connection is already
     *                   authenticated.
     *
     * @return
     *          - #ER_OK if the connection was secured or an error status indicating that the
     *            connection could not be secured.
     *          - #ER_BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment::EnablePeerSecurity() has not been called.
     *          - #ER_AUTH_FAIL if the attempt(s) to authenticate the peer failed.
     *          - Other error status codes indicating a failure.
     */
    QStatus SecureConnection(bool forceAuth = false);

    /**
     * Asynchronously secure the connection to the remote peer for this proxy object. Peer-to-peer
     * connections can only be secured if EnablePeerSecurity() was previously called on the bus
     * attachment for this proxy object. If the peer-to-peer connection is already secure this
     * function does nothing. Note that peer-to-peer connections are automatically secured when a
     * method call requiring encryption is sent.
     *
     * Notification of success or failure is via the AuthListener passed to EnablePeerSecurity().
     *
     * @param forceAuth  If true, forces an re-authentication even if the peer connection is already
     *                   authenticated.
     *
     * @return
     *          - #ER_OK if securing could begin.
     *          - #ER_BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment::EnablePeerSecurity() has not been called.
     *          - Other error status codes indicating a failure.
     */
    QStatus SecureConnectionAsync(bool forceAuth = false);

    /**
     * Checks if other refers to the same internal instance has this.
     *
     * @param other  The object being compared
     * @return true iff this has identical internals to other
     */
    bool iden(const ProxyBusObject& other) const { return internal.iden(other.internal); }

    /**
     * Equivalence operator.
     *
     * @param other  The object being compared
     * @return true iff this is the same as other
     */
    bool operator==(const ProxyBusObject& other) const;

    /**
     * Less than operator.
     *
     * @param other  The object being compared
     * @return true iff this is less than other
     */
    bool operator<(const ProxyBusObject& other) const;

    /**
     * Assignment operator.
     *
     * @param other  The object being assigned from
     * @return a copy of the ProxyBusObject
     */
    ProxyBusObject& operator=(const ProxyBusObject& other);

    /**
     * Copy constructor
     *
     * @param other  The object being copied from.
     */
    ProxyBusObject(const ProxyBusObject& other);

    /**
     * Indicates if this is a valid (usable) proxy bus object.
     *
     * @return true if a valid proxy bus object, false otherwise.
     */
    bool IsValid() const;

    /**
     * Indicates if the remote object for this proxy bus object is secure.
     *
     * @return  true if the object is secure
     */
    bool IsSecure() const;

    /**
     * Enable property caching for this proxy bus object.
     */
    void EnablePropertyCaching();

  private:

    class Internal;
    qcc::ManagedObj<Internal> internal; /**< Internal ProxyBusObject state */

    bool isExiting;             /**< true iff ProxyBusObject is in the process of being destroyed */

    ProxyBusObject(qcc::ManagedObj<Internal> internal);

    /**
     * @internal
     * Called by XmlHelper to set the secure flag when introspecting.
     *
     * @param isSecure  indicates if node is secure or not
     */
    void SetSecure(bool isSecure);

    /**
     * @internal
     * Method return handler used to process synchronous method calls.
     *
     * @param msg     Method return message
     * @param context Opaque context passed from method_call to method_return
     */
    void SyncReplyHandler(Message& msg, void* context);

    /**
     * @internal
     * Introspection method_reply handler. (Internal use only)
     *
     * @param message Method return Message
     * @param context Opaque context passed from method_call to method_return
     */
    void IntrospectMethodCB(Message& message, void* context);

    /**
     * @internal
     * GetProperty method_reply handler. (Internal use only)
     *
     * @param message Method return Message
     * @param context Opaque context passed from method_call to method_return
     */
    void GetPropMethodCB(Message& message, void* context);

    /**
     * @internal
     * GetAllProperties method_reply handler. (Internal use only)
     *
     * @param message Method return Message
     * @param context Opaque context passed from method_call to method_return
     */
    void GetAllPropsMethodCB(Message& message, void* context);

    /**
     * @internal
     * SetProperty method_reply handler. (Internal use only)
     *
     * @param message Method return Message
     * @param context Opaque context passed from method_call to method_return
     */
    void SetPropMethodCB(Message& message, void* context);

    /**
     * @internal
     * Set the B2B endpoint to use for all communication with remote object.
     * This method is for internal use only.
     *
     * @param b2bEp the RemoteEndpoint for Bus to Bus communication
     */
    void SetB2BEndpoint(RemoteEndpoint& b2bEp);

    /**
     * @internal
     * Internal introspection xml parse tree type.
     */
    struct IntrospectionXml;

    /**
     * @internal
     * Parse a single introspection @<node@> element.
     *
     * @param node  XML element (must be a @<node@>).
     *
     * @return
     *       - #ER_OK if completely successful.
     *       - An error status otherwise
     */
    static QStatus ParseNode(const IntrospectionXml& node);

    /**
     * @internal
     * Parse a single introspection @<interface@> element.
     *
     * @param ifc  XML element (must be an @<interface@>).
     *
     * @return
     *       - #ER_OK if completely successful.
     *       - An error status otherwise
     */
    static QStatus ParseInterface(const IntrospectionXml& ifc);
};

/**
 * _ProxyBusObject is a reference counted (managed) version of ProxyBusObject
 */
QCC_DEPRECATED(typedef qcc::ManagedObj<ProxyBusObject> _ProxyBusObject);

}

#endif
