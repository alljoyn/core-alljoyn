/**
 * @file
 * BTAccessor declaration for Windows
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

#ifndef _ALLJOYN_SDPRECORDBUILDER_H
#define _ALLJOYN_SDPRECORDBUILDER_H

#include <WinDef.h>
#include <stdlib.h>

namespace ajn {

// This class builds an Service Discovery Protocol record in the Bluetooth Specification
// format. At this time it does not include all attributes, only those required by AllJoyn.
class SdpRecordBuilder {
  public:
    SdpRecordBuilder() :
        recordHead(0),
        recordSize(0),
        bufferLength(0),
        sequenceDepth(0)
    {
    }

    ~SdpRecordBuilder()
    {
        free(this->recordHead);

        this->recordHead = 0;
        this->recordSize = 0;
        this->bufferLength = 0;
        this->sequenceDepth = 0;
    }

    /**
     * Copy constructor.
     * There probably isn't any need for this but it does prevent a default copy constructor
     * from being used and double freeing memory.
     *
     * @param builder The existing SdpRecordBuilder to copy.
     */
    SdpRecordBuilder(const SdpRecordBuilder& source)
    {
        *this = source;
    }

    /**
     * There probably isn't any need for the = operator but it does prevent a default copy
     * constructor from being used and double freeing memory.
     *
     * @param builder The existing SdpRecordBuilder to copy.
     */
    SdpRecordBuilder& operator=(const SdpRecordBuilder& source);

    /**
     * Return the SDP record or NULL if an error.
     * An error can result from the sequence depth being non-zero (there must be the
     * same number of begin EndSequence() calls as BeginSequence() calls.
     * The current depth of sequences can be determined by calling GetSequenceDepth().
     */
    BYTE*GetRecord() const
    {
        BYTE*returnValue = 0;

        if (0 == this->sequenceDepth) {
            returnValue = (BYTE*)this->recordHead;
        }

        return returnValue;
    }

    /**
     * Return the size of the SDP record. This can return non-zero even if GetRecord() returns
     * NULL. Use GetRecord() to check for a (probably) valid record.
     */
    size_t GetRecordSize() const
    {
        return this->recordSize;
    }

    /**
     * Add an attrbute to the SDP record.
     */
    bool AddAttribute(WORD attribute)
    {
        return this->AddDataElementUnsignedWord(attribute);
    }

    /**
     * Begin a data element sequence. Sequences may be nested up to MAX_SEQUENCE_DEPTH deep.
     * Returns true if successful or false if insufficient memory.
     */
    bool BeginSequence();

    /**
     * End a data element sequence.
     * Returns true if successful.
     */
    bool EndSequence();

    /**
     * Get the current depth of the sequence objects. This must be zero before
     * calling GetRecord().
     */
    int GetSequenceDepth()
    {
        return this->sequenceDepth;
    }

    /**
     * Add a "Nil, the null type".
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementNil()
    {
        return this->AddHeader(NIL, ONEBYTE);
    }

    /**
     * Add a "Unsigned Integer" of 1 byte in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUnsignedByte(BYTE data)
    {
        bool returnValue = this->AddHeader(UNSIGNEDINT, ONEBYTE);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Unsigned Integer" of 2 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUnsignedWord(WORD data)
    {
        bool returnValue = this->AddHeader(UNSIGNEDINT, TWOBYTES);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Unsigned Integer" of 4 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUnsignedDword(DWORD data)
    {
        bool returnValue = this->AddHeader(UNSIGNEDINT, FOURBYTES);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Unsigned Integer" of 8 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUnsignedQword(unsigned __int64 data)
    {
        bool returnValue = this->AddHeader(UNSIGNEDINT, EIGHTBYTES);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Signed twos-complement integer" of 1 byte in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementSignedByte(BYTE data)
    {
        bool returnValue = this->AddHeader(SIGNEDINT, ONEBYTE);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Signed twos-complement integer" of 2 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementSignedWord(WORD data)
    {
        bool returnValue = this->AddHeader(SIGNEDINT, TWOBYTES);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Signed twos-complement integer" of 4 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementSignedDword(DWORD data)
    {
        bool returnValue = this->AddHeader(SIGNEDINT, FOURBYTES);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "Signed twos-complement integer" of 8 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementSignedQword(DWORD data)
    {
        bool returnValue = this->AddHeader(SIGNEDINT, EIGHTBYTES);

        if (returnValue) {
            returnValue = this->Add(data, true);
        }

        return returnValue;
    }

    /**
     * Add a "UUID, a universally unique identifier" of 2 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUuid(WORD uuid)
    {
        bool returnValue = this->AddHeader(UUID, TWOBYTES);

        if (returnValue) {
            returnValue = this->Add(uuid, true);
        }

        return returnValue;
    }

    /**
     * Add a "UUID, a universally unique identifier" of 4 bytes in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUuid(DWORD uuid)
    {
        bool returnValue = this->AddHeader(UUID, FOURBYTES);

        if (returnValue) {
            returnValue = this->Add(uuid, true);
        }

        return returnValue;
    }

    /**
     * Add a "UUID, a universally unique identifier", a full 128 bits, in size.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUuid(::GUID uuid);

    /**
     * Add a "Text string". The string must be nul terminated.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementText(const char*string)
    {
        return this->Add(string, STRING);
    }

    /**
     * Add a "Boolean".
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementBoolean(bool value)
    {
        bool returnValue = this->AddHeader(BOOLEAN, ONEBYTE);

        if (returnValue) {
            returnValue = this->Add((BYTE)value, true);
        }

        return returnValue;
    }

    /**
     * Add a "URL, a uniform resource locator". The string must be nul terminated.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddDataElementUrl(const char*string)
    {
        return this->Add(string, URL);
    }

  private:
    const static int MAX_SEQUENCE_DEPTH = 16;
    const static size_t HEADER_FIELD_SIZE = sizeof(BYTE);

    // The offsets into the SDP record to the unclosed data sequences.
    size_t SequenceOffsets[MAX_SEQUENCE_DEPTH];

    /**
     * The current depth of the sequences. This is incremented once for every successful call
     * to BeginSequence() and decremented once for every successful call toe EndSequence().
     */
    int sequenceDepth;

    /**
     * This is the pointer to the beginning of the SDP record. It may change
     * during the building of the record as the buffer grows in length.
     */
    void*recordHead;

    /**
     * This is the pointer to the end of the SDP record. In general it is NOT the
     * end of the buffer.
     */
    size_t recordSize;

    /**
     * This is the length of the buffer that contains the record. It is NOT the length
     * of the SDP record.
     */
    size_t bufferLength;

    enum TypeDescriptor {
        NIL,                // 0.
        UNSIGNEDINT,        // 1.
        SIGNEDINT,          // 2.
        UUID,               // 3.
        STRING,             // 4.
        BOOLEAN,            // 5.
        DATASEQUENCE,       // 6.
        DATAALTERNATIVE,    // 7.
        URL                 // 8.
    };

    enum SizeDescriptor {
        ONEBYTE,        // Index of 0.
        TWOBYTES,       // Index of 1.
        FOURBYTES,      // Index of 2.
        EIGHTBYTES,     // Index of 3.
        SIXTEENBYTES,   // Index of 4.
        BYTESIZE,       // Index of 5. One byte which contains the size.
        WORDSIZE,       // Index of 6. Two bytes which contain the size.
        DWORDSIZE       // Index of 7. Four bytes which contain the size.
    };

    size_t GetRemainingBufferSize(void)
    {
        return this->bufferLength - this->recordSize;
    }

    /**
     * Checks to see if the existing buffer is large enough to add an object of
     * the required size. If it is not then then a larger buffer is allocated.
     * Returns true if successful or false if insufficient memory.
     */
    bool GrowBuffer(size_t requiredSize);

    /**
     * Adds the given BYTE to the end of the SDP record. If the operation fails and
     * undoHeader is true then the record size will be reduced by
     * HEADER_FIELD_SIZE.
     * Returns true if successful or false if insufficient memory.
     */
    bool Add(BYTE dataByte, bool undoHeader = false);

    /**
     * Adds the given WORD to the end of the SDP record. If the operation fails and
     * undoHeader is true then the record size will be reduced by
     * HEADER_FIELD_SIZE.
     * Returns true if successful or false if insufficient memory.
     */
    bool Add(WORD dataWord, bool undoHeader = false);

    /**
     * Adds the given DWORD to the end of the SDP record. If the operation fails and
     * undoHeader is true then the record size will be reduced by
     * HEADER_FIELD_SIZE.
     * Returns true if successful or false if insufficient memory.
     */
    bool Add(DWORD dataDword, bool undoHeader = false);

    /**
     * Adds the given unsigned __int64 to the end of the SDP record. If the operation fails and
     * undoHeader is true then the record size will be reduced by
     * HEADER_FIELD_SIZE.
     * Returns true if successful or false if insufficient memory.
     */
    bool Add(unsigned __int64 dataQword, bool undoHeader = false);

    /**
     * Adds the given string to the end of the SDP record.
     * Returns true if successful or false if insufficient memory.
     */
    bool Add(const char*string, TypeDescriptor type);

    /**
     * Set the given BYTE at the given offset.
     * Returns true if successful or false if the offset exceeds the length of the buffer.
     */
    bool Set(void*destination, BYTE dataByte);

    /**
     * Set the given WORD at the given offset.
     * Returns true if successful or false if the offset exceeds the length of the buffer.
     */
    bool Set(void*destination, WORD dataWord);

    /**
     * Set the given DWORD at the given offset.
     * Returns true if successful or false if the offset exceeds the length of the buffer.
     */
    bool Set(void*destination, DWORD dataWord);

    /**
     * Set the given unsigned __int64 at the given offset.
     * Returns true if successful or false if the offset exceeds the length of the buffer.
     */
    bool Set(void*destination, unsigned __int64 dataQword);

    /**
     * Get the BYTE at the given offset. Returns ~0 if offset is out of range.
     */
    BYTE GetByte(size_t offset);

    /**
     * Get the WORD at the given offset. Returns ~0 if offset is out of range.
     */
    WORD GetWord(size_t offset);

    /**
     * Get the DWORD at the given offset. Returns ~0 if offset is out of range.
     */
    DWORD GetDword(size_t offset);

    /**
     * Adds a data element header to the end of the SDP record.
     * Returns true if successful or false if insufficient memory.
     */
    bool AddHeader(TypeDescriptor type, SizeDescriptor size)
    {
        BYTE header = ((BYTE)type << 3) + (BYTE)size;

        return this->Add(header);
    }

    /**
     * Adds a Service Attribute with the type and size descriptor to the SDP record.
     * Returns true if successful.
     */
    bool AddAttributeAndHeader(WORD attribute, TypeDescriptor type, SizeDescriptor size);

    /**
     * Moves the data at offset until the end of the SDP record bytesToMove higher in memory.
     * Grows the buffer if required.
     * Returns true if successful or false if out of memory.
     */
    bool MoveRecords(size_t offset, size_t bytesToMove);
};

} // namespace ajn

#endif