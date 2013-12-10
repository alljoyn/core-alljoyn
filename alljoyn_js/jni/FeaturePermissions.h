/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
