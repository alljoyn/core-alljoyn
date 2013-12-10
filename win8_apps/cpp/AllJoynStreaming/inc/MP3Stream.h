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

#pragma once

#include <qcc/platform.h>

namespace AllJoynStreaming {

ref class MediaDescription;
ref class MP3Reader;
ref class MP3Pacer;
ref class MediaPacer;
ref class MediaStream;

/// <summary>
/// A Class on top of <c>MediaStream</c>. <c>MP3Stream</c> automatically handles the events OnOpen/OnPlay/OnPause/...
/// triggered by <c>MediaStream</c>.
/// </summary>
/// <remarks>
/// The purpose of this Class to get AllJoyn media streaming working on Windows JavaScript. In WinJS, event handlers are running on the UI thread.
/// So the event handlers should not block wait on UI thread. However, the operation of OnSeekAbsolute/OnSeekRelative handler requires to wait on the thread.
/// To address this problem, <c>MP3Stream</c> is provided to handle the events internally and do not expose the events across ABI.
/// </remarks>
public ref class MP3Stream sealed {
  public:
    MP3Stream(AllJoyn::BusAttachment ^ bus, Platform::String ^ name, MP3Reader ^ mp3Reader, MP3Pacer ^ mp3Pacer);
    /// <summary>
    /// The media stream
    /// </summary>
    property MediaStream ^ Stream;
    /// <summary>
    /// prefill time in millisecond
    /// </summary>
    property uint32_t PreFill;

  private:
    ~MP3Stream();
    bool MediaStreamOnOpenHandler(AllJoyn::SocketStream ^ sinkSocket);
    void MediaStreamOnCloseHandler();
    bool MediaStreamOnPlayHandler();
    bool MediaStreamOnPauseHandler();
    bool MediaStreamOnSeekRelativeHandler(int32_t offset, MediaSeekUnits units);
    bool MediaStreamOnSeekAbsoluteHandler(uint32_t position, MediaSeekUnits units);

    MediaPacer ^ _mediaPacer;
    MP3Reader ^ _mp3Reader;
    AllJoyn::SocketStream ^ _sinkSock;
    uint32_t _startTime;
    uint32_t _timeStamp;
};

}
