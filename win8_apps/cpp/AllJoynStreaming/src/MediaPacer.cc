// ****************************************************************************
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// ******************************************************************************

#include <qcc/time.h>
#include <algorithm>
#include <windows.h>
#include <ppltasks.h>

#include "MediaPacer.h"
#include "MediaCommon.h"
#include "Helper.h"
#include "Status.h"

namespace AllJoynStreaming {

using namespace Windows::System::Threading;
using namespace Windows::Foundation;
using namespace concurrency;

static const size_t defaultChunking = 4096;

class MediaPacer::Internal {
  public:

    Internal(MediaPacer ^ pacer) : running(false), stopEvent(INVALID_HANDLE_VALUE), pacer(pacer), socket(nullptr), prefill(0)
    {
    }

    void Start();
    void Stop();
    void Run();
    bool IsRunning();

    bool running;
    HANDLE stopEvent;
    MediaPacer ^ pacer;
    uint32_t bitRate;
    uint32_t jitter;
    bool raw;
    AllJoyn::SocketStream ^ socket;
    size_t chunking;
    double tick;
    uint32_t timestamp;
    uint32_t prefill;
};

bool MediaPacer::Internal::IsRunning()
{
    return running;
}

void MediaPacer::Internal::Start()
{
    task<void> ([this] () {
                    try {
                        ThreadPool::RunAsync(ref new WorkItemHandler([this](IAsyncAction ^ operation) {
                                                                         running = true;
                                                                         Run();
                                                                     }, Platform::CallbackContext::Any));
                    } catch (...) {
                        running = false;
                        QCC_LogError(ER_OS_ERROR, ("Creating MediaPacer thread"));
                    }
                });
}

void MediaPacer::Internal::Stop()
{
    running = false;
    if (stopEvent != INVALID_HANDLE_VALUE) {
        ::SetEvent(stopEvent);
    }
}

void MediaPacer::Internal::Run()
{
    QCC_DbgHLPrintf(("MediaPacer::Internal::Run"));
    try{
        /* Zero setting for timestamp */
        uint32_t start = timestamp;
        /* Basis relates the _internal timestamp to the free running millisecond timer */
        uint32_t basis = qcc::GetTimestamp() - timestamp;
        /* Total number of bytes or frames */
        size_t total = 0;

        stopEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        while (!IsRunning()) {
            uint32_t count = 0;
            Platform::Array<uint32_t> ^ outData = ref new Platform::Array<uint32_t>(1);
            try {
                if (raw) {
                    pacer->RequestBytes(timestamp, socket, chunking, outData);
                } else {
                    pacer->RequestFrames(timestamp, socket, chunking, outData);
                }
            } catch (Platform::Exception ^ e) {
                break;
            }
            count = outData[0];

            if (count > chunking) {
                QCC_LogError(ER_FAIL, ("More data delivered than requested"));
                break;
            }
            /* Time already elapsed completing the request */
            uint32_t elapsed = (qcc::GetTimestamp() - basis) - timestamp;
            /* Elapsed can go negative due to rounding in the timestamp computation */
            if ((int32_t)elapsed < 0) {
                elapsed = 0;
            }
            /* Time deadline for to next send */
            uint32_t deadline = static_cast<uint32_t>(count * tick + 0.5);
            /*
             * Free-run until prefill time expires
             */
            if (prefill) {
                prefill -= (prefill > deadline) ? deadline : prefill;
                if (prefill == 0) {
                    //printf("Done prefilling\n");
                }
            } else {
                if (elapsed > deadline) {
                    if (elapsed > jitter) {
                        pacer->JitterMiss(timestamp, socket, elapsed);
                    }
                } else {
                    int32_t maxWaitMs = (deadline - elapsed);
                    DWORD ret = WaitForSingleObjectEx(stopEvent, maxWaitMs, FALSE);
                    if (ret == WAIT_OBJECT_0) {
                        ::ResetEvent(stopEvent);
                    }
                    if (ret == WAIT_FAILED) {
                        QCC_DbgHLPrintf(("MediaPacer::Internal::Run Wait Fail"));
                    }

                }
            }
            /* Compute timestamp from the total to avoid cumulative rounding errors */
            total += count;
            timestamp = start + static_cast<uint32_t>(total * tick + 0.5);
        }
    } catch (Platform::Exception ^ e) {
        auto m = e->Message;
    }

    QCC_DbgHLPrintf(("MediaPacer::Internal::Run exit"));

    return;
}

void MediaPacer::Start(AllJoyn::SocketStream ^ socket, uint32_t timestamp, uint32_t prefill)
{
    QCC_DbgHLPrintf(("MediaPacer::Start"));
    QStatus status = ER_OK;
    while (true) {
        if (!_internal.IsRunning()) {
            _internal.timestamp = timestamp;
            _internal.prefill = prefill;
            _internal.socket = socket;
            _internal.Start();
            break;
        } else {
            status = ER_MEDIA_STREAM_ALREADY_STARTED;
            break;
        }
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaPacer::Stop()
{
    QCC_DbgHLPrintf(("MediaPacer::Stop"));
    _internal.Stop();
}

bool MediaPacer::IsRunning()
{
    return _internal.IsRunning();
}

MediaPacer::MediaPacer(MediaDescription ^ description, uint32_t jitter, uint32_t throttleRate) : _internal(*(new Internal(this)))
{
    /*
     * Compute the repetition time in the basic units - frames or bytes
     */
    switch (description->MType) {
    case MediaType::AUDIO:
        /* Tick is average time in milliseconds between each audio frame */
        _internal.tick = 1000.0 * description->SamplesPerFrame / description->SampleFrequency;
        _internal.raw = false;
        break;

    case MediaType::VIDEO:
        /* Tick is average time in milliseconds between each video frame */
        _internal.tick = 1000.0 / description->FrameRate;
        _internal.raw = false;
        break;

    default:
        /* Tick is average time in milliseconds between byte */
        _internal.tick = 1000.0 / throttleRate;
        _internal.raw = true;
        break;
    }
    if (jitter == 0) {
        /* No jitter given so use the time it takes to send 4 chunks */
        _internal.jitter = static_cast<uint32_t>(4 * defaultChunking * _internal.tick);
        _internal.chunking = defaultChunking;
    } else {
        /* A jitter less than the 1.5 * the tick time is unreasonable */
        _internal.jitter = std::max(jitter, static_cast<uint32_t>(1.5 * _internal.tick));
        _internal.chunking = static_cast<uint32_t>(_internal.jitter / _internal.tick);
    }
    QCC_DbgHLPrintf(("Initialized media pacer: tick=%f chunking=%d", (float)_internal.tick, _internal.chunking));
}

MediaPacer::~MediaPacer()
{
    _internal.Stop();
    delete &_internal;
}

}


