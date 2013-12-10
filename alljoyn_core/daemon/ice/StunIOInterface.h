#ifndef _STUNIOINTERFACE_H
#define _STUNIOINTERFACE_H
/**
 * @file
 *
 * This file defines the STUN Parsing and Rendering interface.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include stun.h in C++ code.
#endif

#include <assert.h>
#include <iterator>
#include <string>
#include <vector>
#include <qcc/platform.h>
#include <qcc/Socket.h>
#include "ScatterGatherList.h"
#include <alljoyn/Status.h>

using namespace qcc;

/**
 * This defines the interface for rendering and parsing STUN messages and STUN
 * message attributes.
 */
class StunIOInterface {
  public:

    virtual ~StunIOInterface(void) { }

    /**
     * This method iterates over the contents of a buffer and converts the
     * contents of that buffer into an easily accessed form.  Implementations
     * of this function shall handle byte ordering conversion.  The caller to
     * this function shall provide iterators to the buffer as received from
     * the network interface.
     *
     * @param buf     IN:  A reference to pointer pointing to the beginning of
     *                     the buffer to be parsed.
     *                OUT: The pointer updated to point to octet after the last
     *                     octet consumed in the parsing.
     * @param bufSize IN:  Number of octets in buffer to be parsed.
     *                OUT: Number of octets left in buffer after parsing.
     *
     * @return Indication of whether the parsing succeeded or not.
     *         ER_BUFFER_TOO_SMALL indicates that the buffer does not contain
     *         the whole message as expected.
     */
    virtual QStatus Parse(const uint8_t*& buf, size_t& bufSize) = 0;

    /**
     * This method fills a buffer with the binary representation of the
     * implementing class for transmission over the network interface.
     * Implementations of this function shall handle byte ordering conversion.
     * The caller to this function shall provide iterators to the buffer as
     * received from the network interface.
     *
     * @param buf       IN:     A reference to a pointer pointing to the
     *                          beginning of the buffer where the message will
     *                          be rendered.
     *                  OUT:    The pointer updated to point to the next empty
     *                          location in the buffer (i.e., one past tha last
     *                          rendered octet).
     * @param bufSize   IN:     Available space in the buffer in octets.
     *                  OUT:    Remaining octets in the buffer after rendering.
     * @param sg        OUT:    A reference to a scatter-gather list where
     *                          addtional buffers may be added.
     *
     * @return Indication of whether the parsing succeeded or not.
     *         ER_BUFFER_TOO_SMALL indicates that the buffer too small to
     *         contain the complete rendering.
     */
    virtual QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const = 0;

    /**
     * This returns the number of octets required to render this object into a
     * common buffer that is passed in via the buf parameter of RenderBinary.
     * This differs from Size in that some objects may provide a separate
     * buffer of data for use in generating a scatter-gather list, in which
     * case, this would just return the number of octets that still need to be
     * placed in common buffer.
     *
     * @return Number of octets that RenderBinary will fill.
     */
    virtual size_t RenderSize(void) const = 0;

    /**
     * Computes the size of the object in octets.  This is the total size of
     * the object when rendered or parsed.
     *
     * @return Number of octets covering the entire binary representation of
     *         the object.
     */
    virtual size_t Size(void) const = 0;

    /**
     * Renders a human readable form in the output stream when built in debug
     * mode.  Does nothing in release mode.
     *
     * @return Human readable representation in a string.
     */
    virtual String ToString(void) const { return String(); }

    /**
     * Template function to read a basic integer type from a buffer that was
     * received from a network interface into a variable of that basic integer
     * type.  The buffer pointer is incremented appropriately and the buffer
     * size is decremented to indicate the number of octets remaining after
     * reading.  This function is only intended to be instantiated for 16 and
     * 32 bit integer types.
     *
     * @param ptr       A reference to a pointer to the buffer to be read.
     * @param bufSize   Number of octets available in the buffer.
     * @param data      A reference to the variable that the value will be read into.
     */
    template <typename T>
    static void ReadNetToHost(const uint8_t*& ptr, size_t& bufSize, T& data)
    {
        unsigned int i;

        assert(ptr != NULL);
        assert(bufSize >= sizeof(T));

        for (i = 0; i < sizeof(T); ++i) {
            data <<= 8;
            data |= *ptr;
            ++ptr;
        }

        bufSize -= sizeof(T);
    }

    /**
     * Template function to write a variable of a basic integer type to a
     * buffer that will be sent to a network interface.  The buffer pointer is
     * incremented appropriately and the buffer size is decremented to
     * indicate the number of octets remaining after writing.  This function
     * is only intended to be instantiated for 16 and 32 bit integer types.
     *
     * @param ptr       A reference to a pointer to the buffer that will be written to.
     * @param bufSize   Number of octets available in the buffer.
     * @param data      The value that will be written into ptr.
     * @param sg        Scatter-gather list to be updated with the new buffer size.
     */
    template <typename T>
    static void WriteHostToNet(uint8_t*& ptr, size_t& bufSize, T data, ScatterGatherList& sg)
    {
        unsigned int i;

        assert(ptr != NULL);
        assert(bufSize >= sizeof(T));

        sg.AddBuffer(ptr, sizeof(T));
        sg.IncDataSize(sizeof(T));

        for (i = sizeof(T); i > 0; --i) {
            ptr[i - 1] = static_cast<uint8_t>(data & 0xFF);
            data >>= 8;
        }

        bufSize -= sizeof(T);
        ptr += sizeof(T);
    }
};

#endif
