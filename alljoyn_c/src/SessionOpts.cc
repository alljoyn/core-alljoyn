/**
 * @file
 * Class for encapsulating Session option information.
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

#include <alljoyn/MsgArg.h>
#include <alljoyn/Session.h>
#include <alljoyn_c/MsgArg.h>
#include <alljoyn_c/Session.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_sessionopts_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_sessionopts AJ_CALL alljoyn_sessionopts_create(uint8_t traffic, QCC_BOOL isMultipoint,
                                                       uint8_t proximity, alljoyn_transportmask transports)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_sessionopts) new ajn::SessionOpts((ajn::SessionOpts::TrafficType)traffic, isMultipoint == QCC_TRUE ? true : false,
                                                      (ajn::SessionOpts::Proximity)proximity, (ajn::TransportMask)transports);
}

void AJ_CALL alljoyn_sessionopts_destroy(alljoyn_sessionopts opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::SessionOpts*)opts;
}

uint8_t AJ_CALL alljoyn_sessionopts_get_traffic(const alljoyn_sessionopts opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((const ajn::SessionOpts*)opts)->traffic;
}

void AJ_CALL alljoyn_sessionopts_set_traffic(alljoyn_sessionopts opts, uint8_t traffic)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::SessionOpts*)opts)->traffic = (ajn::SessionOpts::TrafficType)traffic;
}

QCC_BOOL AJ_CALL alljoyn_sessionopts_get_multipoint(const alljoyn_sessionopts opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((const ajn::SessionOpts*)opts)->isMultipoint ? QCC_TRUE : QCC_FALSE);
}

void AJ_CALL alljoyn_sessionopts_set_multipoint(alljoyn_sessionopts opts, QCC_BOOL isMultipoint)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    (isMultipoint == QCC_TRUE) ? ((ajn::SessionOpts*)opts)->isMultipoint = true :
                                                                           ((ajn::SessionOpts*)opts)->isMultipoint = false;
}

uint8_t AJ_CALL alljoyn_sessionopts_get_proximity(const alljoyn_sessionopts opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((const ajn::SessionOpts*)opts)->proximity;
}

void AJ_CALL alljoyn_sessionopts_set_proximity(alljoyn_sessionopts opts, uint8_t proximity)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::SessionOpts*)opts)->proximity = (ajn::SessionOpts::Proximity)proximity;
}

alljoyn_transportmask AJ_CALL alljoyn_sessionopts_get_transports(const alljoyn_sessionopts opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((const ajn::SessionOpts*)opts)->transports;
}

void AJ_CALL alljoyn_sessionopts_set_transports(alljoyn_sessionopts opts, alljoyn_transportmask transports)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::SessionOpts*)opts)->transports = (ajn::TransportMask)transports;
}

QCC_BOOL AJ_CALL alljoyn_sessionopts_iscompatible(const alljoyn_sessionopts one, const alljoyn_sessionopts other)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((const ajn::SessionOpts*)one)->IsCompatible(*((const ajn::SessionOpts*)other)) == true ? QCC_TRUE : QCC_FALSE);
}

int32_t AJ_CALL alljoyn_sessionopts_cmp(const alljoyn_sessionopts one, const alljoyn_sessionopts other)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const ajn::SessionOpts& _one = *((const ajn::SessionOpts*)one);
    const ajn::SessionOpts& _other = *((const ajn::SessionOpts*)other);

    return (_one == _other ? 0 : (_other < _one ? 1 : -1));
}