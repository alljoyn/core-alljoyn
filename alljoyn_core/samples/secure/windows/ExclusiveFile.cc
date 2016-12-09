/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Debug.h>

#include <windows.h>

#include "../ExclusiveFile.h"

#define QCC_MODULE "EXCLUSIVE_FILE"

static void ReSlash(qcc::String& inStr)
{
    size_t pos = inStr.find_first_of("/");
    while (pos != qcc::String::npos) {
        inStr[pos] = '\\';
        pos = inStr.find_first_of("/", pos + 1);
    }
}

ExclusiveFile::ExclusiveFile(qcc::String fileName) : handle(INVALID_HANDLE_VALUE), locked(false)
{
    ReSlash(fileName);

    DWORD attributes = FILE_FLAG_WRITE_THROUGH | FILE_ATTRIBUTE_HIDDEN;

    /* Compress leading slashes - we're not going to handle UNC paths */
    size_t skip = 0;
    while ('\\' == fileName[skip]) {
        ++skip;
    }
    skip = (skip > 0) ? (skip - 1) : 0;

    /* Create the intermediate directories */
    size_t begin = skip;

    /* Skip creating c:\ */
    if (fileName[begin + 1] == ':') {
        begin += 2;
    }

    for (size_t end = fileName.find("\\", begin); end != qcc::String::npos; end = fileName.find("\\", begin)) {
        /* Skip consecutive slashes */
        if (begin == end) {
            ++begin;
            continue;
        }

        /* Get the directory path */
        qcc::String p = fileName.substr(skip, end);

        /* Only try to create the directory if it doesn't already exist */
        if (CreateDirectoryA(p.c_str(), NULL)) {
            if (!SetFileAttributesA(p.c_str(), attributes)) {
                QCC_LogError(ER_OS_ERROR, ("SetFileAttributes() %s failed with (%d)", p.c_str(), ::GetLastError()));
                return;
            }
        } else {
            DWORD lastError = ::GetLastError();
            if (ERROR_ALREADY_EXISTS != lastError) {
                QCC_LogError(ER_OS_ERROR, ("CreateDirectory() %s failed with (%d)", p.c_str(), lastError));
                return;
            }
        }
        begin = end + 1;
    }

    /* Create or open the file */
    handle = CreateFileA(fileName.substr(skip).c_str(),
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_ALWAYS,
                         attributes,
                         INVALID_HANDLE_VALUE);
    if (handle == INVALID_HANDLE_VALUE) {
        QCC_ASSERT(!"Failed to create file");
        abort();
    }
}

ExclusiveFile::~ExclusiveFile()
{
    if (locked) {
        FlushFileBuffers(handle);
        OVERLAPPED ovl = { };
        QCC_VERIFY(TRUE == UnlockFileEx(handle, 0, 0, 0xFFFFFFFF, &ovl));
    }
}

QStatus ExclusiveFile::AcquireExclusiveLock()
{
    QCC_ASSERT(!locked);
    if (!locked) {
        OVERLAPPED ovl = { 0 };
        locked = LockFileEx(handle,
                            LOCKFILE_EXCLUSIVE_LOCK,
                            0, 0, 0xFFFFFFFF, &ovl);
        QCC_ASSERT(locked);
    }
    return (locked ? ER_OK : ER_OS_ERROR);
}

void ExclusiveFile::ReleaseExclusiveLock()
{
    QCC_ASSERT(locked);
    if (locked) {
        FlushFileBuffers(handle);
        OVERLAPPED ovl = { };
        BOOL ret = UnlockFileEx(handle, 0, 0, 0xFFFFFFFF, &ovl);
        QCC_ASSERT(ret);
        if (ret) {
            locked = false;
        }
    }
}

bool ExclusiveFile::HasExclusiveLock() const
{
    return locked;
}

QStatus ExclusiveFile::GetSize(int64_t& fileSize) const
{
    LARGE_INTEGER fileSizeLargeInt;

    if (!::GetFileSizeEx(handle, &fileSizeLargeInt)) {
        QStatus status = ER_OS_ERROR;
        QCC_LogError(status, ("GetFileSizeEx return error=(%#x) status=(%#x)", ::GetLastError(), status));
        return status;
    }

    fileSize = fileSizeLargeInt.QuadPart;
    return ER_OK;
}

bool ExclusiveFile::Truncate()
{
    QCC_ASSERT(locked);
    if (!::SetEndOfFile(handle)) {
        QCC_LogError(ER_OS_ERROR, ("SetEndOfFile failed. error=%d", ::GetLastError()));
        return false;
    }
    return true;
}

QStatus ExclusiveFile::ResetFilePointer()
{
    if (::SetFilePointer(handle, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        QCC_LogError(ER_OS_ERROR, ("SetFilePointer failed. error=%d", ::GetLastError()));
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus ExclusiveFile::Read(void* buf, size_t reqBytes, size_t& actualBytes)
{
    QStatus status = ResetFilePointer();
    if (status != ER_OK) {
        return status;
    }
    DWORD readBytes;
    BOOL ret = ReadFile(handle, buf, reqBytes, &readBytes, NULL);

    if (ret) {
        actualBytes = readBytes;
        return ((0 < reqBytes) && (0 == readBytes)) ? ER_EOF : ER_OK;
    } else {
        DWORD error = ::GetLastError();
        if (ERROR_HANDLE_EOF == error) {
            actualBytes = 0;
            return ER_EOF;
        } else {
            QCC_LogError(ER_FAIL, ("ReadFile returned error (%d)", error));
            return ER_FAIL;
        }
    }
}

QStatus ExclusiveFile::Write(const void* buf, size_t numBytes, size_t& numSent)
{
    QCC_ASSERT(locked);
    QStatus status = ResetFilePointer();
    if (status != ER_OK) {
        return status;
    }

    DWORD writeBytes;
    BOOL ret = WriteFile(handle, buf, numBytes, &writeBytes, NULL);

    if (ret) {
        numSent = writeBytes;
        return ER_OK;
    } else {
        QCC_LogError(ER_FAIL, ("WriteFile failed. error=%d", ::GetLastError()));
        return ER_FAIL;
    }
}