////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#import "AJNAuthenticationListener.h"
#import "AJNBus.h"
#import "AJNBusListener.h"
#import "AJNKeyStoreListener.h"
#import "AJNObject.h"
#import "AJNSessionListener.h"
#import "AJNSessionOptions.h"
#import "AJNSessionPortListener.h"
#import "AJNSignalHandler.h"
#import "AJNStatus.h"
#import "AJNInterfaceDescription.h"
#import "AJNTranslator.h"
#import "AJNAboutListener.h"

@protocol AJNBusObject;

@class AJNBusObject;
@class AJNProxyBusObject;
@class AJNInterfaceDescription;
@class AJNInterfaceMember;

/**
 * Block definition for joining a session asynchronously
 */
typedef void (^ AJNJoinSessionBlock)(QStatus status, AJNSessionId sessionId, AJNSessionOptions *opts, void *context);

////////////////////////////////////////////////////////////////////////////////

/**
 * Delegate used to receive notifications when joining a session asynchronously
 */
@protocol AJNSessionDelegate <NSObject>

/** Called when joinSessionAsync completes.
 * @param sessionId         The identifier of the session that was joined.
 * @param status            A status code indicating success or failure of the join operation.
 * @param sessionOptions    Session options for the newly joined session.
 * @param context           User defined context which will be passed as-is to callback.
 */
- (void)didJoinSession:(AJNSessionId) sessionId status:(QStatus)status sessionOptions:(AJNSessionOptions *)sessionOptions context:(AJNHandle)context;

@end

////////////////////////////////////////////////////////////////////////////////

/**
 * Block definition for setting a link timeout asynchronously
 */
typedef void (^ AJNLinkTimeoutBlock)(QStatus status, uint32_t timeout, void *context);

////////////////////////////////////////////////////////////////////////////////

/**
 * Delegate used to receive notifications when setting a link timeout asynchronously
 */
@protocol AJNLinkTimeoutDelegate <NSObject>

/**
 * Called when setLinkTimeoutAsync completes.
 *
 * @param timeout      Timeout value (possibly adjusted from original request).
 * @param status       ER_OK if successful
 * @param context      User defined context which will be passed as-is to callback.
 */
- (void)didSetLinkTimeoutTo:(uint32_t)timeout status:(QStatus)status context:(void *)context;

@end

/**
 * Block definition for pinging a peer asynchronously
 */
typedef void (^ AJNPingPeerBlock)(QStatus status, void *context);

/**
 * Delegate used to receive notifications when we ping a peer asynchronously
 */
@protocol AJNPingPeerDelegate <NSObject>

/**
 * Called when pingAsync completes.
 *
 * @param status
 *   - #ER_OK on success
 *   - #ER_ALLJOYN_PING_FAILED Ping failed
 *   - #ER_ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out
 *   - #ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session
 *   - #ER_ALLJOYN_PING_REPLY_UNIMPLEMENTED the remote routing node does not implement Ping
 *   - #ER_ALLJOYN_PING_REPLY_UNREACHABLE the name pinged is unreachable
 *   - #ER_BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error
 * @param context      User defined context which will be passed as-is to callback.
 */
- (void)pingPeerHasStatus:(QStatus)status context:(void *)context;

@end

////////////////////////////////////////////////////////////////////////////////

/**
 * The top-level object responsible for connecting to and optionally managing a message bus.
 */
@interface AJNBusAttachment : AJNObject

/**
 * Indicate whether bus is currently connected.
 *
 * Messages can only be sent or received when the bus is connected.
 *
 * @return true if the bus is connected.
 */
@property (nonatomic, readonly) BOOL isConnected;

/**
 * Determine if the bus attachment has been started.
 *
 * @return true if the message bus has been started.
 */
@property (nonatomic, readonly) BOOL isStarted;

/**
 * Determine if the bus attachment has been stopped.
 *
 * @return true if the message bus has been stopped.
 */
@property (nonatomic, readonly) BOOL isStopping;

/**
 * Get the unique name of this BusAttachment. Returns an empty string if the bus attachment
 * is not connected.
 *
 * @return The unique name of this BusAttachment.
 */
@property (nonatomic, readonly) NSString *uniqueName;

/**
 * Get the GUID of this BusAttachment.
 *
 * The returned value may be appended to an advertised well-known name in order to guarantee
 * that the resulting name is globally unique.
 *
 * @return GUID of this BusAttachment as a string.
 */
@property (nonatomic, readonly) NSString *uniqueIdentifier;

/**
 * Check if peer security has been enabled for this bus attachment.
 *
 * @return   Returns true if peer security has been enabled, false otherwise.
 */
@property (nonatomic, readonly) BOOL isPeerSecurityEnabled;

/**
 * Get the concurrent method and signal handler limit.
 *
 * @return The maximum number of concurrent method and signal handlers.
 */
@property (nonatomic, readonly) NSUInteger concurrency;

/**
 * Get the org.freedesktop.DBus proxy object.
 *
 * @return org.freedesktop.DBus proxy object
 */
@property (nonatomic, readonly) AJNProxyBusObject *dbusProxyObject;

/**
 * Get the org.alljoyn.Bus proxy object.
 *
 * @return org.alljoyn.Bus proxy object
 */
@property (nonatomic, readonly) AJNProxyBusObject *allJoynProxyObject;

/**
 * Get the org.alljoyn.Debug proxy object.
 *
 * @return org.alljoyn.Debug proxy object
 */
@property (nonatomic, readonly) AJNProxyBusObject *allJoynDebugProxyObject;

/**
 * Returns the existing activated interface descriptions.
 */
@property (nonatomic, readonly) NSArray *interfaces;

/**
 * Construct a BusAttachment.
 *
 * @param applicationName       Name of the application.
 * @param allowRemoteMessages   True if this attachment is allowed to receive messages from remote devices.
 */
- (id)initWithApplicationName:(NSString *)applicationName allowRemoteMessages:(BOOL)allowRemoteMessages;

/**
 * Construct a BusAttachment.
 *
 * @param applicationName       Name of the application.
 * @param allowRemoteMessages   True if this attachment is allowed to receive messages from remote devices.
 * @param maximumConcurrentOperations           The maximum number of concurrent method and signal handlers locally executing.
 */
- (id)initWithApplicationName:(NSString *)applicationName allowRemoteMessages:(BOOL)allowRemoteMessages maximumConcurrentOperations:(NSUInteger)maximumConcurrentOperations;

/**
 * Explicitly destroys the underlying AllJoyn C++ API BusAttachment object
 */
- (void)destroy;

