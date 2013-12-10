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

import android.app.Activity;
import android.app.Dialog;
import android.view.View;
import android.widget.Button;
import android.util.Log;

public class DialogBuilder {
    private static final String TAG = "alljoyn.Dialogs";
      
    public Dialog createMainExitDialog(Activity activity, final AllJoynApp application) {
       	Log.i(TAG, "createmainExitDialog()");
    	final Dialog dialog = new Dialog(activity);
    	dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
    	dialog.setContentView(R.layout.mainexitdialog);
	        	       	
    	Button yes = (Button)dialog.findViewById(R.id.mainExitOk);
    	yes.setOnClickListener(new View.OnClickListener() {
    		public void onClick(View view) {
    			dialog.cancel();
    			application.exit();
    		}
    	});
	            
    	Button no = (Button)dialog.findViewById(R.id.mainExitCancel);
    	no.setOnClickListener(new View.OnClickListener() {
    		public void onClick(View view) {
    			dialog.cancel();
    		}
    	});
    	
    	return dialog;
    }
    
    public Dialog createDebugSettleDialog(Activity activity, final AllJoynApp application) {
       	Log.i(TAG, "createmainExitDialog()");
    	final Dialog dialog = new Dialog(activity);
    	dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
    	dialog.setContentView(R.layout.debugsettledialog);
	        	       	
    	Button yes = (Button)dialog.findViewById(R.id.debugSettleOk);
    	yes.setOnClickListener(new View.OnClickListener() {
    		public void onClick(View view) {
    			application.ensureRunning();
    			dialog.cancel();
    		}
    	});
	            
    	Button no = (Button)dialog.findViewById(R.id.debugSettleCancel);
    	no.setOnClickListener(new View.OnClickListener() {
    		public void onClick(View view) {
    			application.ensureRunning();
    			dialog.cancel();
    		}
    	});
    	
    	return dialog;
    }
}
