/*
 * Copyright AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.observer;



import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnMultiChoiceClickListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;

public class Client extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    static final int MESSAGE_INCOMING_EVENT = 1;
    static final int MESSAGE_UPDATE_UI = 2;
    static final int MESSAGE_DOOR_EVENT = 3;

    static final String TAG = "BasicObserver";


    private DoorAdapter mDoorListAdapter;
    private ListView mMsgListView;
    private ListView mDoorListView;
    private Menu menu;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private BusHandler mBusHandler;
    private final UIHandler mHandler = new UIHandler();

    private static class UIHandler extends Handler {
        ArrayAdapter<String> mMsgListViewArrayAdapter;
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_INCOMING_EVENT:
                mMsgListViewArrayAdapter.add("Incoming event:  " + msg.obj);
                break;
            case MESSAGE_UPDATE_UI:
                Runnable uiTask = (Runnable) msg.obj;
                uiTask.run();
                break;
            case MESSAGE_DOOR_EVENT:
                mMsgListViewArrayAdapter.add("Door Event: " + msg.obj);
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

        mHandler.mMsgListViewArrayAdapter = new ArrayAdapter<String>(this,
                R.layout.message) {
            @Override
            public void add(String object) {
                //only log 25 messages.
                if (getCount() == 25) {
                    remove(getItem(0));
                }
                super.add(object);
            }

        };
        mMsgListView = (ListView) findViewById(R.id.MsgListView);
        mMsgListView.setAdapter(mHandler.mMsgListViewArrayAdapter);
        mDoorListAdapter = new DoorAdapter(mHandler);

        mDoorListView = (ListView) findViewById(R.id.DoorListView);
        mDoorListView.setAdapter(mDoorListAdapter);

        mDoorListView.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                mBusHandler.sendMessage(mBusHandler.obtainMessage(BusHandler.KNOCK_ON_DOOR,
                        mDoorListView.getItemAtPosition(position)));
                return true;
            }
        });

        mDoorListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                mBusHandler.sendMessage(mBusHandler.obtainMessage( BusHandler.TOGGLE_DOOR,
                        mDoorListView.getItemAtPosition(position)));
            }
        });

        /*
         * Make all AllJoyn calls through a separate handler thread to prevent
         * blocking the UI.
         */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(mDoorListAdapter, busThread.getLooper());

        /* Connect to an AllJoyn object. */
        mBusHandler.sendMessage(mBusHandler.obtainMessage(BusHandler.CONNECT, this));
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
        case R.id.createDoor: {
            AlertDialog.Builder alert = new AlertDialog.Builder(this);
            alert.setTitle("Create Door");
            alert.setMessage("Please provide a name for the new door");
            final EditText input = new EditText(this);
            alert.setView(input);
            alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    String value = input.getText().toString();
                    mBusHandler.sendMessage(mBusHandler.obtainMessage(BusHandler.CREATE_DOOR, value));
                }
            });
            alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                }
            });
            alert.show();
            return true;
        }
        case R.id.deleteDoor: {
            AlertDialog.Builder alert = new AlertDialog.Builder(this);
            alert.setTitle("Delete Doors");
            final String[] doorNames = mBusHandler.getDoorNames();
            final boolean[] checkedDoors = new boolean[doorNames.length];
            alert.setMultiChoiceItems(doorNames, checkedDoors, new OnMultiChoiceClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                    checkedDoors[which] = isChecked;
                }
            });

            alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    ArrayList<String> doors = new ArrayList<String>();
                    for (int i = 0; i < checkedDoors.length; i++) {
                        if (checkedDoors[i]) {
                            doors.add(doorNames[i]);
                        }
                    }
                    if (doors.size() > 0) {
                        mBusHandler.sendMessage(mBusHandler.obtainMessage(BusHandler.DELETE_DOOR, doors));
                    }
                }
            });

            alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                }
            });
            alert.show();
            return true;
        }
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
}
