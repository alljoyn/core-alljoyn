/**
 * @file
 *
 * This file implements methods from the Environ class.
 */

/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include <qcc/platform.h>

#include <windows.h>
#include <map>

#include <qcc/winrt/utility.h>
#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Logger.h>

#include <Status.h>

#define QCC_MODULE "ENVIRON"

using namespace std;

namespace qcc {

Environ* Environ::GetAppEnviron(void)
{
    static Environ* env = NULL;      // Environment variable singleton.
    if (env == NULL) {
        env = new Environ();
    }
    return env;
}

qcc::String Environ::Find(const qcc::String& key, const char* defaultValue)
{
    qcc::String val;
    lock.Lock();
    val = vars[key];
    if (val.empty()) {
        if (strcmp(key.c_str(), "APPLICATIONDATA") == 0) {
            Platform::String ^ documentFolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
            qcc::String strDocumentFolder = PlatformToMultibyteString(documentFolder);
            if (!strDocumentFolder.empty()) {
                Add(key.c_str(), strDocumentFolder.c_str());
                val = vars[key];
            }
        }
    }
    if (val.empty() && defaultValue) {
        val = defaultValue;
    }
    lock.Unlock();
    return val;
}

void Environ::Preload(const char* keyPrefix)
{
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
    return (ER_NONE == status) ? ER_OK : status;
}

}   /* namespace */
