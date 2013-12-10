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
#include "FeaturePermissions.h"

#include "PluginData.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::StringMapKey, int32_t> PluginData::permissionLevels;

QStatus PluginData::PermissionLevel(Plugin& plugin, const qcc::String& feature, int32_t& level)
{
    QCC_DbgTrace(("%s(feature=%s)", __FUNCTION__, feature.c_str()));

    QStatus status = ER_OK;
    qcc::String origin;
    std::map<qcc::StringMapKey, int32_t>::iterator it;

    level = DEFAULT_DENIED;

    if (feature != ALLJOYN_FEATURE) {
        status = ER_FAIL;
        QCC_LogError(status, ("feature '%s' not supported", feature.c_str()));
        goto exit;
    }

    status = plugin->Origin(origin);
    if (ER_OK != status) {
        goto exit;
    }
    lock.Lock();
    it = permissionLevels.find(origin);
    if (it != permissionLevels.end()) {
        level = it->second;
        QCC_DbgTrace(("Using session level %d", level));
    } else {
        status = PersistentPermissionLevel(plugin, origin, level);
        if (ER_OK == status) {
            QCC_DbgTrace(("Using persistent level %d", level));
            permissionLevels[origin] = level;
        }
    }
    lock.Unlock();

exit:
    return status;
}

QStatus PluginData::SetPermissionLevel(Plugin& plugin, const qcc::String& feature, int32_t level, bool remember)
{
    QCC_DbgTrace(("SetPermissionLevel(feature=%s,level=%d,remember=%d)", feature.c_str(), level, remember));

    QStatus status = ER_OK;
    qcc::String origin;
    qcc::String permission;

    if (feature != ALLJOYN_FEATURE) {
        status = ER_FAIL;
        QCC_LogError(status, ("feature '%s' not supported", feature.c_str()));
        goto exit;
    }

    status = plugin->Origin(origin);
    if (ER_OK != status) {
        goto exit;
    }
    lock.Lock();
    permissionLevels[origin] = level;
    if (remember) {
        status = SetPersistentPermissionLevel(plugin, origin, level);
    }
    lock.Unlock();

exit:
    return status;
}
