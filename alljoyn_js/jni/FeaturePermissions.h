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
#ifndef _FEATUREPERMISSIONS_H
#define _FEATUREPERMISSIONS_H

#include "Plugin.h"
#include <alljoyn/Status.h>

/*
 * Feature identifiers.
 */
#define ALLJOYN_FEATURE "org.alljoyn.bus"

/*
 * Permission levels.
 */
#define USER_ALLOWED 2
#define DEFAULT_ALLOWED 1
#define DEFAULT_DENIED -1
#define USER_DENIED -2

class RequestPermissionListener {
  public:
    virtual ~RequestPermissionListener() { }
    virtual void RequestPermissionCB(int32_t level, bool remember) = 0;
};

/**
 * @param listener called when the user allows or denies permission.  If this function returns ER_OK then
 *                 the listener must remain valid until its RequestPermissionCB() is called.
 */
QStatus RequestPermission(Plugin& plugin, const qcc::String& feature, RequestPermissionListener* listener);

QStatus PersistentPermissionLevel(Plugin& plugin, const qcc::String& origin, int32_t& level);
QStatus SetPersistentPermissionLevel(Plugin& plugin, const qcc::String& origin, int32_t level);

#endif // _FEATUREPERMISSIONS_H