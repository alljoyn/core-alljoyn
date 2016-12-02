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
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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