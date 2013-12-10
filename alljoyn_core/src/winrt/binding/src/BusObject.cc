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

#include "BusObject.h"

#include <alljoyn/BusObject.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Session.h>
#include <BusAttachment.h>
#include <InterfaceDescription.h>
#include <MsgArg.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <alljoyn/MsgArg.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

BusObject::BusObject(BusAttachment ^ bus, Platform::String ^ path, bool isPlaceholder)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in bus
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check for invalid values in path
        if (nullptr == path) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert path to qcc::String
        qcc::String strPath = PlatformToMultibyteString(path);
        // Check for failed conversion
        if (strPath.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get the unmanaged version
        ajn::BusAttachment* attachment = bus->_busAttachment;

        // Create the managed version of _BusObject
        const char* str1 = strPath.c_str();
        _mBusObject = new qcc::ManagedObj<_BusObject>(bus, *attachment, str1, isPlaceholder);
        // Check for allocation failure
        if (NULL == _mBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a pointer to the _BusObject for convenience
        _busObject = &(**_mBusObject);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusObject::BusObject(const qcc::ManagedObj<_BusObject>* busObject)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in busObject
        if (nullptr == busObject) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach to the managed _BusObject
        _mBusObject = new qcc::ManagedObj<_BusObject>(*busObject);
        // Check for failed allocation
        if (NULL == _mBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a pointer to the _BusObject for convenience
        _busObject = &(**_mBusObject);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusObject::~BusObject()
{
    // Delete the managed _BusObject to adjust ref count
    if (NULL != _mBusObject) {
        delete _mBusObject;
        _mBusObject = NULL;
        _busObject = NULL;
    }
}

void BusObject::EmitPropChanged(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val, ajn::SessionId id)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert ifcName to qcc::String
        qcc::String strIfcName = PlatformToMultibyteString(ifcName);
        // Check for failed conversion
        if (nullptr != ifcName && strIfcName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert propName to qcc::String
        qcc::String strPropName = PlatformToMultibyteString(propName);
        // Check for failed conversion
        if (nullptr != propName && strPropName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get the unmanaged version
        ajn::MsgArg* msgArg = val->_msgArg;
        // Call the real API
        _busObject->EmitPropChanged(strIfcName.c_str(), strPropName.c_str(), *msgArg, id);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::MethodReply(Message ^ msg, const Platform::Array<MsgArg ^> ^ args)
{
    ::QStatus status = ER_OK;
    ajn::MsgArg* msgScratch = NULL;

    while (true) {
        // Check for invalid values in msg
        if (nullptr == msg) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::Message* mMessage = *(msg->_message);
        // Convert MsgArg wrapper array
        size_t argsCount = 0;
        if (nullptr != args & args->Length > 0) {
            argsCount = args->Length;
            // Allocate the scratch pad for conversion
            msgScratch = new ajn::MsgArg[argsCount];
            // Check for allocation failure
            if (NULL == msgScratch) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            for (int i = 0; i < argsCount; i++) {
                // Check for invalid value
                if (nullptr == args[i]) {
                    status = ER_BUFFER_TOO_SMALL;
                    break;
                }
                // Get the unmanaged type
                ajn::MsgArg* arg = args[i]->_msgArg;
                // Do a deep copy
                msgScratch[i] = *arg;
            }
            if (ER_OK != status) {
                break;
            }
        }
        // Call the real API
        status = _busObject->MethodReply(*mMessage, msgScratch, argsCount);
        break;
    }

    // Clean up temporary allocation
    if (NULL != msgScratch) {
        delete [] msgScratch;
        msgScratch = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::MethodReply(Message ^ msg, Platform::String ^ error, Platform::String ^ errorMessage)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in msg
        if (nullptr == msg) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::Message* mMessage = *(msg->_message);
        // Convert error to qcc::String
        qcc::String strError = PlatformToMultibyteString(error);
        // Check for conversion error
        if (nullptr != error && strError.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert errorMessage to qcc::String
        qcc::String strErrorMessage = PlatformToMultibyteString(errorMessage);
        // Check for conversion error
        if (nullptr != errorMessage && strErrorMessage.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busObject->MethodReply(*mMessage, strError.c_str(), strErrorMessage.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::MethodReplyWithQStatus(Message ^ msg, QStatus s)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in msg
        if (nullptr == msg) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::Message* mMessage = *(msg->_message);
        // Call the real API
        status = _busObject->MethodReply(*mMessage, (::QStatus)(int)s);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::Signal(Platform::String ^ destination,
                       ajn::SessionId sessionId,
                       InterfaceMember ^ signal,
                       const Platform::Array<MsgArg ^> ^ args,
                       uint16_t timeToLive,
                       uint8_t flags)
{
    ::QStatus status = ER_OK;
    ajn::MsgArg* msgArgs = NULL;

    while (true) {
        // Convert destination to qcc::String
        qcc::String strDestination = PlatformToMultibyteString(destination);
        // Check for conversion failure
        if (nullptr != destination && strDestination.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in signal
        if (nullptr == signal) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Get the unmanaged value
        ajn::InterfaceDescription::Member* member = *(signal->_member);
        // Convert MsgArg wrapper array
        size_t numArgs = 0;
        if (nullptr != args && args->Length > 0) {
            numArgs = args->Length;
            // Allocate the msgArgs
            msgArgs = new ajn::MsgArg[numArgs];
            // Check for allocation failure
            if (NULL == msgArgs) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            for (int i = 0; i < numArgs; ++i) {
                // Check for invalid values in array
                if (nullptr == args[i]) {
                    status = ER_BUFFER_TOO_SMALL;
                    break;
                }
                // Deep copy the msgarg
                msgArgs[i] = *(args[i]->_msgArg);
            }
            if (ER_OK != status) {
                break;
            }
        }
        // Call the real API
        status = _busObject->Signal(strDestination.c_str(), sessionId, *member, msgArgs, numArgs, timeToLive, flags);
        break;
    }

    // Clean up the temporary allocation
    if (NULL != msgArgs) {
        delete [] msgArgs;
        msgArgs = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}


void BusObject::CancelSessionlessMessageBySN(uint32_t serialNumber)
{
    ::QStatus status = ER_OK;
    status = _busObject->CancelSessionlessMessage(serialNumber);

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::CancelSessionlessMessage(Message ^ msg)
{
    ::QStatus status = ER_OK;
    status = _busObject->CancelSessionlessMessage(msg->CallSerial);

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::AddInterface(InterfaceDescription ^ iface)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in add interface
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged value
        ajn::InterfaceDescription* id = *(iface->_interfaceDescr);
        // Call the real API
        status = _busObject->AddInterface(*id);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusObject::AddMethodHandler(InterfaceMember ^ member, MessageReceiver ^ receiver)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values
        if (nullptr == member) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unamanaged value
        ajn::InterfaceDescription::Member* imember = *(member->_member);
        if (nullptr == receiver) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Perform some sanity on the receiver
        if (!Bus->IsSameBusAttachment(receiver->_receiver->Bus)) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Get the unmanaged MethodHandler
        ajn::MessageReceiver::MethodHandler handler = receiver->_receiver->GetMethodHandler();
        // Call the real API
        status = _busObject->AddMethodHandler(imember, handler, (void*)receiver);
        if (ER_OK == status) {
            // Add an object reference
            AddObjectReference(&(_busObject->_mutex), receiver, &(_busObject->_messageReceiverMap));
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Windows::Foundation::EventRegistrationToken BusObject::Get::add(BusObjectGetHandler ^ handler)
{
    // Add an event handler for Get
    return _busObject->_eventsAndProperties->Get::add(handler);
}

void BusObject::Get::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for Get
    _busObject->_eventsAndProperties->Get::remove(token);
}

QStatus BusObject::Get::raise(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<MsgArg ^> ^ val)
{
    // Invoke an event handler for Get
    return _busObject->_eventsAndProperties->Get::raise(ifcName, propName, val);
}

Windows::Foundation::EventRegistrationToken BusObject::Set::add(BusObjectSetHandler ^ handler)
{
    // Add an event handler for Set
    return _busObject->_eventsAndProperties->Set::add(handler);
}

void BusObject::Set::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for Set
    _busObject->_eventsAndProperties->Set::remove(token);
}

QStatus BusObject::Set::raise(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val)
{
    // Dispatch an event handler for Set
    return _busObject->_eventsAndProperties->Set::raise(ifcName, propName, val);
}

Windows::Foundation::EventRegistrationToken BusObject::GenerateIntrospection::add(BusObjectGenerateIntrospectionHandler ^ handler)
{
    // Add an event handler for GenerateIntrospection
    return _busObject->_eventsAndProperties->GenerateIntrospection::add(handler);
}

void BusObject::GenerateIntrospection::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for GenerateIntrospection
    _busObject->_eventsAndProperties->GenerateIntrospection::remove(token);
}

Platform::String ^ BusObject::GenerateIntrospection::raise(bool deep, uint32_t indent)
{
    // Invoke an event handler for GenerateIntrospection
    return _busObject->_eventsAndProperties->GenerateIntrospection::raise(deep, indent);
}

Windows::Foundation::EventRegistrationToken BusObject::ObjectRegistered::add(BusObjectObjectRegisteredHandler ^ handler)
{
    // Add an event handler for ObjectRegistered
    return _busObject->_eventsAndProperties->ObjectRegistered::add(handler);
}

void BusObject::ObjectRegistered::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for ObjectRegistered
    _busObject->_eventsAndProperties->ObjectRegistered::remove(token);
}

void BusObject::ObjectRegistered::raise(void)
{
    // Dispatch an event handler for ObjectRegistered
    _busObject->_eventsAndProperties->ObjectRegistered::raise();
}

Windows::Foundation::EventRegistrationToken BusObject::ObjectUnregistered::add(BusObjectObjectUnregisteredHandler ^ handler)
{
    // Add an event handler for ObjectUnregistered
    return _busObject->_eventsAndProperties->ObjectUnregistered::add(handler);
}

void BusObject::ObjectUnregistered::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for ObjectUnregistered
    _busObject->_eventsAndProperties->ObjectUnregistered::remove(token);
}

void BusObject::ObjectUnregistered::raise(void)
{
    // Invoke an event handler for ObjectUnregistered
    _busObject->_eventsAndProperties->ObjectUnregistered::raise();
}

Windows::Foundation::EventRegistrationToken BusObject::GetAllProps::add(BusObjectGetAllPropsHandler ^ handler)
{
    // Add an event handler for GetAllProps
    return _busObject->_eventsAndProperties->GetAllProps::add(handler);
}

void BusObject::GetAllProps::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for GetAllProps
    _busObject->_eventsAndProperties->GetAllProps::remove(token);
}

void BusObject::GetAllProps::raise(InterfaceMember ^ member, Message ^ msg)
{
    // Dispatch an event handler for GetAllProps
    _busObject->_eventsAndProperties->GetAllProps::raise(member, msg);
}

Windows::Foundation::EventRegistrationToken BusObject::Introspect::add(BusObjectIntrospectHandler ^ handler)
{
    // Add an event handler for Introspect
    return _busObject->_eventsAndProperties->Introspect::add(handler);
}

void BusObject::Introspect::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Remove an event handler for Introspect
    _busObject->_eventsAndProperties->Introspect::remove(token);
}

void BusObject::Introspect::raise(InterfaceMember ^ member, Message ^ msg)
{
    // Dispatch an event handler for Introspect
    _busObject->_eventsAndProperties->Introspect::raise(member, msg);
}

BusAttachment ^ BusObject::Bus::get()
{
    // Get the value of Bus from the internal ref class
    return _busObject->_eventsAndProperties->Bus;
}

Platform::String ^ BusObject::Name::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if value has already been wrapped
        if (nullptr == _busObject->_eventsAndProperties->Name) {
            // Call the real API
            qcc::String oName = _busObject->GetName();
            // Convert result to Platform::String
            result = MultibyteToPlatformString(oName.c_str());
            // Check for conversion failure
            if (nullptr == result && !oName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busObject->_eventsAndProperties->Name = result;
        } else {
            // Return Name
            result = _busObject->_eventsAndProperties->Name;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ BusObject::Path::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if value has already been wrapped
        if (nullptr == _busObject->_eventsAndProperties->Name) {
            // Call the real API
            qcc::String oPath = _busObject->GetPath();
            // Convert path to Platform::String
            result = MultibyteToPlatformString(oPath.c_str());
            // Check for failed conversion
            if (nullptr == result && !oPath.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busObject->_eventsAndProperties->Path = result;
        } else {
            // Return Path
            result = _busObject->_eventsAndProperties->Path;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

MessageReceiver ^ BusObject::Receiver::get()
{
    // Get the MessageReceiver
    return _busObject->_eventsAndProperties->Receiver;
}

_BusObject::_BusObject(BusAttachment ^ b, ajn::BusAttachment& bus, const char* path, bool isPlaceholder)
    :  ajn::BusObject(bus, path, isPlaceholder)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create internal ref class
        _eventsAndProperties = ref new __BusObject();
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create receiver
        AllJoyn::MessageReceiver ^ receiver =  ref new AllJoyn::MessageReceiver(b);
        if (nullptr == receiver) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store the receiver in the private ref class
        _eventsAndProperties->Receiver = receiver;
        // Set the default Get handler
        _eventsAndProperties->Get += ref new BusObjectGetHandler([&] (Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<MsgArg ^> ^ val)->AllJoyn::QStatus {
                                                                     return DefaultBusObjectGetHandler(ifcName, propName, val);
                                                                 });
        // Set the default Set handler
        _eventsAndProperties->Set += ref new BusObjectSetHandler([&] (Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val)->AllJoyn::QStatus {
                                                                     return DefaultBusObjectSetHandler(ifcName, propName, val);
                                                                 });
        // Set the default GenerateIntrospection handler
        _eventsAndProperties->GenerateIntrospection += ref new BusObjectGenerateIntrospectionHandler([&] (bool deep, size_t indent)->Platform::String ^ {
                                                                                                         return DefaultBusObjectGenerateIntrospectionHandler(deep, indent);
                                                                                                     });
        // Set the ObjectRegistered handler
        _eventsAndProperties->ObjectRegistered += ref new BusObjectObjectRegisteredHandler([&] (void) {
                                                                                               DefaultBusObjectObjectRegisteredHandler();
                                                                                           });
        // Set the ObjectUnregistered handler
        _eventsAndProperties->ObjectUnregistered += ref new BusObjectObjectUnregisteredHandler([&] (void) {
                                                                                                   DefaultBusObjectObjectUnregisteredHandler();
                                                                                               });
        // Set the GetAllProps handler
        _eventsAndProperties->GetAllProps += ref new BusObjectGetAllPropsHandler([&] (InterfaceMember ^ member, Message ^ msg) {
                                                                                     DefaultBusObjectGetAllPropsHandler(member, msg);
                                                                                 });
        // Set the Introspect handler
        _eventsAndProperties->Introspect += ref new BusObjectIntrospectHandler([&] (InterfaceMember ^ member, Message ^ msg) {
                                                                                   DefaultBusObjectIntrospectHandler(member, msg);
                                                                               });
        // Store the busattachment in the private ref class
        _eventsAndProperties->Bus = b;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_BusObject::~_BusObject()
{
    _eventsAndProperties = nullptr;
    _mReceiver = NULL;
    // Clear the map of message receivers associated with this BusObject
    ClearObjectMap(&(this->_mutex), &(this->_messageReceiverMap));
}

QStatus _BusObject::DefaultBusObjectGetHandler(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<MsgArg ^> ^ val)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert ifcName to qcc::String
        qcc::String strIfcName = PlatformToMultibyteString(ifcName);
        // Check for conversion failure
        if (nullptr != ifcName && strIfcName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert propName to qcc::String
        qcc::String strPropName = PlatformToMultibyteString(propName);
        // Check for conversion failure
        if (nullptr != propName && strPropName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        ajn::MsgArg msgArg;
        // Call the real API
        status = ajn::BusObject::Get(strIfcName.c_str(), strPropName.c_str(), msgArg);
        if (ER_OK == status) {
            // Convert to wrapped MsgArg
            MsgArg ^ newArg = ref new MsgArg(&msgArg);
            // Store the result
            val[0] = newArg;
        }
        break;
    }

    return (AllJoyn::QStatus)(int)status;
}

QStatus _BusObject::DefaultBusObjectSetHandler(Platform::String ^ ifcName, Platform::String ^ propName, MsgArg ^ val)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert ifcName to qcc::String
        qcc::String strIfcName = PlatformToMultibyteString(ifcName);
        // Check for conversion failure
        if (nullptr != ifcName && strIfcName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert propName to qcc::String
        qcc::String strPropName = PlatformToMultibyteString(propName);
        // Check for conversion failure
        if (nullptr != propName && strPropName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get the unmanaged version
        ajn::MsgArg* msgArg = val->_msgArg;
        // Call the real API
        status = ajn::BusObject::Set(strIfcName.c_str(), strPropName.c_str(), *msgArg);
        break;
    }

    return (AllJoyn::QStatus)(int)status;
}

Platform::String ^ _BusObject::DefaultBusObjectGenerateIntrospectionHandler(bool deep, uint32_t indent)
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Call the real API
        qcc::String introspected = ajn::BusObject::GenerateIntrospection(deep, indent);
        // Convert the result to Platform::String
        result = MultibyteToPlatformString(introspected.c_str());
        // Check for conversion error
        if (nullptr == result && !introspected.empty()) {
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

void _BusObject::DefaultBusObjectObjectRegisteredHandler(void)
{
    // Call the real API
    ajn::BusObject::ObjectRegistered();
}

void _BusObject::DefaultBusObjectObjectUnregisteredHandler(void)
{
    // Call the real API
    ajn::BusObject::ObjectUnregistered();
}

void _BusObject::DefaultBusObjectGetAllPropsHandler(InterfaceMember ^ member, Message ^ msg)
{
    // Get the unmanaged Member
    ajn::InterfaceDescription::Member* imember = *(member->_member);
    // Get the unmananged Message
    ajn::Message* mMsg = *(msg->_message);
    ajn::Message& m = *mMsg;
    // Call the real API
    ajn::BusObject::GetAllProps(imember, m);
}

void _BusObject::DefaultBusObjectIntrospectHandler(InterfaceMember ^ member, Message ^ msg)
{
    // Get the unmanaged InterfaceDescription
    ajn::InterfaceDescription::Member* imember = *(member->_member);
    // Get the unmanaged Message
    ajn::Message* mMsg = *(msg->_message);
    ajn::Message& m = *mMsg;
    // Call the real API
    ajn::BusObject::Introspect(imember, m);
}

::QStatus _BusObject::AddInterface(const ajn::InterfaceDescription& iface)
{
    // Call the real API
    return ajn::BusObject::AddInterface(iface);
}

::QStatus _BusObject::Get(const char* ifcName, const char* propName, ajn::MsgArg& val)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Convert ifcName to Platform::String
        Platform::String ^ strIfcName = MultibyteToPlatformString(ifcName);
        // Check for conversion error
        if (strIfcName == nullptr && NULL != ifcName && '\0' != ifcName[0]) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert propName to Platform::String
        Platform::String ^ strPropName = MultibyteToPlatformString(propName);
        // Check for conversion error
        if (strPropName == nullptr && NULL != propName && '\0' != propName[0]) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create an out parameter for the result
        Platform::Array<MsgArg ^> ^ msgArgArray = ref new Platform::Array<MsgArg ^>(1);
        // Check for allocation failure
        if (nullptr == msgArgArray) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the Get handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     status = (::QStatus)(int)_eventsAndProperties->Get(strIfcName, strPropName, msgArgArray);
                                                                                                                 }));
        if (ER_OK == status) {
            MsgArg ^ msgArgOut = msgArgArray[0];
            // Get the out msgarg
            ajn::MsgArg* msgArg = msgArgOut->_msgArg;
            // Deep copy the result
            val = *msgArg;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return status;
}

::QStatus _BusObject::Set(const char* ifcName, const char* propName, ajn::MsgArg& val)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Convert ifcName to Platform::String
        Platform::String ^ strIfcName = MultibyteToPlatformString(ifcName);
        // Check for conversion error
        if (strIfcName == nullptr && NULL != ifcName && '\0' != ifcName[0]) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert propName to Platform::String
        Platform::String ^ strPropName = MultibyteToPlatformString(propName);
        // Check for conversion error
        if (strPropName == nullptr && NULL != propName && '\0' != propName[0]) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create a wrapped MsgArg
        MsgArg ^ msgArg = ref new MsgArg(&val);
        if (nullptr == msgArg) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the Set handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     status = (::QStatus)(int)_eventsAndProperties->Set(strIfcName, strPropName, msgArg);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return status;
}

qcc::String _BusObject::GenerateIntrospection(bool deep, uint32_t indent)
{
    ::QStatus status = ER_OK;
    qcc::String result;

    while (true) {
        Platform::String ^ ret = nullptr;
        // Call the GenerateIntrospection handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     ret = _eventsAndProperties->GenerateIntrospection(deep, indent);
                                                                                                                 }));
        // Convert the return value to Platform::String
        result = PlatformToMultibyteString(ret);
        // Check for conversion error
        if (nullptr != ret && result.empty()) {
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

void _BusObject::ObjectRegistered(void)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Call the ObjectRegistered handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->ObjectRegistered();
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusObject::ObjectUnregistered(void)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Call the ObjectUnregistered handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->ObjectUnregistered();
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusObject::GetAllProps(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create a wrapped InterfaceMember
        InterfaceMember ^ imember = ref new InterfaceMember(member);
        // Check for allocation failure
        if (nullptr == imember) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create a wrapped message
        Message ^ m = ref new Message(&msg);
        // Check for allocation failure
        if (nullptr == m) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the GetAllProps handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->GetAllProps(imember, m);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusObject::Introspect(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create a wrapped InterfaceMember
        InterfaceMember ^ imember = ref new InterfaceMember(member);
        // Check for allocation failure
        if (nullptr == imember) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create a wrapped message
        Message ^ m = ref new Message(&msg);
        // Check for allocation failure
        if (nullptr == m) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the Introspect handler through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->Introspect(imember, m);
                                                                                                                 }));
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _BusObject::CallMethodHandler(ajn::MessageReceiver::MethodHandler handler, const ajn::InterfaceDescription::Member* member, ajn::Message& message, void* context)
{
    // Convert the void context to the actual receiver
    AllJoyn::MessageReceiver ^ receiver = reinterpret_cast<AllJoyn::MessageReceiver ^>(context);
    if (nullptr != receiver) {
        // Receivers are only specified for method handlers
        receiver->_receiver->MethodHandler(member, message);
    } else if (NULL != handler) {
        // Signals, Get[Prop]/Set[Prop] have no way to specify the context/receiver
        (this->*handler)(member, message);
    }
}

__BusObject::__BusObject()
{
    Bus = nullptr;
    Name = nullptr;
    Path = nullptr;
    Receiver = nullptr;
}

__BusObject::~__BusObject()
{
    Bus = nullptr;
    Name = nullptr;
    Path = nullptr;
    Receiver = nullptr;
}

}
