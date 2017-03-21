/*
 * Copyright (c) Open Connectivity Foundation, AllJoyn Open Source Project Contributors and others.
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
        try {
            AboutData about = new AboutData(aboutData);
            mClient.addListEntry("Found " + about.getAppName() + " @ " + busName + " (" + about.getDeviceName() + ")");
            mDoors.add(busName);
        } catch (BusException e) {
            mClient.addListEntry("Found " + busName + " - error parsing announcement: " + e.getMessage());
            Log.e(TAG, "announced(): Failed to parse AboutData for " + busName);
            e.printStackTrace();
        }
    }

    public Set<String> getDoorNames() {
        return mDoors;
    }

    /* Remove the matching entry from the cached set of door busNames. */
    public void removeDoorName(String name) {
        mDoors.remove(name);
    }

}
