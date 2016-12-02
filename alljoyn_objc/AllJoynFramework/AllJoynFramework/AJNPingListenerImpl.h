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
#import <alljoyn/PingListener.h>
#import "AJNPingListener.h"

using namespace ajn;

class AJNPingListenerImpl : public ajn::PingListener {
    protected:
        /**
         * Objective C delegate called when one of the below virtual functions
         * is called.
         */
        __weak id<AJNPingListener> m_delegate;

    public:
        /**
         * Constructor for the AJNPingListener implementation.
         *
         * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
         */
        AJNPingListenerImpl(id<AJNPingListener> aDelegate);

        /**
         * Virtual destructor for derivable class.
         */
        virtual ~AJNPingListenerImpl();

        /**
         * Destination Lost: called when the destination becomes unreachable.
         * Called once.
         *
         * @param  group Pinging group name
         * @param  destination Destination that was pinged
         */
        virtual void DestinationLost(const qcc::String& group, const qcc::String& destination);

        /**
         * Destination Found: called when the destination becomes reachable.
         * Called once.
         *
         * @param  group Pinging group name
         * @param  destination Destination that was pinged
         */
        virtual void DestinationFound(const qcc::String& group, const qcc::String& destination);

        /**
         * Accessor for Objective-C delegate.
         *
         * @return  The Objective-C delegate called to handle the above event methods.
         */
        id<AJNPingListener> getDelegate();

        /**
         * Mutator for Objective-C delegate.
         *
         * @param delegate    The Objective-C delegate called to handle the above event methods.
         */
        void setDelegate(id<AJNPingListener> delegate);
};

inline id<AJNPingListener> AJNPingListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNPingListenerImpl::setDelegate(id<AJNPingListener> delegate)
{
    m_delegate = delegate;
}