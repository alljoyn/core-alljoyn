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
#import "AJNPingListenerImpl.h"

AJNPingListenerImpl::AJNPingListenerImpl(id<AJNPingListener> aDelegate):
    m_delegate(aDelegate)
{
}

AJNPingListenerImpl::~AJNPingListenerImpl()
{
    m_delegate = nil;
}

void AJNPingListenerImpl::DestinationLost(const qcc::String& group, const qcc::String& destination)
{
    if ([m_delegate respondsToSelector:@selector(destinationLost:group:destination:)]) {
           [m_delegate destinationLost:[NSString stringWithCString:group.c_str() encoding:NSUTF8StringEncoding] forDestination:[NSString stringWithCString:destination.c_str() encoding:NSUTF8StringEncoding]];
    }
}

void AJNPingListenerImpl::DestinationFound(const qcc::String& group, const qcc::String& destination)
{
    if ([m_delegate respondsToSelector:@selector(destinationFound:group:destination:)]) {
           [m_delegate destinationFound:[NSString stringWithCString:group.c_str() encoding:NSUTF8StringEncoding] forDestination:[NSString stringWithCString:destination.c_str() encoding:NSUTF8StringEncoding]];
    }
}