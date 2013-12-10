////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#import "AJNSessionOptions.h"
#import "AJNStatus.h"

@class AJNBusAttachment;
@class AJNInterfaceDescription;
@class AJNInterfaceMember;
@class AJNMessage;
@class AJNMessageArgument;
@class AJNProxyBusObject;

/**
 * Asynchronous callback delegate for proxy bus object
 */
@protocol AJNProxyBusObjectDelegate <NSObject>

@optional

/**
 * Callback registered with AJNProxyBusObject::introspectRemoteObjectAsync
 *
 * @param object       Remote bus object that was introspected
 * @param context   Context passed into introspectRemoteObjectAsync
 * @param status ER_OK if successful
 */
- (void)didCompleteIntrospectionOfObject:(AJNProxyBusObject *)object context:(AJNHandle)context withStatus:(QStatus)status;

/**
 * Reply handler for asynchronous method call.
 *
 * @param replyMessage  The received message.
 * @param context       User-defined context passed to MethodCall and returned upon reply.
 */
- (void)didReceiveMethodReply:(AJNMessage *)replyMessage context:(AJNHandle)context;

/**
 * Handler for receiving the value of a property asynchronously
 *
 * @param value     If status is ER_OK a MsgArg containing the returned property value
 * @param object    Remote bus object that was introspected
 * @param status    - ER_OK if the property get request was successfull or:
 *                  - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
 *                  - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 *                  - Other error status codes indicating the reason the get request failed.
 * @param context   Caller provided context passed in to GetPropertyAsync()
 */
- (void)didReceiveValueForProperty:(AJNMessageArgument *)value ofObject:(AJNProxyBusObject *)object completionStatus:(QStatus)status context:(AJNHandle)context;

/**
 * Handler for receiving all the values of all properties on an object asynchronously
 *
 * @param values        If status is ER_OK an array of dictionary entries, signature "a{sv}" listing the properties.
 * @param object        Remote bus object that was introspected
 * @param status      - ER_OK if the get all properties request was successfull or:
 *                    - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
 *                    - Other error status codes indicating the reason the get request failed.
 * @param context       Caller provided context passed in to GetPropertyAsync()
 */
- (void)didReceiveValuesForAllProperties:(AJNMessageArgument *)values ofObject:(AJNProxyBusObject *)object completionStatus:(QStatus)status context:(AJNHandle)context;

/**
 * Callback registered with SetPropertyAsync()
 *
 * @param object    Remote bus object that was introspected
 * @param status    - ER_OK if the property was successfully set or:
 *                  - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
 *                  - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 *                  - Other error status codes indicating the reason the set request failed.
 * @param context   Caller provided context passed in to SetPropertyAsync()
 */
- (void)didComleteSetPropertyOnObject:(AJNProxyBusObject *)object completionStatus:(QStatus)status context:(AJNHandle)context;


@end

////////////////////////////////////////////////////////////////////////////////

/**
 * Each ProxyBusObject instance represents a single DBus/AllJoyn object registered
 * somewhere on the bus. ProxyBusObjects are used to make method calls on these
 * remotely located DBus objects.
 */
@interface AJNProxyBusObject : AJNObject

/**
 * Return the absolute object path for the remote object.
 *
 * @return Object path
 */
@property (nonatomic, readonly) NSString *path;

/**
 * Return the remote service name for this object.
 *
 * @return Service name (typically a well-known service name but may be a unique name)
 */
@property (nonatomic, readonly) NSString *serviceName;

/**
 * Return the session Id for this object.
 *
 * @return Session Id
 */
@property (nonatomic, readonly) AJNSessionId sessionId;

/**
 * Returns the interfaces implemented by this object. Note that all proxy bus objects
 * automatically inherit the "org.freedesktop.DBus.Peer" which provides the built-in "ping"
 * method, so this method always returns at least that one interface.
 *
 * @return  The interfaces implemented by the object.
 */
@property (nonatomic, readonly) NSArray *interfaces;

/**
 * Returns an array of ProxyBusObjects for the children of this ProxyBusObject.
 *
 * @return  The children of the object, or nil if there are none.
 */
