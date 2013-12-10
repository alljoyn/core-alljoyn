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

#include "InterfaceProperty.h"

#include <BusAttachment.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

InterfaceProperty::InterfaceProperty(Platform::String ^ name, Platform::String ^ signature, uint8_t access)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check name for invalid values
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check signature for invalid values
        if (nullptr == signature) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert signature to qcc::String
        qcc::String strSignature = PlatformToMultibyteString(signature);
        // Check for conversion error
        if (strSignature.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }

        // Create managed _InterfaceProperty
        const char* name = strName.c_str();
        const char* signature = strSignature.c_str();
        _mProperty = new qcc::ManagedObj<_InterfaceProperty>(name, signature, access);
        // Check for allocation error
        if (NULL == _mProperty) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a pointer to _InterfaceProperty for convenience
        _property = &(**_mProperty);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceProperty::InterfaceProperty(const ajn::InterfaceDescription::Property* interfaceProperty)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check interfaceProperty for invalid values
        if (NULL == interfaceProperty) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create _InterfaceProperty managed obj
        const char* propName = interfaceProperty->name.c_str();
        const char* propSignature = interfaceProperty->signature.c_str();
        _mProperty = new qcc::ManagedObj<_InterfaceProperty>(propName, propSignature, interfaceProperty->access);
        // Check for allocation error
        if (NULL == _mProperty) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a poitner to _InterfaceProperty for convenience
        _property = &(**_mProperty);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceProperty::~InterfaceProperty()
{
    // Delete the _InterfaceProperty managed object to adjust ref count
    if (NULL != _mProperty) {
        delete _mProperty;
        _mProperty = NULL;
        _property = NULL;
    }
}

Platform::String ^ InterfaceProperty::Name::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _property->_eventsAndProperties->Name) {
            qcc::String strName = _property->_property->name;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strName.c_str());
            // Check for conversion error
            if (nullptr == result && !strName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _property->_eventsAndProperties->Name = result;
        } else {
            // Return Name
            result = _property->_eventsAndProperties->Name;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceProperty::Signature::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapper value already exists
        if (nullptr == _property->_eventsAndProperties->Signature) {
            qcc::String strSignature = _property->_property->signature;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strSignature.c_str());
            // Check for conversion error
            if (nullptr == result && !strSignature.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _property->_eventsAndProperties->Signature = result;
        } else {
            // Return Signature
            result = _property->_eventsAndProperties->Signature;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint8_t InterfaceProperty::Access::get()
{
    ::QStatus status = ER_OK;
    uint8_t result = (uint8_t)-1;

    while (true) {
        // Check the wrapped value already exists
        if ((uint8_t)-1 == _property->_eventsAndProperties->Access) {
            result = _property->_property->access;
            // Store the result
            _property->_eventsAndProperties->Access = result;
        } else {
            // Return Access
            result = _property->_eventsAndProperties->Access;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_InterfaceProperty::_InterfaceProperty(const char* name, const char* signature, uint8_t access)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the private ref class
        _eventsAndProperties = ref new __InterfaceProperty();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create the Property
        _property = new ajn::InterfaceDescription::Property(name, signature, access);
        // Check for allocation error
        if (NULL == _property) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_InterfaceProperty::_InterfaceProperty(const ajn::InterfaceDescription::Property* property)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create private ref class
        _eventsAndProperties = ref new __InterfaceProperty();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check property for invalid values
        if (NULL == property) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create Property
        _property = new ajn::InterfaceDescription::Property(property->name.c_str(), property->signature.c_str(), property->access);
        if (NULL == _property) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_InterfaceProperty::~_InterfaceProperty()
{
    _eventsAndProperties = nullptr;
    // Clean up allocated Property
    if (NULL != _property) {
        delete _property;
        _property = NULL;
    }
}

_InterfaceProperty::operator ajn::InterfaceDescription::Property * ()
{
    return _property;
}

__InterfaceProperty::__InterfaceProperty()
{
    Name = nullptr;
    Signature = nullptr;
    Access = (uint8_t)-1;
}

__InterfaceProperty::~__InterfaceProperty()
{
    Name = nullptr;
    Signature = nullptr;
    Access = (uint8_t)-1;
}

}
