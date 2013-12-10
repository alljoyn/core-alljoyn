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

#include "BusAttachment.h"

#include <alljoyn/BusObject.h>
#include <alljoyn/BusListener.h>
#include <BusInternal.h>
#include <SocketStream.h>
#include <InterfaceDescription.h>
#include <MessageReceiver.h>
#include <BusObject.h>
#include <ProxyBusObject.h>
#include <BusListener.h>
#include <SessionListener.h>
#include <SessionPortListener.h>
#include <SessionOpts.h>
#include <AuthListener.h>
#include <KeyStoreListener.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <qcc/Event.h>
#include <qcc/winrt/SocketWrapper.h>
#include <ObjectReference.h>
#include <AllJoynException.h>
#include <ctxtcall.h>
#include <ppltasks.h>

using namespace Windows::Foundation;

namespace AllJoyn {

// Synchronization object to the _nativeToManagedMap map
qcc::Mutex _mutex;
// Provides a walk around registration of BusAttachment for certain callbacks
std::map<void*, void*> _nativeToManagedMap;

BusAttachment::BusAttachment(Platform::String ^ applicationName, bool allowRemoteMessages, uint32_t concurrency)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in applicationName
        if (nullptr == applicationName) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check for failed conversion of applicationName string
        qcc::String strApplicationName = PlatformToMultibyteString(applicationName);
        if (strApplicationName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create the managed version of BusAttachment
        const char* name = strApplicationName.c_str();
        _mBusAttachment = new qcc::ManagedObj<_BusAttachment>(name, allowRemoteMessages, concurrency);
        if (NULL == _mBusAttachment) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a reference to the busattachment for convenience
        _busAttachment = &(**_mBusAttachment);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusAttachment::BusAttachment(const ajn::BusAttachment* busAttachment)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in busAttachment
        if (NULL == busAttachment) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Try and lookup the managed value of busattachment associated with this pointer
        _mutex.Lock();
        void* ba = _nativeToManagedMap[(void*)busAttachment];
        _mutex.Unlock();
        // Fail out if no such mapping exists
        if (NULL == ba) {
            status = ER_FAIL;
            break;
        }
        // Re-type the void* from the map
        qcc::ManagedObj<_BusAttachment>* mba = reinterpret_cast<qcc::ManagedObj<_BusAttachment>*>(ba);
        // Attach to the managed version
        _mBusAttachment = new qcc::ManagedObj<_BusAttachment>(*mba);
        // Check for failed allocation
        if (NULL == _mBusAttachment) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a reference to the busattachment for convenience
        _busAttachment = &(**_mBusAttachment);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusAttachment::BusAttachment(const qcc::ManagedObj<_BusAttachment>* busAttachment)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid vvalue in busAttachment
        if (NULL == busAttachment) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach to the managed version
        _mBusAttachment = new qcc::ManagedObj<_BusAttachment>(*busAttachment);
        if (NULL == _mBusAttachment) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store a reference to the busattachment for convenience
        _busAttachment = &(**_mBusAttachment);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

BusAttachment::~BusAttachment()
{
    // Decrement the managed count to _BusAttachment
    if (NULL != _mBusAttachment) {
        delete _mBusAttachment;
        _mBusAttachment = NULL;
        _busAttachment = NULL;
    }
}

uint32_t BusAttachment::GetConcurrency()
{
    // Call the real API
    return _busAttachment->GetConcurrency();
}

void BusAttachment::EnableConcurrentCallbacks()
{
    // Call the real API
    _busAttachment->EnableConcurrentCallbacks();
}

void BusAttachment::CreateInterface(Platform::String ^ name, Platform::WriteOnlyArray<AllJoyn::InterfaceDescription ^> ^ iface, bool secure)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid value in iface
        if (nullptr == iface || iface->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }
        ajn::InterfaceDescription* intDescr;
        // Call the real API
        status = _busAttachment->CreateInterface(strName.c_str(), intDescr, secure);
        if (ER_OK == status) {
            // Since intDescr is an out parameter, convert from ajn value
            InterfaceDescription ^ id = ref new InterfaceDescription(intDescr);
            // Fill the out parameter
            iface[0] = id;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        // For failed API, where iface is available, specify the nullptr value
        if (nullptr !=  iface) {
            iface[0] = nullptr;
        }
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::CreateInterfacesFromXml(Platform::String ^ xml)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in xml
        if (nullptr == xml) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert xml to qcc::String
        qcc::String strXml = PlatformToMultibyteString(xml);
        // Check for failed conversion
        if (strXml.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->CreateInterfacesFromXml(strXml.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

uint32_t BusAttachment::GetInterfaces(Platform::WriteOnlyArray<InterfaceDescription ^> ^ ifaces)
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
        result = _busAttachment->GetInterfaces((const ajn::InterfaceDescription**)idescArray, ifaces->Length);
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

InterfaceDescription ^ BusAttachment::GetInterface(Platform::String ^ name)
{
    ::QStatus status = ER_OK;
    InterfaceDescription ^ idRef = nullptr;

    while (true) {
        // Check for invalid values in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        const ajn::InterfaceDescription* id = _busAttachment->GetInterface(strName.c_str());
        // Convert failure to exception
        if (NULL == id) {
            status = ER_FAIL;
            break;
        }
        // Convert the unmanaged version
        idRef = ref new InterfaceDescription(id);
        // Check for failed construction
        if (nullptr == idRef) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return idRef;
}

void BusAttachment::DeleteInterface(InterfaceDescription ^ iface)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in iface
        if (nullptr == iface) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::InterfaceDescription* id = *(iface->_interfaceDescr);
        // Call the real API
        status = _busAttachment->DeleteInterface(*id);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::Start()
{
    // Call the real API
    ::QStatus status = _busAttachment->Start();
    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Windows::Foundation::IAsyncAction ^ BusAttachment::StopAsync()
{
    // Create an async operation to handle Stop
    IAsyncAction ^ action = concurrency::create_async([this](concurrency::cancellation_token ct) {
                                                          ::QStatus status = ER_OK;

                                                          while (true) {
                                                              // Call the real API to stop
                                                              status = _busAttachment->Stop();
                                                              if (ER_OK != status) {
                                                                  break;
                                                              }
                                                              // Call the real API to join
                                                              status =  _busAttachment->Join();
                                                              break;
                                                          }

                                                          // Bubble up any QStatus error as exception
                                                          if (ER_OK != status) {
                                                              QCC_THROW_EXCEPTION(status);
                                                          }
                                                      });
    return action;
}

bool BusAttachment::IsStarted()
{
    // Call the real API
    return _busAttachment->IsStarted();
}

bool BusAttachment::IsStopping()
{
    // Call the real API
    return _busAttachment->IsStopping();
}

IAsyncAction ^ BusAttachment::ConnectAsync(Platform::String ^ connectSpec)
{
    // Create an async operation for Connect
    IAsyncAction ^ action = concurrency::create_async([this, connectSpec](concurrency::cancellation_token ct) {
                                                          ::QStatus status = ER_OK;
                                                          while (true) {
                                                              // Check for invalid values specified in connectSpec
                                                              if (nullptr == connectSpec) {
                                                                  status = ER_BAD_ARG_1;
                                                                  break;
                                                              }
                                                              // Convert connectSpec to qcc::String
                                                              qcc::String strConnectSpec = PlatformToMultibyteString(connectSpec);
                                                              // Check for failed conversion
                                                              if (strConnectSpec.empty()) {
                                                                  status = ER_OUT_OF_MEMORY;
                                                                  break;
                                                              }
                                                              // Call the real API
                                                              status = _busAttachment->Connect(strConnectSpec.c_str());
                                                              break;
                                                          }

                                                          // Bubble up any QStatus error as exception
                                                          if (ER_OK != status) {
                                                              QCC_THROW_EXCEPTION(status);
                                                          }
                                                      });
    return action;
}

IAsyncAction ^ BusAttachment::DisconnectAsync(Platform::String ^ connectSpec)
{
    // Create an async operation for Disconnect
    IAsyncAction ^ action = concurrency::create_async([this, connectSpec](concurrency::cancellation_token ct) {
                                                          ::QStatus status = ER_OK;
                                                          while (true) {
                                                              // Check for invalid values specified in connectSpec
                                                              if (nullptr == connectSpec) {
                                                                  status = ER_BAD_ARG_1;
                                                                  break;
                                                              }
                                                              // Convert connectSpec to qcc::String
                                                              qcc::String strConnectSpec = PlatformToMultibyteString(connectSpec);
                                                              // Check for failed conversion
                                                              if (strConnectSpec.empty()) {
                                                                  status = ER_OUT_OF_MEMORY;
                                                                  break;
                                                              }
                                                              // Call the real API
                                                              status = _busAttachment->Disconnect(strConnectSpec.c_str());
                                                              break;
                                                          }

                                                          // Bubble up any QStatus error as exception
                                                          if (ER_OK != status) {
                                                              QCC_THROW_EXCEPTION(status);
                                                          }
                                                      });
    return action;
}

bool BusAttachment::IsConnected()
{
    // Call the real API
    return _busAttachment->IsConnected();
}

void BusAttachment::RegisterBusObject(BusObject ^ obj)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in obj
        if (nullptr == obj) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::BusObject* bo = obj->_busObject;
        // Call the real API
        status = _busAttachment->RegisterBusObject(*bo);
        if (ER_OK == status) {
            // Add an object reference
            AddObjectReference(&(_busAttachment->_mutex), obj, &(_busAttachment->_busObjectMap));
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::UnregisterBusObject(BusObject ^ object)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in object
        if (nullptr == object) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged type
        ajn::BusObject* bo = object->_busObject;
        // Call the real API
        _busAttachment->UnregisterBusObject(*bo);
        // Remove object reference
        RemoveObjectReference(&(_busAttachment->_mutex), object, &(_busAttachment->_busObjectMap));
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::RegisterSignalHandler(MessageReceiver ^ receiver,
                                          InterfaceMember ^ member,
                                          Platform::String ^ srcPath)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in receiver
        if (nullptr == receiver) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check for invalid values in member
        if (nullptr == member) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Get the unmanaged version of _MessageReceiver
        _MessageReceiver* mreceiver = receiver->_receiver;
        // Get the unmanaged version of SignalHandler
        ajn::MessageReceiver::SignalHandler handler = mreceiver->GetSignalHandler();
        // Store the unmanaged version of MessageReceiver
        ajn::MessageReceiver* ajnreceiver = mreceiver;
        // Get the unmanaged version of Member
        ajn::InterfaceDescription::Member* imember = *(member->_member);
        // Convert srcpath to qcc::String
        qcc::String strSrcPath = PlatformToMultibyteString(srcPath);
        // Check for conversion error
        if (nullptr != srcPath && strSrcPath.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->RegisterSignalHandler(ajnreceiver, handler, imember, strSrcPath.c_str());
        if (ER_OK == status) {
            // Add an object reference
            AddObjectReference(&(_busAttachment->_mutex), receiver, &(_busAttachment->_signalHandlerMap));
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::UnregisterSignalHandler(MessageReceiver ^ receiver,
                                            InterfaceMember ^ member,
                                            Platform::String ^ srcPath)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in receiver
        if (nullptr == receiver) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check for invalid values in member
        if (nullptr == member) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Get the unmanaged version of _MessageReceiver;
        _MessageReceiver* mreceiver = receiver->_receiver;
        // Get the unmanaged version of SignalHandler
        ajn::MessageReceiver::SignalHandler handler = mreceiver->GetSignalHandler();
        // Store the unmanaged version of MessageReceiver
        ajn::MessageReceiver* ajnreceiver = mreceiver;
        // Get the unmanaged version of Member
        ajn::InterfaceDescription::Member* imember = *(member->_member);
        // Convert srcPath to qcc::String
        qcc::String strSrcPath = PlatformToMultibyteString(srcPath);
        // Check for conversion error
        if (nullptr != srcPath && strSrcPath.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->UnregisterSignalHandler(ajnreceiver, handler, imember, strSrcPath.c_str());
        if (ER_OK == status) {
            // Remove object reference
            RemoveObjectReference(&(_busAttachment->_mutex), receiver, &(_busAttachment->_signalHandlerMap));
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::EnablePeerSecurity(Platform::String ^ authMechanisms,
                                       AuthListener ^ listener,
                                       Platform::String ^ keyStoreFileName,
                                       bool isShared)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in authMechanisms
        if (nullptr == authMechanisms) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert authMechanisms to qcc::String
        qcc::String strAuthMechanisms = PlatformToMultibyteString(authMechanisms);
        // Check for failed conversion
        if (strAuthMechanisms.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in listener
        if (nullptr == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::AuthListener* al = listener->_listener;
        // Check for invalid values in keyStoreFileName
        if (nullptr == keyStoreFileName) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Convert keyStoreFileName to qcc::String
        qcc::String strKeyStoreFileName = PlatformToMultibyteString(keyStoreFileName);
        // Check for conversion error
        if (strKeyStoreFileName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->EnablePeerSecurity(strAuthMechanisms.c_str(), al, strKeyStoreFileName.c_str(), isShared);
        if (ER_OK == status) {
            // Store reference to the auth listener
            _busAttachment->_authListener = listener;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::DisablePeerSecurity(AuthListener ^ listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in listener
        if (nullptr == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged value
        ajn::AuthListener* al = listener->_listener;
        // Call the real API
        status = _busAttachment->EnablePeerSecurity(NULL, al, NULL, false);
        if (ER_OK == status) {
            // Clear reference to the auth listener
            _busAttachment->_authListener = nullptr;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

bool BusAttachment::IsPeerSecurityEnabled()
{
    // Call the real API
    return _busAttachment->IsPeerSecurityEnabled();
}

void BusAttachment::RegisterBusListener(BusListener ^ listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in listener
        if (nullptr == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::BusListener* bl = listener->_listener;
        // Walk around the native ptr in the callback
        _mutex.Lock();
        _nativeToManagedMap[_busAttachment] = _mBusAttachment;
        // Call the real API
        _busAttachment->RegisterBusListener(*bl);
        // Remove the object in map
        _nativeToManagedMap.erase(_busAttachment);
        _mutex.Unlock();
        // Add an object reference
        AddObjectReference(&(_busAttachment->_mutex), listener, &(_busAttachment->_busListenerMap));
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::UnregisterBusListener(BusListener ^ listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in listener
        if (nullptr == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::BusListener* bl = listener->_listener;
        // Call the real API
        _busAttachment->UnregisterBusListener(*bl);
        // Remove object reference
        RemoveObjectReference(&(_busAttachment->_mutex), listener, &(_busAttachment->_busListenerMap));
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::RegisterKeyStoreListener(KeyStoreListener ^ listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in listener
        if (nullptr == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Get the unmanaged version
        ajn::KeyStoreListener* ksl = listener->_listener;
        // Call the real API
        status = _busAttachment->RegisterKeyStoreListener(*ksl);
        if (ER_OK == status) {
            // Store reference to the key store listener
            _busAttachment->_keyStoreListener = listener;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::UnregisterKeyStoreListener()
{
    // Call the default API
    ::QStatus status = _busAttachment->UnregisterKeyStoreListener();

    if (ER_OK == status) {
        // Remove reference to any key store listener
        _busAttachment->_keyStoreListener = nullptr;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::ReloadKeyStore()
{
    ::QStatus status = ER_OK;

    while (true) {
        // Call the real API
        status = _busAttachment->ReloadKeyStore();
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::ClearKeyStore()
{
    // Call the real API
    _busAttachment->ClearKeyStore();
}

void BusAttachment::ClearKeys(Platform::String ^ guid)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid vlues in guid
        if (nullptr == guid) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert guid to qcc::String
        qcc::String strGuid = PlatformToMultibyteString(guid);
        // Check for failed conversion
        if (strGuid.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->ClearKeys(strGuid.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::SetKeyExpiration(Platform::String ^ guid, uint32_t timeout)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in guid
        if (nullptr == guid) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert guid to qcc::String
        qcc::String strGuid = PlatformToMultibyteString(guid);
        // Check for failed conversion
        if (strGuid.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->SetKeyExpiration(strGuid.c_str(), timeout);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::GetKeyExpiration(Platform::String ^ guid, Platform::WriteOnlyArray<uint32> ^ timeout)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in guid
        if (nullptr == guid) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert guid to qcc::String
        qcc::String strGuid = PlatformToMultibyteString(guid);
        // Check for failed conversion
        if (strGuid.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid value specified in timeout
        if (timeout == nullptr || timeout->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }
        uint32_t expiration;
        // Call the real API
        status = _busAttachment->GetKeyExpiration(strGuid.c_str(), expiration);
        if (ER_OK == status) {
            // Store the out value
            timeout[0] = expiration;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::AddLogonEntry(Platform::String ^ authMechanism, Platform::String ^ userName, Platform::String ^ password)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in authMechanism
        if (nullptr == authMechanism) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert authMechanism to qcc::String
        qcc::String strAuthMechanism = PlatformToMultibyteString(authMechanism);
        // Check for failed conversion
        if (strAuthMechanism.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in userName
        if (nullptr == userName) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Convert userName to qcc::String
        qcc::String strUserName = PlatformToMultibyteString(userName);
        // Check for failed conversion
        if (strUserName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in password
        if (nullptr == password) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Convert password to qcc::String
        qcc::String strPassword = PlatformToMultibyteString(password);
        // Check for failed conversion
        if (strPassword.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->AddLogonEntry(strAuthMechanism.c_str(), strUserName.c_str(), strPassword.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::RequestName(Platform::String ^ requestedName, uint32_t flags)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check requestedName for invalid values
        if (nullptr == requestedName) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert requestName to qcc::String
        qcc::String strRequestedName = PlatformToMultibyteString(requestedName);
        // Check for failed conversion
        if (strRequestedName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->RequestName(strRequestedName.c_str(), flags);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::ReleaseName(Platform::String ^ name)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid valus in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->ReleaseName(strName.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::AddMatch(Platform::String ^ rule)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in rule
        if (nullptr == rule) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert rule to qcc::String
        qcc::String strRule = PlatformToMultibyteString(rule);
        // Check for failed conversion
        if (strRule.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->AddMatch(strRule.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::RemoveMatch(Platform::String ^ rule)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in rule
        if (nullptr == rule) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert rule to qcc::String
        qcc::String strRule = PlatformToMultibyteString(rule);
        // Check for failed conversion
        if (strRule.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->RemoveMatch(strRule.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::AdvertiseName(Platform::String ^ name, TransportMaskType transports)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->AdvertiseName(strName.c_str(), (int)transports);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::CancelAdvertiseName(Platform::String ^ name, TransportMaskType transports)
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
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->CancelAdvertiseName(strName.c_str(), (int)transports);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::FindAdvertisedName(Platform::String ^ namePrefix)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert namePrefix to qcc::String
        qcc::String strNamePrefix = PlatformToMultibyteString(namePrefix);
        // Check for failed conversion
        if (nullptr != namePrefix && strNamePrefix.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->FindAdvertisedName(strNamePrefix.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::FindAdvertisedNameByTransport(Platform::String ^ namePrefix, TransportMaskType transports)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert namePrefix to qcc::String
        qcc::String strNamePrefix = PlatformToMultibyteString(namePrefix);
        // Check for failed conversion
        if (nullptr != namePrefix && strNamePrefix.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->FindAdvertisedNameByTransport(strNamePrefix.c_str(), (int)transports);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::CancelFindAdvertisedName(Platform::String ^ namePrefix)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert namePrefix to qcc::String
        qcc::String strNamePrefix = PlatformToMultibyteString(namePrefix);
        // Check for invalid conversion
        if (nullptr != namePrefix && strNamePrefix.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->CancelFindAdvertisedName(strNamePrefix.c_str());
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::CancelFindAdvertisedNameByTransport(Platform::String ^ namePrefix, TransportMaskType transports)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert namePrefix to qcc::String
        qcc::String strNamePrefix = PlatformToMultibyteString(namePrefix);
        // Check for invalid conversion
        if (nullptr != namePrefix && strNamePrefix.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->CancelFindAdvertisedNameByTransport(strNamePrefix.c_str(), (int)transports);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::BindSessionPort(ajn::SessionPort sessionPort_in, Platform::WriteOnlyArray<ajn::SessionPort> ^ sessionPort_out, SessionOpts ^ opts, SessionPortListener ^ listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in sessionPort_out
        if (nullptr == sessionPort_out || sessionPort_out->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Check for invalid values in opts
        if (nullptr == opts) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Get the unmanaged SessionOpts
        ajn::SessionOpts* options = opts->_sessionOpts;
        if (nullptr == listener) {
            status = ER_BAD_ARG_4;
            break;
        }
        // Get the unmanaged SessionPortListener
        ajn::SessionPortListener* spl = listener->_listener;
        ajn::SessionPort port = sessionPort_in;
        // Call the real API
        status = _busAttachment->BindSessionPort(port, *options, *spl);
        if (ER_OK == status) {
            // Add a port reference
            AddPortReference(&(_busAttachment->_mutex), port, listener, &(_busAttachment->_sessionPortListenerMap));
            // Set the out parameter
            sessionPort_out[0] = port;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::UnbindSessionPort(ajn::SessionPort sessionPort)
{
    // Call the real API
    ::QStatus status = _busAttachment->UnbindSessionPort(sessionPort);

    if (ER_OK == status) {
        // Remove port reference
        RemovePortReference(&(_busAttachment->_mutex), sessionPort, &(_busAttachment->_sessionPortListenerMap));
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Windows::Foundation::IAsyncOperation<JoinSessionResult ^> ^ BusAttachment::JoinSessionAsync(Platform::String ^ sessionHost,
                                                                                            ajn::SessionPort sessionPort,
                                                                                            SessionListener ^ listener,
                                                                                            SessionOpts ^ opts_in,
                                                                                            Platform::WriteOnlyArray<SessionOpts ^> ^ opts_out,
                                                                                            Platform::Object ^ context)
{
    ::QStatus status = ER_OK;
    Windows::Foundation::IAsyncOperation<JoinSessionResult ^> ^ result = nullptr;

    while (true) {
        // Check for invalid values in sessionHost
        if (nullptr == sessionHost) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert sessionHost to qcc::String
        qcc::String strSessionHost = PlatformToMultibyteString(sessionHost);
        // Check for failed conversion
        if (strSessionHost.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        qcc::ManagedObj<_SessionListener>* msl = NULL;
        ajn::SessionListener* ajnlistener = NULL;
        if (nullptr != listener) {
            // Store the unmanaged version
            ajnlistener = listener->_listener;
        }
        // Check for invalid values specified in opts_in
        if (nullptr == opts_in) {
            status = ER_BAD_ARG_5;
            break;
        }
        // Store te unmanaged SessionOpts
        ajn::SessionOpts* opts = opts_in->_sessionOpts;
        // Check for invalid values in opts_out
        if (nullptr == opts_out || opts_out->Length != 1) {
            status = ER_BAD_ARG_6;
            break;
        }
        // Create the join session result
        JoinSessionResult ^ joinSessionResult = ref new JoinSessionResult(this, listener, context);
        // Check for allocation failure
        if (nullptr == joinSessionResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->JoinSessionAsync(strSessionHost.c_str(),
                                                  sessionPort,
                                                  ajnlistener,
                                                  *opts,
                                                  _busAttachment,
                                                  (void*)joinSessionResult);
        if (ER_OK == status) {
            // Convert the out value of SessionOpts
            SessionOpts ^ newOpts = ref new SessionOpts(opts);
            if (nullptr == newOpts) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the out parameter
            opts_out[0] = newOpts;
        } else {
            break;
        }
        // Create an async operation for the join
        result = concurrency::create_async([this, joinSessionResult]()->JoinSessionResult ^
                                           {
                                               // Wait for the operation
                                               joinSessionResult->Wait();
                                               // Result is now complete
                                               return joinSessionResult;
                                           });
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void BusAttachment::SetSessionListener(ajn::SessionId sessionId, SessionListener ^ listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        ajn::SessionListener* sl = NULL;
        if (nullptr != listener) {
            // Store the unmanaged version
            sl = listener->_listener;
        }
        // Call the real API
        status = _busAttachment->SetSessionListener(sessionId, sl);
        if (ER_OK == status) {
            if (sl != NULL) {
                // Add an id reference
                AddIdReference(&(_busAttachment->_mutex), sessionId, listener, &(_busAttachment->_sessionListenerMap));
            } else {
                // Remove an id reference
                RemoveIdReference(&(_busAttachment->_mutex), sessionId, &(_busAttachment->_sessionListenerMap));
            }
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::LeaveSession(ajn::SessionId sessionId)
{
    // Call the real API
    ::QStatus status = _busAttachment->LeaveSession(sessionId);

    if (ER_OK == status) {
        // Remove an id reference
        RemoveIdReference(&(_busAttachment->_mutex), sessionId, &(_busAttachment->_sessionListenerMap));
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::GetSessionSocketStream(ajn::SessionId sessionId, Platform::WriteOnlyArray<AllJoyn::SocketStream ^> ^ socketStream)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in socketStream
        if (nullptr == socketStream || socketStream->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }
        qcc::SocketFd sock;
        // Call the real API
        status = _busAttachment->GetSessionFd(sessionId, sock);
        if (ER_OK == status) {
            // Re-cast the sockfd to the underlying type
            qcc::winrt::SocketWrapper ^ s = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sock);
            // Create SocketStream
            AllJoyn::SocketStream ^ ss = ref new AllJoyn::SocketStream(s);
            // Check for memory allocation failure
            if (nullptr == ss) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Set the out parameter
            socketStream[0] = ss;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Windows::Foundation::IAsyncOperation<SetLinkTimeoutResult ^> ^ BusAttachment::SetLinkTimeoutAsync(ajn::SessionId sessionid,
                                                                                                  uint32 linkTimeout,
                                                                                                  Platform::Object ^ context)
{
    ::QStatus status = ER_OK;
    Windows::Foundation::IAsyncOperation<SetLinkTimeoutResult ^> ^ result = nullptr;

    while (true) {
        // Create set link timeout result
        SetLinkTimeoutResult ^ setLinkTimeoutResult = ref new SetLinkTimeoutResult(this, context);
        // Check for allocation failure
        if (nullptr == setLinkTimeoutResult) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        status = _busAttachment->SetLinkTimeoutAsync(sessionid,
                                                     linkTimeout,
                                                     _busAttachment,
                                                     (void*)setLinkTimeoutResult);
        if (ER_OK != status) {
            break;
        }
        // Create an async operation
        result = concurrency::create_async([this, setLinkTimeoutResult]()->SetLinkTimeoutResult ^
                                           {
                                               // Wait for the operation to complete
                                               setLinkTimeoutResult->Wait();
                                               // result is now ready
                                               return setLinkTimeoutResult;
                                           });
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

void BusAttachment::NameHasOwner(Platform::String ^ name, Platform::WriteOnlyArray<bool> ^ hasOwner)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in hasOwner
        if (hasOwner == nullptr || hasOwner->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }
        bool owned;
        // Call the real API
        status = _busAttachment->NameHasOwner(strName.c_str(), owned);
        if (ER_OK == status) {
            // Set the out parameter
            hasOwner[0] = owned;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::GetPeerGUID(Platform::String ^ name, Platform::WriteOnlyArray<Platform::String ^> ^ guid)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in name
        if (nullptr == name) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert name to qcc::String
        qcc::String strName = PlatformToMultibyteString(name);
        // Check for failed conversion
        if (strName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check for invalid values in guid
        if (nullptr == guid || guid->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }
        qcc::String peerGuid;
        // Call the real API
        status = _busAttachment->GetPeerGUID(strName.c_str(), peerGuid);
        if (ER_OK == status) {
            // Convert c style string to Platform::String
            Platform::String ^ tempGuid = MultibyteToPlatformString(peerGuid.c_str());
            // Check for failed conversion
            if (nullptr == tempGuid) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store out parameter
            guid[0] = tempGuid;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

bool BusAttachment::IsSameBusAttachment(BusAttachment ^ other)
{
    // Check for invalid values in other
    if (nullptr == other) {
        return false;
    }
    // Is same value?
    return &(**_mBusAttachment) == &(**other->_mBusAttachment);
}

void BusAttachment::OnAppSuspend()
{
    ::QStatus status = _busAttachment->OnAppSuspend();
    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void BusAttachment::OnAppResume()
{
    ::QStatus status = _busAttachment->OnAppResume();
    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

ProxyBusObject ^ BusAttachment::DBusProxyBusObject::get()
{
    ::QStatus status = ER_OK;
    ProxyBusObject ^ result = nullptr;

    while (true) {
        // Check if wrapped version already exists
        if (nullptr == _busAttachment->_eventsAndProperties->DBusProxyBusObject) {
            // Call the real API
            const ajn::ProxyBusObject* proxyObj = &_busAttachment->GetDBusProxyObj();
            if (NULL == proxyObj) {
                // Do nothing. just return
                break;
            }
            // Create the wrapped version
            result = ref new ProxyBusObject(this, proxyObj);
            // Check for allocation errors
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busAttachment->_eventsAndProperties->DBusProxyBusObject = result;
        } else {
            // Return the DBusProxyBusObject
            result = _busAttachment->_eventsAndProperties->DBusProxyBusObject;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

ProxyBusObject ^ BusAttachment::AllJoynProxyBusObject::get()
{
    ::QStatus status = ER_OK;
    ProxyBusObject ^ result = nullptr;

    while (true) {
        // Check if the wrapped version already exists
        if (nullptr == _busAttachment->_eventsAndProperties->AllJoynProxyBusObject) {
            // Call the real API
            const ajn::ProxyBusObject* proxyObj = &_busAttachment->GetAllJoynProxyObj();
            if (NULL == proxyObj) {
                // Do nothing. just return
                break;
            }
            // Create the wrapped version
            result = ref new ProxyBusObject(this, proxyObj);
            // Check for failed allocation
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busAttachment->_eventsAndProperties->AllJoynProxyBusObject = result;
        } else {
            // Return the AllJoynProxyBusObject
            result = _busAttachment->_eventsAndProperties->AllJoynProxyBusObject;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

ProxyBusObject ^ BusAttachment::AllJoynDebugProxyBusObject::get()
{
    ::QStatus status = ER_OK;
    ProxyBusObject ^ result = nullptr;

    while (true) {
        // Check if the wrapped version already exists
        if (nullptr == _busAttachment->_eventsAndProperties->AllJoynDebugProxyBusObject) {
            // Call the real API
            const ajn::ProxyBusObject* proxyObj = &_busAttachment->GetAllJoynDebugObj();
            if (NULL == proxyObj) {
                // Do nothing. just return
                break;
            }
            // Create the wrapped version
            result = ref new ProxyBusObject(this, proxyObj);
            // Check for failed allocation
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busAttachment->_eventsAndProperties->AllJoynDebugProxyBusObject = result;
        } else {
            // Return the AllJoynDebugProxyBusObject
            result = _busAttachment->_eventsAndProperties->AllJoynDebugProxyBusObject;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ BusAttachment::UniqueName::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped version already exists
        if (nullptr == _busAttachment->_eventsAndProperties->UniqueName) {
            // Call the real API
            qcc::String uniqueName = _busAttachment->GetUniqueName();
            // Convert the qcc::String to Platform::String
            result = MultibyteToPlatformString(uniqueName.c_str());
            // Check for failed conversion
            if (nullptr == result && !uniqueName.empty()) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busAttachment->_eventsAndProperties->UniqueName = result;
        } else {
            // Return the UniqueName
            result = _busAttachment->_eventsAndProperties->UniqueName;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

Platform::String ^ BusAttachment::GlobalGUIDString::get()
{
    ::QStatus status = ER_OK;
    Platform::String ^ result = nullptr;

    while (true) {
        // Check if the wrapped version already exists
        if (nullptr == _busAttachment->_eventsAndProperties->GlobalGUIDString) {
            // Call the real API and convert result to Platform::String
            result = MultibyteToPlatformString(_busAttachment->GetGlobalGUIDString().c_str());
            // Check for failed conversion
            if (nullptr == result) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the result
            _busAttachment->_eventsAndProperties->GlobalGUIDString = result;
        } else {
            // Return the GlobalGUIDString
            result = _busAttachment->_eventsAndProperties->GlobalGUIDString;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

uint32_t BusAttachment::Timestamp::get()
{
    ::QStatus status = ER_OK;
    uint32_t result = (uint32_t)-1;

    while (true) {
        // Check if the wrapped value already exists
        if ((uint32_t)-1 == _busAttachment->_eventsAndProperties->Timestamp) {
            // Call the real API
            result = _busAttachment->GetTimestamp();
            // Store the result
            _busAttachment->_eventsAndProperties->Timestamp = result;
        } else {
            // Return the Timestamp
            result = _busAttachment->_eventsAndProperties->Timestamp;
        }
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return result;
}

_BusAttachment::_BusAttachment(const char* applicationName, bool allowRemoteMessages, uint32_t concurrency)
    : BusAttachment(applicationName, allowRemoteMessages, concurrency), ajn::BusAttachment::JoinSessionAsyncCB(),
    _keyStoreListener(nullptr), _authListener(nullptr), _dispatcher(nullptr), _originSTA(false)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the internal ref class
        _eventsAndProperties = ref new __BusAttachment();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get the current CoreWindow dispatcher
        Windows::UI::Core::CoreWindow ^ window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
        if (nullptr != window) {
            _dispatcher = window->Dispatcher;
        }
        // Store the current apartment state
        _originSTA = IsOriginSTA();
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_BusAttachment::~_BusAttachment()
{
    _eventsAndProperties = nullptr;
    _keyStoreListener = nullptr;
    _authListener = nullptr;
    // Clear out the bus object map
    ClearObjectMap(&(this->_mutex), &(this->_busObjectMap));
    // Clear out the signal handler map
    ClearObjectMap(&(this->_mutex), &(this->_signalHandlerMap));
    // Clear out the bus listener map
    ClearObjectMap(&(this->_mutex), &(this->_busListenerMap));
    // Clear out the session port listener map
    ClearPortMap(&(this->_mutex), &(this->_sessionPortListenerMap));
    // Clear out the session listener map
    ClearIdMap(&(this->_mutex), &(this->_sessionListenerMap));
}

void _BusAttachment::JoinSessionCB(::QStatus s, ajn::SessionId sessionId, const ajn::SessionOpts& opts, void* context)
{
    ::QStatus status = ER_OK;
    JoinSessionResult ^ joinSessionResult = reinterpret_cast<JoinSessionResult ^>(context);

    try {
        while (true) {
            // Create wrapped sessionopts
            SessionOpts ^ options = ref new SessionOpts(&opts);
            // Check for allocation failure
            if (nullptr == options) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            // Add an id reference for the session
            AddIdReference(&(this->_mutex), sessionId, joinSessionResult->Listener, &(this->_sessionListenerMap));
            // Store the current status
            joinSessionResult->Status = (AllJoyn::QStatus)s;
            // Store the sessionId
            joinSessionResult->SessionId = sessionId;
            // Store the options
            joinSessionResult->Opts = options;
            break;
        }

        // Bubble up any QStatus error as exception
        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }

        // Signal result is complete
        joinSessionResult->Complete();
    } catch (Platform::Exception ^ pe) {
        // Forward Platform::Exception
        joinSessionResult->_exception = pe;
        joinSessionResult->Complete();
    } catch (std::exception& e) {
        // Forward std::exception
        joinSessionResult->_stdException = new std::exception(e);
        joinSessionResult->Complete();
    }
}

void _BusAttachment::SetLinkTimeoutCB(::QStatus s, uint32_t timeout, void* context)
{
    ::QStatus status = ER_OK;
    SetLinkTimeoutResult ^ setLinkTimeoutResult = reinterpret_cast<SetLinkTimeoutResult ^>(context);
    // Store the current status
    setLinkTimeoutResult->Status = (AllJoyn::QStatus)s;
    // Store the timeout value
    setLinkTimeoutResult->Timeout = timeout;
    // Signal the result is complete
    setLinkTimeoutResult->Complete();
}

void _BusAttachment::DispatchCallback(Windows::UI::Core::DispatchedHandler ^ callback)
{
    Windows::UI::Core::CoreWindow ^ window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    Windows::UI::Core::CoreDispatcher ^ dispatcher = nullptr;
    if (nullptr != window) {
        dispatcher = window->Dispatcher;
    }
    if (_originSTA && nullptr != _dispatcher && _dispatcher != dispatcher) {
        // Our origin was STA and the thread dispatcher doesn't match up. Move execution to the origin dispatcher thread.
        Windows::Foundation::IAsyncAction ^ op = _dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                                                                       ref new Windows::UI::Core::DispatchedHandler([this, callback] () {
                                                                                                                        callback();
                                                                                                                    }));
        // Since we are now queued up, enable concurrency to prevent any unnecessary blocking (this turns whatever callback does into a no op)
        EnableConcurrentCallbacks();
        // Exceptions are caught by specific handlers. If wait() below throws, that is a bug in the handler wrapper implementation.
        concurrency::task<void> dispatcherOp(op);
        dispatcherOp.wait();
    } else {
        // In this case, our source origin is MTA or we are STA with either no dispatcher (no UI threads involved) or we are already in the dispatcher thread
        // for the STA compartment.
        callback();
    }
}

bool _BusAttachment::IsOriginSTA()
{
    APTTYPE aptType;
    APTTYPEQUALIFIER aptTypeQualifier;
    // Gets the value of the apartment for the current thread
    HRESULT hr = ::CoGetApartmentType(&aptType, &aptTypeQualifier);
    if (SUCCEEDED(hr)) {
        // These apartment types are returned for STA
        if (aptType == APTTYPE_MAINSTA || aptType == APTTYPE_STA) {
            return true;
        }
    }
    // Not STA
    return false;
}

__BusAttachment::__BusAttachment()
{
    DBusProxyBusObject = nullptr;
    AllJoynProxyBusObject = nullptr;
    AllJoynDebugProxyBusObject = nullptr;
    UniqueName = nullptr;
    GlobalGUIDString = nullptr;
    Timestamp = (uint32_t)-1;
}

__BusAttachment::~__BusAttachment()
{
    DBusProxyBusObject = nullptr;
    AllJoynProxyBusObject = nullptr;
    AllJoynDebugProxyBusObject = nullptr;
    UniqueName = nullptr;
    GlobalGUIDString = nullptr;
    Timestamp = (uint32_t)-1;
}

}
