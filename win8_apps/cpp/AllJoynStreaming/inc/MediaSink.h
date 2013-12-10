/**
 * @file
 * This file defines the class MediaSink.
 * A MediaSink represents a object than can receive streaming media from a MediaSource object.
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
#include <Status.h>
#include "MediaCommon.h"

namespace AllJoynStreaming {

ref class MediaDescription;
ref class MediaRenderer;

/// <summary>The result for <c>ListStreamsAsync()</c></summary>
public ref class ListStreamResult sealed {
  public:
    /// <summary>An array of media stream descriptions</summary>
    property Platform::Array<MediaDescription ^> ^ Streams;
};

/// <summary>
/// A <c>MediaSink</c> instance represents an object than can multiplex streaming media from a MediaSource object
/// to one or more MediaRenderer instances. A MediaSink can be associated with one or more open media streams.
/// </summary>
public ref class MediaSink sealed {
  public:
    /// <summary>Constructor</summary>
    /// <param name="bus"> The bus attachment.</param>
    MediaSink(AllJoyn::BusAttachment ^ bus);

    /// <summary>Asynchronously connect to the media source at the named bus name. The source remains connected while this media sink object exists.</summary>
    /// <param name="busName"> The remote bus name for the media source.</param>
    /// <param name="sessionId"> The sessionId for the session being connected to.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ ConnectSourceAsync(Platform::String ^ busName, uint32_t sessionId);

    /// <summary>Asynchronously list the media streams offered by the currently connected media source.</summary>
    /// <param name="bus"> The bus attachment.</param>
    /// <param name="bus"> The bus attachment.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncOperation<ListStreamResult ^> ^ ListStreamsAsync();

    /// <summary>Asynchronously open a media stream. This wil be be one of the media streams listed by ListStreamsAsync().</summary>
    /// <param name="streamName">The media stream to open.</param>
    /// <param name="renderer">The media rendering object that receives notifications about the media stream.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ OpenStreamAsync(Platform::String ^ streamName, MediaRenderer ^ renderer);

    /// <summary>Check if a specific media stream is currently paused.</summary>
    /// <param name="streamName">The media stream to check.</param>
    /// <returns>True if the media stream socket is open and is paused.</returns>
    bool IsPaused(Platform::String ^ streamName);

    /// <summary>Check if a specific media stream is open.</summary>
    /// <param name="streamName">The media stream to check.</param>
    /// <returns>True if the media stream socket is open.</returns>

    bool IsOpen(Platform::String ^ streamName);

    /// <summary>Asynchronously request the remote device to start playing the currently open media streams.</summary>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ PlayAsync();

    /// <summary>Asynchronously request the remote device to stop playing the currently open media streams.</summary>
    /// <param name="drain">If true and the stream is succesfully paused drain the media socket by reading until the read blocks.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ PauseAsync(bool drain);

    /// <summary>Asynchronously seek forward or backwards in the currently open media streams.</summary>
    /// <param name="offset">Position relative to start of the stream.</param>
    /// <param name="units">Specifies the units for the seek operation, for example bytes or milliseconds.</param>
    Windows::Foundation::IAsyncAction ^ SeekRelativeAsync(int32_t offset, uint8_t units);

    /// <summary>Asynchronously seek to an absolute position in the currently open media streams.</summary>
    /// <param name="position">Position relative to start of the stream.</param>
    /// <param name="units">Specifies the units for the seek operation, for example bytes or milliseconds.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ SeekAbsoluteAsync(uint32_t position, uint8_t units);

    /// <summary>Asynchronously close all currently open media streams.</summary>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ CloseAsync();

    /// <summary>Asynchronously  close a specific media stream. Call CloseAsync() to close all open media streams.</summary>
    /// <param name="streamName">The media stream to close.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncAction ^ CloseStreamAsync(Platform::String ^ streamName);

  private:
    friend ref class MediaHTTPStreamer;
    /// <summary>Default destructor.</summary>
    ~MediaSink();

    /**
     * Request the remote device to start playing the currently open media streams.</summary>
     */
    void Play();

    /**
     * Request the remote device to stop playing the currently open media streams.
     *
     * @param drain  If true and the stream is succesfully paused drain the media socket by reading
     *               until the read blocks.
     */
    void Pause(bool drain);

    /**
     * Seek forward or backwards in the currently open media streams.
     *
     * @param offset  Offset relative to current position. Units depend on the type of media stream.
     * @param units   Specifies the units for the seek operation, for example bytes or milliseconds.
     */
    void SeekRelative(int32_t offset, uint8_t units);

    /**
     * Seek to an absolute position in the currently open media streams.
     *
     * @param position Position relative to start of the stream.
     * @param units   Specifies the units for the seek operation, for example bytes or milliseconds.
     */
    void SeekAbsolute(uint32_t position, uint8_t units);

    /**
     * Close a specific media stream. Call Close() to close all open media streams.
     *
     * @param streamName    The media stream to close.
     */
    void CloseStream(Platform::String ^ streamName);

    /**
     * Close all currently open media streams.
     */
    void Close();

    /**
     * Get the stream description associated with this socket.
     */
    void GetDescription(Platform::String ^ streamName, Platform::WriteOnlyArray<MediaDescription ^> ^ description);

    /**
     * Get the proxy object for a stream.
     *
     * @param  streamName  The stream name to get.
     */
    AllJoyn::ProxyBusObject ^ GetStreamProxy(Platform::String ^ streamName);

    /**
     * Signal handler for a StreamOpened signal from a media stream.
     *
     * @param member     The member that was called
     * @param sourcePath Object path of the media stream object that sent this signal.
     * @param msg        The method call message
     */
    void StreamOpened(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg);

    /**
     * Signal handler for a StreamClosed signal from a media stream.
     *
     * @param member     The member that was called
     * @param sourcePath Object path of the media stream object that sent this signal.
     * @param msg        The method call message
     */
    void StreamClosed(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg);

    /**
     * Signal handler for a StreamPaused signal from a media stream.
     *
     * @param member     The member that was called
     * @param sourcePath Object path of the media stream object that sent this signal.
     * @param msg        The method call message
     */
    void StreamPaused(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg);

    /**
     * Signal handler for a StreamPlay signal from a media stream.
     *
     * @param member     The member that was called
     * @param sourcePath Object path of the media stream object that sent this signal.
     * @param msg        The method call message
     */
    void StreamPlaying(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg);

    /**
     * Get the properties for a stream object.
     *
     * @param stream      Proxy bus object for the stream.
     * @param description Description of stream to be filled in from the properties.
     */
    QStatus GetStreamProperties(AllJoyn::ProxyBusObject ^ stream, MediaDescription ^ description);

    class Internal;
    Internal* _internal;
    AllJoyn::BusObject ^ MediaSinkBusObject;
    AllJoyn::BusAttachment ^ Bus;
};

