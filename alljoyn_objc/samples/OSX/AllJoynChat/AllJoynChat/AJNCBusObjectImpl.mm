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
#import "AJNCBusObjectImpl.h"
#import "AJNCBusObject.h"
#import "AJNCConstants.h"

AJNCBusObjectImpl::AJNCBusObjectImpl(ajn::BusAttachment &bus, const char *path, id<AJNBusObject> aDelegate) : AJNBusObjectImpl(bus,path,aDelegate)
{
    const ajn::InterfaceDescription* chatIntf = bus.GetInterface([kInterfaceName UTF8String]);
    assert(chatIntf);
    AddInterface(*chatIntf);

    /* Store the Chat signal member away so it can be quickly looked up when signals are sent */
    chatSignalMember = chatIntf->GetMember("Chat");
    assert(chatSignalMember);
}

/* send a chat signal */
QStatus AJNCBusObjectImpl::SendChatSignal(const char* msg, ajn::SessionId sessionId)
{
    NSLog(@"SendChatSignal( %s, %u)", msg, sessionId);

    ajn::MsgArg chatArg("s", msg);
    uint8_t flags = 0;
    return Signal(NULL, sessionId, *chatSignalMember, &chatArg, 1, 0, flags);
}

