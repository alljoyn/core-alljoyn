/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#include "CredentialsInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _CredentialsInterface::constants;

std::map<qcc::String, int32_t>& _CredentialsInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("PASSWORD",          0x0001);
        CONSTANT("USER_NAME",         0x0002);
        CONSTANT("CERT_CHAIN",        0x0004);
        CONSTANT("PRIVATE_KEY",       0x0008);
        CONSTANT("LOGON_ENTRY",       0x0010);
        CONSTANT("EXPIRATION",        0x0020);
        CONSTANT("NEW_PASSWORD",      0x1001);
        CONSTANT("ONE_TIME_PWD",      0x2001);
    }
    return constants;
}

_CredentialsInterface::_CredentialsInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_CredentialsInterface::~_CredentialsInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}