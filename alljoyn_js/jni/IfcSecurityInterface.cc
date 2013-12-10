/*
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#include "IfcSecurityInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _IfcSecurityInterface::constants;

std::map<qcc::String, int32_t>& _IfcSecurityInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("INHERIT",  0x00);     /**< Inherit the security of the object that implements the interface */
        CONSTANT("REQUIRED", 0x01);     /**< Security is required for an interface */
        CONSTANT("OFF",      0x02);     /**< Security does not apply to this interface */
    }
    return constants;
}

_IfcSecurityInterface::_IfcSecurityInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_IfcSecurityInterface::~_IfcSecurityInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