@property (nonatomic, readonly) NSArray *children;

/**
 * Indicates if this is a valid (usable) proxy bus object.
 *
 * @return true if a valid proxy bus object, false otherwise.
 */
@property (nonatomic, readonly) BOOL isValid;

/**
 * Indicates if the remote object for this proxy bus object is secure.
 *
 * @return  true if the object is secure
 */
@property (nonatomic, readonly) BOOL isSecure;

/**
 * Create an empty proxy object that refers to an object at given remote service name. Note
 * that the created proxy object does not contain information about the interfaces that the
 * actual remote object implements with the exception that org.freedesktop.DBus.Peer
 * interface is special-cased (per the DBus spec) and can always be called on any object. Nor
 * does it contain information about the child objects that the actual remote object might
 * contain.
 *
 * To fill in this object with the interfaces and child object names that the actual remote
 * object describes in its introspection data, call IntrospectRemoteObject() or
 * IntrospectRemoteObjectAsync().
 *
 * @param busAttachment  The bus.
 * @param serviceName    The remote service name (well-known or unique).
 * @param path           The absolute (non-relative) object path for the remote object.
 * @param sessionId      The session id the be used for communicating with remote object.
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment serviceName:(NSString *)serviceName objectPath:(NSString *)path sessionId:(AJNSessionId)sessionId;

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
 * @param bus        The bus.
 * @param service    The remote service name (well-known or unique).
 * @param path       The absolute (non-relative) object path for the remote object.
 * @param sessionId  The session id the be used for communicating with remote object.
 * @param shouldEnableSecurity     The security mode for the remote object.
 */
- (id)initWithBusAttachment:(AJNBusAttachment *)busAttachment serviceName:(NSString *)serviceName objectPath:(NSString *)path sessionId:(AJNSessionId)sessionId enableSecurity:(BOOL)shouldEnableSecurity;

/**
 * Add an existing interface to this object using the interface's name.
 *
 * @param interfaceName   Name of existing interface to add to this object.
 * @return  - ER_OK if successful.
 *          - An error status otherwise.
 */
- (QStatus)addInterfaceNamed:(NSString *)interfaceName;

/**
 * Add an interface to this ProxyBusObject.
 *
 * Occasionally, AllJoyn library user may wish to call a method on
 * a ProxyBusObject that was not reported during introspection of the remote object.
 * When this happens, the InterfaceDescription will have to be registered with the
 * Bus manually and the interface will have to be added to the ProxyBusObject using this method.
 * @remark
 * The interface added via this call must have been previously registered with the
 * Bus. (i.e. it must have come from a call to Bus::GetInterface()).
 *
 * @param interfaceDescription    The interface to add to this object. Must come from Bus::GetInterface().
 * @return  - ER_OK if successful.
 *          - An error status otherwise
 */
- (QStatus)addInterfaceFromDescription:(AJNInterfaceDescription *)interfaceDescription;

/**
 * Returns a pointer to an interface description. Returns nil if the object does not implement
 * the requested interface.
 *
 * @param name  The name of interface to get.
 *
 * @return  - A pointer to the requested interface description.
 *          - nil if requested interface is not implemented or not found
 */
- (AJNInterfaceDescription *)interfaceWithName:(NSString *)name;

/**
 * Tests if this object implements the requested interface.
 *
 * @param name  The interface to check
 *
 * @return  TRUE if the object implements the requested interface
 */
- (BOOL)implementsInterfaceWithName:(NSString *)name;

/**
 * Get a path descendant ProxyBusObject (child) by its relative path name.
 *
 * For example, if this ProxyBusObject's path is "/foo/bar", then you can
 * retrieve the ProxyBusObject for "/foo/bar/bat/baz" by calling
 * GetChild("bat/baz")
 *
 * @param path the relative path for the child.
 *
 * @return  - The (potentially deep) descendant ProxyBusObject
 *          - nil if not found.
 */
- (AJNProxyBusObject *)childAtPath:(NSString *)path;

