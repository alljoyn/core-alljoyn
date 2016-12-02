/**
 * @file
 * alljoyn_abouticonobj represents a bus object that implements the org.alljoyn.Icon interface.
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

#ifndef _ALLJOYN_ABOUTICONOBJ_C_H
#define _ALLJOYN_ABOUTICONOBJ_C_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/MsgArg.h>
#include <alljoyn_c/AboutIcon.h>
#include <alljoyn_c/BusAttachment.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * alljoyn_abouticonobj is an AllJoyn BusObject that implements the
 * org.alljoyn.Icon standard interface. Applications that provide AllJoyn IoE
 * services to receive info about the Icon of the service.
 */
typedef struct _alljoyn_abouticonobj_handle* alljoyn_abouticonobj;

/**
 * Allocate a new alljoyn_abouticonobj object.
 *
 * @param bus  the bus
 * @param icon the icon content
 *
 * @return The allocated alljoyn_abouticonobj.
 */
extern AJ_API alljoyn_abouticonobj AJ_CALL alljoyn_abouticonobj_create(alljoyn_busattachment bus,
                                                                       alljoyn_abouticon icon);

/**
 * Free an alljoyn_abouticonobj object.
 *
 * @param icon the alljoyn_abouticonobj to be freed.
 */
extern AJ_API void AJ_CALL alljoyn_abouticonobj_destroy(alljoyn_abouticonobj icon);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ALLJOYN_ABOUTICONOBJ_C_H */