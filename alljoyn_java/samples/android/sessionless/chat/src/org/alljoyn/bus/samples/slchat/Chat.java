/*
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.slchat;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.MessageContext;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.annotation.BusSignalHandler;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;

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

public class Chat extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "SessionlessChat";

    /* Handler for UI messages
     * This handler updates the UI depending on the message received.
     */
    private static final int MESSAGE_CHAT = 1;
    private static final int MESSAGE_POST_TOAST = 2;

    private class PingInfo{
        private String senderName;
        private String message;
        public PingInfo(String senderName, String message){
            this.senderName = senderName;
            this.message = message;
        }
        public String getSenderName(){
            return this.senderName;
        }
        public String getMessage(){
            return this.message;
        }
    }
    private Handler mHandler = new Handler(new Handler.Callback() {

        @Override
        public boolean handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_CHAT:
                /* Add the chat message received to the List View */
                String ping = (String) msg.obj;
                mListViewArrayAdapter.add(ping);
                break;
            case MESSAGE_POST_TOAST:
                /* Post a toast to the UI */
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }

            return true;
        }
    });

    /* UI elements */
    private ListView mListView;
    private EditText mNameEditText;
    private EditText mMessageEditText;

    /* ArrayAdapter used to manage the chat messages
     * received by this application.
     */
    private ArrayAdapter<String> mListViewArrayAdapter;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private Handler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        /* Create an adapter to manage the list view of chat messages */
        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);

        /* Set the adapter for the list view */
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        mNameEditText = (EditText) findViewById(R.id.NameEditText);

        /* Set the action to be taken when the 'Return' key is pressed in the text box */
        mMessageEditText = (EditText) findViewById(R.id.MessageEditText);
        mMessageEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_NULL
                        && event.getAction() == KeyEvent.ACTION_UP) {
                    /* Send a sessionless signal chat message using the mBusHandler. */
                    String senderName = mNameEditText.getText().toString();
                    String message = view.getText().toString();

                    Message msg = mBusHandler.obtainMessage(BusHandlerCallback.CHAT,
                                                            new PingInfo(senderName,message));

                    mBusHandler.sendMessage(msg);
                    mMessageEditText.setText("");
                }
                return true;
            }
        });

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new Handler(busThread.getLooper(),new BusHandlerCallback());

        mBusHandler.sendEmptyMessage(BusHandlerCallback.CONNECT);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        /* Handle item selection */
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

        /* Disconnect to prevent any resource leaks. */
        mBusHandler.sendEmptyMessage(BusHandlerCallback.DISCONNECT);
    }

    /* The class that is our AllJoyn service.  It implements the ChatInterface. */
    class ChatService implements ChatInterface, BusObject {
        public ChatService(BusAttachment bus){
            this.bus = bus;
        }
        /*
         * This is the Signal Handler code which has the interface name and the name of the signal
         * which is sent by the client. It prints out the string it receives as parameter in the
         * signal on the UI.
         *
         * This code also prints the string it received from the user and the string it is
         * returning to the user to the screen.
         */
        @BusSignalHandler(iface = "org.alljoyn.bus.samples.slchat", signal = "Chat")
        public void Chat(String senderName, String message) {
            Log.i(TAG, "Signal  : " + senderName +": "+ message);
            sendUiMessage(MESSAGE_CHAT, senderName + ": "+ message);
        }

        /* Helper function to send a message to the UI thread. */
        private void sendUiMessage(int what, Object obj) {
            mHandler.sendMessage(mHandler.obtainMessage(what, obj));
        }

        private BusAttachment bus;
    }

    /* This Callback class will handle all AllJoyn calls. See onCreate(). */
    class BusHandlerCallback implements Handler.Callback {

        /* The AllJoyn BusAttachment */
        private BusAttachment mBus;

        /* The AllJoyn SignalEmitter used to emit sessionless signals */
        private SignalEmitter emitter;

        private ChatInterface mChatInterface = null;
        private ChatService myChatService = null;

        /* These are the messages sent to the BusHandlerCallback from the UI. */
        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;
        public static final int CHAT = 3;

        @Override
        public boolean handleMessage(Message msg) {
            switch (msg.what) {
            /* Connect to the bus and register to obtain chat messages. */
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                /*
                 * All communication through AllJoyn begins with a BusAttachment.
                 *
                 * A BusAttachment needs a name. The actual name is unimportant except for internal
                 * security. As a default we use the class name as the name.
                 *
                 * By default AllJoyn does not allow communication between devices (i.e. bus to bus
                 * communication).  The second argument must be set to Receive to allow
                 * communication between devices.
                 */
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * Create and register a bus object
                 */
                myChatService = new ChatService(mBus);
                Status status = mBus.registerBusObject(myChatService, "/ChatService");
                if (Status.OK != status) {
                    logStatus("BusAttachment.registerBusObject()", status);
                    return false;
                }

                /*
                 * Connect the BusAttachment to the bus.
                 */
                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (status != Status.OK) {
                    finish();
                    return false;
                }

                /*
                 *  We register our signal handler which is implemented inside the ChatService
                 */
                status = mBus.registerSignalHandlers(myChatService);
                if (status != Status.OK) {
                    Log.i(TAG, "Problem while registering signal handler");
                    return false;
                }

                /*
                 *  Add rule to receive chat messages(sessionless signals)
                 */
                status = mBus.addMatch("sessionless='t'");
                if (status == Status.OK) {
                    Log.i(TAG,"AddMatch was called successfully");
                }

                break;
            }

            /* Release all resources acquired in connect. */
            case DISCONNECT: {
                /*
                 * It is important to unregister the BusObject before disconnecting from the bus.
                 * Failing to do so could result in a resource leak.
                 */
                mBus.disconnect();
                mBusHandler.getLooper().quit();
                break;
            }
            /*
             * Call the service's Ping method through the ProxyBusObject.
             *
             * This will also print the String that was sent to the service and the String that was
             * received from the service to the user interface.
             */
            case CHAT: {
                try {
                    if(emitter == null){
                        /* Create an emitter to emit a sessionless signal with the desired message.
                         * The session ID is set to zero and the sessionless flag is set to true.
                         */
                        emitter = new SignalEmitter(myChatService, 0, SignalEmitter.GlobalBroadcast.Off);
                        emitter.setSessionlessFlag(true);
                        /* Get the ChatInterface for the emitter */
                        mChatInterface = emitter.getInterface(ChatInterface.class);
                    }
                    if (mChatInterface != null) {
                        PingInfo info = (PingInfo) msg.obj;
                        /* Send a sessionless signal using the chat interface we obtained. */
                        Log.i(TAG,"Sending chat "+info.getSenderName()+": " + info.getMessage());
                        mChatInterface.Chat(info.getSenderName(), info.getMessage());
                    }
                } catch (BusException ex) {
                    logException("ChatInterface.Chat()", ex);
                }
                break;
            }
            default:
                break;
            }
            return true;
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
}
