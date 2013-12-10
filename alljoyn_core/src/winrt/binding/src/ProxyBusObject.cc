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

#include "ProxyBusObject.h"

#include <BusAttachment.h>
#include <InterfaceDescription.h>
#include <MessageReceiver.h>
#include <MsgArg.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

ProxyBusObject::ProxyBusObject(BusAttachment ^ bus, Platform::String ^ service, Platform::String ^ path, ajn::SessionId sessionId)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert service to qcc::String
        qcc::String strService = PlatformToMultibyteString(service);
        if (strService.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check path for invalid values
        if (nullptr == path) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Convert path to qcc::String
        qcc::String strPath = PlatformToMultibyteString(path);
        if (strPath.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create ProxyBusObject managed object
        const char* serviceName = strService.c_str();
        const char* pathName = strPath.c_str();
        _mProxyBusObject = new qcc::ManagedObj<_ProxyBusObject>(bus, serviceName, pathName, sessionId);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusObject for convenience
        _proxyBusObject = &(**_mProxyBusObject);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

ProxyBusObject::ProxyBusObject(BusAttachment ^ bus, const ajn::ProxyBusObject* proxyBusObject)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check proxyBusObject for invalid values
        if (NULL == proxyBusObject) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Create ProxyBusObject managed object
        _mProxyBusObject = new qcc::ManagedObj<_ProxyBusObject>(bus, proxyBusObject);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusObject for convenience
        _proxyBusObject = &(**_mProxyBusObject);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

ProxyBusObject::ProxyBusObject(BusAttachment ^ bus, const ajn::_ProxyBusObject* proxyBusObject)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check proxyBusObject for invalid values
        if (NULL == proxyBusObject) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Create ProxyBusObject managed object
        _mProxyBusObject = new qcc::ManagedObj<_ProxyBusObject>(bus, proxyBusObject);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusObject for convenience
        _proxyBusObject = &(**_mProxyBusObject);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

}

ProxyBusObject::ProxyBusObject(BusAttachment ^ bus, const qcc::ManagedObj<_ProxyBusObject>* proxyBusObject)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check bus for invalid values
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check proxyBusObject for invalid values
        if (NULL == proxyBusObject) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Attach proxyBusObject to _ProxyBusObject managed object
        _mProxyBusObject = new qcc::ManagedObj<_ProxyBusObject>(*proxyBusObject);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusObject for convenience
        _proxyBusObject = &(**_mProxyBusObject);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

ProxyBusObject::~ProxyBusObject()
{
    // Delete _ProxyBusObject managed object to adjust ref count
    if (NULL != _mProxyBusObject) {
        delete _mProxyBusObject;
        _mProxyBusObject = NULL;
        _proxyBusObject = NULL;
    }
}

Windows::Foundation::IAsyncOperation<IntrospectRemoteObjectResult ^> ^ ProxyBusObject::IntrospectRemoteObjectAsync(Platform::Object ^ context)
{
    ::QStatus status = ER_OK;
    Windows::Foundation::IAsyncOperation<IntrospectRemoteObjectResult ^> ^ result = nullptr;

    while (true) {
        // Get unmanaged Listener
        ajn::ProxyBusObject::Listener* listener = _proxyBusObject->_proxyBusObjectListener;
        // Get unmanaged IntrospectCB
        ajn::ProxyBusObject::Listener::IntrospectCB cb = _proxyBusObject->_proxyBusObjectListener->GetProxyListenerIntrospectCBHandler();
        // Create IntrospectRemoteObjectResult with context
        IntrospectRemoteObjectResult ^ introspectRemoteObjectResult = ref new IntrospectRemoteObjectResult(this, context);
        // Check for allocation error
        if (nullptr == introspectRemoteObjectResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->IntrospectRemoteObjectAsync(listener, cb, (void*)introspectRemoteObjectResult);
        if (ER_OK != status) {
            break;
        }
        // Create async operation
        result = concurrency::create_async([this, introspectRemoteObjectResult]()->IntrospectRemoteObjectResult ^
                                           {
                                               // Wait for introspectRemoteObjectResult to complete
                                               introspectRemoteObjectResult->Wait();
                                               // Result is ready
                                               return introspectRemoteObjectResult;
                                           });
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Windows::Foundation::IAsyncOperation<GetPropertyResult ^> ^ ProxyBusObject::GetPropertyAsync(
    Platform::String ^ iface,
    Platform::String ^ property,
    Platform::Object ^ context,
    uint32_t timeout)
{
    ::QStatus status = ER_OK;
    Windows::Foundation::IAsyncOperation<GetPropertyResult ^> ^ result = nullptr;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert iface to qcc::String
        qcc::String strIface = PlatformToMultibyteString(iface);
        // Check for conversion failure
        if (strIface.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check property for invalid values
        if (nullptr == property) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert property to qcc::String
        qcc::String strProperty = PlatformToMultibyteString(property);
        // Check for conversion failure
        if (strProperty.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get unmanaged Listener
        ajn::ProxyBusObject::Listener* listener = _proxyBusObject->_proxyBusObjectListener;
        // Get unmanaged GetPropertyCB
        ajn::ProxyBusObject::Listener::GetPropertyCB cb = _proxyBusObject->_proxyBusObjectListener->GetProxyListenerGetPropertyCBHandler();
        // Create GetPropertyResult with context
        GetPropertyResult ^ getPropertyResult = ref new GetPropertyResult(this, context);
        // Check for allocation errror
        if (nullptr == getPropertyResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetPropertyAsync(
            strIface.c_str(),
            strProperty.c_str(),
            listener,
            cb,
            (void*)getPropertyResult,
            timeout);
        if (ER_OK != status) {
            break;
        }
        // Create async operation
        result = concurrency::create_async([this, getPropertyResult]()->GetPropertyResult ^
                                           {
                                               // Wait for getPropertyResul to complete
                                               getPropertyResult->Wait();
                                               // Result is ready
                                               return getPropertyResult;
                                           });
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Windows::Foundation::IAsyncOperation<GetAllPropertiesResult ^> ^ ProxyBusObject::GetAllPropertiesAsync(
    Platform::String ^ iface,
    Platform::Object ^ context,
    uint32_t timeout)
{
    ::QStatus status = ER_OK;
    Windows::Foundation::IAsyncOperation<GetAllPropertiesResult ^> ^ result = nullptr;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert iface to qcc::String
        qcc::String strIface = PlatformToMultibyteString(iface);
        // Check for conversion failure
        if (strIface.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get unmanaged Listener
        ajn::ProxyBusObject::Listener* listener = _proxyBusObject->_proxyBusObjectListener;
        // Get unmanaged GetPropertyCB
        ajn::ProxyBusObject::Listener::GetPropertyCB cb = _proxyBusObject->_proxyBusObjectListener->GetProxyListenerGetAllPropertiesCBHandler();
        // Create GetAllPropertiesResult with context
        GetAllPropertiesResult ^ getAllPropertiesResult = ref new GetAllPropertiesResult(this, context);
        // Check for allocation error
        if (nullptr == getAllPropertiesResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetAllPropertiesAsync(strIface.c_str(),
                                                                                 listener,
                                                                                 cb,
                                                                                 (void*)getAllPropertiesResult,
                                                                                 timeout);
        if (ER_OK != status) {
            break;
        }
        // Create async operation
        result = concurrency::create_async([this, getAllPropertiesResult]()->GetAllPropertiesResult ^
                                           {
                                               // Wait for getAllPropertiesResult to complete
                                               getAllPropertiesResult->Wait();
                                               // Result is ready
                                               return getAllPropertiesResult;
                                           });
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Windows::Foundation::IAsyncOperation<SetPropertyResult ^> ^ ProxyBusObject::SetPropertyAsync(
    Platform::String ^ iface,
    Platform::String ^ property,
    MsgArg ^ value,
    Platform::Object ^ context,
    uint32_t timeout)
{
    ::QStatus status = ER_OK;
    Windows::Foundation::IAsyncOperation<SetPropertyResult ^> ^ result = nullptr;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert iface to qcc::String
        qcc::String strIface = PlatformToMultibyteString(iface);
        // Check for conversion failure
        if (strIface.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check property for invalid values
        if (nullptr == property) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert property to qcc::String
        qcc::String strProperty = PlatformToMultibyteString(property);
        // Check for conversion failure
        if (strProperty.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check value for invalid values
        if (nullptr == value) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Get unmanaged MsgArg
        ajn::MsgArg* arg = value->_msgArg;
        // Get unmanaged Listener
        ajn::ProxyBusObject::Listener* listener = _proxyBusObject->_proxyBusObjectListener;
        // Get unmanaged SetPropertyCB
        ajn::ProxyBusObject::Listener::SetPropertyCB cb = _proxyBusObject->_proxyBusObjectListener->GetProxyListenerSetPropertyCBHandler();
        // Create SetPropertyResult with context
        SetPropertyResult ^ setPropertyResult = ref new SetPropertyResult(this, context);
        // Check for allocation error
        if (nullptr == setPropertyResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->SetPropertyAsync(strIface.c_str(),
                                                                            strProperty.c_str(),
                                                                            *arg,
                                                                            listener,
                                                                            cb,
                                                                            (void*)setPropertyResult,
                                                                            timeout);
        if (ER_OK != status) {
            break;
        }
        // Create async operation
        result = concurrency::create_async([this, setPropertyResult]()->SetPropertyResult ^
                                           {
                                               // Wait for setPropertyResult to complete
                                               setPropertyResult->Wait();
                                               // Result is ready
                                               return setPropertyResult;
                                           });
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t ProxyBusObject::GetInterfaces(Platform::WriteOnlyArray<InterfaceDescription ^> ^ ifaces)
{
    ::QStatus status = ER_OK;
    ajn::InterfaceDescription** idescArray = NULL;
    size_t result = -1;

    while (true) {
        // Check if out array has been specified
        if (nullptr != ifaces && ifaces->Length > 0) {
            // Allocate unmanaged InterfaceDescription* array
            idescArray = new ajn::InterfaceDescription *[ifaces->Length];
            // Check for allocation error
            if (NULL == idescArray) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
        }
        // Call the real API
        result = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetInterfaces((const ajn::InterfaceDescription**)idescArray, ifaces->Length);
        if (result > 0 && NULL != idescArray) {
            for (int i = 0; i < result; i++) {
                // Create InterfaceDescription
                InterfaceDescription ^ id = ref new InterfaceDescription(idescArray[i]);
                // Check for allocation error
                if (nullptr == id) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store the result
                ifaces[i] = id;
            }
        }
        break;
    }

    // Delete the temporary InterfaceDescription* array
    if (NULL != idescArray) {
        delete [] idescArray;
        idescArray = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

InterfaceDescription ^ ProxyBusObject::GetInterface(Platform::String ^ iface)
{
    ::QStatus status = ER_OK;
    InterfaceDescription ^ result = nullptr;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert iface to qcc::String
        qcc::String strIface = PlatformToMultibyteString(iface);
        // Check for conversion failure
        if (strIface.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        const ajn::InterfaceDescription* ret = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetInterface(strIface.c_str());
        if (NULL != ret) {
            // Create InterfaceDescription
            result = ref new InterfaceDescription(ret);
            // Check for allocation error
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
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

bool ProxyBusObject::ImplementsInterface(Platform::String ^ iface)
{
    ::QStatus status = ER_OK;
    bool result = false;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert iface to qcc::String
        qcc::String strIface = PlatformToMultibyteString(iface);
        // Check for conversion error
        if (strIface.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        result = ((ajn::ProxyBusObject*)*_proxyBusObject)->ImplementsInterface(strIface.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void ProxyBusObject::AddInterface(InterfaceDescription ^ iface)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check iface for invalid values
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged InterfaceDescription
        ajn::InterfaceDescription* id = *(iface->_interfaceDescr);
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->AddInterface(*id);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void ProxyBusObject::AddInterfaceWithString(Platform::String ^ name)
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
        // Check for conversion failure
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->AddInterface(strName.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

uint32_t ProxyBusObject::GetChildren(Platform::WriteOnlyArray<ProxyBusObject ^> ^ children)
{
    ::QStatus status = ER_OK;
    ajn::_ProxyBusObject** pboArray = NULL;
    size_t result = -1;

    while (true) {
        // Check if children is specified
        if (nullptr != children && children->Length > 0) {
            // Allocate unmanaged _ProxyBusObject* array
            pboArray = new ajn::_ProxyBusObject *[children->Length];
            // Check for allocation failure
            if (NULL == pboArray) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
        }
        // Call the real API
        result = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetManagedChildren(pboArray, children->Length);
        if (result > 0 && NULL != pboArray) {
            for (int i = 0; i < result; i++) {
                // Create ProxyBusObject
                ProxyBusObject ^ pbo = ref new ProxyBusObject(Bus, pboArray[i]);
                // Check for allocation error
                if (nullptr == pbo) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                } else {
                    // Store the result
                    children[i] = pbo;
                    // Delete the managed object to adjust ref count
                    delete pboArray[i];
                    pboArray[i] = NULL;
                }
            }
        }
        break;
    }

    // Create up temporary unmanaged _ProxyBusObject* array
    if (NULL != pboArray) {
        delete [] pboArray;
        pboArray = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

ProxyBusObject ^ ProxyBusObject::GetChild(Platform::String ^ path)
{
    ::QStatus status = ER_OK;
    ProxyBusObject ^ result = nullptr;

    while (true) {
        // Check path for invalid values
        if (nullptr == path) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert path to qcc::String
        qcc::String strPath = PlatformToMultibyteString(path);
        // Check for failed conversion
        if (strPath.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        ajn::_ProxyBusObject* ret = (ajn::_ProxyBusObject*)((ajn::ProxyBusObject*)*_proxyBusObject)->GetManagedChild(strPath.c_str());
        if (NULL != ret) {
            // Create ProxyBusObject
            result = ref new ProxyBusObject(Bus, ret);
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            } else {
                // Adjust the ref count of the managed object returned
                delete ret;
                ret = NULL;
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

void ProxyBusObject::AddChild(ProxyBusObject ^ child)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check child for invalid values
        if (nullptr == child) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get unamanged ProxyBusObject
        ajn::ProxyBusObject* pbo = (ajn::ProxyBusObject*)*(child->_proxyBusObject);
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->AddChild(*pbo);
        if (ER_OK == status) {
            // Often the path will be rewritten which will allocate a new ProxyBusObject
            qcc::String childPath = pbo->GetPath();
            size_t idx = childPath.size() + 1;
            size_t end = childPath.find_first_of('/', idx);
            qcc::String item = childPath.substr(0, (qcc::String::npos == end) ? end : end - 1);
            // Convert item to Platform::String
            Platform::String ^ strItem = MultibyteToPlatformString(item.c_str());
            // Check for failed conversion
            if (nullptr == strItem) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Look up child by path
            ProxyBusObject ^ tempChild = this->GetChild(strItem);
            // Check for the unexpected
            if (nullptr == tempChild) {
                status = ER_FAIL;
                break;
            }
            // Get the unmanaged ProxyBusObject from child
            pbo = (ajn::ProxyBusObject*)*(tempChild->_proxyBusObject);
            // Add an object reference
            AddObjectReference2(&(_proxyBusObject->_mutex), (void*)pbo, tempChild, &(_proxyBusObject->_childObjectMap));
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void ProxyBusObject::RemoveChild(Platform::String ^ path)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Look up child by path
        ProxyBusObject ^ child = GetChild(path);
        // Check for the unexpected
        if (nullptr == child) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert path to qcc::String
        qcc::String strPath = PlatformToMultibyteString(path);
        // Check for failed conversion
        if (strPath.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get unmanaged ProxyBusObject
        ajn::ProxyBusObject* pbo = (ajn::ProxyBusObject*)*(child->_proxyBusObject);
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->RemoveChild(strPath.c_str());
        if (ER_OK == status) {
            // Remove object reference
            RemoveObjectReference2(&(_proxyBusObject->_mutex), (void*)pbo, &(_proxyBusObject->_childObjectMap));
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Windows::Foundation::IAsyncOperation<MethodCallResult ^> ^ ProxyBusObject::MethodCallAsync(InterfaceMember ^ method,
                                                                                           const Platform::Array<MsgArg ^> ^ args,
                                                                                           Platform::Object ^ context,
                                                                                           uint32_t timeout,
                                                                                           uint8_t flags)
{
    ::QStatus status = ER_OK;
    ajn::MsgArg* msgScratch = NULL;
    Windows::Foundation::IAsyncOperation<MethodCallResult ^> ^ result = nullptr;

    while (true) {
        // Check method for invalid values
        if (nullptr == method) {
            status = ER_BAD_ARG_1;
            break;
        }
        size_t argsCount = 0;
        // Convert MsgArg array to unmanaged array
        if (nullptr != args & args->Length > 0) {
            argsCount = args->Length;
            // Allocate the unmanaged MsgArg array
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
                // Get unmanaged MsgArg
                ajn::MsgArg* arg = args[i]->_msgArg;
                // Deep copy the msgarg
                msgScratch[i] = *arg;
            }
            if (ER_OK != status) {
                break;
            }
        }
        // Create MethodCallResult
        MethodCallResult ^ methodCallResult = ref new MethodCallResult(this, context);
        // Check for allocation error
        if (nullptr == methodCallResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get unmanaged Member
        ajn::InterfaceDescription::Member* imethod = *(method->_member);
        // Get unmanaged MessageReceiver
        ajn::MessageReceiver* mreceiver = Receiver->_receiver;
        // Get unmanaged ReplyHandler
        ajn::MessageReceiver::ReplyHandler handler = static_cast<ajn::MessageReceiver::ReplyHandler>(&_ProxyBusObject::ReplyHandler);
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->MethodCallAsync(*imethod,
                                                                           mreceiver,
                                                                           handler,
                                                                           msgScratch,
                                                                           argsCount,
                                                                           (void*)methodCallResult,
                                                                           timeout,
                                                                           flags);
        if (ER_OK != status) {
            break;
        }
        // Create async operation
        result = concurrency::create_async([this, methodCallResult]()->MethodCallResult ^
                                           {
                                               // Wait for methodCallResult to complete
                                               methodCallResult->Wait();
                                               // Result is ready
                                               return methodCallResult;
                                           });
        break;
    }

    // Delete temporary unmanaged MsgArg array
    if (NULL != msgScratch) {
        delete [] msgScratch;
        msgScratch = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Windows::Foundation::IAsyncOperation<MethodCallResult ^> ^ ProxyBusObject::MethodCallAsync(Platform::String ^ ifaceName,
                                                                                           Platform::String ^ methodName,
                                                                                           const Platform::Array<MsgArg ^> ^ args,
                                                                                           Platform::Object ^ context,
                                                                                           uint32_t timeout,
                                                                                           uint8_t flags)
{
    ::QStatus status = ER_OK;
    ajn::MsgArg* msgScratch = NULL;
    Windows::Foundation::IAsyncOperation<MethodCallResult ^> ^ result = nullptr;

    while (true) {
        // Check ifaceName for invalid values
        if (nullptr == ifaceName) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert ifaceName to qcc::String
        qcc::String strIfaceName = PlatformToMultibyteString(ifaceName);
        // Check for conversion failure
        if (strIfaceName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check methodName for invalid values
        if (nullptr == methodName) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert methodName to qcc::String
        qcc::String strMethodName = PlatformToMultibyteString(methodName);
        // Check for conversion failure
        if (strMethodName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        size_t argsCount = 0;
        // Convert MsgArg array to unmanaged array
        if (nullptr != args & args->Length > 0) {
            argsCount = args->Length;
            // Allocate the unmanaged MsgArg array
            msgScratch = new ajn::MsgArg[argsCount];
            // Check for allocation error
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
                // Get unmanaged MsgArg
                ajn::MsgArg* arg = args[i]->_msgArg;
                // Deep copy the msgarg
                msgScratch[i] = *arg;
            }
            if (ER_OK != status) {
                break;
            }
        }
        // Create MethodCallResult
        MethodCallResult ^ methodCallResult = ref new MethodCallResult(this, context);
        // Check for allocation error
        if (nullptr == methodCallResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get unmanaged MessageReceiver
        ajn::MessageReceiver* mreceiver = Receiver->_receiver;
        // Get unmanaged ReplyHandler
        ajn::MessageReceiver::ReplyHandler handler = static_cast<ajn::MessageReceiver::ReplyHandler>(&_ProxyBusObject::ReplyHandler);
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->MethodCallAsync(strIfaceName.c_str(),
                                                                           strMethodName.c_str(),
                                                                           mreceiver,
                                                                           handler,
                                                                           msgScratch,
                                                                           argsCount,
                                                                           (void*)methodCallResult,
                                                                           timeout,
                                                                           flags);
        if (ER_OK != status) {
            break;
        }
        // Create async operation
        result = concurrency::create_async([this, methodCallResult]()->MethodCallResult ^
                                           {
                                               // Wait for methodCallResult to complete
                                               methodCallResult->Wait();
                                               // Result is ready
                                               return methodCallResult;
                                           });
        break;
    }

    // Delete temporary unmanaged msgArg array
    if (NULL != msgScratch) {
        delete [] msgScratch;
        msgScratch = NULL;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void ProxyBusObject::ParseXml(Platform::String ^ xml, Platform::String ^ identifier)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check xml for invalid values
        if (nullptr == xml) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert xml to qcc::String
        qcc::String strXml = PlatformToMultibyteString(xml);
        // Check for conversion error
        if (strXml.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check identifier for invalid values
        if (nullptr == identifier) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert identifier to qcc::String
        qcc::String strIdentifier = PlatformToMultibyteString(identifier);
        // Check for conversion error
        if (strIdentifier.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = ((ajn::ProxyBusObject*)*_proxyBusObject)->ParseXml(strXml.c_str(), strIdentifier.c_str());
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void ProxyBusObject::SecureConnectionAsync(bool forceAuth)
{
    // Call the real API
    ::QStatus status = ((ajn::ProxyBusObject*)*_proxyBusObject)->SecureConnectionAsync(forceAuth);

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

bool ProxyBusObject::IsValid()
{
    // Call the real API
    return ((ajn::ProxyBusObject*)*_proxyBusObject)->IsValid();
}

BusAttachment ^ ProxyBusObject::Bus::get()
{
    // Call the real API
    return _proxyBusObject->_eventsAndProperties->Bus;
}

Platform::String ^ ProxyBusObject::Name::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _proxyBusObject->_eventsAndProperties->Name) {
            // Call the real API
            qcc::String strName = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetServiceName();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strName.c_str());
            // Check for conversion failure
            if (nullptr == result && !strName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _proxyBusObject->_eventsAndProperties->Name = result;
        } else {
            // Return Name
            result = _proxyBusObject->_eventsAndProperties->Name;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ ProxyBusObject::Path::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if wrapped value already exists
        if (nullptr == _proxyBusObject->_eventsAndProperties->Path) {
            // Call the real API
            qcc::String strPath = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetPath();
            // Convert return value to Platform::String
            result = MultibyteToPlatformString(strPath.c_str());
            // Check for conversion failure
            if (nullptr == result && !strPath.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _proxyBusObject->_eventsAndProperties->Path = result;
        } else {
            // Return Path
            result = _proxyBusObject->_eventsAndProperties->Path;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

MessageReceiver ^ ProxyBusObject::Receiver::get()
{
    // Return Receiver
    return _proxyBusObject->_eventsAndProperties->Receiver;
}

ajn::SessionId ProxyBusObject::SessionId::get()
{
    ::QStatus status = ER_OK;
    ajn::SessionId result = (ajn::SessionId)-1;

    while (true) {
        // Check if wrapped value already exists
        if ((ajn::SessionId)-1 == _proxyBusObject->_eventsAndProperties->SessionId) {
            // Call the real API
            result = ((ajn::ProxyBusObject*)*_proxyBusObject)->GetSessionId();
            // Store the result
            _proxyBusObject->_eventsAndProperties->SessionId = result;
        } else {
            // Return SessionId
            result = _proxyBusObject->_eventsAndProperties->SessionId;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_ProxyBusObject::_ProxyBusObject(BusAttachment ^ b, const ajn::ProxyBusObject* proxyBusObject)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create private ref class
        _eventsAndProperties = ref new __ProxyBusObject();
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create _ProxyBusObjectListener
        _proxyBusObjectListener = new _ProxyBusObjectListener(this);
        // Check for allocation error
        if (NULL == _proxyBusObjectListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create MessageReceiver
        AllJoyn::MessageReceiver ^ receiver =  ref new AllJoyn::MessageReceiver(b);
        // Check for allocation error
        if (nullptr == receiver) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Attach proxyBusObjec to managed _ProxyBusObject
        _mProxyBusObject = new ajn::_ProxyBusObject(*proxyBusObject);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusOBject for convenience
        _proxyBusObject = &(**_mProxyBusObject);
        // Store Receiver
        _eventsAndProperties->Receiver = receiver;
        // Store BusAttachment
        _eventsAndProperties->Bus = b;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_ProxyBusObject::_ProxyBusObject(BusAttachment ^ b, const ajn::_ProxyBusObject* proxyBusObject)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create private ref class
        _eventsAndProperties = ref new __ProxyBusObject();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create _ProxyBusObjectListener
        _proxyBusObjectListener = new _ProxyBusObjectListener(this);
        // Check for allocation error
        if (NULL == _proxyBusObjectListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create MessageReceiver
        AllJoyn::MessageReceiver ^ receiver =  ref new AllJoyn::MessageReceiver(b);
        // Check for allocation error
        if (nullptr == receiver) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Attach proxyBusObject to managed _ProxyBusObject
        _mProxyBusObject = new ajn::_ProxyBusObject(*proxyBusObject);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusObject for convenience
        _proxyBusObject = &(**_mProxyBusObject);
        // Store Receiver
        _eventsAndProperties->Receiver = receiver;
        // Store BusAttachment
        _eventsAndProperties->Bus = b;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_ProxyBusObject::_ProxyBusObject(BusAttachment ^ b, const char* service, const char* path, ajn::SessionId sessionId)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create private ref class
        _eventsAndProperties = ref new __ProxyBusObject();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create _ProxyBusObjectListener
        _proxyBusObjectListener = new _ProxyBusObjectListener(this);
        // Check for allocation error
        if (NULL == _proxyBusObjectListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create MessageReceiver
        AllJoyn::MessageReceiver ^ receiver =  ref new AllJoyn::MessageReceiver(b);
        // Check for allocation error
        if (nullptr == receiver) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create ProxyBusObject from parameters
        ajn::ProxyBusObject pbo(*(b->_busAttachment), service, path, sessionId);
        // Create _ProxyBusObject
        _mProxyBusObject = new ajn::_ProxyBusObject(pbo);
        // Check for allocation error
        if (NULL == _mProxyBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _ProxyBusObject for convienence
        _proxyBusObject = &(**_mProxyBusObject);
        // Store Receiver
        _eventsAndProperties->Receiver = receiver;
        // Store BusAttachment
        _eventsAndProperties->Bus = b;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_ProxyBusObject::~_ProxyBusObject()
{
    _eventsAndProperties = nullptr;
    _mReceiver = NULL;
    // Delete _ProxyBusObjectListener
    if (NULL != _proxyBusObjectListener) {
        delete _proxyBusObjectListener;
        _proxyBusObjectListener = NULL;
    }
    // Delete managed _ProxyBusObject
    if (NULL != _mProxyBusObject) {
        delete _mProxyBusObject;
        _mProxyBusObject = NULL;
    }
    _proxyBusObject = NULL;
    // Clear object map
    ClearObjectMap(&(this->_mutex), &(this->_childObjectMap));
}

void _ProxyBusObject::ReplyHandler(ajn::Message& msg, void* context)
{
    ::QStatus status = ER_OK;
    MethodCallResult ^ methodCallResult = reinterpret_cast<MethodCallResult ^>(context);

    try {
        while (true) {
            // Create Message
            Message ^ message = ref new Message(&msg);
            // Check for allocation error
            if (nullptr == message) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            methodCallResult->Message = message;
            break;
        }

        // Bubble up any QStatus error as an exception
        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }

        // Signal the result is complete
        methodCallResult->Complete();
    } catch (Platform::Exception ^ pe) {
        // Forward Platform::Exception
        methodCallResult->_exception = pe;
        methodCallResult->Complete();
    } catch (std::exception& e) {
        // Forward std::exception
        methodCallResult->_stdException = new std::exception(e);
        methodCallResult->Complete();
    }
}

_ProxyBusObject::operator ajn::ProxyBusObject * ()
{
    // Return unmanaged ProxyBusObject
    return _proxyBusObject;
}

_ProxyBusObject::operator ajn::_ProxyBusObject * ()
{
    // Return managed _ProxyBusObject
    return _mProxyBusObject;
}

_ProxyBusObjectListener::_ProxyBusObjectListener(_ProxyBusObject* proxybusobject)
    : _proxyBusObject(proxybusobject)
{
}

_ProxyBusObjectListener::~_ProxyBusObjectListener()
{
    _proxyBusObject = nullptr;
}

ajn::ProxyBusObject::Listener::IntrospectCB _ProxyBusObjectListener::GetProxyListenerIntrospectCBHandler()
{
    // Return unmanaged IntrospectCB
    return static_cast<ajn::ProxyBusObject::Listener::IntrospectCB>(&AllJoyn::_ProxyBusObjectListener::IntrospectCB);
}

ajn::ProxyBusObject::Listener::GetPropertyCB _ProxyBusObjectListener::GetProxyListenerGetPropertyCBHandler()
{
    // Return unmanaged GetPropertyCB
    return static_cast<ajn::ProxyBusObject::Listener::GetPropertyCB>(&AllJoyn::_ProxyBusObjectListener::GetPropertyCB);
}

ajn::ProxyBusObject::Listener::GetAllPropertiesCB _ProxyBusObjectListener::GetProxyListenerGetAllPropertiesCBHandler()
{
    // Return unmanaged GetAllPropertiesCB
    return static_cast<ajn::ProxyBusObject::Listener::GetAllPropertiesCB>(&AllJoyn::_ProxyBusObjectListener::GetAllPropertiesCB);
}

ajn::ProxyBusObject::Listener::SetPropertyCB _ProxyBusObjectListener::GetProxyListenerSetPropertyCBHandler()
{
    // Return unmanaged SetPropertyCB
    return static_cast<ajn::ProxyBusObject::Listener::SetPropertyCB>(&AllJoyn::_ProxyBusObjectListener::SetPropertyCB);
}

void _ProxyBusObjectListener::IntrospectCB(::QStatus s, ajn::ProxyBusObject* obj, void* context)
{
    ::QStatus status = ER_OK;
    IntrospectRemoteObjectResult ^ introspectRemoteObjectResult = reinterpret_cast<IntrospectRemoteObjectResult ^>(context);
    // Store the result
    introspectRemoteObjectResult->Status = (AllJoyn::QStatus)s;
    // Signal the result is complete
    introspectRemoteObjectResult->Complete();
}

void _ProxyBusObjectListener::GetPropertyCB(::QStatus s, ajn::ProxyBusObject* obj, const ajn::MsgArg& value, void* context)
{
    ::QStatus status = ER_OK;
    GetPropertyResult ^ getPropertyResult = reinterpret_cast<GetPropertyResult ^>(context);

    try {
        while (true) {
            // Create MsgArg
            MsgArg ^ arg = ref new MsgArg(&value);
            // Check for allocation error
            if (nullptr == arg) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the results
            getPropertyResult->Value = arg;
            getPropertyResult->Status = (AllJoyn::QStatus)s;
            break;
        }

        // Bubble up any QStatus error as an exception
        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }

        // Signal the result is complete
        getPropertyResult->Complete();
    } catch (Platform::Exception ^ pe) {
        // Forward Platform::Exception
        getPropertyResult->_exception = pe;
        getPropertyResult->Complete();
    } catch (std::exception& e) {
        // Forward std::exception
        getPropertyResult->_stdException = new std::exception(e);
        getPropertyResult->Complete();
    }
}

void _ProxyBusObjectListener::GetAllPropertiesCB(::QStatus s, ajn::ProxyBusObject* obj, const ajn::MsgArg& value, void* context)
{
    ::QStatus status = ER_OK;
    GetAllPropertiesResult ^ getAllPropertiesResult = reinterpret_cast<GetAllPropertiesResult ^>(context);

    try {
        while (true) {
            // Create MsgArg
            MsgArg ^ arg = ref new MsgArg(&value);
            // Check for allocation error
            if (nullptr == arg) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the results
            getAllPropertiesResult->Value = arg;
            getAllPropertiesResult->Status = (AllJoyn::QStatus)s;
            break;
        }

        // Bubble up any QStatus error as an exception
        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }

        // Signal the result is complete
        getAllPropertiesResult->Complete();
    } catch (Platform::Exception ^ pe) {
        // Forward Platform::Exception
        getAllPropertiesResult->_exception = pe;
        getAllPropertiesResult->Complete();
    } catch (std::exception& e) {
        // Forward std::exception
        getAllPropertiesResult->_stdException = new std::exception(e);
        getAllPropertiesResult->Complete();
    }
}

void _ProxyBusObjectListener::SetPropertyCB(::QStatus s, ajn::ProxyBusObject* obj, void* context)
{
    ::QStatus status = ER_OK;
    SetPropertyResult ^ setPropertyResult = reinterpret_cast<SetPropertyResult ^>(context);
    setPropertyResult->Status = (AllJoyn::QStatus)s;

    // Signal the result is complete
    setPropertyResult->Complete();
}

__ProxyBusObject::__ProxyBusObject()
{
    Bus = nullptr;
    Name = nullptr;
    Path = nullptr;
    Receiver = nullptr;
    SessionId = (ajn::SessionId)-1;
}

__ProxyBusObject::~__ProxyBusObject()
{
    Bus = nullptr;
    Name = nullptr;
    Path = nullptr;
    Receiver = nullptr;
    SessionId = (ajn::SessionId)-1;
}

}
