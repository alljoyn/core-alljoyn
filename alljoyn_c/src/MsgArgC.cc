/**
 * @file
 *
 * This file implements the BusAttachmentC class.
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
#include "MsgArgC.h"
#include <alljoyn_c/Status.h>
#include <alljoyn/MsgArg.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

namespace ajn {
QStatus MsgArgC::MsgArgUtilsSetVC(MsgArg* args, size_t& numArgs, const char* signature, va_list* argp)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return MsgArgUtils::SetV(args, numArgs, signature, argp);
}

QStatus MsgArgC::VBuildArgsC(const char*& signature, size_t sigLen, MsgArg* arg, size_t maxArgs, va_list* argp, size_t* count)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return VBuildArgs(signature, sigLen, arg, maxArgs, argp, count);
}

QStatus MsgArgC::VParseArgsC(const char*& signature, size_t sigLen, const MsgArg* argList, size_t numArgs, va_list* argp)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return VParseArgs(signature, sigLen, argList, numArgs, argp);
}
}
