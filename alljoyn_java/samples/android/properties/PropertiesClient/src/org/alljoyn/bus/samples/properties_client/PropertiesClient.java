/**
 * This is an Android sample on how to use the AllJoyn properties.
 * This client will get/set two properties from the properties service:
 *    background color (as a string) and text size in dip (as an int)
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
package org.alljoyn.bus.samples.properties_client;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
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
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

/*
 * Properties Client sample code.
 */
public class PropertiesClient extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "PropertiesClient";
    private static final String SERVICE_NAME = "org.alljoyn.bus.samples.properties";

    // Values used by mHandler to specify different types of messages
    private static final int MESSAGE_UPDATE_BACKGROUND_COLOR = 1;
    private static final int MESSAGE_UPDATE_TEXT_SIZE = 2;
    private static final int MESSAGE_POST_TOAST = 3;
    private static final int MESSAGE_START_PROGRESS_DIALOG = 4;
    private static final int MESSAGE_STOP_PROGRESS_DIALOG = 5;

    // Values used to as default text sizes. tiny/small/medium/regular/large/x-large
    private static final int TEXT_TINY = 8;
    private static final int TEXT_SMALL = 12;
    private static final int TEXT_MEDIUM = 16;
    private static final int TEXT_REGULAR = 20;
    private static final int TEXT_LARGE = 24;
    private static final int TEXT_XLARGE = 28;

    String mBackgroundColor;
    int mTextSize;

    //layout items
    Spinner mColorSpinner;
    Spinner mTextSizeSpinner;
    TextView mTextSizeLabel;
    TextView mBackgroundColorLabel;
    Button mGetPropertiesButton;
    Button mSetPropertiesButton;
    Menu menu;

    private BusHandler mBusHandler;
    private ProgressDialog mDialog;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_UPDATE_BACKGROUND_COLOR:
                mBackgroundColor = (String) msg.obj;
                updateColor();
                break;
            case MESSAGE_UPDATE_TEXT_SIZE:
                mTextSize = msg.arg1;
                updateTextSize();
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            case MESSAGE_START_PROGRESS_DIALOG:
                mDialog = ProgressDialog.show(PropertiesClient.this,
                                              "",
                                              "Finding Properties Service.\nPlease wait...",
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
        mTextSizeSpinner.setSelection(3); //3 is regular text size at startup.
        mTextSizeSpinner.setOnItemSelectedListener(new TextSizeSelectedListener());

        mTextSizeLabel = (TextView) findViewById(R.id.text_size_label);
        mBackgroundColorLabel = (TextView) findViewById(R.id.background_color_label);

        mGetPropertiesButton = (Button)  findViewById(R.id.get_properties);
        mGetPropertiesButton.setOnClickListener(new GetPropertiesListener());

        mSetPropertiesButton = (Button)  findViewById(R.id.set_properties);
        mSetPropertiesButton.setOnClickListener(new SetPropertiesListener());

        mBackgroundColor = "";
        mTextSize = -1;

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

        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    /**
     * when a new color is selected from the drop down box selector change the variable
     * mbackgroundColor to match the color selected.
     */
    public class ColorSelectedListener implements OnItemSelectedListener {
        /**
         * update mbackgroundColor based on users selection
         */
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {

            switch(pos) {
            case 0:  mBackgroundColor = "Red";    break;
            case 1:  mBackgroundColor = "Green";  break;
            case 2:  mBackgroundColor = "Blue";   break;
            case 3:  mBackgroundColor = "Yellow"; break;
            default: mBackgroundColor = "Red"; break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // if nothing is selected leave mbackgroundColor the same.
        }
    }

    /**
     * When a new text size is selected change the value of mtextSize to reflect
     * the selected size.
     */
    public class TextSizeSelectedListener implements OnItemSelectedListener {
        /**
         * update mtextSize based on users selection.
         */
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {

            switch(pos) {
            case 0:  mTextSize = TEXT_TINY; break;
            case 1:  mTextSize = TEXT_SMALL; break;
            case 2:  mTextSize = TEXT_MEDIUM; break;
            case 3:  mTextSize = TEXT_REGULAR; break;
            case 4:  mTextSize = TEXT_LARGE; break;
            case 5:  mTextSize = TEXT_XLARGE; break;
            default: mTextSize = TEXT_REGULAR; break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // if nothing is selected leave mtextSize the same.
        }
    }

    /*
     * When the 'Get Properties' button is pressed get the background color
     * and text size from the properties service.  Update the values shown on
     * the screen to reflect the values obtained from the service.
     */
    public class GetPropertiesListener implements View.OnClickListener {
        public void onClick(View v) {

            mBusHandler.sendEmptyMessage(BusHandler.GET_BACKGROUND_COLOR_PROPERTY);
            mBusHandler.sendEmptyMessage(BusHandler.GET_TEXT_SIZE_PROPERTY);
        }
    }

    /*
     * Send the values stored in mbackgroundColor and mtextSize to the
     * properties service.
     * Assume that the service has changed to match the values selected and
     * update the screen to reflect that assumption.
     */
    public class SetPropertiesListener implements View.OnClickListener {
        public void onClick(View v) {
            Message msg = mBusHandler.obtainMessage(BusHandler.SET_BACKGROUND_COLOR_PROPERTY,
                                                    mBackgroundColor);
            mBusHandler.sendMessage(msg);
            msg = mBusHandler.obtainMessage(BusHandler.SET_TEXT_SIZE_PROPERTY);
            msg.arg1 = mTextSize;
            mBusHandler.sendMessage(msg);
        }

    }

    /*
     * update the screen to match the value stored in mbackgroundColor
     */
    private void updateColor() {
        mBackgroundColorLabel.setText(mBackgroundColor);
        if (mBackgroundColor.equals("Red")) {
            mColorSpinner.setSelection(0);
        } else if (mBackgroundColor.equals("Green")) {
            mColorSpinner.setSelection(1);
        } else if (mBackgroundColor.equals("Blue")) {
            mColorSpinner.setSelection(2);
        } else if (mBackgroundColor.equals("Yellow")) {
            mColorSpinner.setSelection(3);
        }
    }

    /*
     * update the screen to reflect the value stored in mtextSize
     * the values are divided into five text sizes.
     * tiny/small/medium/regular/large/x-large
     */
    private void updateTextSize() {
        if (mTextSize <= TEXT_TINY) {
            mTextSizeSpinner.setSelection(0);
            mTextSizeLabel.setText(getResources().getStringArray(R.array.text_size_array)[0]);
        } else if (mTextSize <= TEXT_SMALL) {
            mTextSizeSpinner.setSelection(1);
            mTextSizeLabel.setText(getResources().getStringArray(R.array.text_size_array)[1]);
        } else if (mTextSize <= TEXT_MEDIUM) {
            mTextSizeSpinner.setSelection(2);
            mTextSizeLabel.setText(getResources().getStringArray(R.array.text_size_array)[2]);
        } else if (mTextSize <= TEXT_REGULAR) {
            mTextSizeSpinner.setSelection(3);
            mTextSizeLabel.setText(getResources().getStringArray(R.array.text_size_array)[3]);
        } else if (mTextSize <= TEXT_LARGE) {
            mTextSizeSpinner.setSelection(4);
            mTextSizeLabel.setText(getResources().getStringArray(R.array.text_size_array)[4]);
        } else { //the text size is larger than TEXT_LARGE display that x-large is being used
            mTextSizeSpinner.setSelection(5);
            mTextSizeLabel.setText(getResources().getStringArray(R.array.text_size_array)[5]);
        }
    }

    /*
     * This class will handle all AllJoyn calls.
     * For a detailed description of the code shown in the BusHandler
     * class see the SimpleClient sample code.
     */
    class BusHandler extends Handler {

        // Values used by mBusHandler to specify different types of messages
        public static final int CONNECT = 1;
        public static final int JOIN_SESSION = 2;
        public static final int DISCONNECT = 3;
        public static final int GET_BACKGROUND_COLOR_PROPERTY = 4;
        public static final int SET_BACKGROUND_COLOR_PROPERTY = 5;
        public static final int GET_TEXT_SIZE_PROPERTY = 6;
        public static final int SET_TEXT_SIZE_PROPERTY = 7;


        private BusAttachment mBus;
        private ProxyBusObject mProxyObj;
        private PropertiesInterface mPropertiesInterface;

        private int     mSessionId;
        private boolean mIsConnected;
        private boolean mIsStoppingDiscovery;

        private static final short CONTACT_PORT=42;

        public BusHandler(Looper looper) {
            super(looper);

            mIsConnected = false;
            mIsStoppingDiscovery = false;
        }

        public void handleMessage(Message msg) {
            switch(msg.what) {
            case (CONNECT): {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

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

                // Connect the BusAttachment with the bus.
                Status status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (Status.OK != status) {
                    finish();
                    return;
                }

                status = mBus.findAdvertisedName(SERVICE_NAME);
                logStatus("BusAttachement.findAdvertisedName()", status);
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
                        logInfo(String.format("MyBusListener.sessionLost(sessionId = %d, reason = %d)", sessionId,reason));
                        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
                    }
                });
                logStatus("BusAttachment.joinSession()", status);
                if (status == Status.OK) {
                    mProxyObj =  mBus.getProxyBusObject(SERVICE_NAME,
                                                        "/testProperties",
                                                        sessionId.value,
                                                        new Class<?>[] { PropertiesInterface.class });

                    mPropertiesInterface = mProxyObj.getInterface(PropertiesInterface.class);
                    mSessionId = sessionId.value;
                    mIsConnected = true;
                    mHandler.sendEmptyMessage(MESSAGE_STOP_PROGRESS_DIALOG);
                }
                break;
            }
            case (DISCONNECT): {
                mIsStoppingDiscovery = true;
                if (mIsConnected) {
                    Status status = mBus.leaveSession(mSessionId);
                    logStatus("BusAttachment.leaveSession()", status);
                }
                mBus.disconnect();
                getLooper().quit();
                break;
            }
            case (GET_BACKGROUND_COLOR_PROPERTY): {
                if (!mIsConnected) {
                    break;
                }
                try {
                    String backgroundColor = mPropertiesInterface.getBackGroundColor();
                    mHandler.sendMessage(mHandler.obtainMessage(MESSAGE_UPDATE_BACKGROUND_COLOR, backgroundColor));
                } catch (BusException e) {
                }
                break;
            }
            case (SET_BACKGROUND_COLOR_PROPERTY): {
                if (!mIsConnected) {
                    break;
                }
                try {
                    mPropertiesInterface.setBackGroundColor((String)msg.obj);
                    mHandler.sendMessage(mHandler.obtainMessage(MESSAGE_UPDATE_BACKGROUND_COLOR, (String) msg.obj));
                } catch (BusException e) {
                    logException(getString(R.string.get_properties_error), e);
                }
                break;
            }
            case (GET_TEXT_SIZE_PROPERTY): {
                if (!mIsConnected) {
                    break;
                }
                try {
                    int textSize = mPropertiesInterface.getTextSize();
                    Message textMsg = mHandler.obtainMessage(MESSAGE_UPDATE_TEXT_SIZE);
                    textMsg.arg1 = textSize;
                    mHandler.sendMessage(textMsg);
                } catch (BusException e) {
                    logException(getString(R.string.get_properties_error), e);
                }
                break;
            }
            case (SET_TEXT_SIZE_PROPERTY): {
                if (!mIsConnected) {
                    break;
                }
                try {
                    mPropertiesInterface.setTextSize(msg.arg1);
                    Message textMsg = mHandler.obtainMessage(MESSAGE_UPDATE_TEXT_SIZE);
                    textMsg.arg1 = msg.arg1;
                    mHandler.sendMessage(textMsg);
                } catch (BusException e) {
                    logException(getString(R.string.get_properties_error), e);
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