/**
 * Add a child object (direct or deep object path descendant) to this object.
 * If you add a deep path descendant, this method will create intermediate
 * ProxyBusObject children as needed.
 *
 * @remark  - It is an error to try to add a child that already exists.
 *          - It is an error to try to add a child that has an object path that is not a descendant of this object's path.
 *
 * @param child  Child ProxyBusObject
 * @return
 *      - #ER_OK if successful.
 *      - #ER_BUS_BAD_CHILD_PATH if the path is a bad path
 *      - #ER_BUS_OBJ_ALREADY_EXISTS the the object already exists on the ProxyBusObject
 */
- (QStatus)addChild:(AJNProxyBusObject *)child;

/**
 * Remove a child object and any descendants it may have.
 *
 * @param path   Absolute or relative (to this ProxyBusObject) object path.
 * @return  - ER_OK if successful.
 *          - ER_BUS_BAD_CHILD_PATH if the path given was not a valid path
 *          - ER_BUS_OBJ_NOT_FOUND if the Child object was not found
 *          - ER_FAIL any other unexpected error.
 */
- (QStatus)removeChildAtPath:(NSString *)path;

/**
 * Make a synchronous method call from this object
 *
 * @param method        Method being invoked.
 * @param arguments     The arguments for the method call (can be NULL)
 * @param reply         The reply message received for the method call
 *
 * @return  - ER_OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
 *          - ER_BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
 */
- (QStatus)callMethod:(AJNInterfaceMember *)method withArguments:(NSArray *)arguments methodReply:(AJNMessage **)reply;

/**
 * Make a synchronous method call from this object
 *
 * @param method       Method being invoked.
 * @param arguments    The arguments for the method call (can be NULL)
 * @param reply        The reply message received for the method call
 * @param timeout      Timeout specified in milliseconds to wait for a reply
 * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
 *
 *
 * @return  - ER_OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
 *          - ER_BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
 */
- (QStatus)callMethod:(AJNInterfaceMember *)method withArguments:(NSArray *)arguments methodReply:(AJNMessage **)reply timeout:(uint32_t)timeout flags:(uint8_t)flags;

/**
 * Make an asynchronous method call from this object
 *
 * @param method        Method being invoked.
 * @param arguments     The arguments for the method call (can be NULL)
 * @param replyDelegate The object to be called when the asych method call completes.
 * @param context       User-defined context that will be returned to the reply delegate.
 * @param timeout       Timeout specified in milliseconds to wait for a reply
 * @param flags         Logical OR of the message flags for this method call. The following flags apply to method calls:
 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
 * @return  - ER_OK if successful
 *          - An error status otherwise
 */
- (QStatus)callMethod:(AJNInterfaceMember *)method withArguments:(NSArray *)arguments methodReplyDelegate:(id<AJNProxyBusObjectDelegate>) replyDelegate context:(AJNHandle)context timeout:(uint32_t)timeout flags:(uint8_t)flags;

/**
 * Make a synchronous method call from this object
 *
 * @param methodName    Name of method.
 * @param interfaceName Name of interface.
 * @param arguments     The arguments for the method call (can be NULL)
 * @param reply         The reply message received for the method call
 *
 * @return  - ER_OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
 *          - ER_BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
 */
- (QStatus)callMethodWithName:(NSString *)methodName onInterfaceWithName:(NSString *)interfaceName withArguments:(NSArray *)arguments methodReply:(AJNMessage **)reply;

/**
 * Make a synchronous method call from this object
 *
 * @param methodName    Name of method.
 * @param interfaceName Name of interface.
 * @param arguments     The arguments for the method call (can be NULL)
 * @param reply         The reply message received for the method call
 * @param timeout       Timeout specified in milliseconds to wait for a reply
 * @param flags         Logical OR of the message flags for this method call. The following flags apply to method calls:
 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
 *
 * @return  - ER_OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
 *          - ER_BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
 */
- (QStatus)callMethodWithName:(NSString *)methodName onInterfaceWithName:(NSString *)interfaceName withArguments:(NSArray *)arguments methodReply:(AJNMessage **)reply timeout:(uint32_t)timeout flags:(uint8_t)flags;

