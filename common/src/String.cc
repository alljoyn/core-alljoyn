/**
 * @file
 *
 * "STL-like" version of string.
 */

/******************************************************************************
 * Copyright (c) 2010-2015, AllSeen Alliance. All rights reserved.
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
#include <limits>
#include <new>

#if defined(WIN32) || (defined(QCC_OS_DARWIN) && MAC_OS_X_VERSION_MAX_ALLOWED < 1070)
/*
 * memmem not provided on Windows
 */
static const void* memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen)
{
    if (haystack && needle) {
        const char* pos = (const char*)haystack;
        while (haystacklen >= needlelen) {
            if (memcmp(pos, needle, needlelen) == 0) {
                return pos;
            }
            pos++;
            --haystacklen;
        }
    }
    return NULL;
}
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

namespace qcc {

/* Global Data */

uint64_t emptyStringDummy[RequiredArrayLength(sizeof(String), uint64_t)];

String& emptyString = (String &)emptyStringDummy;

const String& String::Empty = (String &)emptyString;

String::ManagedCtx String::nullContext = { 0 };

int stringInitCounter = 0;
bool StringInit::cleanedup = false;
StringInit::StringInit()
{
    if (stringInitCounter++ == 0) {
        //placement new
        new (&emptyString)String();
    }
}

StringInit::~StringInit()
{
    if (--stringInitCounter == 0 && !cleanedup) {
        //placement delete
        emptyString.~String();
        cleanedup = true;
    }
}
void StringInit::Cleanup()
{
    if (!cleanedup) {
        //placement delete
        emptyString.~String();
        cleanedup = true;
    }
}

String::String()
{
    context = &nullContext;
}

String::String(char c, size_t sizeHint)
{
    NewContext(&c, 1, sizeHint);
}

String::String(const char* str, size_t strLen, size_t sizeHint)
{
    if ((str && *str) || sizeHint) {
        NewContext(str, strLen, sizeHint);
    } else {
        context = &nullContext;
    }
}

String::String(size_t n, char c, size_t sizeHint)
{
    NewContext(NULL, 0, MAX(n, sizeHint));
    ::memset(context->c_str, c, n);
    context->offset += n;
    context->c_str[context->offset] = '\0';
}

String::String(const String& copyMe)
{
    context = copyMe.context;
    IncRef();
}

String::~String()
{
    DecRef(context);
}

String& String::operator=(const String& assignFromMe)
{
    /* avoid unnecessary free/malloc when assigning to self */
    if (&assignFromMe != this) {
        /* Decrement ref of current context */
        DecRef(context);

        /* Reassign this Managed Obj */
        context = assignFromMe.context;

        /* Increment the ref */
        IncRef();
    }

    return *this;
}

String& String::assign(const char* str, size_t len)
{
    if (context == &nullContext) {
        append(str, len);
    } else if (context->refCount == 1) {
        context->offset = 0;
        context->c_str[0] = 0;
        append(str, len);
    } else {
        /* Decrement ref of current context */
        DecRef(context);
        NewContext(str, len, len);
    }
    return *this;
}

char& String::operator[](size_t pos)
{
    if ((context != &nullContext) && (1 != context->refCount)) {
        /* Modifying a context with other refs, so make a copy */
        ManagedCtx* oldContext = context;
        NewContext(oldContext->c_str, size(), oldContext->capacity);
        DecRef(oldContext);
    }
    return context->c_str[pos];
}

bool String::operator<(const String& str) const
{
    /* Include the null in the compare to catch case when two strings have different lengths */
    if ((context != &nullContext) && str.context) {
        return (context != str.context) && (0 > ::memcmp(context->c_str,
                                                         str.context->c_str,
                                                         MIN(context->offset, str.context->offset) + 1));
    } else {
        return size() < str.size();
    }
}

int String::compare(size_t pos, size_t n, const String& s) const
{
    int ret;
    if ((context != &nullContext) && s.context) {
        if ((pos == 0) && (context == s.context)) {
            ret = 0;
        } else {
            size_t subStrLen = MIN(context->offset - pos, n);
            size_t sLen = s.context->offset;
            ret = ::memcmp(context->c_str + pos, s.context->c_str, MIN(subStrLen, sLen));
            if ((0 == ret) && (subStrLen < sLen)) {
                ret = -1;
            } else if ((0 == ret) && (subStrLen > sLen)) {
                ret = 1;
            }
        }
    } else {
        if ((context != &nullContext) && (0 < n) && (npos != pos)) {
            ret = 1;
        } else if (0 < s.size()) {
            ret = -1;
        } else {
            ret = 0;
        }
    }
    return ret;
}

int String::compare(size_t pos, size_t n, const String& s, size_t sPos, size_t sn) const
{
    int ret;
    if ((pos == sPos) && (context == s.context)) {
        ret = 0;
    } else {
        size_t subStrLen = MIN(context->offset - pos, n);
        size_t sSubStrLen = MIN(s.context->offset - sPos, sn);
        ret = ::memcmp(context->c_str + pos, s.context->c_str + sPos, MIN(subStrLen, sSubStrLen));
        if ((0 == ret) && (subStrLen < sSubStrLen)) {
            ret = -1;
        } else if ((0 == ret) && (subStrLen > sSubStrLen)) {
            ret = 1;
        }
    }
    return ret;
}

size_t String::secure_clear()
{
    uint32_t refs = 1;
    if (context != &nullContext) {
        memset(context->c_str, 0, context->capacity);
        context->offset = 0;
        refs = context->refCount;
        DecRef(context);
        context = &nullContext;
    }
    return refs - 1;
}

void String::clear(size_t sizeHint)
{
    DecRef(context);
    context = &nullContext;
}

String& String::append(const char c)
{
    size_t totalLen = 1 + context->offset;
    if ((1 != context->refCount) || (totalLen > context->capacity)) {
        /* Append wont fit in existing allocation or modifying a context with other refs */
        ManagedCtx* oldContext = context;
        NewContext(oldContext->c_str, oldContext->offset, totalLen + totalLen / 2);
        DecRef(oldContext);
    }
    context->c_str[context->offset] = c;
    context->c_str[++context->offset] = '\0';
    return *this;
}

String& String::append(const char* str, size_t strLen)
{
    if (NULL == str) {
        return *this;
    }
    if (0 == strLen) {
        strLen = ::strlen(str);
    }
    if (0 == strLen) {
        return *this;
    }

    // Make sure strLen does not cause any kind of overflow.
    strLen = MIN(strLen, std::numeric_limits<uint32_t>::max() - context->offset - 1);
    size_t totalLen = strLen + context->offset;
    if ((1 != context->refCount) || (totalLen > context->capacity)) {
        /* Append wont fit in existing allocation or modifying a context with other refs */
        size_t sizeHint = totalLen;
        if (totalLen < (std::numeric_limits<uint32_t>::max() - totalLen / 2)) {
            sizeHint += totalLen / 2;
        }
        ManagedCtx* oldContext = context;
        NewContext(oldContext->c_str, oldContext->offset, sizeHint);
        DecRef(oldContext);
    }
    size_t copySize = MIN(strLen, context->capacity - context->offset);
    ::memcpy(context->c_str + context->offset, str, copySize);
    context->offset += copySize;
    context->c_str[context->offset] = '\0';
    return *this;
}

String& String::erase(size_t pos, size_t n)
{
    /* Trying to erase past the end of the string, do nothing. */
    if (pos >= size()) {
        return *this;
    }

    if (context != &nullContext) {
        if (context->refCount != 1) {
            /* Need a new context */
            ManagedCtx* oldContext = context;
            NewContext(context->c_str, context->offset, context->capacity);
            DecRef(oldContext);
        }
        n = MIN(n, size() - pos);
        ::memmove(context->c_str + pos, context->c_str + pos + n, size() - pos - n + 1);
        context->offset -= n;
    }
    return *this;
}

void String::resize(size_t n, char c)
{
    if ((n > 0) && (context == &nullContext)) {
        NewContext(NULL, 0, n);
    }

    size_t curSize = size();
    if (n < curSize) {
        if (context->refCount == 1) {
            context->offset = n;
            context->c_str[n] = '\0';
        } else {
            ManagedCtx* oldContext = context;
            NewContext(context->c_str, n, n);
            DecRef(oldContext);
        }
    } else if (n > curSize) {
        if (n >= context->capacity) {
            ManagedCtx* oldContext = context;
            NewContext(context->c_str, curSize, n);
            DecRef(oldContext);
        }
        ::memset(context->c_str + curSize, c, n - curSize);
        context->offset = n;
        context->c_str[n] = '\0';
    }
}

void String::reserve(size_t newCapacity)
{
    if ((0 < newCapacity) && (context == &nullContext)) {
        NewContext(NULL, 0, newCapacity);
    }

    newCapacity = MAX(newCapacity, size());
    if (newCapacity != context->capacity) {
        size_t sz = size();
        ManagedCtx* oldContext = context;
        NewContext(context->c_str, sz, newCapacity);
        DecRef(oldContext);
    }
}

String& String::insert(size_t pos, const char* str, size_t strLen)
{
    if (NULL == str) {
        return *this;
    }
    if (0 == strLen) {
        strLen = ::strlen(str);
    }

    if (context == &nullContext) {
        NewContext(NULL, 0, strLen);
    }

    pos = MIN(pos, context->offset);

    size_t totalLen = strLen + context->offset;
    if ((1 != context->refCount) || (totalLen > context->capacity)) {
        /* insert wont fit in existing allocation or modifying a context with other refs */
        ManagedCtx* oldContext = context;
        NewContext(oldContext->c_str, oldContext->offset, totalLen + totalLen / 2);
        DecRef(oldContext);
    }
    ::memmove(context->c_str + pos + strLen, context->c_str + pos, context->offset - pos + 1);
    ::memcpy(context->c_str + pos, str, strLen);
    context->offset += strLen;
    return *this;
}

size_t String::find(const char* str, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }

