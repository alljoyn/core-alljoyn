/**
 * @file
 * This is a very primitive implementation of machine wide lock.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _EXCLUSIVE_FILE_H
#define _EXCLUSIVE_FILE_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>

class ExclusiveFile {

  public:

    ExclusiveFile(qcc::String fileName);

    ~ExclusiveFile();

    QStatus AcquireExclusiveLock();

    void ReleaseExclusiveLock();

    bool HasExclusiveLock() const;

    QStatus GetSize(int64_t& fileSize) const;

    QStatus Read(void* buf, size_t reqBytes, size_t& actualBytes);

    bool Truncate();

    QStatus Write(const void* buf, size_t numBytes, size_t& numSent);

  private:

    QStatus ResetFilePointer();
#if !defined(QCC_OS_GROUP_WINDOWS)
    int fd;
#else
    HANDLE handle;
#endif
    bool locked;
};

#endif