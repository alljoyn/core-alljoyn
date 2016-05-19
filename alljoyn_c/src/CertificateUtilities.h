/**
 * @file
 * This contains helper utilities for the C binding
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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