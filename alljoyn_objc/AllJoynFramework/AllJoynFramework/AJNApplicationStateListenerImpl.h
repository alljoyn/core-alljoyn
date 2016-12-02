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
#import <alljoyn/ApplicationStateListener.h>
#import "AJNApplicationStateListener.h"

using namespace ajn;

class AJNApplicationStateListenerImpl : public ajn::ApplicationStateListener {
    protected:
        /**
         * Objective C delegate called when one of the below virtual functions
         * is called.
         */
        __weak id<AJNApplicationStateListener> m_delegate;

    public:
        /**
         * Constructor for the AJNApplicationStateListener implementation.
         *
         * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
         */
        AJNApplicationStateListenerImpl(id<AJNApplicationStateListener> aDelegate);

        /**
         * Virtual destructor for derivable class.
         */
        virtual ~AJNApplicationStateListenerImpl();

        /**
         * Handler for the org.allseen.Bus.Application's State sessionless signal.
         *
         * @param[in] busName          unique name of the remote BusAttachment that
         *                             sent the State signal
         * @param[in] publicKeyInfo the application public key
         * @param[in] state the application state
         */
        virtual void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state);

        /**
         * Accessor for Objective-C delegate.
         *
         * return delegate         The Objective-C delegate called to handle the above event methods.
         */
        id<AJNApplicationStateListener> getDelegate();

        /**
         * Mutator for Objective-C delegate.
         *
         * @param delegate    The Objective-C delegate called to handle the above event methods.
         */
        void setDelegate(id<AJNApplicationStateListener> delegate);
};

inline id<AJNApplicationStateListener> AJNApplicationStateListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNApplicationStateListenerImpl::setDelegate(id<AJNApplicationStateListener> delegate)
{
    m_delegate = delegate;
}
