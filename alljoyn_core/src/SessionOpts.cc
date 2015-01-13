/**
 * @file
 * Class for encapsulating Session option information.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014-2015, AllSeen Alliance. All rights reserved.
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
#include <qcc/Util.h>
#include <alljoyn/MsgArg.h>

#include <assert.h>

#include "SessionInternal.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;

namespace ajn {

/** SessionOpts key values */
#define SESSIONOPTS_TRAFFIC     "traf"
#define SESSIONOPTS_ISMULTICAST "multi"
#define SESSIONOPTS_PROXIMITY   "prox"
#define SESSIONOPTS_TRANSPORTS  "trans"
#define SESSIONOPTS_NAMETRANSFER  "names"

bool SessionOpts::IsCompatible(const SessionOpts& other) const
{
    /* No overlapping transports means opts are not compatible */
    if (0 == (transports & other.transports)) {
        return false;
    }

    /* Not overlapping traffic types means opts are not compatible */
    if (0 == (traffic & other.traffic)) {
        return false;
    }

    /* Not overlapping proximities means opts are not compatible */
    if (0 == (proximity & other.proximity)) {
        return false;
    }

    /* Note that isMultipoint is not a condition of compatibility */

    return true;
}

qcc::String SessionOpts::ToString() const
{
    QCC_DbgTrace(("UDPTransport::Join()"));
    qcc::String str = "traffic=";
    switch (traffic) {
    case TRAFFIC_MESSAGES:
        str.append("TRAFFIC_MESSAGES");
        break;

    case TRAFFIC_RAW_UNRELIABLE:
        str.append("TRAFFIC_RAW_UNRELIABLE");
        break;

    case TRAFFIC_RAW_RELIABLE:
        str.append("TRAFFIC_RAW_RELIABLE");
        break;

    default:
        str.append("unknown");
        break;
    }

    str.append(", isMultipoint=");
    str.append(isMultipoint ? "true" : "false");

    str.append(", proximity=");
    switch (proximity) {
    case PROXIMITY_ANY:
        str.append("PROXIMITY_ANY");
        break;

    case PROXIMITY_PHYSICAL:
        str.append("PROXIMITY_PHYSICAL");
        break;

    case PROXIMITY_NETWORK:
        str.append("PROXIMITY_NETWORK");
        break;

    default:
        str.append("unknown");
        break;
    }

    return str;
}

QStatus GetSessionOpts(const MsgArg& msgArg, SessionOpts& opts)
{
    const MsgArg* dictArray;
    size_t numDictEntries;
    QStatus status = msgArg.Get("a{sv}", &numDictEntries, &dictArray);
    if (status == ER_OK) {
        for (size_t n = 0; n < numDictEntries; ++n) {
            const char* key = dictArray[n].v_dictEntry.key->v_string.str;
            const MsgArg* val = dictArray[n].v_dictEntry.val->v_variant.val;

            dictArray[n].Get("{sv}", &key, &val);
            if (::strcmp(SESSIONOPTS_TRAFFIC, key) == 0) {
                uint8_t tmp;
                val->Get("y", &tmp);
                opts.traffic = static_cast<SessionOpts::TrafficType>(tmp);
            } else if (::strcmp(SESSIONOPTS_ISMULTICAST, key) == 0) {
                val->Get("b", &opts.isMultipoint);
            } else if (::strcmp(SESSIONOPTS_PROXIMITY, key) == 0) {
                val->Get("y", &opts.proximity);
            } else if (::strcmp(SESSIONOPTS_TRANSPORTS, key) == 0) {
                val->Get("q", &opts.transports);
            } else if (::strcmp(SESSIONOPTS_NAMETRANSFER, key) == 0) {
                uint8_t tmp;
                val->Get("y", &tmp);
                opts.nameTransfer = static_cast<SessionOpts::NameTransferType>(tmp);
            }
        }
    }
    return status;
}

void SetSessionOpts(const SessionOpts& opts, MsgArg& msgArg)
{
    MsgArg trafficArg("y", opts.traffic);
    MsgArg isMultiArg("b", opts.isMultipoint);
    MsgArg proximityArg("y", opts.proximity);
    MsgArg transportsArg("q", opts.transports);
    MsgArg nameTransferArg("y", opts.nameTransfer);

    MsgArg entries[5];
    entries[0].Set("{sv}", SESSIONOPTS_TRAFFIC, &trafficArg);
    entries[1].Set("{sv}", SESSIONOPTS_ISMULTICAST, &isMultiArg);
    entries[2].Set("{sv}", SESSIONOPTS_PROXIMITY, &proximityArg);
    entries[3].Set("{sv}", SESSIONOPTS_TRANSPORTS, &transportsArg);
    entries[4].Set("{sv}", SESSIONOPTS_NAMETRANSFER, &nameTransferArg);
    QStatus status = msgArg.Set("a{sv}", ArraySize(entries), entries);
    if (status == ER_OK) {
        msgArg.Stabilize();
    } else {
        QCC_LogError(status, ("Failed to set SessionOpts message arg"));
    }

}

}