/**
 * Make an asynchronous method call from this object
 *
 * @param methodName    Name of method.
 * @param interfaceName Name of interface.
 * @param arguments     The arguments for the method call (can be NULL)
 * @param replyDelegate The object to be called when the asych method call completes.
 * @param context       User-defined context that will be returned to the reply delegate.
 * @param timeout       Timeout specified in milliseconds to wait for a reply
 * @param flags         Logical OR of the message flags for this method call. The following flags apply to method calls:
 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
 * @return  - ER_OK if successful
 *          - An error status otherwise
 */
- (QStatus)callMethodWithName:(NSString *)methodName onInterfaceWithName:(NSString *)interfaceName withArguments:(NSArray *)arguments methodReplyDelegate:(id<AJNProxyBusObjectDelegate>)replyDelegate context:(AJNHandle)context timeout:(uint32_t)timeout flags:(uint8_t)flags;

/**
 * Query the remote object on the bus to determine the interfaces and
 * children that exist. Use this information to populate this proxy's
 * interfaces and children.
 *
 * This call causes messages to be send on the bus, therefore it cannot
 * be called within AllJoyn callbacks (method/signal/reply handlers or
 * ObjectRegistered callbacks, etc.)
 *
 * @return  - ER_OK if successful
 *          - An error status otherwise
 */
- (QStatus)introspectRemoteObject;

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
 * @param completionHandler Pointer to the delegate object that will receive the callback.
 * @param context           User defined context which will be passed as-is to callback.
 * @return  - ER_OK if successful.
 *          - An error status otherwise
 */
- (QStatus)introspectRemoteObject:(id<AJNProxyBusObjectDelegate>)completionHandler context:(AJNHandle)context;

/**
 * Initialize this proxy object from an XML string. Calling this method does several things:
 *
 *  - Create and register any new InterfaceDescription(s) that are mentioned in the XML.
 *     (Interfaces that are already registered with the bus are left "as-is".)
 *  - Add all the interfaces mentioned in the introspection data to this ProxyBusObject.
 *  - Recursively create any child ProxyBusObject(s) and create/add their associated @n
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
 * @param xmlProxyObjectDescription         An XML string in DBus introspection format.
 * @param identifier  An optional identifying string to include in error logging messages.
 *
 * @return  - ER_OK if parsing is completely successful.
 *          - An error status otherwise.
 */
- (QStatus)buildFromXml:(NSString *)xmlProxyObjectDescription errorLogId:(NSString *)identifier;

/**
 * Get a property from an interface on the remote object.
 *
 * @param propertyName    The name of the property to get.
 * @param interfaceName   Name of interface to retrieve property from.
 *
 * @return The property's value wrapped in an AJNMessageArgument object if successful. Otherwise, the return value is nil.
 */
- (AJNMessageArgument *)propertyWithName:(NSString *)propertyName forInterfaceWithName:(NSString *)interfaceName;

/**
 * Make an asynchronous request to get a property from an interface on the remote object.
 * The property value is passed to the callback function.
 *
 * @param propertyName      The name of the property to get.
 * @param interfaceName     Name of interface to retrieve property from.
 * @param delegate          Reference to the object that will receive the completion callback.
 * @param context           User defined context which will be passed as-is to callback.
 * @param timeout           Timeout specified in milliseconds to wait for a reply
 * @return
 *      - ER_OK if the request to get the property was successfully issued .
 *      - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
 *      - An error status otherwise
 */
- (QStatus)propertyWithName:(NSString *)propertyName forInterfaceWithName:(NSString *)interfaceName completionDelegate:(id<AJNProxyBusObjectDelegate>)delegate context:(AJNHandle)context timeout:(uint32_t)timeout;

/**
 * Get all properties from an interface on the remote object.
 *
 * @param values            Property values returned as an array of dictionary entries, signature "a{sv}".
 * @param interfaceName     Name of interface to retrieve all properties from.
 *
 * @return  - ER_OK if the property was obtained.
 *          - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
 *          - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 */
- (QStatus)propertyValues:(AJNMessageArgument **)values ofInterfaceWithName:(NSString *)interfaceName;

