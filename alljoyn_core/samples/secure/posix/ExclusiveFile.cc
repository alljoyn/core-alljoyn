/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Debug.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "../ExclusiveFile.h"

#define QCC_MODULE "EXCLUSIVE_FILE"

ExclusiveFile::ExclusiveFile(qcc::String fileName) : fd(-1), locked(false)
{
#ifdef QCC_OS_ANDROID
    /* Android uses per-user groups so user and group permissions are the same */
    mode_t fileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    mode_t dirMode = S_IRWXU | S_IRWXG | S_IXOTH;
#else
    /* Default for plain posix is user permissions only */
    mode_t fileMode = S_IRUSR | S_IWUSR;
    mode_t dirMode = S_IRWXU | S_IXGRP | S_IXOTH;
#endif

    /* Create the intermediate directories */
    size_t begin = 0;
    for (size_t end = fileName.find("/", begin); end != qcc::String::npos; end = fileName.find("/", begin)) {

        /* Skip consecutive slashes */
        if (begin == end) {
            ++begin;
            continue;
        }

        /* Get the directory path */
        qcc::String p = fileName.substr(0, end);

        /* Only try to create the directory if it doesn't already exist */
        struct stat sb;
        if (0 > stat(p.c_str(), &sb)) {
            if (0 > mkdir(p.c_str(), dirMode)) {
                QCC_LogError(ER_OS_ERROR, ("mkdir(%s) failed with '%s'", p.c_str(), strerror(errno)));
                return;
            }
        }
        begin = end + 1;
    }

    /* Create and open the file */
    fd = open(fileName.c_str(), O_CREAT | O_RDWR, fileMode);
    if (fd < 0) {
        QCC_ASSERT(!"Failed to create file");
        abort();
    }
}

ExclusiveFile::~ExclusiveFile()
{
    if (locked) {
        QCC_VERIFY(true == flock(fd, LOCK_UN));
    }
}

QStatus ExclusiveFile::AcquireExclusiveLock()
{
    QCC_ASSERT(!locked);
    if (!locked) {
        int ret = flock(fd, LOCK_EX);
        QCC_ASSERT(ret == 0);
        locked = (ret == 0);
    }
    return (locked ? ER_OK : ER_OS_ERROR);
}

void ExclusiveFile::ReleaseExclusiveLock()
{
    QCC_ASSERT(locked);
    if (locked) {
        int ret = flock(fd, LOCK_UN);
        QCC_ASSERT(ret == 0);
        if (ret == 0) {
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
    struct stat buf = { };

    if (0 > fstat(fd, &buf)) {
        QStatus status = ER_OS_ERROR;
        QCC_LogError(status, ("fstat returned error (%d)", errno));
        return status;
    }

    fileSize = buf.st_size;
    return ER_OK;
}


bool ExclusiveFile::Truncate()
{
    QCC_ASSERT(locked);
    int ftrunRet = ftruncate(fd, lseek(fd, 0, SEEK_CUR));
    if (ftrunRet < 0) {
        QCC_LogError(ER_OS_ERROR, ("ftruncate fd %d failed with '%s'", fd, strerror(errno)));
        return false;
    }
    return true;
}

QStatus ExclusiveFile::ResetFilePointer()
{
    int lseekRet = lseek(fd, 0, SEEK_SET);
    if (lseekRet < 0) {
        QCC_LogError(ER_OS_ERROR, ("Lseek fd %d failed with '%s'", fd, strerror(errno)));
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
    if (reqBytes == 0) {
        actualBytes = 0;
        return ER_OK;
    }
    ssize_t ret = read(fd, buf, reqBytes);
    if (0 > ret) {
        QCC_LogError(ER_FAIL, ("read returned error (%d)", errno));
        return ER_FAIL;
    } else {
        actualBytes = ret;
        return (0 == ret) ? ER_EOF : ER_OK;
    }
}

QStatus ExclusiveFile::Write(const void* buf, size_t numBytes, size_t& numSent)
{
    QCC_ASSERT(locked);
    QStatus status = ResetFilePointer();
    if (status != ER_OK) {
        return status;
    }
    ssize_t ret = write(fd, buf, numBytes);
    if (0 <= ret) {
        numSent = ret;
        return ER_OK;
    } else {
        QCC_LogError(ER_FAIL, ("write failed (%d)", errno));
        return ER_FAIL;
    }
}