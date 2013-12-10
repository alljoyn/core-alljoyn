/**
 * @file
 * This file defines the class MediaSource.
 * A MediaSource represents an object than can deliver streaming media to a MediaSink object.
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
#include "MediaCommon.h"

namespace AllJoynStreaming {

ref class MediaStream;

/// <summary>A <c>MediaSource</c> instance represents an object than manages MediaStreams that can deliver streaming media to a MediaSink object.</summary>
public ref class MediaSource sealed {
  public:
    /// <summary> MediaSource constructor</summary>
    /// <param name="bus">The bus attachment for this object</param>
    MediaSource(AllJoyn::BusAttachment ^ bus);

    /// <summary> Add a media stream to this media source object</summary>
    /// <param name="mediaStream">The media stream to add</param>
    void AddStream(MediaStream ^ mediaStream);

    /// <summary> Remove a media stream from this media source object</summary>
    /// <param name="mediaStream">The bus attachment for this object</param>
    void RemoveStream(MediaStream ^ mediaStream);

  private:
    ~MediaSource();
    AllJoyn::QStatus Get(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<AllJoyn::MsgArg ^> ^ val);
    AllJoyn::BusObject ^ MediaSourceBusObject;
    AllJoyn::BusAttachment ^ Bus;
    class Internal;
    Internal* _internal;
};

/// <summary> Called when a request was received to open this media stream.</summary>
/// <param name="sinkSocket">An open socket to the media sink that requested to open the stream.</param>
public delegate bool MediaStreamOnOpen(AllJoyn::SocketStream ^ sinkSocket);

/// <summary> Called when a request was received to close this media stream.</summary>
public delegate void MediaStreamOnClose();

/// <summary> Called when a request was received to play this media stream.</summary>
public delegate bool MediaStreamOnPlay();

/// <summary> Called when a request was received to pause this media stream.</summary>
public delegate bool MediaStreamOnPause();

/// <summary> Called when a request was received to seek forward or backwards in this stream.</summary>
/// <param name="offset">A positive or negative offset relative to the current play-point in the stream.</param>
/// <param name="units">Specifies the units for the seek operation, for example bytes or milliseconds.</param>
public delegate bool MediaStreamOnSeekRelative(int32_t offset, MediaSeekUnits units);

/// <summary> Called when a request was received to seek to a specific point in this stream.</summary>
/// <param name="sinkSocket">A position in the stream.</param>
/// <param name="sinkSocket">Specifies the units for the seek operation, for example bytes or milliseconds.</param>
public delegate bool MediaStreamOnSeekAbsolute(uint32_t position, MediaSeekUnits units);

/// <summary> Used to interact with media content with an internal AllJoyn BusObject.</summary>
public ref class MediaStream sealed {
  public:
    /// <summary> MediaStream constructor</summary>
    /// <param name="bus">The bus attachment for this object</param>
    /// <param name="name">The name of this media stream</param>
    /// <param name="description">Parameters that describe this media stream</param>
    MediaStream(AllJoyn::BusAttachment ^ bus, Platform::String ^ name, MediaDescription ^ description);

    /// <summary>Play this stream</summary>
    void Play();

    /// <summary>Pause this stream</summary>
    void Pause();

    /// <summary>Close this stream</summary>
    void Close();

    /// <summary>Get the name of this steam</summary>
    Platform::String ^ GetStreamName();

    /// <summary>Get properties that describes this media stream</summary>
    MediaDescription ^ GetDescription();

    /// <summary>Returns true if this stream has one or more subscribers</summary>
    bool IsOpen();

    /// <summary>Called when a request was received to open this media stream</summary>
    event MediaStreamOnOpen ^ OnOpen;

    /// <summary>Called when a request was received to close this media stream</summary>
    event MediaStreamOnClose ^ OnClose;

    /// <summary>Called when a request was received to play this media stream</summary>
    event MediaStreamOnPlay ^ OnPlay;

    /// <summary>Called when a request was received to pause this media stream</summary>
    event MediaStreamOnPause ^ OnPause;

    /// <summary>Called when a request was received to seek forward or backwards in this stream</summary>
    event MediaStreamOnSeekRelative ^ OnSeekRelative;

    /// <summary>Called when a request was received to seek to a specific point in this stream</summary>
    event MediaStreamOnSeekAbsolute ^ OnSeekAbsolute;

    /// <summary>The MediaSource object</summary>
    property MediaSource ^ Source;

  private:
    ~MediaStream();
    /**
     * Get property handler
     */
    AllJoyn::QStatus Get(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<AllJoyn::MsgArg ^> ^ val);
    /**
     * Method handler for Open method call to this bus object.
     */
    void OpenHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ messag);

    /**
     * Method handler for Close method call to this bus object.
     */
    void CloseHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ messag);

    /**
     * Method handler for Play method call to this bus object.
     */
    void PlayHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ messag);

    /**
     * Method handler for Pause method call to this bus object.
     */
    void PauseHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ messag);

    /**
     * Method handler for SeekRelative method call to this bus object.
     */
    void SeekRelativeHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ messag);

    /**
     * Method handler for SeekRelative method call to this bus object.
     */
    void SeekAbsoluteHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ messag);

    class Internal;
    Internal* _internal;
    /**
     * Properties that describe this media stream.
     */
    MediaDescription ^ Description;
    friend ref class MediaSource;
    AllJoyn::BusObject ^ StreamBusObject;
    AllJoyn::BusAttachment ^ Bus;
    bool paused;
};

}
