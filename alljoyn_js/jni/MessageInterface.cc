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
#include "MessageInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _MessageInterface::constants;

std::map<qcc::String, int32_t>& _MessageInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("ALLJOYN_FLAG_NO_REPLY_EXPECTED", 0x01);
        CONSTANT("ALLJOYN_FLAG_AUTO_START",        0x02);
        CONSTANT("ALLJOYN_FLAG_ALLOW_REMOTE_MSG",  0x04);
        CONSTANT("ALLJOYN_FLAG_SESSIONLESS",       0x10);
        CONSTANT("ALLJOYN_FLAG_GLOBAL_BROADCAST",  0x20);
        CONSTANT("ALLJOYN_FLAG_COMPRESSED",        0x40);
        CONSTANT("ALLJOYN_FLAG_ENCRYPTED",         0x80);
    }
    return constants;
}

_MessageInterface::_MessageInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_MessageInterface::~_MessageInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}