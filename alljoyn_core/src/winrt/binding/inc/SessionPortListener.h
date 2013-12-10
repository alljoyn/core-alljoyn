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

#include <alljoyn/SessionPortListener.h>
#include <alljoyn/Session.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <qcc/ManagedObj.h>

namespace AllJoyn {

ref class SessionOpts;
ref class BusAttachment;

/// <summary>
///Accept or reject an incoming JoinSession request. The session does not exist until
///after this function returns.
/// </summary>
/// <remarks>
///This callback is only used by session creators. Therefore it is only called on listeners
///passed to BusAttachment::BindSessionPort.
/// </remarks>
/// <param name="sessionPort">Session port that was joined.</param>
/// <param name="joiner">Unique name of potential joiner.</param>
/// <param name="opts">Session options requested by the joiner.</param>
/// <returns>
///Return true if JoinSession request is accepted. false if rejected.
/// </returns>
public delegate bool SessionPortListenerAcceptSessionJoinerHandler(ajn::SessionPort sessionPort, Platform::String ^ joiner, SessionOpts ^ opts);

/// <summary>
///Called by the bus when a session has been successfully joined. The session is now fully up.
/// </summary>
/// <remarks>
///This callback is only used by session creators. Therefore it is only called on listeners
///passed to BusAttachment::BindSessionPort.
/// </remarks>
/// <param name="sessionPort">Session port that was joined.</param>
/// <param name="id">Id of session.</param>
/// <param name="joiner">Unique name of the joiner.</param>
public delegate void SessionPortListenerSessionJoinedHandler(ajn::SessionPort sessionPort, ajn::SessionId id, Platform::String ^ joiner);

ref class __SessionPortListener {
  private:
    friend ref class SessionPortListener;
    friend class _SessionPortListener;
    __SessionPortListener();
    ~__SessionPortListener();

    event SessionPortListenerAcceptSessionJoinerHandler ^ AcceptSessionJoiner;
    event SessionPortListenerSessionJoinedHandler ^ SessionJoined;
    property BusAttachment ^ Bus;
};

class _SessionPortListener : protected ajn::SessionPortListener {
  protected:
    friend class qcc::ManagedObj<_SessionPortListener>;
    friend ref class SessionPortListener;
    friend ref class BusAttachment;
    _SessionPortListener(BusAttachment ^ bus);
    ~_SessionPortListener();

    bool DefaultSessionPortListenerAcceptSessionJoinerHandler(ajn::SessionPort sessionPort, Platform::String ^ joiner, SessionOpts ^ opts);
    void DefaultSessionPortListenerSessionJoinedHandler(ajn::SessionPort sessionPort, ajn::SessionId id, Platform::String ^ joiner);
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts);
    void SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner);

    __SessionPortListener ^ _eventsAndProperties;
};

/// <summary>
///AllJoyn uses this class to inform user of session related events.
/// </summary>
public ref class SessionPortListener sealed {
  public:
    SessionPortListener(BusAttachment ^ bus);

    /// <summary>
    ///Called when the a JoinSession request has been made.
    /// </summary>
    event SessionPortListenerAcceptSessionJoinerHandler ^ AcceptSessionJoiner
    {
        Windows::Foundation::EventRegistrationToken add(SessionPortListenerAcceptSessionJoinerHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        bool raise(ajn::SessionPort sessionPort, Platform::String ^ joiner, SessionOpts ^ opts);
    }

    /// <summary>
    ///Called when the session has been successfully joined.
    /// </summary>
    event SessionPortListenerSessionJoinedHandler ^ SessionJoined
    {
        Windows::Foundation::EventRegistrationToken add(SessionPortListenerSessionJoinedHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(ajn::SessionPort sessionPort, ajn::SessionId id, Platform::String ^ joiner);
    }

    /// <summary>
    ///Retrieve the <c>BusAttachment</c> object related to this <c>SessionPortListener</c>.
    /// </summary>
    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

  private:
    friend ref class BusAttachment;
    SessionPortListener(const qcc::ManagedObj<_SessionPortListener>* listener);
    ~SessionPortListener();

    qcc::ManagedObj<_SessionPortListener>* _mListener;
    _SessionPortListener* _listener;
};

}
// SessionPortListener.h
