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
package org.alljoyn.bus.bundle.tests.bundledservice;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.Color;
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


public class Service extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final int PASS_COLOR = Color.GREEN;
    private static final int FAIL_COLOR = Color.RED;
    private static final int WARN_COLOR = Color.YELLOW;

    private static final String TAG = "BundledService";
    private static final CharSequence SERVICE_PASS_CS="Service OK";
    private static final CharSequence CONNECT_FAIL_CS="Connection Fail";
    private static final CharSequence REGISTER_FAIL_CS="Register Fail";
    private static final CharSequence BIND_FAIL_CS="BIND Fail";
    private static final CharSequence ADVERTISE_FAIL_CS="BIND Fail";

    private static final int SERVICE_OK = 1;
    private static final int CONNECT_FAIL = 2;
    private static final int REGISTER_FAIL = 3;
    private static final int BIND_FAIL = 4;
    private static final int ADVERTISE_FAIL = 5;
    private static final int DISCONNECT_OK = 6;

    private TextView mTextView;
    private Button mButton;
    private Menu menu;

    /* UI handler */
    private Handler mHandler = new Handler()
    {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            /* Service connect, advertise and bind OK */
            case SERVICE_OK:
                mTextView.setText(SERVICE_PASS_CS);
                mTextView.setTextColor(PASS_COLOR);

                if (isClientBundleInstalled())
                {
                	mButton.setEnabled(true);
                };

                break;
            case CONNECT_FAIL:
                mTextView.setText(CONNECT_FAIL_CS);
                mTextView.setTextColor(FAIL_COLOR);
                break;

            case REGISTER_FAIL:
                mTextView.setText(REGISTER_FAIL_CS);
                mTextView.setTextColor(WARN_COLOR);
                break;

            case BIND_FAIL:
                mTextView.setText(BIND_FAIL_CS);
                mTextView.setTextColor(WARN_COLOR);
                break;

            case ADVERTISE_FAIL:
                mTextView.setText(ADVERTISE_FAIL_CS);
                mTextView.setTextColor(WARN_COLOR);
                break;

            case DISCONNECT_OK:
            	finish();
            	break;
            default:
                break;
            }
        }
    };

    /* The AllJoyn object that is our service. */
    private BundledService mBundledService;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private Handler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mTextView = (TextView) findViewById(R.id.textConnect);
        mButton = (Button) findViewById(R.id.button);

        mButton.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v)
            {
                // Launch client bundle app if installed
            	launchClientBundle();
            }
        });
        mButton.setEnabled(false);

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        /* Start our service. */
        mBundledService = new BundledService();
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
    }

    private boolean isClientBundleInstalled()
    {
    	boolean clientExist = false;

    	try{
    	     ApplicationInfo info = getPackageManager()
    	                             .getApplicationInfo("org.alljoyn.bus.bundle.tests.clientbundle", 0 );
    	     clientExist = true;
    	     //-- application exists
    	    } catch( PackageManager.NameNotFoundException e ){
    	     //-- application doesn't exist
    	    	logInfo("Client bundle app not installed!");
    	}

    	return clientExist;

    }
    private void launchClientBundle()
    {
    	{
    		// Launch client bundle app
    		Intent i = new Intent("android.intent.action.MAIN");
    		i.setComponent(new ComponentName("org.alljoyn.bus.bundle.tests.clientbundle", "org.alljoyn.bus.bundle.tests.clientbundle.Client"));

    		this.startActivity(i);
    	}
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

    /* The class that is our AllJoyn service.  It implements the ServiceInterface. */
    class BundledService implements ServiceInterface, BusObject {

        /*
         * This is the code run when the client makes a call to the Ping method of the
         * SimpleInterface.  This implementation just returns the received String to the caller.
         */
        public String Ping(String inStr) {

            return inStr;
        }

    }

    /* This class will handle all AllJoyn calls. See onCreate(). */
    class BusHandler extends Handler {
        /*
         * Name used as the well-known name and the advertised name.  This name must be a unique name
         * both to the bus and to the network as a whole.  The name uses reverse URL style of naming.
         */
        private static final String SERVICE_NAME = "org.alljoyn.bus.bundle.tests.ping";
        private static final short CONTACT_PORT=42;

        private BusAttachment mBus;

        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;

        public BusHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            /* Connect to the bus and start our service. */
            case CONNECT: {

                mBus = new BusAttachment(getApplicationContext().getPackageName(),
                			BusAttachment.RemoteMessage.Receive);
                /*
                 * Create a bus listener class
                 */
                mBus.registerBusListener(new BusListener());

                /*
                 * To make a service available to other AllJoyn peers, first register a BusObject with
                 * the BusAttachment at a specific path.
                 *
                 * Our service is the SimpleService BusObject at the "/SimpleService" path.
                 */
                Status status = mBus.registerBusObject(mBundledService, "/BundledService");
                logStatus("BusAttachment.registerBusObject()", status);
                if (status != Status.OK) {
                	mHandler.sendEmptyMessage(REGISTER_FAIL);
                    return;
                }


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
                 * Create a new session listening on the contact port of the chat service.
                 */
                Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);

                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = SessionOpts.TRANSPORT_WLAN;

                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        if (sessionPort == CONTACT_PORT) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                });
                logStatus(String.format("BusAttachment.bindSessionPort(%d, %s)",
                          contactPort.value, sessionOpts.toString()), status);
                if (status != Status.OK) {
                	mHandler.sendEmptyMessage(BIND_FAIL);
                    return;
                }

                /*
                 * request a well-known name from the bus
                 */
                int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;

                status = mBus.requestName(SERVICE_NAME, flag);
                logStatus(String.format("BusAttachment.requestName(%s, 0x%08x)", SERVICE_NAME, flag), status);
                if (status == Status.OK) {
                	/*
                	 * If we successfully obtain a well-known name from the bus
                	 * advertise the same well-known name
                	 */
                	status = mBus.advertiseName(SERVICE_NAME, SessionOpts.TRANSPORT_ANY);
                    logStatus(String.format("BusAttachement.advertiseName(%s)", SERVICE_NAME), status);
                    if (status != Status.OK) {
                    	/*
                         * If we are unable to advertise the name, release
                         * the well-known name from the local bus.
                         */
                        status = mBus.releaseName(SERVICE_NAME);
                        logStatus(String.format("BusAttachment.releaseName(%s)", SERVICE_NAME), status);
                        mHandler.sendEmptyMessage(ADVERTISE_FAIL);
                    	return;
                    }
                    else
                    {
                    	mHandler.sendEmptyMessage(SERVICE_OK);
                    }
                }

                break;
            }

            /* Release all resources acquired in connect. */
            case DISCONNECT: {
                /*
                 * It is important to unregister the BusObject before disconnecting from the bus.
                 * Failing to do so could result in a resource leak.
                 */
                mBus.unregisterBusObject(mBundledService);
                mBus.disconnect();
                mBusHandler.getLooper().quit();

                mHandler.sendEmptyMessage(DISCONNECT_OK);

                break;
            }

            default:
                break;
            }
        }
    }

    private void logStatus(String msg, Status status)
    {
        String log = String.format("%s: %s", msg, status);
        if (status == Status.OK) {
            Log.i(TAG, log);
        } else {
            Log.e(TAG, log);
        }
    }

    private void logInfo(String msg)
    {
        Log.i(TAG, msg);
    }
}
