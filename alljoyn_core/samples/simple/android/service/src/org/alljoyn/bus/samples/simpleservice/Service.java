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
package org.alljoyn.bus.samples.simpleservice;


import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Toast;

public class Service extends Activity {

    private ArrayAdapter<String> listViewArrayAdapter;
    private ListView listView;
    private Menu menu;

    /** Called when activity's onCreate is called */
    private native int simpleOnCreate();

    /** Called when activity's onDestroy is called */
    private native void simpleOnDestroy();

    /** Called to start service with a given well-known name */
    private native boolean startService(String name, String packageName);

    /** Called to stop service */
    private native void stopService(String name);

    HandlerThread bus_thread = new HandlerThread("BusHandler");
    { bus_thread.start(); }
    Handler bus_handler = new Handler(bus_thread.getLooper());

    /** Handler used to post messages from C++ into UI thread */
    private Handler handler = new Handler();

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        listViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        listView = (ListView) findViewById(R.id.ListView);
        listView.setAdapter(listViewArrayAdapter);

        Button startButton = (Button) findViewById(R.id.StartButton);
        startButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(final View v) {
                final EditText edit = (EditText) findViewById(R.id.AdvertisedName);

                bus_handler.post(new Runnable() {
                    // called from BusHandler thread!
                    public void run() {
                        if (startService(edit.getText().toString(), getPackageName())) {
                            handler.post(new Runnable() {
                                // post Runnable object back to main thread
                                public void run() {
                                    Button startButton = (Button) v;
                                    startButton.setEnabled(false);
                                    Button stopButton = (Button) findViewById(R.id.StopButton);
                                    stopButton.setEnabled(true);
                                    hideInputMethod(edit);
                                    edit.setEnabled(false);
                                }
                            });
                        }
                    }
                });
            }
        });

        Button stopButton = (Button) findViewById(R.id.StopButton);
        stopButton.setEnabled(false);

        stopButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(final View v) {
                final EditText edit = (EditText) findViewById(R.id.AdvertisedName);

                bus_handler.post(new Runnable() {
                    // called from BusHandler thread!
                    public void run() {
                        stopService(edit.getText().toString());

                        handler.post(new Runnable() {
                            public void run() {
                                Button stopButton = (Button) v;
                                stopButton.setEnabled(false);
                                Button startButton = (Button) findViewById(R.id.StartButton);
                                startButton.setEnabled(true);
                                edit.setEnabled(true);
                            }
                        });
                    }
                });
            }
        });

        // Initialize the native part of the sample and connect to remote alljoyn instance
        bus_handler.post(new Runnable() {
            public void run() {
                final int ret = simpleOnCreate();
                if (0 != ret) {
                    handler.post(new Runnable() {
                        public void run() {
                            Toast.makeText(Service.this, "simpleOnCreate failed with " + ret, Toast.LENGTH_LONG).show();
                            finish();
                        }
                    });
                }
            }
        });
    }

    @Override
    protected void onStop() {
        super.onStop();
        // Don't let this app run once it is off screen (it is confusing for a demo)
        this.finish();
    }

    @Override
    protected void onDestroy() {
        simpleOnDestroy();
        bus_handler.getLooper().quit();
        super.onDestroy();
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

    public void hideInputMethod(View focusView)
    {
        InputMethodManager inputManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        inputManager.hideSoftInputFromWindow(focusView.getWindowToken(), 0);
    }

    public void PingCallback(final String sender, final String pingStr) {
        handler.post(new Runnable() {
            public void run() {
                String s = String.format("Message from %s: \"%s\"", sender.substring(sender.length() - 10), pingStr);
                listViewArrayAdapter.add(s);
                listView.setSelection(listView.getBottom());
            }
        });
    }


    static {
        System.loadLibrary("SimpleService");
    }
}