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

#include "MediaCommon.h"
#include "MediaSource.h"
#include "MediaPacer.h"
#include "MP3Pacer.h"
#include "MP3Reader.h"
#include "MP3Stream.h"
#include "Helper.h"
#include "Status.h"

namespace AllJoynStreaming {


MP3Stream::MP3Stream(AllJoyn::BusAttachment ^ bus, Platform::String ^ name, MP3Reader ^ mp3Reader, MP3Pacer ^ mp3Pacer) :
    _startTime(0),
    _timeStamp(0)
{
    PreFill = 0;
    Stream = ref new MediaStream(bus, name, mp3Reader->GetDescription());
    if (Stream == nullptr) {
        QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
    }
    _mp3Reader = mp3Reader;
    _mediaPacer = mp3Pacer->Pacer;

    Stream->OnOpen += ref new MediaStreamOnOpen([&](AllJoyn::SocketStream ^ sinkSocket) {
                                                    return MediaStreamOnOpenHandler(sinkSocket);
                                                });

    Stream->OnClose += ref new MediaStreamOnClose([&]() {
                                                      MediaStreamOnCloseHandler();
                                                  });

    Stream->OnPlay += ref new MediaStreamOnPlay([&]() {
                                                    return MediaStreamOnPlayHandler();
                                                });


    Stream->OnPause += ref new MediaStreamOnPause([&]() {
                                                      return MediaStreamOnPauseHandler();
                                                  });


    Stream->OnSeekRelative += ref new MediaStreamOnSeekRelative([&](int32_t offset, MediaSeekUnits units) {
                                                                    return MediaStreamOnSeekRelativeHandler(offset, units);
                                                                });


    Stream->OnSeekAbsolute += ref new MediaStreamOnSeekAbsolute([&](uint32_t position, MediaSeekUnits units) {
                                                                    return MediaStreamOnSeekAbsoluteHandler(position, units);
                                                                });
}

MP3Stream::~MP3Stream()
{
    Stream = nullptr;
    _mp3Reader = nullptr;
    _mediaPacer = nullptr;
}

bool MP3Stream::MediaStreamOnOpenHandler(AllJoyn::SocketStream ^ sinkSocket)
{
    _startTime = 0;
    _timeStamp = _startTime;
    _sinkSock = sinkSocket;
    return true;
}

void MP3Stream::MediaStreamOnCloseHandler()
{
    _mediaPacer->Stop();
}

bool MP3Stream::MediaStreamOnPlayHandler()
{
    try {
        if (!_mediaPacer->IsRunning()) {
            _mediaPacer->Start(_sinkSock, _timeStamp, PreFill);
            return true;
        }
    } catch (Platform::Exception ^ e) {
    }
    return false;
}

bool MP3Stream::MediaStreamOnPauseHandler()
{
    try {
        if (_mediaPacer->IsRunning()) {
            _mediaPacer->Stop();
            return true;
        }
    } catch (Platform::Exception ^ e) {
    }
    return false;
}

bool MP3Stream::MediaStreamOnSeekRelativeHandler(int32_t offset, MediaSeekUnits units)
{
    try{
        if (_mediaPacer->IsRunning()) {
            _mediaPacer->Stop();
        }
        _mp3Reader->SetPosRelative(offset, units);
        _timeStamp = _startTime + _mp3Reader->Timestamp;
        _mediaPacer->Start(_sinkSock, _timeStamp, PreFill);
        return true;
    }catch (Platform::Exception ^ e) {
    }
    return false;
}

bool MP3Stream::MediaStreamOnSeekAbsoluteHandler(uint32_t position, MediaSeekUnits units)
{
    try{
        if (_mediaPacer->IsRunning()) {
            _mediaPacer->Stop();
        }
        _mp3Reader->SetPosAbsolute(position, units);
        _timeStamp = _startTime + _mp3Reader->Timestamp;
        _mediaPacer->Start(_sinkSock, _startTime, PreFill);
        return true;
    }catch (Platform::Exception ^ e) {
    }
    return false;
}

}


