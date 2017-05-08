/**
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus.samples.securitymanager;

import android.content.Context;
import android.util.Log;

import org.alljoyn.bus.ApplicationStateListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.common.KeyInfoNISTP256;

import java.security.Key;
import java.util.HashMap;
import java.util.Map;

public class SecurityApplicationStateListener implements ApplicationStateListener {

    private static final String TAG = "SAppStateListener";

    private Map<KeyInfoNISTP256, OnlineApplication> secureDevicesMap;
    public Map<KeyInfoNISTP256, OnlineApplication> getMap() {
        return secureDevicesMap;
    }

    private MainActivity context;

    public SecurityApplicationStateListener(MainActivity context) {
        this.context = context;
        secureDevicesMap = new HashMap<KeyInfoNISTP256, OnlineApplication>();
    }

    @Override
    public void state(String busName,
                      KeyInfoNISTP256 key,
                      PermissionConfigurator.ApplicationState applicationState) {
        Log.i(TAG, "State Callback " + applicationState);
        OnlineApplication app = new OnlineApplication(busName, applicationState, OnlineApplication.ApplicationSyncState.SYNC_OK);
        secureDevicesMap.put(key, app);
        context.refresh();
    }
}
