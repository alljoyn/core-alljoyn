/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
package org.allseen.sample.eventaction;

import android.app.Application;

public class MyApplication extends Application implements EventActionListener {
	EventActionListener mChainedListener = null;
	 public void onCreate() {
	
	 }
	 
	 public void setChainedListener(EventActionListener listener) {
		 mChainedListener = listener;
	 }
	 
	 @Override
	public void onEventFound(Device info) {
		 if(mChainedListener != null)
			 mChainedListener.onEventFound(info);
	}

	@Override
	public void onActionsFound(Device info) {
		if(mChainedListener != null)
			 mChainedListener.onActionsFound(info);
	}
	
	@Override
	public void onRuleEngineFound(final String sessionName, final String friendlyName) {
		if(mChainedListener != null)
			 mChainedListener.onRuleEngineFound(sessionName, friendlyName);
	}

	@Override
	public void onAppLost(String busName) {
		if(mChainedListener != null)
			 mChainedListener.onAppLost(busName);
	}
	
	@Override
	public void onAppReturned(String busName) {
		if(mChainedListener != null)
			 mChainedListener.onAppReturned(busName);
	}
}