    const char* base = context->c_str;
    const char* p = static_cast<const char*>(::memmem(base + pos, context->offset - pos, str, ::strlen(str)));
    return p ? p - base : npos;
}

size_t String::find(const String& str, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }
    if (0 == str.size()) {
        return 0;
    }

    const char* base = context->c_str;
    const char* p = static_cast<const char*>(::memmem(base + pos,
                                                      context->offset - pos,
                                                      str.context->c_str,
                                                      str.context->offset));
    return p ? p - base : npos;
}

size_t String::find_first_of(const char c, size_t pos) const
{
    if ((context == &nullContext) || (pos >= size())) {
        return npos;
    }

    const char* ret = ::strchr(context->c_str + pos, (int) c);
    return ret ? (ret - context->c_str) : npos;
}

size_t String::find_last_of(const char c, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }

    size_t i = MIN(pos, size());
    while (i-- > 0) {
        if (c == context->c_str[i]) {
            return i;
        }
    }
    return npos;
}

size_t String::find_first_of(const char* set, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }

    size_t i = pos;
    size_t endIdx = size();
    while (i < endIdx) {
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                return i;
            }
        }
        ++i;
    }
    return npos;
}

size_t String::find_last_of(const char* set, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }

    size_t i = MIN(pos, size());
    while (i-- > 0) {
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                return i;
            }
        }
    }
    return npos;
}

