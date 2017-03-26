/**
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/

package org.alljoyn.bus.samples.dynamicsignalclient;

import org.alljoyn.bus.AbstractDynamicBusObject;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.DynamicBusObject;
import org.alljoyn.bus.GenericInterface;
import org.alljoyn.bus.OnJoinSessionListener;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * This is the main Activity class for the DynamicSignalClient sample application.
 * <p>
 * This sample app is modeled after the existing SignalClient sample app.
 * <p>
 * The aim is to provide a "Dynamic Bus Object" version of the SignalClient sample,
 * which therefore can also illustrate the differences in implementing a dynamic bus object
 * versus a (static) bus object.
 * <p>
 * Therefore, much of this sample is equivalent to the SignalClient sample, with main
 * difference being how the bus interface (SimpleInterface) and the bus object
 * (SignalService) are implemented. Also note that with dynamic bus objects, signals
 * are emitted using the GenericInterface.class rather than specific bus interface classes.
 */
public class Client extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final int MESSAGE_PING = 1;
    private static final int MESSAGE_POST_TOAST = 2;
    private static final int MESSAGE_START_PROGRESS_DIALOG = 3;
    private static final int MESSAGE_STOP_PROGRESS_DIALOG = 4;

    private static final String TAG = "DynamicSignalClient";

    private EditText mEditText;
    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private BusHandler mBusHandler;

    private ProgressDialog mDialog;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_PING:
                String ping = (String) msg.obj;
                mListViewArrayAdapter.add("Ping:  " + ping);
                mEditText.setText("");
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            case MESSAGE_START_PROGRESS_DIALOG:
                mDialog = ProgressDialog.show(Client.this,
                                              "",
                                              "Finding Simple Service.\nPlease wait...",
                                              true,
                                              true);
                break;
            case MESSAGE_STOP_PROGRESS_DIALOG:
                mDialog.dismiss();
                break;
            default:
                break;
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        mEditText = (EditText) findViewById(R.id.EditText);
        mEditText.setInputType(InputType.TYPE_CLASS_TEXT);
        mEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    /* Send Ping signal to the remote object. */
                    Message msg = mBusHandler.obtainMessage(BusHandler.PING, view.getText().toString());
                    mBusHandler.sendMessage(msg);
                }
                return true;
            }
        });

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        /* Connect to an AllJoyn object. */
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
        case R.id.quit:
            finish();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        /* Disconnect to prevent resource leaks. */
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }


    /* This class will handle all AllJoyn calls. See onCreate(). */
    class BusHandler extends Handler {
        /*
         * Name used as the well-known name and the advertised name of the service this client is
         * interested in. This name must be a unique name both to the bus and to the network as a
         * whole.
         * The name uses reverse URL style of naming, and matches the name used by the service.
         *
         * Note: This value is the same used by the SignalClient and SignalService sample apps.
         * This will allow DynamicSignalClient to also work with the SignalService sample app.
         */
        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.simple";
        private static final short CONTACT_PORT = 42;

        private BusAttachment mBus;

        private int     mSessionId;
        private boolean mIsConnected;
        private boolean mIsStoppingDiscovery;

        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int JOIN_SESSION = 2;
        public static final int DISCONNECT = 3;
        public static final int PING = 4;

        private DynamicBusObject mySignalService;
        private GenericInterface mSignalProxy = null; // generic interface to the remote bus object proxy

        public BusHandler(Looper looper) {
            super(looper);
            mIsConnected = false;
            mIsStoppingDiscovery = false;
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            /* Connect to a remote instance of an object implementing the SimpleInterface. */
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                /*
                 * All communication through AllJoyn begins with a BusAttachment.
                 *
                 * A BusAttachment needs a name. The actual name is unimportant except for internal
                 * security. As a default we use the class name as the name.
                 *
                 * By default AllJoyn does not allow communication between devices (i.e. bus to bus
                 * communication). The second argument must be set to Receive to allow communication
                 * between devices.
                 */
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * Create a bus listener class.
                 */
                mBus.registerBusListener(new BusListener() {
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                        logInfo(String.format("BusListener.foundAdvertisedName(%s, 0x%04x, %s)", name, transport, namePrefix));
                        /*
                         * This client will only join the first service that it sees advertising
                         * the indicated well-known name. If the program is already a member of
                         * a session (i.e. connected to a service) we will not attempt to join
                         * another session.
                         * It is possible to join multiple session however joining multiple
                         * sessions is not shown in this sample.
                         */
                        if (!mIsConnected) {
                            Message msg = obtainMessage(JOIN_SESSION, name);
                            sendMessage(msg);
                        }
                    }
                });

                /*
                 * Create and register the dynamic bus object (used to send signals).
                 *
                 * Note: When the bus object is only being used to send signals, it would be simpler
                 * to just instantiate an empty anonymous dynamic bus object as follows:
                 * mSignalService = new AbstractDynamicBusObject(mBus, SimpleInterface.getBusObjectInfo("/SignalService")) {};
                 */
                mySignalService = new SignalService(mBus, SimpleInterface.getBusObjectInfo("/SignalService"));

                Status status = mBus.registerBusObject(mySignalService, mySignalService.getPath());
                if (Status.OK != status) {
                    logStatus("BusAttachment.registerBusObject()", status);
                    return;
                }

                /* To communicate with AllJoyn objects, we must connect the BusAttachment to the bus. */
                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (Status.OK != status) {
                    finish();
                    return;
                }

                /*
                 * Now find an instance of the AllJoyn object we want to call. We start by looking for
                 * a name, then connecting to the device that is advertising that name.
                 *
                 * In this case, we are looking for the well-known SERVICE_NAME.
                 */
                status = mBus.findAdvertisedName(SERVICE_NAME);
                logStatus(String.format("BusAttachment.findAdvertisedName(%s)", SERVICE_NAME), status);
                if (Status.OK != status) {
                    finish();
                    return;
                }

                break;
            }
            case (JOIN_SESSION): {
                /*
                 * If already connected or if discovery is currently starting/stopping don't join to any other sessions.
                 */
                if (mIsConnected || mIsStoppingDiscovery) {
                    break;
                }

                /*
                 * In order to join the session, we need to provide the well-known
                 * contact port. This is pre-arranged between both sides as part
                 * of the definition of the chat service. As a result of joining
                 * the session, we get a session identifier which we must use to
                 * identify the created session communication channel whenever we
                 * talk to the remote side.
                 */
                mIsConnected = true;
                SessionOpts sessionOpts = new SessionOpts();
                Status status = mBus.joinSession((String) msg.obj, CONTACT_PORT, sessionOpts, new SessionListener() {
                    @Override
                    public void sessionLost(int sessionId, int reason) {
                        mIsConnected = false;
                        logInfo(String.format("BusListener.sessionLost(sessionId = %d, reason = %d)", sessionId,reason));
                        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
                    }
                /* This is the asynchronous callback, invoked from AllJoyn when the join session attempt has completed. */
                }, new OnJoinSessionListener() {
                    @Override
                    public void onJoinSession(Status status, int sessionId, SessionOpts opts, Object context) {
                        logStatus("BusAttachment.joinSession() - sessionId: " + sessionId, status);
                        mIsConnected = (status == Status.OK || status == Status.ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED);
                        if (status == Status.OK) {
                            mSessionId = sessionId;
                            mHandler.sendEmptyMessage(MESSAGE_STOP_PROGRESS_DIALOG);

                            /*
                             * Create a signal emitter proxy with the interface definitions provided by the
                             * dynamic bus object. The proxy is used later to send signals to the remote object.
                             *
                             * Note that with dynamic bus objects we use the GenericInterface.class
                             * with the SignalEmitter rather than a specific bus interface class.
                             */
                            SignalEmitter emitter = new SignalEmitter(mySignalService, sessionId, SignalEmitter.GlobalBroadcast.Off);
                            mSignalProxy = emitter.getInterface(GenericInterface.class);
                        }
                    }
                }, null);

                logStatus("BusAttachment.joinSession() requested", status);
                break;
            }

            /* Release all resources acquired in the connect. */
            case DISCONNECT: {
                mIsStoppingDiscovery = true;
                if (mIsConnected) {
                    Status status = mBus.leaveSession(mSessionId);
                    logStatus("BusAttachment.leaveSession()", status);
                }
                mBus.unregisterBusObject(mySignalService);
                mBus.disconnect();
                getLooper().quit();
                break;
            }

            /*
             * Send Ping signal through the SignalEmitter proxy to the remote object.
             * This will also print the String to the user interface.
             */
            case PING: {
                try {
                    if (mSignalProxy != null) {
                        sendUiMessage(MESSAGE_PING, msg.obj);
                        mSignalProxy.signal(SimpleInterface.INTERFACE, SimpleInterface.SIGNAL_PING, (String)msg.obj);
                    }
                } catch (BusException ex) {
                    logException("SimpleInterface.Ping()", ex);
                }
                break;
            }
            default:
                break;
            }
        }

        /* Helper function to send a message to the UI thread. */
        private void sendUiMessage(int what, Object obj) {
            mHandler.sendMessage(mHandler.obtainMessage(what, obj));
        }
    }

    /*
     * Print the message and status to the Android log. If the result is the expected
     * result only print it to the log. Otherwise print it to the error log and
     * send a Toast to the users screen.
     */
    private void logStatus(String msg, Status status) {
        String log = String.format("%s: %s", msg, status);
        if (status == Status.OK) {
            Log.i(TAG, log);
        } else {
            Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
            mHandler.sendMessage(toastMsg);
            Log.e(TAG, log);
        }
    }

    /* Print the message and exception to the error log, and send Toast to the user screen. */
    private void logException(String msg, BusException ex) {
        String log = String.format("%s: %s", msg, ex);
        Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
        mHandler.sendMessage(toastMsg);
        Log.e(TAG, log, ex);
    }

    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }

}
