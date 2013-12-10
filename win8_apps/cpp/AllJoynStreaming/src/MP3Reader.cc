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

#include <ppltasks.h>

#include "MP3Reader.h"

using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace concurrency;

namespace AllJoynStreaming {

static const uint32_t BR_MAP[] = {
    0, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 0
};

static const uint32_t FREQ_MAP[] = {
    44100, 48000, 32000, 0
};

/*
 * 4 byte mp3 frame header
 */
static const size_t HDR_LEN = 4;

/*
 * Max frame size for MP3 computed using max bit rate and minimum sample rate.
 */
const size_t MP3Reader::MaxFrameLen = (144 * 320000 / 32000 + 1 + 2);

MP3Reader::MP3Reader()
{
    uint8_t seekCaps = (uint8_t)(MediaSeekPosition::TO_START | MediaSeekPosition::TO_POSITION | MediaSeekPosition::FORWARDS | MediaSeekPosition::BACKWARDS);
    description = ref new MediaDescription("audio/mpeg", 0, true, seekCaps);
}

MP3Reader::~MP3Reader()
{
}

IAsyncOperation<bool> ^ MP3Reader::SetFileAsync(StorageFile ^ file)
{
    auto op = create_async([ = ] {
                               bool success = false;
                               if (file) {
                                   try {
                                       auto openOp = file->OpenAsync(FileAccessMode::Read);
                                       Concurrency::task<IRandomAccessStream ^> openTask(openOp);
                                       openTask.wait();
                                       iStream = openOp->GetResults();
                                       if (ParseHeader()) {
                                           description->Size = iStream->Size - startOffset;
                                           description->SetAudioProperties(SamplesPerFrame, sampleRate, bitRate);
                                           success = true;
                                       }
                                   } catch (...) {
                                   }
                               }
                               return success;
                           });
    return op;
}

uint32_t MP3Reader::ReadFrames(IBuffer ^ buffer, uint32_t numFrames)
{
    return GetBytes(buffer, numFrames * frameLen);
}

bool MP3Reader::SetPosAbsolute(uint32_t position, MediaSeekUnits units)
{
    uint64_t pos = startOffset;

    if (units == MediaSeekUnits::MILLISECONDS) {
        pos += position * (bitRate / 8000);
    } else if (units == MediaSeekUnits::SECONDS) {
        pos += position * (bitRate / 8);
    } else if (units == MediaSeekUnits::BYTES) {
        pos += position;
    } else {
        return false;
    }
    iStream->Seek(pos);
    return true;
}

bool MP3Reader::SetPosRelative(int32_t offset, MediaSeekUnits units)
{
    int64_t newPos;
    int64_t pos = iStream->Position;

    if (units == MediaSeekUnits::MILLISECONDS) {
        newPos = pos + offset * (int64_t)(bitRate / 8000);
    } else if (units == MediaSeekUnits::SECONDS) {
        newPos = pos + offset * (int64_t)(bitRate / 8);
    } else if (units == MediaSeekUnits::BYTES) {
        newPos = pos + offset;
    } else {
        return false;
    }
    if (newPos < startOffset) {
        newPos = startOffset;
    }
    iStream->Seek(newPos);
    return true;
}

uint32_t MP3Reader::Timestamp::get()
{
    uint32_t curr = iStream->Position;
    if (curr < 0) {
        return 0;
    } else {
        return (curr - startOffset) * 8000 / bitRate;
    }
    return 0;
}

byte MP3Reader::GetByte()
{
    Buffer ^ buf = ref new Buffer(1);
    GetBytes(buf, 1);
    auto rdr = DataReader::FromBuffer(buf);
    return rdr->ReadByte();
}

size_t MP3Reader::GetBytes(IBuffer ^ buf, size_t len)
{
    buf->Length = 0;
    if (len > buf->Capacity) {
        len = buf->Capacity;
    }
    try {
        IAsyncOperationWithProgress<IBuffer ^, uint32>  ^ readOp = iStream->ReadAsync(buf, len, InputStreamOptions::None);
        concurrency::task<IBuffer ^> readTask(readOp);
        readTask.wait();
        return buf->Length;
    } catch (Platform::Exception ^ pe) {
        HRESULT hr = pe->HResult;
        return 0;
    } catch (const std::exception& e) {
        return 0;
    }
}

bool MP3Reader::ParseHeader()
{
    startOffset = 0;
    bool inSynch = true;
    bool ok = true;

    iStream->Seek(0);

    while (ok) {
        byte hdr[HDR_LEN + 6];
        if (inSynch) {
            hdr[0] = GetByte();
            hdr[1] = GetByte();
            hdr[2] = GetByte();
            hdr[3] = GetByte();
        } else {
            hdr[0] = hdr[1];
            hdr[1] = hdr[2];
            hdr[2] = hdr[3];
            hdr[3] = GetByte();
        }
        /* Match 12 bit sync word and MPEG-1 layer 3 */
        if (hdr[0] == 0xFF && (hdr[1] == 0xFB || hdr[1] == 0xFA)) {
            bitRate = BR_MAP[hdr[2] >> 4];
            sampleRate = FREQ_MAP[(hdr[2] >> 2) & 0x3];
            uint32_t pad = (hdr[2] >> 1) & 1;
            if (sampleRate && bitRate) {
                frameLen = (144 * bitRate / sampleRate) + pad - HDR_LEN;
                startOffset = iStream->Position - HDR_LEN;
                iStream->Seek(startOffset);
                break;
            }
        } else if ((strncmp((char*)hdr, "ID3", 3) == 0) && (hdr[3] < 0x80)) {
            // Got an ID3 header - read enough of the header to get the length
            for (size_t i = HDR_LEN; i < sizeof(hdr); ++i) {
                hdr[i] = GetByte();
            }
            // Unpack the ID3 header length then skip over it
            size_t id3Len = ((hdr[6] & 0x7F) << 21) + ((hdr[7] & 0x7F) << 14) + ((hdr[8] & 0x7F) << 7) + (hdr[9] & 0x7F);
            iStream->Seek(sizeof(hdr) + id3Len);
        } else {
            inSynch = false;
        }
    }
    return ok;
}

}

