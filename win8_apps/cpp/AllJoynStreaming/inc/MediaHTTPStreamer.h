/**
 * @file
 * This file defines a class that emualates an HTTP media server.
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
#include "MediaSink.h"

namespace AllJoynStreaming {

/// <summary>Holds the current state of the HTTP media streamer.</summary>
public enum class HTTPState {
    /// <summary>Indicates fatal error</summary>
    FATAL_ERROR,
    /// <summary>Indicates streamer is listening for an incoming HTTP connection.</summary>
    HTTP_LISTEN,
    /// <summary>Indicates an HTTP GET was received (arg is uint32_t specifying the start offset for the GET).</summary>
    HTTP_GETTING,
    /// <summary>Indicates an HTTP connection was dropped.</summary>
    HTTP_DISCONNECT,
    /// <summary>Indicated the media socket has closed and the streamer is shutting down.</summary>
    SOCKET_CLOSED
};

/// <summary>Called by HTTPStreamer to notify the current state</summary>
public delegate void MediaHTTPStreamListenerStateChange(HTTPState state, Platform::Object ^ arg);

ref class MediaSink;

/// <summary>A basic HTTP server that will listen for and respond to HTTP streaming requests </summary>
public ref class MediaHTTPStreamer sealed {

  public:
    /// <summary>Constructor</summary>
    MediaHTTPStreamer(MediaSink ^ mediaSink);

    /// <summary>Start the HTTP Streamer</summary>
    /// <param name="sock">The SocketStream from where to read stream data. </param>
    /// <param name="mimeType">The MIME type of the media stream </param>
    /// <param name="length">The total length of the media </param>
    /// <param name="port">The port HTTP Streamer will listening on for connections from clients</param>
    void Start(AllJoyn::SocketStream ^ sock, Platform::String ^ mimeType, uint32_t length, uint16_t port);

    /// <summary>Stop the MediaHTTPStreamer. It will stop serving connections</summary>
    void Stop();

    /// <summary>Triggered when the state of the MediaHTTPStreamer has changed.</summary>
    event MediaHTTPStreamListenerStateChange ^ StateChange;

    /// <summary>Use the default internal handler for the event <c>StateChange</c>. By default it is true. </summary>
    /// <remarks>
    /// There is a story for usage of this property. When handling StateChange event, some blocking operations are required.
    /// For example, when the state is HTTP_GETTING, the stream has to seek to a specified position first before sending data to clients.
    /// JavaScript has single-threaded, and it does not allow blocking call in the event handler. Thus, MediaHTTPStreamer provides a default
    /// handler that internally do those blocking operations if UseDefaultStateChangedHandler is true. To provide customized handler in C#/C++ applications
    //  UseDefaultStateChangedHandler should be explicitly set false.
    /// </remarks>
    property bool UseDefaultStateChangedHandler;

  private:
    ~MediaHTTPStreamer();
    class Internal;
    Internal& _internal;
    MediaSink ^ _mediaSink;
    void DefaultMediaHTTPStreamListenerStateChangeHandler(HTTPState state, Platform::Object ^ arg);
};

}
