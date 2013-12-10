#ifndef _UNICODE_H
#define _UNICODE_H
/**
 * @file
 *
 * This file defines a convenience abstraction layer for ConvertUTF.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include unicode.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/unicode.h>
#include <qcc/String.h>
#include "ConvertUTF.h"
#include <alljoyn/Status.h>

/*
 * wstring is defined in the android NDK 8 but not defined in NDK7
 * So, defining wstring in case it is not defined
 */
namespace std {
typedef basic_string<wchar_t> wstring;
}


using namespace qcc;

/** @internal */
#define QCC_MODULE "CONVERT_UTF"

/**
 * Perform a UTF conversion from a UTF-8 sequence in a String to a UTF-16
 * or UTF-32 sequence in a std::wstring.  Whether UTF-16 or UTF-32 is used
 * depends on sizeof(wchar_t).
 *
 * @param src       A UTF-8 sequence in a String.
 * @param dest      OUT: A reference to where the UTF-16/-32 sequence is to be
 *                       written.
 * @param strict    Indicate whether src is checked for strict compliance with
 *                  the Unicode spec or not.
 *
 * @return ER_OK is conversion was successful; ER_UTF_CONVERSION_FAILED if
 *         conversion was unsuccessful.
 */
inline QStatus ConvertUTF(const String src, std::wstring& dest, bool strict = false)
{
    QStatus status = ER_OK;
    ConversionResult result;
    const UTF8*srcStart = reinterpret_cast<const UTF8*>(src.c_str());
    size_t strLen = src.length();
    WideUTF*tmpDest = new WideUTF[strLen];
    WideUTF*tmpDestStart = tmpDest;

    result = ConvertUTF8ToWChar(&srcStart, srcStart + strLen,
                                &tmpDestStart, tmpDest + strLen,
                                strict ? strictConversion : lenientConversion);

    if (result != conversionOK) {
        status = ER_UTF_CONVERSION_FAILED;
        QCC_LogError(status, ("ConvertUTF string -> wstring: %d", result));
        goto exit;
    }
    dest = std::wstring(reinterpret_cast<wchar_t*>(tmpDest), tmpDestStart - tmpDest);

exit:
    delete tmpDest;
    return status;
}

/**
 * @overload
 * Perform a UTF conversion from a UTF-16 or UTF-32 sequence in a
 * std::wstring to a UTF-8 sequence in a String.  Whether UTF-16 or
 * UTF-32 is used depends on sizeof(wchar_t).
 *
 * @param src       A UTF-16/-32 sequence in a std::wstring.
 * @param dest      OUT: A reference to where the UTF-8 sequence is to be
 *                       written.
 * @param strict    Indicate whether src is checked for strict compliance with
 *                  the Unicode spec or not.
 *
 * @return ER_OK is conversion was successful; ER_UTF_CONVERSION_FAILED if
 *         conversion was unsuccessful.
 */
inline QStatus ConvertUTF(const std::wstring src, String& dest, bool strict = false)
{
    QStatus status = ER_OK;
    ConversionResult result;
    const WideUTF*srcStart = reinterpret_cast<const WideUTF*>(src.data());
    size_t srcStrLen = src.length();
    size_t destStrLen = srcStrLen * 4;  // Need extra room for multi-byte characters.
    UTF8*tmpDest = new UTF8[destStrLen];
    UTF8*tmpDestStart = tmpDest;

    result = ConvertWCharToUTF8(&srcStart, srcStart + srcStrLen,
                                &tmpDestStart, tmpDest + destStrLen,
                                strict ? strictConversion : lenientConversion);

    if (result != conversionOK) {
        status = ER_UTF_CONVERSION_FAILED;
        QCC_LogError(status, ("ConvertUTF wstring -> string: %d", result));
        goto exit;
    }
    dest = String(reinterpret_cast<char*>(tmpDest), tmpDestStart - tmpDest);

exit:
    delete tmpDest;
    return status;
}

#undef QCC_MODULE
#endif