/**
 * Create an interface description with a given name.
 *
 * Typically, interfaces that are implemented by BusObjects are created here.
 * Interfaces that are implemented by remote objects are added automatically by
 * the bus if they are not already present via AJNProxyBusObject::introspectRemoteObject.
 *
 * Because interfaces are added both explicitly (via this method) and implicitly
 * (via AJNProxyBusObject::introspectRemoteObject), there is the possibility that creating
 * an interface here will fail because the interface already exists. If this happens, the
 * ER_BUS_IFACE_ALREADY_EXISTS will be returned and NULL will be returned in the iface [OUT]
 * parameter.
 *
 * Interfaces created with this method need to be activated using AJNInterfaceDescription::activate
 * once all of the methods, signals, etc have been added to the interface. The interface will
 * be unaccessible (via AJNBusAttachment::interfaceWithName) until
 * it is activated.
 *
 * @param interfaceName             The requested interface name.
 *
 * @return  - Interface description
 *          - nil if cannot be created.
 */
- (AJNInterfaceDescription *)createInterfaceWithName:(NSString *)interfaceName;

/**
 * Create an interface description with a given name.
 *
 * Typically, interfaces that are implemented by BusObjects are created here.
 * Interfaces that are implemented by remote objects are added automatically by
 * the bus if they are not already present via AJNProxyBusObject::introspectRemoteObject.
 *
 * Because interfaces are added both explicitly (via this method) and implicitly
 * (via AJNProxyBusObject::introspectRemoteObject), there is the possibility that creating
 * an interface here will fail because the interface already exists. If this happens, the
 * ER_BUS_IFACE_ALREADY_EXISTS will be returned and NULL will be returned in the iface [OUT]
 * parameter.
 *
 * Interfaces created with this method need to be activated using AJNInterfaceDescription::activate
 * once all of the methods, signals, etc have been added to the interface. The interface will
 * be unaccessible (via AJNBusAttachment::interfaceWithName) until
 * it is activated.
 *
 * @param interfaceName             The requested interface name.
 * @param shouldEnableSecurity      If true the interface is secure and method calls and signals will be encrypted.
 *
 * @return  - Interface description
 *          - nil if cannot be created.
 */
- (AJNInterfaceDescription *)createInterfaceWithName:(NSString *)interfaceName enableSecurity:(BOOL)shouldEnableSecurity;

/**
 * Create an interface description with a given name.
 *
 * Typically, interfaces that are implemented by BusObjects are created here.
 * Interfaces that are implemented by remote objects are added automatically by
 * the bus if they are not already present via ProxyBusObject::IntrospectRemoteObject().
 *
 * Because interfaces are added both explicitly (via this method) and implicitly
 * (via ProxyBusObject::IntrospectRemoteObject), there is the possibility that creating
 * an interface here will fail because the interface already exists. If this happens, the
 * ER_BUS_IFACE_ALREADY_EXISTS will be returned and NULL will be returned in the iface [OUT]
 * parameter.
 *
 * Interfaces created with this method need to be activated using InterfaceDescription::Activate()
 * once all of the methods, signals, etc have been added to the interface. The interface will
 * be unaccessible (via BusAttachment::GetInterfaces() or BusAttachment::GetInterface()) until
 * it is activated.
 *
 * @param name   The requested interface name.
 * @param[out] iface
 *      - Interface description
 *      - NULL if cannot be created.
 * @param secPolicy The security policy for this interface
 *
 * @return
 *      - #ER_OK if creation was successful.
 *      - #ER_BUS_IFACE_ALREADY_EXISTS if requested interface already exists
 */

- (AJNInterfaceDescription *)createInterfaceWithName:(NSString *)interfaceName withInterfaceSecPolicy:(enum AJNInterfaceSecurityPolicy)interfaceSecurityPolicy;

/**
 * Retrieve an existing activated InterfaceDescription.
 *
 * @param interfaceName       Interface name
 *
 * @return  - A pointer to the registered interface
 *          - nil if interface doesn't exist
 */
- (AJNInterfaceDescription *)interfaceWithName:(NSString *)interfaceName;

/**
 * Delete an interface description with a given name.
 *
 * Deleting an interface is only allowed if that interface has never been activated.
 *
 * @param interfaceName  The name of the un-activated interface to be deleted.
 *
 * @return  - ER_OK if deletion was successful
 *          - ER_BUS_NO_SUCH_INTERFACE if interface was not found
 */
- (QStatus)deleteInterfaceWithName:(NSString *)interfaceName;

/**
 * Delete an interface description with a given name.
 *
 * Deleting an interface is only allowed if that interface has never been activated.
 *
 * @param interfaceDescription  The un-activated interface to be deleted.
 *
 * @return  - ER_OK if deletion was successful
 *          - ER_BUS_NO_SUCH_INTERFACE if interface was not found
 */
- (QStatus)deleteInterface:(AJNInterfaceDescription *)interfaceDescription;

/**
 * Initialize one more interface descriptions from an XML string in DBus introspection format.
 * The root tag of the XML can be a \<node\> or a stand alone \<interface\> tag. To initialize more
 * than one interface the interfaces need to be nested in a \<node\> tag.
 *
 * Note that when this method fails during parsing, the return code will be set accordingly.
 * However, any interfaces which were successfully parsed prior to the failure may be registered
 * with the bus.
 *
 * @param xmlString     An XML string in DBus introspection format.
 *
 * @return  - ER_OK if parsing is completely successful.
 *          - An error status otherwise.
 */
- (QStatus)createInterfacesFromXml:(NSString *)xmlString;

/**
 * Register an object that will receive bus event notifications.
 *
 * @param listener  Object instance that will receive bus event notifications.
 */
- (void)registerBusListener:(id<AJNBusListener>)listener;

/**
 * Unregister an object that was previously registered with RegisterBusListener.
 *
 * @param listener  Object instance to un-register as a listener.
 */
- (void)unregisterBusListener:(id<AJNBusListener>)listener;

/**
 * Destroy the bus listener.
 *
 * @param listener  Object instance to un-register as a listener.
 */
- (void)destroyBusListener:(id<AJNBusListener>)listener;

/**
 * Register a signal handler.
 *
 * @param handler       The delegate receiving the signal.
 */
- (void)registerSignalHandler:(id<AJNSignalHandler>)handler;

/**
 * Unregister a signal handler.
 *
 * @param handler       The delegate receiving the signal.
 */
- (void)unregisterSignalHandler:(id<AJNSignalHandler>)handler;

/**
 * Unregister all signal and reply handlers for the specified message receiver. This function is
 * intended to be called from within the destructor of a MessageReceiver class instance. It
 * prevents any pending signals or replies from accessing the MessageReceiver after the message
 * receiver has been freed.
 *
 * @param receiver       The message receiver, such as a signal handler or bus listener, that is being unregistered.
 * @return ER_OK if successful.
 */
- (void)unregisterAllHandlersForReceiver:(id<AJNHandle>)receiver;

/**
 * Register a BusObject
 *
 * @param busObject      BusObject to register.
 *
 * @return  - ER_OK if successful.
 *          - ER_BUS_BAD_OBJ_PATH for a bad object path
 */
- (QStatus)registerBusObject:(AJNBusObject *)busObject;


/**
 * Register a BusObject
 *
 * @param obj      BusObject to register.
 * @param shouldEnableSecurity   true if authentication is required to access this object.
 *
 * @return
 *      - #ER_OK if successful.
 *      - #ER_BUS_BAD_OBJ_PATH for a bad object path
 */
