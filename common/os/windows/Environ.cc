/**
 * @file
 *
 * This file implements methods from the Environ class.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, 2014 AllSeen Alliance. All rights reserved.
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

#include <windows.h>
#include <map>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Logger.h>
#include <qcc/CommonGlobals.h>

#include <Status.h>

#define QCC_MODULE "ENVIRON"

using namespace std;

namespace qcc {

Environ* Environ::GetAppEnviron(void)
{
    return &commonGlobals.environSingleton;
}

qcc::String Environ::Find(const qcc::String& key, const char* defaultValue)
{
    qcc::String val;

    lock.Lock();
    if (vars.count(key) == 0) {
        char c;
        char* val = &c;
        DWORD len = GetEnvironmentVariableA(key.c_str(), val, 0);
        if (len) {
            val = new char[len];
            GetEnvironmentVariableA(key.c_str(), val, len);
            vars[key] = val;
            delete [] val;
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
    LPWCH env = GetEnvironmentStringsW();
    LPWSTR var = env ? reinterpret_cast<LPWSTR>(env) + 1 : NULL;
    if (var == NULL) {
        Log(LOG_ERR, "Environ::Preload unable to read Environment Strings");
        lock.Unlock();
        return;
    }
    while (*var != NULL) {
        size_t len = wcslen((const wchar_t*)var);
        char* ansiBuf = (char*)malloc(len + 1);
        if (NULL == ansiBuf) {
            break;
        }
        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)var, (int)len, ansiBuf, (int)len, NULL, NULL);
        ansiBuf[len] = '\0';
        if (strncmp(ansiBuf, keyPrefix, prefixLen) == 0) {
            size_t nameLen = prefixLen;
            while (ansiBuf[nameLen] != '=') {
                ++nameLen;
            }
            qcc::String key(ansiBuf, nameLen);
            Find(key, NULL);
        }
        free(ansiBuf);
        var += len + 1;
    }
    if (env) {
        FreeEnvironmentStringsW(env);
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
            size_t endPos = line.find('#');
            if (qcc::String::npos != endPos) {
                line = line.substr(0, endPos);
            }
            size_t eqPos = line.find('=');
            if (qcc::String::npos != eqPos) {
                vars[Trim(line.substr(0, eqPos))] = Trim(line.substr(eqPos + 1));
            }
        }
    }
    lock.Unlock();
    return (ER_EOF == status) ? ER_OK : status;
}

}   /* namespace */
