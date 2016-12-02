/**
 * @file
 * This contains helper utilities for the C binding
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

#ifndef _ALLJOYN_C_UTILITIES_H
#define _ALLJOYN_C_UTILITIES_H

#include <qcc/GUID.h>
#include <qcc/CertificateECC.h>
#include <qcc/CertificateHelper.h>

/*
 * The "certs" variable must be freed by the caller using "delete[]".
 */
QStatus ExtractCertificates(AJ_PCSTR identCertChain, size_t* certCount, qcc::CertificateX509** certs);

/* This helper sets the GUID128 object equal to the raw bytes in the groupId array. */
QStatus GetGroupId(const uint8_t* groupId, size_t groupSize, qcc::GUID128& extractedId);

#endif