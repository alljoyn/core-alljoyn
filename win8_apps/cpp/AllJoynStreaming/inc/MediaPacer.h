/**
 * @file
 * This file defines a utility base class for pacing the streaming of stored media data at the
 * correct rate based on the media parameters. It can also be used to pace delivery of raw data.
 */
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

/// <summary>Requests one or more complete frames be written to the socket. This is used for pacing the delivery of frame-structured data such as MP3 audio or MPEG video.</summary>
/// <param name="timestamp">The time in millseconds this request is issued. This time is based on the time passed in to Start().</param>
/// <param name="socket">The socket the media data should be written to.</param>
/// <param name="maxFrames">The maximum number of frames to write to meet the jitter target.</param>
/// <param name="gotFrames">The number of frame received. It is a writeonly array of size 1.</param>
public delegate void MediaPacerRequestFrames(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t maxFrames, Platform::WriteOnlyArray<uint32_t> ^ gotFrames);

/// <summary>Requests one or more bytes be written to the socket. This is used for pacing delivery of raw data. </summary>
/// <param name="timestamp">The time in millseconds this request is issued. This time is based on the time passed in to Start().</param>
/// <param name="socket">The socket the media data should be written to.</param>
/// <param name="maxBytes">The maximum number of bytes to write to meet the jitter target.</param>
/// <param name="gotBytes">The number of bytes received. It is a writeonly array of size 1.</param>
public delegate void MediaPacerRequestBytes(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t maxBytes, Platform::WriteOnlyArray<uint32_t> ^ gotBytes);

/// <summary>Called if the jitter target could not be met. The media server may choose to discard frames or take other corrective action.</summary>
/// <param name="timestamp">The timestamp identifying the next frame to be sent.</param>
/// <param name="socket">The socket for the stream with the jitter miss.</param>
/// <param name="jitter">The actual jitter.</param>
public delegate void MediaPacerJitterMiss(uint32_t timestamp, AllJoyn::SocketStream ^ socket, uint32_t jitter);

ref class MediaDescription;

/// <summary>A MediaPacer delivers a media stream at a preset data rate.</summary>
public ref class MediaPacer sealed {
  public:
    /// <summary>
    /// MediaPacer Constructor
    /// </summary>
    /// <param name="description">Describes the media type and framing information.</param>
    /// <param name="jitter">Specifies the target jitter in milliseconds. This value, along with the framing
    ///                      information in the media description is used to decide how much data to request
    ///                      on each call to RequestFrame() or RequestRaw().</param>
    /// <param name="throttleRate">For bulk data such as images or text this is the bit rate (specified as
    ///                            bits per seconds) to throttle the delivery at. For continously streaming
    ///                            data such as audio and video this value is ignored and shoule be zero,
    ///	                           instead the bit rate is obtained from the media description.</param>
    MediaPacer(MediaDescription ^ description, uint32_t jitter, uint32_t throttleRate);

    /// <summary>
    /// Start requesting data.
    /// </summary>
    /// <param name="socket"> The socket the media data will be written to.</param>
    /// <param name="timestamp"> Initialize the millisecond timestamp returned in each data request.</param>
    /// <param name="prefill"> Specifies the prefill time in milliseconds. During the prefill time data is sent as fast as possible.</param>
    void Start(AllJoyn::SocketStream ^ socket, uint32_t timestamp, uint32_t prefill);

    /// <summary>
    /// Stop requesting data.
    /// </summary>
    void Stop();

    /// <summary>
    /// Whether the pacer is running or not
    /// </summary>
    bool IsRunning();

    /// <summary>Used for pacing the delivery of frame-structured data such as MP3 audio or MPEG video. Requests one or more complete frames be written to the socket.</summary>
    event MediaPacerRequestFrames ^ RequestFrames;

    /// <summary>Used for pacing delivery of raw data. Requests one or more complete bytes be written to the socket.</summary>
    event MediaPacerRequestBytes ^ RequestBytes;

    /// <summary>Called if the jitter target could not be met. The media server may choose to discard frames or take other corrective action.</summary>
    event MediaPacerJitterMiss ^ JitterMiss;

  private:
    ~MediaPacer();
    class Internal;

    /**
     * Internal data for the pacer.
     */
    Internal& _internal;
};

}
