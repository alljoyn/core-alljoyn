/**
 * @file
 *
 * This file implements the BusAttachmentC class.
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