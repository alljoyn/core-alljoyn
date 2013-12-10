/**
 * @file
 *
 * Sink/Source wrapper FILE operations for WinRT
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

#include <windows.h>
#include <assert.h>
#include <cstdlib>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/winrt/utility.h>

#include <qcc/FileStream.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE  "STREAM"

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
    wchar_t* wFileName = NULL;
    while (true) {
        ReSlash(fileName);
        wFileName = MultibyteToWideString(fileName.c_str());
        if (NULL == wFileName) {
            QCC_LogError(ER_OS_ERROR, ("MultibyteToWideString %s failed", fileName.c_str()));
            break;
        }
        handle = CreateFile2(wFileName,
                             GENERIC_READ,
                             0,
                             OPEN_EXISTING,
                             NULL);
        if (INVALID_HANDLE_VALUE == handle) {
            QCC_LogError(ER_OS_ERROR, ("CreateFile(GENERIC_READ) %s failed (%d)", fileName.c_str(), ::GetLastError()));
            break;
        }
        break;
    }
    if (NULL != wFileName) {
        delete [] wFileName;
        wFileName = NULL;
    }
}

FileSource::FileSource() : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(false)
{
    // STDIN/STDOUT/STDERR are not supported. Allow default initializations to fail this path on subsequent calls.
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
        return ((0 < reqBytes) && (0 == readBytes)) ? ER_NONE : ER_OK;
    } else {
        DWORD error = ::GetLastError();
        if (ERROR_HANDLE_EOF == error) {
            actualBytes = 0;
            return ER_NONE;
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
    wchar_t* wFileName = NULL;
    while (true) {
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

        bool failedCreateDirecotry = false;

        for (size_t end = fileName.find('\\', begin); end != String::npos; end = fileName.find('\\', begin)) {
            /* Skip consecutive slashes */
            if (begin == end) {
                ++begin;
                continue;
            }
            /* Get the directory path */
            qcc::String p = fileName.substr(skip, end);
            wchar_t* wTemp = MultibyteToWideString(p.c_str());
            if (NULL == wTemp) {
                failedCreateDirecotry = true;
                QCC_LogError(ER_OS_ERROR, ("MultibyteToWideString %s failed", p.c_str()));
                break;
            }
            /* Only try to create the directory if it doesn't already exist */
            if (CreateDirectoryW(wTemp, NULL)) {
                if (!SetFileAttributesW(wTemp, attributes)) {
                    failedCreateDirecotry = true;
                    QCC_LogError(ER_OS_ERROR, ("SetFileAttributes() %s failed with (%d)", p.c_str(), ::GetLastError()));
                    break;
                }
            } else if (ERROR_ALREADY_EXISTS != ::GetLastError()) {
                QCC_LogError(ER_OS_ERROR, ("CreateDirectory() %s failed with (%d)", p.c_str(), ::GetLastError()));
            }
            if (NULL != wTemp) {
                delete [] wTemp;
                wTemp = NULL;
            }
            begin = end + 1;
        }

        if (failedCreateDirecotry) {
            break;
        }

        wFileName = MultibyteToWideString(fileName.substr(skip).c_str());
        if (NULL == wFileName) {
            QCC_LogError(ER_OS_ERROR, ("MultibyteToWideString %s failed", fileName.substr(skip).c_str()));
            break;
        }

        /* Create and open the file */
        handle = CreateFile2(wFileName,
                             GENERIC_WRITE,
                             0,
                             CREATE_ALWAYS,
                             NULL);
        if (INVALID_HANDLE_VALUE == handle) {
            QCC_LogError(ER_OS_ERROR, ("CreateFile(GENERIC_WRITE) %s failed (%d)", fileName.c_str(), ::GetLastError()));
        }
        break;
    }
    if (NULL != wFileName) {
        delete [] wFileName;
        wFileName = NULL;
    }
}

FileSink::FileSink() : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(false), locked(false)
{
    // STDIN/STDOUT/STDERR are not supported. Allow default initializations to fail this path on subsequent calls.
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
