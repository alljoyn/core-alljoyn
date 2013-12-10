/**
 * @file
 * SessionPortListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to receive session port related event information.
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
#ifndef _ALLJOYN_SESSIONPORTLISTENER_H
#define _ALLJOYN_SESSIONPORTLISTENER_H

#ifndef __cplusplus
#error Only include SessionPortListener.h in C++ code.
#endif

#include <alljoyn/Session.h>
#include <alljoyn/SessionListener.h>

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of session related events.
 */
class SessionPortListener {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~SessionPortListener() { }

    /**
     * Accept or reject an incoming JoinSession request. The session does not exist until this
     * after this function returns.
     *
     * See also these sample file(s): @n
     * basic/basic_service.cc @n
     * basic/signal_service.cc @n
     * chat/android/jni/Chat_jni.cpp @n
     * chat/linux/chat.cc @n
     * FileTransfer/FileTransferService.cc @n
     * secure/DeskTopSharedKSService.cc @n
     * simple/android/service/jni/Service_jni.cpp @n
     * windows/chat/ChatLib32/ChatClasses.cpp @n
     * windows/chat/ChatLib32/ChatClasses.h @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.cpp @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.h @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/src/MediaSource.cc @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/chat/chat/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Service.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * This callback is only used by session creators. Therefore it is only called on listeners
     * passed to BusAttachment::BindSessionPort.
     *
     * @param sessionPort    Session port that was joined.
     * @param joiner         Unique name of potential joiner.
     * @param opts           Session options requested by the joiner.
     * @return   Return true if JoinSession request is accepted. false if rejected.
     */
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return false; }

    /**
     * Called by the bus when a session has been successfully joined. The session is now fully up.
     *
     * See also these sample file(s): @n
     * chat/android/jni/Chat_jni.cpp @n
     * chat/linux/chat.cc @n
     * FileTransfer/FileTransferService.cc @n
     * simple/android/service/jni/Service_jni.cpp @n
     * windows/chat/ChatLib32/ChatClasses.cpp @n
     * windows/chat/ChatLib32/ChatClasses.h @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.cpp @n
     * windows/PhotoChat/AllJoynBusLib/AllJoynConnection.h @n
     *
     * For Windows 8 see also these sample file(s): @n
     * cpp/AllJoynStreaming/src/MediaSource.cc @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.cpp @n
     * cpp/Basic/Basic_Service/BasicService/AllJoynObjects.h @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.cpp @n
     * cpp/Basic/Signal_Service/SignalService/AllJoynObjects.h @n
     * cpp/Chat/Chat/AllJoynObjects.cpp @n
     * cpp/Chat/Chat/AllJoynObjects.h @n
     * cpp/Secure/Secure/AllJoynObjects.cpp @n
     * cpp/Secure/Secure/AllJoynObjects.h @n
     * csharp/blank/blank/Common/Listeners.cs @n
     * csharp/FileTransfer/Client/Common/Listeners.cs @n
     * csharp/Secure/Secure/Common/Listeners.cs @n
     * csharp/Sessions/Sessions/Common/MyBusListener.cs @n
     * csharp/Sessions/Sessions/Common/SessionOperations.cs @n
     *
     * This callback is only used by session creators. Therefore it is only called on listeners
     * passed to BusAttachment::BindSessionPort.
     *
     * @param sessionPort    Session port that was joined.
     * @param id             Id of session.
     * @param joiner         Unique name of the joiner.
     */
    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {  }
};

}

#endif
