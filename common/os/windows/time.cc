/**
 * @file
 *
 * Platform specific header files that defines time related functions
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

uint32_t qcc::GetTimestamp(void)
{
    static uint32_t base = 0;
    struct _timeb timebuffer;
    uint32_t ret_val;

    _ftime(&timebuffer);

    ret_val = ((uint32_t)timebuffer.time) * 1000;
    ret_val += timebuffer.millitm;

#ifdef RANDOM_TIMESTAMPS
    /*
     * Randomize time base
     */
    while (!base) {
        srand(ret_val);
        base = rand() | (rand() << 16);
    }
#endif

    return ret_val + base;
}

uint64_t qcc::GetTimestamp64(void)
{
    static uint32_t base = 0;
    struct _timeb timebuffer;
    uint64_t ret_val;

    _ftime(&timebuffer);

    ret_val = ((uint64_t)timebuffer.time) * 1000;
    ret_val += timebuffer.millitm;

#ifdef RANDOM_TIMESTAMPS
    /*
     * Randomize time base
     */
    while (!base) {
        srand(ret_val);
        base = rand() | (rand() << 16);
    }
#endif

    return ret_val + base;
}

uint64_t qcc::GetEpochTimestamp(void)
{
    return GetTimestamp64();
}

void qcc::GetTimeNow(Timespec* ts)
{
    struct _timeb timebuffer;

    _ftime(&timebuffer);

    ts->seconds = timebuffer.time;
    ts->mseconds = timebuffer.millitm;
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
