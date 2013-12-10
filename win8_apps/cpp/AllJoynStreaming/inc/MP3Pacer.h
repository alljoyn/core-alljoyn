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
#include "MediaPacer.h"
#include "MP3Reader.h"

namespace AllJoynStreaming {

ref class MediaPacer;
ref class MP3Reader;
ref class MediaDescription;

/// <summary>
/// A Class on top of <c>MediaStream</c>. It creates a MediaStream instance and provides event handlers for RequestFrames and JitterMiss which are
/// triggered by <c>MediaStream</c>. The RequestFrames event handler handles the request of audio frames by fetching data from MP3Reader and writing to the socket
/// </summary>
/// <remarks>
/// The purpose of this Class to get AllJoyn media streaming working on Windows JavaScript. In WinJS, event handlers are running on the UI thread.
/// So the event handlers should not block wait on UI thread. However, the operation of RequestFrames handler requires to wait on the thread to get audio frames from
/// MP3Reader and write to network socket. To address this problem, <c>MP3Pacer</c> is provided to handle the events internally and do not expose the events across ABI.
/// </remarks>
public ref class MP3Pacer sealed {
  public:
    /// <summary>
    /// MP3Pacer Constructor
    /// </summary>
    /// <param name="mp3Reader">The MP3 stream reader that reads audio samples from audio files in response to audio sample requests.</param>
    /// <param name="jitter">Allowed clock mismatch.</param>
    MP3Pacer(MP3Reader ^ mp3Reader, uint32_t jitter);

    /// <summary>
    /// Start the thread to pace the pumping of audio stream
    /// </summary>
    /// <param name="socket"> The <c>SocketStream</c> object got from an AllJoyn raw session </param>
    /// <param name="timestamp"> The absolute time point of audio stream from which the pacer pumps data samples </param>
    /// <param name="prefill"> Minimum data in millisecond unit sent to the SocketStream before starting pacing the data pumping speed </param>
    void Start(AllJoyn::SocketStream ^ socket, uint32_t timestamp, uint32_t prefill);

    /// <summary>
    /// Stop the pacing thread that controls the pumping of the MP3 audio stream
    /// </summary>
    void Stop();

    /// <summary>
    /// Whether the pacer is running or not
    /// </summary>
    bool IsRunning();

    /// <summary>
    /// The <c>MediaPacer</c> object created by MP3Pacer
    /// </summary>
    property MediaPacer ^ Pacer;

  private:
    ~MP3Pacer();
    void MP3PacerRequestFramesHandler(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t maxFrames, Platform::WriteOnlyArray<uint32_t> ^ gotFrames);
    void MP3PacerJitterMissHandler(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t jitter);

    MP3Reader ^ _mp3Reader;
    uint8_t* _byteArray;
};

}
