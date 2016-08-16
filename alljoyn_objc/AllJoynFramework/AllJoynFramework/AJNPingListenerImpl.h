////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
         * @return delegate         The Objective-C delegate called to handle the above event methods.
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
