/**
 * @file
 *
 * Log control.
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
#ifndef _QCC_LOG_H
#define _QCC_LOG_H

#include <qcc/platform.h>

/**
 * Set AllJoyn debug levels.
 *
 * @param module    name of the module to generate debug output
 * @param level     debug level to set for the module
 */
void AJ_CALL QCC_SetDebugLevel(const char* module, uint32_t level);

/**
 * Set AllJoyn logging levels.
 *
 * @param logEnv    A semicolon separated list of KEY=VALUE entries used
 *                  to set the log levels for internal AllJoyn modules.
 *                  (i.e. ALLJOYN=7;ALL=1)
 */
void AJ_CALL QCC_SetLogLevels(const char* logEnv);

/**
 * Indicate whether AllJoyn logging goes to OS logger or stdout
 *
 * @param  useOSLog   true iff OS specific logging should be used rather than print for AllJoyn debug messages.
 */
void AJ_CALL QCC_UseOSLogging(bool useOSLog);


#endif