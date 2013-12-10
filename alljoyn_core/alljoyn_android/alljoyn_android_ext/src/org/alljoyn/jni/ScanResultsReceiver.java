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

import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.util.Log;


import java.util.ArrayList;
import java.util.List;



// This guys needs access to the Map which the original ProximityService is going to send back to the native code


public class ScanResultsReceiver extends BroadcastReceiver{
	
	AllJoynAndroidExt jniProximity;
	AlarmManager alarmMgr;
	//
	// Need a constructor that will take the instance on Proximity Service
	//
	public ScanResultsReceiver(AllJoynAndroidExt jniProximity){
		super();
		this.jniProximity = jniProximity; 
		//alarmMgr = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
	}
	public ScanResultsReceiver(){
		super();
	}
	
	@Override
	public void onReceive(Context c, Intent intent){
		
		if(intent.getAction().equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)){
			
			
			Log.v("ScanResultsReceiver", "SCAN_RESULTS_AVAILABLE_ACTION received");
			
			List<ScanResult> scanResults = jniProximity.wifiMgr.getScanResults();
			//
			// If not access point were returned then we
			//
			if(scanResults.size() == 0){
				Log.v("ScanResultsReceiver", "Result size = 0");
				jniProximity.scanResultsObtained = true;
				return;
			}
			else{
				Log.v("ScanResultsReceiver", "Result size  = " + Integer.toString(scanResults.size()));
			}
			
			
			jniProximity.lastScanTime = System.currentTimeMillis();
			
			// If scan WAS requested we thrash all old results and just create and entirely new scan Results map
			// If scan WAS NOT requested we append the results obtained from this scan to our original result
			
			jniProximity.scanResultMessage = new ScanResultMessage[scanResults.size()];
						
			String currentBSSID = jniProximity.wifiMgr.getConnectionInfo().getBSSID();
			int currentBSSIDIndex = 0;
			
			for (ScanResult result : scanResults){

					jniProximity.scanResultMessage[currentBSSIDIndex] = new ScanResultMessage();
					jniProximity.scanResultMessage[currentBSSIDIndex].bssid = result.BSSID;
					jniProximity.scanResultMessage[currentBSSIDIndex].ssid = result.SSID;
					if(currentBSSID !=null && currentBSSID.equals(result.BSSID)){
						jniProximity.scanResultMessage[currentBSSIDIndex].attached = true;
					}
					currentBSSIDIndex++;
					
			}
			currentBSSIDIndex = 0;
			jniProximity.scanResultsObtained = true;
		}
	
	}

}
