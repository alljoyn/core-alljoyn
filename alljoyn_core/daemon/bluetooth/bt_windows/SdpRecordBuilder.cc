/**
 * @file
 * Utility class to build a SDP record.
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

#include <qcc/platform.h>
#include <assert.h>

#include "SdpRecordBuilder.h"

namespace ajn {

bool SdpRecordBuilder::BeginSequence()
{
    bool returnValue = false;

    if (this->sequenceDepth < SdpRecordBuilder::MAX_SEQUENCE_DEPTH) {
        // Assume the sequence will fit into 255 bytes. If it doesn't
        // the size descriptor will be updated during EndSequence().
        BYTE size = 0;

        this->SequenceOffsets[this->sequenceDepth] = this->recordSize;

        if (this->AddHeader(DATASEQUENCE, BYTESIZE) && this->Add(size)) {
            this->sequenceDepth++;
            returnValue = true;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::EndSequence()
{
    // When EndSequence is called we need to update the data element size descriptor of this
    // sequence. Originally a place holder was made using a BYTE. This may or may not be
    // adequate.
    // The offset into the SDP record for the current sequence attribute type descriptor is
    // at SequenceOffsets[sequenceDepth - 1].
    bool returnValue = false;

    if (this->sequenceDepth > 0) {
        // The stored offset is of the header field. The size is just beyond that.
        const size_t offset = this->SequenceOffsets[--this->sequenceDepth];

        // The sequence length does not include the header (1 byte) or size
        // field (currently 1 byte) so subtract 2.
        const size_t sequenceLength = this->GetRecordSize() - offset - 2;

        if (sequenceLength <= _UI8_MAX) {
            // Just need to update the size which fits into a single byte.
            BYTE size = (BYTE)sequenceLength;
            BYTE*destination = (BYTE*)this->recordHead + offset;

            returnValue = this->Set(destination + HEADER_FIELD_SIZE, size);
        } else {
            if (sequenceLength <= _UI16_MAX) {
                // Need to add more bytes so we have room for a WORD where we had only a BYTE
                // before.
                returnValue =
                    this->MoveRecords(offset + HEADER_FIELD_SIZE, sizeof(WORD) - sizeof(BYTE));

                if (returnValue) {
                    // The destination variable initialization cannot be moved before the
                    // MoveRecords() because MoveRecords might cause this->recordHead to be changed.
                    BYTE*destination = (BYTE*)this->recordHead + offset;
                    WORD size = (WORD)sequenceLength;
                    BYTE attribute = ((BYTE)DATASEQUENCE << 3) + (BYTE)WORDSIZE;

                    returnValue = this->Set(destination, attribute) &&
                                  this->Set(destination + HEADER_FIELD_SIZE, size);
                }
            } else {
                assert(sequenceLength <= _UI32_MAX);
                // Need to add more bytes so we have room for a DWORD where we had only a BYTE
                // before.
                returnValue =
                    this->MoveRecords(offset + HEADER_FIELD_SIZE, sizeof(DWORD) - sizeof(BYTE));

                if (returnValue) {
                    // The destination variable initialization cannot be moved before the
                    // MoveRecords() because MoveRecords might cause this->recordHead to be changed.
                    BYTE*destination = (BYTE*)this->recordHead + offset;
                    DWORD size = (DWORD)sequenceLength;
                    BYTE attribute = ((BYTE)DATASEQUENCE << 3) + (BYTE)DWORDSIZE;

                    returnValue = this->Set(destination, attribute) &&
                                  this->Set(destination + HEADER_FIELD_SIZE, size);
                }
            }
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::MoveRecords(size_t offset, size_t bytesToMove)
{
    bool returnValue = false;

    // If needed grow the buffer. Once this is accomplished there is no failure path.
    if (this->GetRecordSize() + bytesToMove < this->bufferLength ||
        this->GrowBuffer(bytesToMove)) {
        returnValue = true;
    }

    if (returnValue) {
        BYTE*source = (BYTE*)this->recordHead + offset;
        BYTE*destination = source + bytesToMove;
        size_t destinationSize = this->bufferLength - (offset + bytesToMove);
        size_t maxCount = this->GetRecordSize() - offset;

        if (0 == memmove_s(destination, destinationSize, source, maxCount)) {
            this->recordSize += bytesToMove;
        } else {
            returnValue = false;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::AddDataElementUuid(::GUID uuid)
{
    bool returnValue = this->AddHeader(UUID, SIXTEENBYTES);

    if (returnValue) {
        size_t undoSize = HEADER_FIELD_SIZE;

        if (this->Add(uuid.Data1)) {
            undoSize += sizeof(uuid.Data1);

            if (this->Add(uuid.Data2)) {
                undoSize += sizeof(uuid.Data2);

                if (this->Add(uuid.Data3)) {
                    undoSize += sizeof(uuid.Data3);

                    int i = 0;

                    do {
                        returnValue = this->Add(uuid.Data4[i]);

                        if (returnValue) {
                            undoSize += sizeof(uuid.Data4[i]);
                        }
                    } while (returnValue && ++i < _countof(uuid.Data4));
                }
            }
        }

        if (!returnValue) {
            assert(this->recordSize >= undoSize);
            this->recordSize -= undoSize;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::Add(const char*string, TypeDescriptor type)
{
    bool returnValue = false;

    if (string) {
        size_t undoSize = HEADER_FIELD_SIZE;
        // According to the Bluetooth spec do not include the nul. But Windows Phone 7 does.
        size_t length = strlen(string);

        if (length <= _UI8_MAX) {
            BYTE size = (BYTE)length;

            returnValue = this->AddHeader(type, BYTESIZE);

            if (returnValue) {
                returnValue = this->Add(size, true);
                undoSize += sizeof(size);
            }
        } else {
            if (length <= _UI16_MAX) {
                WORD size = (WORD)length;

                returnValue = this->AddHeader(type, WORDSIZE);

                if (returnValue) {
                    returnValue = this->Add(size, true);
                    undoSize += sizeof(size);
                }
            } else {
                assert(length <= _UI32_MAX);
                DWORD size = (DWORD)length;

                returnValue = this->AddHeader(type, DWORDSIZE);

                if (returnValue) {
                    returnValue = this->Add(size, true);
                    undoSize += sizeof(size);
                }
            }
        }

        // At this point either returnValue is true and undoSize is valid or if returnValue
        // is false then there is no need for undo because the failure of the
        // AddHeader() or Add() undid the adding of the header field.
        if (returnValue) {
            returnValue = this->GrowBuffer(length);

            if (returnValue) {
                memcpy_s((BYTE*)this->recordHead + this->recordSize,
                         this->GetRemainingBufferSize(),
                         string,
                         length);

                this->recordSize += length;
            } else {
                this->recordSize -= undoSize;
            }
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::GrowBuffer(size_t requiredSize)
{
    // Always grow the buffer this many bytes extra bytes in addition to those
    // actually required. This decreases the frequence of actually reallocating memory.
    static const size_t EXTRA_BUFFER_LENGTH = 64;
    bool returnValue = true;
    size_t bufferSizeRequired = this->GetRecordSize() + requiredSize;

    if (bufferSizeRequired > this->bufferLength) {
        void*const oldHead = this->recordHead;
        const size_t oldLength = this->bufferLength;

        this->bufferLength = bufferSizeRequired + EXTRA_BUFFER_LENGTH;
        this->recordHead = realloc(this->recordHead, this->bufferLength);

        if (!this->recordHead) {
            // Out of memory.
            returnValue = false;
            this->bufferLength = oldLength;
            this->recordHead = oldHead;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::Add(BYTE dataByte, bool undoHeader)
{
    bool returnValue = false;

    if (this->GrowBuffer(sizeof(dataByte))) {
        void*destination = (BYTE*)this->recordHead + this->GetRecordSize();

        this->Set(destination, dataByte);
        this->recordSize += sizeof(dataByte);
        returnValue = true;
    } else {
        if (undoHeader && this->recordSize > HEADER_FIELD_SIZE) {
            this->recordSize -= HEADER_FIELD_SIZE;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::Add(WORD dataWord, bool undoHeader)
{
    bool returnValue = false;

    if (this->GrowBuffer(sizeof(dataWord))) {
        void*destination = (BYTE*)this->recordHead + this->GetRecordSize();

        this->Set(destination, dataWord);
        this->recordSize += sizeof(dataWord);
        returnValue = true;
    } else {
        if (undoHeader && this->recordSize > HEADER_FIELD_SIZE) {
            this->recordSize -= HEADER_FIELD_SIZE;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::Add(DWORD dataDword, bool undoHeader)
{
    bool returnValue = false;

    if (this->GrowBuffer(sizeof(dataDword))) {
        void*destination = (BYTE*)this->recordHead + this->GetRecordSize();

        this->Set(destination, dataDword);
        this->recordSize += sizeof(dataDword);
        returnValue = true;
    } else {
        if (undoHeader && this->recordSize > HEADER_FIELD_SIZE) {
            this->recordSize -= HEADER_FIELD_SIZE;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::Add(unsigned __int64 dataQword, bool undoHeader)
{
    bool returnValue = false;

    if (this->GrowBuffer(sizeof(dataQword))) {
        void*destination = (BYTE*)this->recordHead + this->GetRecordSize();

        this->Set(destination, dataQword);
        this->recordSize += sizeof(dataQword);
        returnValue = true;
    } else {
        if (undoHeader && this->recordSize > HEADER_FIELD_SIZE) {
            this->recordSize -= HEADER_FIELD_SIZE;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::AddAttributeAndHeader(WORD attribute,
                                             TypeDescriptor type,
                                             SizeDescriptor size)
{
    bool returnValue = false;

    if (this->AddAttribute(attribute)) {
        BYTE header = ((BYTE)type << 3) + (BYTE)size;

        if (this->AddHeader(type, size)) {
            returnValue = true;
        } else {
            // Get rid of the attribute already in the record.
            this->recordSize -= HEADER_FIELD_SIZE;
        }
    }

    return returnValue;
}

bool SdpRecordBuilder::Set(void*destination, BYTE dataByte)
{
    bool returnValue = false;
    BYTE*byteDestination = (BYTE*)destination;
    const size_t destinationOffset = byteDestination - (BYTE*)this->recordHead;

    if (destinationOffset + sizeof(dataByte) <= this->bufferLength) {
        byteDestination[0] = dataByte;

        returnValue = true;
    }

    return returnValue;
}

bool SdpRecordBuilder::Set(void*destination, WORD dataWord)
{
    bool returnValue = false;
    BYTE*byteDestination = (BYTE*)destination;
    const size_t destinationOffset = byteDestination - (BYTE*)this->recordHead;

    if (destinationOffset + sizeof(dataWord) <= this->bufferLength) {
        // One byte at a time to avoid problems with memory alignment and get proper order.
        byteDestination[0] = (BYTE)(dataWord >>  8);
        byteDestination[1] = (BYTE)(dataWord & 0xff);

        returnValue = true;
    }

    return returnValue;
}

bool SdpRecordBuilder::Set(void*destination, DWORD dataDword)
{
    bool returnValue = false;
    BYTE*byteDestination = (BYTE*)destination;
    const size_t destinationOffset = byteDestination - (BYTE*)this->recordHead;

    if (destinationOffset + sizeof(dataDword) <= this->bufferLength) {
        // One byte at a time to avoid problems with memory alignment and get proper order.
        byteDestination[0] = (BYTE)(dataDword >> 24);
        byteDestination[1] = (BYTE)(dataDword >> 16);
        byteDestination[2] = (BYTE)(dataDword >>  8);
        byteDestination[3] = (BYTE)(dataDword & 0xff);

        returnValue = true;
    }

    return returnValue;
}

bool SdpRecordBuilder::Set(void*destination, unsigned __int64 dataQword)
{
    bool returnValue = false;
    BYTE*byteDestination = (BYTE*)destination;
    const size_t destinationOffset = byteDestination - (BYTE*)this->recordHead;

    if (destinationOffset + sizeof(dataQword) <= this->bufferLength) {
        // One byte at a time to avoid problems with memory alignment and get proper order.
        byteDestination[0] = (BYTE)(dataQword >> 56);
        byteDestination[1] = (BYTE)(dataQword >> 48);
        byteDestination[2] = (BYTE)(dataQword >> 40);
        byteDestination[3] = (BYTE)(dataQword >> 32);
        byteDestination[4] = (BYTE)(dataQword >> 24);
        byteDestination[5] = (BYTE)(dataQword >> 16);
        byteDestination[6] = (BYTE)(dataQword >>  8);
        byteDestination[7] = (BYTE)(dataQword & 0xff);

        returnValue = true;
    }

    return returnValue;
}

BYTE SdpRecordBuilder::GetByte(size_t offset)
{
    BYTE returnValue = ~0;

    if (offset + sizeof(returnValue) <= this->GetRecordSize()) {
        returnValue = *((BYTE*)this->recordHead + offset);
    }

    return returnValue;
}

WORD SdpRecordBuilder::GetWord(size_t offset)
{
    WORD returnValue = ~0;

    if (offset + sizeof(returnValue) <= this->GetRecordSize()) {
        // Use memcpy_s() to avoid problems with memory alignment.
        memcpy_s(&returnValue,
                 sizeof(returnValue),
                 ((BYTE*)this->recordHead + offset),
                 sizeof(returnValue));
    }

    return returnValue;
}

DWORD SdpRecordBuilder::GetDword(size_t offset)
{
    DWORD returnValue = ~0;

    if (offset + sizeof(returnValue) <= this->GetRecordSize()) {
        // Use memcpy_s() to avoid problems with memory alignment.
        memcpy_s(&returnValue,
                 sizeof(returnValue),
                 ((BYTE*)this->recordHead + offset),
                 sizeof(returnValue));
    }

    return returnValue;
}

SdpRecordBuilder& SdpRecordBuilder::operator=(const SdpRecordBuilder& source)
{
    // Make sure we are not assigning to ourselves otherwise very bad things happen.
    if (this != &source) {
        this->recordHead = 0;
        this->recordSize = 0;
        this->bufferLength = 0;
        this->sequenceDepth = 0;

        if (source.recordHead) {
            const size_t size = source.recordSize;

            free(this->recordHead);
            this->recordHead = malloc(size);

            if (this->recordHead) {
                memcpy_s(this->recordHead, size, source.recordHead, size);

                this->recordSize = source.recordSize;
                this->bufferLength = source.bufferLength;
                this->sequenceDepth = source.sequenceDepth;
            }
        }
    }

    return *this;
}

} // namespace ajn
