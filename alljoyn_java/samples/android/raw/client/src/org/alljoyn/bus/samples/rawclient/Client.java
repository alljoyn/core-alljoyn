/*
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

package org.alljoyn.bus.samples.rawclient;

import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.Field;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
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

public class Client extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final int MESSAGE_PING = 1;
    private static final int MESSAGE_PING_REPLY = 2;
    private static final int MESSAGE_POST_TOAST = 3;
    private static final int MESSAGE_START_PROGRESS_DIALOG = 4;
    private static final int MESSAGE_STOP_PROGRESS_DIALOG = 5;

    private static final String TAG = "RawClient";

    private EditText mEditText;
    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;
    private Menu menu;
    private ProgressDialog mDialog;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private BusHandler mBusHandler;

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
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            case MESSAGE_START_PROGRESS_DIALOG:
                mDialog = ProgressDialog.show(Client.this,
                                              "",
                                              "Finding Service.\nPlease wait...",
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
        mEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_NULL
                        && event.getAction() == KeyEvent.ACTION_UP) {
                    /*
                     * Send the text in the control to the service over
                     * the raw session connection.
                     */
                    Message msg = mBusHandler.obtainMessage(BusHandler.SEND_RAW, view.getText().toString());
                    mBusHandler.sendMessage(msg);

                    view.setText("");
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

        /* Disconnect to prevent resource leaks. */
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    /* This class will handle all AllJoyn calls. See onCreate(). */
    class BusHandler extends Handler {
        /*
         * Name used as the well-known name and the advertised name of the service this client is
         * interested in.  This name must be a unique name both to the bus and to the network as a
         * whole.
         *
         * The name uses reverse URL style of naming, and matches the name used by the service.
         */
        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.raw";
        private static final short CONTACT_PORT = 88;

        /*
         * TODO: Remove this hack when ephemeral sockets work.
         */
        private static final String RAW_SERVICE_NAME = "org.alljoyn.bus.samples.yadda888";
        private boolean mHaveServiceName = false;
        private boolean mHaveRawServiceName = false;

        private BusAttachment mBus = null;
        private ProxyBusObject mProxyObj = null;
        private RawInterface mRawInterface = null;

        private int mMsgSessionId = -1;
        private int mRawSessionId = -1;
        private boolean mIsConnected = false;;
        private boolean mIsStoppingDiscovery = false;

        private OutputStream mOutputStream = null;
        private boolean mStreamUp = false;

        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int JOIN_SESSION = 2;
        public static final int DISCONNECT = 3;
        public static final int SEND_RAW = 4;

        public BusHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            /* Connect to a remote instance of an object implementing the RawInterface. */
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
                 * If using the debug version of the AllJoyn libraries, tell
                 * them to write debug output to the OS log so we can see it
                 * using adb logcat.  Turn on all of the debugging output from
                 * the Java language bindings (module ALLJOYN_JAVA).
                 */
                mBus.useOSLogging(true);
                mBus.setDebugLevel("ALLJOYN_JAVA", 7);

                /*
                 * Create a bus listener class to handle callbacks from the
                 * BusAttachement and tell the attachment about it
                 */
                mBus.registerBusListener(new BusListener() {
                    /**
                     * Callback indicating that the system has found an instance
                     * of a name we are searching for.
                     *
                     * When the service comes up it does housekeeping related to
                     * creating all of the resources and when it is ready to
                     * accept clients, it advertises SERVICE_NAME.  We look for
                     * instances of services advertising this name and when we
                     * find one, we join its sessions.  As usual, we send a
                     * message to the AllJoyn handler to make this happen.
                     */
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                        logInfo(String.format("MyBusListener.foundAdvertisedName(%s, 0x%04x, %s)", name, transport, namePrefix));
                        if (name.equals(SERVICE_NAME)) {
                            mHaveServiceName = true;
                        }
                        if (name.equals(RAW_SERVICE_NAME)) {
                            mHaveRawServiceName = true;
                        }
                        if (mHaveServiceName == true && mHaveRawServiceName == true) {
                            /*
                             * This client will only join the first service that it sees advertising
                             * the indicated well-known names.  If the program is already a member of
                             * a session (i.e. connected to a service) we will not attempt to join
                             * another session.
                             * It is possible to join multiple session however joining multiple
                             * sessions is not shown in this sample.
                             */
                            if(!mIsConnected){
                                mBusHandler.sendEmptyMessage(BusHandler.JOIN_SESSION);
                            }
                        }
                    }
                });

                /* To communicate with AllJoyn objects, we must connect the BusAttachment to the bus. */
                Status status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (Status.OK != status) {
                    finish();
                    return;
                }

                /*
                 * Now find an instance of the AllJoyn service we want to talk to.  We start by looking for
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

                status = mBus.findAdvertisedName(RAW_SERVICE_NAME);
                logStatus(String.format("BusAttachement.findAdvertisedName(%s)", RAW_SERVICE_NAME), status);
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

                Status status = mBus.joinSession(SERVICE_NAME, contactPort, sessionId, sessionOpts, new SessionListener(){
                    @Override
                    public void sessionLost(int sessionId, int reason) {
                        mIsConnected = false;
                        logInfo(String.format("MyBusListener.sessionLost(sessionId = %d, reason = %d)", sessionId,reason));
                        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
                    }
                });
                logStatus("BusAttachment.joinSession()", status);
                if (Status.OK != status) {
                    break;
                }

                /*
                 * To communicate with an AllJoyn object, we create a ProxyBusObject.
                 * A ProxyBusObject is composed of a name, path, sessionID and interfaces.
                 *
                 * This ProxyBusObject is located at the well-known SERVICE_NAME, under path
                 * "/RawService", uses sessionID of CONTACT_PORT, and implements the RawInterface.
                 */
                mProxyObj = mBus.getProxyBusObject(SERVICE_NAME, "/RawService", sessionId.value,
                                                   new Class<?>[] { RawInterface.class });

                /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
                mRawInterface =  mProxyObj.getInterface(RawInterface.class);

                mMsgSessionId = sessionId.value;
                mIsConnected = true;
                mHandler.sendEmptyMessage(MESSAGE_STOP_PROGRESS_DIALOG);
                break;
            }

            /* Release all resources acquired in the connect. */
            case DISCONNECT: {
                mIsStoppingDiscovery = true;
                if (mIsConnected) {
                    Status status = mBus.leaveSession(mMsgSessionId);
                    logStatus("BusAttachment.leaveSession(): message-based session", status);

                    status = mBus.leaveSession(mRawSessionId);
                    logStatus("BusAttachment.leaveSession(): raw session", status);
                }
                mBus.disconnect();
                if (mStreamUp == true) {
                    try {
                        mOutputStream.close();
                    } catch (IOException ex) {
                        logInfo("Exception closing output stream");
                    }
                }
                getLooper().quit();
                break;
            }

            /*
             * We have a string to send to the server via a raw session
             * socket.  If this is the first string we've ever sent on this
             * session, we need to get a so-called raw session started in the
             * service.  We are eventually going to talk to the raw session
             * over a socket FileDescriptor, but don't confuse this "socket
             * used to communicate over a raw session" with the idea of a
             * BSD raw socket (which allow you to provide your own IP or
             * Ethernet headers).  Raw sessions only means that the data
             * sent over the session will be sent using the underlying socket
             * fiel descriptor and will not be encapsulated in AllJoyn
             * messages).
             *
             * Once we have joined the raw session, we can retrieve the
             * underlying session's OS socket file descriptor and build a Java
             * FileDescriptor using a private constructor found via reflection.
             * We then create a Java output stream using that FileDescriptor.
             *
             * Once/If we have the raw session all set up, and have a Java IO
             * stream ready, we simply send the bytes of the string to the
             * service directly using the Java stream.  This completely
             * bypasses AllJoyn which was used for discovery and connection
             * establishment.
             */
            case SEND_RAW: {
                if (mIsConnected == false || mStreamUp == false) {
                    try {
                        /*
                         * In order get a raw session to join, we need to get an
                         * ephemeral contact port.  As a part of the RawInterface,
                         * we have a method used to get that port.  Note that
                         * errors are returned through Java exceptions, and so we
                         * wrap this code in a try catch block.
                         */
                        logInfo("RequestRawSession()");
                        short contactPort = mRawInterface.RequestRawSession();
                        logInfo(String.format("RequestRawSession() returns %d", contactPort));

                        /*
                         * Now join the raw session.  Note that we are asking
                         * for TRAFFIC_RAW_RELIABLE.  Once this happens, the
                         * session is ready for raw traffic that will not be
                         * encapsulated in AllJoyn messages.
                         */
                        SessionOpts sessionOpts = new SessionOpts();
                        sessionOpts.traffic = SessionOpts.TRAFFIC_RAW_RELIABLE;
                        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();
                        logInfo("joinSession()");
                        Status status = mBus.joinSession(RAW_SERVICE_NAME, contactPort, sessionId, sessionOpts, new SessionListener());
                        logStatus("BusAttachment.joinSession()", status);
                        if (status != Status.OK) {
                            break;
                        }
                        mRawSessionId = sessionId.value;

                        /*
                         * The session is in raw mode, but we need to get a
                         * socket file descriptor back from AllJoyn that
                         * represents the established connection.  We are then
                         * free to do whatever we want with the sock.
                         */
                        logInfo("getSessionFd()");
                        Mutable.IntegerValue sockFd = new Mutable.IntegerValue();
                        status = mBus.getSessionFd(mRawSessionId, sockFd);
                        logStatus("BusAttachment.getSessionFd()", status);
                        if (status != Status.OK) {
                            break;
                        }

                        /*
                         * We have a socked FD, but now we need a Java file
                         * descriptor.  There is no appropriate constructor,
                         * public or not in the Dalvik FileDescriptor, so
                         * we new up a file descriptor and set its internal
                         * descriptor field using reflection.
                         */
                        Field field = FileDescriptor.class.getDeclaredField("descriptor");
                        field.setAccessible(true);
                        FileDescriptor fd = new FileDescriptor();
                        field.set(fd, sockFd.value);

                        /*
                         * Now that we have a FileDescriptor with an AllJoyn
                         * raw session socket FD in it, we can use it like
                         * any other "normal" FileDescriptor.
                         */
                        mOutputStream = new FileOutputStream(fd);
                        mStreamUp = true;
                    } catch (Throwable ex) {
                        logInfo(String.format("Exception bringing up raw stream: %s", ex.toString()));
                    }
                }

                /*
                 * If we've sucessfully created an output stream from the raw
                 * session connection established by AllJoyn, we write the
                 * byte string (from the user) to the TCP stream that now
                 * underlies it all.
                 */
                if (mStreamUp == true) {
                    String string = (String)msg.obj + "\n";
                    try {
                        logInfo(String.format("Writing %s to output stream", string));
                        mOutputStream.write(string.getBytes());
                        logInfo(String.format("Flushing stream", string));
                        mOutputStream.flush();
                    } catch (IOException ex) {
                        logInfo("Exception writing and flushing the string");
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

    /*
     * print the status or result to the Android log. If the result is the expected
     * result only print it to the log.  Otherwise print it to the error log and
     * Sent a Toast to the users screen.
     */
    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }
}
