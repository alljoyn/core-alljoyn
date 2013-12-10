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

#include <MessageReceiver.h>
#include <MsgArg.h>
#include <Status_CPP0x.h>
#include <qcc/Mutex.h>
#include <map>

namespace AllJoyn {

ref class BusAttachment;
ref class InterfaceDescription;
ref class BusObject;
ref class MessageReceiver;
ref class Message;
ref class InterfaceMember;

/// <summary>
/// Handle a bus request to read a property from this object.
/// BusObjects that implement properties should override this method.
/// The default version simply returns #ER_BUS_NO_SUCH_PROPERTY
/// </summary>
/// <param name="ifcName">Identifies the interface that the property is defined on</param>
/// <param name="propName">Identifies the the property to get</param>
/// <param name="val">Returns the property value. The type of this value is the actual value type.</param>
/// <returns>#ER_BUS_NO_SUCH_PROPERTY (Should be changed by user implementation of BusObject)</returns>
public delegate QStatus BusObjectGetHandler(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<MsgArg ^> ^ val);

/// <summary>
/// Handle a bus attempt to write a property value to this object.
/// BusObjects that implement properties should override this method.
/// This default version just replies with #ER_BUS_NO_SUCH_PROPERTY
/// </summary>
/// <param name="ifcName">Identifies the interface that the property is defined on</param>
/// <param name="propName">Identifies the the property to set</param>
/// <param name="val">The property value to set. The type of this value is the actual value type.</param>
/// <returns>#ER_BUS_NO_SUCH_PROPERTY (Should be changed by user implementation of BusObject)</returns>
public delegate QStatus BusObjectSetHandler(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val);

/// <summary>
/// Returns a description of the object in the D-Bus introspection XML format.
/// This method can be overridden by derived classes in order to customize the
/// introspection XML presented to remote nodes. Note that to DTD description and
/// the root element are not generated.
/// </summary>
/// <param name="deep">Include XML for all descendants rather than stopping at direct children.</param>
/// <param name="indent">Number of characters to indent the XML</param>
/// <returns>Description of the object in D-Bus introspection XML format</returns>
public delegate Platform::String ^ BusObjectGenerateIntrospectionHandler(bool deep, uint32_t indent);

/// <summary>
/// Called by the message bus when the object has been successfully registered. The object can
/// perform any initialization such as adding match rules at this time.
/// </summary>
public delegate void BusObjectObjectRegisteredHandler(void);

/// <summary>
/// Called by the message bus when the object has been successfully unregistered
/// </summary>
/// <remarks>
/// This base class implementation @b must be called explicitly by any overriding derived class.
/// </remarks>
public delegate void BusObjectObjectUnregisteredHandler(void);

/// <summary>
/// Default handler for a bus attempt to read all properties on an interface.
/// </summary>
/// <remarks>
/// A derived class can override this function to provide a custom handler for the GetAllProps
/// method call. If overridden the custom handler must compose an appropriate reply message
/// listing all properties on this object.
/// </remarks>
/// <param name="member">Identifies the org.freedesktop.DBus.Properties.GetAll method.</param>
/// <param name="msg">The Properties.GetAll request.</param>
public delegate void BusObjectGetAllPropsHandler(InterfaceMember ^ member, Message ^ msg);

/// <summary>
/// Default handler for a bus attempt to read the object's introspection data.
/// </summary>
/// <remarks>
/// A derived class can override this function to provide a custom handler for the GetProp method
/// call. If overridden the custom handler must compose an appropriate reply message.
/// </remarks>
/// <param name="member">Identifies the Introspect method.</param>
/// <param name="msg">The Introspectable.Introspect request.</param>
public delegate void BusObjectIntrospectHandler(InterfaceMember ^ member, Message ^ msg);

ref class __BusObject {
  private:
    friend ref class BusObject;
    friend class _BusObject;
    __BusObject();
    ~__BusObject();

    event BusObjectGetHandler ^ Get;
    event BusObjectSetHandler ^ Set;
    event BusObjectGenerateIntrospectionHandler ^ GenerateIntrospection;
    event BusObjectObjectRegisteredHandler ^ ObjectRegistered;
    event BusObjectObjectUnregisteredHandler ^ ObjectUnregistered;
    event BusObjectGetAllPropsHandler ^ GetAllProps;
    event BusObjectIntrospectHandler ^ Introspect;
    property BusAttachment ^ Bus;
    property Platform::String ^ Name;
    property Platform::String ^ Path;
    property MessageReceiver ^ Receiver;
};

class _BusObject : protected ajn::BusObject {
  protected:
    friend class qcc::ManagedObj<_BusObject>;
    friend ref class BusObject;
    friend ref class BusAttachment;
    _BusObject(BusAttachment ^ b, ajn::BusAttachment& bus, const char* path, bool isPlaceholder);
    ~_BusObject();

