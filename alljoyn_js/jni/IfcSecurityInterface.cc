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
#include "IfcSecurityInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _IfcSecurityInterface::constants;

std::map<qcc::String, int32_t>& _IfcSecurityInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("INHERIT",  0x00);     /**< Inherit the security of the object that implements the interface */
        CONSTANT("REQUIRED", 0x01);     /**< Security is required for an interface */
        CONSTANT("OFF",      0x02);     /**< Security does not apply to this interface */
    }
    return constants;
}

_IfcSecurityInterface::_IfcSecurityInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_IfcSecurityInterface::~_IfcSecurityInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}
