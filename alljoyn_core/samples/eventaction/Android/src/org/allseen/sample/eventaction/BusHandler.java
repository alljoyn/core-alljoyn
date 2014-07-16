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
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;

public class BusHandler extends Handler {
	public static final String TAG = "BusHandler";
		
	private Context mContext;
	private EventActionListener listener;
	private SharedPreferences mPrefs; 
	private static final String PREFS_KEY = "engine_rules";

    /* These are the messages sent to the BusHandler from the UI/Application thread. */
    public static final int INITIALIZE = 0;
    public static final int START_RULE_ENGINE = 1;
    public static final int INTROSPECT = 2;
    public static final int ADD_RULE = 3;
    public static final int REMOVE_RULE = 4;
    public static final int REMOVE_ALL_RULES = 5;
    public static final int ADD_SAVED_RULE = 6;
    public static final int SHUTDOWN = 100;
    
    
    private HashMap<String,Device> remoteInfo = new HashMap<String, Device>();
    
    /*
     * Native JNI methods
     */
    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("MyAllJoynCode");
    }

	 /** Initialize AllJoyn in JNI(C++) */
    private native void initialize(String packageName);
    
    /** Start the local rule engine */
    private native void startRuleEngine();
    
    /** Perform introspection with description in JNI(C++) */
    private native String doIntrospection(String sessionName, String path, int sessionId);
    
    public native void setEngine(String jEngineName);

    /** Save and enable a rule */
    private native void addRule(//event
								  String jEUniqueName, String jEPath,
								  String jEIface, String jEMember, String jESig,
								//action
								  String jAUniqueName, String jAPath,
								  String jAIface, String jAMember, String jASig, boolean persist);
    
    /** add a rule that was saved in persistent memory */
    private native void addSavedRule(//event
								  String jEUniqueName, String jEPath,
								  String jEIface, String jEMember, String jESig,
								  String jEDeviceId, String jEAppId, short Eport,
								//action
								  String jAUniqueName, String jAPath,
								  String jAIface, String jAMember, String jASig,
								  String jADeviceId, String jAAppId, short Aport);
    
    /** clear out the saved rules in local memory and persistent memory */
    private native void deleteAllRules();
    
    /** Clean up and shutdown AllJoyn in JNI(C++) */
    private native void shutdown();

    /**
     * Constructor
     */
    public BusHandler(Looper looper, Context context, EventActionListener listener) {
        super(looper);
        this.mContext = context;
        this.listener = listener;
        this.mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
    }
    
    /**
     * Callback from jni code invoked via an About Annouce callback
     * @param sessionName	name of the remote application
     * @param sessionId		sessionId that was set
     * @param friendlyName	name to display in the UI
     */
	public void foundEventActionApplication(String sessionName, int sessionId, String friendlyName) {
		Log.d(TAG, "Found device with friendly name: "+friendlyName);
		Message msg = this.obtainMessage(INTROSPECT);
		Bundle data = new Bundle();
		data.putString("name", sessionName);
		data.putString("friendly", friendlyName);
		msg.setData(data);
		msg.arg1 = sessionId;
		this.sendMessage(msg);
	}

    /**
     * Callback from jni code invoked when a device goes away
     * @param sessionId		sessionId that was lost
     */
	public void lostEventActionApplication(int sessionId) {
		Log.d(TAG, "Lost applicatoin with session id: "+sessionId);
		listener.onEventActionLost(sessionId);
	}
	
	/**
	 * Callback from the jni code
	 * @param sessionName name of the remote device that contains a rule engine
	 */
	public void foundRuleEngineApplication(String sessionName, String friendlyName) {
		Log.d(TAG, "Found Rule Engine: "+sessionName);
		listener.onRuleEngineFound(sessionName, friendlyName);
	}
	
	/**
	 * Set the serialized rule to persistent memory
	 * @param data serialized rule
	 */
	public void saveRule(String data) {
		Editor editor = mPrefs.edit();
		/** If using a newer version of Android use the following, otherwise create a long string */
		//Set<String> rules = mPrefs.getStringSet(PREFS_KEY, new HashSet<String>());
		//rules.add(data);
		//editor.clear();
		//editor.putStringSet(PREFS_KEY, rules);
		/** For Gingerbread use the following */
		String rules = mPrefs.getString(PREFS_KEY, "");
		rules += data + ";";
		editor.clear();
		editor.putString(PREFS_KEY, rules);
		Log.d(TAG, "Save Rule: "+data);
		editor.commit();
	}
	
	/**
	 * Method invoked via JNI to read the preferences and then save each stored rule
	 */
	public void loadRules() {
		/** If using a newer version of Android use the following, otherwise create a long string */
		//Set<String> rules = mPrefs.getStringSet(PREFS_KEY, new HashSet<String>());
		//for (String rule : rules) {
			//Log.d(TAG, "Parsing saved rule: "+rule);
			//parseSavedRuleAndAdd(rule);
		//}
		/** For Gingerbread use the following */
		String ruleString = mPrefs.getString(PREFS_KEY, "");
		Log.d(TAG, "Read saved string: "+ruleString);
		if(ruleString.length() > 0) {
			String[] rules = ruleString.split("\\;");
			for (int i = 0; i < rules.length; i++) {
				if (rules[i] != null && rules[i].length() > 0) { 
					Log.d(TAG, "Parsing saved rule: "+rules[i]);
					parseSavedRuleAndAdd(rules[i]);
				}
			}
		}
	}

	/**
	 * Method invoked via JNI to clear Android preferences
	 */
	public void clearRules() {
		Editor editor = mPrefs.edit();
		editor.clear();
		editor.commit();
	}
	
	/**
	 * Helper method to parse the saved rule that was persisted
	 */
	public void parseSavedRuleAndAdd(String rule) {
		try{
			Bundle b = new Bundle();
			String parts[] = rule.split("[|]");
			String event[] = parts[0].split("[,]");
			String action[] = parts[1].split("[,]");
			//event
			b.putString("eUniqueName", event[0]);
			b.putString("ePath", event[1]);
			b.putString("eIface", event[2]);
			b.putString("eMember", event[3]);
			b.putString("eSig", event.length > 4 ? event[4] : "");
			b.putString("eDeviceId", event.length > 5 ? event[5] : "");
			b.putString("eAppId", event.length > 6 ? event[6] : "");
			b.putShort("ePort", event.length > 7 && event[7].trim().length() != 0 ? Short.parseShort(event[7]) : 0);
			//action
			b.putString("aUniqueName", action[0]);
			b.putString("aPath", action[1]);
			b.putString("aIface", action[2]);
			b.putString("aMember", action[3]);
			b.putString("aSig", action.length > 4 ? action[4] : "");
			b.putString("aDeviceId", event.length > 5 ? event[5] : "");
			b.putString("aAppId", event.length > 6 ? event[6] : "");
			b.putShort("aPort", event.length > 7 && event[7].trim().length() != 0 ? Short.parseShort(event[7]) : 0);
			
			Message msg = this.obtainMessage(ADD_SAVED_RULE);
			msg.setData(b);
			msg.arg1 = 0;
			this.sendMessage(msg);
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Helper method to send an Add Rule request to the message Handler
	 */
	public void addRule(Bundle b) {
		Message msg = this.obtainMessage(ADD_RULE);
		msg.setData(b);
		msg.arg1 = 1;
		this.sendMessage(msg);
	}

	/**
	 * Helper method to send an Remove Rules request to the message Handler
	 */
	public void removeAllRules() {
		this.sendEmptyMessage(REMOVE_ALL_RULES);
	}
    

	/**
	 * Helper method to start the introspection process on a remote AllJoyn Appliction
	 */
    public void introspect(String sessionName, int sessionId, String path, DescriptionParser node) {
    	try{
        	String xml = doIntrospection(sessionName, path, sessionId);
        	if(xml == null) {
        		/** The device must not support the new org.allseen.Introspectable interface */
        		return;
        	}
	    	node.parse(xml);

	    	/*
	    	 * Parsing complete, collect the actions and assing them into the remoteInfo object
	    	 */
	    	Device info = remoteInfo.get(sessionName);
	    	for(Description eai : node.getActions()) {
		    	info.addAction((ActionDescription)eai);
	    	}
	    	for(Description eai : node.getEvents()) {
	    		info.addEvent((EventDescription)eai);
	    	}
	    	/*
	    	 * Introspect then parse the children paths
	    	 */
	    	for(DescriptionParser in : node.getChidren()) {
	    		/*
	    		 * Recursively call and process the child nodes looking for actions and events
	    		 */
	    		introspect(sessionName, sessionId, in.getPath(), in);
	    	}
	    	
	    	/*
	    	 * If the root node then parsing complete, inform listener of events/actions found
	    	 */
	    	if(node.getPath().equals("/")) { //root node, recursive parsing complete 
	    		if(info.getActions().size() > 0) { //We have actions so inform as such to the UI
	    			listener.onActionsFound(info);
	    		}
	    		if(info.getEvents().size() > 0) { //We have events so inform as such to the UI
	    			listener.onEventsFound(info);
	    		}
	    	}
    	}catch(Exception e) {
    		e.printStackTrace();
    	}
    }
    

	/**
	 * Helper method used by Handler to call into JNI to save the rule.
	 */
    private void doAddRule(Bundle b, boolean persist) {
    	Log.d(TAG, "Save the rule!");
    	addRule(//event
    			b.getString("eUniqueName"),
				b.getString("ePath"),
				b.getString("eIface"),
				b.getString("eMember"),
				b.getString("eSig"),
				//action
				b.getString("aUniqueName"),
				b.getString("aPath"),
				b.getString("aIface"),
				b.getString("aMember"),
				b.getString("aSig"),
				persist);
    }
    

	/**
	 * Helper method used by Handler to call into JNI to add a saved rule.
	 * Saved Rules will not try and re-save to persitent memory again.
	 */
    private void doAddSavedRule(Bundle b) {
    	Log.d(TAG, "Adding Saved rule");
    	addSavedRule(//event
    			b.getString("eUniqueName"),
				b.getString("ePath"),
				b.getString("eIface"),
				b.getString("eMember"),
				b.getString("eSig"),
				b.getString("eDeviceId"),
				b.getString("eAppId"),
				b.getShort("ePort"),
				//action
				b.getString("aUniqueName"),
				b.getString("aPath"),
				b.getString("aIface"),
				b.getString("aMember"),
				b.getString("aSig"),
				b.getString("aDeviceId"),
				b.getString("aAppId"),
				b.getShort("aPort"));
    }
    
	/**
	 * Handles the messages so that the UI thread is not blocked
	 */
    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
        /* Connect to the bus and start our service. */
        case INITIALIZE:
        { 
        	initialize(mContext.getPackageName());
        	break;
        }
        case START_RULE_ENGINE:
        {
        	startRuleEngine();
        	break;
        }
        case INTROSPECT:
        {
        	try{
	        	String sessionName = msg.getData().getString("name");
	        	Device device = new Device(sessionName, msg.arg1, msg.getData().getString("friendly"), "/");
	        	remoteInfo.put(sessionName, device);
	        	introspect(device.getSessionName(),
	        			device.getSessionId(),
	        			device.getPath(),
	        			new DescriptionParser(device.getPath())
	        			);
        	} catch( Exception e ) {
        		e.printStackTrace();
        	}
        	break;
        }
        case ADD_RULE:
        {
        	try{
        		doAddRule(msg.getData(), msg.arg1 == 1);
        	} catch( Exception e ) {
        		e.printStackTrace();
        	}
        	break;
        }
        case ADD_SAVED_RULE:
        {
        	try{
        		doAddSavedRule(msg.getData());
        	} catch( Exception e ) {
        		e.printStackTrace();
        	}
        	break;
        }
        case REMOVE_ALL_RULES:
        {
        	try{
        		deleteAllRules();
        	} catch (Exception e) {
        		e.printStackTrace();
        	}
        	break;
        }
        /* Release all resources acquired in connect. */
        case SHUTDOWN: {
            shutdown();
            break;   
        }

        default:
            break;
        }
    }
    
    
}
