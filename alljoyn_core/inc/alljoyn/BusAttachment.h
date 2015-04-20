/**
 * @file BusAttachment.h is the top-level object responsible for connecting to a
 * message bus.
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
#ifndef _ALLJOYN_BUSATTACHMENT_H
#define _ALLJOYN_BUSATTACHMENT_H

#ifndef __cplusplus
#error Only include BusAttachment.h in C++ code.
#endif

#include <qcc/platform.h>

#include <qcc/String.h>
#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/Session.h>
#include <alljoyn/SessionListener.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/Status.h>
#include <alljoyn/Translator.h>

namespace ajn {


/**
 * %BusAttachment is the top-level object responsible for connecting to and optionally managing a message bus.
 */
class BusAttachment : public MessageReceiver {

  public:

    /**
     * Pure virtual base class implemented by classes that wish to call JoinSessionAsync().
     */
    class JoinSessionAsyncCB {
      public:
        /** Destructor */
        virtual ~JoinSessionAsyncCB() { }

        /**
         * Called when JoinSessionAsync() completes.
         *
         * @param status       ER_OK if successful
         * @param sessionId    Unique identifier for session.
         * @param opts         Session options.
         * @param context      User defined context which will be passed as-is to callback.
         */
        virtual void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) = 0;
    };

    /**
     * Pure virtual base class implemented by classes that wish to call LeaveSessionAsync() or any of its variants.
     */
    class LeaveSessionAsyncCB {
      public:
        /** Destructor */
        virtual ~LeaveSessionAsyncCB() { }

        /**
         * Called when LeaveSessionAsync() or any of its variants completes.
         *
         * @param status       ER_OK if successful
         * @param context      User defined context which will be passed as-is to callback.
         */
        virtual void LeaveSessionCB(QStatus status, void* context) = 0;
    };

    /**
     * Pure virtual base class implemented by classes that wish to call SetLinkTimeoutAsync().
     */
    class SetLinkTimeoutAsyncCB {
      public:
        /** Destructor */
        virtual ~SetLinkTimeoutAsyncCB() { }

        /**
         * Called when SetLinkTimeoutAsync() completes.
         *
         * @param status       ER_OK if successful
         * @param timeout      Timeout value (possibly adjusted from original request).
         * @param context      User defined context which will be passed as-is to callback.
         */
        virtual void SetLinkTimeoutCB(QStatus status, uint32_t timeout, void* context) = 0;
    };

    /**
     * Pure virtual base class implemented by classes that wish to call PingAsync().
     */
    class PingAsyncCB {
      public:
        /** Destructor */
        virtual ~PingAsyncCB() { }

        /**
         * Called when PingAsync() completes.
         *
         * @param status
         *   - #ER_OK the name is present and responding
         *   - #ER_ALLJOYN_PING_REPLY_UNREACHABLE the name is no longer present
         *   <br>
         *   The following status values indicate that the router cannot determine if the
         *   remote name is present and responding:
         *   - #ER_ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out
         *   - #ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session
         *   - #ER_ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE the remote routing node does not implement Ping
         *   <br>
         *   The following status values indicate an error with the ping call itself:
         *   - #ER_ALLJOYN_PING_FAILED Ping failed
         *   - #ER_BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error
         * @param context      User defined context which will be passed as-is to callback.
         */
        virtual void PingCB(QStatus status, void* context) = 0;
    };

    /**
     * Pure virtual base class implemented by classes that wish to call GetNameOwnerAsync().
     */
    class GetNameOwnerAsyncCB {
      public:
        /** Destructor */
        virtual ~GetNameOwnerAsyncCB() { }

        /**
         * Called when GetNameOwnerAsync() completes.
         *
         * @param status        ER_OK if successful
         * @param uniqueName    Unique name that owns the requested alias.
         * @param context       User defined context which will be passed as-is to callback.
         */
        virtual void GetNameOwnerCB(QStatus status, const char* uniqueName, void* context) = 0;
    };

    /**
     * Construct a BusAttachment.
     *
     * @param applicationName       Name of the application.
     * @param allowRemoteMessages   True if this attachment is allowed to receive messages from remote devices.
     * @param concurrency           The maximum number of concurrent method and signal handlers locally executing.
     */
    BusAttachment(const char* applicationName, bool allowRemoteMessages = false, uint32_t concurrency = 4);

    /** Destructor */
    virtual ~BusAttachment();

    /**
     * Get the concurrent method and signal handler limit.
     *
     * @return The maximum number of concurrent method and signal handlers.
     */
    uint32_t GetConcurrency();

    /**
     * Get the connect spec used by the BusAttachment
     *
     * @return The string representing the connect spec used by the BusAttachment
     */
    qcc::String GetConnectSpec();

    /**
     * Allow the currently executing method/signal handler to enable concurrent callbacks
     * during the scope of the handler's execution.
     */
    void EnableConcurrentCallbacks();

    /**
     * Create an interface description with a given name.
     *
     * Typically, interfaces that are implemented by BusObjects are created here.
     * Interfaces that are implemented by remote objects are added automatically by
     * the bus if they are not already present via ProxyBusObject::IntrospectRemoteObject().
     *
     * Because interfaces are added both explicitly (via this method) and implicitly
     * (via @c ProxyBusObject::IntrospectRemoteObject), there is the possibility that creating
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
     *
     * @see ProxyBusObject::IntrospectRemoteObject, InterfaceDescription::Activate, BusAttachment::GetInterface
     */
    QStatus CreateInterface(const char* name, InterfaceDescription*& iface, InterfaceSecurityPolicy secPolicy);

    /**
     * Deprecated API for creating an interface description with a given name.
     *
     * @param name   The requested interface name.
     * @param[out] iface
     *      - Interface description
     *      - NULL if cannot be created.
     * @param secure If true the interface is secure and method calls and signals will be encrypted.
     *
     * @return
     *      - #ER_OK if creation was successful.
     *      - #ER_BUS_IFACE_ALREADY_EXISTS if requested interface already exists
     */
    QStatus CreateInterface(const char* name, InterfaceDescription*& iface, bool secure = false) {
        return CreateInterface(name, iface, secure ? AJ_IFC_SECURITY_REQUIRED : AJ_IFC_SECURITY_INHERIT);
    }

    /**
     * Initialize one more interface descriptions from an XML string in DBus introspection format.
     * The root tag of the XML can be a \<node\> or a stand alone \<interface\> tag. To initialize more
     * than one interface the interfaces need to be nested in a \<node\> tag.
     *
     * Note that when this method fails during parsing, the return code will be set accordingly.
     * However, any interfaces which were successfully parsed prior to the failure may be registered
     * with the bus.
     *
     * @param xml     An XML string in DBus introspection format.
     *
     * @return
     *      - #ER_OK if parsing is completely successful.
     *      - An error status otherwise.
     */
    QStatus CreateInterfacesFromXml(const char* xml);

    /**
     * Returns the existing activated InterfaceDescriptions.
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
     * Retrieve an existing activated InterfaceDescription.
     *
     * @param name       Interface name
     *
     * @return
     *      - A pointer to the registered interface
     *      - NULL if interface doesn't exist
     */
    const InterfaceDescription* GetInterface(const char* name) const;

    /**
     * Delete an interface description with a given name.
     *
     * Deleting an interface is only allowed if that interface has never been activated.
     *
     * @param iface  The un-activated interface to be deleted.
     *
     * @return
     *      - #ER_OK if deletion was successful
     *      - #ER_BUS_NO_SUCH_INTERFACE if interface was not found
     */
    QStatus DeleteInterface(InterfaceDescription& iface);

    /**
     * @brief Start the process of spinning up the independent threads used in
     * the bus attachment, preparing it for action.
     *
     * This method only begins the process of starting the bus. Sending and
     * receiving messages cannot begin until the bus is Connect()ed.
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
     * As soon as Start() is called, clients of a bus attachment with listeners
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
     * The BusAttachment methods Start(), Stop() and Join() all work together to
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
     * Note that neither of Start() nor Stop() are synchronous in the sense that
     * one has actually accomplished the desired effect upon the return from a
     * call.  Of particular interest is the fact that after a call to Stop(),
     * threads will still be *running* for some non-deterministic time.
     *
     * In order to wait until all of the threads have actually stopped, a
     * blocking call is required.  In threading packages this is typically
     * called join, and our corresponding method is called Join().
     *
     * A Start() method call should be thought of as mapping to a threading
     * package start function.  it causes the activity threads in the
     * BusAttachment to be spun up and gets the attachment ready to do its main
     * job.  As soon as Start() is called, the user should be prepared for one
     * or more of these threads of execution to pop out of the bus attachment
     * and into a listener callback.
     *
     * The Stop() method call should be thought of as mapping to a threading
     * package stop function.  It asks the BusAttachment to begin shutting down
     * its various threads of execution, but does not wait for any threads to exit.
     *
     * A call to the Join() method should be thought of as mapping to a
     * threading package join function call.  It blocks and waits until all of
     * the threads in the BusAttachment have in fact exited their Run functions,
     * gone through the stopping state and have returned their status.  When
     * the Join() method returns, one may be assured that no threads are running
     * in the bus attachment, and therefore there will be no callbacks in
     * progress and no further callbacks will ever come out of a particular
     * instance of a bus attachment.
     *
     * It is important to understand that since Start(), Stop() and Join() map
     * to threads concepts and functions, one should not expect them to clean up
     * any bus attachment state when they are called.  These functions are only
     * present to help in orderly termination of complex threading systems.
     *
     * @see Stop()
     * @see Join()
     *
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_BUS_ALREADY_STARTED if already started
     *      - Other error status codes indicating a failure
     */
    QStatus Start();

    /**
     * @brief Ask the threading subsystem in the bus attachment to begin the
     * process of ending the execution of its threads.
     *
     * The Stop() method call on a bus attachment should be thought of as
     * mapping to a threading package stop function.  It asks the BusAttachment
     * to begin shutting down its various threads of execution, but does not
     * wait for any threads to exit.
     *
     * A call to Stop() is implied as one of the first steps in the destruction
     * of a bus attachment.
     *
     * @warning There is no guarantee that a listener callback may begin executing
     * after a call to Stop().  To achieve that effect, the Stop() must be followed
     * by a Join().
     *
     * @see Start()
     * @see Join()
     *
     * @return
     *     - #ER_OK if successful.
     *     - An error QStatus if unable to begin the process of stopping the
     *       message bus threads.
     */
    QStatus Stop();

    /**
     * @brief Wait for all of the threads spawned by the bus attachment to be
     * completely exited.
     *
     * A call to the Join() method should be thought of as mapping to a
     * threading package join function call.  It blocks and waits until all of
     * the threads in the BusAttachment have, in fact, exited their Run functions,
     * gone through the stopping state and have returned their status.  When
     * the Join() method returns, one may be assured that no threads are running
     * in the bus attachment, and therefore there will be no callbacks in
     * progress and no further callbacks will ever come out of the instance of a
     * bus attachment on which Join() was called.
     *
     * A call to Join() is implied as one of the first steps in the destruction
     * of a bus attachment.  Thus, when a bus attachment is destroyed, it is
     * guaranteed that before it completes its destruction process, there will be
     * no callbacks in process.
     *
     * @warning If Join() is called without a previous Stop() it will result in
     * blocking "forever."
     *
     * @see Start()
     * @see Stop()
     *
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_BUS_ALREADY_STARTED if already started
     *      - Other error status codes indicating a failure
     */
    QStatus Join();

    /**
     * @brief Determine if the bus attachment has been Start()ed.
     *
     * @see Start()
     * @see Stop()
     * @see Join()
     *
     * @return true if the message bus has been Start()ed.
     */
    bool IsStarted() { return isStarted; }

    /**
     * @brief Determine if the bus attachment has been Stop()ed.
     *
     * @see Start()
     * @see Stop()
     * @see Join()
     *
     * @return true if the message bus has been Start()ed.
     */
    bool IsStopping() { return isStopping; }

    /**
     * Connect to an AllJoyn router at a specific connectSpec destination.
     *
     * If there is no router present at the given connectSpec or if the router
     * at the connectSpec has an incompatible AllJoyn version, this method will
     * attempt to use a bundled router if one exists.
     *
     * @param connectSpec  A transport connection spec string of the form:
     *                     @c "<transport>:<param1>=<value1>,<param2>=<value2>...[;]"
     *
     * @return
     *      - #ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Connect(const char* connectSpec);

    /**
     * Connect to a local AllJoyn router.
     *
     * Locate a local AllJoyn router that is compatible with this
     * AllJoyn client's version and connect to the router.
     *
     * @return
     *      - #ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Connect();

    /**
     * Disconnect a remote bus address connection.
     *
     * @deprecated When bundled router is enabled and in-use, the connectSpec
     * will be ignored and the bundled router connectSpec will be used.  Use
     * Disconnect() instead which will use correct connectSpec.
     *
     * @param connectSpec  The transport connection spec used to connect.
     *
     * @return
     *          - #ER_OK if successful
     *          - #ER_BUS_BUS_NOT_STARTED if the bus is not started
     *          - #ER_BUS_NOT_CONNECTED if the %BusAttachment is not connected to the bus
     *          - Other error status codes indicating a failure
     */
    QCC_DEPRECATED(QStatus Disconnect(const char* connectSpec));

    /**
     * %Disconnect the %BusAttachment from the remote bus.
     *
     * @return
     *          - #ER_OK if successful
     *          - #ER_BUS_BUS_NOT_STARTED if the bus is not started
     *          - #ER_BUS_NOT_CONNECTED if the %BusAttachment is not connected to the bus
     *          - Other error status codes indicating a failure
     */
    QStatus Disconnect();

    /**
     * Indicate whether bus is currently connected.
     *
     * Messages can only be sent or received when the bus is connected.
     *
     * @return true if the bus is connected.
     */
    bool IsConnected() const;

    /**
     * Register a BusObject
     *
     * @param obj      BusObject to register.
     * @param secure   true if authentication is required to access this object.
     *
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_BAD_OBJ_PATH for a bad object path
     *      - #ER_BUS_OBJ_ALREADY_EXISTS if an object is already registered at this path
     */
    QStatus RegisterBusObject(BusObject& obj, bool secure = false);

    /**
     * Unregister a BusObject
     *
     * @param object  Object to be unregistered.
     */
    void UnregisterBusObject(BusObject& object);

    /**
     * Get the org.freedesktop.DBus proxy object.
     *
     * @return org.freedesktop.DBus proxy object
     */
    const ProxyBusObject& GetDBusProxyObj();

    /**
     * Get the org.alljoyn.Bus proxy object.
     *
     * @return org.alljoyn.Bus proxy object
     */
    const ProxyBusObject& GetAllJoynProxyObj();

    /**
     * Get the org.alljoyn.Debug proxy object.
     *
     * @return org.alljoyn.Debug proxy object
     */
    const ProxyBusObject& GetAllJoynDebugObj();

    /**
     * Get the unique name of this BusAttachment. Returns an empty string if the bus attachment
     * is not connected.
     *
     * @return The unique name of this BusAttachment.
     */
    const qcc::String GetUniqueName() const;

    /**
     * Get the unique name of specified alias.
     *
     * @param alias Alias name to lookup.
     *
     * @return The unique name of the alias.
     */
    qcc::String GetNameOwner(const char* alias);

    /**
     * Get the unique name of specified alias.
     *
     * @param alias Alias name to lookup.
     *
     * @return The unique name of the alias.
     */
    qcc::String GetNameOwner(const qcc::String& alias) { return GetNameOwner(alias.c_str()); }

    /**
     * Get the unique name of specified alias asyncronously
     *
     * @param[in] alias    Alias name to lookup.
     * @param[in] callback pointer to function that is called with the results
     *                     of the `GetNameOwner` method call.
     * @param[in] context  User defined context which will be passed as-is to
     *                     the callback.
     *
     * @return status code from sending the message to the local routing node.
     */
    QStatus GetNameOwnerAsync(const char* alias, GetNameOwnerAsyncCB* callback, void* context);

    /**
     * Get the unique name of specified alias asyncronously
     *
     * @param[in] alias Alias name to lookup.
     * @param[in] callback pointer to function that is called with the results
     *                     of the `GetNameOwner` method call.
     * @param[in] context  User defined context which will be passed as-is to the
     *                     callback.
     *
     * @return status code from sending the message to the local routing node.
     */
    QStatus GetNameOwnerAsync(const qcc::String& alias, GetNameOwnerAsyncCB* callback, void* context)
    {
        return GetNameOwnerAsync(alias.c_str(), callback, context);
    }

    /**
     * Get the GUID of this BusAttachment as a 32 character hex string.
     *
     * The returned value may be appended to an advertised well-known name in order to guarantee
     * that the resulting name is globally unique.
     *
     * @return GUID of this BusAttachment as a string.
     */
    const qcc::String& GetGlobalGUIDString() const;

    /**
     * Get the GUID of this BusAttachment as an 8 character string.
     *
     * The returned value may be appended to an advertised well-known name in
     * order to guarantee that the resulting name is globally unique.  Note:
     * This version of the string is not quite as unique as the version that
     * returns 32 hex characters, but it is sufficient since the returned
     * string is identical to the base portion of the unique name.  Also, the
     * returned string may begin with a digit and so the application would
     * need to accomodate that if used immediately after a "." in a d-bus
     * compliant bus name.
     *
     * @return GUID of this BusAttachment as a string.
     */
    const qcc::String& GetGlobalGUIDShortString() const;

    /**
     * Register a signal handler.
     *
     * Signals are forwarded to the signalHandler if sender, interface, member and path
     * qualifiers are ALL met.
     *
     * @param receiver       The object receiving the signal.
     * @param signalHandler  The signal handler method.
     * @param member         The interface/member of the signal.
     * @param srcPath        The object path of the emitter of the signal or NULL for all paths.
     * @return #ER_OK
     */
    QStatus RegisterSignalHandler(MessageReceiver* receiver,
                                  MessageReceiver::SignalHandler signalHandler,
                                  const InterfaceDescription::Member* member,
                                  const char* srcPath);

    /**
     * Register a signal handler.
     *
     * Signals are forwarded to the signalHandler if sender, interface, member and rule
     * qualifiers are ALL met.
     *
     * @param receiver       The object receiving the signal.
     * @param signalHandler  The signal handler method.
     * @param member         The interface/member of the signal.
     * @param matchRule      A filter rule.
     * @return #ER_OK
     */
    QStatus RegisterSignalHandlerWithRule(MessageReceiver* receiver,
                                          MessageReceiver::SignalHandler signalHandler,
                                          const InterfaceDescription::Member* member,
                                          const char* matchRule);

    /**
     * Unregister a signal handler.
     *
     * Remove the signal handler that was registered with the given parameters.
     *
     * @param receiver       The object receiving the signal.
     * @param signalHandler  The signal handler method.
     * @param member         The interface/member of the signal.
     * @param srcPath        The object path of the emitter of the signal or NULL for all paths.
     * @return #ER_OK
     */
    QStatus UnregisterSignalHandler(MessageReceiver* receiver,
                                    MessageReceiver::SignalHandler signalHandler,
                                    const InterfaceDescription::Member* member,
                                    const char* srcPath);

    /**
     * Unregister a signal handler.
     *
     * Remove the signal handler that was registered with the given parameters.
     *
     * @param receiver       The object receiving the signal.
     * @param signalHandler  The signal handler method.
     * @param member         The interface/member of the signal.
     * @param matchRule      A filter rule.
     * @return #ER_OK
     */
    QStatus UnregisterSignalHandlerWithRule(MessageReceiver* receiver,
                                            MessageReceiver::SignalHandler signalHandler,
                                            const InterfaceDescription::Member* member,
                                            const char* matchRule);

    /**
     * Unregister all signal and reply handlers for the specified message receiver. This function is
     * intended to be called from within the destructor of a MessageReceiver class instance. It
     * prevents any pending signals or replies from accessing the MessageReceiver after the message
     * receiver has been freed.
     *
     * @param receiver       The message receiver that is being unregistered.
     * @return ER_OK if successful.
     */
    QStatus UnregisterAllHandlers(MessageReceiver* receiver);

    /**
     * Enable peer-to-peer security. This function must be called by applications that want to use
     * authentication and encryption. The bus must have been started by calling
     * BusAttachment::Start() before this function is called. If the application is providing its
     * own key store implementation it must have already called RegisterKeyStoreListener() before
     * calling this function.
     *
     * Once peer security has been enabled it is not possible to change the authMechanism set without
     * clearing it first (setting authMechanism to NULL). This is true regardless of whether the BusAttachment
     * has been disconnected or not.
     *
     * @param authMechanisms   The authentication mechanism(s) to use for peer-to-peer authentication.
     *                         If this parameter is NULL peer-to-peer authentication is disabled.  This is a
     *                         space separated list of any of the following values:
     *                         ALLJOYN_SRP_LOGON, ALLJOYN_SRP_KEYX, ALLJOYN_ECDHE_NULL, ALLJOYN_ECDHE_PSK,
     *                         ALLJOYN_ECDHE_ECDSA, GSSAPI.
     *
     * @param listener         Passes password and other authentication related requests to the application.
     *
     * @param keyStoreFileName Optional parameter to specify the filename of the default key store. The
     *                         default value is the applicationName parameter of BusAttachment().
     *                         Note that this parameter is only meaningful when using the default
     *                         key store implementation.
     *
     * @param isShared         optional parameter that indicates if the key store is shared between multiple
     *                         applications. It is generally harmless to set this to true even when the
     *                         key store is not shared but it adds some unnecessary calls to the key store
     *                         listener to load and store the key store in this case.
     *
     * @return
     *      - #ER_OK if peer security was enabled.
     *      - #ER_BUS_BUS_NOT_STARTED BusAttachment::Start has not be called
     */
    QStatus EnablePeerSecurity(const char* authMechanisms,
                               AuthListener* listener,
                               const char* keyStoreFileName = NULL,
                               bool isShared = false);

    /**
     * Check is peer security has been enabled for this bus attachment.
     *
     * @return   Returns true if peer security has been enabled, false otherwise.
     */
    bool IsPeerSecurityEnabled();


    /**
     * Register an object that will receive bus event notifications.
     *
     * @param listener  Object instance that will receive bus event notifications.
     */
    virtual void RegisterBusListener(BusListener& listener);

    /**
     * Unregister an object that was previously registered with RegisterBusListener.
     *
     * @param listener  Object instance to un-register as a listener.
     */
    virtual void UnregisterBusListener(BusListener& listener);

    /**
     * Set a key store listener to listen for key store load and store requests.
     * This overrides the internal key store listener.
     *
     * @param listener  The key store listener to set.
     *
     * @return
     *      - #ER_OK if the key store listener was set
     *      - #ER_BUS_LISTENER_ALREADY_SET if a listener has been set by this function or because
     *         EnablePeerSecurity has been called.
     */
    QStatus RegisterKeyStoreListener(KeyStoreListener& listener);

    /**
     * Unregister a previously registered KeyStore. This will return control for
     * load and store requests to the default internal keystore listener.
     *
     * @return
     *      - #ER_OK if the key store Unregistered
     */
    QStatus UnregisterKeyStoreListener();

    /**
     * Reloads the key store for this bus attachment. This function would normally only be called in
     * the case where a single key store is shared between multiple bus attachments, possibly by different
     * applications. It is up to the applications to coordinate how and when the shared key store is
     * modified.
     *
     * @return - ER_OK if the key store was successfully reloaded
     *         - An error status indicating that the key store reload failed.
     */
    QStatus ReloadKeyStore();

    /**
     * Clears all stored keys from the key store. All store keys and authentication information is
     * deleted and cannot be recovered. Any passwords or other credentials will need to be reentered
     * when establishing secure peer connections.
     */
    void ClearKeyStore();

    /**
     * Clear the keys associated with a specific remote peer as identified by its peer GUID. The
     * peer GUID associated with a bus name can be obtained by calling GetPeerGUID().
     *
     * @param guid  The guid of a remote authenticated peer.
     *
     * @return  - ER_OK if the keys were cleared
     *          - ER_UNKNOWN_GUID if there is no peer with the specified GUID
     *          - Other errors
     */
    QStatus ClearKeys(const qcc::String& guid);

    /**
     * Set the expiration time on keys associated with a specific remote peer as identified by its
     * peer GUID. The peer GUID associated with a bus name can be obtained by calling GetPeerGUID().
     * If the timeout is 0 this is equivalent to calling ClearKeys().
     *
     * @param guid     The GUID of a remote authenticated peer.
     * @param timeout  The time in seconds relative to the current time to expire the keys.
     *
     * @return  - ER_OK if the expiration time was successfully set.
     *          - ER_UNKNOWN_GUID if there is no authenticated peer with the specified GUID
     *          - Other errors
     */
    QStatus SetKeyExpiration(const qcc::String& guid, uint32_t timeout);

    /**
     * Get the expiration time on keys associated with a specific authenticated remote peer as
     * identified by its peer GUID. The peer GUID associated with a bus name can be obtained by
     * calling GetPeerGUID().
     *
     * @param guid     The GUID of a remote authenticated peer.
     * @param timeout  The time in seconds relative to the current time when the keys will expire.
     *
     * @return  - ER_OK if the expiration time was successfully set.
     *          - ER_UNKNOWN_GUID if there is no authenticated peer with the specified GUID
     *          - Other errors
     */
    QStatus GetKeyExpiration(const qcc::String& guid, uint32_t& timeout);

    /**
     * Adds a logon entry string for the requested authentication mechanism to the key store. This
     * allows an authenticating server to generate offline authentication credentials for securely
     * logging on a remote peer using a user-name and password credentials pair. This only applies
     * to authentication mechanisms that support a user name + password logon functionality.
     *
     * @param authMechanism The authentication mechanism.
     * @param userName      The user name to use for generating the logon entry.
     * @param password      The password to use for generating the logon entry. If the password is
     *                      NULL the logon entry is deleted from the key store.
     *
     * @return
     *      - #ER_OK if the logon entry was generated.
     *      - #ER_BUS_INVALID_AUTH_MECHANISM if the authentication mechanism does not support
     *                                       logon functionality.
     *      - #ER_BAD_ARG_2 indicates a null string was used as the user name.
     *      - #ER_BAD_ARG_3 indicates a null string was used as the password.
     *      - Other error status codes indicating a failure
     */
    QStatus AddLogonEntry(const char* authMechanism, const char* userName, const char* password);

    /**
     * Request a well-known name.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.RequestName method call to the local router
     * and interprets the response.
     *
     * @param[in]  requestedName  Well-known name being requested.
     * @param[in]  flags          Bitmask of DBUS_NAME_FLAG_* defines (see DBusStdDefines.h)
     *
     * @return
     *      - #ER_OK iff router response was received and request was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus RequestName(const char* requestedName, uint32_t flags);

    /**
     * Release a previously requested well-known name.
     *
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.ReleaseName method call to the local router
     * and interprets the response.
     *
     * @param[in]  name          Well-known name being released.
     *
     * @return
     *      - #ER_OK iff router response was received amd the name was successfully released.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus ReleaseName(const char* name);

    /**
     * Add a DBus match rule.
     *
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch method call to the local router.
     *
     * @param[in]  rule  Match rule to be added (see DBus specification for format of this string).
     *
     * @return
     *      - #ER_OK if the AddMatch request was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus AddMatch(const char* rule);

    /**
     * Remove a DBus match rule.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch method call to the local router.
     *
     * @param[in]  rule  Match rule to be removed (see DBus specification for format of this string).
     *
     * @return
     *      - #ER_OK if the RemoveMatch request was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus RemoveMatch(const char* rule);

    /**
     * Add a DBus match rule.
     *
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch method call to the local router.
     * In contrast with the regular AddMatch, this method does not wait for a reply from the local router. That makes
     * the call non-blocking, and hence useful in cases where deadlocks might occur otherwise.
     *
     * @param[in]  rule  Match rule to be added (see DBus specification for format of this string).
     *
     * @return
     *      - #ER_OK if the AddMatch request was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus AddMatchNonBlocking(const char* rule);

    /**
     * Remove a DBus match rule.
     *
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch method call to the local router.
     * In contrast with the regular RemoveMatch, this method does not wait for a reply from the local router. That makes
     * the call non-blocking, and hence useful in cases where deadlocks might occur otherwise.
     *
     * @param[in]  rule  Match rule to be removed (see DBus specification for format of this string).
     *
     * @return
     *      - #ER_OK if the RemoveMatch request was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus RemoveMatchNonBlocking(const char* rule);

    /**
     * Advertise the existence of a well-known name to other (possibly disconnected) AllJoyn routers.
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.AdvertisedName method call to the local router
     * and interprets the response.
     *
     * @param[in]  name          the well-known name to advertise. (Must be owned by the caller via RequestName).
     * @param[in]  transports    Set of transports to use for sending advertisement.
     *
     * @return
     *      - #ER_OK iff router response was received and advertise was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus AdvertiseName(const char* name, TransportMask transports);

    /**
     * Stop advertising the existence of a well-known name to other AllJoyn routers.
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.CancelAdvertiseName method call to the local router
     * and interprets the response.
     *
     * @param[in]  name          A well-known name that was previously advertised via AdvertiseName.
     * @param[in]  transports    Set of transports whose name advertisement will be canceled.
     *
     * @return
     *      - #ER_OK iff router response was received and advertisements were successfully stopped.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus CancelAdvertiseName(const char* name, TransportMask transports);

    /**
     * Register interest in a well-known name prefix for the purpose of discovery over transports included in TRANSPORT_ANY
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method call to the local router
     * and interprets the response.
     *
     * @param[in]  namePrefix    Well-known name prefix that application is interested in receiving
     *                           BusListener::FoundAdvertisedName notifications about.
     *
     * @return
     *      - #ER_OK iff router response was received and discovery was successfully started.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus FindAdvertisedName(const char* namePrefix);

    /**
     * Register interest in a well-known name prefix for the purpose of discovery over specified transports
     *
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method call to the local router
     * and interprets the response.
     *
     * @param[in]  namePrefix    Well-known name prefix that application is interested in receiving
     *                           BusListener::FoundAdvertisedName notifications about.
     * @param[in]  transports    Transports over which to do well-known name discovery
     *
     * @return
     *      - #ER_OK iff router response was received and discovery was successfully started.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus FindAdvertisedNameByTransport(const char* namePrefix, TransportMask transports);

    /**
     * Cancel interest in a well-known name prefix that was previously
     * registered with FindAdvertisedName. This cancels well-known name discovery over transports
     * included in TRANSPORT_ANY.  This method is a shortcut/helper
     * that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
     * call to the local router and interprets the response.
     *
     * @param[in]  namePrefix    Well-known name prefix that application is no longer interested in receiving
     *                           BusListener::FoundAdvertisedName notifications about.
     *
     * @return
     *      - #ER_OK iff router response was received and cancel was successfully completed.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus CancelFindAdvertisedName(const char* namePrefix);

    /**
     * Cancel interest in a well-known name prefix that was previously
     * registered with FindAdvertisedName. This cancels well-known name discovery over
     * the specified transports.  This method is a shortcut/helper
     * that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
     * call to the local router and interprets the response.
     *
     * @param[in]  namePrefix    Well-known name prefix that application is no longer interested in receiving
     *                           BusListener::FoundAdvertisedName notifications about.
     * @param[in]  transports    Transports over which to cancel well-known name discovery
     *
     * @return
     *      - #ER_OK iff router response was received and cancel was successfully completed.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus CancelFindAdvertisedNameByTransport(const char* namePrefix, TransportMask transports);

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
     * @param[in,out] sessionPort      SessionPort value to bind or SESSION_PORT_ANY to allow this method
     *                                 to choose an available port. On successful return, this value
     *                                 contains the chosen SessionPort.
     *
     * @param[in]     opts             Session options that joiners must agree to in order to
     *                                 successfully join the session.
     *
     * @param[in]     listener  Called by the bus when session related events occur.
     *
     * @return
     *      - #ER_OK iff router response was received and the bind operation was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus BindSessionPort(SessionPort& sessionPort, const SessionOpts& opts, SessionPortListener& listener);

    /**
     * Cancel an existing port binding.
     *
     * @param[in]   sessionPort    Existing session port to be un-bound.
     *
     * @return
     *      - #ER_OK iff router response was received and the bind operation was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus UnbindSessionPort(SessionPort sessionPort);

    /**
     * Join a session.
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to the local router
     * and interprets the response.
     *
     * All transports specified in opts will be tried.  If the join fails over one of the transports, the join will
     * be tried over subsequent ones until all are tried until the join succeeds or they all fail.
     *
     * @param[in]  sessionHost      Bus name of attachment that is hosting the session to be joined.
     * @param[in]  sessionPort      SessionPort of sessionHost to be joined.
     * @param[in]  listener         Optional listener called when session related events occur. May be NULL.
     * @param[out] sessionId        Unique identifier for session.
     * @param[in,out] opts          Session options.
     *
     * @return
     *      - #ER_OK iff router response was received and the session was successfully joined.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus JoinSession(const char* sessionHost, SessionPort sessionPort, SessionListener* listener,
                        SessionId& sessionId, SessionOpts& opts);

    /**
     * Join a session.
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to the local router
     * and interprets the response.
     *
     * All transports specified in opts will be tried.  If the join fails over one of the transports, the join will
     * be tried over subsequent ones until all are tried until the join succeeds or they all fail.
     *
     * This call executes asynchronously. When the JoinSession response is received, the callback will be called.
     *
     * @param[in]  sessionHost      Bus name of attachment that is hosting the session to be joined.
     * @param[in]  sessionPort      SessionPort of sessionHost to be joined.
     * @param[in]  listener         Optional listener called when session related events occur. May be NULL.
     * @param[in]  opts             Session options.
     * @param[in]  callback         Called when JoinSession response is received.
     * @param[in]  context          User defined context which will be passed as-is to callback.
     *
     * @return
     *      - #ER_OK iff method call to local router response was was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus JoinSessionAsync(const char* sessionHost,
                             SessionPort sessionPort,
                             SessionListener* listener,
                             const SessionOpts& opts,
                             BusAttachment::JoinSessionAsyncCB* callback,
                             void* context = NULL);


    /**
     * Set the SessionListener for an existing sessionId.
     * This method cannot be called on a self-joined session.
     *
     * Calling this method will override the listener set by a previous call to SetSessionListener,
     * SetHostedSessionListener, SetJoinedSessionListener or any listener specified in JoinSession.
     *
     * @param sessionId    The session id of an existing session.
     * @param listener     The SessionListener to associate with the session. May be NULL to clear previous listener.
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_NO_SESSION if session did not exist
     */
    QStatus SetSessionListener(SessionId sessionId, SessionListener* listener);

    /**
     * Set the SessionListener for an existing sessionId on the joiner side.
     *
     * Calling this method will override the listener set by a previous call to SetSessionListener, SetJoinedSessionListener
     * or any listener specified in JoinSession.
     *
     * @param sessionId    The session id of an existing session.
     * @param listener     The SessionListener to associate with the session. May be NULL to clear previous listener.
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_NO_SESSION if session did not exist or if not joiner side of the session
     */
    QStatus SetJoinedSessionListener(SessionId sessionId, SessionListener* listener);

    /**
     * Set the SessionListener for an existing sessionId on the host side.
     *
     * Calling this method will override the listener set by a previous call to SetSessionListener or SetHostedSessionListener.
     *
     * @param sessionId    The session id of an existing session.
     * @param listener     The SessionListener to associate with the session. May be NULL to clear previous listener.
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_NO_SESSION if session did not exist or if not host side of the session
     */
    QStatus SetHostedSessionListener(SessionId sessionId, SessionListener* listener);

    /**
     * Leave an existing session.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call to the local router
     * and interprets the response.
     * This method cannot be called on self-joined session.
     *
     * @param[in]  sessionId     Session id.
     *
     * @return
     *      - #ER_OK if router response was received and the leave operation was successfully completed.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - #ER_BUS_NO_SESSION if session did not exist.
     *      - Other error status codes indicating a failure.
     */
    QStatus LeaveSession(const SessionId& sessionId);

    /**
     * Leave an existing session as host. This function will fail if you were not the host.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveHostedSession method call to the local router
     * and interprets the response.
     *
     * @param[in]  sessionId     Session id.
     *
     * @return
     *      - #ER_OK if router response was received and the leave operation was successfully completed.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - #ER_BUS_NO_SESSION if session did not exist or if not host of the session.
     *      - Other error status codes indicating a failure.
     */
    QStatus LeaveHostedSession(const SessionId& sessionId);

    /**
     * Leave an existing session as joiner. This function will fail if you were not the joiner.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveJoinedSession method call to the local router
     * and interprets the response.
     *
     * @param[in]  sessionId     Session id.
     *
     * @return
     *      - #ER_OK if router response was received and the leave operation was successfully completed.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - #ER_BUS_NO_SESSION if session did not exist or if not joiner of the session.
     *      - Other error status codes indicating a failure.
     */
    QStatus LeaveJoinedSession(const SessionId& sessionId);

    /**
     * Leave an existing session.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call to the local router
     * and interprets the response.
     * This method cannot be called on self-joined session.
     *
     * This call executes asynchronously. When the LeaveSession response is received, the callback will be called.
     *
     * @param[in]  sessionId     Session id.
     * @param[in]  callback      Called when LeaveSession response is received.
     * @param[in]  context       User defined context which will be passed as-is to callback.
     *
     * @return
     *      - #ER_OK iff method call to local router response was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus LeaveSessionAsync(const SessionId& sessionId,
                              BusAttachment::LeaveSessionAsyncCB* callback,
                              void* context = NULL);

    /**
     * Leave an existing session as host. This function will fail if you were not the host.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveHostedSession method call to the local router
     * and interprets the response.
     *
     * This call executes asynchronously. When the LeaveHostedSession response is received, the callback will be called.
     *
     * @param[in]  sessionId     Session id.
     * @param[in]  callback      Called when LeaveHostedSession response is received.
     * @param[in]  context       User defined context which will be passed as-is to callback.
     *
     * @return
     *      - #ER_OK iff method call to local router response was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus LeaveHostedSessionAsync(const SessionId& sessionId,
                                    BusAttachment::LeaveSessionAsyncCB* callback,
                                    void* context = NULL);

    /**
     * Leave an existing session as joiner. This function will fail if you were not the joiner.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveJoinedSession method call to the local router
     * and interprets the response.
     *
     * This call executes asynchronously. When the LeaveJoinedSession response is received, the callback will be called.
     *
     * @param[in]  sessionId     Session id.
     * @param[in]  callback      Called when LeaveJoinedSession response is received.
     * @param[in]  context       User defined context which will be passed as-is to callback.
     *
     * @return
     *      - #ER_OK iff method call to local router response was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus LeaveJoinedSessionAsync(const SessionId& sessionId,
                                    BusAttachment::LeaveSessionAsyncCB* callback,
                                    void* context = NULL);

    /**
     * Remove a member from an existing multipoint session.
     * This function may be called by the binder of the session to forcefully remove a member from a session.
     *
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.RemoveSessionMember method call to the local router
     * and interprets the response.
     *
     * @param[in]  sessionId     Session id.
     * @param[in]  memberName    Member to remove.
     *
     * @return
     *      - #ER_OK iff router response was received and the remove member operation was successfully completed.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus RemoveSessionMember(SessionId sessionId, qcc::String memberName);

    /**
     * Get the file descriptor for a raw (non-message based) session.
     *
     * @param sessionId   Id of an existing streaming session.
     * @param sockFd      [OUT] Socket file descriptor for session.
     *
     * @return ER_OK if successful.
     */
    QStatus GetSessionFd(SessionId sessionId, qcc::SocketFd& sockFd);

    /**
     * Set the link timeout for a session.
     *
     * Link timeout is the maximum number of seconds that an unresponsive router-to-router connection
     * will be monitored before declaring the session lost (via SessionLost callback). Link timeout
     * defaults to 0 which indicates that AllJoyn link monitoring is disabled.
     *
     * Each transport type defines a lower bound on link timeout to avoid defeating transport
     * specific power management algorithms.
     *
     * @param sessionid     Id of session whose link timeout will be modified.
     * @param linkTimeout   [IN/OUT] Max number of seconds that a link can be unresponsive before being
     *                      declared lost. 0 indicates that AllJoyn link monitoring will be disabled. On
     *                      return, this value will be the resulting (possibly upward) adjusted linkTimeout
     *                      value that acceptable to the underlying transport.
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED if local router does not support SetLinkTimeout
     *      - #ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT if SetLinkTimeout not supported by destination
     *      - #ER_BUS_NO_SESSION if the Session id is not valid
     *      - #ER_ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED if SetLinkTimeout failed
     *      - #ER_BUS_NOT_CONNECTED if the BusAttachment is not connected to the router
     */
    QStatus SetLinkTimeout(SessionId sessionid, uint32_t& linkTimeout);


    /**
     * Set the link timeout for a session.
     *
     * Link timeout is the maximum number of seconds that an unresponsive router-to-router connection
     * will be monitored before declaring the session lost (via SessionLost callback). Link timeout
     * defaults to 0 which indicates that AllJoyn link monitoring is disabled.
     *
     * Each transport type defines a lower bound on link timeout to avoid defeating transport
     * specific power management algorithms.
     *
     * This call executes asynchronously. When the SetLinkTimeout response is received, the callback will be called.
     *
     * @param[in] sessionid     Id of session whose link timeout will be modified.
     * @param[in] linkTimeout   Max number of seconds that a link can be unresponsive before being
     *                          declared lost. 0 indicates that AllJoyn link monitoring will be disabled. On
     *                          return, this value will be the resulting (possibly upward) adjusted linkTimeout
     *                          value that acceptable to the underlying transport.
     * @param[in]  callback     Called when SetLinkTimeout response is received.
     * @param[in]  context      User defined context which will be passed as-is to callback.
     *
     * @return
     *      - #ER_OK iff method call to local router response was was successful.
     *      - #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *      - Other error status codes indicating a failure.
     */
    QStatus SetLinkTimeoutAsync(SessionId sessionid,
                                uint32_t linkTimeout,
                                BusAttachment::SetLinkTimeoutAsyncCB* callback,
                                void* context = NULL);

    /**
     * Determine whether a given well-known name exists on the bus.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.NameHasOwner method call to the router
     * and interprets the response.
     *
     * @param[in]  name       The well known name that the caller is inquiring about.
     * @param[out] hasOwner   If return is ER_OK, indicates whether name exists on the bus.
     *                        If return is not ER_OK, hasOwner parameter is not modified.
     * @return
     *      - #ER_OK if name ownership was able to be determined.
     *      - An error status otherwise
     */
    QStatus NameHasOwner(const char* name, bool& hasOwner);

    /**
     * Get the peer GUID for this peer of the local peer or an authenticated remote peer. The bus
     * names of a remote peer can change over time, specifically the unique name is different each
     * time the peer connects to the bus and a peer may use different well-known-names at different
     * times. The peer GUID is the only persistent identity for a peer. Peer GUIDs are used by the
     * authentication mechanisms to uniquely and identify a remote application instance. The peer
     * GUID for a remote peer is only available if the remote peer has been authenticated.
     *
     * @param name  Name of a remote peer or NULL to get the local (this application's) peer GUID.
     * @param guid  Returns the guid for the local or remote peer depending on the value of name.
     *
     * @return
     *      - #ER_OK if the requested GUID was obtained.
     *      - An error status otherwise.
     */
    QStatus GetPeerGUID(const char* name, qcc::String& guid);

    /**
     * This sets the debug level of the local AllJoyn router if that router
     * was built in debug mode.
     *
     * The debug level can be set for individual subsystems or for "ALL"
     * subsystems.  Common subsystems are "ALLJOYN" for core AllJoyn code,
     * "ALLJOYN_OBJ" for the sessions management code and "ALLJOYN_NS" for the
     * TCP name services.  Debug levels for specific subsystems override the
     * setting for "ALL" subsystems.  For example if "ALL" is set to 7, but
     * "ALLJOYN_OBJ" is set to 1, then detailed debug output will be generated
     * for all subsystems except for "ALLJOYN_OBJ" which will only generate high
     * level debug output.  "ALL" defaults to 0 which is off, or no debug
     * output.
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
     * @param module    name of the module to generate debug output
     * @param level     debug level to set for the module
     *
     * @return
     *     - #ER_OK if debug request was successfully sent to the AllJoyn
     *       router.
     *     - #ER_BUS_NO_SUCH_OBJECT if router was not built in debug mode.
     */
    QStatus SetDaemonDebug(const char* module, uint32_t level);

    /**
     * Returns the current non-absolute real-time clock used internally by AllJoyn. This value can be
     * compared with the timestamps on messages to calculate the time since a timestamped message
     * was sent.
     *
     * @return  The current timestamp in milliseconds.
     */
    static uint32_t AJ_CALL GetTimestamp();

    /**
     * Determine if you are able to find a remote connection based on its BusName.
     * The BusName can be the Unique or well-known name.
     *
     * @param name The unique or well-known name to ping
     * @param timeout Timeout specified in milliseconds to wait for reply
     *
     * @return
     *   - #ER_OK the name is present and responding
     *   - #ER_ALLJOYN_PING_REPLY_UNREACHABLE the name is no longer present
     *   <br>
     *   The following return values indicate that the router cannot determine if the
     *   remote name is present and responding:
     *   - #ER_ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out
     *   - #ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session
     *   - #ER_ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE the remote routing node does not implement Ping
     *   <br>
     *   The following return values indicate an error with the ping call itself:
     *   - #ER_ALLJOYN_PING_FAILED Ping failed
     *   - #ER_BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error
     *   - #ER_BUS_NOT_CONNECTED the BusAttachment is not connected to the bus
     *   - #ER_BUS_BAD_BUS_NAME the name parameter is not a valid bus name
     *   - An error status otherwise
     */
    QStatus Ping(const char* name, uint32_t timeout);

    /**
     * Determine if you are able to find a remote connection based on its BusName.
     * The BusName can be the Unique or well-known name.
     *
     * This call executes asynchronously. When the PingAsync response is received,
     * the callback will be called.
     *
     * @param[in] name The  unique or well-known name to ping
     * @param[in] timeout   Timeout specified in milliseconds to wait for reply
     * @param[in] callback  Called when PingAsync response is received.
     * @param[in] context   User defined context which will be passed as-is to callback.
     *
     * @see PingAsyncCB
     *
     * @return
     *     - #ER_OK iff method call to local router response was successful.
     *     - #ER_BUS_NOT_CONNECTED the BusAttachment is not connected to the bus
     *     - #ER_BUS_BAD_BUS_NAME the name parameter is not a valid bus name
     *     - Other error status codes indicating a failure.
     */
    QStatus PingAsync(const char* name, uint32_t timeout, BusAttachment::PingAsyncCB* callback, void* context);

    /**
     * Set a Translator for all BusObjects and InterfaceDescriptions. This Translator is used for
     * descriptions appearing in introspection. Note that any Translators set on a specific
     * InterfaceDescription or BusObject will be used for those specific elements - this Translator
     * is used only for BusObjects and InterfaceDescriptions that do not have Translators set for them.
     *
     * @param[in]  translator       The Translator instance
     */
    void SetDescriptionTranslator(Translator* translator);

    /**
     * Get this BusAttachment's Translator
     *
     * @return This BusAttachment's Translator
     */
    Translator* GetDescriptionTranslator();

    /**
     * Registers a handler to receive the org.alljoyn.about Announce signal.
     *
     * The handler is only called if a call to WhoImplements has been has been
     * called.
     *
     * Important: the AboutListener should be registered before calling WhoImplements
     *
     * @param[in] aboutListener reference to AboutListener
     */
    void RegisterAboutListener(AboutListener& aboutListener);

    /**
     * Unregisters the AnnounceHandler from receiving the org.alljoyn.about Announce signal.
     *
     * @param[in] aboutListener reference to AboutListener to unregister
     */
    void UnregisterAboutListener(AboutListener& aboutListener);

    /**
     * Unregisters all AboutListeners from receiving any org.alljoyn.about Announce signal
     */
    void UnregisterAllAboutListeners();

    /**
     * List the interfaces your application is interested in.  If a remote device
     * is announcing that interface then all Registered AboutListeners will
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
     * If the AboutListener should be called if "com.example.Audio"
     * <em>or</em> "com.example.Video" interfaces are implemented then call
     * WhoImplements multiple times:
     * @code
     *
     * RegisterAboutListener(aboutListener);
     * const char* audioInterface[] = {"com.example.Audio"};
     * WhoImplements(audioInterface, sizeof(audioInterface) / sizeof(audioInterface[0]));
     * const char* videoInterface[] = {"com.example.Video"};
     * WhoImplements(videoInterface, sizeof(videoInterface) / sizeof(videoInterface[0]));
     * @endcode
     *
     * The interface name may be a prefix followed by a `*`.  Using
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
     * It is the AboutListeners responsibility to parse through the reported
     * interfaces to figure out what should be done in response to the Announce
     * signal.
     *
     * WhoImplements is ref counted. If WhoImplements is called with the same
     * list of interfaces multiple times then CancelWhoImplements must also be
     * called multiple times with the same list of interfaces.
     *
     * Note: specifying NULL for the implementsInterfaces parameter could have
     * significant impact on network performance and should be avoided unless
     * all announcements are needed.
     *
     * @param[in] implementsInterfaces a list of interfaces that the Announce
     *               signal reports as implemented. NULL to receive all Announce
     *               signals regardless of interfaces
     * @param[in] numberInterfaces the number of interfaces in the
     *               implementsInterfaces list
     * @return status
     */
    QStatus WhoImplements(const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * Non-blocking variant of WhoImplements
     * @see WhoImplements(const char**, size_t)
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
    QStatus WhoImplementsNonBlocking(const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * List an interface your application is interested in.  If a remote device
     * is announcing that interface then the all Registered AnnounceListeners will
     * be called.
     *
     * This is identical to WhoImplements(const char**, size_t)
     * except this is specialized for a single interface not several interfaces.
     *
     * Note: specifying NULL for the interface parameter could have significant
     * impact on network performance and should be avoided unless all
     * announcements are needed.
     *
     * @see WhoImplements(const char**, size_t)
     * @param[in] iface     interface that the remote user must implement to
     *                          receive the announce signal.
     *
     * @return
     *    - #ER_OK on success
     *    - An error status otherwise
     */
    QStatus WhoImplements(const char* iface);

    /**
     * Non-blocking variant of WhoImplements
     * @see WhoImplements(const char**, size_t)
     *
     * @param[in] iface     interface that the remote user must implement to
     *                          receive the announce signal.
     * @return
     *    - #ER_OK on success
     *    - An error status otherwise
     */
    QStatus WhoImplementsNonBlocking(const char* iface);

    /**
     * Stop showing interest in the listed interfaces. Stop receiving announce
     * signals from the devices with the listed interfaces.
     *
     * Note if WhoImplements has been called multiple times the announce signal
     * will still be received for any interfaces that still remain.
     *
     * @param[in] implementsInterfaces a list of interfaces. The list must match the
     *                                 list previously passed to the WhoImplements
     *                                 member function
     * @param[in] numberInterfaces the number of interfaces in the
     *               implementsInterfaces list
     * @return
     *    - #ER_OK on success
     *    - #ER_BUS_MATCH_RULE_NOT_FOUND if interfaces added using the WhoImplements
     *                                   member function were not found.
     *    - An error status otherwise
     */
    QStatus CancelWhoImplements(const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * Non-blocking variant of CancelWhoImplements.
     *
     * @see CancelWhoImplements(const char**, size_t)
     * @param[in] implementsInterfaces a list of interfaces. The list must match the
     *                                 list previously passed to the WhoImplements
     *                                 member function
     * @param[in] numberInterfaces the number of interfaces in the
     *               implementsInterfaces list
     * @return
     *    - #ER_OK on success
     *    - An error status otherwise
     */
    QStatus CancelWhoImplementsNonBlocking(const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * Stop showing interest in the listed interfaces. Stop receiving announce
     * signals from the devices with the listed interfaces.
     *
     * This is identical to CancelWhoImplements(const char**, size_t)
     * except this is specialized for a single interface not several interfaces.
     *
     * @see CancelWhoImplements(const char**, size_t)
     * @param[in] iface     interface that the remove user must implement to
     *                          receive the announce signal.
     *
     * @return
     *    - #ER_OK on success
     *    - #ER_BUS_MATCH_RULE_NOT_FOUND if interface added using the WhoImplements
     *                                   member function were not found.
     *    - An error status otherwise
     */
    QStatus CancelWhoImplements(const char* iface);

    /**
     * Non-blocking variant of CancelWhoImplements.
     *
     * @see CancelWhoImplements(const char**, size_t)
     * @param[in] iface     interface that the remove user must implement to
     *                          receive the announce signal.
     * @return
     *    - #ER_OK on success
     *    - An error status otherwise
     */
    QStatus CancelWhoImplementsNonBlocking(const char* iface);

    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Class for internal state for bus attachment.
     */
    class Internal;

    /**
     * @internal
     * Get a pointer to the internal BusAttachment state.
     *
     * @return A pointer to the internal state.
     */
    Internal& GetInternal() { return *busInternal; }

    /**
     * @internal
     * Get a pointer to the internal BusAttachment state.
     *
     * @return A pointer to the internal state.
     */
    const Internal& GetInternal() const { return *busInternal; }
    /// @endcond
  protected:
    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Construct a BusAttachment.
     *
     * @param internal     Internal state.
     * @param concurrency  The maximum number of concurrent method and signal
     *                     handlers locally executing.
     */
    BusAttachment(Internal* internal, uint32_t concurrency);
    /// @endcond

    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Notify AllJoyn that the application is suspending. Exclusively-held
     * resource should be released so that other applications will not be
     * prevented from acquiring the resource.
     *
     * @return
     *      - #ER_OK on success
     *      - #ER_BUS_NOT_CONNECTED if BusAttachment is not connected
     *      - #ER_ALLJOYN_ONAPPSUSPEND_REPLY_FAILED if the app suspend request failed
     *      - #ER_ALLJOYN_ONAPPSUSPEND_REPLY_UNSUPPORTED if app suspend is not supported
     *      - #ER_BUS_UNEXPECTED_DISPOSITION if OnAppSuspend enters an unexpected state
     */
    QStatus OnAppSuspend();
    /// @endcond

    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Notify AllJoyn that the application is resuming so that it can re-acquire
     * the resource that has been released when the application was suspended.
     *
     * @return
     *      - #ER_OK on success
     *      - #ER_BUS_NOT_CONNECTED if BusAttachment is not connected
     *      - #ER_ALLJOYN_ONAPPRESUME_REPLY_FAILED if the app resume request failed
     *      - #ER_ALLJOYN_ONAPPRESUME_REPLY_UNSUPPORTED if app resume is not supported
     *      - #ER_BUS_UNEXPECTED_DISPOSITION if OnAppResume enters an unexpected state
     */
    QStatus OnAppResume();
    /// @endcond

  private:
    typedef uint16_t SessionSideMask;

    typedef enum {
        SESSION_SIDE_HOST = 0,
        SESSION_SIDE_JOINER = 1,
        SESSION_SIDE_NUM
    } SessionSide;

    static const uint8_t SESSION_SIDE_MASK_HOST = (1 << SESSION_SIDE_HOST);
    static const uint8_t SESSION_SIDE_MASK_JOINER = (1 << SESSION_SIDE_JOINER);
    static const uint8_t SESSION_SIDE_MASK_BOTH = (SESSION_SIDE_MASK_HOST | SESSION_SIDE_MASK_JOINER);

    /**
     * Assignment operator is private.
     */
    BusAttachment& operator=(const BusAttachment&);

    /**
     * Copy constructor is private.
     */
    BusAttachment(const BusAttachment&);

    /**
     * Stop the bus, optionally blocking until all of the threads join
     */
    QStatus StopInternal(bool blockUntilStopped = true);

    /**
     * Wait until all of the threads have stopped (join).
     */
    void WaitStopInternal();

    /**
     * Validate the response to SetLinkTimeout
     */
    QStatus GetLinkTimeoutResponse(Message& reply, uint32_t& timeout);

    /**
     * Validate the response to JoinSession
     */
    QStatus GetJoinSessionResponse(Message& reply, SessionId& sessionId, SessionOpts& opts);

    /**
     * Leave the session as host and/or joiner, asynchronous version
     */
    QStatus LeaveSessionAsync(const SessionId& sessionId, const char* method, SessionSideMask bitset, BusAttachment::LeaveSessionAsyncCB* callback, void* context);

    /**
     * Leave the session as host and/or joiner
     */
    QStatus LeaveSession(const SessionId& sessionId, const char*method, SessionSideMask bitset);

    /**
     * Clear session listeners for a particular session
     */
    void ClearSessionListener(SessionId sessionId, SessionSideMask bitset);

    /**
     * Remove references to session
     */
    void ClearSessionSet(SessionId sessionId, SessionSideMask bitset);

    /**
     * Register signal handlers for BusListener
     */
    QStatus RegisterSignalHandlers();

    /**
     * Unregister signal handlers for BusListener
     */
    void UnregisterSignalHandlers();

    qcc::String connectSpec;  /**< The connect spec used to connect to the bus */
    bool isStarted;           /**< Indicates if the bus has been started */
    bool isStopping;          /**< Indicates Stop has been called */
    uint32_t concurrency;     /**< The maximum number of concurrent method and signal handlers locally executing */
    Internal* busInternal;    /**< Internal state information */

    /**
     * Internal Class used to insure that the BusAttachment is the last object
     * destroyed.
     */
    class JoinObj {
      public:
        /**
         * Construct a JoinObj.
         *
         * @param bus the `this` pointer that the internal JoinObj is a member of
         */
        JoinObj(BusAttachment* bus) : bus(bus) { }
        /**
         * Destroy a JoinObj.
         */
        ~JoinObj() {
            bus->WaitStopInternal();
        }
      private:
        /**
         * private copy constructor to ensure the JoinObj is never copied
         *
         * @param other a JoinObj
         */
        JoinObj(const JoinObj& other);
        /**
         * private assignment operator to ensure the JoinObj is never copied
         *
         * @param other a JoinObj
         * @return nothing is returned this is here to prevent the auto
         *         generation of an assignment operator by the compiler.
         */
        JoinObj& operator =(const JoinObj& other);

        /**
         * pointer to the BusAttachment containing the JoinObj
         */
        BusAttachment* bus;
    };

    Translator* translator;       /**< Global translator for descriptions */

    JoinObj joinObj;          /**< MUST BE LAST MEMBER. Ensure all threads are joined before BusAttachment destruction */

};

}

#endif
