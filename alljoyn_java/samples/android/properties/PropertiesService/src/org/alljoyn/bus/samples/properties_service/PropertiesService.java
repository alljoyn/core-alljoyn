/*
 * This is an Android sample on how to use the AllJoyn properties.
 * This service provides two properties:
 *    background color (as a string) and text size in dip (as an int)
 *
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
package org.alljoyn.bus.samples.properties_service;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import android.app.Activity;
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
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemSelectedListener;

/*
 * Properties Service sample code.
 */
public class PropertiesService extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "PropertiesService";

    private static final int MESSAGE_UPDATE_TEXT_SIZE = 1;
    private static final int MESSAGE_UPDATE_COLOR = 2;
    private static final int MESSAGE_POST_TOAST = 3;

    Spinner mColorSpinner;
    Spinner mTextSizeSpinner;

    TextView mLoremText;
    LinearLayout mPrimarylayout;
    private Menu menu;


    private Handler mHandler;
    private BusHandler mBusHandler;

    private AllJoynProperties mProperties;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mColorSpinner = (Spinner) findViewById(R.id.color_spinner);
        ArrayAdapter<CharSequence> colorAdapter = ArrayAdapter.createFromResource(
                this, R.array.color_array, android.R.layout.simple_spinner_item);
        colorAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mColorSpinner.setAdapter(colorAdapter);
        mColorSpinner.setOnItemSelectedListener(new ColorSelectedListener());

        mTextSizeSpinner = (Spinner) findViewById(R.id.text_size);
        ArrayAdapter<CharSequence> textSizeAdapter = ArrayAdapter.createFromResource(
                this, R.array.text_size_array, android.R.layout.simple_spinner_item);
        textSizeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mTextSizeSpinner.setAdapter(textSizeAdapter);
        mTextSizeSpinner.setOnItemSelectedListener(new TextSizeSelectedListener());

        mLoremText = (TextView) findViewById(R.id.lorem_ipsum);
        mPrimarylayout = (LinearLayout) findViewById(R.id.main_layout);

        mHandler = new UpdateUI();

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        /* Start our service. */
        mProperties = new AllJoynProperties();

        // Update the UI to show the default values.
        updateColor();
        updateTextSize();

        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
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
        // Use the mBusHandler to disconnect from the bus. Failing to to this could result in memory leaks
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }


    /*
     * update the background color based on user input for the text-size dropdown box.
     */
    public class ColorSelectedListener implements OnItemSelectedListener {
        /**
         * Update the background color
         */
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            switch(pos) {
            case 0: mProperties.setBackGroundColor(AllJoynProperties.COLOR_RED); break;
            case 1: mProperties.setBackGroundColor(AllJoynProperties.COLOR_GREEN); break;
            case 2: mProperties.setBackGroundColor(AllJoynProperties.COLOR_BLUE); break;
            case 3: mProperties.setBackGroundColor(AllJoynProperties.COLOR_YELLOW); break;
            default: mProperties.setBackGroundColor(AllJoynProperties.COLOR_RED); break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // leave everything unchanged if nothing is selected.
        }

    }

    /*
     * update the text-size based on user input for the text-size dropdown box.
     */
    public class TextSizeSelectedListener implements OnItemSelectedListener {

        /*
         * update the text-size.
         */
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            switch(pos) {
            case 0:  mProperties.setTextSize(AllJoynProperties.TEXT_TINY);  break;
            case 1:  mProperties.setTextSize(AllJoynProperties.TEXT_SMALL); break;
            case 2:  mProperties.setTextSize(AllJoynProperties.TEXT_MEDIUM); break;
            case 3:  mProperties.setTextSize(AllJoynProperties.TEXT_REGULAR); break;
            case 4:  mProperties.setTextSize(AllJoynProperties.TEXT_LARGE); break;
            case 5:  mProperties.setTextSize(AllJoynProperties.TEXT_XLARGE); break;
            default: mProperties.setTextSize(AllJoynProperties.TEXT_REGULAR); break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // leave everything unchanged if nothing is selected.
        }
    }

    /**
     * handler that will update the screen view based on currently stored
     * values for background-color and text-size.
     */
    public class UpdateUI extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MESSAGE_UPDATE_TEXT_SIZE: {
                updateTextSize();
                break;
            }
            case MESSAGE_UPDATE_COLOR: {
                updateColor();
                break;
            }
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }
        }
    };

    /*
     * update the background color show on the screen as well as dropdown boxes.
     * colors are Red/Green/Blue/Yellow.
     */
    private void updateColor() {
        String backgroundColor = mProperties.getBackGroundColor();
        if (backgroundColor.equals(AllJoynProperties.COLOR_RED)) {
            mPrimarylayout.setBackgroundColor(Color.RED);
            mColorSpinner.setSelection(0);
        } else if (backgroundColor.equals(AllJoynProperties.COLOR_GREEN)) {
            mPrimarylayout.setBackgroundColor(Color.GREEN);
            mColorSpinner.setSelection(1);
        } else if (backgroundColor.equalsIgnoreCase(AllJoynProperties.COLOR_BLUE)) {
            mPrimarylayout.setBackgroundColor(Color.BLUE);
            mColorSpinner.setSelection(2);
        } else if (backgroundColor.equals(AllJoynProperties.COLOR_YELLOW)) {
            mPrimarylayout.setBackgroundColor(Color.YELLOW);
            mColorSpinner.setSelection(3);
        }
    }

    /*
     * Update the text size for the text shown on the screen as well as dropdown boxes.
     */
    private void updateTextSize() {
        int textSize = mProperties.getTextSize();
        //update the actual text size
        mLoremText.setTextSize(textSize);
        //Update the spinner to reflect the text size.
        if (textSize <= AllJoynProperties.TEXT_TINY) {
            mTextSizeSpinner.setSelection(0);
        } else if (textSize <= AllJoynProperties.TEXT_SMALL) {
            mTextSizeSpinner.setSelection(1);
        } else if (textSize <= AllJoynProperties.TEXT_MEDIUM) {
            mTextSizeSpinner.setSelection(2);
        } else if (textSize <= AllJoynProperties.TEXT_REGULAR) {
            mTextSizeSpinner.setSelection(3);
        } else if (textSize <= AllJoynProperties.TEXT_LARGE) {
            mTextSizeSpinner.setSelection(4);
        } else { //it the textSize is larger than Large set spinner to x-large
            mTextSizeSpinner.setSelection(5);
        }
    }

    /*
     * The PropertiesInterface needs to be implemented so there is a place
     * that contains the properties we are interested in.  This class
     * implements all of the get/set methods listed.
     * When ever a set method is used send a Message to the UI so it can
     * be updated based on the new property.
     */
    class AllJoynProperties  implements PropertiesInterface, BusObject {

        // Values used to as default text sizes.
        public static final int TEXT_TINY = 8;
        public static final int TEXT_SMALL = 12;
        public static final int TEXT_MEDIUM = 16;
        public static final int TEXT_REGULAR = 20;
        public static final int TEXT_LARGE = 24;
        public static final int TEXT_XLARGE = 28;

        // Values used as possible colors that can be selected.
        public static final String COLOR_RED = "Red";
        public static final String COLOR_GREEN = "Green";
        public static final String COLOR_BLUE = "Blue";
        public static final String COLOR_YELLOW = "Yellow";

        private String mBackgroundColor;
        private int    mTextSize;

        public AllJoynProperties() {
            //default values used at startup.
            mBackgroundColor = COLOR_RED;
            mTextSize = TEXT_REGULAR; //text size in dip
        }

        public String getBackGroundColor() {
            return mBackgroundColor;
        }

        public void setBackGroundColor(String color) {
            this.mBackgroundColor = color;
            mHandler.sendEmptyMessage(MESSAGE_UPDATE_COLOR);
        }

        public int getTextSize() {
            return mTextSize;
        }

        public void setTextSize(int size) {
            this.mTextSize = size;
            mHandler.sendEmptyMessage(MESSAGE_UPDATE_TEXT_SIZE);
        }
    }

    /*
     * This class will handle all AllJoyn calls.
     * For a detailed description of each part of the code shown in the BusHandler
     * class see the SimpleService sample code.
     */
    class BusHandler extends Handler {
        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.properties";
        private static final short CONTACT_PORT = 42;

        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;

        private BusAttachment mBus;

        public BusHandler(Looper looper) {
            super(looper);
        }

        public void handleMessage(Message msg) {
            switch (msg.what) {
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                mBus.registerBusListener(new BusListener());

                /*
                 * Register the BusObject with the path "/testProperties"
                 */
                Status status = mBus.registerBusObject(mProperties, "/testProperties");
                logStatus("BusAttachment.registerBusObject()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (status != Status.OK) {
                    finish();
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
                sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

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
                    finish();
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
                    logStatus("BusAttachement.advertiseName()", status);
                    if (status != Status.OK) {
                        /*
                         * If we are unable to advertise the name, release
                         * the name from the local bus.
                         */
                        status = mBus.releaseName(SERVICE_NAME);
                        logStatus(String.format("BusAttachment.releaseName(%s)", SERVICE_NAME), status);
                        finish();
                        return;
                    }
                }

                break;
            }
            case DISCONNECT: {
                mBus.unregisterBusObject(mProperties);
                mBus.disconnect();
                mBusHandler.getLooper().quit();
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
}