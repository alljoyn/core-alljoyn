/**
 * @file
 *
 * Sink implementation which stores data in a qcc::String.
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

#ifndef _QCC_STRINGSINK_H
#define _QCC_STRINGSINK_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Stream.h>

namespace qcc {

/**
 * StringSink provides Sink based storage for bytes.
 */
class StringSink : public Sink {
  public:

    /** Destructor */
    virtual ~StringSink() { }

    /**
     * Push bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent)
    {
        str.append((char*)buf, numBytes);
        numSent = numBytes;
        return ER_OK;
    }

    /**
     * Get reference to sink storage.
     * @return string used to hold Sink data.
     */
    qcc::String& GetString() { return str; }

    /**
     * Clear existing bytes from sink.
     */
    void Clear() { str.clear(); }

  private:
    qcc::String str;    /**< storage for byte stream */
};

}

#endif