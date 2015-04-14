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
#include <qcc/platform.h>

#include <windows.h>
#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <qcc/time.h>

const qcc::Timespec qcc::Timespec::Zero;

/*
 * Unit tests require that the timestamp resolution must be at least as good as the
 * resolution of qcc::Sleep(), so ::QueryPerformanceCounter() is being used to generate
 * timestamps. Time sources having lower performance overhead but also low resolution,
 * such as ::GetTickCount64(), are unacceptable.
 */
static uint64_t counters_per_second = 0;
static uint64_t base_counter = 0;

void qcc::TimestampInit(void)
{
    LARGE_INTEGER value;
    ::QueryPerformanceFrequency(&value);
    counters_per_second = value.QuadPart;

    ::QueryPerformanceCounter(&value);
    base_counter = value.QuadPart;
}

void qcc::TimestampShutdown(void)
{
    counters_per_second = 0;
}

uint32_t qcc::GetTimestamp(void)
{
    return (uint32_t)qcc::GetTimestamp64();
}

uint64_t qcc::GetTimestamp64(void)
{
    /* Start timestamp values from zero, to match the Posix implementation */
    LARGE_INTEGER new_counter;
    ::QueryPerformanceCounter(&new_counter);
    uint64_t ret_val = new_counter.QuadPart - base_counter;

    /* Convert to milliseconds before dividing, to avoid losing more precision */
    ret_val *= 1000;
    ret_val /= counters_per_second;
    return ret_val;
}

uint64_t qcc::GetEpochTimestamp(void)
{
    struct __timeb64 time_buffer;
    _ftime64(&time_buffer);

    uint64_t ret_val = time_buffer.time * (uint64_t)1000;
    ret_val += time_buffer.millitm;
    return ret_val;
}

void qcc::GetTimeNow(Timespec* ts)
{
    uint64_t timestamp = qcc::GetTimestamp64();
    ts->seconds = timestamp / (uint64_t)1000;
    ts->mseconds = (uint16_t)(timestamp % 1000);
}

qcc::String qcc::UTCTime()
{
    static const char* Day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char* Month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    char buf[32];
    SYSTEMTIME systime;
    GetSystemTime(&systime);
    snprintf(buf, 32, "%s, %02d %s %04d %02d:%02d:%02d GMT",
             Day[systime.wDayOfWeek],
             systime.wDay,
             Month[systime.wMonth - 1],
             systime.wYear,
             systime.wHour,
             systime.wMinute,
             systime.wSecond);

    return buf;
}

