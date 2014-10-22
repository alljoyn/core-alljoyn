/**
 * @file
 *
 * Implemenation of StreamPump.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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

#include <vector>
#include <qcc/StreamPump.h>
#include <qcc/Event.h>
#include <qcc/ManagedObj.h>

#include <Status.h>

#define QCC_MODULE "STREAM"

using namespace std;
using namespace qcc;

StreamPump::StreamPump(Stream* streamA, Stream* streamB, size_t chunkSize, const char* name, bool isManaged) :
    Thread(name), streamA(streamA), streamB(streamB), chunkSize(chunkSize), isManaged(isManaged)
{
    /* Keep the object alive until Run exits */
    if (isManaged) {
        ManagedObj<StreamPump>::wrap(this).IncRef();
    }
}

QStatus StreamPump::Start(void* arg, ThreadListener* listener)
{
    QStatus status = Thread::Start(arg, listener);
    if ((status != ER_OK) && isManaged) {
        ManagedObj<StreamPump>::wrap(this).DecRef();
    }
    return status;
}

ThreadReturn STDCALL StreamPump::Run(void* args)
{
    // TODO: Make sure streams are non-blocking

    Event& streamASrcEv = streamA->GetSourceEvent();
    Event& streamBSrcEv = streamB->GetSourceEvent();
    Event& streamASinkEv = streamA->GetSinkEvent();
    Event& streamBSinkEv = streamB->GetSinkEvent();
    size_t bToAOffset = 0;
    size_t aToBOffset = 0;
    size_t bToALen = 0;
    size_t aToBLen = 0;
    uint8_t* aToBBuf = new uint8_t[chunkSize];
    uint8_t* bToABuf = new uint8_t[chunkSize];

    QStatus status = ER_OK;
    while ((status == ER_OK) && !IsStopping()) {
        vector<Event*> checkEvents;
        vector<Event*> sigEvents;
        checkEvents.push_back((aToBOffset == aToBLen) ? &streamASrcEv : &streamBSinkEv);
        checkEvents.push_back((bToAOffset == aToBLen) ? &streamBSrcEv : &streamASinkEv);
        status = Event::Wait(checkEvents, sigEvents);
        if (status == ER_OK) {
            for (size_t i = 0; i < sigEvents.size(); ++i) {
                if (sigEvents[i] == &streamASrcEv) {
                    status = streamA->PullBytes(aToBBuf, chunkSize, aToBLen, 0);
                    if (status == ER_OK) {
                        status = streamB->PushBytes(aToBBuf, aToBLen, aToBOffset);
                        if (status != ER_OK) {
                            QCC_LogError(status, ("Stream::PushBytes failed"));
                        }
                    } else if (status == ER_EOF) {
                        status = ER_OK;
                    } else {
                        QCC_LogError(status, ("Stream::PullBytes failed"));
                    }
                } else if (sigEvents[i] == &streamBSinkEv) {
                    size_t r;
                    status = streamB->PushBytes(aToBBuf + aToBOffset, aToBLen - aToBOffset, r);
                    if (status == ER_OK) {
                        aToBOffset += r;
                    } else {
                        QCC_LogError(status, ("Stream::PushBytes failed"));
                    }
                } else if (sigEvents[i] == &streamBSrcEv) {
                    status = streamB->PullBytes(bToABuf, chunkSize, bToALen, 0);
                    if (status == ER_OK) {
                        status = streamA->PushBytes(bToABuf, bToALen, bToAOffset);
                        if (status != ER_OK) {
                            QCC_LogError(status, ("Stream::PushBytes failed"));
                        }
                    } else if (status == ER_EOF) {
                        status = ER_OK;
                    } else {
                        QCC_LogError(status, ("Stream::PullBytes failed"));
                    }
                } else if (sigEvents[i] == &streamASinkEv) {
                    size_t r;
                    status = streamA->PushBytes(bToABuf + bToAOffset, bToALen - bToAOffset, r);
                    if (status == ER_OK) {
                        bToAOffset += r;
                    } else {
                        QCC_LogError(status, ("Stream::PushBytes failed"));
                    }
                }
                if (aToBOffset == aToBLen) {
                    aToBOffset = aToBLen = 0;
                }
                if (bToAOffset == bToALen) {
                    bToAOffset = bToALen = 0;
                }
            }
        }
    }
    delete[] aToBBuf;
    delete[] bToABuf;
    if (isManaged) {
        ManagedObj<StreamPump>::wrap(this).DecRef();
    }
    return (ThreadReturn) ER_OK;
}
