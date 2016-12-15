/*
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 */
package org.alljoyn.bus.samples;

import java.util.Map;
import java.util.HashMap;

import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.ProxyBusObject;

public class DoorSessionManager {
    public static final short DOOR_APPLICATION_PORT = 12345;

    private Map<String, Session> sessionMap;
    private BusAttachment mBus;

    public DoorSessionManager(BusAttachment ba) {
        sessionMap = new HashMap<String, Session>();
        mBus = ba;
    }

    public Door getProxyDoorObject(String busName) throws BusException {
        if (sessionMap.containsKey(busName)) {
            return sessionMap.get(busName).getDoorInterface();
        } else {
            SessionOpts sessionOpts = new SessionOpts();

            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            Status status = mBus.joinSession(busName,
                    DOOR_APPLICATION_PORT,
                    sessionId, sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                return null;
            }

            System.out.println(String.format("BusAttachement.joinSession successful sessionId = %d", sessionId.value));

            ProxyBusObject mProxyObj = mBus.getProxyBusObject(busName,
                    Door.DOOR_PATH,
                    sessionId.value,
                    new Class<?>[] {Door.class});

            Door mDoor = mProxyObj.getInterface(Door.class);

            Session session = new Session(sessionId, mDoor);
            sessionMap.put(busName, session);

            return mDoor;
        }
    }

    public void stop() {
        for (Map.Entry<String, Session> entry : sessionMap.entrySet()) {
            mBus.leaveSession(entry.getValue().getSessionId().value);
        }
    }
}