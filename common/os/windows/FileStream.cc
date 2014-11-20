/**
 * @file
 *
 * Sink/Source wrapper FILE operations for windows
 */

/******************************************************************************
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
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <windows.h>

#include <qcc/FileStream.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE  "STREAM"

QStatus qcc::DeleteFile(qcc::String fileName)
{
    if (::DeleteFileA(fileName.c_str())) {
        return ER_OK;
    } else {
        return ER_OS_ERROR;
    }
}

static void ReSlash(qcc::String& inStr)
{
    size_t pos = inStr.find_first_of("/");
    while (pos != qcc::String::npos) {
        inStr[pos] = '\\';
        pos = inStr.find_first_of("/", pos + 1);
    }
}

static HANDLE DupHandle(HANDLE inHandle)
{
    HANDLE outHandle = INVALID_HANDLE_VALUE;
    DuplicateHandle(GetCurrentProcess(),
                    inHandle,
                    GetCurrentProcess(),
                    &outHandle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);
    return outHandle;
}

FileSource::FileSource(qcc::String fileName) : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(true), locked(false)
{
    ReSlash(fileName);
    handle = CreateFileA(fileName.c_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         INVALID_HANDLE_VALUE);

    if (INVALID_HANDLE_VALUE == handle) {
        QCC_LogError(ER_OS_ERROR, ("CreateFile(GENERIC_READ) %s failed (%d)", fileName.c_str(), ::GetLastError()));
    }
}

FileSource::FileSource() : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(false)
{
    handle = GetStdHandle(STD_INPUT_HANDLE);

    if (NULL == handle) {
        QCC_LogError(ER_OS_ERROR, ("GetStdHandle failed (%d)", ::GetLastError()));
    }
}

FileSource::FileSource(const FileSource& other) :
    handle((other.handle == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : DupHandle(other.handle)),
    event(&Event::alwaysSet),
    ownsHandle(true),
    locked(other.locked)
{
}

FileSource FileSource::operator=(const FileSource& other)
{
    if (&other != this) {
        if (ownsHandle && (INVALID_HANDLE_VALUE != handle)) {
            CloseHandle(handle);
        }
        handle = (other.handle == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : DupHandle(other.handle);
        event = &Event::alwaysSet;
        ownsHandle = true;
        locked = other.locked;
    }
    return *this;
}

FileSource::~FileSource()
{
    if (ownsHandle && (INVALID_HANDLE_VALUE != handle)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

QStatus FileSource::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    if (INVALID_HANDLE_VALUE == handle) {
        return ER_INIT_FAILED;
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

bool FileSource::Lock(bool block)
{
    if (INVALID_HANDLE_VALUE == handle) {
        return false;
    }
    if (locked) {
        return true;
    } else {
        OVERLAPPED ovl = { 0 };
        locked = LockFileEx(handle,
                            block ? LOCKFILE_EXCLUSIVE_LOCK : LOCKFILE_FAIL_IMMEDIATELY,
                            0, 0, 0xFFFFFFFF, &ovl);
        return locked;

    }
}

void FileSource::Unlock()
{
    if (handle != INVALID_HANDLE_VALUE && locked) {
        OVERLAPPED ovl = { 0 };
        UnlockFileEx(handle, 0, 0, 0xFFFFFFFF, &ovl);
        locked = false;
    }
}

FileSink::FileSink(qcc::String fileName, Mode mode) : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(true), locked(false)
{
    ReSlash(fileName);

    DWORD attributes;
    switch (mode) {
    case PRIVATE :
        attributes = FILE_ATTRIBUTE_HIDDEN;
        break;

    case WORLD_READABLE:
    case WORLD_WRITABLE:
        attributes = FILE_ATTRIBUTE_NORMAL;
        break;

    default:
        QCC_LogError(ER_BAD_ARG_2, ("Invalid mode"));
        return;
    }

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

    for (size_t end = fileName.find('\\', begin); end != String::npos; end = fileName.find('\\', begin)) {


        /* Skip consecutive slashes */
        if (begin == end) {
            ++begin;
            continue;
        }

        /* Get the directory path */
        String p = fileName.substr(skip, end);

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

    /* Create and open the file */
    handle = CreateFileA(fileName.substr(skip).c_str(),
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         CREATE_ALWAYS,
                         attributes,
                         INVALID_HANDLE_VALUE);

    if (INVALID_HANDLE_VALUE == handle) {
        QCC_LogError(ER_OS_ERROR, ("CreateFile(GENERIC_WRITE) %s failed (%d)", fileName.c_str(), ::GetLastError()));
    }
}

FileSink::FileSink() : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(false), locked(false)
{
    handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (NULL == handle) {
        QCC_LogError(ER_OS_ERROR, ("GetStdHandle failed (%d)", ::GetLastError()));
    }
}

FileSink::FileSink(const FileSink& other) :
    handle((other.handle == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : DupHandle(other.handle)),
    event(&Event::alwaysSet),
    ownsHandle(true),
    locked(other.locked)
{
}

FileSink FileSink::operator=(const FileSink& other)
{
    if (&other != this) {
        if (ownsHandle && (INVALID_HANDLE_VALUE != handle)) {
            CloseHandle(handle);
        }
        handle = (other.handle == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : DupHandle(other.handle);
        event = &Event::alwaysSet;
        ownsHandle = true;
        locked = other.locked;
    }
    return *this;
}

FileSink::~FileSink() {
    if (INVALID_HANDLE_VALUE != handle) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

QStatus FileSink::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{

    if (INVALID_HANDLE_VALUE == handle) {
        return ER_INIT_FAILED;
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

bool FileSink::Lock(bool block)
{
    if (INVALID_HANDLE_VALUE == handle) {
        return false;
    }
    if (locked) {
        return true;
    } else {
        OVERLAPPED ovl = { 0 };
        locked = LockFileEx(handle,
                            block ? LOCKFILE_EXCLUSIVE_LOCK : LOCKFILE_FAIL_IMMEDIATELY,
                            0, 0, 0xFFFFFFFF, &ovl);
        return locked;

    }
}

void FileSink::Unlock()
{
    if (handle != INVALID_HANDLE_VALUE && locked) {
        OVERLAPPED ovl = { 0 };
        UnlockFileEx(handle, 0, 0, 0xFFFFFFFF, &ovl);
        locked = false;
    }
}
