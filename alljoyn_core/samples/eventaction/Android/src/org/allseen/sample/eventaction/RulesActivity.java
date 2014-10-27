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

import java.util.HashMap;
import java.util.Vector;

import org.allseen.sample.event.tester.ActionDescription;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.FragmentManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;

public class RulesActivity extends Activity implements EventActionListener {    
	private ActionsFragment actionsFragment;
	private EventsFragment eventsFragment;
	private RulesFragment rulesFragment;
	
	private MulticastLock m_multicastLock;
	private WifiManager m_wifi;
	
	//private Spinner mRuleSelector;
    private ArrayAdapter<String> ruleEngineAdapter;
    private HashMap<String, String> mfriendlyToBusMap = new HashMap<String, String>();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.rulesassigner_main);
		
		m_wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);

		FragmentManager fm = this.getFragmentManager();
		actionsFragment = (ActionsFragment) fm.findFragmentById(R.id.rules_actions_fragment);
		eventsFragment = (EventsFragment) fm.findFragmentById(R.id.rules_events_fragment);
		rulesFragment = (RulesFragment) fm.findFragmentById(R.id.rules_fragment);
		
		/*
		 * Deprecated support for remote rule engine.  Leaving as a placeholder
		 */
//		mRuleSelector = (Spinner)this.findViewById(R.id.rule_selector);
//	    ArrayList<String> ruleEngineList = new ArrayList<String>();
//		ruleEngineList.add("Local");
//		ruleEngineAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, ruleEngineList);  //array you are populating  
//		ruleEngineAdapter.setDropDownViewResource(android.R.layout.simple_dropdown_item_1line);
//		mRuleSelector.setAdapter(ruleEngineAdapter);
//		mRuleSelector.setOnItemSelectedListener(new OnItemSelectedListener() {
//			@Override
//            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
//				//Position 0 is local rule engine
//	            String selected = position == 0 ? null : ruleEngineAdapter.getItem(position);
//	            if(BackgroundService.mEventHandler != null) {
//	            	BackgroundService.mEventHandler.setEngine(mfriendlyToBusMap.get(selected));
//	            }
//            }
//			@Override
//            public void onNothingSelected(AdapterView<?> parentView) {
//	            
//            }
//		});

		Button b = (Button)this.findViewById(R.id.rule_save);
		b.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					org.allseen.sample.event.tester.EventDescription event = eventsFragment.getSelectedEvent();
					Rules r = new Rules(event);
					Vector<org.allseen.sample.event.tester.ActionDescription> actions;
					actions = actionsFragment.getSelectedActions();
					for(ActionDescription a : actions) {
						r.addAction(a);
					}
					BackgroundService.mEventHandler.enableEvent(r);
					RulesFragment.addRules(r);
					//Unset the checkboxes since saved
					eventsFragment.unsetAllChecks();
					actionsFragment.unsetAllChecks();
					actionsFragment.clearSelectedActions();
					rulesFragment.clearChecked();
				} catch(Exception e) {
					e.printStackTrace();
				}
			}
		});
		
		b = (Button)this.findViewById(R.id.rule_clear_saved);
		b.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder builder = new AlertDialog.Builder(RulesActivity.this);
                builder.setTitle("Are you sure you'd like to delete all rules?");
                builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        rulesFragment.deleteAllRules();
                    }
                });
                builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        dialog.dismiss();
                    }
                });

                AlertDialog dialog = builder.create();
                dialog.show();
            }
		});
		
		b = (Button)this.findViewById(R.id.rule_clear_selected_rules);
		b.setOnClickListener(new OnClickListener() {
		    @Override
		    public void onClick(View v) {
		    	rulesFragment.removeSelected();
		    }
		});
		
		lockMulticast();
		
		((MyApplication)getApplication()).setChainedListener(this);
		 Intent i= new Intent(this, BackgroundService.class);
		 startService(i);
	}
	
	@Override
    public void onDestroy() {
        super.onDestroy();
        unlockMulticast();
		((MyApplication)getApplication()).setChainedListener(null);
	}
	
	public void lockMulticast() {
		if(m_multicastLock == null) {
			m_multicastLock = m_wifi.createMulticastLock("multicastLock");
			m_multicastLock.setReferenceCounted(true);
			m_multicastLock.acquire();
		}
	}
	
	public void unlockMulticast() {
		if(m_multicastLock != null) {
			m_multicastLock.release();
			m_multicastLock = null;
		}
	}

	@Override
    public void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
	}

	@Override
	public void onEventFound(Device info) {
		eventsFragment.addDevice(info);
		BackgroundService.mEventHandler.enablePendingRules(info.getSessionName(), info.getFriendlyName());
	}

	@Override
	public void onActionsFound(Device info) {
		actionsFragment.addDevice(info);
		//Now update any previous rules so that they work
		RulesFragment.updateRules(info.getSessionName(), info.getFriendlyName());
		BackgroundService.mEventHandler.enablePendingRules(info.getSessionName(), info.getFriendlyName());
	}
	
	@Override
	public void onRuleEngineFound(final String sessionName, final String friendlyName) {
		runOnUiThread(new Runnable() {
			@Override
            public void run() {
				mfriendlyToBusMap.put(friendlyName, sessionName);
				ruleEngineAdapter.add(friendlyName);
				ruleEngineAdapter.notifyDataSetChanged();
			}
		});
	}

	@Override
	public void onEventLost(int sessionId) {
		
	}

	@Override
	public void onActionLost(int sessionId) {
		
	}

}
