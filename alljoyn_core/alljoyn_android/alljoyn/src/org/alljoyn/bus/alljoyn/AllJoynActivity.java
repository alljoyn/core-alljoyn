/*
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.alljoyn;

import org.alljoyn.bus.alljoyn.AllJoynApp;
import org.alljoyn.bus.alljoyn.DialogBuilder;

import android.app.Activity;
import android.app.Dialog;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class AllJoynActivity extends Activity {
    private static final String TAG = "alljoyn.AllJoynActivity";

    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "onCreate()");
    	super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        mExitButton = (Button)findViewById(R.id.mainExit);
        mExitButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showDialog(DIALOG_EXIT_ID);
        	}
        });
        
        mApp = (AllJoynApp)getApplication();
        
        if (mApp.running() == false) {
            boolean waitForDebuggerToSettle = false;
            if (waitForDebuggerToSettle) {
            	showDialog(DIALOG_DEBUG_ID);
            } else {
            	mApp.ensureRunning();
            }
        }
    }
    
	public void onDestroy() {
        Log.i(TAG, "onDestroy()");
    	super.onDestroy();
 	}
	
    public static final int DIALOG_EXIT_ID = 0;
    private static final int DIALOG_DEBUG_ID = 1;

    protected Dialog onCreateDialog(int id) {
    	Log.i(TAG, "onCreateDialog()");
        switch(id) {
        case DIALOG_EXIT_ID:
	        { 
	        	DialogBuilder builder = new DialogBuilder();
	        	return builder.createMainExitDialog(this, mApp);
	        }        	
        case DIALOG_DEBUG_ID:
            {
	        	DialogBuilder builder = new DialogBuilder();
	        	return builder.createDebugSettleDialog(this, mApp);
            }
        }
        assert(false);
        return null;
    }
	   
    private AllJoynApp mApp;
    private Button mExitButton;
}
