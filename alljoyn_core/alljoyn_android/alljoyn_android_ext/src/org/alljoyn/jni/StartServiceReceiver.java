/**
 * @file
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.jni;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.util.Log;

public class StartServiceReceiver extends BroadcastReceiver{
	
	public StartServiceReceiver(){
		super();
	}
	
	@Override
	public void onReceive(Context c, Intent intent){
			
			Intent serviceIntent = new Intent();
			serviceIntent.setAction("org.alljoyn.jni.alljoynandroidext");
			c.startService(serviceIntent);
		
	}
}