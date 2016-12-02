/**
 * @file
 * BusAttachment is the top-level object responsible for connecting to and optionally managing a message bus.
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
#include <alljoyn_c/PasswordManager.h>
#include <alljoyn/PasswordManager.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

QStatus AJ_CALL alljoyn_passwordmanager_setcredentials(const char* authMechanism, const char* password)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ajn::PasswordManager::SetCredentials(authMechanism, password);
}