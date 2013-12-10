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

#include "MessageHeaderFields.h"

#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

MessageHeaderFields::MessageHeaderFields(const ajn::HeaderFields* headers)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check headers for invalid values
        if (NULL == headers) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create _MessageHeaderFields
        _MessageHeaderFields* hf = new _MessageHeaderFields(headers);
        // Check for allocation error
        if (NULL == hf) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Attach hf to _MessageHeaderFields managed object
        _mMessageHeaderFields = new qcc::ManagedObj<_MessageHeaderFields>(hf);
        // Check for allocation error
        if (NULL == _mMessageHeaderFields) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _MessageHeaderFields for convenience
        _messageHeaderFields = &(**_mMessageHeaderFields);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MessageHeaderFields::MessageHeaderFields(const qcc::ManagedObj<_MessageHeaderFields>* headers)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check headers for invalid values
        if (NULL == headers) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach headers to _MessageHeaderFields managed object
        _mMessageHeaderFields = new qcc::ManagedObj<_MessageHeaderFields>(*headers);
        // Check for allocation error
        if (NULL == _mMessageHeaderFields) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _MessageHeaderFields for convenience
        _messageHeaderFields = &(**_mMessageHeaderFields);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MessageHeaderFields::~MessageHeaderFields()
{
    // Delete _MessageHeaderFields managed object to adjust ref count
    if (NULL != _mMessageHeaderFields) {
        delete _mMessageHeaderFields;
        _mMessageHeaderFields = NULL;
        _messageHeaderFields = NULL;
    }
}

Platform::String ^ MessageHeaderFields::ConvertToString(uint32_t indent)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Call the real API
        qcc::String ret = _messageHeaderFields->ToString(indent);
        // Convert return value to Platform::String
        result = MultibyteToPlatformString(ret.c_str());
        // Check for conversion error
        if (nullptr == result && !ret.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::Array<MsgArg ^> ^ MessageHeaderFields::Field::get()
{
    ::QStatus status = ER_OK;
    Platform::Array<MsgArg ^> ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _messageHeaderFields->_eventsAndProperties->Field) {
            // Allocate MsgArg array large enough to hold each field type
            result = ref new Platform::Array<MsgArg ^>(ajn::ALLJOYN_HDR_FIELD_UNKNOWN);
            // Check for allocation error
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            for (int i = 0; i < ajn::ALLJOYN_HDR_FIELD_UNKNOWN; i++) {
                // Create the wrapped MsgArg
                MsgArg ^ tempArg = ref new MsgArg(&(_messageHeaderFields->field[i]));
                // Check for allocation error
                if (nullptr == tempArg) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store the wrapped MsgArg
                result[i] = tempArg;
            }
            // Store the result
            _messageHeaderFields->_eventsAndProperties->Field = result;
        } else {
            // Return Field
            result = _messageHeaderFields->_eventsAndProperties->Field;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::Array<bool> ^ MessageHeaderFields::Compressible::get()
{
    ::QStatus status = ER_OK;
    Platform::Array<bool> ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _messageHeaderFields->_eventsAndProperties->Compressible) {
            // Allocate array to hold each field value
            result = ref new Platform::Array<bool>(ajn::ALLJOYN_HDR_FIELD_UNKNOWN + 1);
            // Check for allocation error
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            for (int i = 0; i < ajn::ALLJOYN_HDR_FIELD_UNKNOWN + 1; i++) {
                // Store the field result
                result[i] = _messageHeaderFields->Compressible[i];
            }
            // Store the result
            _messageHeaderFields->_eventsAndProperties->Compressible = result;
        } else {
            // Return Compressible
            result = _messageHeaderFields->_eventsAndProperties->Compressible;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::Array<AllJoynTypeId> ^ MessageHeaderFields::FieldType::get()
{
    ::QStatus status = ER_OK;
    Platform::Array<AllJoynTypeId> ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _messageHeaderFields->_eventsAndProperties->FieldType) {
            // Allocate array to hold each field value
            result = ref new Platform::Array<AllJoynTypeId>(ajn::ALLJOYN_HDR_FIELD_UNKNOWN + 1);
            // Check for allocation error
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            for (int i = 0; i < ajn::ALLJOYN_HDR_FIELD_UNKNOWN + 1; i++) {
                // Store the fieldtype result
                result[i] = (AllJoynTypeId)(int)_messageHeaderFields->FieldType[i];
            }
            // Store the result
            _messageHeaderFields->_eventsAndProperties->FieldType = result;
        } else {
            // Return FieldType
            result = _messageHeaderFields->_eventsAndProperties->FieldType;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_MessageHeaderFields::_MessageHeaderFields(const ajn::HeaderFields* headers)
    : ajn::HeaderFields(*headers)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create internal ref class
        _eventsAndProperties = ref new __MessageHeaderFields();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
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

_MessageHeaderFields::~_MessageHeaderFields()
{
    _eventsAndProperties = nullptr;
}

__MessageHeaderFields::__MessageHeaderFields()
{
    Field = nullptr;
    Compressible = nullptr;
    FieldType = nullptr;
}

__MessageHeaderFields::~__MessageHeaderFields()
{
    Field = nullptr;
    Compressible = nullptr;
    FieldType = nullptr;
}

}
