/**
 * @file
 *
 * This file implements methods from the Environ class.
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

#include <qcc/platform.h>

#include <map>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

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

static uint64_t _environSingleton[RequiredArrayLength(sizeof(Environ), uint64_t)];
static Environ& environSingleton = (Environ&)_environSingleton;
static bool initialized = false;

void Environ::Init()
{
    if (!initialized) {
        new (&environSingleton)Environ();
        initialized = true;
    }
}

void Environ::Shutdown()
{
    if (initialized) {
        environSingleton.~Environ();
        initialized = false;
    }
}

Environ* Environ::GetAppEnviron(void)
{
    return &environSingleton;
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