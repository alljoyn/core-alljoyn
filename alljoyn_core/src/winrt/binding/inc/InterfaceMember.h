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

namespace AllJoyn {

ref class InterfaceDescription;

ref class __InterfaceMember {
  private:
    friend ref class InterfaceMember;
    friend class _InterfaceMember;
    __InterfaceMember();
    ~__InterfaceMember();

    property InterfaceDescription ^ Interface;
    property AllJoynMessageType MemberType;
    property Platform::String ^ Name;
    property Platform::String ^ Signature;
    property Platform::String ^ ReturnSignature;
    property Platform::String ^ ArgNames;
    property uint8_t Annotation;
    property Platform::String ^ AccessPerms;
};

class _InterfaceMember {
  protected:
    friend class qcc::ManagedObj<_InterfaceMember>;
    friend ref class InterfaceMember;
    friend ref class BusAttachment;
    friend ref class BusObject;
    friend class _BusObject;
    friend ref class ProxyBusObject;
    _InterfaceMember(const ajn::InterfaceDescription* iface, ajn::AllJoynMessageType type, const char* name,
                     const char* signature, const char* returnSignature, const char* argNames,
                     uint8_t annotation, const char* accessPerms);
    _InterfaceMember(const ajn::InterfaceDescription::Member* member);
    ~_InterfaceMember();

    operator ajn::InterfaceDescription::Member * ();

    __InterfaceMember ^ _eventsAndProperties;
    ajn::InterfaceDescription::Member* _member;
};

public ref class InterfaceMember sealed {
  public:
    /// <summary>
    /// constructor
    /// </summary>
    /// <param name="iface">Interface that this member belongs to</param>
    /// <param name="type">Member type</param>
    /// <param name="name">Member name</param>
    /// <param name="signature">Method call IN arguments (NULL for signals)</param>
    /// <param name="returnSignature">Signal or method call OUT arguments</param>
    /// <param name="argNames">Comma separated list of argument names - can be NULL</param>
    /// <param name="annotation">Exclusive OR of flags MEMBER_ANNOTATE_NO_REPLY and MEMBER_ANNOTATE_DEPRECATED</param>
    /// <param name="accessPerms">Required permissions to invoke this call</param>
    InterfaceMember(InterfaceDescription ^ iface, AllJoynMessageType type, Platform::String ^ name,
                    Platform::String ^ signature, Platform::String ^ returnSignature, Platform::String ^ argNames,
                    uint8_t annotation, Platform::String ^ accessPerms);

    /// <summary>
    ///Interface that this member belongs to
    /// </summary>
    property InterfaceDescription ^ Interface
    {
        InterfaceDescription ^ get();
    }

    /// <summary>
    ///Member type
    /// </summary>
    property AllJoynMessageType MemberType
    {
        AllJoynMessageType get();
    }

    /// <summary>
    ///Member name
    /// </summary>
    property Platform::String ^ Name
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Method call IN arguments (NULL for signals)
    /// </summary>
    property Platform::String ^ Signature
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Signal or method call OUT arguments
    /// </summary>
    property Platform::String ^ ReturnSignature
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Comma separated list of argument names - can be NULL
    /// </summary>
    property Platform::String ^ ArgNames
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Required permissions to invoke this call
    /// </summary>
    property Platform::String ^ AccessPerms
    {
        Platform::String ^ get();
    }

  private:
    friend ref class BusAttachment;
    friend ref class BusObject;
    friend class _BusObject;
    friend ref class ProxyBusObject;
    friend class _MessageReceiver;
    friend ref class InterfaceDescription;
    InterfaceMember(const ajn::InterfaceDescription::Member * interfaceMember);
    ~InterfaceMember();

    qcc::ManagedObj<_InterfaceMember>* _mMember;
    _InterfaceMember* _member;
};

}
// InterfaceMember.h
