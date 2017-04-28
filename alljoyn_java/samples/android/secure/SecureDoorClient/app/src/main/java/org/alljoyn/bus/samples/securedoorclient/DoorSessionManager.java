/*
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
package org.alljoyn.bus.samples.securedoorclient;

import java.util.Map;
import java.util.HashMap;

import org.alljoyn.bus.OnJoinSessionListener;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ProxyBusObject;

public class DoorSessionManager {
    private static final String TAG = "DoorSessionManager";

    private Map<String, Session> mSessionMap; // Key: busName, Value: Session
    private BusAttachment mBus;
    private SessionListener mSessionListener;


    public interface ResponseListener {
        /* Send response to the listener. Presence of the door and fail argument values is mutually exclusive. */
        public void response(String bus, Door door, Exception fail);
    }


    public DoorSessionManager(BusAttachment bus, SessionListener sessionListener, Client client) {
        mSessionMap = new HashMap<String, Session>();
        mBus = bus;
        mSessionListener = sessionListener;
    }

    /*
     * Retrieve the cached Door proxy object. If none, join session with the named door service,
     * create/cache a proxy object of the door service, and then return this door proxy object.
     * Result is returned asynchronously via the given ResponseListener.
     */
    public void getProxyDoorObject(final String busName, final ResponseListener listener) {
        if (mSessionMap.containsKey(busName)) {
            Door door = mSessionMap.get(busName).getDoorInterface();
            sendResponse(listener, busName, door, null);
        } else {
            SessionOpts sessionOpts = new SessionOpts();

            /* Call asynchronous joinSession, where onJoinSession callback is invoked when join session attempt has completed. */
            Status status = mBus.joinSession(busName, Client.CONTACT_PORT, sessionOpts, mSessionListener,
                new OnJoinSessionListener() {
                    @Override
                    public void onJoinSession(Status status, int sessionId, SessionOpts opts, Object context) {
                        if (status != Status.OK) {
                            sendResponse(listener, busName, null, new BusException("Join session failed with error " + status.name()));
                        } else {
                            ProxyBusObject mProxyObj = mBus.getProxyBusObject(busName,
                                    Door.DOOR_PATH,
                                    sessionId,
                                    new Class<?>[]{Door.class});

                            Door door = mProxyObj.getInterface(Door.class);
                            Session session = new Session(new Mutable.IntegerValue(sessionId), door);
                            mSessionMap.put(busName, session);

                            sendResponse(listener, busName, door, null);
                        }
                    }
                },
                null);

            if (status != Status.OK) {
                sendResponse(listener, busName, null, new BusException("Join session failed with error " + status.name()));
            }
        }
    }

    /* Remove the matching entry from the cached session map. Return the associated key (busName) of the sessionMap entry that was removed. */
    public String removeSession(int sessionId) {
        String busName = null;
        for (Map.Entry<String,Session> entry : mSessionMap.entrySet()) {
            Session sess = entry.getValue();
            if (sess.getSessionId().value == sessionId) {
                busName = entry.getKey();
                mSessionMap.remove(busName);
                break;
            }
        }
        return busName;
    }

    public void stop(String busName) {
        Session sess = mSessionMap.remove(busName);
        if (sess != null) {
            mBus.leaveSession(sess.getSessionId().value);
        }
    }

    public void stop() {
        for (Map.Entry<String, Session> entry : mSessionMap.entrySet()) {
            mBus.leaveSession(entry.getValue().getSessionId().value);
        }
        mSessionMap.clear();
    }

    /* Send response from a new thread. We don't want to run on the AllJoyn thread when being called from the OnJoinSessionListener. */
    private void sendResponse(final ResponseListener listener, final String busName, final Door door, final Exception ex) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                listener.response(busName, door, ex);
            }
        }).start();
    }

}
