/**
 * @file
 * Transport is a base class for all Message Bus Transport implementations.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#include <qcc/String.h>
#include "Transport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

QStatus Transport::ParseArguments(const char* transportName,
                                  const char* args,
                                  map<qcc::String, qcc::String>& argMap)
{
    qcc::String tpNameStr(transportName);
    tpNameStr.push_back(':');
    qcc::String argStr(args);

    /* Skip to the first param */
    size_t pos = argStr.find(tpNameStr);

    if (qcc::String::npos == pos) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    } else {
        pos += tpNameStr.size();
    }

    size_t endPos = 0;
    while (qcc::String::npos != endPos) {
        size_t eqPos = argStr.find_first_of('=', pos);
        endPos = (eqPos == qcc::String::npos) ? qcc::String::npos : argStr.find_first_of(",;", eqPos);
        if (qcc::String::npos != eqPos) {
            argMap[argStr.substr(pos, eqPos - pos)] = (qcc::String::npos == endPos) ? argStr.substr(eqPos + 1) : argStr.substr(eqPos + 1, endPos - eqPos - 1);
        }
        pos = endPos + 1;
    }
    return ER_OK;
}

}
