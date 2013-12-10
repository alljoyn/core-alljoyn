/*
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.alljoyn;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.app.PendingIntent;
import android.os.IBinder;
import android.util.Log;
import android.net.wifi.WifiManager;

public class AllJoynService extends Service {
	private static final String TAG = "alljoyn.AllJoynService";

	public IBinder onBind(Intent intent) {
        Log.i(TAG, "onBind()");
        return null;
	}

	public void onCreate() {
        super.onCreate();
        Log.i(TAG, "onCreate()");
        
        WifiManager wifi = (android.net.wifi.WifiManager)getSystemService(android.content.Context.WIFI_SERVICE);
        mMulticastLock = wifi.createMulticastLock("AllJoynService");
        mMulticastLock.setReferenceCounted(false);
        mMulticastLock.acquire();

        CharSequence title = "AllJoyn";
        CharSequence message = "Service started.";
        Intent intent = new Intent(this, AllJoynActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
        Notification notification = new Notification(R.drawable.icon, null, System.currentTimeMillis());
        notification.setLatestEventInfo(this, title, message, pendingIntent);
        notification.flags |= Notification.DEFAULT_SOUND | Notification.FLAG_ONGOING_EVENT | Notification.FLAG_NO_CLEAR;

        Log.i(TAG, "onCreate(): startForeground()");
        startForeground(NOTIFICATION_ID, notification);
 	}

	public void onDestroy() {
        super.onDestroy();
		Log.i(TAG, "onDestroy()");
        if (mMulticastLock != null) {
            mMulticastLock.release();
            mMulticastLock = null;
        }
 	}

    WifiManager.MulticastLock mMulticastLock = null;
   
	public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
		Log.i(TAG, "onStartCommand()");
        return START_STICKY;
	}
	
    private static final int NOTIFICATION_ID = 0xdefaced;
}