/**
 * @file
 * This file defines the alljoyn_permissionconfigurator and related functions
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

#ifndef _ALLJOYN_C_PERMISSIONCONFIGURATOR_H
#define _ALLJOYN_C_PERMISSIONCONFIGURATOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * alljoyn_permissionconfigurator is a handle which exposes Security 2.0 permission
 * management capabilities to the application.
 */
typedef struct _alljoyn_permissionconfigurator_handle* alljoyn_permissionconfigurator;

typedef enum {
    NOT_CLAIMABLE,      /**< The application is not claimed and not accepting claim requests. */
    CLAIMABLE,          /**< The application is not claimed and is accepting claim requests. */
    CLAIMED,            /**< The application is claimed and can be configured. */
    NEED_UPDATE         /**< The application is claimed, but requires a configuration update (after a software upgrade). */
} alljoyn_applicationstate;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif