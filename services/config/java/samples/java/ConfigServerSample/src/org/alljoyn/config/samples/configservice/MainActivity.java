/******************************************************************************
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
 ******************************************************************************/

package org.alljoyn.config.samples.configservice;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class MainActivity extends Activity {

    private static final String TAG = "MainActivity";

    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;

    // Intent for the Config Service
    private Intent mServiceIntent;

    // Receive broadcasts indicating Config Service state
    private BroadcastReceiver mBroadcastReceiver;

    // Handler used to update UI display
    protected static final int MESSAGE_ADD_LIST_ENTRY = 1;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_ADD_LIST_ENTRY:
                    mListViewArrayAdapter.add((String)msg.obj);
                    break;
                default:
                    Log.w(TAG, "Unsupported message type : " + msg.what);
                    break;
            }
        }
    };

	@Override
	protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onDestroy");
		super.onCreate(savedInstanceState);

        // Start Config Service
        mServiceIntent = new Intent(this, ConfigService.class);
        startService(mServiceIntent);

        // Setup UI
        setContentView(R.layout.main);
        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        // Setup broadcast receiver used to display notifications from the Config Service
        mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                Bundle extras = intent.getExtras();
                if (action.equals(ConfigService.BROADCAST_NOTIFICATION)) {
                    addListEntry(extras.getString(ConfigService.BROADCAST_NOTIFICATION_MESSAGE));
                } else {
                    Log.w(TAG, "Unsupported broadcast type : " + action);
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(ConfigService.BROADCAST_NOTIFICATION);
        registerReceiver(mBroadcastReceiver, filter);
	}

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.quit:
                finish();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        stopService(mServiceIntent);
        unregisterReceiver(mBroadcastReceiver);
        super.onDestroy();
    }

    private void addListEntry(String msg) {
        Message entry = mHandler.obtainMessage(MESSAGE_ADD_LIST_ENTRY, msg);
        mHandler.sendMessage(entry);
        Log.i(TAG, msg);
    }

}