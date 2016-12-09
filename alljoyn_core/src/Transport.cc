/**
 * @file
 * Transport is a base class for all Message Bus Transport implementations.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Debug.h>
#include "Transport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

QStatus Transport::ParseArguments(const char* transportName,
                                  const char* args,
                                  map<String, String>& argMap)
{
    String tpNameStr(transportName);
    tpNameStr.push_back(':');
    String argStr(args);

    /* Skip to the first param */
    size_t pos = argStr.find(tpNameStr);

    if (String::npos == pos) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    } else {
        pos += tpNameStr.size();
    }

    size_t endPos = 0;
    while (String::npos != endPos) {
        size_t eqPos = argStr.find_first_of('=', pos);
        endPos = (eqPos == String::npos) ? String::npos : argStr.find_first_of(",;", eqPos);
        if (String::npos != eqPos) {
            String keyStr = argStr.substr(pos, eqPos - pos);
            String valStr;
            if (String::npos == endPos) {
                if ((eqPos + 1) < argStr.size()) {
                    valStr = argStr.substr(eqPos + 1);
                }
            } else {
                if (endPos > (eqPos + 1)) {
                    valStr = argStr.substr(eqPos + 1, endPos - eqPos - 1);
                }
            }
            if (argMap.count(keyStr) > 0) {
                QCC_LogError(ER_WARNING, ("Transport::ParseArguments(): argMap[%s] already exists, changing old value '%s' to new value '%s'",
                                          keyStr.c_str(), argMap[keyStr].c_str(), valStr.c_str()));
            }
            argMap[keyStr] = valStr;
        }
        pos = endPos + 1;
    }
    return ER_OK;
}

}