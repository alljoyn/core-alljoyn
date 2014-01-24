/**
 * @file
 */

/******************************************************************************
 * Copyright (c) 2012,2014, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.proximity;

import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.util.Log;


public class AllJoynAndroidExt{
	 
	
	// For logging
	private static final String TAG = "AllJoynAndroidExt";
	public static Context ctx = null;
	
	// Moved here from bus handler
	
	BroadcastReceiver receiver;
	
	static WifiManager wifiMgr;
	
	AlarmManager alarmMgr;
	
	public long lastScanTime;
	
	public long currentTime;
	
	public Runnable tUpdateTimerRunnable;
	
	public Handler tHandler;
	
	final int NOT_CONNECTED_TO_ANY_NW = -1;
	
//	boolean startScan = false;
	static ScanResultMessage[] scanResultMessage;
	static ScanResultMessage[] scanResultMessageToBeSent;
	static boolean scanResultsObtained;
	static boolean scanRequested = false;
	static boolean stopScanRequested = false;
	static boolean isFirstScan=true;


	// This needs to be accessible by the function which periodically populates results in the map
	boolean getScanResultsCallCompleted = false;
	
	public void updateTimer(){
		
		//
		// Check if the time since the last scan has been 60 secs
		// If Not just reset the time since last scan to 60 secs
		// else
		// Request a scan
		//
		try{
			currentTime = System.currentTimeMillis();
			long timeSinceLastScan = currentTime - lastScanTime;
			if(timeSinceLastScan >= 60000){
				if(!wifiMgr.isWifiEnabled() || !wifiMgr.startScan()){
					Log.v(TAG,"startScan() returned error");
					lastScanTime = System.currentTimeMillis();
				}
				tHandler.postDelayed(tUpdateTimerRunnable, 60000);
			} else {
				tHandler.postDelayed(tUpdateTimerRunnable, 60000 - timeSinceLastScan);
			}
		} catch (SecurityException se){
			Log.e(TAG,"Possible missing permissions needed to run ICE over AllJoyn. Discovery over ICE in AllJoyn may not work due to this");
			se.printStackTrace();
		}
	}
	
	public AllJoynAndroidExt(Context sContext){
		ctx = sContext;
		wifiMgr = (WifiManager)ctx.getSystemService(Context.WIFI_SERVICE);
		
		try{
			if(!wifiMgr.startScan()){
				Log.v(TAG,"startScan() returned error");
			}
			
			if(tHandler == null){
			tHandler = new Handler();
			} 
		
			tUpdateTimerRunnable = new Runnable() {
	
				public void run() {
					// TODO Auto-generated method stub
					updateTimer();
				}
			};
		
		tHandler.postDelayed(tUpdateTimerRunnable, 0);
	
		if(alarmMgr == null){
			alarmMgr = (AlarmManager)ctx.getSystemService(Context.ALARM_SERVICE);
			Intent alarmIntent = new Intent(ctx, ScanResultsReceiver.class);
			PendingIntent pi = PendingIntent.getBroadcast(ctx, 0, alarmIntent, 0);
			alarmMgr.setRepeating(AlarmManager.RTC_WAKEUP, System.currentTimeMillis(), 1000 * 60 * 10, pi); // Millisec * Second * Minute
		}
		} catch (SecurityException se){
			Log.e(TAG,"Possible missing permissions needed to run ICE over AllJoyn. Discovery over ICE in AllJoyn may not work due to this");
			se.printStackTrace();
		}
	}
	
	public static void PrepareScanResults(){
	    //Log.v(TAG,"Inside PrepareScanResults()");
		
		// if scanResultMessage is null check whom are we connected to and send that name 
		
		if(scanResultMessage == null || (scanResultMessage.length == 0)){
			scanResultMessageToBeSent = new ScanResultMessage[0];
			return;
		}
			
		scanResultMessageToBeSent = new ScanResultMessage[scanResultMessage.length];
			
		for(int currentBSSIDIndex=0 ; currentBSSIDIndex < scanResultMessage.length ; currentBSSIDIndex++){
				
			scanResultMessageToBeSent[currentBSSIDIndex] = new ScanResultMessage();
			scanResultMessageToBeSent[currentBSSIDIndex].bssid = scanResultMessage[currentBSSIDIndex].bssid;
			scanResultMessageToBeSent[currentBSSIDIndex].ssid = scanResultMessage[currentBSSIDIndex].ssid;
			scanResultMessageToBeSent[currentBSSIDIndex].attached = scanResultMessage[currentBSSIDIndex].attached;
		}
		//Log.v(TAG,"Returning from PrepareScanResults()");	
			
	}
		
	// GetScanResults
	public static ScanResultMessage[] Scan(boolean request_scan){
			
			scanRequested = request_scan;
			boolean isConnected = false;
			
			// For test ony ... always on scan
			//request_scan = true;
			// Not requested scan results 
			if(ctx == null){
				PrepareScanResults();
				scanResultMessage = null;
				return scanResultMessageToBeSent;
			}
			// Else wifi is turned on and we can proceed with the scan
			try{
				if(scanResultMessage == null && request_scan){
				    //Requested Scan Results
				    // Only a start scan or timeout scan can restart the scan processing so we set this boolean
				    // stopScanRequested = false;
						wifiMgr = (WifiManager)ctx.getSystemService(Context.WIFI_SERVICE);
						if(wifiMgr == null){
							Log.v(TAG," Wifi Manager is NULL");	
						}
						boolean wifiEnabled = wifiMgr.isWifiEnabled(); 
						
						// This handles wifi not being enabled and hotspot being enabled
						
						if(!wifiEnabled){
							// return empty map since wifi is not enabled
							Log.v(TAG," Wifi not enabled");
							scanResultMessage = new ScanResultMessage[0];
							return scanResultMessage; 
						} else{
							Log.v(TAG," Wifi is enabled");
						}
					
						if(isFirstScan){
							if(wifiEnabled && wifiMgr.getConnectionInfo().getBSSID() != null){
								isConnected = true;
							}
							if(scanResultMessage == null && isConnected){
								scanResultMessage = new ScanResultMessage[1];
								scanResultMessage[0] = new ScanResultMessage();
								
								String currentBSSID = wifiMgr.getConnectionInfo().getBSSID();
								String currentSSID = wifiMgr.getConnectionInfo().getSSID();
								
								scanResultMessage[0].bssid = currentBSSID;
								scanResultMessage[0].ssid = currentSSID;
								scanResultMessage[0].attached = true;
							}
							PrepareScanResults();
							isFirstScan = false;
							return scanResultMessageToBeSent; 
						}
						
						scanResultsObtained = false;
		
	//					if(receiver == null){
	//						// Pass the map and the boolean scanResultsObtained here and use the same map to form the return message 
	//						receiver = new ScanResultsReceiver(this);
	//						registerReceiver(receiver, new IntentFilter(
	//								WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
	//					}
						
						if(!wifiMgr.startScan()){
							Log.v(TAG,"startScan() returned error");
						}
						else {
							Log.v(TAG,"startScan() call was successful");
						}
						
						// Check the boolean passed to the ScanResultsReceiver
						// If it was set then you can return the result 
						// Note : It can be the case that the scan did not return any results 
						while(true){
							Log.v(TAG,"Waiting for scanResultsObtained");
							if(scanResultsObtained){
								break;
							}
							else{
								try{	
									Thread.sleep(5000);
								}catch(InterruptedException ie){
									Log.v(TAG, "Thread was interrupted while it was sleeping");
								}
							}
						}
						//PrepareScanResults();
				}
				else{
					//
					// If ScanResultMessage is NULL and a scan was not requested then we need to check it we are still connected to a  
					// network. 
					// If Yes then we need to add this network to the scan results that are returned
					// If Not then we return what we have already received
					//
					
					// There is a corner case here where you are not connected to any network and Scan() function is called when 
					// it is ok to have scanResultMessage = null so we check if we are connected to any network first
					
					isConnected = false;
					
					boolean wifiEnabled = wifiMgr.isWifiEnabled(); 
					
					if(wifiEnabled && wifiMgr.getConnectionInfo() != null){
						isConnected = true;
					}
							
					if(scanResultMessage == null && isConnected){
						scanResultMessage = new ScanResultMessage[1];
						scanResultMessage[0] = new ScanResultMessage();
						
						String currentBSSID = wifiMgr.getConnectionInfo().getBSSID();
						String currentSSID = wifiMgr.getConnectionInfo().getSSID();
						
						scanResultMessage[0].bssid = currentBSSID;
						scanResultMessage[0].ssid = currentSSID;
						scanResultMessage[0].attached = true;
					}
					//PrepareScanResults();
				}
			}catch (SecurityException se){
				Log.e(TAG,"Possible missing permissions needed to run ICE over AllJoyn. Discovery over ICE in AllJoyn may not work due to this");
				se.printStackTrace();
			}
			PrepareScanResults();
			scanResultMessage = null;
			
			//Log.v(TAG,"************************FINAL SCAN RESULTS ****************************************");
			//for(int i=0 ; i < scanResultMessageToBeSent.length ; i++){
			//	ScanResultMessage result = scanResultMessageToBeSent[i];
			//	Log.v("Entry-->",result.bssid + " " + result.ssid + " " + result.attached);
			//}
			//Log.v(TAG,"*************************************************************************************");
			return scanResultMessageToBeSent;
		}
	
}
