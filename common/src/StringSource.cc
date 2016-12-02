/**
 * @file
 *
 * Source wrapper for std::string
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <qcc/platform.h>

#include <cstring>

#include <qcc/Stream.h>
#include <qcc/StringSource.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "STREAM"

QStatus StringSource::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    QCC_UNUSED(timeout);

    QStatus status = ER_OK;
    actualBytes = (std::min)(reqBytes, str.size() - outIdx);
    if (0 < actualBytes) {
        memcpy(buf, str.data() + outIdx, actualBytes);
        outIdx += actualBytes;
    } else if (outIdx == str.size()) {
        status = ER_EOF;
    }
    return status;
}

