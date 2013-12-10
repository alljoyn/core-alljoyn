/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 */
#include "MessageInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _MessageInterface::constants;

std::map<qcc::String, int32_t>& _MessageInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("ALLJOYN_FLAG_NO_REPLY_EXPECTED", 0x01);
        CONSTANT("ALLJOYN_FLAG_AUTO_START",        0x02);
        CONSTANT("ALLJOYN_FLAG_ALLOW_REMOTE_MSG",  0x04);
        CONSTANT("ALLJOYN_FLAG_SESSIONLESS",       0x10);
        CONSTANT("ALLJOYN_FLAG_GLOBAL_BROADCAST",  0x20);
        CONSTANT("ALLJOYN_FLAG_COMPRESSED",        0x40);
        CONSTANT("ALLJOYN_FLAG_ENCRYPTED",         0x80);
    }
    return constants;
}

_MessageInterface::_MessageInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_MessageInterface::~_MessageInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}
