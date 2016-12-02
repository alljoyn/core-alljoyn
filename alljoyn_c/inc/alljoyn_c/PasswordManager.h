/**
 * @file PasswordManager.h is the top-level object that provides the interface to
 * set credentials used for the authentication of thin clients.
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
#ifndef _ALLJOYN_C_PASSWORDMANAGER_H
#define _ALLJOYN_C_PASSWORDMANAGER_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/Status.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Set credentials used for the authentication of thin clients.
 *
 * @warning Before invoking Connect() to BusAttachment, the application should call SetCredentials
 * if it expects to be able to communicate to/from thin clients.
 * The bundled router will start advertising the name as soon as it is started and MUST have
 * the credentials set to be able to authenticate any thin clients that may try to use the
 * bundled router to communicate with the app.
 * @param authMechanism  The name of the authentication mechanism issuing the request.
 * @param password       The password.
 *
 * @return
 *      - #ER_OK  if the credentials was successfully set.
 */
extern AJ_API QStatus AJ_CALL alljoyn_passwordmanager_setcredentials(const char* authMechanism, const char* password);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
