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
#include "CredentialsInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _CredentialsInterface::constants;

std::map<qcc::String, int32_t>& _CredentialsInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("PASSWORD",          0x0001);
        CONSTANT("USER_NAME",         0x0002);
        CONSTANT("CERT_CHAIN",        0x0004);
        CONSTANT("PRIVATE_KEY",       0x0008);
        CONSTANT("LOGON_ENTRY",       0x0010);
        CONSTANT("EXPIRATION",        0x0020);
        CONSTANT("NEW_PASSWORD",      0x1001);
        CONSTANT("ONE_TIME_PWD",      0x2001);
    }
    return constants;
}

_CredentialsInterface::_CredentialsInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_CredentialsInterface::~_CredentialsInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}
