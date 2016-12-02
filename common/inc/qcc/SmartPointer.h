/**
 * @file SmartPointer.h
 *
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