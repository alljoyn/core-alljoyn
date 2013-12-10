/**
 * @file
 *
 * Utility functions for Windows
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/windows/utility.h>

#define QCC_MODULE "UTILITY"


void strerror_r(uint32_t errCode, char* ansiBuf, uint16_t ansiBufSize)
{
    LPTSTR unicodeBuf = 0;

    uint32_t len = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &unicodeBuf,
        ansiBufSize - 1, NULL);
    memset(ansiBuf, 0, ansiBufSize);
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)unicodeBuf, -1, ansiBuf, (int)len, NULL, NULL);
    LocalFree(unicodeBuf);
}
