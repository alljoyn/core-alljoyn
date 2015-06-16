/*
 * Buffer.h
 *
 *  Created on: Jun 1, 2015
 *      Author: erongo
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#ifndef BUFFER_H_
#define BUFFER_H_

#include <memory>
#include <vector>

namespace nio {

class Buffer {
  public:
    Buffer();

    Buffer(uint8_t* buf, uint32_t buflen, uint32_t capacity);

    ~Buffer();

    void Set(uint8_t* buf, uint32_t buflen, uint32_t capacity);

    uint8_t* GetBuffer();
    uint32_t GetLength();
    uint32_t GetCapacity();

//private:
    uint8_t* m_buffer;
    uint32_t m_length;
    uint32_t m_capacity;

};

} /* namespace nio */

#endif /* BUFFER_H_ */
