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
#import <alljoyn/BusAttachment.h>
#import <alljoyn/InterfaceDescription.h>
#import <alljoyn/MessageReceiver.h>
#import "AJNBusAttachment.h"
#import "AJNSignalHandler.h"

class AJNSignalHandlerImpl : public ajn::MessageReceiver {

  protected:

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNSignalHandler> m_delegate;

  public:

    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNSignalHandlerImpl(id<AJNSignalHandler> aDelegate);

    /**
     * Pure virtual registration function. Implement in derived classes to handle
     * registration of signal handlers.
     */
    virtual void RegisterSignalHandler(ajn::BusAttachment& bus) = 0;

    /**
     * Pure virtual unregistration function. Implement in derived classes to
     * handle unregistration of signal handlers.
     */
    virtual void UnregisterSignalHandler(ajn::BusAttachment& bus) = 0;

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNSignalHandlerImpl();

    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNSignalHandler> getDelegate();

    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNSignalHandler> delegate);
};

inline id<AJNSignalHandler> AJNSignalHandlerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNSignalHandlerImpl::setDelegate(id<AJNSignalHandler> delegate)
{
    m_delegate = delegate;
}