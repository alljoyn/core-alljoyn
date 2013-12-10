////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
    
    // if we are using sessionless signals, ignore the session (obviously)
    if (gMessageFlags == kAJNMessageFlagSessionless) {
        sessionId = 0;
    }
    
    return Signal(NULL, sessionId, *chatSignalMember, &chatArg, 1, 0, gMessageFlags);
}


