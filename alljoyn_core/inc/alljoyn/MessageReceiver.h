#ifndef _ALLJOYN_MESSAGERECEIVER_H
#define _ALLJOYN_MESSAGERECEIVER_H

#include <qcc/platform.h>

#include <alljoyn/Message.h>

/**
 * @file
 * MessageReceiver is a base class implemented by any class
 * which wishes to receive AllJoyn messages
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

namespace ajn {

/**
 * %MessageReceiver is a pure-virtual base class that is implemented by any
 * class that wishes to receive AllJoyn messages from the AllJoyn library.
 *
 * Received messages can be either signals, method_replies or errors.
 */
class MessageReceiver {
  public:
    /** Destructor */
    virtual ~MessageReceiver() { }

    /**
     * MethodHandlers are %MessageReceiver methods which are called by AllJoyn library
     * to forward AllJoyn method_calls to AllJoyn library users.
     *
     * @param member    Method interface member entry.
     * @param message   The received method call message.
     */
    typedef void (MessageReceiver::* MethodHandler)(const InterfaceDescription::Member* member, Message& message);

    /**
     * ReplyHandlers are %MessageReceiver methods which are called by AllJoyn library
     * to forward AllJoyn method_reply and error responses to AllJoyn library users.
     *
     * @param message   The received message.
     * @param context   User-defined context passed to MethodCall and returned upon reply.
     */
    typedef void (MessageReceiver::* ReplyHandler)(Message& message, void* context);

    /**
     * SignalHandlers are %MessageReceiver methods which are called by AllJoyn library
     * to forward AllJoyn received signals to AllJoyn library users.
     *
     * @param member    Method or signal interface member entry.
     * @param srcPath   Object path of signal emitter.
     * @param message   The received message.
     */
    typedef void (MessageReceiver::* SignalHandler)(const InterfaceDescription::Member* member, const char* srcPath, Message& message);

};

}

#endif
