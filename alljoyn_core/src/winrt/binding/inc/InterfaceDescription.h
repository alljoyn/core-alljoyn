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

#include <alljoyn/InterfaceDescription.h>
#include <Message.h>
#include <InterfaceMember.h>
#include <InterfaceProperty.h>

namespace AllJoyn {

ref class InterfaceDescription;

public enum class PropAccessType {
    /// <summary>Read Access type</summary>
    PROP_ACCESS_READ  = ajn::PROP_ACCESS_READ,
    /// <summary>Write Access type</summary>
    PROP_ACCESS_WRITE = ajn::PROP_ACCESS_WRITE,
    /// <summary>Read-Write Access type</summary>
    PROP_ACCESS_RW    = ajn::PROP_ACCESS_RW
};

public enum class MemberAnnotationType {
    /// <summary>No reply annotate flag</summary>
    MEMBER_ANNOTATE_NO_REPLY   = ajn::MEMBER_ANNOTATE_NO_REPLY,
    /// <summary>Deprecated annotate flag</summary>
    MEMBER_ANNOTATE_DEPRECATED = ajn::MEMBER_ANNOTATE_DEPRECATED
};

ref class __InterfaceDescription {
  private:
    friend ref class InterfaceDescription;
    friend class _InterfaceDescription;
    __InterfaceDescription();
    ~__InterfaceDescription();

    property Platform::String ^ Name;
};

class _InterfaceDescription {
  protected:
    friend class qcc::ManagedObj<_InterfaceDescription>;
    friend ref class InterfaceDescription;
    friend ref class BusAttachment;
    friend ref class BusObject;
    friend ref class InterfaceMember;
    friend ref class ProxyBusObject;
    _InterfaceDescription(const ajn::InterfaceDescription* interfaceDescr);
    ~_InterfaceDescription();

    operator ajn::InterfaceDescription * ();

