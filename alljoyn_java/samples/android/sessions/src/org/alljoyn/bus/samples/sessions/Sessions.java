/*
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.sessions;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;

import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.widget.ToggleButton;

public class Sessions extends Activity {

    /* Load the native alljoyn_java library. */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "Sessions";

    /* Configure the following three variables to suit your needs */

    /* Which transport(s) would you like to advertise on? */
    private short AdvertiseOn = SessionOpts.TRANSPORT_WFD;

    /* Enter the network credentials if you wish to connect AP from the application. */
    private String networkName = "Enter SSID";
    private String networkPassword = "Enter Password";

    /* Are you finding other devices running the same application?
     * If so, modify this variable and your service names will be personalized!
     *
     * For e.g.: The default service names are "cool.app.uno, cool.app.dos, etc.
     *
     * Modifying the string to "foo" will cause the service names
     * to be "cool.foo.uno", "cool.foo.dos", etc.
     */
    private String personalizeMyServices = "app";

    /* Configuration ends here */

    private static final int MESSAGE_POST_TOAST = 1;

    private String operation;
    private String operationComplete;

    private String serviceToBeDiscovered;
    private String serviceToBeAdvertised;
    private String serviceToBeCancelDiscovered;
    private String serviceToBeCancelAdvertised;

    private boolean discoverStatus;

    private short transportFoundOn;

    private WifiConfiguration myConf;
    private int networkId = 0;

    private int connectID = 0;
    private String connectTo;

    private String deviceName = "s" + android.os.Build.SERIAL.toLowerCase();

    ArrayList<String> advertisingList = new ArrayList<String>();
    ArrayList<String> discoveringList = new ArrayList<String>();
    ArrayList<String> discoveredList = new ArrayList<String>();

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }
        }
    };

    public class MyPingFunc extends Thread {
        public void run(){
            String pingReq = "ping 4.2.2.2";
            Runtime reqExec = Runtime.getRuntime();
            int count = 1;
            Process proExec;
            try {
                proExec = reqExec.exec(pingReq);
                BufferedReader inpStr = new BufferedReader (new InputStreamReader(proExec.getInputStream()));
                while (count <= 20 && inpStr.readLine() != null) {
                    Log.d (TAG, inpStr.readLine());
                    count++;
                }
                inpStr.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
            runOnUiThread (new Runnable() {
                public void run() {
                    ToggleButton ICMPPing = (ToggleButton) findViewById(R.id.ICMPPing);
                    ICMPPing.setChecked(false);
                }
            });
        }
    }

    public void ServiceNameAlertBuilder(final String operation) {

        CharSequence[] advItems = {"cool."+personalizeMyServices+".uno", "cool."+personalizeMyServices+".dos", "cool."+personalizeMyServices+".tres"};
        CharSequence[] disItems = {"cool."+personalizeMyServices+".uno", "cool."+personalizeMyServices+".dos", "cool."+personalizeMyServices+".tres", "cool."+personalizeMyServices+" (Prefix)", "* (All)"};

        final CharSequence[] items;

        if (operation == "Find" || operation == "CancelFind") {
            items = disItems;
        } else {
            items = advItems;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Please select a service name...");
        builder.setItems(items, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                if (operation == "Find") {
                    if (items[item].toString().contains("Prefix"))
                        serviceToBeDiscovered = "cool."+personalizeMyServices;
                    else if (items[item].toString().contains("All"))
                        serviceToBeDiscovered = "";
                    else
                        serviceToBeDiscovered = items[item].toString();
                    mBusHandler.sendEmptyMessage(BusHandler.FIND);
                    DisplayProgress("Enabling selected discovery, please wait...", null, "Discover", 4000);
                }
                else if (operation == "Advertise") {
                    serviceToBeAdvertised = items[item].toString();
                    mBusHandler.sendEmptyMessage(BusHandler.ADVERTISE);
                    DisplayProgress("Enabling selected advertisement, please wait...", "Advertisements for "+serviceToBeAdvertised+"."+deviceName+" are now enabled.", "Advertise", 4000);
                }
                else if (operation == "CancelFind") {
                    if (items[item].toString().contains("Prefix"))
                        serviceToBeCancelDiscovered = "cool."+personalizeMyServices;
                    else if (items[item].toString().contains("All"))
                        serviceToBeCancelDiscovered = "";
                    else
                        serviceToBeCancelDiscovered = items[item].toString();
                    mBusHandler.sendEmptyMessage(BusHandler.CANCELFIND);
                    DisplayProgress("Disabling selected discovery, please wait...", "Discovery for "+serviceToBeCancelDiscovered+" is now disabled.", null, 4000);
                }
                else if (operation == "CancelAdvertise") {
                    serviceToBeCancelAdvertised = items[item].toString();
                    mBusHandler.sendEmptyMessage(BusHandler.CANCELADVERTISE);
                    DisplayProgress("Disabling selected advertisement, please wait...", "Advertisements for "+serviceToBeAdvertised+"."+deviceName+" are now disabled.", "Advertise", 4000);
                }
            }
        });
        AlertDialog alert = builder.create();
        alert.show();
    }

    /* The AllJoyn object that is our service. */
    private SessionsService mSessionsService;

    /* Handler used to make calls to AllJoyn methods. See onCreate(). */
    private Handler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /* Make all AllJoyn calls through a separate handler thread to prevent blocking the UI. */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);

        String service = Context.WIFI_SERVICE;
        final WifiManager localWiFiAdapter = (WifiManager)getSystemService(service);
        final ToggleButton WiFi = (ToggleButton) findViewById(R.id.WiFi);

        if (localWiFiAdapter == null) {
            Toast toast = Toast.makeText(getApplicationContext(), "The device does not support WiFi.", Toast.LENGTH_LONG);
            toast.setGravity(Gravity.BOTTOM|Gravity.FILL_HORIZONTAL, 0, 0);
            toast.show();
        } else if (localWiFiAdapter.isWifiEnabled()) {
            WiFi.setChecked(true);
        } else {
            WiFi.setChecked(false);
        }

        myConf = new WifiConfiguration();
        myConf.SSID = "\""+networkName+"\"";
        myConf.preSharedKey = "\""+networkPassword+"\"";
        myConf.status = WifiConfiguration.Status.ENABLED;

        WiFi.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View WiFiOption) {

                if (WiFi.isChecked()) {
                    localWiFiAdapter.setWifiEnabled(true);
                    DisplayProgress("Turning ON Wi-Fi, please wait...", "Wi-Fi is turned ON.", null, 4000);
                } else {
                    localWiFiAdapter.setWifiEnabled(false);
                    DisplayProgress("Turning OFF Wi-Fi, please wait...", "Wi-Fi is turned OFF.", null, 4000);

                    final ToggleButton ConnectToAP = (ToggleButton) findViewById(R.id.ConnectToAP);
                    ConnectToAP.setChecked(false);
                }
            }
        });

        final ToggleButton ConnectToAP = (ToggleButton) findViewById(R.id.ConnectToAP);
        ConnectToAP.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if(localWiFiAdapter.isWifiEnabled()) {
                    if(ConnectToAP.isChecked()) {
                        if (networkName == "Enter SSID" || networkPassword == "Enter Password") {
                            makeAToast("Please set the SSID and Password in the Test Application, then retry...", "High");
                            ConnectToAP.setChecked(false);
                            return;
                        }
                        networkId = localWiFiAdapter.addNetwork(myConf);
                        localWiFiAdapter.enableNetwork(networkId, true);
                        localWiFiAdapter.reconnect();
                        DisplayProgress("Connecting to "+networkName+", please wait...", "You should now be connected to "+networkName+".", null, 10000);
                    } else {
                        localWiFiAdapter.disconnect();
                        localWiFiAdapter.removeNetwork(networkId);
                        DisplayProgress("Disconnecting from "+networkName+", please wait...", "You should now be disconnected from "+networkName+".", null, 4000);
                    }
                } else {
                    makeAToast("Wi-Fi is currently turned OFF. Please turn it ON and retry...", "High");
                    ConnectToAP.setChecked(false);
                }
            }
        });

        final ToggleButton ICMPPing = (ToggleButton) findViewById(R.id.ICMPPing);
        ICMPPing.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (ICMPPing.isChecked()) {
                    MyPingFunc myPingThread = new MyPingFunc();
                    myPingThread.start();
                } else {
                    makeAToast("The Ping operation will automatically stop after 20 iterations, please wait...", "High");
                    ICMPPing.setChecked(true);
                }
            }
        });

        Button mFindAdvertisedNameButton = (Button)findViewById(R.id.FindAdvertisedName);
        mFindAdvertisedNameButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                ServiceNameAlertBuilder("Find");
            }
        });

        Button mCancelFindAdvertisedNameButton = (Button)findViewById(R.id.CancelFindAdvertisedName);
        mCancelFindAdvertisedNameButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                ServiceNameAlertBuilder("CancelFind");
            }
        });

        final Button mFindingNamesButton = (Button)findViewById(R.id.DiscoveringList);
        mFindingNamesButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mFindingNamesButton.isEnabled()) {
                    String AllDiscoveries = "";

                    if (discoveringList != null) {
                        for (String disv: discoveringList) {
                            if (disv != null) {
                                AllDiscoveries += "\n\n# ";
                                AllDiscoveries += disv;
                            }
                        }
                    }

                    if (discoveringList == null || AllDiscoveries == "")
                        makeAToast("No services are currently being discovered for.", "Low");
                    else
                        makeAToast("The following services are currently being discovered for..."+AllDiscoveries, "High");
                }
            }
        });

        final Button mFoundNamesButton = (Button)findViewById(R.id.DiscoveredList);
        mFoundNamesButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mFoundNamesButton.isEnabled()) {
                    String AllDiscs = "";

                    if (discoveredList != null) {
                        for (String dis: discoveredList) {
                            if (dis != null) {
                                AllDiscs += "\n\n# ";
                                AllDiscs += dis;
                            }
                        }
                    }

                    if (discoveredList == null || AllDiscs == "")
                        makeAToast("No services successfully discovered yet.", "Low");
                    else
                        makeAToast("The following services have been successfully discovered..."+AllDiscs, "High");
                }
            }
        });

        Button mAdvertiseNameButton = (Button)findViewById(R.id.AdvertiseName);
        mAdvertiseNameButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                ServiceNameAlertBuilder("Advertise");
            }
        });

        Button mCancelAdvertiseNameButton = (Button)findViewById(R.id.CancelAdvertiseName);
        mCancelAdvertiseNameButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                ServiceNameAlertBuilder("CancelAdvertise");
            }
        });

        final Button mAdvertisedNamesButton = (Button)findViewById(R.id.AdvertisedList);
        mAdvertisedNamesButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mAdvertisedNamesButton.isEnabled()) {
                    String AllAdvs = "";

                    if (advertisingList != null) {
                        for (String adv: advertisingList) {
                            if (adv != null) {
                                AllAdvs += "\n\n# ";
                                AllAdvs += adv;
                            }
                        }
                    }

                    if (advertisingList == null || AllAdvs == "")
                        makeAToast("No services successfully advertised yet.", "Low");
                    else
                        makeAToast("The following services are currently being advertised..."+AllAdvs, "High");
                }
            }
        });

        final ToggleButton mRemoteConnect = (ToggleButton)findViewById(R.id.RemoteConnect);
        mRemoteConnect.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mRemoteConnect.isChecked()) {
                    if (discoverStatus == false) {
                        makeAToast("Please Find a remote Service you wish to connect to and retry...", "High");
                        mRemoteConnect.setChecked(false);
                    }
                    else if (discoveredList != null) {
                        for (String dis: discoveredList) {
                            if (dis != null) {
                                connectTo = dis;
                                mBusHandler.sendEmptyMessage(BusHandler.JOIN);
                                mRemoteConnect.setChecked(false);
                                DisplayProgress("Connecting to remote Peer, please wait...", null, null, 10000);
                            }
                        }
                    }
                } else {
                    mBusHandler.sendEmptyMessage(BusHandler.LEAVE);
                    DisplayProgress("Disconnecting from remote Peer, please wait...", "You should now be disconnected to remote Peer.", null, 10000);
                }
            }
        });

        Button mDataTransfer = (Button)findViewById(R.id.TransferData);
        mDataTransfer.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (connectID == 0) {
                    makeAToast("Please establish a Session with the remote Peer before attempting to send data...", "High");
                } else {
                    mBusHandler.sendEmptyMessage(BusHandler.TRANSFERDATA);
                }
            }
        });

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_main, menu);
        return true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        /* Disconnect to prevent any resource leaks. */
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    /* The class that is our AllJoyn service.  It implements the SimpleInterface. */
    class SessionsService implements SessionsInterface, BusObject {

        /*
         * This is the code run when the client makes a call to the Ping method of the
         * SimpleInterface.  This implementation just returns the received String to the caller.
         *
         * This code also prints the string it received from the user and the string it is
         * returning to the user to the screen.
         */
        public String Ping(String inStr) {
            Log.d(TAG, "Ping Received: " + inStr);
            return ("Ping Response: " + inStr);
        }
    }

    /* This class will handle all AllJoyn calls. See onCreate(). */
    @SuppressLint("HandlerLeak")
    class BusHandler extends Handler {

        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int FIND = 2;
        public static final int CANCELFIND = 3;
        public static final int ADVERTISE = 4;
        public static final int CANCELADVERTISE = 5;
        public static final int JOIN = 6;
        public static final int LEAVE = 7;
        public static final int DISCONNECT = 8;
        public static final int TRANSFERDATA = 9;

        private static final short CONTACT_PORT=545;

        private BusAttachment mBus;

        private ProxyBusObject mProxyObj;
        private SessionsInterface mSessionsInterface;

        public BusHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case CONNECT: {
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /* Start our service. */
                mSessionsService = new SessionsService();

                Status status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);

                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = AdvertiseOn;

                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        if (sessionPort == CONTACT_PORT) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                    public void sessionJoined(short sessionPort, int id, String joiner) {
                        Log.i(TAG, "SessionPortListener.sessionJoined(" + sessionPort + ", " + id + ", " + joiner + ")");
                        runOnUiThread (new Runnable() {
                            public void run() {
                                ToggleButton mRemoteConnect = (ToggleButton)findViewById(R.id.RemoteConnect);
                                mRemoteConnect.setChecked(true);
                                makeAToast("###  SUCCESS  ###\n\n"+"Session Established (ID = "+connectID+")", "High");
                            }
                        });
                        connectID = id;
                        mBus.setSessionListener(id, new SessionListener() {
                            public void sessionLost(final int sessionId, int reason) {
                                Log.i(TAG, "BusListener.sessionLost(sessionId=" + sessionId + ",reason=" + reason + ")");
                                runOnUiThread (new Runnable() {
                                    public void run() {
                                        ToggleButton mRemoteConnect = (ToggleButton)findViewById(R.id.RemoteConnect);
                                        mRemoteConnect.setChecked(false);
                                        makeAToast("###  SUCCESS  ###\n\n"+"Session Lost (ID = "+sessionId+")", "High");
                                    }
                                });
                                connectID = 0;
                            }
                        });
                    }
                });
                logStatus(String.format("BusAttachment.bindSessionPort(%d, %s)",
                                        contactPort.value, sessionOpts.toString()), status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                status = mBus.registerBusObject(mSessionsService, "/SessionsService");
                logStatus("BusAttachment.registerBusObject()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                mBus.registerBusListener(new BusListener() {
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                        Log.e(TAG, String.format("org.alljoyn.Bus.FoundName signal detected."));
                        discoverStatus = true;

                        transportFoundOn = transport;

                        if (!discoveredList.contains(name))
                            discoveredList.add(name);

                        runOnUiThread (new Runnable() {
                            public void run() {
                                makeAToast("###  SUCCESS  ###\n\nThe '"+serviceToBeDiscovered+"' service was found.\n\nClick 'Joyn with Peer' to connect...", "High");
                            }
                        });
                    }
                    @Override
                    public void lostAdvertisedName (String name, short transport, String namePrefix) {
                        Log.e(TAG, String.format("org.alljoyn.Bus.LostName signal detected."));
                        discoverStatus = false;

                        if (discoveredList.contains(name))
                            discoveredList.remove(name);

                        runOnUiThread (new Runnable() {
                            public void run() {
                                makeAToast("###  SUCCESS  ###\n\nThe '"+serviceToBeDiscovered+"' service was lost.", "Low");
                            }
                        });
                    }
                });
                break;
            }
            case FIND: {
                discoverStatus = false;
                Status status = mBus.findAdvertisedName(serviceToBeDiscovered);
                logStatus(String.format("BusAttachment.findAdvertisedName(%s)", serviceToBeDiscovered), status);
                if (Status.OK != status) {
                    return;
                }
                if (!discoveringList.contains(serviceToBeDiscovered)) {
                    discoveringList.add(serviceToBeDiscovered);
                }
                break;
            }
            case CANCELFIND: {
                Status status = mBus.cancelFindAdvertisedName(serviceToBeCancelDiscovered);
                logStatus(String.format("BusAttachment.cancelFindAdvertisedName(%s)", serviceToBeCancelDiscovered), status);
                if (Status.OK != status) {
                    return;
                }
                if (discoveringList.contains(serviceToBeCancelDiscovered)) {
                    discoveringList.remove(serviceToBeCancelDiscovered);
                }
                break;
            }
            case ADVERTISE: {
                int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;

                Status status = mBus.requestName(serviceToBeAdvertised+"."+deviceName, flag);
                logStatus(String.format("BusAttachment.requestName(%s, 0x%08x)", serviceToBeAdvertised+"."+deviceName, flag), status);
                if (status != Status.OK) {
                    return;
                }
                status = mBus.advertiseName(serviceToBeAdvertised+"."+deviceName, AdvertiseOn);
                logStatus(String.format("BusAttachment.advertiseName(%s)", serviceToBeAdvertised+"."+deviceName), status);
                if (status != Status.OK) {
                    status = mBus.releaseName(serviceToBeAdvertised+"."+deviceName);
                    logStatus(String.format("BusAttachment.releaseName(%s)", serviceToBeAdvertised+"."+deviceName), status);
                    return;
                }
                if (!advertisingList.contains(serviceToBeAdvertised+"."+deviceName)) {
                    advertisingList.add(serviceToBeAdvertised+"."+deviceName);
                }
                break;
            }
            case CANCELADVERTISE: {
                Status status = mBus.cancelAdvertiseName(serviceToBeCancelAdvertised+"."+deviceName, AdvertiseOn);
                logStatus(String.format("BusAttachment.cancelAdvertiseName(%s)", serviceToBeCancelAdvertised+"."+deviceName), status);
                if (status != Status.OK) {
                    return;
                }
                status = mBus.releaseName(serviceToBeCancelAdvertised+"."+deviceName);
                logStatus(String.format("BusAttachment.releaseName(%s)", serviceToBeCancelAdvertised+"."+deviceName), status);
                if (status != Status.OK) {
                    return;
                }
                if (advertisingList.contains(serviceToBeCancelAdvertised+"."+deviceName)) {
                    advertisingList.remove(serviceToBeCancelAdvertised+"."+deviceName);
                }
                break;
            }
            case JOIN: {
                SessionOpts sessionOpts = new SessionOpts();
                Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

                sessionOpts.transports = transportFoundOn;
                Log.d(TAG, "Join Session initiated over the "+sessionOpts.transports+" transport.");

                final Status status = mBus.joinSession(connectTo, CONTACT_PORT, sessionId, sessionOpts, new SessionListener() {
                    public void sessionLost(final int sessionId, int reason) {
                        runOnUiThread (new Runnable() {
                            public void run() {
                                final ToggleButton mRemoteConnect = (ToggleButton)findViewById(R.id.RemoteConnect);
                                mRemoteConnect.setChecked(false);
                                logStatus(String.format("MyBusListener.sessionLost(sessionId=" + sessionId + ",reason=" + reason + ")"), null);
                                makeAToast("###  SUCCESS  ###\n\n"+"Session Lost (ID = "+sessionId+")", "High");
                            }
                        });
                    }
                });
                if (status != Status.OK) {
                    runOnUiThread (new Runnable() {
                        public void run() {
                            final ToggleButton mRemoteConnect = (ToggleButton)findViewById(R.id.RemoteConnect);
                            mRemoteConnect.setChecked(false);
                            makeAToast("!!!  ERROR  !!!\n\n"+"Session NOT Established.\n(Error Code = " + status.toString() + ")", "High");
                        }
                    });
                    Log.d (TAG, status.toString());
                    return;
                }
                runOnUiThread (new Runnable() {
                    public void run() {
                        final ToggleButton mRemoteConnect = (ToggleButton)findViewById(R.id.RemoteConnect);
                        mRemoteConnect.setChecked(true);
                    }
                });
                logStatus("BusAttachment.joinSession() - sessionId: " + sessionId.value, status);
                connectID = sessionId.value;
                runOnUiThread (new Runnable() {
                    public void run() {
                        makeAToast("###  SUCCESS  ###\n\n"+"Session Established (ID = "+connectID+")", "High");
                    }
                });
                break;
            }
            case LEAVE: {
                Status status = Status.OK;
                if (connectID != 0)
                    status = mBus.leaveSession(connectID);
                else
                    makeAToast("You have not joined any sessions yet.", "High");
                if (status != Status.OK) {
                    return;
                }
                connectID = 0;
                break;
            }
            case DISCONNECT: {
                mBus.disconnect();
                getLooper().quit();
                break;
            }
            case TRANSFERDATA: {
                mProxyObj =  mBus.getProxyBusObject(connectTo,
                                                    "/SessionsService",
                                                    connectID,
                                                    new Class<?>[] { SessionsInterface.class });

                mSessionsInterface =  mProxyObj.getInterface(SessionsInterface.class);
                String reply;
                try {
                    int counter = 1;
                    while (counter <= 100) {
                        Log.d(TAG, "Ping Request: MC #" + counter);
                        reply = mSessionsInterface.Ping("MC #" + counter);
                        Log.d(TAG, reply);
                        counter++;
                    }
                } catch (BusException e) {
                    e.printStackTrace();
                }
                break;
            }
            default:
                break;
            }
        }
    }

    private void makeAToast (String sMessage, String sImportance) {
        int displayTime = Toast.LENGTH_SHORT * 2;

        if (sImportance == "High")
            displayTime = Toast.LENGTH_LONG;

        Toast toast = Toast.makeText(getApplicationContext(), sMessage, displayTime);
        toast.setGravity(Gravity.BOTTOM|Gravity.FILL_HORIZONTAL, 0, 0);
        toast.show();
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

    public void DisplayProgress (String sMessage, String oMessage, String sOperation, final int iDuration) {
        final ProgressDialog dialog;
        dialog = new ProgressDialog(this);
        operation = sOperation;
        operationComplete = oMessage;
        dialog.setMessage(sMessage);
        dialog.show();
        Thread background = new Thread (new Runnable() {
            public void run() {
                try {
                    Thread.sleep(iDuration);
                    dialog.dismiss();
                } catch (java.lang.InterruptedException ex) {
                    /* Do something */
                }
            }
        });
        background.start();
        dialog.setOnDismissListener(new DialogInterface.OnDismissListener()
        {
            public void onDismiss(DialogInterface dialog) {
                if (!discoverStatus && operation == "Discover")
                    makeAToast("!!!  INFO  !!!\n\nThe '"+serviceToBeDiscovered+"' service could not be discovered. The discovery will remain active until cancelled.", "High");
                if (operationComplete != null)
                    makeAToast("###  SUCCESS  ###\n\n"+operationComplete, "Low");
            }
        });
    }

}
