/**
 * @file
 *
 * Platform specific header files that defines time related functions
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
#ifndef _QCC_TIME_H
#define _QCC_TIME_H

#include <alljoyn/Status.h>
#include <qcc/platform.h>
#include <qcc/String.h>

namespace qcc {


/**
 * Actually more than 500 million years from now but who's counting.
 */
static const uint64_t END_OF_TIME = static_cast<uint64_t>(-1);

/*
 * Forward declaration.
 */
template <typename TimeBase>
struct Timespec;

struct EpochTime; /**< Time relative to Epoch. */
struct MonotonicTime; /**< Time relative to some unspecified starting time. */

/**
 * Granularity of GetTimestamp64(), GetTimestamp() and GetTimeNow().
 *
 * GetTimestamp64() is based on ::GetTickCount64() on Windows. ::GetTickCount64()
 * typically has a 10-16 milliseconds granularity, so the result of GetTimestamp64()
 * on Windows can be up to ~15 milliseconds smaller than expected at any given time.
 */
#if defined(QCC_OS_GROUP_WINDOWS)
    #define QCC_TIMESTAMP_GRANULARITY 15
#else
    #define QCC_TIMESTAMP_GRANULARITY 0
#endif

/**
 * Gets the current time, relative to some unspecified starting time.
 *
 * @param[out] ts Timespec filled with current time, relative to some
 *                unspecified starting time.
 */
void GetTimeNow(Timespec<MonotonicTime>* ts);

/** Timespec */
template <typename TimeBase>
struct Timespec {

    uint64_t seconds;       /**< The number of seconds. */
    uint16_t mseconds;      /**< The number of milliseconds. */

    Timespec() : seconds(0), mseconds(0) { }

    /**
     * Constructs a Timespec that refers to an Epoch based time or a
     * future time relative to the current time.
     *
     * @param millis The time in milliseconds.
     */
    explicit Timespec(uint64_t millis);

    Timespec& operator+=(uint32_t ms) {
        seconds += (ms + mseconds) / 1000;
        mseconds = (uint16_t)((ms + mseconds) % 1000);
        return *this;
    }

    bool operator<(const Timespec& other) const {
        return (seconds < other.seconds) || ((seconds == other.seconds) && (mseconds < other.mseconds));
    }

    bool operator<=(const Timespec& other) const {
        return (seconds < other.seconds) || ((seconds == other.seconds) && (mseconds <= other.mseconds));
    }

    bool operator==(const Timespec& other) const {
        return (seconds == other.seconds) && (mseconds == other.mseconds);
    }

    bool operator!=(const Timespec& other) const {
        return !(*this == other);
    }

    /**
     * Gets the value of this Timespec in milliseconds.
     *
     * @return The value of this Timespec in milliseconds.
     */
    uint64_t GetMillis() const {
        return (seconds * 1000) + (uint64_t)mseconds;
    }
};

template <> inline
Timespec<EpochTime>::Timespec(uint64_t millis)
{
    seconds = millis / 1000;
    mseconds = (uint16_t)(millis % 1000);
}

template <> inline
Timespec<MonotonicTime>::Timespec(uint64_t millis)
{
    GetTimeNow(this);
    seconds += (millis + mseconds) / 1000;
    mseconds = (uint16_t)((mseconds + millis) % 1000);
}

/**
 * Gets the current time in milliseconds, relative to the first call of
 * GetTimestamp() or GetTimestamp64().
 *
 * @deprecated due to rollover every 8 days.
 *
 * @return The time in milliseconds.
 */
uint32_t GetTimestamp();

/**
 * Gets the current time in milliseconds, relative to the first call of
 * GetTimestamp() or GetTimestamp64().
 *
 * @return The time in milliseconds.
 */
uint64_t GetTimestamp64();

/**
 * Gets the current time in milliseconds since the Epoch.
 *
 * @return The time in milliseconds.
 */
uint64_t GetEpochTimestamp();

/**
 * Returns a formatted string for current UTC date and time. Format conforms to
 * RFC 1123 e.g. "Tue, 30 Aug 2011 17:01:45 GMT".
 *
 * @return The formatted date/time string.
 */
qcc::String UTCTime();

/**
 * Wrapper for mktime.
 *
 * @return The time in seconds.
 */
int64_t ConvertStructureToTime(struct tm* timeptr);

/**
 * Wrapper for gmtime.
 *
 * @param[in] timer The calendar time value to be formated into a tm structure.
 * @param tm set to the time in a tm structure.
 *
 * @return ER_OK if success else ER_FAIL.
 */
QStatus ConvertTimeToStructure(const int64_t* timer, struct tm* tm);

/**
 * Wrapper for localtime
 *
 * @param timer calendar time value to be formated into a tm structure
 * @param tm set to the Time in a tm structure
 *
 * @return ER_OK if success else ER_FAIL.
 */
QStatus ConvertToLocalTime(const int64_t* timer, struct tm* tm);

/**
 * Wrapper for strftime
 *
 * @return Number of characters placed in dest.
 */
size_t FormatTime(char* dest, size_t destSize, const char* format, const struct tm* timeptr);

}

template <typename T>
inline qcc::Timespec<T> operator+(const qcc::Timespec<T>& ts, uint32_t ms)
{
    qcc::Timespec<T> ret;
    ret.seconds = ts.seconds + (ts.mseconds + ms) / 1000;
    ret.mseconds = (uint16_t)((ts.mseconds + ms) % 1000);
    return ret;
}

template <typename T>
inline int64_t operator-(const qcc::Timespec<T>& ts1, const qcc::Timespec<T>& ts2)
{
    int64_t t1 = (int64_t) ts1.seconds;
    t1 *= 1000;
    t1 += ts1.mseconds;

    int64_t t2 = (int64_t) ts2.seconds;
    t2 *= 1000;
    t2 += ts2.mseconds;

    return t1 - t2;
}

#endif
