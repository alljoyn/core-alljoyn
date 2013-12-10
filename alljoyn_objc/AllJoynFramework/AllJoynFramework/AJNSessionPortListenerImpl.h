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
#import <alljoyn/SessionPortListener.h>
#import <alljoyn/TransportMask.h>
#import "AJNBusListener.h"
#import "AJNBusAttachment.h"

class AJNSessionPortListenerImpl : public ajn::SessionPortListener {
  protected:
    static const char* AJN_SESSION_PORT_LISTENER_DISPATCH_QUEUE_NAME;
    __weak AJNBusAttachment* busAttachment;

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNSessionPortListener> m_delegate;

  public:


    /**
     * Constructor for the AJN session port listener implementation.
     *
     * @param aBusAttachment    Objective C bus attachment wrapper object.
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNSessionPortListenerImpl(AJNBusAttachment* aBusAttachment, id<AJNSessionPortListener> aDelegate);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNSessionPortListenerImpl();

    /**
     * Accept or reject an incoming JoinSession request. The session does not exist until this
     * after this function returns.
     *
     * This callback is only used by session creators. Therefore it is only called on listeners
     * passed to BusAttachment::BindSessionPort.
     *
     * @param sessionPort    Session port that was joined.
     * @param joiner         Unique name of potential joiner.
     * @param opts           Session options requested by the joiner.
     * @return   Return true if JoinSession request is accepted. false if rejected.
     */
    virtual bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts);

    /**
     * Called by the bus when a session has been successfully joined. The session is now fully up.
     *
     * This callback is only used by session creators. Therefore it is only called on listeners
     * passed to BusAttachment::BindSessionPort.
     *
     * @param sessionPort    Session port that was joined.
     * @param id             Id of session.
     * @param joiner         Unique name of the joiner.
     */
    virtual void SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId sessionId, const char* joiner);


    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNSessionPortListener> getDelegate();

    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNSessionPortListener> delegate);
};

inline id<AJNSessionPortListener> AJNSessionPortListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNSessionPortListenerImpl::setDelegate(id<AJNSessionPortListener> delegate)
{
    m_delegate = delegate;
}
