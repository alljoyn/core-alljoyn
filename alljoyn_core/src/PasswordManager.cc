/**
 * @file
 * This file implements the PasswordManager class that provides the interface to
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include "PasswordManager.h"

using namespace qcc;

namespace ajn {

String* PasswordManager::authMechanism = NULL;
String* PasswordManager::password = NULL;

void PasswordManager::Init()
{
    authMechanism = new String("ANONYMOUS");
    password = new String();
}

void PasswordManager::Shutdown()
{
    delete authMechanism;
    authMechanism = NULL;
    delete password;
    password = NULL;
}

}