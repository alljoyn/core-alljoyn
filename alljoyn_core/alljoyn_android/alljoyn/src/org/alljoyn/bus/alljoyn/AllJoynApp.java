/*
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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

import android.app.Application;

import android.content.SharedPreferences; 
import android.content.ComponentName;

import android.content.Intent;
import android.content.res.Configuration;

import android.util.Log;

import java.util.ArrayList;

import org.alljoyn.bus.alljoyn.AllJoynService;

public class AllJoynApp extends Application {
    private static final String TAG = "alljoyn.AllJoynApp";
    
    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "onCreate()");
        
        /*
         * Update the shared preferences for this app.
         */
        updatePrefs();

        /*
         * Start a service to represent AllJoyn to the Android system.  We
         * aren't interested in getting involved with Service lifecycle since
         * our goal is really to reproduce a system daemon; so we just spin
         * up the daemon when the application is run.  We use the Android 
         * Service to convince the application framework not to kill us.  In
         * particular we set the service priority to foreground.
         */
        Intent intent = new Intent(this, AllJoynService.class);
        mRunningService = startService(intent);
        if (mRunningService == null) {
            Log.i(TAG, "onCreate(): failed to startService()");
        }
        
        ensureRunning();
	}
    
    ComponentName mRunningService = null;
    
    public void exit() {
        Log.i(TAG, "exit()");
        System.exit(0);
    }
    
    private void updatePrefs() {
        Log.i(TAG, "updatePrefs()");
        
        SharedPreferences sharedPreferences = getSharedPreferences("AllJoynPreferences", MODE_PRIVATE);

        /*
         * When calling into the daemon, use this as the program name
         */
        mName = sharedPreferences.getString("name", "alljoyn-daemon"); 
        Log.i(TAG, "updatePrefs(): name = " + mName);
        
        /*
         * When calling into the daemon use the --session flag if true
         */
        mSession = sharedPreferences.getBoolean("session", false);
        Log.i(TAG, "updatePrefs(): session = " + mSession);

        /*
         * When calling into the daemon use the --system flag if true
         */
        mSystem = sharedPreferences.getBoolean("system", false);
        Log.i(TAG, "updatePrefs(): system = " + mSystem);
        
        /*
         * If not --internal, use this configuration
         */
        mConfig = sharedPreferences.getString("config", 
                                              "<busconfig>" +
                                              "  <listen>unix:abstract=alljoyn</listen>" +
                                              "  <listen>launchd:env=DBUS_LAUNCHD_SESSION_BUS_SOCKET</listen>" +
                                              "  <listen>tcp:addr=0.0.0.0,port=9955,family=ipv4</listen>" +
                                              "  <limit auth_timeout=\"20000\"/>" +
                                              "  <limit max_incomplete_connections=\"4\"/>" +
                                              "  <limit max_completed_connections=\"16\"/>" +
                                              "  <property ns_interfaces=\"*\"/>" +
                                              "</busconfig>");
        Log.i(TAG, "updatePrefs(): config = " + mConfig);
        
        /*
         * When calling into the daemon use the --no-bt flag if true
         */
        mNoBT = sharedPreferences.getBoolean("no-bt", true);
        Log.i(TAG, "updatePrefs(): no-bt = " + mNoBT);
        
        /*
         * When calling into the daemon use the --no-tcp flag if true
         */
        mNoTCP = sharedPreferences.getBoolean("no-tcp", false);
        Log.i(TAG, "updatePrefs(): no-tcp = " + mNoTCP);
        
        /*
         * When calling into the daemon, use this verbosity level
         */
        mVerbosity = sharedPreferences.getString("verbosity", "7"); 
        Log.i(TAG, "updatePrefs(): verbosity = " + mVerbosity);
    }
      
    public boolean running() {
    	return mThread.isAlive();
    }
    
    public void ensureRunning() {
    	if (running() == false) {
    		startAllJoynDaemonThread();
    	}
    }
    
    public void startAllJoynDaemonThread() {
        Log.i(TAG, "startAllJoynDaemonThread()");
        
        Log.i(TAG, "startAllJoynDaemonThread(): Spinning up daemon thread.");
        mThread.start();
    }
     
    @Override
    public void onLowMemory() {
    	super.onLowMemory();
    }
    
    @Override
    public void onTerminate() {
    	super.onTerminate();
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
    	super.onConfigurationChanged(newConfig);
    }
   
    private boolean mSession = false;
    private boolean mSystem = false;
    private String mConfig = null;
    private boolean mNoBT = true;
    private boolean mNoTCP = false;
    private String mVerbosity = "7";
    private String mName = "alljoyn-daemon";

    private Thread mThread = new Thread() {
        public void run()
        {
            Log.i(TAG, "mThread.run()");
            Log.i(TAG,  AllJoynDaemon.getDaemonBuildInfo());            
            ArrayList<String> argv = new ArrayList<String>();
            argv.add(mName);
            argv.add("--config-service");          
            argv.add("--nofork");
            argv.add("--verbosity=" + mVerbosity);
                       
            if (mSystem) {
            	argv.add("--system");
            }
            
            if (mSession) {
            	argv.add("--session");
            }
            
            if (mNoBT) {
                argv.add("--no-bt");            	
            }
            
            if (mNoTCP) {
                argv.add("--no-tcp");            	
            }

            Log.i(TAG, "mThread.run(): calling runDaemon()"); 
            AllJoynDaemon.runDaemon(argv.toArray(), mConfig);
            Log.i(TAG, "mThread.run(): returned from runDaemon().  Self-immolating.");
            exit();
        }
    };
      
    static {
        System.loadLibrary("daemon-jni");
    }
}
