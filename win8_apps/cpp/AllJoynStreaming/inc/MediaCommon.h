/**
 * @file
 * This file defines common definitions required by the classes MediaSink and MediaSource.
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
#include <qcc/String.h>

namespace AllJoynStreaming {

[Platform::Metadata::Flags]
public enum class MediaSeekPosition : uint32_t {
    /// <summary>This flag indicates a media stream supports seeking to the start.</summary>
    TO_START = 0x01,
    /// <summary>This flag indicates a media stream supports seeking to an arbitrary position. This implies support for seeking the start of the stream.</summary>
    TO_POSITION = 0x03,
    /// <summary>This flag indicates a media stream supports relative seeking forwards.</summary>
    FORWARDS = 0x04,
    /// <summary>This flag indicates a media stream supports relative seeking backwards.</summary>
    BACKWARDS = 0x08,
};

/// <summary>
/// Specfies the unit for seek operations. Many of these units are media or media stream specific.
/// When seeking to an absolute position the new postion is a number of units relative to the start
/// of the stream. When seeking to a relative poistion the new position is a numer of units, postive
/// or negative, relative to the current position in the stream.
/// </summary>
[Platform::Metadata::Flags]
public enum class MediaSeekUnits : uint32_t {
    /// <summary>Seek to a position or offset expressed in millseconds</summary>
    MILLISECONDS = 0,
    /// <summary>Seek to a position or offset expressed in seconds</summary>
    SECONDS = 1,
    /// <summary>Seek to a position or offset expressed in bytes (octets)</summary>
    BYTES = 2,
    /// <summary>Seek to a position or offset expressed in media-specific frames</summary>
    FRAMES = 3,
    /// <summary>Seek to a position or offset expressed in media-stream specific tracks </summary>
    TRACKS = 4,
    /// <summary>Seek to a position or offset expressed in media-stream specific pages</summary>
    PAGES = 5,
    /// <summary>Seek to a position or offset expressed in media-stream specific chapters</summary>
    CHAPTERS = 6,
    /// <summary>Seek to a position or offset expressed in media-stream specific indexes or bookmarks</summary>
    INDEX = 7,
};

[Platform::Metadata::Flags]
public enum class MediaType : uint32_t {
    /// <summary>Corresponds to mimetypes audio</summary>
    AUDIO = 0,
    /// <summary>Corresponds to mimetypes video</summary>
    VIDEO = 1,
    /// <summary>Corresponds to mimetypes video</summary>
    IMAGE = 2,
    /// <summary>Corresponds to mimetypes video</summary>
    APPLICATION = 3,
    /// <summary>Corresponds to mimetypes application</summary>
    TEXT = 4,
    /// <summary>Used for unknown mimetypes</summary>
    OTHER = 5,
};

/// <summary>
/// A MediaDescription contains properties that describes a media stream
/// </summary>
public ref class MediaDescription sealed {
  public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="mimeType">The mime type for the media.</param>
    /// <param name="size">The size of the media file if known.</param>
    /// <param name="pausable">Can the media be paused.</param>
    /// <param name="seekable">Bit map of seek capabilities of the media (0 means no seeking).</param>
    MediaDescription(Platform::String ^ mimeType, uint64_t size, bool pausable, uint8_t seekable);

    /// <summary>
    /// Initializer for an audio media type.
    /// </summary>
    /// <param name="samplesPerFrame">The number of samples per frame (e.g. 1152 for MP3).</param>
    /// <param name="sampleFrequency">The sampling frequency (e.g. 44.1KHz for CD audio).</param>
    /// <param name="bitRate">The encoded audio bit rate if known, zero if variable or unknown.</param>
    void SetAudioProperties(uint32_t samplesPerFrame, uint32_t sampleFrequency, uint32_t bitRate);

    /// <summary>
    /// Initializer for a video media type.
    /// </summary>
    /// <param name="frameRate">The number of frames per second</param>
    /// <param name="width">The horizontal screen resolution</param>
    /// <param name="height">The vertical screen resolution</param>
    /// <param name="aspectRatio"> A pair of number specifying the aspect ratio, e.g. {16, 9} -> 16:9</param>
    /// <param name="bitRate">The encoded video bit rate if known, zero if variable or unknown.</param>
    void SetVideoProperties(uint32_t frameRate, uint16_t width, uint16_t height, const Platform::Array<uint8_t> ^ aspectRatio, uint32_t bitRate);

    /// <summary>
    /// Initializer for an image media type.
    /// </summary>
    /// <param name="width">The horizontal screen resolution</param>
    /// <param name="height">The vertical screen resolution</param>
    void SetImageProperties(uint16_t width, uint16_t height);

    /// <summary>The media file size in octets if known, otherwise 0.</summary>
    property uint64_t Size;

    /// <summary>The mime type for the media stream</summary>
    property Platform::String ^ MimeType;

    /// <summary>Indicates if this stream can be paused.</summary>
    property bool Pausable;

    /// <summary>Bit map of seek capabilities of this media (0 means no seeking)</summary>
    property uint8_t Seekable;

    /// <summary>Video and Audio only, the encoded bit rate if known.</summary>
    property uint32_t BitRate;

    /// <summary>Audio only - The sampling frequency (e.g. 44.1KHz for CD audio).</summary>
    property uint32_t SampleFrequency;

    /// <summary>Audio only - The number of samples per frame (e.g. 1152 for MP3).</summary>
    property uint32_t SamplesPerFrame;

    /// <summary>Coarse-grained media type derived from the mimetype</summary>
    property MediaType MType;

    /// <summary>Video only - the number of frames per second</summary>
    property uint32_t FrameRate;

    /// <summary>the horizontal screen resolution in pixels</summary>
    property uint32_t Width;

    /// <summary>The vertical screen resolution in pixels</summary>
    property uint32_t Height;

    /// <summary>Video only - the aspect ratio, e.g. 16:9 or 4:3</summary>
    property uint8_t AspectRatio0;
    property uint8_t AspectRatio1;

    /// <summary>A name for the media stream</summary>
    property Platform::String ^ StreamName;

  private:

    friend ref class MediaSink;
    /**
     * Helper function to get the AllJoyn media type for a given mimetype.
     *
     * @param mimeType The mimetype to resolve.
     *
     * @return a coarse-grained media type derived from the mimetype
     */
    static MediaType ResolveMediaType(const qcc::String& mimeType);
    /**
     * Default constructor.
     */
    MediaDescription() { }
};

