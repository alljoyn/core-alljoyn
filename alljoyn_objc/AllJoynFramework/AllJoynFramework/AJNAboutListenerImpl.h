////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

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