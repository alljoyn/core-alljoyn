/**
 * @file
 *
 * Define atomic read-modify-write memory options for Microsoft compiler
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _TOOLCHAIN_QCC_ATOMIC_H
#define _TOOLCHAIN_QCC_ATOMIC_H

#include <windows.h>

namespace qcc {

/**
 * Increment an int32_t and return its new value atomically.
 *
 * @param mem  Pointer to int32_t to be incremented.
 * @return  New value (after increment) of *mem
 */
inline int32_t IncrementAndFetch(volatile int32_t* mem) {
    return InterlockedIncrement(reinterpret_cast<volatile long*>(mem));
}

/**
 * Decrement an int32_t and return its new value atomically.
 *
 * @param mem  Pointer to int32_t to be decremented.
 * @return  New value (after decrement) of *mem
 */
inline int32_t DecrementAndFetch(volatile int32_t* mem) {
    return InterlockedDecrement(reinterpret_cast<volatile long*>(mem));
}

/**
 * Performs an atomic compare-and-exchange operation on the specified values.
 * It compares two specified 32-bit values and exchanges with another 32-bit
 * value based on the outcome of the comparison.
 *
 * @param mem  Pointer to int32_t to be compared and modified.
 * @param expectedValue  Expected value of *mem.
 * @param newValue  New value of *mem after calling this function, if returning true.
 * @return  true if the initial value of *mem was expectedValue, false otherwise
 */
inline bool CompareAndExchange(volatile int32_t* mem, int32_t expectedValue, int32_t newValue) {
    return (InterlockedCompareExchange(reinterpret_cast<volatile long*>(mem), newValue, expectedValue) == expectedValue);
}

/**
 * Performs an atomic compare-and-exchange operation on the specified pointer values.
 * It compares two specified pointer values and exchanges with another pointer
 * value based on the outcome of the comparison.
 *
 * @param mem  Pointer to the pointer value to be compared and modified.
 * @param expectedValue  Expected value of *mem.
 * @param newValue  New value of *mem after calling this function, if returning true.
 * @return  true if the initial value of *mem was expectedValue, false otherwise
 */
inline bool CompareAndExchangePointer(void* volatile* mem, void* expectedValue, void* newValue) {
    return (InterlockedCompareExchangePointer(mem, newValue, expectedValue) == expectedValue);
}

}

#endif