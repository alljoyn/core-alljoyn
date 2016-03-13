/**
 * @file
 *
 * "STL-like" version of string.
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
#include <qcc/platform.h>
#include <qcc/atomic.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <assert.h>
#include <algorithm>
#include <limits>
#include <new>

#define QCC_MODULE "STRING"

namespace qcc {

/* Global Data */

static uint64_t _emptyString[RequiredArrayLength(sizeof(String), uint64_t)];
static bool initialized = false;

String& emptyString = (String&)_emptyString;

const String& String::Empty = (String&)emptyString;

const std::string::size_type String::npos = std::string::npos;

void String::Init()
{
    if (!initialized) {
        new (&emptyString)String();
        initialized = true;
    }
}

void String::Shutdown()
{
    if (initialized) {
        emptyString.~String();
        initialized = false;
    }
}



String::String(const char* str, std::string::size_type strLen, size_t sizeHint)
{
    if (str) {
        s.reserve(sizeHint);
        s.assign(str, strLen);
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
}

String::String(const char* str, std::string::size_type strLen)
{
    if (str) {
        s.assign(str, strLen);
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
}

String::String(const char* str)
{
    if (str) {
        s = str;
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
}


std::string String::assign(const char* str, std::string::size_type len)
{
    if (str) {
        if (len == 0) {
            QCC_LogError(ER_WARNING, ("Passing len = 0 will not copy entire contents of str in the future!"));
            assert(len != 0);  // Assert fail in debug mode
            if (str) {
                s.assign(str);
            } else {
                s = "";
            }
        } else {
            s.assign(str, len);
        }
    } else {
        QCC_LogError(ER_WARNING, ("Assigning string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
        s = "";
    }
    return s;
}

std::string String::assign(const char* str)
{
    if (str) {
        s.assign(str);
    } else {
        QCC_LogError(ER_WARNING, ("Assigning string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
        s = "";
    }
    return s;
}

std::string::size_type String::secure_clear()
{
    char* d = const_cast<char*>(s.data());
    ClearMemory(d, s.size());
    return 1;
}

std::string String::append(const char* str, std::string::size_type len)
{
    if (str) {
        s.append(str, len);
    } else {
        QCC_LogError(ER_WARNING, ("Appending string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
    return s;
}

std::string String::append(const char* str)
{
    if (str) {
        s.append(str);
    } else {
        QCC_LogError(ER_WARNING, ("Appending string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
    return s;
}

std::string String::insert(std::string::size_type pos, const char* str, std::string::size_type len)
{
    if (str) {
        s.insert(pos, str, len);
    } else {
        QCC_LogError(ER_WARNING, ("Appending string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
    return s;
}

std::string String::insert(std::string::size_type pos, const char* str)
{
    if (str) {
        s.insert(pos, str);
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        assert(str != nullptr);  // Assert fail in debug mode
    }
    return s;
}



std::string String::revsubstr(std::string::size_type pos, std::string::size_type n) const
{
    String r;
    n = std::min(n, s.size());
    r.reserve(n);
    for (auto i = n; i > pos; --i) {
        r.push_back(s[pos + i - 1]);
    }
    return r;
}

}

