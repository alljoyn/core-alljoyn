/**
 * @file
 * ProximityScanner is the platform agnostic header file that the ProximityScanner includes in
 * its code
 * It contains the platform specific header files which are then included in the source
 * depending on the platform
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_PROXIMITYSCANNER_H
#define _ALLJOYN_PROXIMITYSCANNER_H

#include <qcc/platform.h>

#if defined(QCC_OS_GROUP_POSIX) || (QCC_OS_GROUP_ANDROID)
#include <posix/ProximityScanner.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <windows/ProximityScanner.h>
#elif defined(QCC_OS_GROUP_WINRT)
#include <winrt/ProximityScanner.h>
#else
#error No OS GROUP defined.
#endif

#endif
