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

#include <qcc/platform.h>
#include <qcc/String.h>

namespace qcc {


/**
 * Actually more than 500 million years from now but who's counting.
 */
static const uint64_t END_OF_TIME = static_cast<uint64_t>(-1);

typedef enum {
    TIME_ABSOLUTE,
    TIME_RELATIVE
} TimeBase;

/*
 * Forward declaration.
 */
struct Timespec;

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
 * Get the current time.
 * @param ts  [OUT] Timespec filled with current time.
 */
void GetTimeNow(Timespec* ts);

/** Timespec */
struct Timespec {

    static const Timespec Zero;

    uint64_t seconds;       /**< Number of seconds since EPOCH */
    uint16_t mseconds;      /**< Milliseconds in EPOCH */

    Timespec() : seconds(0), mseconds(0) { }

    /**
     * Construct a Timespec that refers to an absolute (EPOCH based) time expressed in milliseconds
     * or a future time relative to the current time now.
     *
     * @param millis   Time in milliseconds.
     * @param base     Indicates if time is relative to now or absolute.
     */
    Timespec(uint64_t millis, TimeBase base = TIME_ABSOLUTE) {
        if (base == TIME_ABSOLUTE) {
            seconds = millis / 1000;
            mseconds = (uint16_t)(millis % 1000);
        } else {
            GetTimeNow(this);
            seconds += (millis + mseconds) / 1000;
            mseconds = (uint16_t)((mseconds + millis) % 1000);
        }
    }

    Timespec& operator+=(const Timespec& other) {
        seconds += other.seconds + (mseconds + other.mseconds) / 1000;
        mseconds = ((mseconds + other.mseconds) % 1000);
        return *this;
    }

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

    uint64_t GetAbsoluteMillis() const { return (seconds * 1000) + (uint64_t)mseconds; }

};

/**
 * Return (non-absolute) timestamp in milliseconds.
 * Deprecated due to rollover every 8 days.
 * @return  timestamp in milliseconds.
 */
uint32_t GetTimestamp(void);

/**
 * Return (non-absolute) timestamp in milliseconds.
 * @return  timestamp in milliseconds.
 */
uint64_t GetTimestamp64(void);

/**
 * Return (non-absolute) timestamp in milliseconds since Epoch.
 * @return  timestamp in milliseconds.
 */
uint64_t GetEpochTimestamp(void);

/**
 * Return a formatted string for current UTC date and time. Format conforms to RFC 1123
 * e.g. "Tue, 30 Aug 2011 17:01:45 GMT"
 *
 * @return  The formatted date/time string.
 */
qcc::String UTCTime();

/**
 * Wrapper for mktime
 * @return  timestamp in seconds
 */
int64_t ConvertStructureToTime(struct tm* timeptr);

/**
 * Wrapper for gmtime
 * @return  Time in a tm structure
 */
struct tm* ConvertTimeToStructure(const int64_t* timer);

/**
 * Wrapper for localtime
 * @return  Time in a tm structure
 */
struct tm* ConvertToLocalTime(const int64_t* timer);

/**
 * Wrapper for strftime
 * @return  Number of characters placed in dest
 */
size_t FormatTime(char* dest, size_t destSize, const char* format, const struct tm* timeptr);

}

inline qcc::Timespec operator+(const qcc::Timespec& tsa, const qcc::Timespec& tsb)
{
    qcc::Timespec ret;
    ret.seconds = tsa.seconds + tsb.seconds + (tsa.mseconds + tsb.mseconds) / 1000;
    ret.mseconds = (tsa.mseconds + tsb.mseconds) % 1000;
    return ret;
}

inline qcc::Timespec operator+(const qcc::Timespec& ts, uint32_t ms)
{
    qcc::Timespec ret;
    ret.seconds = ts.seconds + (ts.mseconds + ms) / 1000;
    ret.mseconds = (uint16_t)((ts.mseconds + ms) % 1000);
    return ret;
}

inline int64_t operator-(const qcc::Timespec& ts1, const qcc::Timespec& ts2)
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
