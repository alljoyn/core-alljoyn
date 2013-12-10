/**
 * @file
 * This file redefines __dllexport or __dllimport on relevant platforms
 *
 * This file also defines the deferred callback mechanism used to make sure the
 * callbacks occur on the same thread that registered for the callback.
 */
/******************************************************************************
 * Copyright (c) 2009-2013, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_C_AJAPI_H
#define _ALLJOYN_C_AJAPI_H

#include <qcc/platform.h>

/** This @#define allows for redefinition to __dllexport or __dllimport on relevant platforms */
#ifndef AJ_API
#  if defined(QCC_OS_GROUP_WINDOWS)
#    define AJ_API __declspec(dllexport)
#  elif defined(QCC_OS_GROUP_POSIX)
#    define AJ_API __attribute__((visibility("default")))
#  else
#    define AJ_API
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unity-specific function to process alternate-thread callbacks on the main thread.
 *
 * @return the number of callbacks processed.
 */
extern AJ_API int alljoyn_unity_deferred_callbacks_process();

/**
 * Enable/disable main thread callback behavior.
 */
extern AJ_API void alljoyn_unity_set_deferred_callback_mainthread_only(QCC_BOOL mainthread_only);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
