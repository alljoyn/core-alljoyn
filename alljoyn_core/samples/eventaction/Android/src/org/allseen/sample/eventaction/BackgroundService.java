/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

package org.allseen.sample.eventaction;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.PowerManager;

public class BackgroundService extends Service {
	public static org.allseen.sample.event.tester.BusHandler mEventHandler;
	private PowerManager.WakeLock wakelock;
	
	private static final int NOTIFICATION_ID = 0x12fa5ed;
	
	@Override
	public void onCreate() {
		
	}
	
	@Override
	public int onStartCommand(Intent i, int flags, int startId) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				if(mEventHandler == null) {
					HandlerThread busThread = new HandlerThread("BusHandler");
					busThread.start();
					HandlerThread busThread2 = new HandlerThread("BusHandler");
					busThread2.start();
					mEventHandler = new org.allseen.sample.event.tester.BusHandler(busThread.getLooper(), BackgroundService.this, (EventActionListener)BackgroundService.this.getApplication());

					mEventHandler.initialize(BackgroundService.this.getPackageName()+"Event");
					
					CharSequence title = "EventAction Rule Engine";
			        CharSequence message = "Rule engine running";
			        Intent intent = new Intent(BackgroundService.this, RulesActivity.class);
			        PendingIntent pendingIntent = PendingIntent.getActivity(BackgroundService.this, 0, intent, 0);
			        Notification notification = new Notification(R.drawable.ic_launcher, null, System.currentTimeMillis());
			        notification.setLatestEventInfo(BackgroundService.this, title, message, pendingIntent);
			        notification.flags |= Notification.DEFAULT_SOUND | Notification.FLAG_ONGOING_EVENT | Notification.FLAG_NO_CLEAR;
			
			        startForeground(NOTIFICATION_ID, notification);
			        
			        PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
			        wakelock = powerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, Constants.TAG);
			        wakelock.acquire();
			         	        
        			RulesFragment.loadRules();
				}
			}});
		t.start();
		
		return Service.START_NOT_STICKY;
	}/* Start our service. */
	
	@Override
	public void onDestroy() {
		if (null != wakelock) {
		    wakelock.release();
		}
	}

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
}
