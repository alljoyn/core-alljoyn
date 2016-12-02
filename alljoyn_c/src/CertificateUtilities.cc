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

#include "CertificateUtilities.h"

#define QCC_MODULE "ALLJOYN_C"

using namespace qcc;

/*
 * The "certs" variable must be freed by the caller using "delete[]".
 */
QStatus ExtractCertificates(AJ_PCSTR identCertChain, size_t* certCount, CertificateX509** certs)
{
    QStatus status = CertificateHelper::GetCertCount(identCertChain, certCount);
    *certs = nullptr;

    if ((ER_OK == status) &&
        (*certCount == 0)) {
        status = ER_INVALID_DATA;
    }

    if (ER_OK == status) {
        *certs = new CertificateX509[*certCount];
        status = CertificateX509::DecodeCertChainPEM(identCertChain, *certs, *certCount);
    }

    if (ER_OK != status) {
        delete[] *certs;
        *certs = nullptr;
    }

    return status;
}

QStatus GetGroupId(const uint8_t* groupId, size_t groupSize, GUID128& extractedId)
{
    if (GUID128::SIZE != groupSize) {
        return ER_INVALID_GUID;
    } else {
        extractedId.SetBytes(groupId);
    }

    return ER_OK;
}