    QStatus DefaultBusObjectGetHandler(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<MsgArg ^> ^ val);
    QStatus DefaultBusObjectSetHandler(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val);
    Platform::String ^ DefaultBusObjectGenerateIntrospectionHandler(bool deep, uint32_t indent);
    void DefaultBusObjectObjectRegisteredHandler(void);
    void DefaultBusObjectObjectUnregisteredHandler(void);
    void DefaultBusObjectGetAllPropsHandler(InterfaceMember ^ member, Message ^ msg);
    void DefaultBusObjectIntrospectHandler(InterfaceMember ^ member, Message ^ msg);
    ::QStatus AddInterface(const ajn::InterfaceDescription& iface);
    ::QStatus Get(const char* ifcName, const char* propName, ajn::MsgArg& val);
    ::QStatus Set(const char* ifcName, const char* propName, ajn::MsgArg& val);
    qcc::String GenerateIntrospection(bool deep, uint32_t indent);
    void ObjectRegistered(void);
    void ObjectUnregistered(void);
    void GetAllProps(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);
    void Introspect(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);
    void CallMethodHandler(ajn::MessageReceiver::MethodHandler handler, const ajn::InterfaceDescription::Member* member, ajn::Message& message, void* context);

    __BusObject ^ _eventsAndProperties;
    qcc::ManagedObj<_MessageReceiver>* _mReceiver;
    std::map<void*, void*> _messageReceiverMap;
    qcc::Mutex _mutex;
};

public ref class BusObject sealed {
  public:
    /// <summary>
    /// BusObject constructor.
    /// </summary>
    /// <param name="bus">Bus that this object exists on.</param>
    /// <param name="path">Object path for object.</param>
    /// <param name="isPlaceholder">Place-holder objects are created by the bus itself and serve only
    /// as parent objects (in the object path sense) to other objects.</param>
    BusObject(BusAttachment ^ bus, Platform::String ^ path, bool isPlaceholder);

    /// <summary>
    /// Emit PropertiesChanged to signal the bus that this property has been updated
    /// </summary>
    /// <param name="ifcName">The name of the interface</param>
    /// <param name="propName">The name of the property being changed</param>
    /// <param name="val">The new value of the property</param>
    /// <param name="id">ID of the session we broadcast the signal to (0 for all)</param>
    void EmitPropChanged(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val, ajn::SessionId id);

    /// <summary>
    /// Reply to a method call.
    /// </summary>
    /// <param name="msg">The method call message</param>
    /// <param name="args">The reply arguments (NULL if no arguments)</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void MethodReply(Message ^ msg, const Platform::Array<MsgArg ^> ^ args);

    /// <summary>
    /// Reply to a method call with an error message.
    /// </summary>
    /// <param name="msg ">The method call message</param>
    /// <param name="error">The name of the error</param>
    /// <param name="errorMessage">An error message string</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void MethodReply(Message ^ msg, Platform::String ^ error, Platform::String ^ errorMessage);

    /// <summary>
    /// Reply to a method call with an error message.
    /// </summary>
    /// <param name="msg">The method call message</param>
    /// <param name="s">The status code for the error</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void MethodReplyWithQStatus(Message ^ msg, QStatus s);

    /// <summary>
    /// Send a signal.
    /// </summary>
    /// <param name="destination">The unique or well-known bus name or the signal recipient (Empty string for broadcast signals)</param>
    /// <param name="sessionId">A unique SessionId for this AllJoyn session instance</param>
    /// <param name="signal">Interface member of signal being emitted.</param>
    /// <param name="args">The arguments for the signal (can be NULL)</param>
    /// <param name="timeToLive">If non-zero this specifies in milliseconds for non-sessionless signals and seconds for
    /// sessionless signals the useful lifetime for this signal. If delivery of the signal is delayed beyond the timeToLive due to
    /// network congestion or other factors the signal may be discarded. There is
    /// no guarantee that expired signals will not still be delivered.</param>
    /// <param name="flags">Logical OR of the message flags for this signals. The following flags apply to signals:
    /// - If ::ALLJOYN_FLAG_GLOBAL_BROADCAST is set broadcast signal (null destination) will be forwarded across bus-to-bus connections.
    /// - If ::ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
    /// - If ::ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void Signal(Platform::String ^ destination,
                ajn::SessionId sessionId,
                InterfaceMember ^ signal,
                const Platform::Array<MsgArg ^> ^ args,
                uint16_t timeToLive,
                uint8_t flags);

