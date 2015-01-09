/**
 * @file
 *
 * This file implements methods from the Environ class.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <map>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/CommonGlobals.h>

#define QCC_MODULE "ENVIRON"

using namespace std;
using namespace qcc;

#if (defined(QCC_OS_DARWIN) && !defined(QCC_OS_IPHONE) && !defined(QCC_OS_IPHONE_SIMULATOR)) || (defined(QCC_OS_DARWIN) && defined(QCC_OS_IPHONE_SIMULATOR))
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char** environ;   // For Linux, this is all that's needed to access
                         // environment variables.
#endif

Environ* Environ::GetAppEnviron(void)
{
    return &commonGlobals.environSingleton;
}

qcc::String Environ::Find(const qcc::String& key, const char* defaultValue)
{
    qcc::String val;
    lock.Lock();
    if (vars.count(key) == 0) {
        char* val = getenv(key.c_str());
        if (val) {
            vars[key] = val;
        }
    }
    val = vars[key];
    if (val.empty() && defaultValue) {
        val = defaultValue;
    }
    lock.Unlock();
    return val;
}

void Environ::Preload(const char* keyPrefix)
{
    size_t prefixLen = strlen(keyPrefix);
    lock.Lock();
    for (char** var = environ; *var != NULL; ++var) {
        char* p = *var;
        if (strncmp(p, keyPrefix, prefixLen) == 0) {
            size_t nameLen = prefixLen;
            while (p[nameLen] != '=') {
                ++nameLen;
            }
            qcc::String key(p, nameLen);
            Find(key, NULL);
        }
    }
    lock.Unlock();
}

void Environ::Add(const qcc::String& key, const qcc::String& value)
{
    lock.Lock();
    vars[key] = value;
    lock.Unlock();
}


QStatus Environ::Parse(Source& source)
{
    QStatus status = ER_OK;
    lock.Lock();
    while (ER_OK == status) {
        qcc::String line;
        status = source.GetLine(line);
        if (ER_OK == status) {
            size_t endPos = line.find_first_of('#');
            if (qcc::String::npos != endPos) {
                line = line.substr(0, endPos);
            }
            size_t eqPos = line.find_first_of('=');
            if (qcc::String::npos != eqPos) {
                qcc::String key = Trim(line.substr(0, eqPos));
                qcc::String val = Trim(line.substr(eqPos + 1));
                vars[key] = val;
                setenv(key.c_str(), val.c_str(), 1);
            }
        }
    }
    lock.Unlock();
    return (ER_EOF == status) ? ER_OK : status;
}