size_t String::find_first_not_of(const char* set, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }

    size_t i = pos;
    size_t endIdx = size();
    while (i < endIdx) {
        bool found = false;
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return i;
        }
        ++i;
    }
    return npos;
}

size_t String::find_last_not_of(const char* set, size_t pos) const
{
    if (context == &nullContext) {
        return npos;
    }

    size_t i = MIN(pos, size());
    while (i-- > 0) {
        bool found = false;
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    return npos;
}

String String::substr(size_t pos, size_t n) const
{
    if ((pos <= size()) && n) {
        return String(context->c_str + pos, MIN(n, size() - pos));
    } else {
        return String();
    }
}

String String::revsubstr(size_t pos, size_t n) const
{
    if (pos <= size()) {
        size_t l = MIN(n, size() - pos);
        String rev("", 0, l);
        char* p = context->c_str + pos + l;
        char* q = rev.context->c_str;
        rev.context->offset = l;
        while (l--) {
            *q++ = *--p;
        }
        return rev;
    } else {
        return String();
    }
}

bool String::operator==(const String& other) const
{
    if (context == other.context) {
        /* If both contexts point to the same memory then they are, by definition, the same. */
        return true;
    }
    size_t otherSize = other.size();
    if ((context != &nullContext) && (0 < otherSize)) {
        if (context->offset != other.context->offset) {
            return false;
        }
        return (0 == ::memcmp(context->c_str, other.context->c_str, context->offset));
    } else {
        /* Both strings must be empty or they aren't equal */
        return (size() == other.size());
    }
}

void String::NewContext(const char* str, size_t strLen, size_t sizeHint)
{
    if (NULL == str) {
        strLen = 0;
    } else if (0 == strLen) {
        strLen = ::strlen(str);
    }

    // Truncate strLen and sizeHint to prevent computational overflows.
    size_t maxSize = std::numeric_limits<uint32_t>::max() - 1 - (sizeof(ManagedCtx) - MinCapacity);
    strLen = MIN(strLen, maxSize);
    sizeHint = MIN(sizeHint, maxSize);

    size_t capacity = MAX(MinCapacity, MAX(strLen, sizeHint));
    size_t mallocSz = capacity + 1 + sizeof(ManagedCtx) - MinCapacity;
    void* newCtxMem = malloc(mallocSz);
    assert(newCtxMem);
    context = new (newCtxMem)ManagedCtx();
    context->refCount = 1;

    context->capacity = static_cast<uint32_t>(capacity);
    context->offset = static_cast<uint32_t>(strLen);
    if (str) {
        ::memcpy(context->c_str, str, strLen);
    }
    context->c_str[strLen] = '\0';
}

void String::IncRef()
{
    /* Increment the ref count */
    if (context != &nullContext) {
        IncrementAndFetch(&context->refCount);
    }
}

void String::DecRef(ManagedCtx* ctx)
{
    /* Decrement the ref count */
    if (ctx != &nullContext) {
        uint32_t refs = DecrementAndFetch(&ctx->refCount);
        if (0 == refs) {
#if defined(QCC_OS_DARWIN) || defined(__clang__)
            ctx->~ManagedCtx();
#else
            ctx->ManagedCtx::~ManagedCtx();
#endif
            free(ctx);
        }
    }
}

}

/* Global function (not part of qcc namespace) */
qcc::String operator+(const qcc::String& s1, const qcc::String& s2)
{
    qcc::String ret(s1);
    return ret.append(s2);
}

std::ostream& operator<<(std::ostream& os, const qcc::String& str)
{
    return os << str.c_str();
}

