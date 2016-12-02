/**
 * @file
 *
 * Custom allocator for STL container that will securely delete
 * contents on memory deletion.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

/**
 * Append the contents of a string to a vector<uint8_t, SecureAllocator<uint8_t> >.
 *
 * @param str   String to be added.
 * @param v     Vector to be added to.
 */
void AJ_CALL AppendStringToSecureVector(const qcc::String& str, std::vector<uint8_t, SecureAllocator<uint8_t> >& v);

}

#endif // _QCC_SECUREALLOCATOR_H