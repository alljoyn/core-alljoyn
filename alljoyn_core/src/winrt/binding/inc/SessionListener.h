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

#include <alljoyn/SessionListener.h>
#include <alljoyn/Session.h>
#include <qcc/ManagedObj.h>

namespace AllJoyn {

ref class BusAttachment;

/// <summary>
///Called by the bus when an existing session becomes disconnected.
/// </summary>
/// <param name="sessionId">Id of session that was lost.</param>
public delegate void SessionListenerSessionLostHandler(ajn::SessionId sessionId);

/// <summary>
/// Called by the bus when a member of a multipoint session is added.
/// </summary>
/// <param name="sessionId">Id of session whose member(s) changed.</param>
/// <param name="uniqueName">Unique name of member who was added.</param>
public delegate void SessionListenerSessionMemberAddedHandler(ajn::SessionId sessionId, Platform::String ^ uniqueName);

/// <summary>
/// Called by the bus when a member of a multipoint session is removed.
/// </summary>
/// <param name="sessionId">Id of session whose member(s) changed.</param>
/// <param name="uniqueName">Unique name of member who was removed.</param>
public delegate void SessionListenerSessionMemberRemovedHandler(ajn::SessionId sessionId, Platform::String ^ uniqueName);

ref class __SessionListener {
  private:
    friend ref class SessionListener;
    friend class _SessionListener;
    __SessionListener();
    ~__SessionListener();

    event SessionListenerSessionLostHandler ^ SessionLost;
    event SessionListenerSessionMemberAddedHandler ^ SessionMemberAdded;
    event SessionListenerSessionMemberRemovedHandler ^ SessionMemberRemoved;
    property BusAttachment ^ Bus;
};

class _SessionListener : protected ajn::SessionListener {
  protected:
    friend class qcc::ManagedObj<_SessionListener>;
    friend ref class SessionListener;
    friend ref class BusAttachment;
    _SessionListener(BusAttachment ^ bus);
    ~_SessionListener();

    void DefaultSessionListenerSessionLostHandler(ajn::SessionId sessionId);
    void DefaultSessionListenerSessionMemberAddedHandler(ajn::SessionId sessionId, Platform::String ^ uniqueName);
    void DefaultSessionListenerSessionMemberRemovedHandler(ajn::SessionId sessionId, Platform::String ^ uniqueName);
    void SessionLost(ajn::SessionId sessionId);
    void SessionMemberAdded(ajn::SessionId sessionId,  const char* uniqueName);
    void SessionMemberRemoved(ajn::SessionId sessionId,  const char* uniqueName);

    __SessionListener ^ _eventsAndProperties;
};

/// <summary>
///AllJoyn uses this class to inform users of session related events.
/// </summary>
public ref class SessionListener sealed {
  public:
    SessionListener(BusAttachment ^ bus);

    /// <summary>
    ///Called by the bus when an existing session becomes disconnected.
    /// </summary>
    event SessionListenerSessionLostHandler ^ SessionLost
    {
        Windows::Foundation::EventRegistrationToken add(SessionListenerSessionLostHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(ajn::SessionId sessionId);
    }

    /// <summary>
    ///Called by the bus when a member of a multipoint session is added.
    /// </summary>
    event SessionListenerSessionMemberAddedHandler ^ SessionMemberAdded
    {
        Windows::Foundation::EventRegistrationToken add(SessionListenerSessionMemberAddedHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(ajn::SessionId sessionId, Platform::String ^ uniqueName);
    }

    /// <summary>
    ///Called by the bus when a member of a multipoint session is removed.
    /// </summary>
    event SessionListenerSessionMemberRemovedHandler ^ SessionMemberRemoved
    {
        Windows::Foundation::EventRegistrationToken add(SessionListenerSessionMemberRemovedHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(ajn::SessionId sessionId, Platform::String ^ uniqueName);
    }

    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

  private:
    friend ref class BusAttachment;
    SessionListener(const qcc::ManagedObj<_SessionListener>* listener);
    ~SessionListener();

    qcc::ManagedObj<_SessionListener>* _mListener;
    _SessionListener* _listener;
};

}
// SessionListener.h