    __InterfaceDescription ^ _eventsAndProperties;
    const ajn::InterfaceDescription* _interfaceDescr;
};


/// <summary>
///Class for describing message bus interfaces. %InterfaceDescription objects describe the methods,
///signals and properties of a BusObject or ProxyBusObject.
/// </summary>
/// <remarks>
///Calling ProxyBusObject::AddInterface(const char*) adds the AllJoyn interface described by an
///%InterfaceDescription to a ProxyBusObject instance. After an  %InterfaceDescription has been
///added, the methods described in the interface can be called. Similarly calling
///BusObject::AddInterface adds the interface and its methods, properties, and signal to a
///BusObject. After an interface has been added method handlers for the methods described in the
///interface can be added by calling BusObject::AddMethodHandler or BusObject::AddMethodHandlers.
///An %InterfaceDescription can be constructed piecemeal by calling InterfaceDescription::AddMethod,
///InterfaceDescription::AddMember(), and InterfaceDescription::AddProperty(). Alternatively,
///calling ProxyBusObject::ParseXml will create the %InterfaceDescription instances for that proxy
///object directly from an XML string. Calling ProxyBusObject::IntrospectRemoteObject or
///ProxyBusObject::IntrospectRemoteObjectAsync also creates the %InterfaceDescription
///instances from XML but in this case the XML is obtained by making a remote Introspect method
///call on a bus object.
/// </remarks>
public ref class InterfaceDescription sealed {
  public:
    /// <summary>
    ///Add a member to the interface.
    /// </summary>
    /// <param name="type"> Message type.</param>
    /// <param name="name"> Name of member.</param>
    /// <param name="inputSig"> Signature of input parameters or NULL for none.</param>
    /// <param name="outSig"> Signature of output parameters or NULL for none.</param>
    /// <param name="argNames"> Comma separated list of input and then output arg names used in annotation XML.</param>
    /// <param name="annotation"> Annotation flags.</param>
    /// <param name="accessPerms"> Required permissions to invoke this call</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
    /// </exception>
    void AddMember(AllJoynMessageType type,
                   Platform::String ^ name,
                   Platform::String ^ inputSig,
                   Platform::String ^ outSig,
                   Platform::String ^ argNames,
                   uint8_t annotation,
                   Platform::String ^ accessPerms);

    /// <summary>Add an annotation to an existing member (signal or method).</summary>
    /// <param name="member"> Name of member</param>
    /// <param name="name"> Name of annotation</param>
    /// <param name="value"> Value for the annotation</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
    /// </exception>
    void AddMemberAnnotation(Platform::String ^ member, Platform::String ^ name, Platform::String ^ value);

    /// <summary>Get annotation to an existing member (signal or method).</summary>
    /// <param name="member"> Name of member</param>
    /// <param name="name"> Name of annotation</param>
    /// <returns>
    ///Value iff member and name exist as an annotation.
    /// </returns>
    Platform::String ^ GetMemberAnnotation(Platform::String ^ member, Platform::String ^ name);

    /// <summary>Lookup a member description by name</summary>
    /// <param name="name">
    ///Name of the member to lookup
    /// </param>
    /// <returns>
    ///- Pointer to member.
    ///- NULL if does not exist.
    /// </returns>
    InterfaceMember ^ GetMember(Platform::String ^ name);

    /// <summary>
    ///Get all the members.
    /// </summary>
    /// <param name="members">
    ///A caller allocated array to receive the members. Can be NULL in
    ///which case no members are returned and the return value gives the number
    ///of members available.
    /// </param>
    /// <returns>
    ///The number of members returned or the total number of members if props is NULL.
    /// </returns>
    uint32_t GetMembers(Platform::WriteOnlyArray<InterfaceMember ^> ^ members);

    /// <summary>
    ///Check for existence of a member. Optionally check the signature also.
    /// </summary>
    /// <remarks>
    ///if the a signature is not provided this method will only check to see if
    ///a member with the given @c name exists.  If a signature is provided a
    ///member with the given @c name and @c signature must exist for this to return true.
    /// </remarks>
    /// <param name="name">
    ///Name of the member to lookup
    /// </param>
    /// <param name="inSig">
    ///Input parameter signature of the member to lookup
    /// </param>
    /// <param name="outSig">
    ///Output parameter signature of the member to lookup (leave NULL for signals)
    /// </param>
    /// <returns>
    ///true if the member name exists.
    /// </returns>
    bool HasMember(Platform::String ^ name, Platform::String ^ inSig, Platform::String ^ outSig);

    /// <summary>
    ///Add a method call member to the interface.
    /// </summary>
    /// <param name="name">
    ///Name of method call member.
    /// </param>
    /// <param name="inputSig">
    ///Signature of input parameters or NULL for none.
    /// </param>
    /// <param name="outSig">
    ///Signature of output parameters or NULL for none.
    /// </param>
    /// <param name="argNames">
    ///Comma separated list of input and then output arg names used in annotation XML.
    /// </param>
    /// <param name="annotation">
    ///Annotation flags.
    /// </param>
    /// <param name="accessPerms">
    ///Access permission requirements on this call
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
    /// </exception>
    void AddMethod(Platform::String ^ name,
                   Platform::String ^ inputSig,
                   Platform::String ^ outSig,
                   Platform::String ^ argNames,
                   uint8_t annotation,
                   Platform::String ^ accessPerms);

    /// <summary>
    ///Lookup a member method description by name
    /// </summary>
    /// <param name="name">
    ///Name of the method to lookup
    /// </param>
    /// <returns>
    ///An InterfaceMember representing the memeber of the interface
    /// </returns>
    InterfaceMember ^ GetMethod(Platform::String ^ name);

    /// <summary>
    ///Add a signal member to the interface.
    /// </summary>
    /// <param name="name">
    ///Name of method call member.
    /// </param>
    /// <param name="sig">
    ///Signature of parameters or NULL for none.
    /// </param>
    /// <param name="argNames">
    ///Comma separated list of arg names used in annotation XML.
    /// </param>
    /// <param name="annotation">
    ///Annotation flags.
    /// </param>
    /// <param name="accessPerms">
    ///Access permission requirements on this call
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
    /// </exception>
    void AddSignal(Platform::String ^ name, Platform::String ^ sig, Platform::String ^ argNames, uint8_t annotation, Platform::String ^ accessPerms);

    /// <summary>
    ///Lookup a member signal description by name
    /// </summary>
    /// <param name="name">
    ///Name of the signal to lookup
    /// </param>
    /// <returns>
    ///An InterfaceMember representing the signal of the interface
    /// </returns>
    InterfaceMember ^ GetSignal(Platform::String ^ name);

    /// <summary>
    ///Lookup a property description by name
    /// </summary>
    /// <param name="name">
    ///Name of the property to lookup
    /// </param>
    /// <returns>
    ///A structure representing the properties of the interface
    /// </returns>
    InterfaceProperty ^ GetProperty(Platform::String ^ name);

    /// <summary>
    ///Get all the properties.
    /// </summary>
    /// <param name="props">
    ///A caller allocated array to receive the properties. Can be NULL in
    ///which case no properties are returned and the return value gives the number
    ///of properties available.
    /// </param>
    /// <returns>
    ///The number of properties returned or the total number of properties if props is NULL.
    /// </returns>
    uint32_t GetProperties(Platform::WriteOnlyArray<InterfaceProperty ^> ^ props);

    /// <summary>
    ///Add a property to the interface.
    /// </summary>
    /// <param name="name">
    ///Name of property.
    /// </param>
    /// <param name="signature">
    /// Property type.
    /// </param>
    /// <param name="access">
    ///#PROP_ACCESS_READ, #PROP_ACCESS_WRITE or #PROP_ACCESS_RW
    /// </param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful.
    /// - #ER_BUS_PROPERTY_ALREADY_EXISTS if the property can not be added because it already exists.
    /// </exception>
    void AddProperty(Platform::String ^ name, Platform::String ^ signature, uint8_t access);

    /// <summary>Add an annotation to an existing property</summary>
    /// <param name="member"> Name of member</param>
    /// <param name="name"> Name of annotation</param>
    /// <param name="value"> Value for the annotation</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
    /// </exception>
    void AddPropertyAnnotation(Platform::String ^ member, Platform::String ^ name, Platform::String ^ value);

    /// <summary>Get the annotation value for a property</summary>
    /// <param name="member"> Name of member</param>
    /// <param name="name"> Name of annotation</param>
    /// <returns>
    ///Value iff member and name exist as an annotation.
    /// </returns>
    Platform::String ^ GetPropertyAnnotation(Platform::String ^ member, Platform::String ^ name);

    /// <summary>Check for existence of a property.</summary>
    /// <param name="name">
    ///Name of the property to lookup
    /// </param>
    /// <returns>
    ///true if the property exists.
    /// </returns>
    bool HasProperty(Platform::String ^ name);

    /// <summary>
    ///Check for existence of any properties
    /// </summary>
    /// <returns>
    ///true if interface has any properties.
    /// </returns>
    bool HasProperties();

    /// <summary>Returns a description of the interface in introspection XML format</summary>
    /// <param name="indent">
    ///Number of space chars to use in XML indentation.
    /// </param>
    /// <returns>
    ///The XML introspection data.
    /// </returns>
    Platform::String ^ Introspect(uint32_t indent);

    /// <summary>
    ///Activate this interface. An interface must be activated before it can be used. Activating an
    ///interface locks the interface so that is can no longer be modified.
    /// </summary>
    void Activate();

    /// <summary>
    ///Indicates if this interface is secure. Secure interfaces require end-to-end authentication.
    ///The arguments for methods calls made to secure interfaces and signals emitted by secure
    ///interfaces are encrypted.
    /// </summary>
    /// <returns>
    ///true if the interface is secure.
    /// </returns>
    bool IsSecure();

    /// <summary>Add an annotation to the interface.</summary>
    /// <param name="name"> Name of annotation</param>
    /// <param name="value"> Value for the annotation</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
    /// </exception>
    void AddAnnotation(Platform::String ^ name, Platform::String ^ value);

    /// <summary>
    ///Get the names and values of annotations.
    /// </summary>
    /// <param name="names"> Names of the retrieved annotations</param>
    /// <param name="values"> Values of the retrieved annotations</param>
    /// <param name="size"> Number of annotations to get</param>
    /// <remarks>
    ///To get the total number of annotations:
    /// - Call with names and values set to null, and size == 0
    /// - Allocate arrays for names and values sized with the initial return value
    /// - Call again with the properly sized arrays and size parameter
    /// </remarks>
    /// <returns>
    ///The number of annotations returned.
    /// </returns>
    uint32_t GetAnnotations(Platform::WriteOnlyArray<Platform::String ^> ^ names, Platform::WriteOnlyArray<Platform::String ^> ^ values, uint32_t size);

    /// <summary>Get the value of an annotation</summary>
    /// <param name="name"> Name of annotation</param>
    /// <returns>
    ///Value iff member and name exist as an annotation.
    /// </returns>
    Platform::String ^ GetAnnotation(Platform::String ^ name);

    /// <summary>
    ///The name of the interface
    /// </summary>
    property Platform::String ^ Name
    {
        Platform::String ^ get();
    }

  private:
    friend ref class BusAttachment;
    friend ref class BusObject;
    friend ref class InterfaceMember;
    friend ref class ProxyBusObject;
    InterfaceDescription(const ajn::InterfaceDescription * interfaceDescr);
    InterfaceDescription(const qcc::ManagedObj<_InterfaceDescription>* interfaceDescr);
    ~InterfaceDescription();

    qcc::ManagedObj<_InterfaceDescription>* _mInterfaceDescr;
    _InterfaceDescription* _interfaceDescr;
};

}
// InterfaceDescription.h
