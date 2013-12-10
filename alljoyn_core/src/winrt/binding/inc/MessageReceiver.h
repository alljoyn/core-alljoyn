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
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/Message.h>
#include <qcc/String.h>
#include <Message.h>
#include <InterfaceDescription.h>
#include <InterfaceMember.h>
#include <qcc/winrt/utility.h>
#include <qcc/ManagedObj.h>
#include <qcc/Mutex.h>
#include <map>
#include <ObjectReference.h>
#include <AllJoynException.h>
#include <BusAttachment.h>

namespace AllJoyn {

ref class InterfaceMember;
ref class Message;
ref class MessageReceiver;
ref class BusAttachment;
ref class BusObject;
ref class ProxyBusObject;

/// <summary>
/// MethodHandlers are %MessageReceiver methods which are called by AllJoyn library
/// to forward AllJoyn method_calls to AllJoyn library users.
/// </summary>
/// <param name="member">Method interface member entry.</param>
/// <param name="message">The received method call message.</param>
public delegate void MessageReceiverMethodHandler(InterfaceMember ^ member, Message ^ message);


/// <summary>
/// SignalHandlers are %MessageReceiver methods which are called by AllJoyn library
/// to forward AllJoyn received signals to AllJoyn library users.
/// </summary>
/// <param name="member">Method or signal interface member entry.</param>
/// <param name="srcPath">Object path of signal emitter.</param>
/// <param name="message">The received message.</param>
public delegate void MessageReceiverSignalHandler(InterfaceMember ^ member, Platform::String ^ srcPath, Message ^ message);

ref class __MessageReceiver {
  private:
    friend ref class MessageReceiver;
    friend class _MessageReceiver;
    __MessageReceiver() { }
    ~__MessageReceiver() { }

    event MessageReceiverMethodHandler ^ MethodHandler;
    event MessageReceiverSignalHandler ^ SignalHandler;
};

class _MessageReceiver : protected ajn::MessageReceiver {
  protected:
    friend class qcc::ManagedObj<_MessageReceiver>;
    friend ref class MessageReceiver;
    friend ref class BusAttachment;
    friend ref class BusObject;
    friend ref class ProxyBusObject;
    friend class _BusObject;
    friend class _ProxyBusObject;
    _MessageReceiver(BusAttachment ^ bus)
        : Bus(bus)
    {
        ::QStatus status = ER_OK;

        while (true) {
            _eventsAndProperties = ref new __MessageReceiver();
            if (nullptr == _eventsAndProperties) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            _eventsAndProperties->MethodHandler += ref new MessageReceiverMethodHandler([&] (InterfaceMember ^ member, Message ^ message) {
                                                                                            DefaultMessageReceiverMethodHandler(member, message);
                                                                                        });
            _eventsAndProperties->SignalHandler += ref new MessageReceiverSignalHandler([&] (InterfaceMember ^ member, Platform::String ^ srcPath, Message ^ message) {
                                                                                            DefaultMessageReceiverSignalHandler(member, srcPath, message);
                                                                                        });
            break;
        }

        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }
    }

    ~_MessageReceiver()
    {
        _eventsAndProperties = nullptr;
        Bus = nullptr;
    }

    void DefaultMessageReceiverMethodHandler(InterfaceMember ^ member, Message ^ message)
    {
    }

    void DefaultMessageReceiverSignalHandler(InterfaceMember ^ member, Platform::String ^ srcPath, Message ^ message)
    {
    }

    ajn::MessageReceiver::MethodHandler GetMethodHandler()
    {
        return static_cast<ajn::MessageReceiver::MethodHandler>(&AllJoyn::_MessageReceiver::MethodHandler);
    }

    ajn::MessageReceiver::SignalHandler GetSignalHandler()
    {
        return static_cast<ajn::MessageReceiver::SignalHandler>(&AllJoyn::_MessageReceiver::SignalHandler);
    }

    void MethodHandler(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
    {
        ::QStatus status = ER_OK;

        try {
            while (true) {
                InterfaceMember ^ imember = ref new InterfaceMember(member);
                if (nullptr == imember) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                Message ^ message = ref new Message(&msg);
                if (nullptr == message) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                       _eventsAndProperties->MethodHandler(imember, message);

                                                                                                   }));
                break;
            }

