/**
 * @file
 * This file defines the alljoyn_factoryresetlistener and related functions that provides
 * the interface between authentication mechansisms and applications.
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
#ifndef _ALLJOYN_C_FACTORYRESETLISTENER_H
#define _ALLJOYN_C_FACTORYRESETLISTENER_H

#include <alljoyn_c/AjAPI.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * alljoyn_factoryresetlistener allows authentication mechanisms to interact with the user or application.
 */
typedef struct _alljoyn_factoryresetlistener_handle* alljoyn_factoryresetlistener;

/**
 * Type for the FactoryReset callback.
 *
 * Framework requests application do a factory reset of its state.
 *
 * @param context  The context pointer passed into the alljoyn_factoryresetlistener_create function.
 */
typedef QStatus (AJ_CALL * alljoyn_factoryresetlistener_factoryreset_ptr)(const void* context);

/**
 * Structure used during alljoyn_factoryresetlistener_create to provide callbacks into C.
 */
typedef struct {
    /**
     * Framework requests application factory reset.
     */
    alljoyn_factoryresetlistener_factoryreset_ptr factory_reset;
} alljoyn_factoryresetlistener_callbacks;

/**
 * Create an alljoyn_factoryresetlistener which will trigger the provided callbacks, passing along the provided context.
 *
 * @param callbacks Callbacks to trigger for associated events.
 * @param context   Context to pass to callback functions.
 *
 * @return Handle to newly allocated alljoyn_factoryresetlistener.
 */
extern AJ_API alljoyn_factoryresetlistener AJ_CALL alljoyn_factoryresetlistener_create(const alljoyn_factoryresetlistener_callbacks* callbacks, const void* context);

/**
 * Destroy an alljoyn_factoryresetlistener.
 *
 * @param listener alljoyn_factoryresetlistener to destroy.
 */
extern AJ_API void AJ_CALL alljoyn_factoryresetlistener_destroy(alljoyn_factoryresetlistener listener);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