- (QStatus) registerBusObject:(AJNBusObject *) busObject enableSecurity:(BOOL)shouldEnableSecurity;

/**
 * Unregister a BusObject
 *
 * @param busObject  Object to be unregistered.
 */
- (void)unregisterBusObject:(AJNBusObject *)busObject;

/**
 * Start the process of spinning up the independent threads used in
 * the bus attachment, preparing it for action.
 *
 * This method only begins the process of starting the bus. Sending and
 * receiving messages cannot begin until the bus is Connected.
 *
 * In most cases, it is not required to understand the threading model of
 * the bus attachment, with one important exception: The bus attachment may
 * send callbacks to registered listeners using its own internal threads.
 * This means that any time a listener of any kind is used in a program, the
 * implication is that a the overall program is multithreaded, irrespective
 * of whether or not threads are explicitly used.  This, in turn, means that
 * any time shared state is accessed in listener methods, that state must be
 * protected.
 *
 * As soon as start is called, clients of a bus attachment with listeners
 * must be prepared to receive callbacks on those listeners in the context
 * of a thread that will be different from the thread running the main
 * program or any other thread in the client.
 *
 * Although intimate knowledge of the details of the threading model are not
 * required to use a bus attachment (beyond the caveat above) we do provide
 * methods on the bus attachment that help users reason about more complex
 * threading situations.  This will apply to situations where clients of the
 * bus attachment are multithreaded and need to interact with the
 * multithreaded bus attachment.  These methods can be especially useful
 * during shutdown, when the two separate threading systems need to be
 * gracefully brought down together.
 *
 * The BusAttachment methods start, stop and waitUntilStopCompleted all work together to
 * manage the autonomous activities that can happen in a BusAttachment.
 * These activities are carried out by so-called hardware threads.  POSIX
 * defines functions used to control hardware threads, which it calls
 * pthreads.  Many threading packages use similar constructs.
 *
 * In a threading package, a start method asks the underlying system to
 * arrange for the start of thread execution.  Threads are not necessarily
 * running when the start method returns, but they are being *started*.  Some time later,
 * a thread of execution appears in a thread run function, at which point the
 * thread is considered *running*.  At some later time, executing a stop method asks the
 * underlying system to arrange for a thread to end its execution.  The system
 * typically sends a message to the thread to ask it to stop doing what it is doing.
 * The thread is running until it responds to the stop message, at which time the
 * run method exits and the thread is considered *stopping*.
 *
 * Note that neither of start nor stop are synchronous in the sense that
 * one has actually accomplished the desired effect upon the return from a
 * call.  Of particular interest is the fact that after a call to stop,
 * threads will still be *running* for some non-deterministic time.
 *
 * In order to wait until all of the threads have actually stopped, a
 * blocking call is required.  In threading packages this is typically
 * called join, and our corresponding method is called waitUntilStopCompleted.
 *
 * A start method call should be thought of as mapping to a threading
 * package start function.  it causes the activity threads in the
 * BusAttachment to be spun up and gets the attachment ready to do its main
 * job.  As soon as start is called, the user should be prepared for one
 * or more of these threads of execution to pop out of the bus attachment
 * and into a listener callback.
 *
 * The stop method call should be thought of as mapping to a threading
 * package stop function.  It asks the BusAttachment to begin shutting down
 * its various threads of execution, but does not wait for any threads to exit.
 *
 * A call to the waitUntilStopCompleted method should be thought of as mapping to a
 * threading package join function call.  It blocks and waits until all of
 * the threads in the BusAttachment have in fact exited their Run functions,
 * gone through the stopping state and have returned their status.  When
 * the waitUntilStopCompleted method returns, one may be assured that no threads are running
 * in the bus attachment, and therefore there will be no callbacks in
 * progress and no further callbacks will ever come out of a particular
 * instance of a bus attachment.
 *
 * It is important to understand that since start, stop and waitUntilStopCompleted map
 * to threads concepts and functions, one should not expect them to clean up
 * any bus attachment state when they are called.  These functions are only
 * present to help in orderly termination of complex threading systems.
 *
 * @return  - ER_OK if successful.
 *          - ER_BUS_BUS_ALREADY_STARTED if already started
 *          - Other error status codes indicating a failure
 */
- (QStatus)start;

/**
 * Ask the threading subsystem in the bus attachment to begin the
 * process of ending the execution of its threads.
 *
 * The stop method call on a bus attachment should be thought of as
 * mapping to a threading package stop function.  It asks the BusAttachment
 * to begin shutting down its various threads of execution, but does not
 * wait for any threads to exit.
 *
 * A call to stop is implied as one of the first steps in the destruction
 * of a bus attachment.
 *
 * @warning There is no guarantee that a listener callback may begin executing
 * after a call to stop.  To achieve that effect, the stop must be followed
 * by a waitUntilStopCompleted.
 *
 * @see start
 * @see waitUntilStopCompleted
 *
 * @return  - ER_OK if successful.
 *          - An error QStatus if unable to begin the process of stopping the message bus threads.
 */
- (QStatus)stop;

/**
 * Wait for all of the threads spawned by the bus attachment to be
 * completely exited.
 *
 * A call to the waitUntilStopCompleted method should be thought of as mapping to a
 * threading package join function call.  It blocks and waits until all of
 * the threads in the BusAttachment have, in fact, exited their Run functions,
 * gone through the stopping state and have returned their status.  When
 * the waitUntilStopCompleted method returns, one may be assured that no threads are running
 * in the bus attachment, and therefore there will be no callbacks in
 * progress and no further callbacks will ever come out of the instance of a
 * bus attachment on which waitUntilStopCompleted was called.
 *
 * A call to waitUntilStopCompleted is implied as one of the first steps in the destruction
 * of a bus attachment.  Thus, when a bus attachment is destroyed, it is
 * guaranteed that before it completes its destruction process, there will be
 * no callbacks in process.
 *
 * @warning If waitUntilStopCompleted is called without a previous waitUntilStopCompleted it will result in
 * blocking "forever."
 *
 * @see start
 * @see stop
 *
 * @return  - ER_OK if successful.
 *          - ER_BUS_BUS_ALREADY_STARTED if already started
 *          - Other error status codes indicating a failure
 */
- (QStatus)waitUntilStopCompleted;

/**
 * Allow the currently executing method/signal handler to enable concurrent callbacks
 * during the scope of the handler's execution.
 *
 */
- (void)enableConcurrentCallbacks;

/**
 * Connect to a remote bus address.
 *
 * @param connectionArguments  A transport connection spec string of the form:
 *                     @c "<transport>:<param1>=<value1>,<param2>=<value2>...[;]"
 *
 * @return  - ER_OK if successful.
 *          - An error status otherwise
 */
- (QStatus)connectWithArguments:(NSString *)connectionArguments;

/**
 * Disconnect a remote bus address connection.
 *
 * @param connectionArguments  The transport connection spec used to connect.
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_BUS_NOT_STARTED if the bus is not started
 *          - ER_BUS_NOT_CONNECTED if the %BusAttachment is not connected to the bus
 *          - Other error status codes indicating a failure
 */
