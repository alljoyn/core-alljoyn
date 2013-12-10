/**
 * @file
 * SessionListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to receive sessions related event information.
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
#ifndef _ALLJOYN_SESSIONLISTENER_H
#define _ALLJOYN_SESSIONLISTENER_H

#ifndef __cplusplus
#error Only include SessionListener.h in C++ code.
#endif

#include <alljoyn/Session.h>

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of session related events.
 */
class SessionListener {
  public:

    /** Reason for the session being lost */
    typedef enum {
        ALLJOYN_SESSIONLOST_INVALID                      = 0x00, /**< Invalid */
        ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION      = 0x01, /**< Remote end called LeaveSession */
        ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY   = 0x02, /**< Remote end closed abruptly */
        ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER            = 0x03, /**< Session binder removed this endpoint by calling RemoveSessionMember */
        ALLJOYN_SESSIONLOST_LINK_TIMEOUT                 = 0x04, /**< Link was timed-out */
        ALLJOYN_SESSIONLOST_REASON_OTHER                 = 0x05 /**< Unspecified reason for session loss */
    } SessionLostReason;

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~SessionListener() { }

    /**
     * Called by the bus when an existing session becomes disconnected. (Deprecated)
     *
     * @see SessionLost(SessionId sessionId, SessionLostReason reason)
     *
     * @param sessionId     Id of session that was lost.
     */
    QCC_DEPRECATED(virtual void SessionLost(SessionId sessionId)) { };

    /**
     * Called by the bus when an existing session becomes disconnected.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferService.cc @n
     * simple/android/client/jni/Client_jni.cpp @n
     * simple/android/service/jni/Service_jni.cpp @n
     *
     * For Windows 8 see also these sample file(s): @n
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
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/BusStress/BusStress/Common/ClientBusListener.cs @n
     * csharp/BusStress/BusStress/Common/ServiceBusListener.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/chat/chat/MainPage.xaml.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * @param sessionId     Id of session that was lost.
     * @param reason        The reason for the session being lost
     */
    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) { }

    /**
     * Called by the bus when a member of a multipoint session is added.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferService.cc @n
     *
     * For Windows 8 see also these sample file(s): @n
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
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * @param sessionId     Id of session whose member(s) changed.
     * @param uniqueName    Unique name of member who was added.
     */
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) { }

    /**
     * Called by the bus when a member of a multipoint session is removed.
     *
     * See also these sample file(s): @n
     * FileTransfer/FileTransferService.cc @n
     *
     * For Windows 8 see also these sample file(s): @n
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
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * @param sessionId     Id of session whose member(s) changed.
     * @param uniqueName    Unique name of member who was removed.
     */
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) { }
};

}

#endif
