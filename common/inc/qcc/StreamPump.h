/**
 * @file
 *
 * StreamPump moves data bidirectionally between two Streams.
 */

/******************************************************************************
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