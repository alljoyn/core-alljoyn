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

ref class __InterfaceProperty {
  private:
    friend ref class InterfaceProperty;
    friend class _InterfaceProperty;
    __InterfaceProperty();
    ~__InterfaceProperty();

    property Platform::String ^ Name;
    property Platform::String ^ Signature;
    property uint8_t Access;
};

class _InterfaceProperty {
  protected:
    friend class qcc::ManagedObj<_InterfaceProperty>;
    friend ref class InterfaceProperty;
    _InterfaceProperty(const char* name, const char* signature, uint8_t access);
    _InterfaceProperty(const ajn::InterfaceDescription::Property* property);
    ~_InterfaceProperty();

    operator ajn::InterfaceDescription::Property * ();

    __InterfaceProperty ^ _eventsAndProperties;
    ajn::InterfaceDescription::Property* _property;
};

public ref class InterfaceProperty sealed {
  public:
    /// <summary>
    ///Constructor
    /// </summary>
    /// <param name="name">Name of the property</param>
    /// <param name="signature">Signature of the property</param>
    /// <param name="access">Access flags for the property</param>
    InterfaceProperty(Platform::String ^ name, Platform::String ^ signature, uint8_t access);

    /// <summary>
    /// Name of the property
    /// </summary>
    property Platform::String ^ Name
    {
        Platform::String ^  get();
    }

    /// <summary>
    ///Signature of the property
    /// </summary>
    property Platform::String ^ Signature
    {
        Platform::String ^  get();
    }

    /// <summary>
    ///Access flags for the property
    /// </summary>
    property uint8_t Access
    {
        uint8_t get();
    }

  private:
    friend ref class InterfaceDescription;
    InterfaceProperty(const ajn::InterfaceDescription::Property * interfaceProperty);
    ~InterfaceProperty();

    qcc::ManagedObj<_InterfaceProperty>* _mProperty;
    _InterfaceProperty* _property;
};

}
// InterfaceProperty.h
