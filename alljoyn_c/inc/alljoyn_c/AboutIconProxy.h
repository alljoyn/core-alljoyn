/**
 * @file
 * alljoyn_about_iconproxy implements a proxy bus object used to interact with
 * a remote org.alljoyn.Icon interface
 */
/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_ABOUTICONPROXY_C_H
#define _ALLJOYN_ABOUTICONPROXY_C_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/AboutIcon.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/Session.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _alljoyn_about_iconproxy_handle* alljoyn_about_iconproxy;

/**
 * Allocate a new alljoyn_about_iconproxy object.
 *
 * @param bus reference to bus attachment object
 * @param[in] busName Unique or well-known name of an AllJoyn bus you have joined
 * @param[in] sessionId the session received after joining an AllJoyn session
 *
 * @return The allocated alljoyn_about_iconproxy.
 */
extern AJ_API alljoyn_about_iconproxy AJ_CALL alljoyn_about_iconproxy_create(alljoyn_busattachment bus,
                                                                             const char* busName,
                                                                             alljoyn_sessionid sessionId);

/**
 * Free an alljoyn_about_iconproxy object.
 *
 * @param proxy The alljoyn_about_iconproxy to be freed.
 */
extern AJ_API void AJ_CALL alljoyn_about_iconproxy_destroy(alljoyn_about_iconproxy proxy);

/**
 * This method makes multiple proxy bus object method calls to fill in the
 * content of the alljoyn_about_icon. Its possible for any of the method calls
 * to fail causing this member function to return an error status.
 *
 * @param[in]  proxy alljoyn_about_iconproxy object
 * @param[out] icon  alljoyn_about_icon that holds icon content
 * @return
 *  - #ER_OK if successful
 *  - an error status indicating failure to get the icon content
 */
extern AJ_API QStatus AJ_CALL alljoyn_about_iconproxy_geticon(alljoyn_about_iconproxy proxy,
                                                              alljoyn_about_icon* icon);

/**
 * @param[in]  proxy alljoyn_about_iconproxy object
 * @param[out] the version of the remote AboutIcon BusObject
 *
 * @return
 * - #ER_OK if successful
 * - an error status indicating failure
 */
extern AJ_API QStatus AJ_CALL alljoyn_about_iconproxy_getversion(alljoyn_about_iconproxy proxy,
                                                                 uint16_t* version);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ALLJOYN_ABOUTICONPROXY_C_H */
