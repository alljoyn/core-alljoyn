/**
 * @file
 * alljoyn_about_iconobj is an Alljoyn BusObject that implements the org.alljoyn.Icon interface
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

#ifndef _ALLJOYN_ABOUTICONOBJ_C_H
#define _ALLJOYN_ABOUTICONOBJ_C_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/MsgArg.h>
#include <alljoyn_c/AboutIcon.h>
#include <alljoyn_c/BusAttachment.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _alljoyn_about_iconobj_handle* alljoyn_about_iconobj;

/**
 * Allocate a new alljoyn_about_iconobj object.
 *
 * @param bus  the bus
 * @param icon the icon content
 *
 * @return The allocated alljoyn_about_iconobj.
 */
extern AJ_API alljoyn_about_iconobj AJ_CALL alljoyn_about_iconobj_create(alljoyn_busattachment bus,
                                                                         alljoyn_about_icon icon);

/**
 * Free an alljoyn_about_iconobj object.
 *
 * @param data the alljoyn_about_iconobj to be freed.
 */
extern AJ_API void AJ_CALL alljoyn_about_iconobj_destroy(alljoyn_about_iconobj icon);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ALLJOYN_ABOUTICONOBJ_C_H */
