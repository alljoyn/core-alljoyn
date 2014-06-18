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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Vector;

import org.allseen.sample.eventaction.BusHandler;
import org.allseen.sample.eventaction.R;

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

	private ActionsFragment actionsFragment;
	private EventsFragment eventsFragment;
	
	private MulticastLock m_multicastLock;
	private WifiManager m_wifi;
	
	private Spinner mRuleSelector;
    private ArrayAdapter<String> ruleEngineAdapter;
    private HashMap<String, String> mfriendlyToBusMap = new HashMap<String, String>();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.rulesassigner_main);
		
		m_wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);

		FragmentManager fm = this.getSupportFragmentManager();
		actionsFragment = (ActionsFragment) fm.findFragmentById(R.id.rules_actions_fragment);
		eventsFragment = (EventsFragment) fm.findFragmentById(R.id.rules_events_fragment);
		
		mRuleSelector = (Spinner)this.findViewById(R.id.rule_selector);
	    ArrayList<String> ruleEngineList = new ArrayList<String>();
		ruleEngineList.add("Local");
		ruleEngineAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, ruleEngineList);  //array you are populating  
		ruleEngineAdapter.setDropDownViewResource(android.R.layout.simple_dropdown_item_1line);
		mRuleSelector.setAdapter(ruleEngineAdapter);
		mRuleSelector.setOnItemSelectedListener(new OnItemSelectedListener() {
			@Override
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				//Position 0 is local rule engine
	            String selected = position == 0 ? null : ruleEngineAdapter.getItem(position);
	            mBusHandler.setEngine(mfriendlyToBusMap.get(selected));
            }
			@Override
            public void onNothingSelected(AdapterView<?> parentView) {
	            
            }
		});

		Button b = (Button)this.findViewById(R.id.rule_save);
		b.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					Bundle b;
					Description event = eventsFragment.getSelectedEvent();
					Vector<Description> actions;
					actions = actionsFragment.getSelectedActions();
					for(int i = 0; i < actions.size(); i++) {
						b = new Bundle();
						//event
						b.putString("eUniqueName", event.getSessionName());
						b.putString("ePath", event.getPath());
						b.putString("eIface", event.getIface());
						b.putString("eMember", event.getMemberName());
						b.putString("eSig", event.getSignature());
						//action
						b.putString("aUniqueName", actions.get(i).getSessionName());
						b.putString("aPath", actions.get(i).getPath());
						b.putString("aIface", actions.get(i).getIface());
						b.putString("aMember", actions.get(i).getMemberName());
						b.putString("aSig", actions.get(i).getSignature());
						RulesActivity.mBusHandler.addRule(b);
					}
					//Unset the checkboxes since saved
					eventsFragment.unsetAllChecks();
					actionsFragment.unsetAllChecks();
					actionsFragment.clearSelectedActions();
				} catch(Exception e) {
					e.printStackTrace();
				}
			}
		});
		
		b = (Button)this.findViewById(R.id.rule_clear_saved);
		b.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				mBusHandler.removeAllRules();
			}
		});
		
		if (mBusHandler == null) {
			/* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
			HandlerThread busThread = new HandlerThread("BusHandler");
			busThread.start();
			mBusHandler = new BusHandler(busThread.getLooper(), this, this);
			mBusHandler.sendEmptyMessage(BusHandler.INITIALIZE);
			mBusHandler.sendEmptyMessage(BusHandler.START_RULE_ENGINE);
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
	public void onEventsFound(Device info) {
		eventsFragment.addDevice(info);
	}

	@Override
	public void onActionsFound(Device info) {
		actionsFragment.addDevice(info);
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
