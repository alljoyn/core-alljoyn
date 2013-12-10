/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.p2p.service;

import android.os.IBinder;
import android.os.Binder;
import android.os.Handler;

import android.app.Service;

import android.content.Intent;

import android.util.Log;

public class P2pHelperAndroidService extends Service {

    static {
        System.loadLibrary("P2pHelperService");
    }

    private static final String TAG = "P2pHelperService";

    public class LocalBinder extends Binder {
        P2pHelperAndroidService getService() {
            return P2pHelperAndroidService.this;
        }
    }

    private final IBinder mBinder = new LocalBinder();

    public IBinder onBind(Intent intent) {
        Log.i(TAG, "onBind()");
        return mBinder;
    }

    private P2pHelperService mP2pHelperService = null;
    private Handler mHandler = null;
    private Runnable mConnect = null;

    private final long connectionRetryInterval = 5000;

    public void onCreate() {
        Log.i(TAG, "onCreate()");
        mHandler = new Handler();
        mConnect = new Runnable() {
            public void run() {
                if (null == mP2pHelperService) {
                    Log.d(TAG, "Attempting to connect to daemon");
                    mP2pHelperService = new P2pHelperService(getApplicationContext(), "unix:abstract=alljoyn");
                }
                mP2pHelperService.startup();
                if (!mP2pHelperService.isConnected()) {
                    Log.d(TAG, "Not connected to daemon");
                    mP2pHelperService = null;
                }
                mHandler.postDelayed(mConnect, connectionRetryInterval);
            }
        };
    }

    private void cleanup() {
        mHandler.removeCallbacks(mConnect);

        if (mP2pHelperService != null) {
            mP2pHelperService.shutdown();
            mP2pHelperService = null;
        }
    }

    public void onDestroy() {
        Log.i(TAG, "onDestroy()");
        cleanup();
    }

    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand()");

        cleanup();
        mHandler.post(mConnect);

        return START_STICKY;
    }

    public void exit() {
        Log.i(TAG, "exit");
        System.exit(0);
    }

}
