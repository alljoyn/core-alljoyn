/*
 * Copyright AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.samples.securedoorclient;

import java.util.Map;
import java.util.HashMap;

import android.util.Log;

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
    private Client mClient;

    public DoorSessionManager(BusAttachment bus, SessionListener sessionListener, Client client) {
        mSessionMap = new HashMap<String, Session>();
        mBus = bus;
        mSessionListener = sessionListener;
        mClient = client;
    }

    public Door getProxyDoorObject(String busName) throws BusException {
        if (mSessionMap.containsKey(busName)) {
            return mSessionMap.get(busName).getDoorInterface();
        } else {
            SessionOpts sessionOpts = new SessionOpts();
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            Status status = mBus.joinSession(busName, Client.CONTACT_PORT, sessionId, sessionOpts, mSessionListener);
            mClient.logStatus(String.format("joinSession(%s)",busName), status);
            if (status != Status.OK) {
                return null;
            }

            ProxyBusObject mProxyObj = mBus.getProxyBusObject(busName,
                    Door.DOOR_PATH,
                    sessionId.value,
                    new Class<?>[] {Door.class});

            Door mDoor = mProxyObj.getInterface(Door.class);

            Session session = new Session(sessionId, mDoor);
            mSessionMap.put(busName, session);

            return mDoor;
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

    public void stop() {
        for (Map.Entry<String, Session> entry : mSessionMap.entrySet()) {
            mBus.leaveSession(entry.getValue().getSessionId().value);
        }
        mSessionMap.clear();
    }

}
