#ifndef _ALLJOYN_MSGARGUTILS_H
#define _ALLJOYN_MSGARGUTILS_H
/**
 * @file
 * This file defines a set of utitilies for message bus data types and values
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

#ifndef __cplusplus
#error Only include MsgArgUtils.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <stdarg.h>
#include <alljoyn/Status.h>

namespace ajn {

class MsgArgUtils {

  public:

    /**
     * Set an array of MsgArgs by applying the Set() method to each MsgArg in turn.
     *
     * @param args     An array of MsgArgs to set.
     * @param numArgs  [in,out] On input the size of the args array. On output the number of MsgArgs
     *                 that were set. There must be at least enought MsgArgs to completely
     *                 initialize the signature.
     *                 there should at least enough.
     * @param signature   The signature for MsgArg values
     * @param argp        A va_list of one or more values to initialize the MsgArg list.
     *
     * @return - ER_OK if the MsgArgs were successfully set.
     *         - ER_BUS_TRUNCATED if the signature was longer than expected.
     *         - Otherwise an error status.
     */
    static QStatus SetV(MsgArg* args, size_t& numArgs, const char* signature, va_list* argp);

};

}

#endif