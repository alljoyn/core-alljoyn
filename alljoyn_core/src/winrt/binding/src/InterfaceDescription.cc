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

#include "InterfaceDescription.h"

#include <InterfaceMember.h>
#include <InterfaceProperty.h>
#include <BusAttachment.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <Collection.h>
#include <AllJoynException.h>

namespace AllJoyn {

InterfaceDescription::InterfaceDescription(const ajn::InterfaceDescription* interfaceDescr)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in interfaceDescr
        if (NULL == interfaceDescr) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create _InterfaceDescription managed object
        _mInterfaceDescr = new qcc::ManagedObj<_InterfaceDescription>(interfaceDescr);
        // Check for allocation error
        if (NULL == _mInterfaceDescr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a pointer to _InterfaceDescription for convenience
        _interfaceDescr = &(**_mInterfaceDescr);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceDescription::InterfaceDescription(const qcc::ManagedObj<_InterfaceDescription>* interfaceDescr)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in interfaceDescr
        if (NULL == interfaceDescr) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attached to managed object
        _mInterfaceDescr = new qcc::ManagedObj<_InterfaceDescription>(*interfaceDescr);
        if (NULL == _mInterfaceDescr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a pointer to _InterfaceDescription for convenience
        _interfaceDescr = &(**_mInterfaceDescr);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceDescription::~InterfaceDescription()
{
    // Delete the managed _InterfaceDescription to adjust ref count
    if (NULL != _mInterfaceDescr) {
        delete _mInterfaceDescr;
        _mInterfaceDescr = NULL;
        _interfaceDescr = NULL;
    }
}

void InterfaceDescription::AddMember(AllJoynMessageType type,
                                     Platform::String ^ name,
                                     Platform::String ^ inputSig,
                                     Platform::String ^ outSig,
                                     Platform::String ^ argNames,
                                     uint8_t annotation,
                                     Platform::String ^ accessPerms)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check name for invalid values
        if (nullptr == name) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert inputSig to qcc::String
        qcc::String strInputSig = PlatformToMultibyteString(inputSig);
        // Check inputSig conversion error
        if (nullptr != inputSig && strInputSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert outSig to qcc::String
        qcc::String strOutSig = PlatformToMultibyteString(outSig);
        // Check for conversion error
        if (nullptr != outSig && strOutSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check argNames for invalid values
        if (nullptr == argNames) {
            status = ER_BAD_ARG_5;
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
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddMember((ajn::AllJoynMessageType)(int)type, strName.c_str(), strInputSig.c_str(), strOutSig.c_str(),
                                                                           strArgNames.c_str(), annotation, strAccessPerms.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void InterfaceDescription::AddMemberAnnotation(Platform::String ^ member, Platform::String ^ name, Platform::String ^ value)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert member to qcc::String
        qcc::String strMember = PlatformToMultibyteString(member);
        // Check for conversion error
        if (nullptr != member && strMember.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert value to qcc::String
        qcc::String strValue = PlatformToMultibyteString(value);
        // Check for conversion error
        if (nullptr != value && strValue.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddMemberAnnotation(strMember.c_str(), strName, strValue);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Platform::String ^ InterfaceDescription::GetMemberAnnotation(Platform::String ^ member, Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Convert member to qcc::String
        qcc::String strMember = PlatformToMultibyteString(member);
        // Check for conversion error
        if (nullptr != member && strMember.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        qcc::String strValue;
        // Call the real API
        if (((ajn::InterfaceDescription*)*_interfaceDescr)->GetMemberAnnotation(strMember.c_str(), strName, strValue)) {
            // Convert result to Platform::String
            result = MultibyteToPlatformString(strValue.c_str());
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

InterfaceMember ^ InterfaceDescription::GetMember(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    InterfaceMember ^ im = nullptr;

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
        // Call the real API
        const ajn::InterfaceDescription::Member* member = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetMember(strName.c_str());
        if (NULL == member) {
            status = ER_FAIL;
            break;
        }
        // Convert member to wrapped InterfaceMember
        im = ref new InterfaceMember(member);
        // Check for allocation error
        if (nullptr == im) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return im;
}

uint32_t InterfaceDescription::GetMembers(Platform::WriteOnlyArray<InterfaceMember ^> ^ members)
{
    ::QStatus status = ER_OK;
    ajn::InterfaceDescription::Member** memberArray = NULL;
    size_t result = -1;

    while (true) {
        // Check for invalid values in members
        if (nullptr != members &&  members->Length >  0) {
            // Create a unmanaged Member array
            memberArray = new ajn::InterfaceDescription::Member * [members->Length];
            // Check for allocation error
            if (NULL == memberArray) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
        }
        // Call the real API
        result = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetMembers((const ajn::InterfaceDescription::Member**)memberArray, (nullptr != members) ? members->Length : 0);
        if (result > 0 && NULL != memberArray) {
            for (int i = 0; i < result; i++) {
                // Convert to wrapped InterfaceMember
                InterfaceMember ^ tempMember = ref new InterfaceMember(memberArray[i]);
                // Check for allocation error
                if (nullptr == tempMember) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store the result
                members[i] = tempMember;
            }
        }
        break;
    }

    // Delete temporary memberArray used for conversion
    if (NULL != memberArray) {
        delete [] memberArray;
        memberArray = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

bool InterfaceDescription::HasMember(Platform::String ^ name, Platform::String ^ inSig, Platform::String ^ outSig)
{
    ::QStatus status = ER_OK;
    bool result = false;

    while (true) {
        // Check name for invalid values
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion failure
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert inSig to qcc::String
        qcc::String strInputSig = PlatformToMultibyteString(inSig);
        // Check for conversion error
        if (nullptr != inSig && strInputSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert outSig to qcc::String
        qcc::String strOutSig = PlatformToMultibyteString(outSig);
        // Check for conversion error
        if (nullptr != outSig && strOutSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        result = ((ajn::InterfaceDescription*)*_interfaceDescr)->HasMember(strName.c_str(), strInputSig.c_str(), strOutSig.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void InterfaceDescription::AddMethod(Platform::String ^ name,
                                     Platform::String ^ inputSig,
                                     Platform::String ^ outSig,
                                     Platform::String ^ argNames,
                                     uint8_t annotation,
                                     Platform::String ^ accessPerms)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check name for invalid values
        if (nullptr == name) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion failure
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert inputSig to qcc::String
        qcc::String strInputSig = PlatformToMultibyteString(inputSig);
        // Check for conversion failure
        if (nullptr != inputSig && strInputSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert outSig to qcc::String
        qcc::String strOutSig = PlatformToMultibyteString(outSig);
        // Check for conversion failure
        if (nullptr != outSig && strOutSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in argNames
        if (nullptr == argNames) {
            status = ER_BAD_ARG_5;
            break;
        }
        // Convert argNames to qcc::String
        qcc::String strArgNames = PlatformToMultibyteString(argNames);
        // Check for conversion failure
        if (strArgNames.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert accessPerms to qcc::String
        qcc::String strAccessPerms = PlatformToMultibyteString(accessPerms);
        // Check for conversion failure
        if (nullptr != accessPerms && strAccessPerms.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddMethod(strName.c_str(), strInputSig.c_str(), strOutSig.c_str(),
                                                                           strArgNames.c_str(), annotation, strAccessPerms.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceMember ^ InterfaceDescription::GetMethod(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    InterfaceMember ^ im = nullptr;

    while (true) {
        // Check for invalid values in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion failure
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call thre real API
        const ajn::InterfaceDescription::Member* member = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetMethod(strName.c_str());
        // Check for API failure
        if (NULL == member) {
            status = ER_FAIL;
            break;
        }
        // Create wrapped InterfaceMember
        im = ref new InterfaceMember(member);
        // Check for allocation error
        if (nullptr == im) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return im;
}

void InterfaceDescription::AddSignal(Platform::String ^ name, Platform::String ^ sig, Platform::String ^ argNames, uint8_t annotation, Platform::String ^ accessPerms)
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
        // Check sid for invalid values
        if (nullptr == sig) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert sig to qcc::String
        qcc::String strSig = PlatformToMultibyteString(sig);
        // Check for conversion error
        if (strSig.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check argNames for invalid values
        if (nullptr == argNames) {
            status = ER_BAD_ARG_3;
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
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddSignal(strName.c_str(), strSig.c_str(), strArgNames.c_str(), annotation, strAccessPerms.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

InterfaceMember ^ InterfaceDescription::GetSignal(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    InterfaceMember ^ im = nullptr;

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
        // Call the real API
        const ajn::InterfaceDescription::Member* member = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetSignal(strName.c_str());
        // Check for API failure
        if (NULL == member) {
            status = ER_FAIL;
            break;
        }
        // Convert member to wrapped InterfaceMember
        im = ref new InterfaceMember(member);
        // Check for allocation error
        if (nullptr == im) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return im;
}

InterfaceProperty ^ InterfaceDescription::GetProperty(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    InterfaceProperty ^ ip = nullptr;

    while (true) {
        // Chek name for invalid values
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
        // Call the real API
        const ajn::InterfaceDescription::Property* property = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetProperty(strName.c_str());
        // Check for API failure
        if (NULL == property) {
            status = ER_FAIL;
            break;
        }
        // Convert property to wrapped InterfaceProperty
        ip = ref new InterfaceProperty(property);
        // Check for allocation error
        if (nullptr == ip) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return ip;
}

uint32_t InterfaceDescription::GetProperties(Platform::WriteOnlyArray<InterfaceProperty ^> ^ props)
{
    ::QStatus status = ER_OK;
    ajn::InterfaceDescription::Property** propertyArray = NULL;
    size_t result = -1;

    while (true) {
        // Check props for invalid vlaues
        if (nullptr != props && props->Length > 0) {
            // Allocate temporary array for properties
            propertyArray = new ajn::InterfaceDescription::Property * [props->Length];
            // Check for allocation error
            if (NULL == propertyArray) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
        }
        // Call the real API
        result = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetProperties((const ajn::InterfaceDescription::Property**)propertyArray, (nullptr != props) ? props->Length : 0);
        if (result > 0 && NULL != propertyArray) {
            for (int i = 0; i < result; i++) {
                // Create wrapped InterfaceProperty from unmanaged value
                InterfaceProperty ^ tempProperty = ref new InterfaceProperty(propertyArray[i]);
                // Check for allocation error
                if (nullptr == tempProperty) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store the result
                props[i] = tempProperty;
            }
        }
        break;
    }

    // Delete temporary property array
    if (NULL != propertyArray) {
        delete [] propertyArray;
        propertyArray = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void InterfaceDescription::AddProperty(Platform::String ^ name, Platform::String ^ signature, uint8_t access)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check name for invalid alues
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
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddProperty(strName.c_str(), strSignature.c_str(), access);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void InterfaceDescription::AddPropertyAnnotation(Platform::String ^ member, Platform::String ^ name, Platform::String ^ value)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert member to qcc::String
        qcc::String strMember = PlatformToMultibyteString(member);
        // Check for conversion error
        if (nullptr != member && strMember.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert value to qcc::String
        qcc::String strValue = PlatformToMultibyteString(value);
        // Check for conversion error
        if (nullptr != value && strValue.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddPropertyAnnotation(strMember.c_str(), strName, strValue);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Platform::String ^ InterfaceDescription::GetPropertyAnnotation(Platform::String ^ member, Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Convert member to qcc::String
        qcc::String strMember = PlatformToMultibyteString(member);
        // Check for conversion error
        if (nullptr != member && strMember.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        qcc::String strValue;
        // Call the real API
        if (((ajn::InterfaceDescription*)*_interfaceDescr)->GetPropertyAnnotation(strMember.c_str(), strName, strValue)) {
            // Convert result to Platform::String
            result = MultibyteToPlatformString(strValue.c_str());
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

bool InterfaceDescription::HasProperty(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    bool result = false;

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
        // Call the real API
        result = ((ajn::InterfaceDescription*)*_interfaceDescr)->HasProperty(strName.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

bool InterfaceDescription::HasProperties()
{
    // Call the real API
    return ((ajn::InterfaceDescription*)*_interfaceDescr)->HasProperties();
}

Platform::String ^ InterfaceDescription::Introspect(uint32_t indent)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Call the real API
        qcc::String strResult = ((ajn::InterfaceDescription*)*_interfaceDescr)->Introspect(indent);
        // Convert the result to Platform::String
        result = MultibyteToPlatformString(strResult.c_str());
        // Check for conversion failure
        if (nullptr == result && !strResult.empty()) {
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

void InterfaceDescription::Activate()
{
    // Call the real API
    ((ajn::InterfaceDescription*)*_interfaceDescr)->Activate();
}

bool InterfaceDescription::IsSecure()
{
    // Call the real API
    return ((ajn::InterfaceDescription*)*_interfaceDescr)->IsSecure();
}

void InterfaceDescription::AddAnnotation(Platform::String ^ name, Platform::String ^ value)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion error
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert value to qcc::String
        qcc::String strValue = PlatformToMultibyteString(value);
        // Check for conversion error
        if (nullptr != value && strValue.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::InterfaceDescription*)*_interfaceDescr)->AddAnnotation(strName, strValue);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

uint32_t InterfaceDescription::GetAnnotations(Platform::WriteOnlyArray<Platform::String ^> ^ names, Platform::WriteOnlyArray<Platform::String ^> ^ values, uint32_t size)
{
    ::QStatus status = ER_OK;
    qcc::String* namesArray = NULL;
    qcc::String* valuesArray = NULL;
    size_t result = -1;
    uint32_t numberToGet = size;

    while (true) {
        if (nullptr != names && nullptr != values) {
            // retrieve only as many as there is space allocated
            numberToGet = std::min(names->Length, values->Length);
            numberToGet = std::min(numberToGet, size);

            if (numberToGet > 0) {
                namesArray = new qcc::String[numberToGet];
                if (NULL == namesArray) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                valuesArray = new qcc::String[numberToGet];
                if (NULL == valuesArray) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
            }
        }

        result = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetAnnotations(namesArray, valuesArray, numberToGet);
        if (result > 0 && NULL != namesArray && NULL != valuesArray) {
            for (int i = 0; i < result; i++) {
                names[i] = MultibyteToPlatformString(namesArray[i].c_str());
                values[i] = MultibyteToPlatformString(valuesArray[i].c_str());
            }
        }
        break;
    }

    if (NULL != namesArray) {
        delete [] namesArray;
        namesArray = NULL;
    }

    if (NULL != valuesArray) {
        delete [] valuesArray;
        valuesArray = NULL;
    }

    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;

}

Platform::String ^ InterfaceDescription::GetAnnotation(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for conversion failure
        if (nullptr != name && strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        qcc::String strValue;
        // Call the real API
        if (((ajn::InterfaceDescription*)*_interfaceDescr)->GetAnnotation(strName, strValue)) {
            // Convert result to Platform::String
            result = MultibyteToPlatformString(strValue.c_str());
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ InterfaceDescription::Name::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _interfaceDescr->_eventsAndProperties->Name) {
            // Call the real API
            qcc::String name = ((ajn::InterfaceDescription*)*_interfaceDescr)->GetName();
            // Convert result to Platform::String
            result = MultibyteToPlatformString(name.c_str());
            // Check for conversion error
            if (nullptr == result && !name.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _interfaceDescr->_eventsAndProperties->Name = result;
        } else {
            // Return Name
            result = _interfaceDescr->_eventsAndProperties->Name;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_InterfaceDescription::_InterfaceDescription(const ajn::InterfaceDescription* interfaceDescr)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the private ref class
        _eventsAndProperties = ref new __InterfaceDescription();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        _interfaceDescr = interfaceDescr;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_InterfaceDescription::~_InterfaceDescription()
{
    _eventsAndProperties = nullptr;
}

_InterfaceDescription::operator ajn::InterfaceDescription * ()
{
    return (ajn::InterfaceDescription*)_interfaceDescr;
}

__InterfaceDescription::__InterfaceDescription()
{
    Name = nullptr;
}

__InterfaceDescription::~__InterfaceDescription()
{
    Name = nullptr;
}

}
