/**
 * @file
 * MessageReceiverC is an implementation of the MessagReceiver base class responsible
 * for mapping a C++ style MessageReceiver to a C style alljoyn_messagereceiver
 * function pointer
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _ALLJOYN_C_MESSAGERECEIVERC_H
#define _ALLJOYN_C_MESSAGERECEIVERC_H

#include <alljoyn/MessageReceiver.h>

namespace ajn {
/*
 * When setting up a asynchronous method call a  callback handler for C
 * alljoyn_messagereceiver_replyhandler_ptr function pointer will be passed in as
 * the callback handler.  AllJoyn expects a MessageReceiver::ReplyHandler method
 * handler.  The function handler will be passed as the part of the void*
 * context that is passed to the internal callback handler and will then be used
 * to map the internal callback handler to the user defined
 * messagereceived_replyhandler callback function pointer.
 */
class MessageReceiverReplyHandlerCallbackContext {
  public:
    MessageReceiverReplyHandlerCallbackContext(alljoyn_messagereceiver_replyhandler_ptr replyhandler_ptr, void* context) :
        replyhandler_ptr(replyhandler_ptr), context(context) { }

    alljoyn_messagereceiver_replyhandler_ptr replyhandler_ptr;
    void* context;
};

class MessageReceiverC : public MessageReceiver {
  public:
    void ReplyHandler(ajn::Message& message, void* context)
    {
        /*
         * The replyhandler_ptr in the context pointer is allocated as part
         * of the alljoyn_proxybusobject_methodcallasync function call or the
         * alljoyn_proxybusobject_methodcallasync_member function call.
         * Once the function alljoyn_messagereceiver_replyhandler_ptr has been
         * called the MessageReceiverReplyHandlerCallbackContext instance must
         * be deleted or it will result in a memory leak.
         */
        MessageReceiverReplyHandlerCallbackContext* in = (MessageReceiverReplyHandlerCallbackContext*)context;
        (in->replyhandler_ptr)((alljoyn_message) & message, in->context);
        in->replyhandler_ptr = NULL;
        delete in;
    }
};
}

#endif