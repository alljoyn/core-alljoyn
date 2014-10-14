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

package org.allseen.sample.event.tester;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Vector;

import org.allseen.sample.event.tester.BusHandler;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Bundle;
import android.os.HandlerThread;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;

public class RulesActivity extends FragmentActivity implements EventActionListener {
	static BusHandler mBusHandler;

	private EventsFragment eventsFragment;
	
	private MulticastLock m_multicastLock;
	private WifiManager m_wifi;
	
    private ArrayAdapter<String> ruleEngineAdapter;
    private HashMap<String, String> mfriendlyToBusMap = new HashMap<String, String>();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.rulesassigner_main);
		
		m_wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);

		FragmentManager fm = this.getSupportFragmentManager();
		eventsFragment = (EventsFragment) fm.findFragmentById(R.id.rules_events_fragment);

		Button b = (Button)this.findViewById(R.id.rule_save);
		b.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					Bundle b;
					Description event;
					event = eventsFragment.getSelectedEvent();
					b = new Bundle();
					//event
					b.putString("eDescription", event.getDescription());
					b.putString("eUniqueName", event.getSessionName());
					b.putString("ePath", event.getPath());
					b.putString("eIface", event.getIface());
					b.putString("eMember", event.getMemberName());
					b.putString("eSig", event.getSignature());
					RulesActivity.mBusHandler.enableEvent(b);
					//Unset the checkboxes since saved
					eventsFragment.unsetAllChecks();
				} catch(Exception e) {
					e.printStackTrace();
				}
			}
		});
		
		if (mBusHandler == null) {
			/* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
			HandlerThread busThread = new HandlerThread("BusHandler");
			busThread.start();
			mBusHandler = new BusHandler(busThread.getLooper(), this, this);
			mBusHandler.sendEmptyMessage(BusHandler.INITIALIZE);
		}
		lockMulticast();
	}
	
	@Override
    public void onDestroy() {
        super.onDestroy();
        unlockMulticast();
	}
	
	public void lockMulticast() {
		Log.d(BusHandler.TAG, "Trying to check if we already have a lock");
		if(m_multicastLock == null) {
			m_multicastLock = m_wifi.createMulticastLock("multicastLock");
			m_multicastLock.setReferenceCounted(true);
			m_multicastLock.acquire();
			Log.d(BusHandler.TAG, "MulticastLock acquired");
		}
	}
	
	public void unlockMulticast() {
		Log.d(BusHandler.TAG, "Trying to check if we have a lock to release");
		if(m_multicastLock != null) {
			Log.d(BusHandler.TAG, "releasing MulticastLock");
			m_multicastLock.release();
			m_multicastLock = null;
			Log.d(BusHandler.TAG, "released MulticastLock");
		}
	}

	public void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
	}

	@Override
	public void onEventFound(Device info) {
		eventsFragment.addDevice(info);
	}
	
	@Override
	public void onEventLost(int sessionId) {
		eventsFragment.removeDevice(sessionId);
	}
	
	@Override
	public void onRuleEngineFound(final String sessionName, final String friendlyName) {
		runOnUiThread(new Runnable() {
			public void run() {
				mfriendlyToBusMap.put(friendlyName, sessionName);
				ruleEngineAdapter.add(friendlyName);
				ruleEngineAdapter.notifyDataSetChanged();
			}
		});
	}

}
