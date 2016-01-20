/**
 * @file
 *
 * Custom allocator for STL container that will securely delete
 * contents on memory deletion.
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
#ifndef _QCC_SECUREALLOCATOR_H
#define _QCC_SECUREALLOCATOR_H

#include <qcc/platform.h>

#include <cstddef>
#include <memory>

#include <qcc/Util.h>

namespace qcc {

/**
 * This class is an allocator that may be used by any STL container where there
 * is a need to ensure that the contained data is securely deleted when the
 * memory gets deallocated.
 */
template <class T>
class SecureAllocator : public std::allocator<T> {
  public:
    typedef std::size_t size_type;  ///< typedef conforming to STL usage
    typedef T* pointer;             ///< typedef conforming to STL usage
    typedef const T* const_pointer; ///< typedef conforming to STL usage

    /**
     * A class that enables an allocator for objects of one type to create
     * storage for another type.
     */
    template <typename U>
    struct rebind {
        typedef SecureAllocator<U> other;
    };

    /** Constructor */
    SecureAllocator() : std::allocator<T>() { }

    /** Copy Constructor */
    SecureAllocator(const SecureAllocator& a) : std::allocator<T>(a) { }

    /** Copy Constructor */
    template <typename U>
    SecureAllocator(const SecureAllocator<U>& a) : std::allocator<T>(a) { }

    /** Destructor */
    virtual ~SecureAllocator() { }

    /** Allocate memory */
    virtual pointer allocate(size_type n, const_pointer hint = 0) {
        return std::allocator<T>::allocate(n, hint);
    }

    /** Deallocate memory, but securely wipe it out first. */
    virtual void deallocate(pointer p, size_type n) {
        ClearMemory(p, n * sizeof(T));
        std::allocator<T>::deallocate(p, n);
    }
};

}

#endif // _QCC_SECUREALLOCATOR_H
