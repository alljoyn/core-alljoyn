/**
 * @file
 *
 * StreamPump moves data bidirectionally between two Streams.
 */

/******************************************************************************
 *
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

#ifndef _QCC_STREAMPUMP_H
#define _QCC_STREAMPUMP_H

#include <qcc/platform.h>
#include <qcc/Stream.h>
#include <qcc/Thread.h>
#include <Status.h>

namespace qcc {

class StreamPump : public qcc::Thread {
  public:

    /** Construct a bi-directional stream pump */
    StreamPump(Stream* streamA, Stream* streamB, size_t chunkSize, const char* name = "pump", bool isManaged = false);

    /** Destructor */
    virtual ~StreamPump()
    {
        delete streamA;
        delete streamB;
    }

    /**
     * Start the data pump.
     *
     * @param arg        The one and only parameter that 'func' will be called with
     *                   (defaults to NULL).
     * @param listener   Listener to be informed of Thread events (defaults to NULL).
     *
     * @return ER_OK if successful.
     */
    QStatus Start(void* arg = NULL, ThreadListener* listener = NULL);

  protected:

    /**
     *  Worker thread used to move data from streamA to streamB
     *
     * @param arg  unused.
     */
    ThreadReturn STDCALL Run(void* arg);

  private:
    /* Private copy constructor - does nothing */
    StreamPump(const StreamPump&);
    /* Private assigment operator - does nothing */
    StreamPump& operator=(const StreamPump&);

    Stream* streamA;
    Stream* streamB;
    const size_t chunkSize;
    const bool isManaged;
};

}  /* namespace */

#endif
