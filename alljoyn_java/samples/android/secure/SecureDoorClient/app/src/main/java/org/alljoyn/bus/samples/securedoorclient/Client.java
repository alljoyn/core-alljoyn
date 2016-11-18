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

package org.alljoyn.bus.samples.securedoorclient;

import org.alljoyn.bus.ApplicationStateListener;
import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.PermissionConfigurationListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.PermissionConfigurator.ApplicationState;
import org.alljoyn.bus.SessionListener;
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

import java.util.Set;

public class Client extends Activity {
    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "SecureDoorClient";

    public static final short CONTACT_PORT = 12345;

    protected static final int MESSAGE_ADD_LIST_ENTRY = 1;
    protected static final int MESSAGE_POST_TOAST = 2;

    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;
    private Menu menu;

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

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private Handler mBusHandler;

    /* The AllJoyn objects used by client. */
    private BusAttachment mBus;
    private DoorAboutListener mDoorAboutListener;
    private DoorSessionManager mDoorSessionManager;

    private static final String defaultManifestXml = "<manifest>" +
            "<node>" +
            "<interface name=\"sample.securitymgr.door.Door\">" +
            "<any>" +
            "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
            "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" +
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
            return Status.OK;
        }
        @Override
        public void policyChanged() {}
        @Override
        public void startManagement() {}
        @Override
        public void endManagement() {}
    };

    private ApplicationStateListener mAppStateListener = new ApplicationStateListener() {
        @Override
        public void state(String busName, KeyInfoNISTP256 keyInfoNISTP256, ApplicationState applicationState) {
            Log.d(TAG, String.format("AppStateListener.state(): bus=%s, state=%s", busName, applicationState.name()));

            /* Only process App State change notifications pertaining to our bus attachment */
            if (!mBus.getUniqueName().equals(busName)) return;

            addListEntry("App is " + applicationState.name().toLowerCase());
            if (applicationState == PermissionConfigurator.ApplicationState.CLAIMED) {
                /* Start listening for announcements from the alljoyn service and setup our session mgr */
                if (mDoorAboutListener == null) {
                    mBusHandler.sendEmptyMessage(BusHandler.START);
                }
            }
        }
    };

    private enum SessionLostReason {ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION, ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY,
        ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER, ALLJOYN_SESSIONLOST_LINK_TIMEOUT, ALLJOYN_SESSIONLOST_REASON_OTHER};
    private SessionListener mSessionListener = new SessionListener() {
        @Override
        public void sessionLost(int sessionId, int reason) {
            String busName = mDoorSessionManager.removeSession(sessionId);
            mDoorAboutListener.removeDoorName(busName);
            Log.i(TAG, String.format("sessionLost(%s) : %s", busName, SessionLostReason.values()[reason].name()));
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

        /* Connect the alljoyn client */
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
        case R.id.open:
        case R.id.close:
        case R.id.stateMethod:
        case R.id.stateProperty:
            executeCommand(item.getItemId());
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
        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int START = 2;
        public static final int DISCONNECT = 3;

        public BusHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            /* Connect to the bus and prepare to be claimable. */
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                /*
                 * Create BusAttachment to communication through AllJoyn. The second argument is set
                 * to Receive to allow communication between devices (i.e. bus to bus communication).
                 */
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * Connect the BusAttachment to the bus.
                 */
                Status status = mBus.connect();
                addListEntry(String.format("Starting door consumer (%s)", mBus.getUniqueName()));
                logStatus("connect()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Register auth listener for ECDHE.
                 */
                status = mBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener,
                            Environment.getExternalStorageDirectory().getAbsolutePath()+"/alljoyn.ks",
                            false, mPcListener);
                logStatus("registerAuthListener()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Setting up the permission configurator so that the service is claimable.
                 */
                try {
                    mBus.getPermissionConfigurator().setClaimCapabilities(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL);
                    mBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);
                } catch (BusException e) {
                    logException("Setup Permission Configurator failed", e);
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

                /* Listen for app state change. */
                mBus.registerApplicationStateListener(mAppStateListener);

                break;
            }

            /* Start listen for announcements from the service, and setup the session manager. */
            case START: {
                /* Register about listener */
                mDoorAboutListener = new DoorAboutListener(Client.this);
                mBus.registerAboutListener(mDoorAboutListener);

                /* Listen for door interface announcements */
                Status status = mBus.whoImplements(new String[] {Door.DOOR_INTF_NAME});
                logStatus("Status whoImplements", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                mDoorSessionManager = new DoorSessionManager(mBus, mSessionListener, Client.this);
                break;
            }

            /* Release all resources acquired in connect. */
            case DISCONNECT: {
                if (mDoorSessionManager != null) mDoorSessionManager.stop();
                if (mDoorAboutListener != null) mBus.unregisterAboutListener(mDoorAboutListener);
                mBus.cancelWhoImplements(new String[] {Door.DOOR_INTF_NAME});
                mBus.unregisterApplicationStateListener(mAppStateListener);
                mBus.disconnect();
                mBusHandler.getLooper().quit();
                mDoorSessionManager = null;
                mDoorAboutListener = null;
                break;
            }

            default:
                break;
            }
        }
    }

    private void executeCommand(Integer command) {
        if (mDoorAboutListener == null) {
            addListEntry("Waiting to be claimed");
            return;
        }

        Set<String> doors = mDoorAboutListener.getDoorNames();
        if (doors.size() == 0) {
            addListEntry("No doors found");
            return;
        }

        if (command.equals(R.id.open)) {
            for (String doorName : doors) {
                addListEntry("open " + doorName);
                try {
                    Door door = mDoorSessionManager.getProxyDoorObject(doorName);
                    if (door != null) door.open();
                } catch (BusException e) {
                    logException(String.format("Failed to open door %s",doorName), e);
                    e.printStackTrace();
                }
            }
        } else if (command.equals(R.id.close)) {
            for (String doorName : doors) {
                addListEntry("close " + doorName);
                try {
                    Door door = mDoorSessionManager.getProxyDoorObject(doorName);
                    if (door != null) door.close();
                } catch (BusException e) {
                    logException(String.format("Failed to close door %s",doorName), e);
                    e.printStackTrace();
                }
            }
        } else if (command.equals(R.id.stateProperty)) {
            for (String doorName : doors) {
                try {
                    Door door = mDoorSessionManager.getProxyDoorObject(doorName);
                    String state = " state is unknown";
                    if (door != null) {
                        state = door.getState() ? " is open" : " is closed";
                    }
                    addListEntry(doorName + state);
                } catch (BusException e) {
                    logException(String.format("Failed to get state of door %s (property call)",doorName), e);
                    e.printStackTrace();
                }
            }
        } else if (command.equals(R.id.stateMethod)) {
            for (String doorName : doors) {
                try {
                    Door door = mDoorSessionManager.getProxyDoorObject(doorName);
                    String state = " state is unknown";
                    if (door != null) {
                        state = door.getStateM() ? " is open" : " is closed";
                    }
                    addListEntry(doorName + state);
                } catch (BusException e) {
                    logException(String.format("Failed to get state of door %s (method call)",doorName), e);
                    e.printStackTrace();
                }
            }
        }
    }

    public void addListEntry(String msg) {
        Message entry = mHandler.obtainMessage(MESSAGE_ADD_LIST_ENTRY, msg);
        mHandler.sendMessage(entry);
        Log.i(TAG, msg);
    }

    public void logStatus(String msg, Status status) {
        String log = String.format("%s: %s", msg, status);
        if (status == Status.OK) {
            Log.i(TAG, log);
        } else {
            Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
            mHandler.sendMessage(toastMsg);
            Log.e(TAG, log);
        }
    }

    public void logException(String msg, Exception ex) {
        String log = String.format("%s: %s", msg, ex.getMessage());
        Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
        mHandler.sendMessage(toastMsg);
        Log.e(TAG, log);
    }

}
