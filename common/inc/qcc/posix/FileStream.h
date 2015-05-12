/**
 * @file
 *
 * Sink/Source wrapper FILE operations
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

#ifndef _OS_QCC_FILESTREAM_H
#define _OS_QCC_FILESTREAM_H

#include <qcc/platform.h>

#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <string>

#include <qcc/String.h>
#include <qcc/Stream.h>
#include <Status.h>

namespace qcc {

/**
 * Platform abstraction for deleting a file
 *
 * @param fileName  The name of the file to delete
 *
 * @return ER_OK if the file was deleted or an error status otherwise.
 */
QStatus DeleteFile(qcc::String fileName);

/**
 * Platform abstraction for checking for the existence of a file
 *
 * @param fileName   The name of the file to check
 *
 * @return ER_OK if the file exists; ER_FAIL if not.
 */
QStatus FileExists(const qcc::String& fileName);

/**
 * FileSource is an implementation of Source used for reading from files.
 */
class FileSource : public Source {
  public:

    /**
     * Create an FileSource
     *
     * @param fileName   Name of file to read/write
     */
    FileSource(qcc::String fileName);

    /**
     * Create an FileSource from stdin
     */
    FileSource();

    /**
     * Copy constructor.
     *
     * @param other   FileSource to copy from.
     */
    FileSource(const FileSource& other);

    /**
     * Assignment.
     *
     * @param other FileSource to copy from.
     * @return This FileSource.
     */
    FileSource operator=(const FileSource& other);

    /** Destructor */
    virtual ~FileSource();

    /**
     * Return the size of the file
     *
     * @param fileSize The size of the file in bytes
     */
    QStatus GetSize(int64_t& fileSize);

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    Event& GetSourceEvent() { return *event; }

    /**
     * Check validity of FILE.
     *
     * @return  true iff stream was successfully initialized.
     */
    bool IsValid() { return 0 <= fd; }

    /**
     * Lock the underlying file for exclusive access
     *
     * @param block  If block is true the function will block until file access if permitted.
     *
     * @return Returns true if the file was locked, false if the file was not locked or if the file
     *         was not valid, i.e. if IsValid() would returnf false;
     */
    bool Lock(bool block = false);

    /**
     * Unlock the file if previously locked
     */
    void Unlock();

  private:
    int fd;        /**< File descriptor */
    Event* event;  /**< I/O event */
    bool ownsFd;   /**< true if sink is responsible for closing fd */
    bool locked;   /**< true if the sink has been locked for exclusive access */
};


/**
 * FileSink is an implementation of Sink used to write to files.
 */
class FileSink : public Sink {

  public:

    /**
     * File creation mode.
     */
    typedef enum {
        PRIVATE = 0,        /**< Private to the calling user */
        WORLD_READABLE = 1, /**< World readable */
        WORLD_WRITABLE = 2, /**< World writable */
    } Mode;

    /**
     * Create an FileSink.
     *
     * @param fileName     Name of file to use as sink.
     */
    FileSink(qcc::String fileName, Mode mode = WORLD_READABLE);

    /**
     * Create an FileSink for stdout
     */
    FileSink();

    /**
     * Copy constructor.
     *
     * @param other   FileSink to copy from.
     */
    FileSink(const FileSink& other);

    /**
     * Assignment.
     *
     * @param other FileSink to copy from.
     * @return This FileSink.
     */
    FileSink operator=(const FileSink& other);

    /** FileSink Destructor */
    virtual ~FileSink();

    /**
     * Push bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);

    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    Event& GetSinkEvent() { return *event; }

    /**
     * Check validity of FILE.
     *
     * @return  true iff stream was successfully initialized.
     */
    bool IsValid() { return 0 <= fd; }

    /**
     * Lock the underlying file for exclusive access
     *
     * @param block  If block is true the function will block until file access if permitted.
     *
     * @return Returns true if the file was locked, false if the file was not locked or if the file
     *         was not valid, i.e. if IsValid() would returnf false;
     */
    bool Lock(bool block = false);

    /**
     * Unlock the file if previously locked
     */
    void Unlock();

  private:

    int fd;        /**< File descriptor */
    Event* event;  /**< I/O event */
    bool ownsFd;   /**< true if sink is responsible for closing fd */
    bool locked;   /**< true if the sink has been locked for exclusive access */
};

}   /* namespace */

#endif
