/**
 * @file
 * This contains helper utilities for the C binding
 */
/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
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