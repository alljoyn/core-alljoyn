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
#include "BusAttachmentInterface.h"

#include "BusAttachmentHost.h"
#include "CallbackNative.h"
#include "FeaturePermissions.h"
#include "HostObject.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _BusAttachmentInterface::constants;

std::map<qcc::String, int32_t>& _BusAttachmentInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("DBUS_NAME_FLAG_ALLOW_REPLACEMENT", 0x01);
        CONSTANT("DBUS_NAME_FLAG_REPLACE_EXISTING",  0x02);
        CONSTANT("DBUS_NAME_FLAG_DO_NOT_QUEUE",      0x04);

        CONSTANT("SESSION_PORT_ANY", 0);
    }
    return constants;
}

_BusAttachmentInterface::_BusAttachmentInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_BusAttachmentInterface::~_BusAttachmentInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _BusAttachmentInterface::Construct(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    int32_t level = 0;

    /*
     * Check permission level first.
     */
    status = PluginData::PermissionLevel(plugin, ALLJOYN_FEATURE, level);
    if (ER_OK != status) {
        status = ER_OK;
        level = 0;
    }
    if (level <= 0) {
        typeError = true;
        plugin->RaiseTypeError("permission denied");
        goto exit;
    }

    {
        BusAttachmentHost busAttachmentHost(plugin);
        ToHostObject<BusAttachmentHost>(plugin, busAttachmentHost, *result);
    }

exit:
    if ((ER_OK == status) && !typeError) {
        return true;
    } else {
        if (ER_OK != status) {
            plugin->RaiseBusError(status);
        }
        return false;
    }
}