- (QStatus)disconnectWithArguments:(NSString *)connectionArguments;

/**
 * Request a well-known name.
 * This method is a shortcut/helper that issues an org.freedesktop.DBus.RequestName method call to the local daemon
 * and interprets the response.
 *
 * @param  name     Well-known name being requested.
 * @param  flags    Bitmask of bus name flags.
 *
 * @return  - ER_OK iff daemon response was received and request was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)requestWellKnownName:(NSString *)name withFlags:(AJNBusNameFlag)flags;

/**
 * Release a previously requested well-known name.
 * This method is a shortcut/helper that issues an org.freedesktop.DBus.ReleaseName method call to the local daemon
 * and interprets the response.
 *
 * @param  name          Well-known name being released.
 *
 * @return  - ER_OK iff daemon response was received amd the name was successfully released.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)releaseWellKnownName:(NSString *)name;

/**
 * Determine whether a given well-known name exists on the bus.
 * This method is a shortcut/helper that issues an org.freedesktop.DBus.NameHasOwner method call to the daemon
 * and interprets the response.
 *
 * @param  name       The well known name that the caller is inquiring about.
 * @return  - TRUE if name ownership was able to be determined.
 *          - FALSE otherwise.
 */
- (BOOL)doesWellKnownNameHaveOwner:(NSString *)name;

/**
 * Make a SessionPort available for external BusAttachments to join.
 *
 * Each BusAttachment binds its own set of SessionPorts. Session joiners use the bound session
 * port along with the name of the attachment to create a persistent logical connection (called
 * a Session) with the original BusAttachment.
 *
 * A SessionPort and bus name form a unique identifier that BusAttachments use when joining a
 * session.
 *
 * SessionPort values can be pre-arranged between AllJoyn services and their clients (well-known
 * SessionPorts).
 *
 * Once a session is joined using one of the service's well-known SessionPorts, the service may
 * bind additional SessionPorts (dynamically) and share these SessionPorts with the joiner over
 * the original session. The joiner can then create additional sessions with the service by
 * calling JoinSession with these dynamic SessionPort ids.
 *
 * @param port      Session port value to bind to allow this method
 *                  to choose an available port. On successful return, this value
 *                  contains the chosen SessionPort.
 *
 * @param options   Session options that joiners must agree to in order to
 *                  successfully join the session.
 *
 * @param delegate  Called by the bus when session related events occur.
 *
 * @return  - ER_OK iff daemon response was received and the bind operation was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)bindSessionOnPort:(AJNSessionPort)port withOptions:(AJNSessionOptions *)options withDelegate:(id<AJNSessionPortListener>)delegate;


/**
 * Make a SessionPort available for external BusAttachments to join.
 *
 * Each BusAttachment binds its own set of SessionPorts. Session joiners use the bound session
 * port along with the name of the attachment to create a persistent logical connection (called
 * a Session) with the original BusAttachment.
 *
 * A SessionPort and bus name form a unique identifier that BusAttachments use when joining a
 * session.
 *
 * SessionPort values can be pre-arranged between AllJoyn services and their clients (well-known
 * SessionPorts).
 *
 * Once a session is joined using one of the service's well-known SessionPorts, the service may
 * bind additional SessionPorts (dynamically) and share these SessionPorts with the joiner over
 * the original session. The joiner can then create additional sessions with the service by
 * calling JoinSession with these dynamic SessionPort ids.
 *
 * Session port value is not specified and thus the API will choose an available port.
 *
 * @param options   Session options that joiners must agree to in order to
 *                  successfully join the session.
 *
 * @param delegate  Called by the bus when session related events occur.
 *
 * @return  - A valid session port number iff daemon response was received and the bind operation was successful.
 *          - kAJNSessionPortAny is returned if there was any error.
 */
- (AJNSessionPort)bindSessionOnAnyPortWithOptions:(AJNSessionOptions *)options withDelegate:(id<AJNSessionPortListener>)delegate;

/**
 * Cancel an existing port binding.
 *
 * @param   port    Existing session port to be un-bound.
 *
 * @return  - ER_OK iff daemon response was received and the bind operation was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)unbindSessionFromPort:(AJNSessionPort)port;


/**
 * Join a session.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to the local daemon
 * and interprets the response.
 *
 * @param  sessionName      Bus name of attachment that is hosting the session to be joined.
 * @param  sessionPort      SessionPort of sessionHost to be joined.
 * @param  delegate         Optional listener called when session related events occur. May be NULL.
 * @param  options          Session options.
 *
 * @return  - The new session identifier.
 *          - A session id of 0 or -1 indicating a failure.
 */
- (AJNSessionId)joinSessionWithName:(NSString *)sessionName onPort:(AJNSessionPort)sessionPort withDelegate:(id<AJNSessionListener>)delegate options:(AJNSessionOptions *)options;

/**
 * Join a session asynchronously.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to the local daemon
 * and interprets the response.
 *
 * This call executes asynchronously. When the JoinSession response is received, the callback will be called.
 *
 * @param  sessionName      Bus name of attachment that is hosting the session to be joined.
 * @param  sessionPort      SessionPort of sessionHost to be joined.
 * @param  delegate         Optional listener called when session related events occur. May be NULL.
 * @param  options          Session options.
 * @param  completionDelegate Delegate to be called when the join completes.
 * @param  context          User defined context which will be passed as-is to callback.
 *
 * @return  - ER_OK iff method call to local daemon response was was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)joinSessionAsyncWithName:(NSString *)sessionName onPort:(AJNSessionPort)sessionPort withDelegate:(id<AJNSessionListener>)delegate options:(AJNSessionOptions *)options joinCompletedDelegate:(id<AJNSessionDelegate>)completionDelegate context:(AJNHandle)context;


/**
 * Join a session asynchronously.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to the local daemon
 * and interprets the response.
 *
 * This call executes asynchronously. When the JoinSession response is received, the callback will be called.
 *
 * @param  sessionName      Bus name of attachment that is hosting the session to be joined.
 * @param  sessionPort      SessionPort of sessionHost to be joined.
 * @param  delegate         Optional listener called when session related events occur. May be NULL.
 * @param  options          Session options.
 * @param  completionBlock  Block to be called when the join completes.
 * @param  context          User defined context which will be passed as-is to callback.
 *
 * @return  - ER_OK iff method call to local daemon response was was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)joinSessionAsyncWithName:(NSString *)sessionName onPort:(AJNSessionPort)sessionPort withDelegate:(id<AJNSessionListener>)delegate options:(AJNSessionOptions *)options joinCompletedBlock:(AJNJoinSessionBlock) completionBlock context:(AJNHandle)context;

/**
 * Set the SessionListener for an existing sessionId.
 * This method cannot be called on a self-joined session.
 *
 * Calling this method will override the listener set by a previous call to SetSessionListener,
 * SetHostedSessionListener, SetJoinedSessionListener or any listener specified in JoinSession.
 *
 * @param delegate     The SessionListener to associate with the session. May be nil to clear previous listener.
 * @param sessionId    The session id of an existing session.
 * @return  - ER_OK if successful.
 *          - ER_BUS_NO_SESSION if session did not exist
 */
