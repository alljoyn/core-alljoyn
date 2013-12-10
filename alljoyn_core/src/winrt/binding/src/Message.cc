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

#include "Message.h"

#include <MsgArg.h>
#include <MessageHeaderFields.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

Message::Message(const ajn::Message* message)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check message for invalid values
        if (NULL == message) {
            status = ER_BAD_ARG_1;
            break;
        }

        _mMessage = new qcc::ManagedObj<_Message>(message);
        if (NULL == _mMessage) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a reference to _Message for convenience
        _message = &(**_mMessage);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Message::~Message()
{
    // Delete _Message managed object to adjust ref count
    if (NULL != _mMessage) {
        delete _mMessage;
        _mMessage = NULL;
        _message = NULL;
    }
}

bool Message::IsBroadcastSignal()
{
    // Call the real API
    return ((ajn::_Message*)*_message)->IsBroadcastSignal();
}

bool Message::IsGlobalBroadcast()
{
    // Call the real API
    return ((ajn::_Message*)*_message)->IsGlobalBroadcast();
}

bool Message::IsSessionless()
{
    // Call the real API
    return ((ajn::_Message*)*_message)->IsSessionless();
}

bool Message::IsExpired(Platform::WriteOnlyArray<uint32_t> ^ tillExpireMS)
{
    ::QStatus status = ER_OK;
    bool result = false;

    while (true) {
        // Check tillExpireMS for invalid values
        if (nullptr == tillExpireMS || tillExpireMS->Length != 1) {
            status = ER_BAD_ARG_1;
            break;
        }
        uint32_t expires = -1;
        // Call the real API
        result = ((ajn::_Message*)*_message)->IsExpired(&expires);
        // Store the result
        tillExpireMS[0] = expires;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

bool Message::IsUnreliable()
{
    // Call the real API
    return ((ajn::_Message*)*_message)->IsUnreliable();
}

bool Message::IsEncrypted()
{
    // Call the real API
    return ((ajn::_Message*)*_message)->IsEncrypted();
}

uint32_t Message::GetArgs(Platform::WriteOnlyArray<MsgArg ^> ^ args)
{
    ::QStatus status = ER_OK;
    size_t result = -1;

    while (true) {
        const ajn::MsgArg* margs = NULL;
        // Call the real API
        ((ajn::_Message*)*_message)->GetArgs(result, margs);
        if (result > 0 && nullptr != args && args->Length > 0) {
            // Convert up to the total length of args specified for output
            for (int i = 0; i < args->Length; i++) {
                // Create the wrapped MsgArg
                MsgArg ^ tempMsgArg = ref new MsgArg(&(margs[i]));
                // Check for allocation error
                if (nullptr == tempMsgArg) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store the result
                args[i] = tempMsgArg;
            }
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

MsgArg ^ Message::GetArg(uint32_t argN)
{
    ::QStatus status = ER_OK;
    MsgArg ^ newArg = nullptr;

    while (true) {
        // Call the real API
        const ajn::MsgArg* arg = ((ajn::_Message*)*_message)->GetArg(argN);
        // Check for error
        if (NULL == arg) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create the wrapped MsgArg
        newArg = ref new MsgArg(arg);
        // Check for allocation error
        if (nullptr == newArg) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return newArg;
}

Platform::String ^ Message::ConvertToString()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Call the real API
        qcc::String ret = ((ajn::_Message*)*_message)->ToString();
        // Convert the result to Platform::String
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

Platform::String ^ Message::GetErrorName(Platform::String ^ errorMessage)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check for invalid values in errorMessage
        if (nullptr == errorMessage) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert errorMessage to qcc::String
        qcc::String strErrorMessage = PlatformToMultibyteString(errorMessage);
        // Check for conversion error
        if (strErrorMessage.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        qcc::String ret = ((ajn::_Message*)*_message)->GetErrorName(&strErrorMessage);
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

Platform::String ^ Message::AuthMechanism::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->AuthMechanism) {
            // Call the real API
            qcc::String strAuthMechanism = ((ajn::_Message*)*_message)->GetAuthMechanism();
            // Convert the result to Platform::String
            result = MultibyteToPlatformString(strAuthMechanism.c_str());
            // Check for conversion error
            if (nullptr == result && !strAuthMechanism.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->AuthMechanism = result;
        } else {
            // Return AuthMechnaism
            result = _message->_eventsAndProperties->AuthMechanism;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

AllJoynMessageType Message::Type::get()
{
    ::QStatus status = ER_OK;
    AllJoynMessageType result = (AllJoynMessageType)(int)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((AllJoynMessageType)(int)-1 == _message->_eventsAndProperties->Type) {
            // Call the real API
            result = (AllJoynMessageType)(int)((ajn::_Message*)*_message)->GetType();
            // Store the result
            _message->_eventsAndProperties->Type = result;
        } else {
            // Return Type
            result = _message->_eventsAndProperties->Type;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint8_t Message::Flags::get()
{
    ::QStatus status = ER_OK;
    uint8_t result = (uint8_t)-1;

    while (true) {
        // Check if the wrapped value already exists
        if ((uint8_t)-1 == _message->_eventsAndProperties->Flags) {
            // Call the real API
            result = ((ajn::_Message*)*_message)->GetFlags();
            // Store the result
            _message->_eventsAndProperties->Flags = result;
        } else {
            // Return Flags
            result = _message->_eventsAndProperties->Flags;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t Message::CallSerial::get()
{
    ::QStatus status = ER_OK;
    uint32_t result = (uint32_t)-1;

    while (true) {
        // Check if the wrapped value already exists
        if ((uint32_t)-1 == _message->_eventsAndProperties->CallSerial) {
            // Call the real API
            result = ((ajn::_Message*)*_message)->GetCallSerial();
            // Store the result
            _message->_eventsAndProperties->CallSerial = result;
        } else {
            // Return CallSerial
            result = _message->_eventsAndProperties->CallSerial;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

MessageHeaderFields ^ Message::HeaderFields::get()
{
    ::QStatus status = ER_OK;
    MessageHeaderFields ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->HeaderFields) {
            // Call the real API
            ajn::HeaderFields headers = ((ajn::_Message*)*_message)->GetHeaderFields();
            // Create the wrapped MessageHeaderFields
            result = ref new MessageHeaderFields(&headers);
            // Check for allocation error
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->HeaderFields = result;
        } else {
            // Return HeaderFields
            result = _message->_eventsAndProperties->HeaderFields;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::Signature::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->Signature) {
            // Call the real API
            qcc::String strSignature = ((ajn::_Message*)*_message)->GetSignature();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strSignature.c_str());
            // Check for conversion error
            if (nullptr == result && !strSignature.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->Signature = result;
        } else {
            // Return Signature
            result = _message->_eventsAndProperties->Signature;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::ObjectPath::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->ObjectPath) {
            // Call the real API
            qcc::String strObjectPath = ((ajn::_Message*)*_message)->GetObjectPath();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strObjectPath.c_str());
            // Check for conversion error
            if (nullptr == result && !strObjectPath.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->ObjectPath = result;
        } else {
            // Return ObjectPath
            result = _message->_eventsAndProperties->ObjectPath;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::Interface::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->Interface) {
            // Call the real API
            qcc::String strInterface = ((ajn::_Message*)*_message)->GetInterface();
            // Convert the return value to Platform::String
            result = MultibyteToPlatformString(strInterface.c_str());
            // Check for conversion error
            if (nullptr == result && !strInterface.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->Interface = result;
        } else {
            // Return Interface
            result = _message->_eventsAndProperties->Interface;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::MemberName::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->MemberName) {
            // Call the real API
            qcc::String strMemberName = ((ajn::_Message*)*_message)->GetMemberName();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strMemberName.c_str());
            // Check for conversion failure
            if (nullptr == result && !strMemberName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->MemberName = result;
        } else {
            // Return MemberName
            result = _message->_eventsAndProperties->MemberName;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t Message::ReplySerial::get()
{
    ::QStatus status = ER_OK;
    uint32_t result = (uint32_t)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((uint32_t)-1 == _message->_eventsAndProperties->ReplySerial) {
            // Call the real API
            result = ((ajn::_Message*)*_message)->GetReplySerial();
            // Store the result
            _message->_eventsAndProperties->ReplySerial = result;
        } else {
            // Return ReplySerial
            result = _message->_eventsAndProperties->ReplySerial;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::Sender::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->Sender) {
            // Call the real API
            qcc::String strSender = ((ajn::_Message*)*_message)->GetSender();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strSender.c_str());
            // Check for conversion error
            if (nullptr == result && !strSender.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->Sender = result;
        } else {
            // Return Sender
            result = _message->_eventsAndProperties->Sender;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::RcvEndpointName::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->RcvEndpointName) {
            // Call the real API
            qcc::String strRcvEndpointName = ((ajn::_Message*)*_message)->GetRcvEndpointName();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strRcvEndpointName.c_str());
            // Check for conversion error
            if (nullptr == result && !strRcvEndpointName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->RcvEndpointName = result;
        } else {
            // Return RcvEndpointName
            result = _message->_eventsAndProperties->RcvEndpointName;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::Destination::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->Destination) {
            // Call the real API
            qcc::String strDestination = ((ajn::_Message*)*_message)->GetDestination();
            // Convert retrun value to Platform::String
            result = MultibyteToPlatformString(strDestination.c_str());
            // Check for conversion failure
            if (nullptr == result && !strDestination.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->Destination = result;
        } else {
            // Return Destination
            result = _message->_eventsAndProperties->Destination;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t Message::CompressionToken::get()
{
    ::QStatus status = ER_OK;
    uint32_t result = (uint32_t)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((uint32_t)-1 == _message->_eventsAndProperties->CompressionToken) {
            // Call the real API
            result = ((ajn::_Message*)*_message)->GetCompressionToken();
            // Store the result
            _message->_eventsAndProperties->CompressionToken = result;
        } else {
            // Return CompressionToken
            result = _message->_eventsAndProperties->CompressionToken;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t Message::SessionId::get()
{
    ::QStatus status = ER_OK;
    uint32_t result = (uint32_t)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((uint32_t)-1 == _message->_eventsAndProperties->SessionId) {
            // Call the real API
            result = ((ajn::_Message*)*_message)->GetSessionId();
            // Store the result
            _message->_eventsAndProperties->SessionId = result;
        } else {
            // Return SessionId
            result = _message->_eventsAndProperties->SessionId;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ Message::Description::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _message->_eventsAndProperties->Description) {
            // Call the real API
            qcc::String strDescription = ((ajn::_Message*)*_message)->Description();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strDescription.c_str());
            // Check for conversion failure
            if (nullptr == result && !strDescription.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _message->_eventsAndProperties->Description = result;
        } else {
            // Return Description
            result = _message->_eventsAndProperties->Description;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t Message::Timestamp::get()
{
    ::QStatus status = ER_OK;
    uint32_t result = (uint32_t)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((uint32_t)-1 == _message->_eventsAndProperties->Timestamp) {
            // Call the real API
            result = ((ajn::_Message*)*_message)->GetTimeStamp();
            // Store the result
            _message->_eventsAndProperties->Timestamp = result;
        } else {
            // Return Timestamp
            result = _message->_eventsAndProperties->Timestamp;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_Message::_Message(const ajn::Message* msg)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create private ref class
        _eventsAndProperties = ref new __Message();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check msg for invalid values
        if (NULL == msg) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach msg as Message managed object
        _mMessage = new ajn::Message(*msg);
        if (NULL == _mMessage) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _Message for convenience
        _message = &(**_mMessage);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_Message::~_Message()
{
    _eventsAndProperties = nullptr;
    // Delete Message managed object to adjust ref count
    if (NULL != _mMessage) {
        delete _mMessage;
        _mMessage = NULL;
        _message = NULL;
    }
}

_Message::operator ajn::Message * ()
{
    return _mMessage;
}

_Message::operator ajn::_Message * ()
{
    return (ajn::_Message*)_message;
}

__Message::__Message()
{
    AuthMechanism = nullptr;
    Type = (AllJoynMessageType)(int)-1;
    Flags = (uint8_t)-1;
    CallSerial = (uint32_t)-1;
    HeaderFields = nullptr;
    Signature = nullptr;
    ObjectPath = nullptr;
    Interface = nullptr;
    MemberName = nullptr;
    ReplySerial = (uint32_t)-1;
    Sender = nullptr;
    RcvEndpointName = nullptr;
    Destination = nullptr;
    CompressionToken = (uint32_t)-1;
    SessionId = (uint32_t)-1;
    Description = nullptr;
    Timestamp = (uint32_t)-1;
}

__Message::~__Message()
{
    AuthMechanism = nullptr;
    Type = (AllJoynMessageType)(int)-1;
    Flags = (uint8_t)-1;
    CallSerial = (uint32_t)-1;
    HeaderFields = nullptr;
    Signature = nullptr;
    ObjectPath = nullptr;
    Interface = nullptr;
    MemberName = nullptr;
    ReplySerial = (uint32_t)-1;
    Sender = nullptr;
    RcvEndpointName = nullptr;
    Destination = nullptr;
    CompressionToken = (uint32_t)-1;
    SessionId = (uint32_t)-1;
    Description = nullptr;
    Timestamp = (uint32_t)-1;
}

}
