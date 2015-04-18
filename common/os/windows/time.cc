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

uint32_t qcc::GetTimestamp(void)
{
    return (uint32_t)GetTimestamp64();
}

uint64_t qcc::GetTimestamp64(void)
{
    /* Start timestamp values from zero, to match the Posix implementation */
    static const uint64_t base_count = ::GetTickCount64();
    uint64_t current_count = ::GetTickCount64();
    return (current_count - base_count);
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
    /* Don't start from zero, and return values relative to an unspecified base, to match the Posix implementation */
    uint64_t current_count = ::GetTickCount64();
    ts->seconds = current_count / (uint64_t)1000;
    ts->mseconds = (uint16_t)(current_count % 1000);
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

int64_t qcc::ConvertStructureToTime(struct tm* timeptr)
{
    return _mktime64(timeptr);
}

struct tm* qcc::ConvertTimeToStructure(const int64_t* timer) {
    return _gmtime64((__time64_t*)timer);
}

struct tm* qcc::ConvertToLocalTime(const int64_t* timer) {
    return _localtime64((__time64_t*)timer);
}

size_t qcc::FormatTime(char* strDest, size_t maxSize, const char* format, const struct tm* timeptr)
{
    return strftime(strDest, maxSize, format, timeptr);
}
