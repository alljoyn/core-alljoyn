/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#pragma once

#include <alljoyn/DBusStd.h>
#include <alljoyn/Session.h>
#include <alljoyn/BusAttachment.h>
#include <TransportMaskType.h>
#include <InterfaceDescription.h>
#include <SessionOpts.h>
#include <SocketStream.h>
#include <qcc/ManagedObj.h>
#include <qcc/Mutex.h>
#include <Status_CPP0x.h>
#include <map>

namespace AllJoyn {

ref class InterfaceMember;
ref class MessageReceiver;
ref class BusObject;
ref class BusListener;
ref class SessionListener;
ref class SessionPortListener;
ref class SessionOpts;
ref class ProxyBusObject;
ref class AuthListener;
ref class KeyStoreListener;

public enum class RequestNameType {
    /// <summary>
    ///Allow others to take ownership of this name
    /// </summary>
    DBUS_NAME_ALLOW_REPLACEMENT = DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
    /// <summary>
    ///Attempt to take ownership of name if already taken
    /// </summary>
    DBUS_NAME_REPLACE_EXISTING = DBUS_NAME_FLAG_REPLACE_EXISTING,
    /// <summary>
    ///Fail if name cannot be immediately obtained
    /// </summary>
    DBUS_NAME_DO_NOT_QUEUE = DBUS_NAME_FLAG_DO_NOT_QUEUE
};

/// <summary>
///The result of the asynchronous operation for joining a session
/// </summary>
public ref class JoinSessionResult sealed {
  public:
    /// <summary>
    ///The BusAttachment object that makes the call.
    /// </summary>
    property BusAttachment ^ Bus;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///Optional listener called when session related events occur. May be NULL.
    /// </summary>
    property SessionListener ^ Listener;
    /// <summary>
    ///Result of the operation. The value is ER_OK is the session is joined successfully.
    /// </summary>
    property QStatus Status;
    /// <summary>
    ///The session id of the session joined.
    /// </summary>
    property ajn::SessionId SessionId;
    /// <summary>
    ///Session options imposed by the session creator.
    /// </summary>
    property SessionOpts ^ Opts;

  private:
    friend ref class BusAttachment;
    friend class _BusAttachment;
    JoinSessionResult(BusAttachment ^ bus, SessionListener ^ listener, Platform::Object ^ context)
    {
        Bus = bus;
        Context = context;
        Listener = listener;
        Status = QStatus::ER_OK;
        SessionId = (ajn::SessionId)-1;
        Opts = nullptr;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~JoinSessionResult()
    {
        Bus = nullptr;
        Context = nullptr;
        Listener = nullptr;
        Opts = nullptr;
        _exception = nullptr;
        if (NULL != _stdException) {
            delete _stdException;
            _stdException = NULL;
        }
    }

    void Wait()
    {
        qcc::Event::Wait(_event, qcc::Event::WAIT_FOREVER);
        // Propagate exception state
        if (nullptr != _exception) {
            throw _exception;
        }
        if (NULL != _stdException) {
            throw _stdException;
        }
    }

    void Complete()
    {
        _event.SetEvent();
    }

    Platform::Exception ^ _exception;
    std::exception* _stdException;
    qcc::Event _event;
};

/// <summary>
///The result of the asynchronous operation for setting the link idle timeout
/// </summary>
public ref class SetLinkTimeoutResult sealed {
  public:
    /// <summary>
    ///The BusAttachment object that makes the call
    /// </summary>
    property BusAttachment ^ Bus;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///Result of the operation. The value is ER_OK if the link timeout is set successfully.
    /// </summary>
    property QStatus Status;
    /// <summary>
    ///The actual link idle timeout value.
    /// </summary>
    property uint32_t Timeout;

  private:
    friend ref class BusAttachment;
    friend class _BusAttachment;
    SetLinkTimeoutResult(BusAttachment ^ bus, Platform::Object ^ context)
    {
        Bus = bus;
        Context = context;
        Status = QStatus::ER_OK;
        Timeout = (uint32_t)-1;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~SetLinkTimeoutResult()
    {
        Bus = nullptr;
        Context = nullptr;
        _exception = nullptr;
        if (NULL != _stdException) {
            delete _stdException;
            _stdException = NULL;
        }
    }

    void Wait()
    {
        qcc::Event::Wait(_event, qcc::Event::WAIT_FOREVER);
        // Propagate exception state
        if (nullptr != _exception) {
            throw _exception;
        }
        if (NULL != _stdException) {
            throw _stdException;
        }
    }

    void Complete()
    {
        _event.SetEvent();
    }

    Platform::Exception ^ _exception;
    std::exception* _stdException;
    qcc::Event _event;
};

ref class __BusAttachment {
  private:
    friend ref class BusAttachment;
    friend class _BusAttachment;
    __BusAttachment();
    ~__BusAttachment();

    property ProxyBusObject ^ DBusProxyBusObject;
    property ProxyBusObject ^ AllJoynProxyBusObject;
    property ProxyBusObject ^ AllJoynDebugProxyBusObject;
    property Platform::String ^ UniqueName;
    property Platform::String ^ GlobalGUIDString;
    property uint32_t Timestamp;
};

class _BusAttachment : protected ajn::BusAttachment, protected ajn::BusAttachment::JoinSessionAsyncCB, protected ajn::BusAttachment::SetLinkTimeoutAsyncCB {
  protected:
    friend class qcc::ManagedObj<_BusAttachment>;
    friend ref class BusAttachment;
    friend ref class BusObject;
    friend ref class ProxyBusObject;
    friend class _BusListener;
    friend class _AuthListener;
    friend class _BusObject;
    friend class _KeyStoreListener;
    friend class _MessageReceiver;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    friend class _SessionListener;
    friend class _SessionPortListener;
    _BusAttachment(const char* applicationName, bool allowRemoteMessages, uint32_t concurrency);
    ~_BusAttachment();

    void JoinSessionCB(::QStatus s, ajn::SessionId sessionId, const ajn::SessionOpts& opts, void* context);
    void SetLinkTimeoutCB(::QStatus s, uint32_t timeout, void* context);
    void DispatchCallback(Windows::UI::Core::DispatchedHandler ^ callback);
    bool IsOriginSTA();

    __BusAttachment ^ _eventsAndProperties;
    KeyStoreListener ^ _keyStoreListener;
    AuthListener ^ _authListener;
    Windows::UI::Core::CoreDispatcher ^ _dispatcher;
    bool _originSTA;
    std::map<void*, void*> _busObjectMap;
    std::map<void*, void*> _signalHandlerMap;
    std::map<void*, void*> _busListenerMap;
    std::map<ajn::SessionPort, std::map<void*, void*>*> _sessionPortListenerMap;
    std::map<ajn::SessionId, std::map<void*, void*>*> _sessionListenerMap;
    qcc::Mutex _mutex;
};

/// <summary>
/// BusAttachment is the top-level object responsible for connecting to and optionally managing a message bus
/// </summary>
public ref class BusAttachment sealed {
  public:

    /// <summary>
    /// Construct a BusAttachment.
    /// </summary>
    /// <param name="applicationName">Name of the application.</param>
    /// <param name="allowRemoteMessages">True if this attachment is allowed to receive messages from remote devices.</param>
    /// <param name="concurrency">The maximum number of concurrent method and signal handlers locally executing. This value isn't enforced and only provided for API completeness.</param>
    BusAttachment(Platform::String ^ applicationName, bool allowRemoteMessages, uint32_t concurrency);

    /// <summary>
    /// Get the concurrent method and signal handler limit.
    /// </summary>
    /// <returns>The maximum number of concurrent method and signal handlers.</returns>
    uint32_t GetConcurrency();

    /// <summary>
    /// Allow the currently executing method/signal handler to enable concurrent callbacks
    /// during the scope of the handler's execution.
    /// </summary>
    void EnableConcurrentCallbacks();

    /// <summary>
    /// Create an interface description with a given name.
    /// </summary>
    /// <remarks>
    /// Typically, interfaces that are implemented by BusObjects are created here.
    /// Interfaces that are implemented by remote objects are added automatically by
    /// the bus if they are not already present via ProxyBusObject::IntrospectRemoteObject().
    /// Because interfaces are added both explicitly (via this method) and implicitly
    /// (via ProxyBusObject::IntrospectRemoteObject), there is the possibility that creating
    /// an interface here will fail because the interface already exists. If this happens, and
    /// exception with be thrown and the HRESULT values with be #ER_BUS_IFACE_ALREADY_EXISTS.
    /// Interfaces created with this method need to be activated using InterfaceDescription::Activate()
    /// once all of the methods, signals, etc have been added to the interface. The interface will
    /// be unaccessible (via BusAttachment::GetInterfaces() or BusAttachment::GetInterface()) until
    /// it is activated.
    /// </remarks>
    /// <param name="name">The requested interface name.</param>
    /// <param name="iface">Returns the newly created interface description.</param>
    /// <param name="secure">If true the interface is secure and method calls and signals will be encrypted.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT == #ER_BUS_IFACE_ALREADY_EXISTS if requested interface already exists
    /// </exception>
    /// <see cref="ProxyBusObject::IntrospectRemoteObject"/>
    /// <see cref="InterfaceDescription::Activate"/>
    /// <see cref="BusAttachment::GetInterface"/>
    void CreateInterface(Platform::String ^ name, Platform::WriteOnlyArray<InterfaceDescription ^> ^ iface, bool secure);

    /// <summary>
    /// Create an interface description from XML
    /// </summary>
    /// <remarks>
    /// Initialize one more interface descriptions from an XML string in DBus introspection format.
    /// The root tag of the XML can be a node or a stand alone interface tag. To initialize more
    /// than one interface the interfaces need to be nested in a node tag.
    ///
    /// Note that when this method fails during parsing, an exception will be thrown.
    /// However, any interfaces which were successfully parsed prior to the failure may be registered
    /// with the bus.
    /// </remarks>
    /// <param name="xml">An XML string in DBus introspection format</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void CreateInterfacesFromXml(Platform::String ^ xml);

    /// <summary>
    /// Returns the existing activated InterfaceDescriptions.
    /// </summary>
    /// <param name="ifaces">A pointer to an InterfaceDescription array to receive the interfaces. Can be NULL in
    /// which case no interfaces are returned and the return value gives the number of interface available.
    /// </param>
    /// <returns>The number of interfaces returned or the total number of interfaces if ifaces is NULL.</returns>
    uint32_t GetInterfaces(Platform::WriteOnlyArray<InterfaceDescription ^> ^ ifaces);

    /// <summary>
    /// Retrieve an existing activated InterfaceDescription.
    /// </summary>
    /// <param name="name">Interface name</param>
    /// <returns>A reference to the registered interface</returns>
    InterfaceDescription ^ GetInterface(Platform::String ^ name);

    /// <summary>
    /// Delete an interface description with a given name.
    /// </summary>
    /// <remarks>
    /// Deleting an interface is only allowed if that interface has never been activated.
    /// </remarks>
    /// <param name="iface">The unactivated interface to be deleted.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NO_SUCH_INTERFACE if the interface was not found
    /// </exception>
    void DeleteInterface(InterfaceDescription ^ iface);

    /// <summary>
    /// Start the process of spinning up the independent threads used in
    /// the bus attachment, preparing it for use.
    /// </summary>
    /// <remarks>
    /// This method only begins the process of starting the bus. Sending and
    /// receiving messages cannot begin until the bus is Connect()ed.
    /// In most cases, it is not required to understand the threading model of
    /// the bus attachment, with one important exception: The bus attachment may
    /// send callbacks to registered listeners using its own internal threads.
    /// This means that any time a listener of any kind is used in a program, the
    /// implication is that a the overall program is multithreaded, irrespective
    /// of whether or not threads are explicitly used.  This, in turn, means that
    /// any time shared state is accessed in listener methods, that state must be
    /// protected.
    /// As soon as Start() is called, clients of a bus attachment with listeners
    /// must be prepared to receive callbacks on those listeners in the context
    /// of a thread that will be different from the thread running the main
    /// program or any other thread in the client.
    /// Although intimate knowledge of the details of the threading model are not
    /// required to use a bus attachment (beyond the caveat above) we do provide
    /// methods on the bus attachment that help users reason about more complex
    /// threading situations.  This will apply to situations where clients of the
    /// bus attachment are multithreaded and need to interact with the
    /// multithreaded bus attachment.  These methods can be especially useful
    /// during shutdown, when the two separate threading systems need to be
    /// gracefully brought down together.
    /// The BusAttachment methods Start(), StopAsync() and JoinSessionAsync() all work together to
    /// manage the autonomous activities that can happen in a BusAttachment.
    /// These activities are carried out by so-called hardware threads.  POSIX
    /// defines functions used to control hardware threads, which it calls
    /// pthreads.  Many threading packages use similar constructs.
    /// In a threading package, a start method asks the underlying system to
    /// arrange for the start of thread execution.  Threads are not necessarily
    /// running when the start method returns, but they are being *started*.  Some time later,
    /// a thread of execution appears in a thread run function, at which point the
    /// thread is considered *running*.  At some later time, executing a stop method asks the
    /// underlying system to arrange for a thread to end its execution.  The system
    /// typically sends a message to the thread to ask it to stop doing what it is doing.
    /// The thread is running until it responds to the stop message, at which time the
    /// run method exits and the thread is considered *stopping*.
    /// Note that neither of Start() nor Stop() are synchronous in the sense that
    /// one has actually accomplished the desired effect upon the return from a
    /// call.  Of particular interest is the fact that after a call to Stop(),
    /// threads will still be *running* for some non-deterministic time.
    /// In order to wait until all of the threads have actually stopped, a
    /// blocking call is required.  In threading packages this is typically
    /// called join, and our corresponding method is called JoinSessionAsync().
    /// A Start() method call should be thought of as mapping to a threading
    /// package start function.  it causes the activity threads in the
    /// BusAttachment to be spun up and gets the attachment ready to do its main
    /// job.  As soon as Start() is called, the user should be prepared for one
    /// or more of these threads of execution to pop out of the bus attachment
    /// and into a listener callback.
    /// The Stop() method call should be thought of as mapping to a threading
    /// package stop function.  It asks the BusAttachment to begin shutting down
    /// its various threads of execution, but does not wait for any threads to exit.
    /// A call to the JoinSessionAsync() method should be thought of as mapping to a
    /// threading package join function call.  It blocks and waits until all of
    /// the threads in the BusAttachment have in fact exited their Run functions,
    /// gone through the stopping state and have returned their status.  When
    /// the JoinAsync() method returns, one may be assured that no threads are running
    /// in the bus attachment, and therefore there will be no callbacks in
    /// progress and no further callbacks will ever come out of a particular
    /// instance of a bus attachment.
    /// It is important to understand that since Start(), StopAsync() and JoinSessionAsync() map
    /// to threads concepts and functions, one should not expect them to clean up
    /// any bus attachment state when they are called.  These functions are only
    /// present to help in orderly termination of complex threading systems.
    /// <see cref="BusAttachment::StopAsync"/>
    /// <see cref="BusAttachment::JoinSessionAsync"/>
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_BUS_ALREADY_STARTED if already started.
    /// Other error status codes indicating a failure.
    /// </exception>
    void Start();

    /// <summary>
    /// Ask the threading subsystem in the bus attachment to begin the
    /// process of ending the execution of its threads.
    /// </summary>
    /// <remarks>
    /// The StopAsync() method call on a bus attachment should be thought of as
    /// mapping to a threading package stop function.
    /// <see cref="BusAttachment::Start"/>
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error if
    /// unable to begin the process of stopping the BusAttachment threads.
    /// </exception>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ StopAsync();

    /// <summary>
    /// Determine if the bus attachment has been started by a call to <see cref="Start"/>.
    /// </summary>
    /// <remarks>
    /// For more information see:
    /// <see cref="BusAttachment::Start"/>
    /// <see cref="BusAttachment::StopAsync"/>
    /// </remarks>
    /// <returns>True if the message bus has been started by a call to <see cref="Start"/>.</returns>
    bool IsStarted();

    /// <summary>
    /// Determine if the bus attachment has been stopped by a call to <see cref="StopAsync"/>.
    /// </summary>
    /// <remarks>
    /// For more information see:
    /// <see cref="BusAttachment::Start"/>
    /// <see cref="BusAttachment::StopAsync"/>
    /// </remarks>
    /// <returns>True if the message bus has been started by a call to <see cref="Start"/>.</returns>
    bool IsStopping();

    /// <summary>
    /// Connect to a remote bus address.
    /// </summary>
    /// <param name="connectSpec">The transport connection spec used to connect.</param>
    /// <exception cref="Platform::COMException">
    /// Upon completion, Platform::COMException will be raised and a HRESULT will contain the AllJoyn
    /// error status code if any error occurred.
    /// </exception>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ ConnectAsync(Platform::String ^ connectSpec);

    /// <summary>
    /// Disconnect a remote bus address connection.
    /// </summary>
    /// <param name="connectSpec">The transport connection spec used to connect.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_BUS_NOT_STARTED if the bus is not started
    /// #ER_BUS_NOT_CONNECTED if the BusAttachment is not connected to the bus
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    Windows::Foundation::IAsyncAction ^ DisconnectAsync(Platform::String ^ connectSpec);

    /// <summary>
    /// Indicate whether bus is currently connected.
    /// </summary>
    /// <remarks>
    /// Messages can only be sent or received when the bus is connected.
    /// </remarks>
    /// <returns>True if the bus is connected.</returns>
    bool IsConnected();

    /// <summary>
    /// Register a BusObject
    /// </summary>
    /// <param name="obj">BusObject to register.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_BAD_OBJ_PATH for a bad object path
    /// </exception>
    void RegisterBusObject(BusObject ^ obj);

    /// <summary>
    /// Unregister a BusObject
    /// </summary>
    /// <param name="object">Object to be unregistered.</param>
    void UnregisterBusObject(BusObject ^ object);

    /// <summary>
    /// Register a signal handler.
    /// </summary>
    /// <remarks>
    /// Signals are forwarded to the signalHandler if sender, interface, member and path
    /// qualifiers are ALL met.
    /// </remarks>
    /// <param name="receiver">The object receiving the signal.</param>
    /// <param name="member">The interface/member of the signal.</param>
    /// <param name="srcPath">The object path of the emitter of the signal or NULL for all paths.</param>
    void RegisterSignalHandler(MessageReceiver ^ receiver,
                               InterfaceMember ^ member,
                               Platform::String ^ srcPath);

    /// <summary>
    /// Unregister a signal handler.
    /// </summary>
    /// <remarks>
    /// Remove the signal handler that was registered with the given parameters.
    /// </remarks>
    /// <param name="receiver">The object receiving the signal.</param>
    /// <param name="member">The interface/member of the signal.</param>
    /// <param name="srcPath">The object path of the emitter of the signal or NULL for all paths.</param>
    void UnregisterSignalHandler(MessageReceiver ^ receiver,
                                 InterfaceMember ^ member,
                                 Platform::String ^ srcPath);

    /// <summary>
    /// Enable peer-to-peer security.
    /// </summary>
    /// <remarks>
    /// This function must be called by applications that want to use
    /// authentication and encryption . The bus must have been started by calling
    /// BusAttachment::Start() before this function is called. If the application is providing its
    /// own key store implementation it must have already called RegisterKeyStoreListener() before
    /// calling this function.
    /// </remarks>
    /// <param name="authMechanisms">The authentication mechanism(s) to use for peer-to-peer authentication.
    ///                         If this parameter is NULL peer-to-peer authentication is disabled.
    /// </param>
    /// <param name="listener">Passes password and other authentication related requests to the application.</param>
    /// <param name="keyStoreFileName">Optional parameter to specify the filename of the default key store. The
    ///                         default value is the applicationName parameter of BusAttachment().
    ///                         Note that this parameter is only meaningful when using the default
    ///                         key store implementation.
    /// </param>
    /// <param name="isShared">optional parameter that indicates if the key store is shared between multiple
    ///                         applications. It is generally harmless to set this to true even when the
    ///                         key store is not shared but it adds some unnecessary calls to the key store
    ///                         listener to load and store the key store in this case.
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_BUS_NOT_STARTED BusAttachment::Start has not be called
    /// </exception>
    void EnablePeerSecurity(Platform::String ^ authMechanisms,
                            AuthListener ^ listener,
                            Platform::String ^ keyStoreFileName,
                            bool isShared);

    /// <summary>
    /// Disable peer-to-peer security.
    /// </summary>
    /// <param name="listener">The listener that was previously registered by
    /// an earlier call to <see cref="BusAttachment::EnablePeerSecurity"/>
    /// </param>
    void DisablePeerSecurity(AuthListener ^ listener);

    /// <summary>
    /// Check is peer security has been enabled for this bus attachment.
    /// </summary>
    /// <returns>Returns true if peer security has been enabled, false otherwise.</returns>
    bool IsPeerSecurityEnabled();

    /// <summary>
    /// Register an object that will receive bus event notifications.
    /// </summary>
    /// <param name="listener">Object instance that will receive bus event notifications.</param>
    void RegisterBusListener(BusListener ^ listener);

    /// <summary>
    /// Unregister an object that was previously registered with RegisterBusListener.
    /// </summary>
    /// <param name="listener">Object instance to un-register as a listener.</param>
    void UnregisterBusListener(BusListener ^ listener);

    /// <summary>
    /// Set a key store listener to listen for key store load and store requests.
    /// </summary>
    /// <remarks>
    /// This overrides the internal key store listener.
    /// </remarks>
    /// <param name="listener">The key store listener to set.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_LISTENER_ALREADY_SET if a listener has been set by this function or because
    /// EnablePeerSecurity has previously been called.
    /// </exception>
    void RegisterKeyStoreListener(KeyStoreListener ^ listener);

    /// <summary>
    /// Set a key store listener to listen for key store load and store requests.
    /// </summary>
    /// <remarks>
    /// This overrides the internal key store listener.
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void UnregisterKeyStoreListener();

    /// <summary>
    /// Reloads the key store for this bus attachment.
    /// </summary>
    /// <remarks>
    /// This function would normally only be called in
    /// the case where a single key store is shared between multiple bus attachments, possibly by different
    /// applications. It is up to the applications to coordinate how and when the shared key store is
    /// modified.
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status indicating why the key store reload failed.
    /// </exception>
    void ReloadKeyStore();

    /// <summary>
    /// Clears all stored keys from the key store.
    /// </summary>
    /// <remarks>
    /// All store keys and authentication information is
    /// deleted and cannot be recovered. Any passwords or other credentials will need to be reentered
    /// when establishing secure peer connections.
    /// </remarks>
    void ClearKeyStore();

    /// <summary>
    /// Clear the keys associated with a specific remote peer as identified by its peer GUID.
    /// </summary>
    /// <remarks>
    /// The peer GUID associated with a bus name can be obtained by calling GetPeerGUID().
    /// </remarks>
    /// <param name="guid">The GUID of a remote authenticated peer.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_KEY_UNAVAILABLE if there is no peer with the specified GUID.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void ClearKeys(Platform::String ^ guid);

    /// <summary>
    /// Set the expiration time on keys associated with a specific remote peer as identified by its peer GUID.
    /// </summary>
    /// <remarks>
    /// The peer GUID associated with a bus name can be obtained by calling GetPeerGUID().
    /// If the timeout is 0 this is equivalent to calling ClearKeys().
    /// </remarks>
    /// <param name="guid">The GUID of a remote authenticated peer.</param>
    /// <param name="timeout">The time in seconds relative to the current time to expire the keys.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_KEY_UNAVAILABLE if there is no authenticated peer with the specified GUID.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void SetKeyExpiration(Platform::String ^ guid, uint32_t timeout);

    /// <summary>
    /// Get the expiration time on keys associated with a specific authenticated remote peer as
    /// identified by its peer GUID.
    /// </summary>
    /// <remarks>
    /// The peer GUID associated with a bus name can be obtained by
    /// calling GetPeerGUID().
    /// </remarks>
    /// <param name="guid">The GUID of a remote authenticated peer.</param>
    /// <param name="timeout">The time in seconds relative to the current time when the keys will expire.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_KEY_UNAVAILABLE if there is no authenticated peer with the specified GUID.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void GetKeyExpiration(Platform::String ^ guid, Platform::WriteOnlyArray<uint32> ^ timeout);

    /// <summary>
    /// Adds a logon entry string for the requested authentication mechanism to the key store
    /// </summary>
    /// <remarks>
    /// This allows an authenticating server to generate offline authentication credentials for securely
    /// logging on a remote peer using a user-name and password credentials pair. This only applies
    /// to authentication mechanisms that support a user name + password logon functionality.
    /// </remarks>
    /// <param name="authMechanism">The authentication mechanism.</param>
    /// <param name="userName">The user name to use for generating the logon entry.</param>
    /// <param name="password">The password to use for generating the logon entry. If the password is
    /// NULL the logon entry is deleted from the key store.
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_INVALID_AUTH_MECHANISM if the authentication mechanism does not support logon functionality.
    /// #ER_BAD_ARG_2 indicates a null string was used as the user name.
    /// #ER_BAD_ARG_3 indicates a null string was used as the password.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void AddLogonEntry(Platform::String ^ authMechanism, Platform::String ^ userName, Platform::String ^ password);

    /// <summary>
    /// Request a well-known name.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.freedesktop.DBus.RequestName method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="requestedName">Well-known name being requested.</param>
    /// <param name="flags">Bitmask of DBUS_NAME_FLAG_* defines see <see cref="AllJoyn::DBusStd"/></param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void RequestName(Platform::String ^ requestedName, uint32_t flags);

    /// <summary>
    /// Release a previously requested well-known name.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.freedesktop.DBus.ReleaseName method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="name">Well-known name being released.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void ReleaseName(Platform::String ^ name);

    /// <summary>
    /// Add a DBus match rule.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch method call to the local daemon.
    /// </remarks>
    /// <param name="rule">Match rule to be added (see DBus specification for format of this string).</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void AddMatch(Platform::String ^ rule);

    /// <summary>
    /// Remove a DBus match rule.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch method call to the local daemon.
    /// </remarks>
    /// <param name="rule">Match rule to be removed (see DBus specification for format of this string).</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void RemoveMatch(Platform::String ^ rule);

    /// <summary>
    /// Advertise the existence of a well-known name to other (possibly disconnected) AllJoyn daemons.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.AdvertisedName method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="name">the well-known name to advertise. (Must be owned by the caller via RequestName).</param>
    /// <param name="transports">Set of transports to use for sending advertisement.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void AdvertiseName(Platform::String ^ name, TransportMaskType transports);

    /// <summary>
    /// Stop advertising the existence of a well-known name to other AllJoyn daemons.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.CancelAdvertiseName method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="name">A well-known name that was previously advertised via AdvertiseName.</param>
    /// <param name="transports">Set of transports whose name advertisement will be canceled.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void CancelAdvertiseName(Platform::String ^ name, TransportMaskType transports);

    /// <summary>
    /// Register interest in a well-known name prefix for the purpose of discovery over transports included in TRANSPORT_ANY.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="namePrefix">Well-known name prefix that application is interested in receiving
    /// BusListener::FoundAdvertisedName notifications about.
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void FindAdvertisedName(Platform::String ^ namePrefix);

    /// <summary>
    /// Register interest in a well-known name prefix for the purpose of discovery over over a set of transports.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="namePrefix">Well-known name prefix that application is interested in receiving
    /// BusListener::FoundAdvertisedName notifications about.
    /// </param>
    /// <param name="transports">Set of transports who will do well-known name discovery.
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void FindAdvertisedNameByTransport(Platform::String ^ namePrefix, TransportMaskType transports);

    /// <summary>
    /// Cancel interest in a well-known name prefix that was previously registered with FindAdvertisedName over transports included in TRANSPORT_ANY.
    /// </summary>
    /// <remarks>
    /// This method is equivalent to CancelFindAdvertisedName(namePrefix, TRANSPORT_ANY).
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
    /// call to the local daemon and interprets the response.
    /// </remarks>
    /// <param name="namePrefix">Well-known name prefix that application is no longer interested in receiving
    /// BusListener::FoundAdvertisedName notifications about.
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void CancelFindAdvertisedName(Platform::String ^ namePrefix);

    /// <summary>
    /// Cancel interest in a well-known name prefix that was previously registered with FindAdvertisedName over a set of transports.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
    /// call to the local daemon and interprets the response.
    /// </remarks>
    /// <param name="namePrefix">Well-known name prefix that application is no longer interested in receiving
    /// BusListener::FoundAdvertisedName notifications about.
    /// </param>
    /// <param name="transports">Set of transports who will cancel well-known name discovery.
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void CancelFindAdvertisedNameByTransport(Platform::String ^ namePrefix, TransportMaskType transports);

    /// <summary>
    /// Make a SessionPort available for external BusAttachments to join.
    /// </summary>
    /// <remarks>
    /// Each BusAttachment binds its own set of SessionPorts. Session joiners use the bound session
    /// port along with the name of the attachment to create a persistent logical connection (called
    /// a Session) with the original BusAttachment.
    /// A SessionPort and bus name form a unique identifier that BusAttachments use when joining a
    /// session.
    /// SessionPort values can be pre-arranged between AllJoyn services and their clients (well-known
    /// SessionPorts).
    /// Once a session is joined using one of the service's well-known SessionPorts, the service may
    /// bind additional SessionPorts (dynamically) and share these SessionPorts with the joiner over
    /// the original session. The joiner can then create additional sessions with the service by
    /// calling JoinSession with these dynamic SessionPort ids.
    /// </remarks>
    /// <param name="sessionPort_in">SessionPort value to bind or SESSION_PORT_ANY to allow this method
    /// to choose an available port.
    /// </param>
    /// <param name="sessionPort_out">On successful return, this value contains the chosen SessionPort.
    /// </param>
    /// <param name="opts">Session options that joiners must agree to in order to successfully join the session.</param>
    /// <param name="listener">Called by the bus when session related events occur.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void BindSessionPort(ajn::SessionPort sessionPort_in, Platform::WriteOnlyArray<ajn::SessionPort> ^ sessionPort_out, SessionOpts ^ opts, SessionPortListener ^ listener);

    /// <summary>
    /// Cancel an existing port binding.
    /// </summary>
    /// <param name="sessionPort">Existing session port to be un-bound.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void UnbindSessionPort(ajn::SessionPort sessionPort);

    /// <summary>
    /// Join a session.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to the local daemon
    /// and interprets the response.
    /// This call executes asynchronously. When the JoinSession response is received, the callback will be called.
    /// </remarks>
    /// <param name="sessionHost">Bus name of attachment that is hosting the session to be joined.</param>
    /// <param name="sessionPort">SessionPort of sessionHost to be joined.</param>
    /// <param name="listener">Optional listener called when session related events occur. May be NULL.</param>
    /// <param name="opts_in">Session options.</param>
    /// <param name="opts_out">Session options.</param>
    /// <param name="context">User defined context which will be passed as-is to callback.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<JoinSessionResult ^> ^ JoinSessionAsync(Platform::String ^ sessionHost,
                                                                                 ajn::SessionPort sessionPort,
                                                                                 SessionListener ^ listener,
                                                                                 SessionOpts ^ opts_in,
                                                                                 Platform::WriteOnlyArray<SessionOpts ^> ^ opts_out,
                                                                                 Platform::Object ^ context);

    /// <summary>
    /// Set the SessionListener for an existing sessionId.
    /// </summary>
    /// <remarks>
    /// Calling this method will override the listener set by a previous call to SetSessionListener or any
    /// listener specified in JoinSessionAsync.
    /// </remarks>
    /// <param name="sessionId">The session id of an existing session.</param>
    /// <param name="listener">The SessionListener to associate with the session. May be NULL to clear previous listener.</param>
    void SetSessionListener(ajn::SessionId sessionId, SessionListener ^ listener);

    /// <summary>
    /// Leave an existing session.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call to the local daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="sessionId">Session id.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    void LeaveSession(ajn::SessionId sessionId);

    /// <summary>
    /// Get the file descriptor for a raw (non-message based) session.
    /// </summary>
    /// <param name="sessionId">Id of an existing streaming session.</param>
    /// <param name="socketStream">Socket stream for the session.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void GetSessionSocketStream(ajn::SessionId sessionId, Platform::WriteOnlyArray<AllJoyn::SocketStream ^> ^ socketStream);

    /// <summary>
    /// Set the link timeout for a session.
    /// </summary>
    /// <remarks>
    /// Link timeout is the maximum number of seconds that an unresponsive daemon-to-daemon connection
    /// will be monitored before declaring the session lost (via SessionLost callback). Link timeout
    /// defaults to 0 which indicates that AllJoyn link monitoring is disabled.
    /// Each transport type defines a lower bound on link timeout to avoid defeating transport
    /// specific power management algorithms.
    /// </remarks>
    /// <param name="sessionid">Id of session whose link timeout will be modified.</param>
    /// <param name="linkTimeout">Max number of seconds that a link can be unresponsive before being
    /// declared lost. 0 indicates that AllJoyn link monitoring will be disabled.
    /// </param>
    /// <param name="context">User defined context which will be passed as-is to callback.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// #ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED if local daemon does not support SetLinkTimeout
    /// #ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT if SetLinkTimeout not supported by destination
    /// #ER_BUS_NO_SESSION if the Session id is not valid
    /// #ER_ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED if SetLinkTimeout failed
    /// #ER_BUS_NOT_CONNECTED if the BusAttachment is not connected to the daemon
    /// Or other error status codes indicating the reason the operation failed.
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<SetLinkTimeoutResult ^> ^ SetLinkTimeoutAsync(ajn::SessionId sessionid,
                                                                                       uint32 linkTimeout,
                                                                                       Platform::Object ^ context);

    /// <summary>
    /// Determine whether a given well-known name exists on the bus.
    /// </summary>
    /// <remarks>
    /// This method is a shortcut/helper that issues an org.freedesktop.DBus.NameHasOwner method call to the daemon
    /// and interprets the response.
    /// </remarks>
    /// <param name="name">The well known name that the caller is inquiring about.</param>
    /// <param name="hasOwner">Indicates whether name exists on the bus.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void NameHasOwner(Platform::String ^ name, Platform::WriteOnlyArray<bool> ^ hasOwner);

    /// <summary>
    /// Get the peer GUID for this peer of the local peer or an authenticated remote peer.
    /// </summary>
    /// <remarks>
    /// The bus names of a remote peer can change over time, specifically the unique name is different each
    /// time the peer connects to the bus and a peer may use different well-known-names at different
    /// times. The peer GUID is the only persistent identity for a peer. Peer GUIDs are used by the
    /// authentication mechanisms to uniquely and identify a remote application instance. The peer
    /// GUID for a remote peer is only available if the remote peer has been authenticated.
    /// </remarks>
    /// <param name="name">Name of a remote peer or NULL to get the local (this application's) peer GUID.</param>
    /// <param name="guid">Returns the GUID for the local or remote peer depending on the value of name.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void GetPeerGUID(Platform::String ^ name, Platform::WriteOnlyArray<Platform::String ^> ^ guid);

    /// <summary>
    /// Compares two BusAttachment references
    /// </summary>
    /// <param name="other">The BusAttachment reference to compare.</param>
    /// <returns>Returns true if this bus and the other bus are the same object.</returns>
    bool IsSameBusAttachment(BusAttachment ^ other);

    /// <summary>
    /// Notify AllJoyn that the application is suspending. Exclusively-held resource should be released so that other applications
    /// will not be prevented from acquiring the resource.
    /// </summary>
    /// <remarks>
    /// On WinRT, an application is suspended when it becomes invisible for 10 seconds. The application should register the event when
    /// the application goes into suspending state, and call OnAppSuspend()in the event handler.
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void OnAppSuspend();

    /// <summary>
    /// Notify AllJoyn that the application is resuming so that it can re-acquire the resource that has been released when the application was suspended.
    /// </summary>
    /// <remarks>
    /// On WinRT, an application is suspended when it becomes invisible for 10 seconds. And it is resumed when users switch it back.
    /// The application should register the event when the application goes into resuming state, and call OnAppResume()in the event handler.
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// </exception>
    void OnAppResume();
    /// <summary>
    /// Get a reference to the org.freedesktop.DBus proxy object.
    /// </summary>
    property ProxyBusObject ^ DBusProxyBusObject
    {
        ProxyBusObject ^ get();
    }

    /// <summary>
    /// Get a reference to the org.alljoyn.Bus proxy object.
    /// </summary>
    property ProxyBusObject ^ AllJoynProxyBusObject
    {
        ProxyBusObject ^ get();
    }

    /// <summary>
    /// Get a reference to the org.alljoyn.Debug proxy object.
    /// </summary>
    property ProxyBusObject ^ AllJoynDebugProxyBusObject
    {
        ProxyBusObject ^ get();
    }

    /// <summary>
    /// Get the unique name property of this BusAttachment.
    /// </summary>
    property Platform::String ^ UniqueName
    {
        Platform::String ^ get();
    }

    /// <summary>
    /// Get the GUID property of this BusAttachment.
    /// </summary>
    /// <remarks>
    /// The returned value may be appended to an advertised well-known name in order to guarantee
    /// that the resulting name is globally unique.
    /// </remarks>
    property Platform::String ^ GlobalGUIDString
    {
        Platform::String ^ get();
    }

    /// <summary>
    /// Returns the current non-absolute millisecond real-time clock used internally by AllJoyn.
    /// </summary>
    /// <remarks>
    /// This value can be compared with the timestamps on messages to calculate the time since a time stamped message was sent.
    /// </remarks>
    property uint32_t Timestamp
    {
        uint32_t get();
    }

  private:
    friend ref class BusObject;
    friend ref class ProxyBusObject;
    friend class _BusListener;
    friend class _AuthListener;
    friend class _BusObject;
    friend class _KeyStoreListener;
    friend class _MessageReceiver;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    friend class _SessionListener;
    friend class _SessionPortListener;
    BusAttachment(const ajn::BusAttachment * busAttachment);
    BusAttachment(const qcc::ManagedObj<_BusAttachment>* busAttachment);
    ~BusAttachment();

    qcc::ManagedObj<_BusAttachment>* _mBusAttachment;
    _BusAttachment* _busAttachment;
};

}
// BusAttachment.h
