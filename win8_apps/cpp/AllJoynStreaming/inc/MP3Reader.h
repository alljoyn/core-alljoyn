/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *****************************************************************************/

#pragma once

#include <qcc/platform.h>
#include "MediaCommon.h"

namespace AllJoynStreaming {

ref class MediaDescription;

/// <summary>A class for reading and parsing out frames from an MP3 file</summary>
public ref class MP3Reader sealed {
  public:

    /// <summary>Constructor</summary>
    MP3Reader();

    /// <summary>Sets the MP3 file to be parsed.</summary>
    /// <param name="file">The MP3 file opened.</param>
    /// <returns>A handle to the async operation which can be used for synchronization.</returns>
    Windows::Foundation::IAsyncOperation<bool> ^ SetFileAsync(Windows::Storage::StorageFile ^ file);

    /// <summary>Get the media description for the MP3 file.</summary>
    /// <return>A reference to the description</return>
    MediaDescription ^ GetDescription() { return description; }

    /// <summary>Reads somes MP3 frames into a buffer</summary>
    /// <remarks>
    /// The previous contents of the buffer are cleared before the new data is written.
    /// </remarks>
    /// <return>The number of bytes read</return>
    uint32_t ReadFrames(Windows::Storage::Streams::IBuffer ^ buffer, uint32_t numFrames);

    /// <return>Returns true if the position was succesfully set, false otherwise</return>
    bool SetPosAbsolute(uint32_t position, MediaSeekUnits units);

    /// <return>True if the position was succesfully set, false otherwise</return>
    bool SetPosRelative(int32_t offset, MediaSeekUnits units);

    /// <summary>Property that provides the timestamp in milliseconds for the current position in the MP3 file</summary>
    property uint32_t Timestamp {
        uint32_t get();
    }

    /// <summary>Property that provides the length of an MP3 frame in bytes</summary>
    property uint32_t FrameLen {
        uint32_t get() { return frameLen; }
    }

  private:
    /// <summary>Destructor</summary>
    ~MP3Reader();

    // Parse to first MP3 header
    bool ParseHeader();

    // Read a byte from the file stream
    uint8_t GetByte();

    // Read bytes from the file stream
    size_t GetBytes(Windows::Storage::Streams::IBuffer ^ buffer, size_t len);

    // Maximum size in bytes of an MP3 frame including the frame header.
    static const size_t MaxFrameLen;

    // MP3 always has 1152 audio samples per frame
    static const size_t SamplesPerFrame = 1152;

    MediaDescription ^ description;

    uint32_t bitRate;
    uint32_t sampleRate;
    size_t frameLen;
    uint64_t startOffset;

    Windows::Storage::Streams::IRandomAccessStream ^ iStream; // Stream for the MP3 data
    Windows::Storage::Streams::DataReader ^ reader;           // Reader for the MP3 data
};

}

