////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
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
