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

#import "AJNBusObjectImpl.h"
#import "AJNBusObject.h"

using namespace ajn;

AJNBusObjectImpl::AJNBusObjectImpl(const char* path, id<AJNBusObject> aDelegate) : BusObject(path), delegate(aDelegate)
{

}

AJNBusObjectImpl::AJNBusObjectImpl(BusAttachment& bus, const char* path, id<AJNBusObject> aDelegate) : BusObject(path), delegate(aDelegate)
{

}

QStatus AJNBusObjectImpl::RegisterSignalHandlers(ajn::BusAttachment &bus)
{
    // override in derived classes to register signal handlers with the bus
    //
    return ER_OK;
}

QStatus AJNBusObjectImpl::UnregisterSignalHandlers(ajn::BusAttachment &bus)
{
    // override in derived classes to unregister signal handlers with the bus
    //
    return ER_OK;    
}

void AJNBusObjectImpl::ObjectRegistered()
{
    @autoreleasepool {
        [delegate objectWasRegistered];
    }
}