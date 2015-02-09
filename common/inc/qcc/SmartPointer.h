/**
 * @file SmartPointer.h
 *
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

#ifndef _ALLJOYN_SMARTPOINTER_H_
#define _ALLJOYN_SMARTPOINTER_H_

#include <qcc/platform.h>
#include <qcc/atomic.h>

namespace qcc {

template <typename T>
class SmartPointer {
  public:

    ~SmartPointer()
    {
        DecRef();
    }

    SmartPointer() : count(NULL), object(NULL) {   }

    SmartPointer(T* t) : count(new int32_t(1)), object(t)   {   }

    SmartPointer(const SmartPointer<T>& other)
        : count(other.count),
        object(other.object)
    {
        IncrementAndFetch(count);
    }

    SmartPointer<T> operator=(const SmartPointer<T>& other)
    {
        DecRef();

        object = other.object;
        count = other.count;
        IncRef();
        return *this;
    }

    SmartPointer<T> operator=(const T* other)
    {
        DecRef();

        object = other;
        count = new int32_t(0);
        IncRef();
        return *this;
    }

    const T& operator*() const { return *object; }
    const T* operator->() const { return object; }
    T* operator->() { return object; }
    T& operator*() { return *object; }


    const T* Get() const { return object; }
    T* Get() { return object; }

    /** Increment the ref count */
    void IncRef()
    {
        IncrementAndFetch(count);
    }

    /** Decrement the ref count and deallocate if necessary. */
    void DecRef()
    {
        const int32_t refs = DecrementAndFetch(count);
        if (0 == refs) {
            delete object;
            object = NULL;
            delete count;
            count = NULL;
        }
    }

  private:
    volatile int32_t* count;
    T* object;
};

}

#endif /* _ALLJOYN_SMARTPOINTER_H_ */
