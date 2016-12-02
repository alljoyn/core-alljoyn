/**
 * @file
 * This file provides access to Alljoyn library version and build information.
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

#include <alljoyn/version.h>
#include <alljoyn_c/version.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

const char* AJ_CALL alljoyn_getversion()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ajn::GetVersion();
}

const char* AJ_CALL alljoyn_getbuildinfo()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ajn::GetBuildInfo();
}

uint32_t AJ_CALL alljoyn_getnumericversion()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ajn::GetNumericVersion();
}