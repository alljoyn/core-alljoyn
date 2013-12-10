/**
 * @file
 * BusListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to asynchronously receive bus  related event information.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BUSLISTENER_H
#define _ALLJOYN_BUSLISTENER_H

#ifndef __cplusplus
#error Only include BusListener.h in C++ code.
#endif

#include <alljoyn/TransportMask.h>

namespace ajn {

/**
 * Foward declaration.
 */
class BusAttachment;
class MsgArg;

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of bus related events.
 */
class BusListener {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~BusListener() { }

    /**
     * Called by the bus when the listener is registered. This gives the listener implementation the
     * opportunity to save a reference to the bus.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     *
     * @param bus  The bus the listener is registered with.
     */
    virtual void ListenerRegistered(BusAttachment* bus) { }

    /**
     * Called by the bus when the listener is unregistered.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     */
    virtual void ListenerUnregistered() { }

    /**
     * Called by the bus when an external bus is discovered that is advertising a well-known name
     * that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
     *
     * See also these sample file(s): @n
     * basic/basic_client.cc @n
     * basic/nameChange_client.cc @n
     * basic/README.windows.txt @n
     * basic/signalConsumer_client.cc @n
     * chat/android/jni/Chat_jni.cpp @n
     * chat/linux/chat.cc @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     * secure/DeskTopSharedKSClient.cc @n
     * simple/android/client/jni/Client_jni.cpp @n
     * windows/chat/ChatLib32/ChatClasses.cpp @n
     * windows/chat/ChatLib32/ChatClasses.h @n
     * windows/Client/Client.cpp @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.cpp @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.h @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/blank/blank/MainPage.xaml.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/App.xaml.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/MainPage.xaml.cs @n
     * csharp/Secure/Secure/Common/Client.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * @param name         A well known name that the remote bus is advertising.
     * @param transport    Transport that received the advertisement.
     * @param namePrefix   The well-known name prefix used in call to FindAdvertisedName that triggered this callback.
     */
    virtual void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) { }

    /**
     * Called by the bus when an advertisement previously reported through FoundName has become unavailable.
     *
     * See also these sample file(s): @n
     * chat/linux/chat.cc @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     * simple/android/client/jni/Client_jni.cpp @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/App.xaml.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/MainPage.xaml.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * @param name         A well known name that the remote bus is advertising that is of interest to this attachment.
     * @param transport    Transport that stopped receiving the given advertised name.
     * @param namePrefix   The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.
     */
    virtual void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) { }

    /**
     * Called by the bus when the ownership of any well-known name changes.
     *
     * See also these sample file(s): @n
     * basic/basic_client.cc @n
     * basic/basic_service.cc @n
     * basic/nameChange_client.cc @n
     * basic/signalConsumer_client.cc @n
     * basic/signal_service.cc @n
     * chat/android/jni/Chat_jni.cpp @n
     * chat/linux/chat.cc @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     * secure/DeskTopSharedKSClient.cc @n
     * secure/DeskTopSharedKSService.cc @n
     * simple/android/client/jni/Client_jni.cpp @n
     * simple/android/service/jni/Service_jni.cpp @n
     * windows/chat/ChatLib32/ChatClasses.cpp @n
     * windows/chat/ChatLib32/ChatClasses.h @n
     * windows/Client/Client.cpp @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.cpp @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.h @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/src/MediaSource.cc @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Service.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     *
     * @param busName        The well-known name that has changed.
     * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
     * @param newOwner       The unique name that now owns the name or NULL if the there is no new owner.
     */
    virtual void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner) { }

    /**
     * Called by the bus when the value of a property changes if that property has annotation
     *
     * @param propName       The well-known name that has changed.
     * @param propValue      The new value of the property; NULL if not present
     */
    virtual void PropertyChanged(const char* propName, const MsgArg* propValue) { }

    /**
     * Called when a BusAttachment this listener is registered with is stopping.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     */
    virtual void BusStopping() { }

    /**
     * Called when a BusAttachment this listener is registered with has become disconnected from
     * the bus.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferClient.cc @n
     * FileTransfer/FileTransferService.cc @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/tests/csharp/MediaPlayerApp/MainPage.xaml.cs @n
     * cpp/AllJoynStreaming/tests/csharp/MediaServerApp/MainPage.xaml.cs @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Client/BasicClient/AllJoynObjects.h @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.cpp @n
     * cpp/Basic/Name_Change_Client/NameChangeClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Consumer_Client/SignalConsumerClient/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/Basic/Basic_Client/BasicClient/Common/BasicClientBusListener.cs @n
     * csharp/Basic/Basic_Client/BasicClient/Common/Class1.cs @n
     * csharp/Basic/Basic_Service/BasicService/Common/BasicServiceBusListener.cs @n
     * csharp/Basic/Name_Change_Client/NameChangeClient/Common/NameChangeBusListener.cs @n
     * csharp/Basic/Signal_Consumer_Client/SignalConsumerClient/Common/SignalConsumerBusListener.cs @n
     * csharp/Basic/Signal_Service/SignalService/Common/SignalServiceBusListener.cs @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     */
    virtual void BusDisconnected() { }

};

}

#endif