- (QStatus)bindSessionListener:(id<AJNSessionListener>)delegate toSession:(AJNSessionId)sessionId;

/**
 * Set the SessionListener for an existing sessionId on the joiner side.
 * Calling this method will override the listener set by a previous call to SetSessionListener or
 * SetJoinedSessionListener or any listener specified in JoinSession.
 *
 * @param delegate     The SessionListener to associate with the session. May be nil to clear previous listener.
 * @param sessionId    The session id of an existing session.
 * @return  - ER_OK if successful.
 *          - ER_BUS_NO_SESSION if session did not exist or if not host side of the session
 */
- (QStatus)bindJoinedSessionListener:(id<AJNSessionListener>)delegate toSession:(AJNSessionId)sessionId;

/**
 * Set the SessionListener for an existing sessionId on the host side.
 * Calling this method will override the listener set by a previous call to SetSessionListener or
 * SetHostedSessionListener.
 *
 * @param delegate     The SessionListener to associate with the session. May be nil to clear previous listener.
 * @param sessionId    The session id of an existing session.
 * @return  - ER_OK if successful.
 *          - ER_BUS_NO_SESSION if session did not exist or if not host side of the session
 */
- (QStatus)bindHostedSessionListener:(id<AJNSessionListener>)delegate toSession:(AJNSessionId)sessionId;

/**
 * Leave an existing session.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call to the local daemon
 * and interprets the response.
 * This method cannot be called on self-joined session.
 *
 * @param  sessionId     Session id.
 *
 * @return  - ER_OK if daemon response was received and the leave operation was successfully completed.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - ER_BUS_NO_SESSION if session did not exist
 *          - Other error status codes indicating a failure.
 */
- (QStatus)leaveSession:(AJNSessionId)sessionId;

/**
 * Leave an existing session as joiner. This function will fail if you were not the joiner.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveJoinedSession method call to the local router
 * and interprets the response.
 *
 * @param  sessionId     Session id.
 *
 * @return - ER_OK if router response was received and the leave operation was successfully completed.
 *         - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *         - ER_BUS_NO_SESSION if session did not exist or if not joiner of the session.
 *         - Other error status codes indicating a failure.
 */
- (QStatus)leaveJoinedSession:(AJNSessionId)sessionId;

/**
 * Leave an existing session as host. This function will fail if you were not the host.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveHostedSession method call to the local router
 * and interprets the response.
 *
 * @param  sessionId     Session id.
 *
 * @return - ER_OK if router response was received and the leave operation was successfully completed.
 *         - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *         - ER_BUS_NO_SESSION if session did not exist or if not host of the session.
 *         - Other error status codes indicating a failure.
 */
- (QStatus)leaveHostedSession:(AJNSessionId)sessionId;

/**
 * Remove a member from an existing multipoint session.
 * This function may be called by the binder of the session to forcefully remove a member from a session.
 *
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.RemoveSessionMember method call to the local
 * daemon and interprets the response.
 *
 * @param  sessionId     Session id.
 * @param  memberName    Member to remove.
 *
 * @return  - #ER_OK iff daemon response was received and the remove member operation was successfully completed.
 *          - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)removeSessionMember:(AJNSessionId)sessionId withName:(NSString *)memberName;

/**
 * Set the link timeout for a session.
 *
 * Link timeout is the maximum number of seconds that an unresponsive daemon-to-daemon connection
 * will be monitored before declaring the session lost (via SessionLost callback). Link timeout
 * defaults to 0 which indicates that AllJoyn link monitoring is disabled.
 *
 * Each transport type defines a lower bound on link timeout to avoid defeating transport
 * specific power management algorithms.
 *
 * @param timeout       Max number of seconds that a link can be unresponsive before being
 *                      declared lost. 0 indicates that AllJoyn link monitoring will be disabled. On
 *                      return, this value will be the resulting (possibly upward) adjusted linkTimeout
 *                      value that acceptable to the underlying transport.
 * @param sessionId     Id of session whose link timeout will be modified.
 *
 * @return  - ER_OK if successful
 *          - ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED if local daemon does not support SetLinkTimeout
 *          - ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT if SetLinkTimeout not supported by destination
 *          - ER_BUS_NO_SESSION if the Session id is not valid
 *          - ER_ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED if SetLinkTimeout failed
 *          - ER_BUS_NOT_CONNECTED if the BusAttachment is not connected to the daemon
 */
- (QStatus)setLinkTimeout:(uint32_t *)timeout forSession:(AJNSessionId)sessionId;

/**
 * Set the link timeout for a session asynchronously.
 *
 * Link timeout is the maximum number of seconds that an unresponsive daemon-to-daemon connection
 * will be monitored before declaring the session lost (via SessionLost callback). Link timeout
 * defaults to 0 which indicates that AllJoyn link monitoring is disabled.
 *
 * Each transport type defines a lower bound on link timeout to avoid defeating transport
 * specific power management algorithms.
 *
 * This call executes asynchronously. When the JoinSession response is received, the callback will be called.
 *
 * @param timeout       Max number of seconds that a link can be unresponsive before being
 *                      declared lost. 0 indicates that AllJoyn link monitoring will be disabled. On
 *                      return, this value will be the resulting (possibly upward) adjusted linkTimeout
 *                      value that acceptable to the underlying transport.
 * @param sessionId     Id of session whose link timeout will be modified.
 * @param delegate      Called when SetLinkTimeout response is received.
 * @param context       User defined context which will be passed as-is to callback.
 *
 * @return  - ER_OK iff method call to local daemon response was was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)setLinkTimeoutAsync:(uint32_t)timeout forSession:(AJNSessionId)sessionId completionDelegate:(id<AJNLinkTimeoutDelegate>)delegate context:(void *)context;

/**
 * Set the link timeout for a session asynchronously.
 *
 * Link timeout is the maximum number of seconds that an unresponsive daemon-to-daemon connection
 * will be monitored before declaring the session lost (via SessionLost callback). Link timeout
 * defaults to 0 which indicates that AllJoyn link monitoring is disabled.
 *
 * Each transport type defines a lower bound on link timeout to avoid defeating transport
 * specific power management algorithms.
 *
 * This call executes asynchronously. When the JoinSession response is received, the callback will be called.
 *
 * @param timeout       Max number of seconds that a link can be unresponsive before being
 *                      declared lost. 0 indicates that AllJoyn link monitoring will be disabled. On
 *                      return, this value will be the resulting (possibly upward) adjusted linkTimeout
 *                      value that acceptable to the underlying transport.
 * @param sessionId     Id of session whose link timeout will be modified.
 * @param block         Called when SetLinkTimeout response is received.
 * @param context       User defined context which will be passed as-is to callback.
 *
 * @return  - ER_OK iff method call to local daemon response was was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)setLinkTimeoutAsync:(uint32_t)timeout forSession:(AJNSessionId)sessionId completionBlock:(AJNLinkTimeoutBlock) block context:(void *)context;

/**
 * Get the file descriptor for a raw (non-message based) session.
 *
 * @param sessionId   Id of an existing streaming session.
 *
 * @return A handle to the socket file descriptor if successful. Otherwise, the return is nil.
 */
