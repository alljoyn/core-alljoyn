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

#include "InterfaceMember.h"

#include <InterfaceDescription.h>
#include <BusAttachment.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

InterfaceMember::InterfaceMember(InterfaceDescription ^ iface, AllJoynMessageType type, Platform::String ^ name,
                                 Platform::String ^ signature, Platform::String ^ returnSignature, Platform::String ^ argNames,
                                 uint8_t annotation, Platform::String ^ accessPerms)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get unmanaged InterfaceDescription
        ajn::InterfaceDescription* id = *(iface->_interfaceDescr);
        // Check id for invalid values
        if (NULL == id) {
            status = ER_FAIL;
            break;
        }
        // Check name for invalid values
        if (nullptr == name) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check signature for invalid value
        if (nullptr == signature) {
            status = ER_BAD_ARG_4;
            break;
        }
        // Convert signature to qcc::String
        qcc::String strSignature = PlatformToMultibyteString(signature);
        // Check for conversion error
        if (strSignature.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check returnSignature for invalid values
        if (nullptr == returnSignature) {
            status = ER_BAD_ARG_5;
            break;
        }
        // Convert returnSignature to qcc::String
        qcc::String strReturnSignature = PlatformToMultibyteString(returnSignature);
        // Check for conversion error
        if (strReturnSignature.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check argNames for invalid values
        if (nullptr == argNames) {
            status = ER_BAD_ARG_6;
            break;
        }
        // Convert argNames to qcc::String
        qcc::String strArgNames = PlatformToMultibyteString(argNames);
        // Check for conversion error
        if (strArgNames.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert accessPerms to qcc::String
        qcc::String strAccessPerms = PlatformToMultibyteString(accessPerms);
        // Check for conversion error
        if (nullptr != accessPerms && strAccessPerms.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create interfacemember managed object
        const char* name = strName.c_str();
        ajn::AllJoynMessageType ajMsgType = (ajn::AllJoynMessageType)(int)type;
        const char* signature = strSignature.c_str();
        const char* retSignature = strReturnSignature.c_str();
        const char* argNames = strArgNames.c_str();
        const char* accessPerms = strAccessPerms.c_str();
        _mMember = new qcc::ManagedObj<_InterfaceMember>(id, ajMsgType, name,
                                                         signature, retSignature, argNames,
                                                         annotation, accessPerms);
        if (NULL == _mMember) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store _InterfaceMember pointer for convenience
        _member = &(**_mMember);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceMember::InterfaceMember(const ajn::InterfaceDescription::Member* interfaceMember)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check interfaceMember for invalid values
        if (NULL == interfaceMember) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create _InterfaceMember managed object
        _mMember = new qcc::ManagedObj<_InterfaceMember>(interfaceMember);
        if (NULL == _mMember) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store _InterfaceMember pointer for convenience
        _member = &(**_mMember);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceMember::~InterfaceMember()
{
    // Delete managed object to adjust ref count
    if (NULL != _mMember) {
        delete _mMember;
        _mMember = NULL;
        _member = NULL;
    }
}

InterfaceDescription ^ InterfaceMember::Interface::get()
{
    ::QStatus status = ER_OK;
    InterfaceDescription ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _member->_eventsAndProperties->Interface) {
            // Convert iface to wrapped version
            result = ref new InterfaceDescription(_member->_member->iface);
            // Check for allocation error
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _member->_eventsAndProperties->Interface = result;
        } else {
            // Return Interface
            result = _member->_eventsAndProperties->Interface;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

AllJoynMessageType InterfaceMember::MemberType::get()
{
    ::QStatus status = ER_OK;
    AllJoynMessageType result = (AllJoynMessageType)(int)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((AllJoynMessageType)(int)-1 == _member->_eventsAndProperties->MemberType) {
            // Convert memberType
            result = (AllJoynMessageType)(int)_member->_member->memberType;
            // Store the result
            _member->_eventsAndProperties->MemberType = result;
        } else {
            // Return MemberType
            result = _member->_eventsAndProperties->MemberType;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceMember::Name::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _member->_eventsAndProperties->Name) {
            qcc::String strName = _member->_member->name;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strName.c_str());
            // Check for conversion error
            if (nullptr == result && !strName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _member->_eventsAndProperties->Name = result;
        } else {
            // Return Name
            result = _member->_eventsAndProperties->Name;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceMember::Signature::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _member->_eventsAndProperties->Signature) {
            qcc::String strSignature = _member->_member->signature;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strSignature.c_str());
            // Check for conversion error
            if (nullptr == result && !strSignature.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _member->_eventsAndProperties->Signature = result;
        } else {
            // Return Signature
            result = _member->_eventsAndProperties->Signature;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceMember::ReturnSignature::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped value already exists
        if (nullptr == _member->_eventsAndProperties->ReturnSignature) {
            qcc::String strReturnSignature = _member->_member->returnSignature;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strReturnSignature.c_str());
            // Check for conversion failure
            if (nullptr == result && !strReturnSignature.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _member->_eventsAndProperties->ReturnSignature = result;
        } else {
            // Return ReturnSignature
            result = _member->_eventsAndProperties->ReturnSignature;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceMember::ArgNames::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _member->_eventsAndProperties->ArgNames) {
            qcc::String strArgNames = _member->_member->argNames;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strArgNames.c_str());
            // Check for conversion error
            if (nullptr == result && !strArgNames.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _member->_eventsAndProperties->ArgNames = result;
        } else {
            // Return ArgNames
            result = _member->_eventsAndProperties->ArgNames;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceMember::AccessPerms::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped values already exists
        if (nullptr == _member->_eventsAndProperties->AccessPerms) {
            qcc::String strAccessPerms = _member->_member->accessPerms;
            // Convert value to Platform::String
            result = MultibyteToPlatformString(strAccessPerms.c_str());
            // Check for conversion error
            if (nullptr == result && !strAccessPerms.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _member->_eventsAndProperties->AccessPerms = result;
        } else {
            // Return AccessPerms
            result = _member->_eventsAndProperties->AccessPerms;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_InterfaceMember::_InterfaceMember(const ajn::InterfaceDescription* iface, ajn::AllJoynMessageType type, const char* name,
                                   const char* signature, const char* returnSignature, const char* argNames,
                                   uint8_t annotation, const char* accessPerms)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the internal private ref class
        _eventsAndProperties = ref new __InterfaceMember();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create Member
        _member = new ajn::InterfaceDescription::Member(iface, type, name,
                                                        signature, returnSignature, argNames,
                                                        annotation, accessPerms);
        // Check for allocation error
        if (NULL == _member) {
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

_InterfaceMember::_InterfaceMember(const ajn::InterfaceDescription::Member* member)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the internal private ref class
        _eventsAndProperties = ref new __InterfaceMember();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check member for invalid values
        if (NULL == member) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create Member
        _member = new ajn::InterfaceDescription::Member(member->iface, member->memberType, member->name.c_str(),
                                                        member->signature.c_str(), member->returnSignature.c_str(), member->argNames.c_str(),
                                                        0, member->accessPerms.c_str());
        // Check for allocation error
        if (NULL == _member) {
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

_InterfaceMember::~_InterfaceMember()
{
    _eventsAndProperties = nullptr;
    // Clean up allocated members
    if (NULL != _member) {
        delete _member;
        _member = NULL;
    }
}

_InterfaceMember::operator ajn::InterfaceDescription::Member * ()
{
    return _member;
}

__InterfaceMember::__InterfaceMember()
{
    Interface = nullptr;
    MemberType = (AllJoynMessageType)(int)-1;
    Name = nullptr;
    Signature = nullptr;
    ReturnSignature = nullptr;
    ArgNames = nullptr;
    Annotation = (uint8_t)-1;
    AccessPerms = nullptr;
}

__InterfaceMember::~__InterfaceMember()
{
    Interface = nullptr;
    MemberType = (AllJoynMessageType)(int)-1;
    Name = nullptr;
    Signature = nullptr;
    ReturnSignature = nullptr;
    ArgNames = nullptr;
    Annotation = (uint8_t)-1;
    AccessPerms = nullptr;
}

}
