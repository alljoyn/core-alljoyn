////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#import <alljoyn/AboutListener.h>
#import <alljoyn/TransportMask.h>
#import "AJNBusListener.h"
#import "AJNBusAttachment.h"
#import "AJNAboutListener.h"

class AJNAboutListenerImpl : public ajn::AboutListener {
protected:
    static const char* AJN_ABOUT_LISTENER_DISPATCH_QUEUE_NAME;
    __weak AJNBusAttachment* busAttachment;
    
    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNAboutListener> m_delegate;
    
public:
    
    /**
     * Constructor for the AJNAboutListener implementation.
     *
     * @param aBusAttachment    Objective C bus attachment wrapper object.
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNAboutListenerImpl(AJNBusAttachment* aBusAttachment, id<AJNAboutListener> aDelegate);
    
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNAboutListenerImpl();
    
    virtual void Announced(const char* busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg);
    
    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNAboutListener> getDelegate();
    
    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNAboutListener> delegate);
};

inline id<AJNAboutListener> AJNAboutListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNAboutListenerImpl::setDelegate(id<AJNAboutListener> delegate)
{
    m_delegate = delegate;
}