/// <summary> Called when a media stream socket is opened.</summary>
/// <param name="mediaSink">The media sink object that opened the socket.</param>
/// <param name="description">A description of the media stream being opened.</param>
/// <param name="socket">The socket for the media stream that has opened.</param>
public delegate void MediaRendererOnOpen(MediaSink ^ mediaSink, MediaDescription ^ description, AllJoyn::SocketStream ^ socket);

/// <summary> Called when a media stream socket is closed.</summary>
/// <param name="socket">The socket for the media stream that has closed.</param>
public delegate void MediaRendererOnClose(AllJoyn::SocketStream ^ socket);

/// <summary> Called a media stream is paused</summary>
/// <param name="socket">The socket for the media stream that has paused.</param>
public delegate void MediaRendererOnPause(AllJoyn::SocketStream ^ socket);

/// <summary> Called a media stream starts to play after a pause or seek.</summary>
/// <param name="socket">The socket for the media stream that is now playing.</param>
public delegate void MediaRendererOnPlay(AllJoyn::SocketStream ^ socket);

/// <summary>
/// Called when seeking in a media stream. The media renderer may want to mute or pause
/// the audio until the seek is complete. OnPlay() will be called when the media seek is complete.
/// The behavior unless overridden is to call OnPause().
/// </summary>
/// <param name="socket">The socket for the media stream.</param>
public delegate void MediaRendererOnSeek(AllJoyn::SocketStream ^ socket);

/// <summary>A <c>MediaRenderer</c> gives notifications about state changes to media streams.</summary>
public ref class MediaRenderer sealed {
  public:
    friend ref class MediaSink;
    /// <summary>Default Constructor</summary>
    MediaRenderer() { }

    /// <summary>Called when a media stream socket is opened.</summary>
    event MediaRendererOnOpen ^ OnOpen;

    /// <summary>Called when a media stream socket is closed.</summary>
    event MediaRendererOnClose ^ OnClose;

    /// <summary>Called a media stream is paused</summary>
    event MediaRendererOnPause ^ OnPause;

    /// <summary>Called a media stream starts to play after a pause or seek.</summary>
    event MediaRendererOnPlay ^ OnPlay;

    /// <summary>Called when seeking in a media stream.</summary>
    event MediaRendererOnSeek ^ OnSeek;
};

}
