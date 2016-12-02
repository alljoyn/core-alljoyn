/**
 * @file
 * This file provides access to AllJoyn library version and build information.
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
#ifndef _ALLJOYN_C_VERSION_H
#define _ALLJOYN_C_VERSION_H

#include <alljoyn_c/AjAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

extern AJ_API const char* AJ_CALL alljoyn_getversion();        /**< Gives the version of AllJoyn Library */
extern AJ_API const char* AJ_CALL alljoyn_getbuildinfo();      /**< Gives build information of AllJoyn Library */
extern AJ_API uint32_t AJ_CALL alljoyn_getnumericversion();    /**< Gives the version of AllJoyn Library as a single number */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif