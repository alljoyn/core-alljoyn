/**
 * @file
 *
 * Sink/Source wrapper FILE operations for windows
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <qcc/LockLevel.h>
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

QStatus qcc::FileExists(const qcc::String& fileName)
{
    if (INVALID_FILE_ATTRIBUTES == ::GetFileAttributesA(fileName.c_str())) {
        return ER_FAIL;
    } else {
        return ER_OK;
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
    if (!DuplicateHandle(GetCurrentProcess(),
                         inHandle,
                         GetCurrentProcess(),
                         &outHandle,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS)) {
        QCC_LogError(ER_OS_ERROR, ("DuplicateHandle return error=(%#x))", ::GetLastError()));
        return INVALID_HANDLE_VALUE;
    }
    return outHandle;
}

FileSource::FileSource(qcc::String fileName) : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(true), locked(false)
{
    ReSlash(fileName);
    handle = CreateFileA(fileName.c_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         INVALID_HANDLE_VALUE);

    if (INVALID_HANDLE_VALUE == handle) {
        /* Not using QCC_LogError intentionally since this can happen in normal operations. */
        QCC_DbgTrace(("CreateFile(GENERIC_READ) %s failed (%d)", fileName.c_str(), ::GetLastError()));
    }
}

FileSource::FileSource() : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(false)
{
    handle = GetStdHandle(STD_INPUT_HANDLE);

    if (NULL == handle) {
        QCC_LogError(ER_OS_ERROR, ("GetStdHandle failed (%d)", ::GetLastError()));
        handle = INVALID_HANDLE_VALUE;
    }
}

FileSource::FileSource(const FileSource& other) :
    handle((other.handle == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : DupHandle(other.handle)),
    event(&Event::alwaysSet),
    ownsHandle(true),
    locked(other.locked)
{
}

FileSource::FileSource(HANDLE osHandle, bool own) :
    handle(osHandle),
    event(&Event::alwaysSet),
    ownsHandle(own),
    locked(false)
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
    Unlock();
    if (ownsHandle && (INVALID_HANDLE_VALUE != handle)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

QStatus FileSource::GetSize(int64_t& fileSize)
{
    QStatus status = ER_OK;
    LARGE_INTEGER fileSizeLargeInt;

    if (INVALID_HANDLE_VALUE == handle) {
        return ER_INIT_FAILED;
    }

    if (!::GetFileSizeEx(handle, &fileSizeLargeInt)) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("GetFileSizeEx return error=(%#x) status=(%#x)", ::GetLastError(), status));
        return status;
    }

    fileSize = fileSizeLargeInt.QuadPart;
    return status;
}

QStatus FileSource::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    QCC_UNUSED(timeout);

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
        /* Intentionally not requesting exclusive lock since FileSource only reads */
        locked = LockFileEx(handle,
                            block ? 0 : LOCKFILE_FAIL_IMMEDIATELY,
                            0, 0, 0xFFFFFFFF, &ovl);
        if (!locked) {
            QCC_LogError(ER_OS_ERROR, ("LockFileEx failed, error=%#x", ::GetLastError()));
        }
        return locked;

    }
}

void FileSource::Unlock()
{
    if (handle != INVALID_HANDLE_VALUE && locked) {
        OVERLAPPED ovl = { 0 };
        if (!UnlockFileEx(handle, 0, 0, 0xFFFFFFFF, &ovl)) {
            QCC_LogError(ER_OS_ERROR, ("UnlockFileEx failed, error=%#x", ::GetLastError()));
            QCC_ASSERT(false);
        }
        locked = false;
    }
}

