/**
 * @file
 *
 * Define utility functions for Windows.
 */

/******************************************************************************
 *
 *
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
#ifndef _OS_QCC_UTILITY_H
#define _OS_QCC_UTILITY_H

#include <qcc/platform.h>
#include <windows.h>

void strerror_r(uint32_t errCode, char* ansiBuf, uint16_t ansiBufSize);

wchar_t* MultibyteToWideString(const char* str);

/**
 * Ensure that Winsock API is loaded.
 * Called before any operation that might be called before winsock has been started.
 */
void WinsockCheck();

/**
 * Clean up Winsock API. Caller must ensure that this is the last call to Winsock.
 */
void WinsockCleanup();

static struct WinsockInit {
    WinsockInit();
    ~WinsockInit();
    static void Cleanup();

  private:
    static bool cleanedup;

} winsockInit;

#endif