    /// <summary>
    /// Remove sessionless message sent from this object from local daemon's store/forward cache.
    /// /<summary>
    /// <param name="serialNumber"> Serial number of previously sent sessionless signal. </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void CancelSessionlessMessageBySN(uint32_t serialNumber);

    /// <summary>
    /// Remove sessionless message sent from this object from local daemon's store/forward cache.
    /// /<summary>
    /// <param name="msg"> Message to be remove. </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void CancelSessionlessMessage(Message ^ msg);

    /// <summary>
    /// Add an interface to this object. If the interface has properties this will also add the
    /// standard property access interface. An interface must be added before its method handlers can be
    /// added. Note that the Peer interface (org.freedesktop.DBus.peer) is implicit on all objects and
    /// cannot be explicitly added, and the Properties interface (org.freedesktop,DBus.Properties) is
    /// automatically added when needed and cannot be explicitly added.
    /// </summary>
    /// Once an object is registered, it should not add any additional interfaces. Doing so would
    /// confuse remote objects that may have already introspected this object.
    /// <param name="iface">The interface to add</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if the interface was successfully added.
    /// - #ER_BUS_IFACE_ALREADY_EXISTS if the interface already exists.
    /// - An error status otherwise
    /// </exception>
    void AddInterface(InterfaceDescription ^ iface);

    /// <summary>
    /// Add a method handler to this object. The interface for the method handler must have already
    /// been added by calling AddInterface().
    /// </summary>
    /// <param name="member">Interface member implemented by handler.</param>
    /// <param name="receiver">Method handler.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if the method handler was added.
    /// - An error status otherwise
    /// </exception>
    void AddMethodHandler(InterfaceMember ^ member, MessageReceiver ^ receiver);

    /// <summary>
    /// Raised when a bus request to read a property from this object.
    /// </summary>
    event BusObjectGetHandler ^ Get
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectGetHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        QStatus raise(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<MsgArg ^> ^ val);
    }

    /// <summary>
    /// Raised when a bus attempts to write a property value to this object.
    /// </summary>
    event BusObjectSetHandler ^ Set
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectSetHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        QStatus raise(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val);
    }

    /// <summary>
    /// Raised when a bus requests a description of the object in the D-Bus introspection XML format.
    /// </summary>
    event BusObjectGenerateIntrospectionHandler ^ GenerateIntrospection
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectGenerateIntrospectionHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        Platform::String ^ raise(bool deep, uint32_t indent);
    }

    /// <summary>
    /// Raised when the object has been successfully registered.
    /// </summary>
    event BusObjectObjectRegisteredHandler ^ ObjectRegistered
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectObjectRegisteredHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(void);
    }

    /// <summary>
    /// Raised when the object has been successfully unregistered
    /// </summary>
    event BusObjectObjectUnregisteredHandler ^ ObjectUnregistered
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectObjectUnregisteredHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(void);
    }

    /// <summary>
    /// Raised when a bus attempts to read all properties on an interface.
    /// </summary>
    event BusObjectGetAllPropsHandler ^ GetAllProps
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectGetAllPropsHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(InterfaceMember ^ member, Message ^ msg);
    }

    /// <summary>
    /// Raised when a bus attempts to read the object's introspection data.
    /// </summary>
    event BusObjectIntrospectHandler ^ Introspect
    {
        Windows::Foundation::EventRegistrationToken add(BusObjectIntrospectHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(InterfaceMember ^ member, Message ^ msg);
    }

    /// <summary>Return the BusAttachment for the object</summary>
    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

    /// <summary>
    /// Get the name of this object.
    /// The name is the last component of the path.
    /// </summary>
    /// <returns>Last component of object path.</returns>
    property Platform::String ^ Name
    {
        Platform::String ^ get();
    }

    /// <summary>
    /// Return the path for the object
    /// </summary>
    /// <returns>Object path</returns>
    property Platform::String ^ Path
    {
        Platform::String ^ get();
    }

    /// <summary>
    /// Return the receiver for the object
    /// </summary>
    /// <returns>A MessageReceiver</returns>
    property MessageReceiver ^ Receiver
    {
        MessageReceiver ^ get();
    }

  private:
    friend ref class BusAttachment;
    BusObject(const qcc::ManagedObj<_BusObject>* busObject);
    ~BusObject();

    qcc::ManagedObj<_BusObject>* _mBusObject;
    _BusObject* _busObject;
};

}
// BusObject.h
