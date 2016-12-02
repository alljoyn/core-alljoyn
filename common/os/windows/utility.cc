/**
 * @file
 *
 * Utility functions for Windows
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

wchar_t* MultibyteToWideString(const char* str)
{
    wchar_t* buffer = NULL;

    if (NULL == str) {
        return buffer;
    }
    size_t charLen = mbstowcs(NULL, str, 0);
    if (charLen == (size_t)-1) {
        return buffer;
    }
    ++charLen;
    buffer = new wchar_t[charLen];
    if (NULL == buffer) {
        return buffer;
    }
    if (mbstowcs(buffer, str, charLen) == (size_t)-1) {
        delete[] buffer;
        buffer = NULL;
        return buffer;
    }
    return buffer;
}