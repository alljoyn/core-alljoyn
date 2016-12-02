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
#import <alljoyn/BusObject.h>
#import <alljoyn/InterfaceDescription.h>

@protocol AJNBusObject;

class AJNBusObjectImpl : public ajn::BusObject {
  protected:
    __weak id<AJNBusObject> delegate;

  public:
    AJNBusObjectImpl(const char* path, id<AJNBusObject> aDelegate);

    AJNBusObjectImpl(ajn::BusAttachment& bus, const char* path, id<AJNBusObject> aDelegate);

    virtual QStatus RegisterSignalHandlers(ajn::BusAttachment& bus);

    virtual QStatus UnregisterSignalHandlers(ajn::BusAttachment& bus);

    virtual void ObjectRegistered();
};