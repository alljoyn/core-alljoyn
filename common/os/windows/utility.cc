/**
 * @file
 *
 * Utility functions for Windows
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

#include <qcc/platform.h>

// Do not change the order of these includes; they are order dependent.
#include <Winsock2.h>
#include <Mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdlib.h>

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/windows/utility.h>

#define QCC_MODULE "UTILITY"

static bool winsockInitialized = false;

void strerror_r(uint32_t errCode, char* ansiBuf, uint16_t ansiBufSize)
{
    if (!ansiBuf || (ansiBufSize == 0)) {
        return;
    }

    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        errCode,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPSTR)ansiBuf,
                        ansiBufSize,
                        NULL)) {
        ansiBuf[0] = '\0';
    }
}


void WinsockCheck()
{
}
static int winsockInitCounter = 0;
bool WinsockInit::cleanedup = false;
WinsockInit::WinsockInit()
{
    if (winsockInitCounter++ == 0) {
        WSADATA wsaData;
        WORD version = MAKEWORD(2, 0);
        int error = WSAStartup(version, &wsaData);
        if (error) {
            printf("WSAStartup failed with error: %d\n", error);
        } else {
            winsockInitialized = true;
        }
    }
}
WinsockInit::~WinsockInit()
{
    if (--winsockInitCounter == 0 && !cleanedup) {
        int error = WSACleanup();
        if (SOCKET_ERROR == error) {
            printf("WSACleanup failed with error: %d\n", WSAGetLastError());
        }
        cleanedup = true;
    }
}
void WinsockInit::Cleanup()
{
    if (!cleanedup) {
        int error = WSACleanup();
        if (SOCKET_ERROR == error) {
            printf("WSACleanup failed with error: %d\n", WSAGetLastError());
        }
        cleanedup = true;
    }
}
wchar_t* MultibyteToWideString(const char* str)
{
    wchar_t* buffer = NULL;

    if (NULL == str) {
        return buffer;
    }
    size_t charLen = mbstowcs(NULL, str, 0);
    if (charLen < 0) {
        return buffer;
    }
    ++charLen;
    buffer = new wchar_t[charLen];
    if (NULL == buffer) {
        return buffer;
    }
    if (mbstowcs(buffer, str, charLen) < 0) {
        delete[] buffer;
        buffer = NULL;
        return buffer;
    }
    return buffer;
}
