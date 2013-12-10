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

#include <alljoyn/Session.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <qcc/ManagedObj.h>
#include "MessageReceiver.h"
#include <qcc/Mutex.h>
#include <qcc/Event.h>
#include <map>

namespace AllJoyn {

ref class BusAttachment;
ref class ProxyBusObject;
ref class InterfaceDescription;
ref class InterfaceMember;
class _ProxyBusObject;

/// <summary>
///The result of the asynchronous operation for introspecting the remote object on the bus to determine the interfaces and children that exist.
/// </summary>
public ref class IntrospectRemoteObjectResult sealed {
  public:
    /// <summary>
    ///The ProxyBusObject object that does the introspection
    /// </summary>
    property ProxyBusObject ^ Proxy;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///The result of the operation.
    /// </summary>
    property QStatus Status;

  private:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    IntrospectRemoteObjectResult(ProxyBusObject ^ proxy, Platform::Object ^ context)
    {
        Proxy = proxy;
        Context = context;
        Status = QStatus::ER_OK;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~IntrospectRemoteObjectResult()
    {
        Proxy = nullptr;
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

/// <summary>
///The result of the asynchronous operation for getting a property of the remote bus object.
/// </summary>
public ref class GetPropertyResult sealed {
  public:
    /// <summary>
    ///The proxy object that makes the call.
    /// </summary>
    property ProxyBusObject ^ Proxy;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///The result of the operation.
    /// </summary>
    property QStatus Status;
    /// <summary>
    ///An <c>MsgArg</c> object that contains the property value.
    /// </summary>
    property MsgArg ^ Value;

  private:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    GetPropertyResult(ProxyBusObject ^ proxy, Platform::Object ^ context)
    {
        Proxy = proxy;
        Context = context;
        Status = QStatus::ER_OK;
        Value = nullptr;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~GetPropertyResult()
    {
        Proxy = nullptr;
        Context = nullptr;
        Value = nullptr;
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
///The result of asynchronous operation for getting all properties of the remote bus object.
/// </summary>
public ref class GetAllPropertiesResult sealed {
  public:
    /// <summary>
    ///The proxy object that makes the call.
    /// </summary>
    property ProxyBusObject ^ Proxy;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///The result of the operation.
    /// </summary>
    property QStatus Status;
    /// <summary>
    ///An <c>MsgArg</c> object that contains the value of all properties. The object's 'Value' properties is a <c>MsgArg</c> array.
    ///Each element of the <c>MsgArg</c> array corresponds to a Key/Value pair
    /// </summary>
    property MsgArg ^ Value;

  private:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    GetAllPropertiesResult(ProxyBusObject ^ proxy, Platform::Object ^ context)
    {
        Proxy = proxy;
        Context = context;
        Status = QStatus::ER_OK;
        Value = nullptr;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~GetAllPropertiesResult()
    {
        Proxy = nullptr;
        Context = nullptr;
        Value = nullptr;
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
///The result of the asynchronous operation for setting a property of the remote bus object.
/// </summary>
public ref class SetPropertyResult sealed {
  public:
    /// <summary>
    ///The proxy object that makes the call.
    /// </summary>
    property ProxyBusObject ^ Proxy;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///The result of the operation.
    /// </summary>
    property QStatus Status;

  private:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    SetPropertyResult(ProxyBusObject ^ proxy, Platform::Object ^ context)
    {
        Proxy = proxy;
        Context = context;
        Status = QStatus::ER_OK;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~SetPropertyResult()
    {
        Proxy = nullptr;
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

/// <summary>
///The result of the asynchronous operation for invoking a method call of the remote bus object.
/// </summary>
public ref class MethodCallResult sealed {
  public:
    /// <summary>
    ///The proxy object that make the call.
    /// </summary>
    property ProxyBusObject ^ Proxy;
    /// <summary>
    ///User defined context which will be passed as-is to callback.
    /// </summary>
    property Platform::Object ^ Context;
    /// <summary>
    ///An <c>Message</c> object that contains the method call reply.
    /// </summary>
    property Message ^ Message;

  private:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    MethodCallResult(ProxyBusObject ^ proxy, Platform::Object ^ context)
    {
        Proxy = proxy;
        Context = context;
        Message = nullptr;
        _exception = nullptr;
        _stdException = NULL;
    }

    ~MethodCallResult()
    {
        Proxy = nullptr;
        Context = nullptr;
        Message = nullptr;
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

ref class __ProxyBusObject {
  private:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    friend class _ProxyBusObjectListener;
    __ProxyBusObject();
    ~__ProxyBusObject();

    property BusAttachment ^ Bus;
    property Platform::String ^ Name;
    property Platform::String ^ Path;
    property MessageReceiver ^ Receiver;
    property ajn::SessionId SessionId;
};

class _ProxyBusObjectListener : protected ajn::ProxyBusObject::Listener {
  protected:
    friend ref class ProxyBusObject;
    friend class _ProxyBusObject;
    _ProxyBusObjectListener(_ProxyBusObject* proxybusobject);
    ~_ProxyBusObjectListener();

    ajn::ProxyBusObject::Listener::IntrospectCB GetProxyListenerIntrospectCBHandler();
    ajn::ProxyBusObject::Listener::GetPropertyCB GetProxyListenerGetPropertyCBHandler();
    ajn::ProxyBusObject::Listener::GetAllPropertiesCB GetProxyListenerGetAllPropertiesCBHandler();
    ajn::ProxyBusObject::Listener::SetPropertyCB GetProxyListenerSetPropertyCBHandler();
    void IntrospectCB(::QStatus s, ajn::ProxyBusObject* obj, void* context);
    void GetPropertyCB(::QStatus status, ajn::ProxyBusObject* obj, const ajn::MsgArg& value, void* context);
    void GetAllPropertiesCB(::QStatus status, ajn::ProxyBusObject* obj, const ajn::MsgArg& value, void* context);
    void SetPropertyCB(::QStatus status, ajn::ProxyBusObject* obj, void* context);

    _ProxyBusObject* _proxyBusObject;
};

class _ProxyBusObject : ajn::MessageReceiver {
  protected:
    friend class qcc::ManagedObj<_ProxyBusObject>;
    friend ref class ProxyBusObject;
    friend class _ProxyBusObjectListener;
    _ProxyBusObject(BusAttachment ^ b, const ajn::ProxyBusObject* proxybusobject);
    _ProxyBusObject(BusAttachment ^ b, const ajn::_ProxyBusObject* proxybusobject);
    _ProxyBusObject(BusAttachment ^ b, const char* service, const char* path, ajn::SessionId sessionId);
    ~_ProxyBusObject();
    operator ajn::ProxyBusObject * ();
    operator ajn::_ProxyBusObject * ();

    void ReplyHandler(ajn::Message& msg, void* context);

    __ProxyBusObject ^ _eventsAndProperties;
    _ProxyBusObjectListener* _proxyBusObjectListener;
    qcc::ManagedObj<_MessageReceiver>* _mReceiver;
    std::map<void*, void*> _childObjectMap;
    qcc::Mutex _mutex;
    ajn::_ProxyBusObject* _mProxyBusObject;
    ajn::ProxyBusObject* _proxyBusObject;
};

/// <summary>
/// Each <c>ProxyBusObject</c>instance represents a single DBus/AllJoyn object registered
/// somewhere on the bus. <c>ProxyBusObject</c>s are used to make method calls on these
/// remotely located DBus objects.
/// </summary>
public ref class ProxyBusObject sealed {
  public:

    /// <summary>
    /// Create an empty proxy object that refers to an object at given remote service name. Note
    /// that the created proxy object does not contain information about the interfaces that the
    /// actual remote object implements with the exception that org.freedesktop.DBus.Peer
    /// interface is special-cased (per the DBus spec) and can always be called on any object. Nor
    /// does it contain information about the child objects that the actual remote object might
    /// contain.
    /// </summary>
    /// <remarks>
    /// To fill in this object with the interfaces and child object names that the actual remote
    /// object describes in its introspection data, call IntrospectRemoteObject() or
    /// IntrospectRemoteObjectAsync().
    /// </remarks>
    /// <param name="bus">The bus.</param>
    /// <param name="service">The remote service name (well-known or unique).</param>
    /// <param name="path">The absolute (non-relative) object path for the remote object.</param>
    /// <param name="sessionId">The session id the be used for communicating with remote object.</param>
    ProxyBusObject(BusAttachment ^ bus, Platform::String ^ service, Platform::String ^ path, ajn::SessionId sessionId);

    /// <summary>
    /// Query the remote object on the bus to determine the interfaces and
    /// children that exist. Use this information to populate this object's
    /// interfaces and children.
    /// </summary>
    /// <remarks>
    /// This call executes asynchronously. When the introspection response
    /// is received from the actual remote object, this ProxyBusObject will
    /// be updated and the callback will be called.
    /// This call exists primarily to allow introspection of remote objects
    /// to be done inside AllJoyn method/signal/reply handlers and ObjectRegistered
    /// callbacks.
    /// </remarks>
    /// <param name="context">User defined context which will be passed as-is to callback.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful.
    /// - An error status otherwise
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<IntrospectRemoteObjectResult ^> ^ IntrospectRemoteObjectAsync(Platform::Object ^ context);

    /// <summary>
    /// Get a property from an interface on the remote object.
    /// </summary>
    /// <param name="iface">Name of interface to retrieve property from.</param>
    /// <param name="property">The name of the property to get.</param>
    /// <param name="context">User defined context which will be passed as-is to callback.</param>
    /// <param name="timeout">Time in milliseconds before the call will expire</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if the property was obtained.
    /// - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
    /// - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<GetPropertyResult ^> ^ GetPropertyAsync(
        Platform::String ^ iface,
        Platform::String ^ property,
        Platform::Object ^ context,
        uint32_t timeout);

    /// <summary>
    /// Get all properties from an interface on the remote object.
    /// </summary>
    /// <param name="iface">Name of interface to retrieve all properties from.</param>
    /// <param name="context">User defined context which will be passed as-is to callback.</param>
    /// <param name="timeout">Time in milliseconds before the call will expire</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if the property was obtained.
    /// - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
    /// - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<GetAllPropertiesResult ^> ^ GetAllPropertiesAsync(
        Platform::String ^ iface,
        Platform::Object ^ context,
        uint32_t timeout);

    /// <summary>
    /// Set a property on an interface on the remote object.
    /// </summary>
    /// <param name="iface">Interface that holds the property</param>
    /// <param name="property">The name of the property to set</param>
    /// <param name="value">The value to set</param>
    /// <param name="context">User defined context which will be passed as-is to callback.</param>
    /// <param name="timeout">Time in milliseconds before the call will expire</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if the property was set
    /// - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the no such interface on this remote object.
    /// - #ER_BUS_NO_SUCH_PROPERTY if the property does not exist
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<SetPropertyResult ^> ^ SetPropertyAsync(
        Platform::String ^ iface,
        Platform::String ^ property,
        MsgArg ^ value,
        Platform::Object ^ context,
        uint32_t timeout);

    /// <summary>
    /// Returns the interfaces implemented by this object. Note that all proxy bus objects
    /// automatically inherit the "org.freedesktop.DBus.Peer" which provides the built-in "ping"
    /// method, so this method always returns at least that one interface.
    /// </summary>
    /// <param name="ifaces">A pointer to an InterfaceDescription array to receive the interfaces. Can be NULL in
    /// which case no interfaces are returned and the return value gives the number
    /// of interface available.</param>
    /// <returns>The number of interfaces returned or the total number of interfaces if ifaces is NULL.</returns>
    uint32_t GetInterfaces(Platform::WriteOnlyArray<InterfaceDescription ^> ^ ifaces);

    /// <summary>
    /// Returns a pointer to an interface description. Returns NULL if the object does not implement
    /// the requested interface.
    /// </summary>
    /// <param name="iface">The name of interface to get.</param>
    /// <returns>
    /// - A pointer to the requested interface description.
    /// - NULL if requested interface is not implemented or not found
    /// </returns>
    InterfaceDescription ^ GetInterface(Platform::String ^ iface);

    /// <summary>
    /// Tests if this object implements the requested interface.
    /// </summary>
    /// <param name="iface">The interface to check</param>
    /// <returns> true if the object implements the requested interface </returns>
    bool ImplementsInterface(Platform::String ^ iface);

    /// <summary>
    /// Add an interface to this ProxyBusObject.
    /// </summary>
    /// <remarks>
    /// Occasionally, AllJoyn library user may wish to call a method on
    /// a ProxyBusObject that was not reported during introspection of the remote object.
    /// When this happens, the InterfaceDescription will have to be registered with the
    /// Bus manually and the interface will have to be added to the ProxyBusObject using this method.
    /// The interface added via this call must have been previously registered with the
    /// Bus. (i.e. it must have come from a call to Bus GetInterface ).
    /// </remarks>
    /// <param name="iface">The interface to add to this object. Must come from Bus  GetInterface .</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful.
    /// - An error status otherwise
    /// </exception>
    void AddInterface(InterfaceDescription ^ iface);

    /// <summary>
    /// Add an existing interface to this object using the interface's name.
    /// </summary>
    /// <param name="name">Name of existing interface to add to this object.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful.
    /// - An error status otherwise.
    /// </exception>
    void AddInterfaceWithString(Platform::String ^ name);

    /// <summary>
    /// Returns an array of ProxyBusObjects for the children of this ProxyBusObject.
    /// </summary>
    /// <param name="children">A pointer to an ProxyBusObject array to receive the children. Can be NULL in
    /// which case no children are returned and the return value gives the number
    /// of children available.</param>
    /// <returns>The number of children returned or the total number of children if children is NULL.</returns>
    uint32_t GetChildren(Platform::WriteOnlyArray<ProxyBusObject ^> ^ children);

    /// <summary>
    /// Get a path descendant ProxyBusObject (child) by its relative path name.
    /// </summary>
    /// <remarks>
    /// For example, if this ProxyBusObject's path is @c "/foo/bar", then you can
    /// retrieve the ProxyBusObject for @c "/foo/bar/bat/baz" by calling
    /// @c GetChild("bat/baz")
    /// </remarks>
    /// <param name="path"> the relative path for the child.</param>
    /// <returns>
    /// - The (potentially deep) descendant ProxyBusObject
    /// - NULL if not found.
    /// </returns>
    ProxyBusObject ^ GetChild(Platform::String ^ path);

    /// <summary>
    /// Add a child object (direct or deep object path descendant) to this object.
    /// If you add a deep path descendant, this method will create intermediate
    /// ProxyBusObject children as needed.
    /// </summary>
    /// <remarks>
    /// - It is an error to try to add a child that already exists.
    /// - It is an error to try to add a child that has an object path that is not a descendant of this object's path.
    /// </remarks>
    /// <param name="child">Child ProxyBusObject</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful.
    /// - #ER_BUS_BAD_CHILD_PATH if the path is a bad path
    /// - #ER_BUS_OBJ_ALREADY_EXISTS the the object already exists on the ProxyBusObject
    /// </exception>
    void AddChild(ProxyBusObject ^ child);

    /// <summary>
    /// Remove a child object and any descendants it may have.
    /// </summary>
    /// <param name="path">Absolute or relative (to this ProxyBusObject) object path.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful.
    /// - #ER_BUS_BAD_CHILD_PATH if the path given was not a valid path
    /// - #ER_BUS_OBJ_NOT_FOUND if the Child object was not found
    /// - #ER_FAIL any other unexpected error.
    /// </exception>
    void RemoveChild(Platform::String ^ path);

    /// <summary>
    /// Make an asynchronous method call from this object
    /// </summary>
    /// <param name="method">Method being invoked.</param>
    /// <param name="args">The arguments for the method call (can be NULL)</param>
    /// <param name="context">User-defined context that will be returned to the reply handler</param>
    /// <param name="timeout">Timeout specified in milliseconds to wait for a reply</param>
    /// <param name="flags">Logical OR of the message flags for this method call. The following flags apply to method calls:
    /// - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
    /// - If #ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
    /// - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<MethodCallResult ^> ^ MethodCallAsync(InterfaceMember ^ method,
                                                                               const Platform::Array<MsgArg ^> ^ args,
                                                                               Platform::Object ^ context,
                                                                               uint32_t timeout,
                                                                               uint8_t flags);

    /// <summary>
    /// Make an asynchronous method call from this object
    /// </summary>
    /// <param name="ifaceName">Name of interface for method.</param>
    /// <param name="methodName">Name of method.</param>
    /// <param name="args">The arguments for the method call (can be NULL)</param>
    /// <param name="context">User-defined context that will be returned to the reply handler</param>
    /// <param name="timeout">Timeout specified in milliseconds to wait for a reply</param>
    /// <param name="flags">Logical OR of the message flags for this method call. The following flags apply to method calls:
    /// - If #ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
    /// - If #ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
    /// - If #ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    /// <returns>A handle to the async operation.</returns>
    Windows::Foundation::IAsyncOperation<MethodCallResult ^> ^ MethodCallAsync(Platform::String ^ ifaceName,
                                                                               Platform::String ^ methodName,
                                                                               const Platform::Array<MsgArg ^> ^ args,
                                                                               Platform::Object ^ context,
                                                                               uint32_t timeout,
                                                                               uint8_t flags);

    /// <summary>
    /// Initialize this proxy object from an XML string. Calling this method does several things:
    /// -# Create and register any new InterfaceDescription(s) that are mentioned in the XML.
    /// (Interfaces that are already registered with the bus are left "as-is".)
    /// -# Add all the interfaces mentioned in the introspection data to this ProxyBusObject.
    /// -# Recursively create any child ProxyBusObject(s) and create/add their associated @n
    /// interfaces as mentioned in the XML. Then add the descendant object(s) to the appropriate
    /// descendant of this ProxyBusObject (in the children collection). If the named child object
    /// already exists as a child of the appropriate ProxyBusObject, then it is updated
    /// to include any new interfaces or children mentioned in the XML.
    /// Note that when this method fails during parsing, the return code will be set accordingly.
    /// However, any interfaces which were successfully parsed prior to the failure
    /// may be registered with the bus. Similarly, any objects that were successfully created
    /// before the failure will exist in this object's set of children.
    /// </summary>
    /// <param name="xml">An XML string in DBus introspection format.</param>
    /// <param name="identifier">An optional identifying string to include in error logging messages.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if parsing is completely successful.
    /// - An error status otherwise.
    /// </exception>
    void ParseXml(Platform::String ^ xml, Platform::String ^ identifier);

    /// <summary>
    /// Asynchronously secure the connection to the remote peer for this proxy object. Peer-to-peer
    /// connections can only be secured if EnablePeerSecurity() was previously called on the bus
    /// attachment for this proxy object. If the peer-to-peer connection is already secure this
    /// function does nothing. Note that peer-to-peer connections are automatically secured when a
    /// method call requiring encryption is sent.
    /// Notification of success or failure is via the AuthListener passed to EnablePeerSecurity().
    /// </summary>
    /// <param name="forceAuth">If true, forces an re-authentication even if the peer connection is already
    /// authenticated.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if securing could begin.
    /// - #ER_BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment::EnablePeerSecurity() has not been called.
    /// - Other error status codes indicating a failure.
    /// </exception>
    void SecureConnectionAsync(bool forceAuth);

    /// <summary>
    /// Indicates if this is a valid (usable) proxy bus object.
    /// </summary>
    /// <returns>true if a valid proxy bus object, false otherwise.</returns>
    bool IsValid();

    /// <summary>The bus attachment for the remote object.</summary>
    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

    /// <summary>The absolute object path for the remote object.</summary>
    property Platform::String ^ Path
    {
        Platform::String ^ get();
    }

    /// <summary>The remote service name for this object.
    /// (typically a well-known service name but may be a unique name)
    /// </summary>
    property Platform::String ^ Name
    {
        Platform::String ^ get();
    }

    /// <summary>The MessageReceiver for this object.</summary>
    property MessageReceiver ^ Receiver
    {
        MessageReceiver ^ get();
    }

    /// <summary>The SessionId for this object.</summary>
    property ajn::SessionId SessionId
    {
        ajn::SessionId get();
    }

  private:
    friend ref class BusAttachment;
    ProxyBusObject(BusAttachment ^ bus, const ajn::ProxyBusObject * proxyBusObject);
    ProxyBusObject(BusAttachment ^ bus, const ajn::_ProxyBusObject * proxyBusObject);
    ProxyBusObject(BusAttachment ^ bus, const qcc::ManagedObj<_ProxyBusObject>* proxyBusObject);
    ~ProxyBusObject();

    qcc::ManagedObj<_ProxyBusObject>* _mProxyBusObject;
    _ProxyBusObject* _proxyBusObject;
};

}
// ProxyBusObject.h
