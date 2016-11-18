/*
 * Copyright AllSeen Alliance. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus.samples.securedoorclient;

import java.util.Set;
import java.util.Map;
import java.util.HashSet;

import android.util.Log;

import org.alljoyn.bus.AboutObjectDescription;
import org.alljoyn.bus.AboutListener;
import org.alljoyn.bus.AboutData;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.BusException;

public class DoorAboutListener implements AboutListener {
    private static final String TAG = "DoorAboutListener";

    private Set<String> mDoors;
    private Client mClient;

    public DoorAboutListener(Client client) {
        mDoors = new HashSet<String>();
        mClient = client;
    }

    public void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData) {
        String appName = "";
        String deviceName = "";
        try {
            AboutData about = new AboutData(aboutData);
            appName = about.getAppName();
            deviceName = about.getDeviceName();
        } catch (BusException e) {
            Log.e(TAG, "announced(): Failed to create AboutData for " +busName);
            e.printStackTrace();
        }

        mClient.addListEntry("Found " + appName + " @ " + busName + " (" + deviceName + ")");
        mDoors.add(busName);
    }

    public Set<String> getDoorNames() {
        return mDoors;
    }

    /* Remove the matching entry from the cached set of door busNames. */
    public void removeDoorName(String name) {
        mDoors.remove(name);
    }

}