- (AJNHandle)socketFileDescriptorForSession:(AJNSessionId)sessionId;

/**
 * Advertise the existence of a well-known name to other (possibly disconnected) AllJoyn daemons.
 *
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.AdvertisedName method call to the local daemon
 * and interprets the response.
 *
 * @param  name         The well-known name to advertise. (Must be owned by the caller via RequestName).
 * @param  mask         Set of transports to use for sending advertisement.
 *
 * @return  - ER_OK iff daemon response was received and advertise was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)advertiseName:(NSString *)name withTransportMask:(AJNTransportMask)mask;

/**
 * Stop advertising the existence of a well-known name to other AllJoyn daemons.
 *
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.CancelAdvertiseName method call to the local daemon
 * and interprets the response.
 *
 * @param  name          A well-known name that was previously advertised via AdvertiseName.
 * @param  mask          Set of transports whose name advertisement will be canceled.
 *
 * @return  - ER_OK iff daemon response was received and advertisements were successfully stopped.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)cancelAdvertisedName:(NSString *)name withTransportMask:(AJNTransportMask)mask;

/**
 * Register interest in a well-known name prefix for the purpose of discovery.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method call to the local daemon
 * and interprets the response.
 *
 * @param  name    Well-known name prefix that application is interested in receiving
 *                           BusListener::FoundAdvertisedName notifications about.
 *
 * @return  - ER_OK iff daemon response was received and discovery was successfully started.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)findAdvertisedName:(NSString *)name;

/**
 * Cancel interest in a well-known name prefix that was previously
 * registered with FindAdvertisedName.  This method is a shortcut/helper
 * that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
 * call to the local daemon and interprets the response.
 *
 * @param  name    Well-known name prefix that application is no longer interested in receiving
 *                           BusListener::FoundAdvertisedName notifications about.
 *
 * @return  - ER_OK iff daemon response was received and cancel was successfully completed.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)cancelFindAdvertisedName:(NSString *)name;

/**
 * Register interest in a well-known name prefix on a transport for the purpose of discovery.
 * This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedNameByTransport method call to the local daemon
 * and interprets the response.
 *
 * @param  name    Well-known name prefix that application is interested in receiving
 *                           BusListener::FoundAdvertisedName notifications about.
 * @param  transports    Transports over which to do well-known name discovery
 *
 * @return  - ER_OK iff daemon response was received and discovery was successfully started.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)findAdvertisedName:(NSString *)name byTransport:(AJNTransportMask)transports;

/**
 * Cancel interest in a well-known name prefix that was previously
 * registered with FindAdvertisedName.  This method is a shortcut/helper
 * that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
 * call to the local daemon and interprets the response.
 *
 * @param  name    Well-known name prefix that application is no longer interested in receiving
 *                           BusListener::FoundAdvertisedName notifications about.
 * @param transports    Transports over which to cancel well-known name discovery
 *
 * @return  - ER_OK iff daemon response was received and cancel was successfully completed.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)cancelFindAdvertisedName:(NSString *)name byTransport:(AJNTransportMask)transports;

/**
 * Add a DBus match rule.
 * This method is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch method call to the local daemon.
 *
 * @param  matchRule  Match rule to be added (see DBus specification for format of this string).
 *
 * @return  - ER_OK if the AddMatch request was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)addMatchRule:(NSString *)matchRule;

/**
 * Remove a DBus match rule.
 * This method is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch method call to the local daemon.
 *
 * @param  matchRule  Match rule to be removed (see DBus specification for format of this string).
 *
 * @return  - ER_OK if the RemoveMatch request was successful.
 *          - ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
 *          - Other error status codes indicating a failure.
 */
- (QStatus)removeMatchRule:(NSString *)matchRule;

/**
 * Adds a logon entry string for the requested authentication mechanism to the key store. This
 * allows an authenticating server to generate offline authentication credentials for securely
 * logging on a remote peer using a user-name and password credentials pair. This only applies
 * to authentication mechanisms that support a user name + password logon functionality.
 *
 * @param authenticationMechanism The authentication mechanism.
 * @param userName      The user name to use for generating the logon entry.
 * @param password      The password to use for generating the logon entry. If the password is
 *                      NULL the logon entry is deleted from the key store.
 *
 * @return  - ER_OK if the logon entry was generated.
 *          - ER_BUS_INVALID_AUTH_MECHANISM if the authentication mechanism does not support
 *                                       logon functionality.
 *          - ER_BAD_ARG_2 indicates a null string was used as the user name.
 *          - ER_BAD_ARG_3 indicates a null string was used as the password.
 *          - Other error status codes indicating a failure
 */
- (QStatus)addLogonEntryToKeyStoreWithAuthenticationMechanism:(NSString *)authenticationMechanism userName:(NSString *)userName password:(NSString *)password;

/**
 * Enable peer-to-peer security. This function must be called by applications that want to use
 * authentication and encryption . The bus must have been started by calling
 * BusAttachment::start before this function is called. If the application is providing its
 * own key store implementation it must have already called registerKeyStoreListener before
 * calling this function.
 *
 * @param authenticationMechanisms   The authentication mechanism(s) to use for peer-to-peer authentication.
 *                         If this parameter is NULL peer-to-peer authentication is disabled.
 *
 * @param listener         Passes password and other authentication related requests to the application.
 *
 * @return  - ER_OK if peer security was enabled.
 *          - ER_BUS_BUS_NOT_STARTED BusAttachment::Start has not be called
 */
- (QStatus)enablePeerSecurity:(NSString *)authenticationMechanisms authenticationListener:(id<AJNAuthenticationListener>)listener;

/**
 * Enable peer-to-peer security. This function must be called by applications that want to use
 * authentication and encryption . The bus must have been started by calling
 * BusAttachment::Start() before this function is called. If the application is providing its
 * own key store implementation it must have already called registerKeyStoreListener before
 * calling this function.
 *
 * @param authenticationMechanisms   The authentication mechanism(s) to use for peer-to-peer authentication.
 *                         If this parameter is NULL peer-to-peer authentication is disabled.
 *
 * @param listener         Passes password and other authentication related requests to the application.
 *
 * @param fileName         Optional parameter to specify the filename of the default key store. The
 *                         default value is the applicationName parameter of BusAttachment::initWithApplicationName.
 *                         Note that this parameter is only meaningful when using the default
 *                         key store implementation.
 *
 * @param isShared         optional parameter that indicates if the key store is shared between multiple
 *                         applications. It is generally harmless to set this to true even when the
 *                         key store is not shared but it adds some unnecessary calls to the key store
 *                         listener to load and store the key store in this case.
 *
 * @return  - ER_OK if peer security was enabled.
 *          - ER_BUS_BUS_NOT_STARTED BusAttachment::Start has not be called
 */
