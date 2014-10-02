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

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Vector;

import javax.xml.parsers.ParserConfigurationException;

import org.allseen.sample.eventaction.EventActionListener;
import org.allseen.sample.eventaction.Rules;
import org.allseen.sample.eventaction.RulesFragment;

import org.allseen.sample.eventaction.Device;
import org.xml.sax.SAXException;

import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class BusHandler extends Handler {
	public static final String TAG = "EventBusHandler";
	
	private Context mContext;
	private EventActionListener listener;

    /* These are the messages sent to the BusHandler from the UI/Application thread. */
    public static final int INTROSPECT = 1;
    public static final int CALL_ACTION = 2;
    public static final int ENABLE_EVENT = 3;
    public static final int SHUTDOWN = 100;
   
    private ArrayList<Rules> pendingRuleList = new ArrayList<Rules>();
    
    private HashMap<String,Device> remoteInfo = new HashMap<String, Device>();
    
    private HashMap<String,String> didFind = new HashMap<String,String>();
   
    private String getParentDir() {
	    String parentDirPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/DEMO";
	    File parentDir = new File(parentDirPath);
	    if (!parentDir.exists()) {
	        parentDir.mkdirs();
	    }
	    return parentDirPath;
	}
    
    /*
     * Native JNI methods
     */
    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("MyAllJoynCode");
    }

	 /** Initialize AllJoyn in JNI(C++) */
    public native void initialize(String packageName);
    
    /** Perform introspection with description in JNI(C++) */
    private native String doIntrospection(String sessionName, String path, int sessionId);
    
    private native void introspectionDone(int sessionId);

    /** Enable listening for an event */
    private native boolean enableEvent(//event
								  String jEUniqueName, String jEPath,
								  String jEIface, String jEMember, String jESig);
    private native void callAction(
			//action
			  String jAUniqueName, String jAPath,
			  String jAIface, String jAMember, String jASig);
    
    public native void setEngine(String jEngineName);

    /** Clean up and shutdown AllJoyn in JNI(C++) */
    private native void shutdown();

    /**
     * Constructor
     */
    public BusHandler(Looper looper, Context context, EventActionListener listener) {
        super(looper);
        this.mContext = context;
        this.listener = listener;
    }
    
    /**
     * Callback from jni code invoked via an About Annouce callback
     * @param sessionName	name of the remote application
     * @param sessionId		sessionId that was set
     * @param friendlyName	name to display in the UI
     */
	public void foundEventActionApplication(final String sessionName, final int sessionId, final String friendlyName) {
		Log.d(TAG, "Found device with friendly name: "+friendlyName);
		this.post(new Runnable() { public void run() {
        	Device device = new Device(sessionName, sessionId, friendlyName, "/");
        	remoteInfo.put(sessionName, device);
        	try {
				introspect(device.getSessionName(),
						device.getSessionId(),
						device.getPath(),
						new DescriptionParser(device.getPath())
						);
			} catch (ParserConfigurationException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			} catch (SAXException e) {
				e.printStackTrace();
			}
		}});
	}
	
	public void failedJoinEventActionApplication(String sessionName) {
		Log.d(TAG, "Failed to join application: "+sessionName);
	}

    /**
     * Callback from jni code invoked when a device goes away
     * @param sessionId		sessionId that was lost
     */
	public void lostEventActionApplication(int sessionId) {
		Log.d(TAG, "Lost applicatoin with session id: "+sessionId);
		listener.onEventLost(sessionId);
		listener.onActionLost(sessionId);
	}
	
	public void callAction(Bundle b) {
		Message msg = this.obtainMessage(CALL_ACTION);
		msg.setData(b);
		msg.arg1 = 1;
		this.sendMessage(msg);
	}
	
	public void onEventReceived(final String from, final String path,
			  final String iface, final String member, final String sig) {
		Log.d(TAG, "Received an event from "+from+" - "+path+" "+iface+"::"+member+"("+sig+")");
		try{
			Bundle b;
			for(Rules rule : RulesFragment.getRules()) {
				EventDescription e = rule.getEvent();
				if(e.getIface().compareTo(iface) == 0 && e.getMemberName().compareTo(member) == 0
						&& e.getSignature().compareTo(sig) == 0) {
					for(ActionDescription a : rule.getActions()) {
						b = new Bundle();
						//action
						b.putString("aUniqueName", a.getSessionName());
						b.putString("aPath", a.getPath());
						b.putString("aIface", a.getIface());
						b.putString("aMember", a.getMemberName());
						b.putString("aSig", a.getSignature());
						callAction(b);
					}
				}
			}
		}catch(Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Helper method to send an Add Rule request to the message Handler
	 */
	public void enableEvent(Rules r) {
    	for(Rules rule : RulesFragment.getRules()) {
			EventDescription e = rule.getEvent();
			if(e.getIface().compareTo(r.getEvent().getIface()) == 0 && e.getMemberName().compareTo(r.getEvent().getMemberName()) == 0
					&& e.getSignature().compareTo(r.getEvent().getSignature()) == 0) {
				//If we have already enabled the listener, then do not do it again
				return;
			}
    	}
		Message msg = this.obtainMessage(ENABLE_EVENT);
		msg.obj = r;
		this.sendMessage(msg);
	}
	
	public void enablePendingRules(String sessionName, String friendlyName) {
		int currLen = pendingRuleList.size();
		/*
		 * The nature of this loop is to try and add each pending element
		 * and in the rawEnable call if it fails it will be added back into
		 * the list of pending rules to be enabled
		 */
		for(int i = 0; i < currLen; i++) {
			Rules rule = pendingRuleList.remove(0);
			for(int j = 0; j < rule.getActions().size(); j++) {
				if(rule.getActions().elementAt(j).getFriendlyName().equals(friendlyName)) {
					//should base this off of deviceId+appId
					rule.getActions().elementAt(j).setSessionName(sessionName);
				}
			}
			boolean ret = enableEvent(
					//event
	    			rule.getEvent().getSessionName(),
	    			rule.getEvent().getPath(),
	    			rule.getEvent().getIface(),
	    			rule.getEvent().getMemberName(),
	    			rule.getEvent().getSignature());
			if(!ret) {
				pendingRuleList.add(rule);
			}
		}
	}
	
	public boolean rawEnable(Rules r) {
		for(Rules rule : RulesFragment.getRules()) {
			EventDescription e = rule.getEvent();
			if(e.getIface().compareTo(r.getEvent().getIface()) == 0 && e.getMemberName().compareTo(r.getEvent().getMemberName()) == 0
					&& e.getSignature().compareTo(r.getEvent().getSignature()) == 0) {
				//If we have already enabled the listener, then do not do it again
				return false;
			}
    	}
		boolean ret = enableEvent(
				//event
    			r.getEvent().getSessionName(),
    			r.getEvent().getPath(),
    			r.getEvent().getIface(),
    			r.getEvent().getMemberName(),
    			r.getEvent().getSignature());
		if(!ret) {
			pendingRuleList.add(r);
		}
		return ret;
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
	    	for(Description eai : node.getEvents()) {
		    	info.addEvent((EventDescription)eai);
	    	}
	    	for(Description eai : node.getActions()) {
		    	info.addAction((ActionDescription)eai);
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
	    		if(info.getEvents().size() > 0) { //We have events so inform as such to the UI
	    			listener.onEventFound(info);
	    		}
	    		if(info.getActions().size() > 0) { //We have actions so inform as such to the UI
	    			listener.onActionsFound(info);
	    		}
    			introspectionDone(sessionId);
	    	}
    	}catch(Exception e) {
    		e.printStackTrace();
    	}
    }
    

	/**
	 * Helper method used by Handler to call into JNI to save the rule.
	 */
    private void doCallAction(Bundle b) {
    	callAction(
				//action
				b.getString("aUniqueName"),
				b.getString("aPath"),
				b.getString("aIface"),
				b.getString("aMember"),
				b.getString("aSig"));
    }
    private boolean doEnableEvent(Rules r) {
    	return enableEvent(
				//event
    			r.getEvent().getSessionName(),
    			r.getEvent().getPath(),
    			r.getEvent().getIface(),
    			r.getEvent().getMemberName(),
    			r.getEvent().getSignature());
    }
    
	/**
	 * Handles the messages so that the UI thread is not blocked
	 */
    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
        /* Connect to the bus and start our service. */
        case INTROSPECT:
        {
        	try{
        		try{Thread.sleep(200); }catch(Exception e) {}
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
        case ENABLE_EVENT:
        {
        	try{
        		doEnableEvent((Rules)msg.obj);
        	} catch( Exception e ) {
        		e.printStackTrace();
        	}
        	break;
        }
        case CALL_ACTION:
        {
        	try{
        		doCallAction(msg.getData());
        	} catch( Exception e ) {
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
