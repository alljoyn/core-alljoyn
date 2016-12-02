/**
 * @file
 * Abstract interface implemented by objects that wish to consume Message Bus
 * messages.
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
#ifndef _ALLJOYN_MESSAGESINK_H
#define _ALLJOYN_MESSAGESINK_H

#ifndef __cplusplus
#error Only include MessageSink.h in C++ code.
#endif

#include <qcc/platform.h>

#include <alljoyn/Message.h>

#include <alljoyn/Status.h>

namespace ajn {

/**
 * Abstract interface implemented by objects that wish to consume Message Bus
 * messages.
 */
class MessageSink {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~MessageSink() { }

    /**
     * Consume a message bus message.
     *
     * @param msg   Message to be consumed.
     * @return #ER_OK if successful.
     */
    virtual QStatus PushMessage(Message& msg) = 0;
};

}

#endif