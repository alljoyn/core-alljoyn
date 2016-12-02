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

#import <alljoyn/BusAttachment.h>
#import <alljoyn/BusObject.h>
#import "BasicSampleObjectImpl.h"
#import "BasicSampleObject.h"

using namespace ajn;

BasicSampleObjectImpl::BasicSampleObjectImpl(ajn::BusAttachment &bus, const char *path, id<MyMethodSample> aDelegate) :
 AJNBusObjectImpl(bus,path,aDelegate)
{
    /** Add the test interface to this object */
    const InterfaceDescription* basicInterface = bus.GetInterface([kBasicObjectInterfaceName UTF8String]);
    assert(basicInterface);
    AddInterface(*basicInterface);
    
    /** Register the method handlers with the object */
    const MethodEntry methodEntries[] = {
        { basicInterface->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&BasicSampleObjectImpl::Concatenate) }
    };
    QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
    }    
}

void BasicSampleObjectImpl::Concatenate(const InterfaceDescription::Member *member, Message& msg)
{
    /* Concatenate the two input strings and reply with the result. */
    qcc::String inStr1 = msg->GetArg(0)->v_string.str;
    qcc::String inStr2 = msg->GetArg(1)->v_string.str;

    NSString *returnValue = [(id<MyMethodSample>)delegate concatenateString:[NSString stringWithCString:inStr1.c_str() encoding:NSUTF8StringEncoding] withString:[NSString stringWithCString:inStr2.c_str() encoding:NSUTF8StringEncoding]];    
    MsgArg outArg("s", [returnValue UTF8String]);
    QStatus status = MethodReply(msg, &outArg, 1);
    if (ER_OK != status) {
        // yeah!
    }
    else {
        // oh noes!
    }
}

void BasicSampleObjectImpl::ObjectRegistered()
{
    AJNBusObjectImpl::ObjectRegistered();
}