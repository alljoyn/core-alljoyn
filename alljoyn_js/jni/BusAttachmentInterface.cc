/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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
    QCC_UNUSED(args);
    QCC_UNUSED(argCount);
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
        return false;
    }
}