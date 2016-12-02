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
#import "AJNApplicationStateListenerImpl.h"


AJNApplicationStateListenerImpl::AJNApplicationStateListenerImpl(id<AJNApplicationStateListener> delegate) :
    m_delegate(delegate)
{
}

AJNApplicationStateListenerImpl::~AJNApplicationStateListenerImpl()
{
    m_delegate = nil;
}

void AJNApplicationStateListenerImpl::State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state)
{
    /*
     * Check that the delegate implements state
     */
    if ([m_delegate respondsToSelector:@selector(state)]) {
        AJNKeyInfoNISTP256 *keyInfo = [[AJNKeyInfoNISTP256 alloc] initWithHandle:(AJNHandle)&publicKeyInfo];

        [m_delegate state:[NSString stringWithCString:busName encoding:NSUTF8StringEncoding] publicKeyInfo:keyInfo state:(AJNApplicationState)state];
    }
}
