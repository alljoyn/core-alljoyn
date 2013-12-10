////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>

#import <alljoyn/Session.h>
#import <alljoyn/SessionListener.h>
#import <alljoyn/TransportMask.h>

#import "AJNBusAttachment.h"
#import "AJNSessionListener.h"

class AJNSessionListenerImpl : public ajn::SessionListener {
  protected:
    __weak AJNBusAttachment* busAttachment;

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNSessionListener> m_delegate;

  public:
    /**
     * Constructor for the AJN session listener implementation.
     *
     * @param aBusAttachment    Objective C bus attachment wrapper object.
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNSessionListenerImpl(AJNBusAttachment* aBusAttachment, id<AJNSessionListener> aDelegate);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNSessionListenerImpl();

    /**
     * Called by the bus when an existing session becomes disconnected.
     *
     * @param sessionId     Id of session that was lost.
     */
    virtual void SessionLost(ajn::SessionId sessionId);

    /**
     * Called by the bus when an existing session becomes disconnected.
     *
     * @param sessionId     Id of session that was lost.
     * @param reason        The reason for the session being lost
     *
     */
    virtual void SessionLost(ajn::SessionId sessionId, SessionLostReason reason);

    /**
     * Called by the bus when a member of a multipoint session is added.
     *
     * @param sessionId     Id of session whose member(s) changed.
     * @param uniqueName    Unique name of member who was added.
     */
    virtual void SessionMemberAdded(ajn::SessionId sessionId, const char* uniqueName);

    /**
     * Called by the bus when a member of a multipoint session is removed.
     *
     * @param sessionId     Id of session whose member(s) changed.
     * @param uniqueName    Unique name of member who was removed.
     */
    virtual void SessionMemberRemoved(ajn::SessionId sessionId, const char* uniqueName);

    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNSessionListener> getDelegate();

    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNSessionListener> delegate);
};

inline id<AJNSessionListener> AJNSessionListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNSessionListenerImpl::setDelegate(id<AJNSessionListener> delegate)
{
    m_delegate = delegate;
}