FileSink::FileSink(qcc::String fileName, Mode mode) : handle(INVALID_HANDLE_VALUE), event(&Event::alwaysSet), ownsHandle(true), locked(false)
{
    ReSlash(fileName);

    DWORD attributes = FILE_FLAG_WRITE_THROUGH;
    switch (mode) {
    case PRIVATE:
        attributes |= FILE_ATTRIBUTE_HIDDEN;
        break;

    case WORLD_READABLE:
    case WORLD_WRITABLE:
        attributes |= FILE_ATTRIBUTE_NORMAL;
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

    for (size_t end = fileName.find("\\", begin); end != String::npos; end = fileName.find("\\", begin)) {


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

    /* Create or open the file */
    handle = CreateFileA(fileName.substr(skip).c_str(),
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_ALWAYS,
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
    Unlock();
    if (INVALID_HANDLE_VALUE != handle) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

HANDLE FileSink::GetOsHandle() const
{
    return handle;
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

bool FileSink::Truncate()
{
    if (!::SetEndOfFile(handle)) {
        QCC_LogError(ER_OS_ERROR, ("SetEndOfFile failed. error=%d", ::GetLastError()));
        return false;
    }
    return true;
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
        /* Requesting exclusive lock since FileSink needs to write */
        locked = LockFileEx(handle,
                            LOCKFILE_EXCLUSIVE_LOCK | (block ? 0 : LOCKFILE_FAIL_IMMEDIATELY),
                            0, 0, 0xFFFFFFFF, &ovl);
        if (!locked) {
            QCC_LogError(ER_OS_ERROR, ("LockFileEx failed, error=%#x", ::GetLastError()));
        }
        return locked;
    }
}

void FileSink::Unlock()
{
    if (handle != INVALID_HANDLE_VALUE && locked) {
        if (!FlushFileBuffers(handle)) {
            QCC_LogError(ER_OS_ERROR, ("FlushFileBuffers failed, error=%#x", ::GetLastError()));
        }
        OVERLAPPED ovl = { };
        if (!UnlockFileEx(handle, 0, 0, 0xFFFFFFFF, &ovl)) {
            QCC_LogError(ER_OS_ERROR, ("UnlockFileEx failed, error=%#x", ::GetLastError()));
            QCC_ASSERT(false);
        }
        locked = false;
    }
}

FileSource* FileLock::GetSource()
{
    return m_source.get();
}

FileSink* FileLock::GetSink()
{
    return m_sink.get();
}

void FileLock::Release()
{
    m_source.reset();
    m_sink.reset();
}

QStatus FileLock::InitReadOnly(const char* fullFileName)
{
    m_source.reset(new FileSource(fullFileName));
    m_sink.reset();
    if (!m_source->IsValid()) {
        m_source.reset();
        return ER_EOF;
    }
    if (!m_source->Lock(true)) {
        return ER_READ_ERROR;
    }
    return ER_OK;
}

QStatus FileLock::InitReadWrite(std::shared_ptr<FileSink> sink)
{
    QCC_ASSERT(sink.get() != nullptr);

    /* This assert fires off if there is a recursive attempt to acquire the write lock */
    QCC_ASSERT(sink.get() != m_sink.get());

    if (!sink->IsValid()) {
        m_source.reset();
        m_sink.reset();
        return ER_EOF;
    }

    /* Reset the file pointer to the beginning */
    HANDLE handle = sink->GetOsHandle();
    if (INVALID_HANDLE_VALUE == handle) {
        QCC_LogError(ER_OS_ERROR, ("FileLock::InitReadWrite failed - invalid file handle"));
        return ER_OS_ERROR;
    }

    if (::SetFilePointer(handle, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        QCC_LogError(ER_OS_ERROR, ("SetFilePointer failed. error=%d", ::GetLastError()));
        return ER_OS_ERROR;
    }

    HANDLE dupHandle = DupHandle(handle);
    if (INVALID_HANDLE_VALUE == dupHandle) {
        QCC_LogError(ER_OS_ERROR, ("DupHandle failed - invalid file handle returned"));
        return ER_OS_ERROR;
    }

    /* Initialize both Source and Sink (for R/W) */
    m_source.reset(new FileSource(dupHandle, true));
    m_sink = sink;
    return ER_OK;
}

FileLocker::FileLocker(const char* fullFileName) : m_fileName(fullFileName), m_sinkLock(LOCK_LEVEL_FILE_LOCKER)
{
}

FileLocker::~FileLocker()
{
}

const char* FileLocker::GetFileName() const
{
    return m_fileName.c_str();
}

bool FileLocker::HasWriteLock() const
{
    QCC_VERIFY(ER_OK == m_sinkLock.Lock(MUTEX_CONTEXT));
    bool locked = (m_sink.get() != nullptr);
    m_sinkLock.Unlock(MUTEX_CONTEXT);
    return locked;
}

QStatus FileLocker::GetFileLockForRead(FileLock* fileLock)
{
    QCC_VERIFY(ER_OK == m_sinkLock.Lock(MUTEX_CONTEXT));
    std::shared_ptr<FileSink> sink = m_sink;
    m_sinkLock.Unlock(MUTEX_CONTEXT);

    if (sink.get() == nullptr) {
        /* Read requested while we don't have exclusive access; get the shared read lock. */
        return fileLock->InitReadOnly(m_fileName.c_str());
    } else {
        /* We have the write lock (m_sink not null), use that handle to return the read lock. */
        return fileLock->InitReadWrite(sink);
    }
}

QStatus FileLocker::GetFileLockForWrite(FileLock* fileLock)
{
    QCC_VERIFY(ER_OK == m_sinkLock.Lock(MUTEX_CONTEXT));
    std::shared_ptr<FileSink> sink = m_sink;
    m_sinkLock.Unlock(MUTEX_CONTEXT);

    if (sink.get() == nullptr) {
        /* Write requested while we don't have exclusive access; error. */
        return ER_BUS_NOT_ALLOWED;
    } else {
        /* We have the write lock (m_sink not null), use that handle to return a copy of the write lock. */
        return fileLock->InitReadWrite(sink);
    }
}

QStatus FileLocker::AcquireWriteLock()
{
    /* First try to acquire the local mutex (sinkLock) before the global Write lock. */
    QCC_VERIFY(ER_OK == m_sinkLock.Lock(MUTEX_CONTEXT));

    /* If this assert fires, it means there's a recursive request to lock. */
    QCC_ASSERT(m_sink.get() == nullptr);
    m_sink.reset();

    std::shared_ptr<FileSink> sink = std::make_shared<FileSink>(m_fileName, FileSink::PRIVATE);
    if (!sink->IsValid()) {
        m_sinkLock.Unlock(MUTEX_CONTEXT);
        return ER_EOF;
    }

    /* Increment the sink ref count while still under the local sinkLock. */
    m_sink = sink;

    /* Release sinkLock in preparation for acquiring the file lock. */
    m_sinkLock.Unlock(MUTEX_CONTEXT);

    /* Try to acquire the file lock. */
    if (!sink->Lock(true)) {
        /* Failed to acquire the file lock, release the sink ref count (under sinkLock). */
        QCC_VERIFY(ER_OK == m_sinkLock.Lock(MUTEX_CONTEXT));
        m_sink.reset();
        m_sinkLock.Unlock(MUTEX_CONTEXT);
    }

    return ER_OK;
}

void FileLocker::ReleaseWriteLock()
{
    QCC_VERIFY(ER_OK == m_sinkLock.Lock(MUTEX_CONTEXT));
    m_sink.reset();
    m_sinkLock.Unlock(MUTEX_CONTEXT);
}
