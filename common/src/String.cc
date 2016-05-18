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
#include <qcc/PerfCounters.h>
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
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_1);

    if (str) {
        s.reserve(sizeHint);
        s.assign(str, strLen);
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
}

String::String(const char* str, std::string::size_type strLen)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_2);

    if (str) {
        s.assign(str, strLen);
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
}

String::String(const char* str)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_3);

    if (str) {
        s = str;
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
}

String::String(const String& str) : s(str.s)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_4);
}

String::String(std::string&& str) : s(str)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_5);
}

String::String(const std::string& str) : s(str)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_6);
}

String::String(const const_iterator& start, const const_iterator& end) : s(start, end)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_7);
}

String::String(size_type n, char c) : s(n, c)
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_8);
}

String::String() : s()
{
    IncrementPerfCounter(PERF_COUNTER_STRING_CREATED_9);
}

String::~String()
{
    IncrementPerfCounter(PERF_COUNTER_STRING_DESTROYED);
}

String& String::assign(const char* str, std::string::size_type len)
{
    if (str) {
        if (len == 0) {
            QCC_LogError(ER_WARNING, ("Passing len = 0 will not copy entire contents of str in the future!"));
            QCC_ASSERT(len != 0);  // Assert fail in debug mode
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
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
        s = "";
    }
    return *this;
}

String& String::assign(const char* str)
{
    if (str) {
        s.assign(str);
    } else {
        QCC_LogError(ER_WARNING, ("Assigning string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
        s = "";
    }
    return *this;
}

std::string::size_type String::secure_clear()
{
    char* d = const_cast<char*>(s.data());
    ClearMemory(d, s.size());
    return 1;
}

String& String::append(const char* str, std::string::size_type len)
{
    if (str) {
        s.append(str, len);
    } else {
        QCC_LogError(ER_WARNING, ("Appending string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
    return *this;
}

String& String::append(const char* str)
{
    if (str) {
        s.append(str);
    } else {
        QCC_LogError(ER_WARNING, ("Appending string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
    return *this;
}

String& String::insert(std::string::size_type pos, const char* str, std::string::size_type len)
{
    if (str) {
        s.insert(pos, str, len);
    } else {
        QCC_LogError(ER_WARNING, ("Appending string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
    return *this;
}

String& String::insert(std::string::size_type pos, const char* str)
{
    if (str) {
        s.insert(pos, str);
    } else {
        QCC_LogError(ER_WARNING, ("Constructing string from nullptr will cause a crash in future versions!"));
        QCC_ASSERT(str != nullptr);  // Assert fail in debug mode
    }
    return *this;
}

String String::revsubstr(std::string::size_type pos, std::string::size_type n) const
{
    String r;
    n = std::min(n, s.size());
    r.reserve(n);
    for (auto i = n; i > pos; --i) {
        r.push_back(s[pos + i - 1]);
    }
    return r;
}

int String::compare(size_type pos, size_type n, const String& other, size_type otherPos, size_type otherN) const
{
    int ret;
    if ((pos >= s.length()) || (otherPos >= other.length())) {
        QCC_ASSERT(!"Position out of range.");
        ret = static_cast<int>(npos);
        return ret;
    }
    if ((s.c_str() != other.c_str()) || (pos != otherPos)) {
        size_t subStrLen = std::min(s.length() - pos, n);
        size_t sLen = std::min(other.length() - otherPos, otherN);
        ret = ::memcmp(s.c_str() + pos, other.c_str() + otherPos, std::min(subStrLen, sLen));
        if ((0 == ret) && (subStrLen < sLen)) {
            ret = -1;
        } else if ((0 == ret) && (subStrLen > sLen)) {
            ret = 1;
        }
    } else {
        ret = 0;
    }
    return ret;
}

int String::compare(size_type pos, size_type n, const String& other) const
{
    int ret;
    if (pos >= s.length()) {
        QCC_ASSERT(!"Position out of range.");
        ret = static_cast<int>(npos);
        return ret;
    }
    if ((s.length() > 0) || (other.length() > 0)) {
        if ((pos == 0) && (s.c_str() == other.c_str())) {
            ret = 0;
        } else {
            size_t subStrLen = std::min(s.length() - pos, n);
            size_t sLen = other.length();
            ret = ::memcmp(s.c_str() + pos, other.c_str(), std::min(subStrLen, sLen));
            if ((0 == ret) && (subStrLen < sLen)) {
                ret = -1;
            } else if ((0 == ret) && (subStrLen > sLen)) {
                ret = 1;
            }
        }
    } else {
        ret = 0;
    }
    return ret;
}

}

