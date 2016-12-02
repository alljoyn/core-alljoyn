#ifndef _ALLJOYN_SESSIONINTERNAL_H
#define _ALLJOYN_SESSIONINTERNAL_H
/**
 * @file
 * AllJoyn session related data types.
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

#include <qcc/platform.h>
#include <alljoyn/Session.h>

/** DBus signature of SessionOpts structure */
#define SESSIONOPTS_SIG "a{sv}"

namespace ajn {

/**
 * Parse a MsgArg into a SessionOpts
 * @param       msgArg   MsgArg to be parsed.
 * @param[OUT]  opts     SessionOpts output.
 * @return  ER_OK if successful.
 */
QStatus GetSessionOpts(const MsgArg& msgArg, SessionOpts& opts);

/**
 * Write a SessionOpts into a MsgArg
 *
 * @param      opts      SessionOpts to be written to MsgArg.
 * @param[OUT] msgArg    MsgArg output.
 */
void SetSessionOpts(const SessionOpts& opts, MsgArg& msgArg);

}

#endif