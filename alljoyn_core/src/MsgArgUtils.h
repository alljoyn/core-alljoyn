#ifndef _ALLJOYN_MSGARGUTILS_H
#define _ALLJOYN_MSGARGUTILS_H
/**
 * @file
 * This file defines a set of utitilies for message bus data types and values
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
