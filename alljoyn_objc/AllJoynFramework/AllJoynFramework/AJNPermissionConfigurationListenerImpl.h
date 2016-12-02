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
#import <alljoyn/PermissionConfigurationListener.h>
#import "AJNPermissionConfigurationListener.h"

using namespace ajn;

class AJNPermissionConfigurationListenerImpl : public ajn::PermissionConfigurationListener {
    protected:
        /**
         * Objective C delegate called when one of the below virtual functions
         * is called.
         */
        __weak id<AJNPermissionConfigurationListener> m_delegate;

    public:
        /**
         * Constructor for the AJNPermissionConfigurationListener implementation.
         *
         * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
         */
        AJNPermissionConfigurationListenerImpl(id<AJNPermissionConfigurationListener> aDelegate);

        /**
         * Virtual destructor for derivable class.
         */
        virtual ~AJNPermissionConfigurationListenerImpl();

        /**
         * Handler for doing a factory reset of application state.
         *
         * @return  Return ER_OK if the application state reset was successful.
         */
        virtual QStatus FactoryReset();

        /**
         * Notification that the security manager has updated the security policy
         * for the application.
         */
        virtual void PolicyChanged();

        /**
         * Notification provided before Security Manager is starting to change settings for this application.
         */
        virtual void StartManagement();

        /**
         * Notification provided after Security Manager finished changing settings for this application.
         */
        virtual void EndManagement();

        /**
         * Accessor for Objective-C delegate.
         *
         * return delegate         The Objective-C delegate called to handle the above event methods.
         */
        id<AJNPermissionConfigurationListener> getDelegate();

        /**
         * Mutator for Objective-C delegate.
         *
         * @param delegate    The Objective-C delegate called to handle the above event methods.
         */
        void setDelegate(id<AJNPermissionConfigurationListener> delegate);
};

inline id<AJNPermissionConfigurationListener> AJNPermissionConfigurationListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNPermissionConfigurationListenerImpl::setDelegate(id<AJNPermissionConfigurationListener> delegate)
{
    m_delegate = delegate;
}