/// <summary>
/// Common helper functions for media streaming.
/// </summary>
public ref class MediaCommon sealed {
  public:
    /// <summary>
    /// This fuction will initialize the following AllJoyn interfaces with the bus
    ///  - org.alljoyn.MediaSink
    ///  - org.alljoyn.MediaSource
    ///  - org.alljoyn.MediaStream
    ///  - org.alljoyn.MediaStream.Video
    ///	 - org.alljoyn.MediaStream.Audio
    ///  - org.alljoyn.MediaStream.Image
    /// </summary>
    /// <param name="bus">The bus attachment the interfaces are being registered on.</param>
    static void CreateInterfaces(AllJoyn::BusAttachment ^ bus);

    /// <summary>
    /// Get the interfaceDescription object for org.alljoyn.MediaSink. It is comprised of signals for notification of the media sink's state
    /// </summary>
    static AllJoyn::InterfaceDescription ^ GetSinkIfc() { return mediaSink; }

    /// <summary>
    /// Get the interfaceDescription object for org.alljoyn.MediaSource. It does not contain any method or signals. Instead it is a placeholder for children BusObjects
    /// </summary>
    static AllJoyn::InterfaceDescription ^ GetSourceIfc() { return mediaSource; }

    /// <summary>
    /// Get the interfaceDescription object for org.alljoyn.MediaStream. It contains methods to control the state of the meida stream and the basic properites
    /// </summary>
    static AllJoyn::InterfaceDescription ^ GetStreamIfc() { return mediaStream; }

    /// <summary>
    /// t the interfaceDescription object for org.alljoyn.MediaStream.Audio. It constains additional properties of an audio media
    /// </summary>
    static AllJoyn::InterfaceDescription ^ GetAudioIfc() { return audioProps; }

    /// <summary>
    /// Get the interfaceDescription object for org.alljoyn.MediaStream.Video. It constains additional properties of a video media
    /// </summary>
    static AllJoyn::InterfaceDescription ^ GetVideoIfc() { return videoProps; }

    /// <summary>
    /// Get the interfaceDescription object for org.alljoyn.MediaStream.Image
    /// </summary>
    static AllJoyn::InterfaceDescription ^ GetImageIfc() { return imageProps; }

  private:
    static AllJoyn::InterfaceDescription ^ mediaSink;
    static AllJoyn::InterfaceDescription ^ mediaSource;
    static AllJoyn::InterfaceDescription ^ mediaStream;
    static AllJoyn::InterfaceDescription ^ videoProps;
    static AllJoyn::InterfaceDescription ^ audioProps;
    static AllJoyn::InterfaceDescription ^ imageProps;
};

}
