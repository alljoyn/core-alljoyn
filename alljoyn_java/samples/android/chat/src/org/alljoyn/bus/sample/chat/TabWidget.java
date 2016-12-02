/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus.sample.chat;

import android.os.Bundle;

import android.app.TabActivity;

import android.widget.TabHost;

import android.content.Intent;
import android.content.res.Resources;

import android.util.Log;

public class TabWidget extends TabActivity {
    private static final String TAG = "chat.TabWidget";
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "onCreate()");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        Resources res = getResources();
        TabHost tabHost = getTabHost();
        TabHost.TabSpec spec;
        Intent intent;

        intent = new Intent().setClass(this, UseActivity.class);
        spec = tabHost.newTabSpec("use").setIndicator("", res.getDrawable(R.drawable.ic_tab_use)).setContent(intent);
        tabHost.addTab(spec);

        intent = new Intent().setClass(this, HostActivity.class);
        spec = tabHost.newTabSpec("host").setIndicator("", res.getDrawable(R.drawable.ic_tab_host)).setContent(intent);
        tabHost.addTab(spec);

        tabHost.setCurrentTab(0);
    }
}