- (QStatus)enablePeerSecurity:(NSString *)authenticationMechanisms authenticationListener:(id<AJNAuthenticationListener>) listener keystoreFileName:(NSString *)fileName sharing:(BOOL)isShared;

/**
 * Set a key store listener to listen for key store load and store requests.
 * This overrides the internal key store listener.
 *
 * @param listener  The key store listener to set.
 *
 * @return  - ER_OK if the key store listener was set
 *          - ER_BUS_LISTENER_ALREADY_SET if a listener has been set by this function or because EnablePeerSecurity has been called.
 */
- (QStatus)registerKeyStoreListener:(id<AJNKeyStoreListener>)listener;

/**
 * Reloads the key store for this bus attachment. This function would normally only be called in
 * the case where a single key store is shared between multiple bus attachments, possibly by different
 * applications. It is up to the applications to coordinate how and when the shared key store is
 * modified.
 *
 * @return - ER_OK if the key store was successfully reloaded
 *         - An error status indicating that the key store reload failed.
 */
- (QStatus)reloadKeyStore;

/**
 * Clears all stored keys from the key store. All store keys and authentication information is
 * deleted and cannot be recovered. Any passwords or other credentials will need to be reentered
 * when establishing secure peer connections.
 */
- (void)clearKeyStore;

/**
 * Clear the keys associated with a specific remote peer as identified by its peer GUID. The
 * peer GUID associated with a bus name can be obtained by calling guidForPeerNamed:.
 *
 * @param peerId  The guid of a remote authenticated peer.
 *
 * @return  - ER_OK if the keys were cleared
 *          - ER_UNKNOWN_GUID if there is no peer with the specified GUID
 *          - Other errors
 */
- (QStatus)clearKeysForRemotePeerWithId:(NSString *)peerId;

/**
 * Get the expiration time on keys associated with a specific authenticated remote peer as
 * identified by its peer GUID. The peer GUID associated with a bus name can be obtained by
 * calling guidForPeerNamed:.
 *
 * @param timeout  The time in seconds relative to the current time when the keys will expire.
 * @param peerId   The GUID of a remote authenticated peer.
 *
 * @return  - ER_OK if the expiration time was successfully set.
 *          - ER_UNKNOWN_GUID if there is no authenticated peer with the specified GUID
 *          - Other errors
 */
- (QStatus)keyExpiration:(uint32_t *)timeout forRemotePeerId:(NSString *)peerId;

/**
 * Set the expiration time on keys associated with a specific remote peer as identified by its
 * peer GUID. The peer GUID associated with a bus name can be obtained by calling guidForPeerNamed:.
 * If the timeout is 0 this is equivalent to calling clearKeysForRemotePeerWithId:.
 *
 * @param timeout    The time in seconds relative to the current time to expire the keys.
 * @param peerId     The GUID of a remote authenticated peer.
 *
 * @return  - ER_OK if the expiration time was successfully set.
 *          - ER_UNKNOWN_GUID if there is no authenticated peer with the specified GUID
 *          - Other errors
 */
- (QStatus)setKeyExpiration:(uint32_t)timeout forRemotePeerId:(NSString *)peerId;

/**
 * Get the peer GUID for this peer of the local peer or an authenticated remote peer. The bus
 * names of a remote peer can change over time, specifically the unique name is different each
 * time the peer connects to the bus and a peer may use different well-known-names at different
 * times. The peer GUID is the only persistent identity for a peer. Peer GUIDs are used by the
 * authentication mechanisms to uniquely and identify a remote application instance. The peer
 * GUID for a remote peer is only available if the remote peer has been authenticated.
 *
 * @param peerName  Name of a remote peer or NULL to get the local (this application's) peer GUID.
 *
 * @return  - On success, returns the guid for the local or remote peer depending on the value of name.
 *          - Returns nil otherwise.
 */
- (NSString*)guidForPeerNamed:(NSString *)peerName;

/**
 * This sets the debug level of the local AllJoyn daemon if that daemon
 * was built in debug mode.
 *
 * The debug level can be set for individual subsystems or for "ALL"
 * subsystems.  Common subsystems are "ALLJOYN" for core AllJoyn code,
 * "ALLJOYN_OBJ" for the sessions management code and "ALLJOYN_NS" for
 * the TCP name services.  Debug levels for specific subsystems override
 * the setting for "ALL" subsystems.  For example if "ALL" is set to 7,
 * but "ALLJOYN_OBJ" is set to 1, then detailed debug output will be
 * generated for all subsystems except for "ALLJOYN_OBJ" which will only
 * generate high level debug output.  "ALL" defaults to 0 which is off,
 * or no debug output.
 *
 * The debug output levels are actually a bit field that controls what
 * output is generated.  Those bit fields are described below:
 *
 *     - 0x1: High level debug prints (these debug printfs are not common)
 *     - 0x2: Normal debug prints (these debug printfs are common)
 *     - 0x4: Function call tracing (these debug printfs are used
 *            sporadically)
 *     - 0x8: Data dump (really only used in the "SOCKET" module - can
 *            generate a *lot* of output)
 *
 * Typically, when enabling debug for a subsystem, the level would be set
 * to 7 which enables High level debug, normal debug, and function call
 * tracing.  Setting the level 0, forces debug output to be off for the
 * specified subsystem.
 *
 * @param level     debug level to set for the module
 * @param module    name of the module to generate debug output
 *
 * @return  - ER_OK if debug request was successfully sent to the AllJoyn daemon.
 *          - ER_BUS_NO_SUCH_OBJECT if daemon was not built in debug mode.
 */
- (QStatus)setDaemonDebugLevel:(uint32_t)level forModule:(NSString *)module;


/**
 * Determine if you are able to find a remote connection based on its BusName.
 * The BusName can be the Unique or well-known name.
 * @param name The unique or well-known name to ping
 * @param timeout Timeout specified in milliseconds to wait for reply
 * @return
 *   - ER_OK the name is present and responding
 *   - ER_ALLJOYN_PING_REPLY_UNREACHABLE the name is no longer present
 *
 *   The following return values indicate that the router cannot determine if the 
 *   remote name is present and responding:
 *   - ER_ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out
 *   - ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session
 *   - ER_ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE the remote routing node does not implement Ping
 *
 *   The following return values indicate an error with the ping call itself:
 *   - ER_ALLJOYN_PING_FAILED Ping failed
 *   - ER_BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error
 *   - ER_BUS_NOT_CONNECTED the BusAttachment is not connected to the bus
 *   - ER_BUS_BAD_BUS_NAME the name parameter is not a valid bus name
 *   - An error status otherwise
 */
- (QStatus)pingPeer:(NSString *)name withTimeout:(uint32_t)timeout;

