/**
 * @file
 * Header file for Android P2P Helper Interface class
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

#ifndef _P2P_HELPER_INTERFACE_H
#define _P2P_HELPER_INTERFACE_H

#ifndef __cplusplus
#error Only include P2PHelperInterface.h in C++ code.
#endif

#include <alljoyn/Status.h>

#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>

#define QCC_MODULE "P2P_HELPER_INTERFACE"

namespace ajn {

class P2PHelperListener {
  public:
    virtual ~P2PHelperListener() { }

    virtual void OnFoundAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device) { }
    virtual void OnLostAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device) { }
    virtual void OnLinkEstablished(int32_t handle, qcc::String& interface) { }
    virtual void OnLinkError(int32_t handle, int32_t error) { }
    virtual void OnLinkLost(int32_t handle) { }

    virtual void HandleFindAdvertisedNameReply(int32_t result) { }
    virtual void HandleCancelFindAdvertisedNameReply(int32_t result) { }
    virtual void HandleAdvertiseNameReply(int32_t result) { }
    virtual void HandleCancelAdvertiseNameReply(int32_t result) { }
    virtual void HandleEstablishLinkReply(int32_t handle) { }
    virtual void HandleReleaseLinkReply(int32_t result) { }
    virtual void HandleGetInterfaceNameFromHandleReply(qcc::String& interface) { }
};

class P2PHelperInterface : public MessageReceiver {
  public:
    static const char* INTERFACE_NAME;
    static const char* WELL_KNOWN_NAME;
    static const char* OBJECT_PATH;

    static const int DEVICE_SHOULD_BE_GO = 14;
    static const int DEVICE_SHOULD_BE_STA = 0;

    static const int P2P_OK = 0;
    static const int P2P_ERR = -1;

    P2PHelperInterface();
    virtual ~P2PHelperInterface();

    QStatus Init(BusAttachment* bus);
    bool ServiceExists(void);
    void SetListener(P2PHelperListener* listener);

    QStatus FindAdvertisedName(const qcc::String& namePrefix, int32_t& result) const;
    QStatus FindAdvertisedNameAsync(const qcc::String& namePrefix) const;

    QStatus CancelFindAdvertisedName(const qcc::String& namePrefix, int32_t& result) const;
    QStatus CancelFindAdvertisedNameAsync(const qcc::String& namePrefix) const;

    QStatus AdvertiseName(const qcc::String& namePrefix, const qcc::String& guid, int32_t& result) const;
    QStatus AdvertiseNameAsync(const qcc::String& namePrefix, const qcc::String& guid) const;

    QStatus CancelAdvertiseName(const qcc::String& namePrefix, const qcc::String& guid, int32_t& result) const;
    QStatus CancelAdvertiseNameAsync(const qcc::String& namePrefix, const qcc::String& guid) const;

    QStatus EstablishLink(const qcc::String& device, uint32_t intent, int32_t& handle) const;
    QStatus EstablishLinkAsync(const qcc::String& device, uint32_t intent) const;

    QStatus ReleaseLink(int32_t handle, int32_t& result) const;
    QStatus ReleaseLinkAsync(int32_t handle) const;

    QStatus GetInterfaceNameFromHandle(int32_t handle, qcc::String& interface) const;
    QStatus GetInterfaceNameFromHandleAsync(int32_t handle) const;

  private:
    QStatus RegisterSignalHandlers(void);
    QStatus UnregisterSignalHandlers(void);

    void HandleAddMatchReply(Message& message, void* context);

    const ProxyBusObject* m_dbusProxyBusObject;
    ProxyBusObject* m_proxyBusObject;
    InterfaceDescription* m_interface;
    BusAttachment* m_bus;
    P2PHelperListener* m_listener;

    class P2PHelperListenerInternal : public MessageReceiver {
      public:
        P2PHelperListenerInternal(P2PHelperInterface* p2pHelperInterface) : m_parent(p2pHelperInterface) { }

        void OnFoundAdvertisedName(const InterfaceDescription::Member* member, const char* sourcePath, Message& message)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::OnFoundAdvertisedName()"));
            if (message->GetType() == MESSAGE_SIGNAL && m_parent && m_parent->m_listener) {
                qcc::String name(message->GetArg(0)->v_string.str);
                qcc::String namePrefix(message->GetArg(1)->v_string.str);
                qcc::String guid(message->GetArg(2)->v_string.str);
                qcc::String device(message->GetArg(3)->v_string.str);

                m_parent->m_listener->OnFoundAdvertisedName(name, namePrefix, guid, device);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::OnFoundAdvertisedName(): Discard."));
            }
        }

        void OnLostAdvertisedName(const InterfaceDescription::Member* member, const char* sourcePath, Message& message)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::OnLostAdvertisedName()"));
            if (message->GetType() == MESSAGE_SIGNAL && m_parent && m_parent->m_listener) {
                qcc::String name(message->GetArg(0)->v_string.str);
                qcc::String namePrefix(message->GetArg(1)->v_string.str);
                qcc::String guid(message->GetArg(2)->v_string.str);
                qcc::String device(message->GetArg(3)->v_string.str);

                m_parent->m_listener->OnLostAdvertisedName(name, namePrefix, guid, device);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::OnLostAdvertisedName(): Discard."));
            }

        }

        void OnLinkEstablished(const InterfaceDescription::Member* member, const char* sourcePath, Message& message)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::OnLinkEstablished()"));
            if (message->GetType() == MESSAGE_SIGNAL && m_parent && m_parent->m_listener) {
                uint32_t handle(message->GetArg(0)->v_int32);
                qcc::String interface(message->GetArg(1)->v_string.str);

                m_parent->m_listener->OnLinkEstablished(handle, interface);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::OnLinkEstablished(): Discard."));
            }
        }

        void OnLinkError(const InterfaceDescription::Member* member, const char* sourcePath, Message& message)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::OnLinkError()"));
            if (message->GetType() == MESSAGE_SIGNAL && m_parent && m_parent->m_listener) {
                m_parent->m_listener->OnLinkError(message->GetArg(0)->v_int32, message->GetArg(1)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::OnLinkError(): Discard."));
            }
        }

        void OnLinkLost(const InterfaceDescription::Member* member, const char* sourcePath, Message& message)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::OnLinkLost()"));
            if (message->GetType() == MESSAGE_SIGNAL && m_parent && m_parent->m_listener) {
                m_parent->m_listener->OnLinkLost(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::OnLinkLost(): Discard."));
            }
        }

        void HandleFindAdvertisedNameReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleFindAdvertisedNameReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                m_parent->m_listener->HandleFindAdvertisedNameReply(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleFindAdvertisedNameReply(): Discard"));
            }
        }

        void HandleCancelFindAdvertisedNameReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleCancelFindAdvertisedNameReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                m_parent->m_listener->HandleCancelFindAdvertisedNameReply(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleCancelFindAdvertisedNameReply(): Discard."));
            }
        }

        void HandleAdvertiseNameReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleAdvertiseNameReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                m_parent->m_listener->HandleAdvertiseNameReply(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleAdvertiseNameReply(): Discard."));
            }
        }

        void HandleCancelAdvertiseNameReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleCancelAdvertiseNameReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                m_parent->m_listener->HandleCancelAdvertiseNameReply(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleCancelAdvertiseNameReply(): Discard."));
            }
        }

        void HandleEstablishLinkReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleEstablishLinkReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                m_parent->m_listener->HandleEstablishLinkReply(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleEstablishLinkReply(): Discard."));
            }
        }

        void HandleReleaseLinkReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleReleaseLinkReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                m_parent->m_listener->HandleReleaseLinkReply(message->GetArg(0)->v_int32);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleReleaseLinkReply(): Discard."));
            }
        }

        void HandleGetInterfaceNameFromHandleReply(Message& message, void* context)
        {
            QCC_DbgPrintf(("P2PHelperListenerInternal::HandleGetInterfaceNameFromHandleReply()"));
            if (message->GetType() == MESSAGE_METHOD_RET && m_parent && m_parent->m_listener) {
                qcc::String interface(message->GetArg(0)->v_string.str);
                m_parent->m_listener->HandleGetInterfaceNameFromHandleReply(interface);
            } else {
                QCC_DbgPrintf(("P2PHelperListenerInternal::HandleGetInterfaceNameFromHandleReply(): Discard."));
            }
        }

      private:
        P2PHelperInterface* m_parent;
    };

    P2PHelperListenerInternal* m_listenerInternal;
};

} // namespace ajn

#undef QCC_MODULE

#endif // _P2P_HELPER_INTERFACE_H
