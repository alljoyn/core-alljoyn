/*
 * This sample is identical to the simple sample, with the addition of security.  Refer to the
 * simple sample for further explanation of the AllJoyn code not called out here.
 *
 * Copyright (c) 2010-2011,2013, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.srpclient;

import java.util.concurrent.CountDownLatch;

import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class Client extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final int DIALOG_GET_CREDENTIALS = 1;

    private static final int MESSAGE_PING = 1;
    private static final int MESSAGE_PING_REPLY = 2;
    private static final int MESSAGE_GET_CREDENTIALS = 3;
    private static final int MESSAGE_AUTH_COMPLETE = 4;
    private static final int MESSAGE_POST_TOAST = 5;
    private static final int MESSAGE_START_PROGRESS_DIALOG = 6;
    private static final int MESSAGE_STOP_PROGRESS_DIALOG = 7;

    private static final String TAG = "SecureSrpClient";

    private EditText mEditText;
    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;
    private Menu menu;
    private CountDownLatch mLatch;
    private String mPassword;
    private ProgressDialog mDialog;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_PING:
                String ping = (String) msg.obj;
                mListViewArrayAdapter.add("Ping:  " + ping);
                break;
            case MESSAGE_PING_REPLY:
                String ret = (String) msg.obj;
                mListViewArrayAdapter.add("Reply:  " + ret);
                mEditText.setText("");
                break;
            case MESSAGE_GET_CREDENTIALS:
                showDialog(DIALOG_GET_CREDENTIALS);
                break;
            case MESSAGE_AUTH_COMPLETE:
                Boolean authenticated = (Boolean) msg.obj;
                if (authenticated.equals(Boolean.FALSE)) {
                    Toast.makeText(Client.this, "Authentication failed", Toast.LENGTH_LONG).show();
                }
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            case MESSAGE_START_PROGRESS_DIALOG:
                mDialog = ProgressDialog.show(Client.this,
                                              "",
                                              "Finding Security Service.\nPlease wait...",
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

    /* The authentication listener for our bus attachment. */
    private SrpKeyXListener mAuthListener;

    private BusHandler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        mEditText = (EditText) findViewById(R.id.EditText);
        mEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_NULL
                        && event.getAction() == KeyEvent.ACTION_UP) {
                    Message msg = mBusHandler.obtainMessage(BusHandler.PING,
                                                            view.getText().toString());
                    mBusHandler.sendMessage(msg);
                }
                return true;
            }
        });

        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        mAuthListener = new SrpKeyXListener();
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
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
            finish();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
        case DIALOG_GET_CREDENTIALS:
            LayoutInflater factory = LayoutInflater.from(this);
            View view = factory.inflate(R.layout.alert_dialog, null);
            EditText editText = (EditText) view.findViewById(R.id.PasswordEditText);
            editText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                    if (actionId == EditorInfo.IME_NULL
                            && event.getAction() == KeyEvent.ACTION_UP) {
                        mPassword = view.getText().toString();
                        mLatch.countDown();
                        dismissDialog(DIALOG_GET_CREDENTIALS);
                    }
                    return true;
                }
            });
            return new AlertDialog.Builder(this)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(R.string.alert_dialog_password)
                .setCancelable(false)
                .setView(view)
                .create();
        default:
            return null;
        }
    }

    private void sendUiMessage(int what, Object obj) {
        mHandler.sendMessage(mHandler.obtainMessage(what, obj));
    }

    /*
     * The main differences between a secure application and a plain application, besides the
     * @Secure annotations of the interfaces, are encapsulated in the AuthListener.  The
     * BusAttachment calls the listener with various authentication requests in the process of
     * authenticating a peer.  The requests made are dependent on the specific authentication
     * mechanism negotiated between the peers.
     */
    class SrpKeyXListener implements AuthListener {

        /* Returns the list of supported mechanisms. */
        public String getMechanisms() {
            return "ALLJOYN_SRP_KEYX";
        }

        /*
         * Persistent authentication and encryption data is stored at this location.
         *
         * This uses the private file area associated with the application package.
         */
        public String getKeyStoreFileName() {
            return getFileStreamPath("alljoyn_keystore").getAbsolutePath();
        }

        /*
         * Authentication requests are being made.  Contained in this call are the mechanism in use,
         * the number of attempts made so far, the desired user name for the requests, and the
         * specific credentials being requested in the form of AuthRequests.
         *
         * The ALLJOYN_SRP_KEYX mechanism will request the password.
         */
        public boolean requested(String authMechanism, String authPeer, int count, String userName,
                AuthRequest[] requests) {
            try {
                if (count <= 3) {
                    /*
                     * We need to wait here for the user to enter the password before we can
                     * return.  The latch takes care of the synchronization for us.
                     */
                    mLatch = new CountDownLatch(1);
                    sendUiMessage(MESSAGE_GET_CREDENTIALS, null);
                    mLatch.await();
                    for (AuthRequest request : requests) {
                        if (request instanceof PasswordRequest) {
                            ((PasswordRequest) request).setPassword(mPassword.toCharArray());
                        }
                    }
                    return true;
                }
            } catch (InterruptedException ex) {
                Log.e(TAG, "Error waiting for password", ex);
            }
            return false;
        }

        public void completed(String authMechanism, String authPeer, boolean authenticated) {
            sendUiMessage(MESSAGE_AUTH_COMPLETE, authenticated);
        }
    }

    class BusHandler extends Handler {
        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.secure";
        private static final short CONTACT_PORT=42;

        private BusAttachment mBus;
        private ProxyBusObject mProxyObj;
        private SecureInterface mSecureInterface;

        private int     mSessionId;
        private boolean mIsConnected;
        private boolean mIsStoppingDiscovery;

        public static final int CONNECT = 1;
        public static final int JOIN_SESSION = 2;
        public static final int DISCONNECT = 3;
        public static final int PING = 4;

        public BusHandler(Looper looper) {
            super(looper);

            mIsConnected = false;
            mIsStoppingDiscovery = false;
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * Register the AuthListener before calling connect() to ensure that everything is
                 * in place before any remote peers access the service.
                 */
                Status status = mBus.registerAuthListener(mAuthListener.getMechanisms(),
                                                          mAuthListener,
                                                          mAuthListener.getKeyStoreFileName());
                logStatus("BusAttachment.registerAuthListener()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                mBus.registerBusListener(new BusListener() {
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                        logInfo(String.format("MyBusListener.foundAdvertisedName(%s, 0x%04x, %s)", name, transport, namePrefix));
                        /*
                         * This client will only join the first service that it sees advertising
                         * the indicated well-known name.  If the program is already a member of
                         * a session (i.e. connected to a service) we will not attempt to join
                         * another session.
                         * It is possible to join multiple session however joining multiple
                         * sessions is not shown in this sample.
                         */
                        if(!mIsConnected){
                            Message msg = obtainMessage(JOIN_SESSION, name);
                            sendMessage(msg);
                        }
                    }
                });

                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (Status.OK != status) {
                    finish();
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
                    finish();
                    return;
                }
                break;
            }

            case (JOIN_SESSION): {
                /*
                 * If discovery is currently being stopped don't join to any other sessions.
                 */
                if (mIsStoppingDiscovery) {
                    break;
                }

                /*
                 * In order to join the session, we need to provide the well-known
                 * contact port.  This is pre-arranged between both sides as part
                 * of the definition of the chat service.  As a result of joining
                 * the session, we get a session identifier which we must use to
                 * identify the created session communication channel whenever we
                 * talk to the remote side.
                 */
                short contactPort = CONTACT_PORT;
                SessionOpts sessionOpts = new SessionOpts();
                Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

                Status status = mBus.joinSession((String) msg.obj, contactPort, sessionId, sessionOpts, new SessionListener(){
                    @Override
                    public void sessionLost(int sessionId, int reason) {
                        mIsConnected = false;
                        logInfo(String.format("MyBusListener.sessionLost(sessionId = %d, reason = %d)", sessionId, reason));
                        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
                    }
                });
                logStatus("BusAttachment.joinSession()", status);

                if (status == Status.OK) {
                    /*
                     * To communicate with an AllJoyn object, we create a ProxyBusObject.
                     * A ProxyBusObject is composed of a name, path, sessionID and interfaces.
                     *
                     * This ProxyBusObject is located at the well-known SERVICE_NAME, under path
                     * "/SimpleService", uses sessionID of CONTACT_PORT, and implements the SimpleInterface.
                     */
                    mProxyObj =  mBus.getProxyBusObject(SERVICE_NAME,
                                                        "/SecureService",
                                                        sessionId.value,
                                                        new Class[] { SecureInterface.class });

                    /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
                    mSecureInterface = mProxyObj.getInterface(SecureInterface.class);

                    mSessionId = sessionId.value;
                    mIsConnected = true;
                    mHandler.sendEmptyMessage(MESSAGE_STOP_PROGRESS_DIALOG);
                }
                break;
            }
            case DISCONNECT: {
                mIsStoppingDiscovery = true;
                if (mIsConnected) {
                    Status status = mBus.leaveSession(mSessionId);
                    logStatus("BusAttachment.leaveSession()", status);
                }
                mBus.disconnect();
                getLooper().quit();
                break;
            }

            case PING: {
                if (mSecureInterface != null) {
                    try {
                        sendUiMessage(MESSAGE_PING, msg.obj);
                        String reply = mSecureInterface.Ping((String) msg.obj);
                        sendUiMessage(MESSAGE_PING_REPLY, reply);
                    } catch (BusException ex) {
                        logException("SecureInterface.Ping()", ex);
                    }
                }
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
            Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
            mHandler.sendMessage(toastMsg);
            Log.e(TAG, log);
        }
    }

    private void logException(String msg, BusException ex) {
        String log = String.format("%s: %s", msg, ex);
        Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
        mHandler.sendMessage(toastMsg);
        Log.e(TAG, log, ex);
    }

    /*
     * print the status or result to the Android log. If the result is the expected
     * result only print it to the log.  Otherwise print it to the error log and
     * Sent a Toast to the users screen.
     */
    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }
}
