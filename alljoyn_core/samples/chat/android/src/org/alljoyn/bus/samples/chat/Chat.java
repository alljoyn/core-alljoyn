/*
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.chat;

import android.app.Activity;
import android.app.Dialog;
import android.content.res.Configuration;
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
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;
import android.widget.Toast;

public class Chat extends Activity {

    private EditText editText;
    private ArrayAdapter<String> listViewArrayAdapter;
    private ListView listView;
    private Menu menu;

    static final int DIALOG_CONNECT = 1;
    static final int DIALOG_ADVERTISE = 2;
    static final int DIALOG_JOIN_SESSION = 3;


    private static final int INCOMINGMESSAGE = 1;
    private static final int SESSIONCREATEFAILED = 2;
    private static final int SENDCHATFAILED = 3;

    /** Called when activity's onCreate is called */
    private native int jniOnCreate(String packageName);

    /** Called when activity's onDestroy is called */
    private native void jniOnDestroy();

    /** Called when activity UI sends a chat message */
    private native int sendChatMsg(String str);

    /** Called when user enters a name to be "Advertised" " */
    private native boolean advertise(String wellKnownName);

    /** Called when user wants to join a session */
    private native boolean joinSession(String sessionName);

    /** Handler used to post messages from C++ into UI thread */

    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what)
            {
            case INCOMINGMESSAGE:
                listViewArrayAdapter.add((String)msg.obj);
                break;
            case SESSIONCREATEFAILED:
                Toast.makeText(Chat.this, "jniOnCreate failed with  " + msg.arg1, Toast.LENGTH_LONG).show();
                finish();
                break;
            case SENDCHATFAILED:
                Log.v("AllJoynChat", String.format("sendChatMsg(%s) failed (%d)", msg.obj, msg.arg1));
                break;
            }
        }
    };

    private BusHandler bus_handler;

    /* Prevent orientation changes */
    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.choice);
        // Ask what would you like to do Advertise a session or join one
        final RadioButton radio_create_seesion = (RadioButton) findViewById(R.id.radio_create_session);
        final RadioButton radio_join_session = (RadioButton) findViewById(R.id.radio_join_session);
        radio_create_seesion.setOnClickListener(radio_listener);
        radio_join_session.setOnClickListener(radio_listener);

        setConnectedState(false);

        HandlerThread bus_thread = new HandlerThread("BusHandler");
        bus_thread.start();
        bus_handler = new BusHandler(bus_thread.getLooper(), this);

        bus_handler.sendMessage(bus_handler.obtainMessage(BusHandler.ONCREATE, getPackageName()));
    }

    @Override
    protected void onDestroy() {
        //jniOnDestroy();
        bus_handler.sendEmptyMessage(BusHandler.ONDESTROY);
        super.onDestroy();
    }

    private OnClickListener radio_listener = new OnClickListener() {
        public void onClick(View v) {
            // Perform action on clicks
            RadioButton rb = (RadioButton) v;
            if(rb.getId() == R.id.radio_create_session){
                Toast.makeText(Chat.this, "You selected " + rb.getText(), Toast.LENGTH_LONG).show();
                showDialog(DIALOG_ADVERTISE);
            }
            else if (rb.getId() == R.id.radio_join_session){
                Toast.makeText(Chat.this, "You selected " + rb.getText(), Toast.LENGTH_LONG).show();
                showDialog(DIALOG_JOIN_SESSION);
            }
        }
    };


    // Send a message to all connected chat clients
    private void chat(String message) {
        // Send the AllJoyn signal to other connected chat clients
        bus_handler.sendMessage(bus_handler.obtainMessage(BusHandler.SENDCHATMSG, message));
        listViewArrayAdapter.add("Me: " + message);
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

    public void ChatCallback(String sender, String chatStr)
    {
        Message msg = handler.obtainMessage(INCOMINGMESSAGE);
        msg.obj = String.format("%s: \"%s\"", sender.substring(sender.length() - 10), chatStr);
        handler.sendMessage(msg);
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        Dialog dialog;
        switch (id) {

        case DIALOG_ADVERTISE:
            dialog = new Dialog(Chat.this);
            dialog.setContentView(R.layout.advertise);
            dialog.setTitle("Name to be advertised:");
            Button okButton = (Button) dialog.findViewById(R.id.AdvertiseOk);
            okButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    View r = v.getRootView();
                    TextView text = (TextView) r.findViewById(R.id.AdvertiseText);
                    bus_handler.sendMessage(bus_handler.obtainMessage(BusHandler.ADVERTISE, text.getText().toString()));
                    listViewArrayAdapter.add("CHAT SESSION \"" + text.getText().toString() + "\"");
                    Toast.makeText(getApplicationContext(), text.getText().toString(), Toast.LENGTH_LONG).show();
                    dismissDialog(DIALOG_ADVERTISE);
                    //setContentView(R.layout.main);

                }
            });
            Button cancelButton = (Button) dialog.findViewById(R.id.AdvertiseCancel);
            cancelButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    dismissDialog(DIALOG_ADVERTISE);
                    finish();
                }
            });
            break;

        case DIALOG_JOIN_SESSION:
            dialog = new Dialog(Chat.this);
            dialog.setContentView(R.layout.joinsession);
            dialog.setTitle("Name of session you want to join:");
            Button joinokButton = (Button) dialog.findViewById(R.id.JoinSessionOk);
            joinokButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    View r = v.getRootView();
                    TextView text = (TextView) r.findViewById(R.id.JoinSessionText);
                    bus_handler.sendMessage(bus_handler.obtainMessage(BusHandler.JOINSESSION, text.getText().toString()));
                    listViewArrayAdapter.add("CHAT SESSION \"" + text.getText().toString() + "\"");
                    Toast.makeText(getApplicationContext(), text.getText().toString(), Toast.LENGTH_LONG).show();
                    dismissDialog(DIALOG_JOIN_SESSION);
                    //setContentView(R.layout.main);
                }
            });
            Button joinCancelButton = (Button) dialog.findViewById(R.id.JoinSessionCancel);
            joinCancelButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    dismissDialog(DIALOG_JOIN_SESSION);
                    finish();
                }
            });
            break;
        default:
            dialog = null;
            break;
        }

        setContentView(R.layout.main);

        listViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        listView = (ListView) findViewById(R.id.ListView);
        listView.setAdapter(listViewArrayAdapter);

        editText = (EditText) findViewById(R.id.EditText);
        editText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_NULL && event.getAction() == KeyEvent.ACTION_UP) {
                    String message = view.getText().toString();
                    chat(message);
                    view.setText("");
                }
                return true;
            }
        });


        return dialog;
    }

    private void setConnectedState(boolean isConnected)
    {
        if (null != menu) {
            menu.getItem(0).setEnabled(!isConnected);
            menu.getItem(1).setEnabled(isConnected);
        }
    }


    private static class BusHandler extends Handler {
        static final int ONCREATE = 1;
        static final int ONDESTROY = 2;
        static final int SENDCHATMSG = 3;
        static final int ADVERTISE = 4;
        static final int JOINSESSION = 5;

        Chat chat;

        BusHandler(Looper looper, Chat chat) {
            super(looper);
            this.chat = chat;
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what)
            {
            case ONCREATE:
            {
                int ret = chat.jniOnCreate((String) msg.obj);

                if (ret != 0) {
                    Message responseMsg = chat.bus_handler.obtainMessage(SESSIONCREATEFAILED);
                    responseMsg.arg1 = ret;
                    chat.handler.sendMessage(responseMsg);
                }

                break;
            }
            case ONDESTROY:
                chat.jniOnDestroy();
                break;
            case SENDCHATMSG:
            {
                int ret = chat.sendChatMsg((String) msg.obj);
                if (ret != 0) {
                    Message responseMsg = chat.bus_handler.obtainMessage(SENDCHATFAILED);
                    responseMsg.obj = msg.obj;
                    responseMsg.arg1 = ret;
                    chat.handler.sendMessage(responseMsg);
                }

                break;
            }
            case ADVERTISE:
                // ignore return value
                chat.advertise((String) msg.obj);
                break;
            case JOINSESSION:
                // ignore return value
                chat.joinSession((String) msg.obj);
                break;
            default: break;
            }
        }
    }

    static {
        System.loadLibrary("Chat");
    }
}