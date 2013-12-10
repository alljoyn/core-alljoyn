/**
 * @file This file defines the class for handling the client and server
 * endpoints for the message bus wire protocol
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/GUID.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#include <BusEndpoint.h>

#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace ajn;

String _BusEndpoint::GetControllerUniqueName() const {

    /* An endpoint with unique name :X.Y has a controller with a unique name :X.1 */
    String ret = GetUniqueName();
    ret[qcc::GUID128::SHORT_SIZE + 2] = '1';
    ret.resize(qcc::GUID128::SHORT_SIZE + 3);
    return ret;
}

void _BusEndpoint::Invalidate()
{
    QCC_DbgPrintf(("Invalidating endpoint type=%d %s", endpointType, GetUniqueName().c_str()));
    isValid = false;
}

