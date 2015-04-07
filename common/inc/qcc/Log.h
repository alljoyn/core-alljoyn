/**
 * @file
 *
 * Log control.
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
