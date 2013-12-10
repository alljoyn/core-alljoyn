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

#include "MP3Pacer.h"
#include "MediaCommon.h"
#include "Helper.h"
#include "Status.h"

namespace AllJoynStreaming {

using namespace Windows::System::Threading;
using namespace Windows::Foundation;
using namespace concurrency;
using namespace Windows::Storage::Streams;

MP3Pacer::MP3Pacer(MP3Reader ^ mp3Reader, uint32_t jitter) :
    _mp3Reader(mp3Reader)
{
    _byteArray = new uint8_t[_mp3Reader->FrameLen];
    if (_byteArray == NULL) {
        QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
    }

    Pacer = ref new MediaPacer(mp3Reader->GetDescription(), jitter, 0);
    if (Pacer == nullptr) {
        QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
    }

    Pacer->RequestFrames += ref new MediaPacerRequestFrames([&] (uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t maxFrames, Platform::WriteOnlyArray<uint32_t> ^ gotFrames) {
                                                                MP3PacerRequestFramesHandler(timestamp, socket, maxFrames, gotFrames);
                                                            });

    Pacer->JitterMiss += ref new MediaPacerJitterMiss([&] (uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t jitter) {
                                                          MP3PacerJitterMissHandler(timestamp, socket, jitter);
                                                      });
}

MP3Pacer::~MP3Pacer()
{
    delete [] _byteArray;
    Pacer = nullptr;
    _mp3Reader = nullptr;
    if (Pacer->IsRunning()) {
        Pacer->Stop();
    }
    Pacer = nullptr;
}

void MP3Pacer::Start(AllJoyn::SocketStream ^ socket, uint32_t timestamp, uint32_t prefill)
{
    QCC_DbgHLPrintf(("MP3Pacer::Start"));
    Pacer->Start(socket, timestamp, prefill);
}

void MP3Pacer::Stop()
{
    QCC_DbgHLPrintf(("MP3Pacer::Stop"));
    Pacer->Stop();
}

bool MP3Pacer::IsRunning()
{
    return Pacer->IsRunning();
}

void MP3Pacer::MP3PacerRequestFramesHandler(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t maxFrames, Platform::WriteOnlyArray<uint32_t> ^ gotFrames)
{
    uint32_t count = 0;
    try{
        for (count = 0; count < maxFrames; count++) {
            Platform::ArrayReference<uint8_t> arrRef(_byteArray, _mp3Reader->FrameLen);
            DataWriter ^ writer = ref new DataWriter();
            writer->WriteBytes(arrRef);
            IBuffer ^ buf = writer->DetachBuffer();
            uint32_t len = _mp3Reader->ReadFrames(buf, 1);
            uint8_t* pos = _byteArray;
            while (len > 0) {
                Platform::Array<int32_t> ^ sent = ref new Platform::Array<int32_t>(1);
                Platform::ArrayReference<uint8_t> arrRef(pos, len);
                socket->Send(arrRef, len, sent);
                len -= sent[0];
                pos += sent[0];
            }
        }
    } catch (Platform::Exception ^ e) {
        QCC_LogError(ER_OS_ERROR, ("MP3PacerRequestFramesHandler Fail"));
    }
    gotFrames[0] = count;
}

void MP3Pacer::MP3PacerJitterMissHandler(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t jitter)
{
    QCC_DbgPrintf(("Failed to meet jitter target - actual jitter = %d\n", jitter));
}

}


