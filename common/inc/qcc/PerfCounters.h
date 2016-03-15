/**
 * @file
 *
 * Counters easily found from a debugger, incremented for frequent SCL actions.
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
#ifndef _QCC_PERFCOUNTERS_H
#define _QCC_PERFCOUNTERS_H

#include <qcc/Util.h>

namespace qcc {

/*
 * @internal
 * Perf counter types.
 */
typedef enum _PerfCounterIndex {
    PERF_COUNTER_ALARM_TRIGGERED = 0,

    PERF_COUNTER_THREAD_CREATED = 1,
    PERF_COUNTER_THREAD_DESTROYED = 2,

    PERF_COUNTER_UDP_TRANSPORT_RUN_OUTER_LOOP = 3,
    PERF_COUNTER_UDP_TRANSPORT_PUMP_OUTER_LOOP = 4,
    PERF_COUNTER_UDP_TRANSPORT_DISPATCHER_OUTER_LOOP = 5,
    PERF_COUNTER_UDP_TRANSPORT_PUMP_RECVCB = 6,
    PERF_COUNTER_UDP_TRANSPORT_ARDP_RUN = 7,

    PERF_COUNTER_SOCKET_SEND = 8,
    PERF_COUNTER_SOCKET_SENDTO = 9,
    PERF_COUNTER_SOCKET_RECV = 10,
    PERF_COUNTER_SOCKET_RECV_WITH_ANCILLARY_DATA = 11,
    PERF_COUNTER_SOCKET_RECV_FROM = 12,
    PERF_COUNTER_SOCKET_RECV_WITH_FDS = 13,
    PERF_COUNTER_SOCKET_SEND_WITH_FDS = 14,

    PERF_COUNTER_STRING_CREATED_1 = 15,
    PERF_COUNTER_STRING_CREATED_2 = 16,
    PERF_COUNTER_STRING_CREATED_3 = 17,
    PERF_COUNTER_STRING_CREATED_4 = 18,
    PERF_COUNTER_STRING_CREATED_5 = 19,
    PERF_COUNTER_STRING_CREATED_6 = 20,
    PERF_COUNTER_STRING_CREATED_7 = 21,
    PERF_COUNTER_STRING_CREATED_8 = 22,
    PERF_COUNTER_STRING_CREATED_9 = 23,
    PERF_COUNTER_STRING_DESTROYED = 24,

    PERF_COUNTER_IPNS_OUTER_LOOP = 25,
    PERF_COUNTER_IPNS_SEND_PROTOCOL_MESSAGE = 26,
    PERF_COUNTER_IPNS_HANDLE_PROTOCOL_MESSAGE = 27,

    /*
     * Insert new counters above this line, then update the total count below.
     * DO NOT remove or change the value of any of the existing counters,
     * because Windbg extensions depend on these existing values.
     */
    PERF_COUNTER_COUNT = 28
} PerfCounterIndex;

/*
 * @internal
 * Counters easily found from a debugger, incremented for frequent SCL actions
 */
extern volatile uint32_t s_PerfCounters[PERF_COUNTER_COUNT];

/*
 * @internal
 * Increment perf counter.
 */
inline void IncrementPerfCounter(PerfCounterIndex index)
{
    QCC_ASSERT(index < ArraySize(s_PerfCounters));

    /*
     * Using IncrementAndFetch here would have produced fully accurate counter
     * values. However, IncrementAndFetch would come with a performance cost.
     * Therefore, some of the IncrementPerfCounter calls will *not* actually
     * update the counter, if two or more threads are updating the same
     * counter at the same time.
     */
    s_PerfCounters[index]++;
}

}

#endif /* #ifndef _QCC_PERFCOUNTERS_H */
