/**
 * @file
 * MessageReceiverC is an implementation of the MessagReceiver base class responsible
 * for mapping a C++ style MessageReceiver to a C style alljoyn_messagereceiver
 * function pointer
 */

/******************************************************************************
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