            if (ER_OK != status) {
                QCC_THROW_EXCEPTION(status);
            }
        } catch (...) {
            // Do nothing
        }
    }

    void SignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
    {
        ::QStatus status = ER_OK;

        try {
            while (true) {
                InterfaceMember ^ imember = ref new InterfaceMember(member);
                if (nullptr == imember) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                Platform::String ^ strSrcPath = MultibyteToPlatformString(srcPath);
                if (nullptr == strSrcPath && srcPath != NULL && srcPath[0] != '\0') {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                Message ^ message = ref new Message(&msg);
                if (nullptr == message) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                       _eventsAndProperties->SignalHandler(imember, strSrcPath, message);
                                                                                                   }));
                break;
            }

            if (ER_OK != status) {
                QCC_THROW_EXCEPTION(status);
            }
        } catch (...) {
            // Do nothing
        }
    }

    __MessageReceiver ^ _eventsAndProperties;
    BusAttachment ^ Bus;
};

/// <summary>MessageReceiver is for notification of a method call or signal call. Users have to provide a handler for the method/signal call </summary>
public ref class MessageReceiver sealed {
  public:
    /// <summary>Constructor </summary>
    MessageReceiver(BusAttachment ^ bus)
    {
        ::QStatus status = ER_OK;

        while (true) {
            if (nullptr == bus) {
                status = ER_BAD_ARG_1;
                break;
            }
            _mReceiver = new qcc::ManagedObj<_MessageReceiver>(bus);
            if (NULL == _mReceiver) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            _receiver = &(**_mReceiver);
            break;
        }

        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }
    }

    /// <summary>Called when a method has been received</summary>
    event MessageReceiverMethodHandler ^ MethodHandler
    {
        Windows::Foundation::EventRegistrationToken add(MessageReceiverMethodHandler ^ handler)
        {
            return _receiver->_eventsAndProperties->MethodHandler::add(handler);
        }

        void remove(Windows::Foundation::EventRegistrationToken token)
        {
            _receiver->_eventsAndProperties->MethodHandler::remove(token);
        }

        void raise(InterfaceMember ^ member, Message ^ message)
        {
            _receiver->_eventsAndProperties->MethodHandler::raise(member, message);
        }
    }

    /// <summary>Called when a signal has been received</summary>
    event MessageReceiverSignalHandler ^ SignalHandler
    {
        Windows::Foundation::EventRegistrationToken add(MessageReceiverSignalHandler ^ handler)
        {
            return _receiver->_eventsAndProperties->SignalHandler::add(handler);
        }

        void remove(Windows::Foundation::EventRegistrationToken token)
        {
            _receiver->_eventsAndProperties->SignalHandler::remove(token);
        }

        void raise(InterfaceMember ^ member, Platform::String ^ srcPath, Message ^ message)
        {
            _receiver->_eventsAndProperties->SignalHandler::raise(member, srcPath, message);
        }
    }

  private:
    friend ref class ProxyBusObject;
    friend ref class BusObject;
    friend ref class BusAttachment;
    friend class _BusObject;
    friend class _ProxyBusObject;
    MessageReceiver()
    {
    }

    MessageReceiver(const qcc::ManagedObj<_MessageReceiver>* receiver)
    {
        ::QStatus status = ER_OK;

        while (true) {
            if (NULL == receiver) {
                status = ER_BAD_ARG_1;
                break;
            }
            _mReceiver = new qcc::ManagedObj<_MessageReceiver>(*receiver);
            if (NULL == _mReceiver) {
                status = ER_OUT_OF_MEMORY;
                break;
            }
            _receiver = &(**_mReceiver);
            break;
        }

        if (ER_OK != status) {
            QCC_THROW_EXCEPTION(status);
        }
    }

    ~MessageReceiver()
    {
        if (NULL != _mReceiver) {
            delete _mReceiver;
            _mReceiver = NULL;
            _receiver = NULL;
        }
    }

    qcc::ManagedObj<_MessageReceiver>* _mReceiver;
    _MessageReceiver* _receiver;
};

}
// MessageReceiver.h
