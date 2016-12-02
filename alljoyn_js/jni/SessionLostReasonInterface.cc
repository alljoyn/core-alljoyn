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
#include "SessionLostReasonInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _SessionLostReasonInterface::constants;

std::map<qcc::String, int32_t>& _SessionLostReasonInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("INVALID",                     0x00); /**< Invalid */
        CONSTANT("REMOTE_END_LEFT_SESSION",     0x01); /**< Remote end called LeaveSession */
        CONSTANT("REMOTE_END_CLOSED_ABRUPTLY",  0x02); /**< Remote end closed abruptly */
        CONSTANT("REMOVED_BY_BINDER",           0x03); /**< Session binder removed this endpoint by calling RemoveSessionMember */
        CONSTANT("LINK_TIMEOUT",                0x04); /**< Link was timed-out */
        CONSTANT("REASON_OTHER",                0x05); /**< Unspecified reason for session loss */
    }
    return constants;
}

_SessionLostReasonInterface::_SessionLostReasonInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_SessionLostReasonInterface::~_SessionLostReasonInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}
