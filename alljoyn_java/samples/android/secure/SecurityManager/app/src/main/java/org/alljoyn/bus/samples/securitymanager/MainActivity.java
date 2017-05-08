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

import android.app.ActivityManager;
import android.os.Handler;
import android.app.AlertDialog;
import android.os.Message;
import android.os.HandlerThread;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements RefreshInterface {

    private static final String TAG = "MainActivity";

    private ListView listView;
    private MainListAdapter adapter;

    private Handler ajHandler;

    /**
     * background alljoyn thread
     */
    private HandlerThread bgAJThread;
    private AlljoynActions ajAct;

    /**
     * Error handler
     */
    protected Handler errorHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AlertDialog alert = new AlertDialog.Builder(MainActivity.this).create();
            switch (msg.what) {
                case AlljoynActions.FAIL:
                    alert.setTitle("Error");
                    alert.setMessage(msg.obj.toString());
                    break;
                case AlljoynActions.SUCCESS:
                    alert.setTitle("Success");
                    alert.setMessage(msg.obj.toString());
                    break;
            }
            alert.show();
        }
    };

    private SecurityApplicationStateListener secAppStateListener;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        bgAJThread = new HandlerThread("Security Manager");
        bgAJThread.start();
        ajHandler = new Handler(bgAJThread.getLooper());
        secAppStateListener = new SecurityApplicationStateListener(this);
        ajAct = new AlljoynActions(this, errorHandler, secAppStateListener);
        ajHandler.post(ajAct.busStart);

        listView = (ListView) findViewById(R.id.listView);

        listView.setOnItemClickListener(onItemClickListener);
        registerForContextMenu(listView);

        adapter = new MainListAdapter(this, secAppStateListener);
        listView.setAdapter(adapter);

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        bgAJThread.quit();
    }

    @Override
    public void refresh() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                adapter.notifyDataSetChanged();
            }
        });
    }

    private AdapterView.OnItemClickListener onItemClickListener = new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            ajAct.setPosition(position);
            ajHandler.post(ajAct.claimAndAddMembership);
        }
    };

    private int menuPosition = 0;

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        if (v.getId() == R.id.listView) {
            // Get the list item position
            AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
            menuPosition = info.position;

            MenuInflater inflater = getMenuInflater();
            inflater.inflate(R.menu.menu_app, menu);
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        int id = item.getItemId();
        ajAct.setPosition(menuPosition);
        if (id == R.id.claim) {
            ajHandler.post(ajAct.claimAndAddMembership);
        } else if(id == R.id.installpol) {
            ajHandler.post(ajAct.installPolicy);
        } else if (id == R.id.installmem){
            ajHandler.post(ajAct.installMembership);
        } else if (id == R.id.unclaim) {
            ajHandler.post(ajAct.unclaim);
        }

        return super.onOptionsItemSelected(item);
    }


}
