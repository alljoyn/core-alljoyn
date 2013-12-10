/**
 * @file
 *
 * Utility functions for WinRT
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
#include <assert.h>
#include <cstdlib>

#include <Status.h>

#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <Status.h>

#define QCC_MODULE "UTILITY"

using namespace Windows::Foundation;

wchar_t* MultibyteToWideString(const char* str)
{
    wchar_t* buffer = NULL;

    while (true) {
        if (NULL == str) {
            break;
        }
        size_t charLen = mbstowcs(NULL, str, 0);
        if (charLen < 0) {
            break;
        }
        ++charLen;
        buffer = new wchar_t[charLen];
        if (NULL == buffer) {
            break;
        }
        if (mbstowcs(buffer, str, charLen) < 0) {
            break;
        }
        break;
    }

    return buffer;
}

Platform::String ^ MultibyteToPlatformString(const char* str)
{
    wchar_t* buffer = NULL;
    Platform::String ^ pstr = nullptr;

    while (true) {
        if (NULL == str) {
            break;
        }
        size_t charLen = mbstowcs(NULL, str, 0);
        if (charLen < 0) {
            break;
        }
        ++charLen;
        buffer = new wchar_t[charLen];
        if (NULL == buffer) {
            break;
        }
        if (mbstowcs(buffer, str, charLen) < 0) {
            break;
        }
        pstr = ref new Platform::String(buffer);
        break;
    }

    if (NULL != buffer) {
        delete [] buffer;
        buffer = NULL;
    }

    return pstr;
}


qcc::String PlatformToMultibyteString(Platform::String ^ str)
{
    char* buffer = NULL;
    const wchar_t* wstr = str->Data();
    qcc::String result = "";

    while (true) {
        if (nullptr == str) {
            break;
        }
        size_t charLen = wcstombs(NULL, wstr, 0);
        if (charLen < 0) {
            break;
        }
        ++charLen;
        buffer = new char[charLen];
        if (NULL == buffer) {
            break;
        }
        if (wcstombs(buffer, wstr, charLen) < 0) {
            break;
        }
        result = buffer;
        break;
    }

    if (NULL != buffer) {
        delete [] buffer;
        buffer = NULL;
    }

    return result;
}