/**
 * Determine if you are able to find a remote connection based on its BusName.
 * The BusName can be the Unique or well-known name.
 * @param name The unique or well-known name to ping
 * @param timeout Timeout specified in milliseconds to wait for reply
 * @param delegate Called when pingPeerAsync response is received.
 * @param context  User defined context which will be passed as-is to callback.
 * @return
 *     - #ER_OK if ping was successful.
 *     - #ER_BUS_NOT_CONNECTED the BusAttachment is not connected to the bus
 *     - #ER_BUS_BAD_BUS_NAME the name parameter is not a valid bus name
 *     - #ER_BAD_ARG_1 a NULL pointer was passed in for the name
 *     - Other error status codes indicating a failure.
 */
- (QStatus)pingPeerAsync:(NSString *)name withTimeout:(uint32_t)timeout completionDelegate:(id<AJNPingPeerDelegate>)delegate context:(void *)context;

/**
 * Determine if you are able to find a remote connection based on its BusName.
 * The BusName can be the Unique or well-known name.
 * @param name The unique or well-known name to ping
 * @param timeout Timeout specified in milliseconds to wait for reply
 * @param block Called when pingPeerAsync response is received.
 * @param context  User defined context which will be passed as-is to callback.
 * @return
 *     - #ER_OK if ping was successful.
 *     - #ER_BUS_NOT_CONNECTED the BusAttachment is not connected to the bus
 *     - #ER_BUS_BAD_BUS_NAME the name parameter is not a valid bus name
 *     - #ER_BAD_ARG_1 a NULL pointer was passed in for the name
 *     - Other error status codes indicating a failure.
 */
- (QStatus)pingPeerAsync:(NSString *)name withTimeout:(uint32_t)timeout completionBlock:(AJNPingPeerBlock)block context:(void *)context;

/**
 * Returns the current non-absolute real-time clock used internally by AllJoyn. This value can be
 * compared with the timestamps on messages to calculate the time since a timestamped message
 * was sent.
 *
 * @return  The current timestamp in milliseconds.
 */
+ (uint32_t)currentTimeStamp;

/**
 * Set this BusAttachment's AJNTranslator
 * @param translator AJNTranslator instance
 */
- (void)setDescriptionTranslator:(id<AJNTranslator>)translator;


/**
 * Registers a handler to receive the org.alljoyn.about Announce signal.
 *
 * The handler is only called if a call to WhoImplements has been has been
 * called.
 *
 * Important the AboutListener should be registered before calling WhoImplments
 *
 * @param[in] aboutListener reference to AboutListener
 */
- (void)registerAboutListener:(id<AJNAboutListener>)aboutListener;

/**
 * Unregisters the AnnounceHandler from receiving the org.alljoyn.about Announce signal.
 *
 * @param[in] aboutListener reference to AboutListener to unregister
 */
- (void)unregisterAboutListener:(id<AJNAboutListener>)aboutListener;

/**
 * Unregisters all AboutListeners from receiving any org.alljoyn.about Announce signal
 */
- (void)unregisterAllAboutListeners;

/**
 * List the interfaces your application is interested in.  If a remote device
 * is announcing that interface then the all Registered AnnounceListeners will
 * be called.
 *
 * For example, if you need both "com.example.Audio" <em>and</em>
 * "com.example.Video" interfaces then do the following.
 * RegisterAboutListener once:
 * @code
 * const char* interfaces[] = {"com.example.Audio", "com.example.Video"};
 * RegisterAboutListener(aboutListener);
 * WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
 * @endcode
 *
 * If the handler should be called if "com.example.Audio" <em>or</em>
 * "com.example.Video" interfaces are implemented then call
 * RegisterAboutListener multiple times:
 * @code
 *
 * RegisterAboutListener(aboutListener);
 * const char* audioInterface[] = {"com.example.Audio"};
 * WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
 * WhoImplements(audioInterface, sizeof(audioInterface) / sizeof(audioInterface[0]));
 * const char* videoInterface[] = {"com.example.Video"};
 * WhoImplements(videoInterface, sizeof(videoInterface) / sizeof(videoInterface[0]));
 * @endcode
 *
 * The interface name may be a prefix followed by a *.  Using
 * this, the example where we are interested in "com.example.Audio" <em>or</em>
 * "com.example.Video" interfaces could be written as:
 * @code
 * const char* exampleInterface[] = {"com.example.*"};
 * RegisterAboutListener(aboutListener);
 * WhoImplements(exampleInterface, sizeof(exampleInterface) / sizeof(exampleInterface[0]));
 * @endcode
 *
 * The AboutListener will receive any announcement that implements an interface
 * beginning with the "com.example." name.
 *
 * If the same AboutListener is used for for multiple interfaces then it is
 * the listeners responsibility to parse through the reported interfaces to
 * figure out what should be done in response to the Announce signal.
 *
 * Note: specifying NULL for the implementsInterfaces parameter could have
 * significant impact on network performance and should be avoided unless
 * its known that all announcements are needed.
 *
 * @param[in] implementsInterfaces a list of interfaces that the Announce
 *               signal reports as implemented. NULL to receive all Announce
 *               signals regardless of interfaces
 * @param[in] numberInterfaces the number of interfaces in the
 *               implementsInterfaces list
 * @return status
 */
- (QStatus)whoImplementsInterfaces:(NSArray *)interfaces numberOfInterfaces:(size_t)numberInterfaces;

/**
 * List an interface your application is interested in.  If a remote device
 * is announcing that interface then the all Registered AnnounceListeners will
 * be called.
 *
 * This is identical to WhoImplements(const char**, size_t)
 * except this is specialized for a single interface not several interfaces.
 *
 * @see WhoImplements(const char**, size_t)
 * @param[in] interface     interface that the remove user must implement to
 *                          receive the announce signal.
 *
 * @return
 *    - #ER_OK on success
 *    - An error status otherwise
 */
- (QStatus)whoImplementsInterface:(NSString*)interface;

/**
 * Stop showing interest in the listed interfaces. Stop recieving announce
 * signals from the devices with the listed interfaces.
 *
 * Note if WhoImplements has been called multiple times the announce signal
 * will still be received for any interfaces that still remain.
 *
 * @param[in] implementsInterfaces a list of interfaces that the Announce
 *               signal reports as implemented. NULL to receive all Announce
 *               signals regardless of interfaces
 * @param[in] numberInterfaces the number of interfaces in the
 *               implementsInterfaces list
 * @return
 *    - #ER_OK on success
 *    - An error status otherwise
 */
- (QStatus)cancelWhoImplementsInterfaces:(NSArray *)interfaces numberOfInterfaces:(size_t)numberInterfaces;

/**
 * Stop showing interest in the listed interfaces. Stop recieving announce
 * signals from the devices with the listed interfaces.
 *
 * This is identical to CancelWhoImplements(const char**, size_t)
 * except this is specialized for a single interface not several interfaces.
 *
 * @see CancelWhoImplements(const char**, size_t)
 * @param[in] interface     interface that the remove user must implement to
 *                          receive the announce signal.
 *
 * @return
 *    - #ER_OK on success
 *    - An error status otherwise
 */
-(QStatus)cancelWhoImplements:(NSString *)interface;

@end
