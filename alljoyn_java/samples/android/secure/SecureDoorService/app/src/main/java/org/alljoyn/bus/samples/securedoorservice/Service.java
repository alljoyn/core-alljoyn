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

package org.alljoyn.bus.samples.securedoorservice;

import org.alljoyn.bus.alljoyn.DaemonInit;

import org.alljoyn.bus.AboutObj;
import org.alljoyn.bus.ApplicationStateListener;
import org.alljoyn.bus.PermissionConfigurator.ApplicationState;
import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.PermissionConfigurationListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.common.KeyInfoNISTP256;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

public class Service extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "SecureDoorService";

    protected static final int MESSAGE_ADD_LIST_ENTRY = 1;
    protected static final int MESSAGE_POST_TOAST = 2;

    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;

    /* Handler used to update UI display. */
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_ADD_LIST_ENTRY:
                mListViewArrayAdapter.add((String)msg.obj);
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }
        }
    };

    private BusAttachment mBus;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private Handler mBusHandler;

    /* The AllJoyn object that is our service. */
    private DoorImpl mDoorService;

    private static final String DEFAULT_MANIFEST_XML = "<manifest>" +
            "<node>" +
            "<interface>" +
            "<any>" +
            "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
            "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" +
            "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" +
            "</any>" +
            "</interface>" +
            "</node>" +
            "</manifest>";

    private AuthListener mAuthListener = new AuthListener() {
        @Override
        public boolean requested(String mechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
            return true;
        }
        @Override
        public void completed(String mechanism, String authPeer, boolean authenticated) {}
    };

    private PermissionConfigurationListener mPcListener = new PermissionConfigurationListener() {
        @Override
        public Status factoryReset() {
            Log.i(TAG, "factoryReset()");
            return Status.OK;
        }
        @Override
        public void policyChanged() {
            Log.i(TAG, "policyChanged()");
        }
        @Override
        public void startManagement() {
            Log.i(TAG, "startManagement()");
        }
        @Override
        public void endManagement() {
            Log.i(TAG, "endManagement()");
        }
    };

    private ApplicationStateListener mAppStateListener = new ApplicationStateListener() {
        @Override
        public void state(String busName, KeyInfoNISTP256 keyInfoNISTP256, ApplicationState applicationState) {
            Log.d(TAG, String.format("AppStateListener.state(): bus=%s, state=%s", busName, applicationState.name()));

            if (mBus.getUniqueName().equals(busName)) {
                addListEntry("App is " + applicationState.name().toLowerCase());
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

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        /* Start our service. */
        mDoorService = new DoorImpl("kitchen", mHandler);
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
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

        /* Disconnect to prevent any resource leaks. */
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }


    /* This class will handle all AllJoyn calls. See onCreate(). */
    class BusHandler extends Handler {
        private static final short CONTACT_PORT = 12345;

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
                DaemonInit.PrepareDaemon(getApplicationContext());

                /*
                 * Create BusAttachment to communication through AllJoyn. The second argument is set
                 * to Receive to allow communication between devices (i.e. bus to bus communication).
                 */
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * Connect the BusAttachment to the bus.
                 */
                Status status = mBus.connect();
                addListEntry(String.format("Starting door producer (%s)", mBus.getUniqueName()));
                logStatus("connect()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Register auth listener for ECDHE.
                 */
                status = mBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                        mAuthListener,
                        Environment.getExternalStorageDirectory().getAbsolutePath() + "/alljoyn.ks",
                        false,
                        mPcListener);
                if (status != Status.OK) {
                    logStatus("registerAuthListener()", status);
                    finish();
                    return;
                }

                /*
                 * Register the door BusObject with the BusAttachment at the "/sample/security/Door" path.
                 */
                status = mBus.registerBusObject(mDoorService, "/sample/security/Door");
                logStatus("registerBusObject()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /* Listen for app state changes */
                mBus.registerApplicationStateListener(mAppStateListener);

                /*
                 * Setting up the permission configurator so that the service is claimable.
                 */
                try {
                    mBus.getPermissionConfigurator().setClaimCapabilities(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL);
                    mBus.getPermissionConfigurator().setManifestTemplateFromXml(DEFAULT_MANIFEST_XML);
                } catch (BusException e) {
                    logException("Setup Permission Configurator", e);
                    finish();
                    return;
                }

                /*
                 * Create a new session listening on the contact port of the door service.
                 */
                Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);
                SessionOpts sessionOpts = new SessionOpts();
                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        Log.i(TAG, "SessionPortListener.acceptSessionJoiner called");
                        return (sessionPort == CONTACT_PORT);
                    }
                });
                logStatus("bindSessionPort()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Using about announcements to be discovered by the security manager.
                 */
                AboutObj aboutObj = new AboutObj(mBus);
                status = aboutObj.announce(contactPort.value, new MyAboutData());
                logStatus("announce()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                break;
            }

            /* Release all resources acquired in connect. */
            case DISCONNECT: {
                Log.i(TAG, "quit");
                mBus.unregisterApplicationStateListener(mAppStateListener);
                mBus.unregisterBusObject(mDoorService);
                mBus.disconnect();
                mBus.release();
                mBusHandler.getLooper().quit();
                break;
            }

            default:
                break;
            }
        }
    }

    private void addListEntry(String msg) {
        Message entry = mHandler.obtainMessage(MESSAGE_ADD_LIST_ENTRY, msg);
        mHandler.sendMessage(entry);
        Log.i(TAG, msg);
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

    private void logException(String msg, Exception ex) {
        String log = String.format("%s: %s", msg, ex.getMessage());
        Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
        mHandler.sendMessage(toastMsg);
        Log.e(TAG, log);
    }

}
