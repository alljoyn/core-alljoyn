/*
 * Copyright (c) 2010-2012, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.bundle.tests.clientbundle;

import java.util.UUID;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.Dialog;
import android.graphics.Color;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class Client extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    public static final int DIALOG_DEBUG_ID = 0;

    private static final String TAG = "BundledClient";

    private static final int PASS_COLOR = Color.GREEN;
    private static final int FAIL_COLOR = Color.RED;
    private static final int WARN_COLOR = Color.YELLOW;

    private static final String TEST_COMPLETE = "Test complete";
    private static final String RESULT_PASS = "Result:Pass";
    private static final String RESULT_FAIL="Result:Fail";
    private static final String RESULT_BLOCK="Result:Block";
    private static final String INFO = "Info:";


    private static final String CONNECT_FAIL_CS = "Connection Fail";
    private static final String FIND_FAIL_CS = "Discover Fail";
    private static final String JOIN_FAIL_CS = "JoinSession Fail";
    private static final String CALL_FAIL_CS = "MethodCalls Fail";
    private static final String FINDING_CS = "Discovering...";
    private static final String FIND_TIMEOUT_CS ="Discover timeout";

    private static final String WIFI_OFF = "Wifi disabled!";
    private static final String AP_OFF = "No access point connected!";
    private static final String SERVICE_OFF = "Service app not launched or quit!";

    private static final int CLIENT_OK = 1;
    private static final int CONNECT_FAIL = 2;
    private static final int FIND_FAIL = 3;
    private static final int JOIN_FAIL = 4;
    private static final int CALL_FAIL = 5;
    private static final int DISCONNECT_OK = 6;
    private static final int FINDING = 7;
    private static final int DISCOVERY_TIMEOUT = 8;

    private TextView mTextView;
    private Button mDbgButton;
    private Menu menu;

    private boolean mIsWifiEnabled = true;
    private String mActiveApMac = null;

    /* Create Pass text */
    private String setPassText()
    {
        StringBuilder sb = new StringBuilder();

        sb.append(TEST_COMPLETE);
        sb.append("\n");
        sb.append(RESULT_PASS);

        return sb.toString();
    }

    /* Create failure text */
    private String setFailText(String info)
    {
        StringBuilder sb = new StringBuilder();

        sb.append(TEST_COMPLETE);
        sb.append("\n");
        sb.append(RESULT_FAIL);
        sb.append("\n");
        sb.append(INFO).append(info);

        return sb.toString();
    }

    /* Create blocked text */
    private String setBlockText(String info)
    {
        StringBuilder sb = new StringBuilder();

        sb.append(TEST_COMPLETE);
        sb.append("\n");
        sb.append(RESULT_BLOCK);
        sb.append("\n");
        sb.append(INFO).append(info);

        return sb.toString();
    }

    /* UI handler */
    private Handler mHandler = new Handler()
    {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            /* Client discovery, joinSession and method call pass */
            case CLIENT_OK:
                mTextView.setText(setPassText());
                mTextView.setTextColor(PASS_COLOR);
                mDbgButton.setEnabled(false);

                break;

            /* Failure in red: bundled daemon cause connect fail */
            case CONNECT_FAIL:
                mTextView.setText(setFailText(CONNECT_FAIL_CS));
                mTextView.setTextColor(FAIL_COLOR);
                mDbgButton.setEnabled(false);
                break;

            case FINDING:
                mTextView.setText(FINDING_CS);
                break;

            /* Warning in yellow */
            case FIND_FAIL:
                mTextView.setText(setBlockText(FIND_FAIL_CS));
                mTextView.setTextColor(WARN_COLOR);
                mDbgButton.setEnabled(false);
                break;

            /* Warning in yellow */
            case JOIN_FAIL:
                mTextView.setText(setBlockText(JOIN_FAIL_CS));
                mTextView.setTextColor(WARN_COLOR);
                /* Display debug dialog */
                mDbgButton.setEnabled(true);
                break;

            /* Warning in yellow */
            case CALL_FAIL:
                mTextView.setText(setBlockText(CALL_FAIL_CS));
                mTextView.setTextColor(WARN_COLOR);
                mDbgButton.setEnabled(false);
                break;
            /* Warning in yellow */
            case DISCOVERY_TIMEOUT:
            	/* Stop discover */
            	mBusHandler.sendEmptyMessage(BusHandler.CANCEL_DISCOVER);

                mTextView.setText(setBlockText(FIND_TIMEOUT_CS));
                mTextView.setTextColor(WARN_COLOR);
                /* Display debug dialog */
                mDbgButton.setEnabled(true);
                break;

            case DISCONNECT_OK:
            	finish();
            	break;

            default:
                break;
            }
        }
    };

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private Handler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mTextView = (TextView) findViewById(R.id.textClient);

        mDbgButton = (Button)findViewById(R.id.dbgButton);
        mDbgButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showDialog(DIALOG_DEBUG_ID);
        	}
        });
        mDbgButton.setEnabled(false);

        /* Get wifi ap mac address for debug purpose */
        mActiveApMac = getWifiMacAddress();

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
    }


    protected Dialog onCreateDialog(int id) {
    	Log.i(TAG, "onCreateDialog()");
        Dialog result = null;
        switch(id) {
        case DIALOG_DEBUG_ID:
	        {
	        	DialogBuilder builder = new DialogBuilder();
	        	result = builder.createDebugDialog(this, getBlockReason());
	        }
        	break;
        default:
            result = null;
            break;
        }
        return result;
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        this.menu = menu;
        return true;
    }

    @Override
	public boolean onOptionsItemSelected(MenuItem item) {
	    // Handle item selection
	    switch (item.getItemId()) {
	    case R.id.quit:
	    	mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);

	        return true;
	    default:
	        return super.onOptionsItemSelected(item);
	    }
	}

    @Override
    protected void onDestroy() {
        super.onDestroy();

    }

    /* Analyze possible block reason */
    private String getBlockReason()
    {
    	String blockReason = null;

    	if (mIsWifiEnabled)
    	{
    		if (null == mActiveApMac)
    		{
    			/* No access point is connected */
    			blockReason = AP_OFF;
    		}
    		else
    		{
    			/* Service app is not launched or quit */
    			blockReason = SERVICE_OFF;
    		}
    	}
    	else
    	{
    		/* Wifi disabled */
    		blockReason = WIFI_OFF;
    	}

    	return blockReason;
    }

    /* Get wifi active access point mac address */
    private String getWifiMacAddress()
    {
    	String apMac = null;

    	WifiManager mWifiManager = (WifiManager) getSystemService(getApplicationContext().WIFI_SERVICE);

    	mIsWifiEnabled = mWifiManager.isWifiEnabled();

    	if (mIsWifiEnabled)
    	{
    		WifiInfo wifiInfo = mWifiManager.getConnectionInfo();

    		apMac = wifiInfo.getBSSID();

    		logInfo("Active access point mac address: " + apMac);
    	}
    	else
    	{
    		logInfo("Wifi disabled!");
    	}

    	return apMac;
    }

    /* This class will handle all AllJoyn calls. See onCreate(). */
    class BusHandler extends Handler {
        /*
         * Name used as the well-known name and the advertised name.  This name must be a unique name
         * both to the bus and to the network as a whole.  The name uses reverse URL style of naming.
         */
        private static final String SERVICE_NAME = "org.alljoyn.bus.bundle.tests.ping";
        private static final short CONTACT_PORT=42;

        private ProxyBusObject mProxyObj;
        private ServiceInterface mServiceInterface;

        private BusAttachment mBus;

        private int 	mSessionId;
        private boolean mIsConnected;
        private boolean mIsStoppingDiscovery;

        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;
        public static final int JOIN_SESSION = 3;
        public static final int CANCEL_DISCOVER = 4;

        public static final int CALLS = 20;
        public static final int TIMEOUT_SEC=20;

        private Runnable mTimeoutTask = new Runnable() {
     	   public void run() {
     		   logInfo("Discovery timeout " + TIMEOUT_SEC +" seconds!");
     	       mHandler.sendEmptyMessage(DISCOVERY_TIMEOUT);
     	   }
     	};

        public BusHandler(Looper looper) {
            super(looper);
            mIsConnected = false;
        }

        private void startDiscoverTimer()
        {
        	// Start a 20 seconds timer
            //removeCallbacks(mTimeoutTask);
        	logInfo("Start discovery timer in bus handler");
            mHandler.postDelayed(mTimeoutTask, TIMEOUT_SEC*1000);

        }
        private void stopDiscoverTimer()
        {
        	//removeCallbacks(mTimeoutTask);
        	logInfo("Stop discovery timer because name found");
        	mHandler.removeCallbacks(mTimeoutTask);
        }

        @Override
        public void handleMessage(Message msg) {
        	Status status = Status.OK;

            switch (msg.what) {
            /* Connect to the bus and start our service. */
            case CONNECT: {

                mBus = new BusAttachment(getApplicationContext().getPackageName(),
                			BusAttachment.RemoteMessage.Receive);
                /*
                 * Create a bus listener class
                 */
                /*
                 * Create a bus listener class
                 */
                mBus.registerBusListener(new BusListener() {
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                    	logInfo(String.format("MyBusListener.foundAdvertisedName(%s, 0x%04x, %s)", name, transport, namePrefix));

                    	stopDiscoverTimer();

                    	/*
                    	 * This client will only join the first service that it sees advertising
                    	 * the indicated well-known name.  If the program is already a member of
                    	 * a session (i.e. connected to a service) we will not attempt to join
                    	 * another session.
                    	 * It is possible to join multiple session however joining multiple
                    	 * sessions is not shown in this sample.
                    	 */
                    	if(!mIsConnected) {
                    	    Message msg = obtainMessage(JOIN_SESSION, name);
                    	    sendMessage(msg);
                    	}
                    }
                });

                /*
                 * The next step in making a service available to other AllJoyn peers is to connect the
                 * BusAttachment to the bus with a well-known name.
                 */
                /*
                 * connect the BusAttachement to the bus
                 */
                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (status != Status.OK)
                {
                	mHandler.sendEmptyMessage(CONNECT_FAIL);
                    return;
                }

                /*
                 * Now find an instance of the AllJoyn object we want to call.  We start by looking for
                 * a name, then connecting to the device that is advertising that name.
                 *
                 * In this case, we are looking for the well-known SERVICE_NAME.
                 */
                status = mBus.findAdvertisedName(SERVICE_NAME);
                logStatus(String.format("BusAttachement.findAdvertisedName(%s)", SERVICE_NAME), status);
                if (Status.OK != status) {
                	mHandler.sendEmptyMessage(FIND_FAIL);
                	return;
                }

                mHandler.sendEmptyMessage(FINDING);

                // Start a 20 seconds timer
                startDiscoverTimer();

                break;


            }
            case (JOIN_SESSION): {
            	/*
                 * If discovery is currently being stopped don't join to any other sessions.
                 */
                if (mIsStoppingDiscovery) {
                    break;
                }

                short contactPort = CONTACT_PORT;
                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.transports = SessionOpts.TRANSPORT_WLAN;
                Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

                status = mBus.joinSession((String) msg.obj, contactPort, sessionId, sessionOpts, new SessionListener() {
                    @Override
                    public void sessionLost(int sessionId) {
                        mIsConnected = false;
                        logInfo(String.format("MyBusListener.sessionLost(%d)", sessionId));

                    }
                });

                logStatus("BusAttachment.joinSession() - sessionId: " + sessionId.value, status);

                if (status != Status.OK)
                {
                	mHandler.sendEmptyMessage(JOIN_FAIL);
                	return;
                }

                mProxyObj =  mBus.getProxyBusObject(SERVICE_NAME,
                									"/BundledService",
                									sessionId.value,
                									new Class<?>[] { ServiceInterface.class });

                /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
                mServiceInterface =  mProxyObj.getInterface(ServiceInterface.class);

                mSessionId = sessionId.value;
                mIsConnected = true;

                String input="";
                String reply="";
                int passedCalls=0;

                try {
                	for(int i = 0; i < CALLS; i++)
                	{
                		// Create 20 random strings and check the return
                		input="bundle" + UUID.randomUUID().toString().substring(0, 5);
                		reply = mServiceInterface.Ping(input);

                		// Reply should be same as input
                		if (input.equals(reply))
                		{
                			logInfo("Reply:" + reply + " match input: " + input);
                			passedCalls++;
                		}
                		else
               			{
               				logInfo("Reply:" + reply + " mismatch input: " + input);
               			}
               		}

				} catch (BusException ex) {
	                logException("SimpleInterface.Ping()", ex);

	            }

				if (passedCalls == CALLS)
				{
					mHandler.sendEmptyMessage(CLIENT_OK);
				}
				else
				{
					mHandler.sendEmptyMessage(CALL_FAIL);
				}

                break;
            }

            /* Cancel find advertised name */
            case CANCEL_DISCOVER:
            	mIsStoppingDiscovery = true;
            	status = mBus.cancelFindAdvertisedName(SERVICE_NAME);
            	logStatus("BusAttachment.cancelFindAdvertisedName()", status);
            	break;

            /* Release all resources acquired in connect. */
            case DISCONNECT: {

            	mIsStoppingDiscovery = true;
            	if (mIsConnected) {
                	status = mBus.leaveSession(mSessionId);
                    logStatus("BusAttachment.leaveSession()", status);
            	}
                mBus.disconnect();
                getLooper().quit();

                mHandler.sendEmptyMessage(DISCONNECT_OK);

                break;
            }

            default:
                break;
            }
        }
    }

    private void logStatus(String msg, Status status) {
        String log = String.format("%s: %s", msg, status);
        if (status == Status.OK) {
            Log.i(TAG, log);
        } else {
            Log.e(TAG, log);
        }
    }

    /*
     * print the status or result to the Android log. If the result is the expected
     * result only print it to the log.  Otherwise print it to the error log and
     * Sent a Toast to the users screen.
     */
    private void logInfo(String msg) {
            Log.i(TAG, msg);
    }

    private void logException(String msg, BusException ex) {
        String log = String.format("%s: %s", msg, ex);

        Log.e(TAG, log, ex);
    }
}
