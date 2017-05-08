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

import org.alljoyn.bus.PermissionConfigurator;

public class OnlineApplication {

    private String busName;
    private PermissionConfigurator.ApplicationState appState;
    private ApplicationSyncState syncState;

    public String getBusName() {
        return busName;
    }

    public PermissionConfigurator.ApplicationState getAppState() {
        return appState;
    }

    public ApplicationSyncState getSyncState() {
        return syncState;
    }

    public enum ApplicationSyncState {
        SYNC_UNKNOWN,
        SYNC_UNMANAGED,
        SYNC_OK,
        SYNC_PENDING,
        SYNC_WILL_RESET,
        SYNC_RESET
    }

    public OnlineApplication(String busName,
                             PermissionConfigurator.ApplicationState appState,
                             ApplicationSyncState syncState) {
        this.busName = busName;
        this.appState = appState;
        this.syncState = syncState;
    }

    public int hashCode() {
        return busName.hashCode();
    }
}
