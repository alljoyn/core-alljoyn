/**
 * @file
 *
 * Toolchain specific wrapper for atomic.h
 */

/******************************************************************************
 *
 *
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
#ifndef _QCC_ATOMIC_H
#define _QCC_ATOMIC_H

#include <qcc/platform.h>

#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/atomic.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/atomic.h>
#else
#error No OS GROUP defined.
#endif

#include <stdio.h>

namespace qcc {

class atomic_int32 {
  public:
    atomic_int32() {
        atomic_int32::operator=(0);
    };

    atomic_int32(const int32_t val) {
        atomic_int32::operator=(val);
    };

    atomic_int32& operator=(int32_t val) {
        AtomicSet(&mVal, val);
        return *this;
    };

    int32_t operator++(int) {
        int32_t old = AtomicFetch(&mVal);
        IncrementAndFetch(&mVal);
        return old;
    };

    atomic_int32& operator++() {
        IncrementAndFetch(&mVal);
        return *this;
    };

    int32_t operator--(int) {
        int32_t old = AtomicFetch(&mVal);
        DecrementAndFetch(&mVal);
        return old;
    };

    atomic_int32& operator--() {
        DecrementAndFetch(&mVal);
        return *this;
    };

    explicit operator int32_t() {
        return AtomicFetch(&mVal);
    };

  private:
    atomic_int32& operator=(const atomic_int32&) = delete;
    atomic_int32& operator=(const atomic_int32&) volatile = delete;
    atomic_int32(atomic_int32&) = delete;
    atomic_int32(atomic_int32&&) = delete;

    int32_t mVal;
};

class atomic_uint32 {
  public:
    atomic_uint32() {
        atomic_uint32::operator=(0);
    };

    atomic_uint32(const uint32_t val) {
        atomic_uint32::operator=(val);
    };

    atomic_uint32& operator=(uint32_t val) {
        AtomicSet(&mVal, val);
        return *this;
    };

    uint32_t operator++(int) {
        uint32_t old = AtomicFetch(&mVal);
        IncrementAndFetch(&mVal);
        return old;
    };

    atomic_uint32& operator++() {
        IncrementAndFetch(&mVal);
        return *this;
    };

    uint32_t operator--(int) {
        uint32_t old = AtomicFetch(&mVal);
        DecrementAndFetch(&mVal);
        return old;
    };

    atomic_uint32& operator--() {
        DecrementAndFetch(&mVal);
        return *this;
    };

    explicit operator uint32_t() {
        return AtomicFetch(&mVal);
    };

  private:
    atomic_uint32& operator=(const atomic_uint32&) = delete;
    atomic_uint32& operator=(const atomic_uint32&) volatile = delete;
    atomic_uint32(atomic_int32&) = delete;
    atomic_uint32(atomic_int32&&) = delete;

    int32_t mVal;
};


class atomic_bool {
  public:
    atomic_bool() {
        atomic_bool::operator=(false);
    };

    atomic_bool(const bool val) {
        atomic_bool::operator=(val);
    };

    atomic_bool& operator=(bool val) {
        volatile int32_t v = val;
        AtomicSet(&mVal, v);
        return *this;
    };

    explicit operator bool() const {
        return AtomicFetch(&mVal);
    };

  private:
    atomic_bool& operator=(const atomic_bool&) = delete;
    atomic_bool& operator=(const atomic_bool&) volatile = delete;
    atomic_bool(atomic_bool&) = delete;
    atomic_bool(atomic_bool&&) = delete;

    int32_t mVal;
};

} //namespace qcc

#endif