/**
 * Make an asynchronous request to get all properties from an interface on the remote object.
 *
 * @param interfaceName     Name of interface to retrieve property from.
 * @param delegate          Reference to the object that will receive the completion callback.
 * @param context           User defined context which will be passed as-is to callback.
 * @param timeout           Timeout specified in milliseconds to wait for a reply
 * @return  - ER_OK if the request to get all properties was successfully issued.
 *          - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
 *          - An error status otherwise
 */
- (QStatus)propertyValuesForInterfaceWithName:(NSString *)interfaceName completionDelegate:(id<AJNProxyBusObjectDelegate>)delegate context:(AJNHandle)context timeout:(uint32_t)timeout;

/**
 * Set a property on an interface on the remote object.
 *
 * @param propertyName  The name of the property to set
 * @param interfaceName     Interface that holds the property
 * @param value     The value to set
 *
 * @return  - ER_OK if the property was set
 *          - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
 *          - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 */
- (QStatus)setPropertyWithName:(NSString *)propertyName forInterfaceWithName:(NSString *)interfaceName toValue:(AJNMessageArgument *)value;

/**
 * Make an asynchronous request to set a property on an interface on the remote object.
 * A callback function reports the success or failure of ther operation.
 *
 * @param interfaceName Remote object's interface on which the property is defined.
 * @param propertyName  The name of the property to set.
 * @param value         The value to set
 * @param delegate      Pointer to the object that will receive the callback.
 * @param context       User defined context which will be passed as-is to callback.
 * @param timeout       Timeout specified in milliseconds to wait for a reply
 * @return  - ER_OK if the request to set the property was successfully issued.
 *          - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the specified interfaces does not exist on the remote object.
 *          - An error status otherwise
 */
- (QStatus)setPropertyWithName:(NSString *)propertyName forInterfaceWithName:(NSString *)interfaceName toValue:(AJNMessageArgument *)value completionDelegate:(id<AJNProxyBusObjectDelegate>)delegate context:(AJNHandle)context timeout:(uint32_t)timeout;

/**
 * Set a uint32 property.
 *
 * @param propertyName  The name of the property to set
 * @param interfaceName     Interface that holds the property
 * @param value         The uint32 value to set
 *
 * @return  - ER_OK if the property was set
 *          - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
 *          - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 */
- (QStatus)setPropertyWithName:(NSString *)propertyName forInterfaceWithName:(NSString *)interfaceName toIntValue:(NSInteger)value;

/**
 * Set a string property.
 *
 * @param propertyName  The name of the property to set
 * @param interfaceName     Interface that holds the property
 * @param value         The string value to set
 *
 * @return  - ER_OK if the property was set
 *          - ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
 *          - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 */
- (QStatus)setPropertyWithName:(NSString *)propertyName forInterfaceWithName:(NSString *)interfaceName toStringValue:(NSString *)value;

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
 * @param forceAuthentication  If true, forces an re-authentication even if the peer connection is already
 *                   authenticated.
 *
 * @return  - ER_OK if the connection was secured or an error status indicating that the
 *            connection could not be secured.
 *          - ER_BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment::EnablePeerSecurity() has not been called.
 *          - ER_AUTH_FAIL if the attempt(s) to authenticate the peer failed.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)secureConnection:(BOOL)forceAuthentication;

/**
 * Asynchronously secure the connection to the remote peer for this proxy object. Peer-to-peer
 * connections can only be secured if EnablePeerSecurity() was previously called on the bus
 * attachment for this proxy object. If the peer-to-peer connection is already secure this
 * function does nothing. Note that peer-to-peer connections are automatically secured when a
 * method call requiring encryption is sent.
 *
 * Notification of success or failure is via the AuthListener passed to EnablePeerSecurity().
 *
 * @param forceAuthentication  If TRUE, forces an re-authentication even if the peer connection is already
 *                   authenticated.
 *
 * @return  - ER_OK if securing could begin.
 *          - ER_BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment::EnablePeerSecurity() has not been called.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)secureConnectionAsync:(BOOL)forceAuthentication;

@end
