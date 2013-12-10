/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.p2p.service;

import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.lang.Runnable;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.content.Context;
import android.content.IntentFilter;
import android.net.wifi.WpsInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pManager;
import android.net.wifi.p2p.WifiP2pManager.ActionListener;
import android.net.wifi.p2p.WifiP2pManager.Channel;
import android.net.wifi.p2p.WifiP2pManager.ConnectionInfoListener;
import android.net.wifi.p2p.WifiP2pManager.GroupInfoListener;
import android.net.wifi.p2p.WifiP2pManager.DnsSdServiceResponseListener;
import android.net.wifi.p2p.WifiP2pManager.DnsSdTxtRecordListener;
import android.net.wifi.p2p.WifiP2pManager.PeerListListener;
import android.net.wifi.p2p.nsd.WifiP2pDnsSdServiceRequest;
import android.net.wifi.p2p.nsd.WifiP2pDnsSdServiceInfo;
import android.util.Log;
import android.os.Handler;

class PeerState {
    private int state;
    public static final int DISCONNECTED = 1;
    public static final int INITIATED = 2;
    public static final int CONNECTING = 3;
    public static final int CONNECTED = 4;
    public static final int DISCONNECTING = 5;

    public PeerState(int s) {
        this.state = s;
    }

    public int get() {
        return this.state;
    }
    public void set(int s) {
        this.state = s;
    }
}

class FindState {
    private int state;
    public static final int IDLE = 1;
    public static final int DISCOVERING = 2;
    public static final int FIND_PEERS = 3;

    public FindState(int s) {
        this.state = s;
    }

    public int get() {
        return this.state;
    }
    public void set(int s) {
        this.state = s;
    }
}

public class P2pManager implements ConnectionInfoListener, DnsSdServiceResponseListener, DnsSdTxtRecordListener, GroupInfoListener, PeerListListener {
    public static final String TAG = "P2pManager";
    private WifiP2pManager manager;
    private P2pInterface busInterface = null;
    private boolean isEnabled = true;
    private Context context = null;
    private P2pReceiver receiver = null;

    private final IntentFilter intentFilter = new IntentFilter();
    private Channel channel;
    private WifiP2pDevice device = null;
    private WifiP2pDeviceList peerList = null;
    private WifiP2pDevice mGroupOwner = null;

    private WifiP2pConfig mPeerConfig = null;
    private PeerState mPeerState = new PeerState(PeerState.DISCONNECTED);
    private FindState mFindState = new FindState(FindState.IDLE);

    private Handler mHandler = null;
    private Runnable mPeriodicDiscovery = null;
    private Runnable mPeriodicFind = null;
    private Runnable mRequestConnectionInfo = null;

    private boolean mPendingConnect = false;
    private boolean isInitiator = false;
    private boolean isStandalone = true;

    private static final long periodicInterval = 40000;
    private static final long connectionTimeout = 150000;


    private ArrayList <String> mServiceRequestList;
    private ArrayList <String> mRequestedNames;

    private class FoundServiceInfo {
        public String name;
        public String prefix;
        public String guid;

        FoundServiceInfo(String name, String prefix, String guid) {
            this.name = name;
            this.prefix = prefix;
            this.guid = guid;
        }
    }

    private HashMap <String, ArrayList<FoundServiceInfo> > mDeviceServices;

    private class LocalServiceInfo {
        public String guid;
        public int timer;
        public String encName;

        LocalServiceInfo (String guid, int timer, String encName) {
            this.guid = guid;
            this.timer = timer;
            this.encName = encName;
        }
    }

    /**
     * Rip off Status.java that is autogenerated in alljoyn.jar
     * This is temporary (hipefully) workaround.
     * Please make sure that any P2P or Alljoyn error code that
     * need to be added and passed on to the daemon has its equivalent in
     * corresponding Status.xml
     */
    public enum Status {

        /** Success.. */
        OK(0x0),
        /** Generic failure.. */
        FAIL(0x1),
        /** Address is NULL or invalid. */
        INVALID_ADDRESS(0x14),
        /** An error occurred in a Wi-Fi Direct helper method call. */
        P2P(0x90dd),
        /** A timeout occurred in a Wi-Fi Direct helper method call. */
        P2P_TIMEOUT(0x90de),
        /** A required Wi-Fi Direct network connection does not exist. */
        P2P_NOT_CONNECTED(0x90df),
        /** Wi-Fi Direct is disabled on the device. */
        P2P_DISABLED(0x90e4),
        /** Wi-Fi Direct resources are in busy state. */
        P2P_BUSY(0x90e5);

        /** Error Code */
        private int errorCode;

        /** Constructor */
        private Status(int errorCode) {
            this.errorCode = errorCode;
        }

        /**
         * Gets the numeric error code.
         *
         * @return the numeric error code
         */
        public int getErrorCode() { return errorCode; }
    }

    private HashMap <String, LocalServiceInfo> mAdvertisedNames;

    private static final int MAX_ADVERTISE_TIMEOUT = 256000;

    private static final String ServiceSuffix = "._alljoyn._tcp.local.";

    public P2pManager(Context context, P2pInterface busInterface) {
        Log.d(TAG, "P2pManager construct: " + hashCode());

        this.context = context;
        this.busInterface = busInterface;

        intentFilter.addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_DISCOVERY_CHANGED_ACTION);
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);

        manager = (WifiP2pManager) context.getSystemService(Context.WIFI_P2P_SERVICE);
        channel = manager.initialize(context, context.getMainLooper(), null);
        manager.setDnsSdResponseListeners(channel, this, this);

        receiver = new P2pReceiver(this);

        mServiceRequestList = new ArrayList<String>();
        mRequestedNames = new ArrayList<String>();
        mAdvertisedNames = new HashMap<String, LocalServiceInfo>();
        mDeviceServices = new HashMap<String, ArrayList<FoundServiceInfo> >();

    }

    public void startup(boolean isStandalone) {

        this.isStandalone = isStandalone;
        Log.d(TAG, "Using preinstalled daemon " + !isStandalone);

        mHandler = new Handler();

        mPeriodicDiscovery = new Runnable() {
            public void run() {
                if (!isEnabled) {
                    return;
                }

                manager.discoverServices(channel,
                                         new WifiP2pManager.ActionListener()
                                         {
                                             public void onSuccess() {
                                                 Log.d(TAG, "Service discovery started");

                                                 synchronized (mFindState) {
                                                     if (mFindState.get() != FindState.DISCOVERING)
                                                         mFindState.set(FindState.DISCOVERING);
                                                 }
                                             }

                                             public void onFailure(int reasonCode) {
                                                 Log.e(TAG, "Service discovery failed: " + reasonCode);
                                                 if (reasonCode == WifiP2pManager.NO_SERVICE_REQUESTS)
                                                     startServiceSearch(false);
                                             }
                                         });
                mHandler.postDelayed(mPeriodicDiscovery, periodicInterval);
            }
        };

        mPeriodicFind = new Runnable() {
            public void run() {
                if (!isEnabled) {
                    return;
                }

                synchronized (mFindState) {
                    if (mFindState.get() == FindState.IDLE)
                        mFindState.set(FindState.FIND_PEERS);
                }

                manager.discoverPeers(channel, null);
                mHandler.postDelayed(mPeriodicFind, periodicInterval);
            }
        };

        final ConnectionInfoListener connInfoListener = this;

        mRequestConnectionInfo = new Runnable() {
            public void run() {
                if (!isEnabled) {
                    return;
                }

                Log.d(TAG, "Connection initiation timeout: Request connection info");
                manager.requestConnectionInfo(channel, connInfoListener);
            }
        };

        context.registerReceiver(receiver, intentFilter); // TODO: When to unregister?

    };

    private int mapError(int code) {
        switch (code) {
            case WifiP2pManager.P2P_UNSUPPORTED:
                return Status.P2P_DISABLED.getErrorCode();
            case WifiP2pManager.BUSY:
                return Status.P2P_BUSY.getErrorCode();
            default:
                return Status.P2P.getErrorCode();
        }
    }

    private void addServiceRequest(final String name) {

        Log.d(TAG, "Adding ServiceRequest for " + name);
        WifiP2pDnsSdServiceRequest serviceRequest = WifiP2pDnsSdServiceRequest.newInstance(name, "_alljoyn._tcp");
        mServiceRequestList.add(name);

        manager.addServiceRequest(channel, serviceRequest,
                                  new WifiP2pManager.ActionListener()
                                  {
                                      public void onSuccess() {
                                          Log.d(TAG, "addServiceRequest( " + name +  " ) success");
                                      }

                                      public void onFailure(int reasonCode) {
                                          Log.e(TAG, "addServiceRequest (" + name + ") failed: " + reasonCode);
                                          synchronized (mServiceRequestList) {
                                              mServiceRequestList.remove(name);
                                          }
                                      }
                                  });
    }

    private void startServiceSearch(boolean start) {

        synchronized (mServiceRequestList) {
            mServiceRequestList.clear();
        }

        synchronized (mRequestedNames) {
            if (mRequestedNames.isEmpty())
                return;
        }

        manager.clearServiceRequests(channel, null);

       WifiP2pDnsSdServiceRequest serviceRequest = WifiP2pDnsSdServiceRequest.newInstance("_alljoyn._tcp");

        manager.addServiceRequest(channel, serviceRequest,
                                  new WifiP2pManager.ActionListener()
                                  {
                                      public void onSuccess() {
                                          Log.d(TAG, "addServiceRequest (find all AJN) success");
                                      }

                                      public void onFailure(int reasonCode) {
                                          Log.e(TAG, "addServiceRequest (find all AJN) failed: " + reasonCode);
                                      }
                                  });

        if (start)
            doDiscoverServices(true);

    }

    private void addLocalServiceName(final String name, String guid, int timer) {

        Map<String, String> txt = new HashMap<String, String>();
        txt.put("GUID", guid);
        txt.put("TIMER", Integer.toString(timer));

        WifiP2pDnsSdServiceInfo serviceInfo = WifiP2pDnsSdServiceInfo.newInstance(name, "_alljoyn._tcp", txt);

        manager.addLocalService(channel, serviceInfo,
                                new ActionListener() {
                                    public void onSuccess() {
                                        Log.d(TAG, "AddLocalServiceName ( " + name + " ) success");
                                    }

                                    public void onFailure(int reasonCode) {
                                        Log.e(TAG, "AddLocalServiceName ( " + name + " ) fail. Reason : " + reasonCode);
                                    }
                                });
    }

    synchronized private void startAdvertisements() {

        if(mFindState.get() == FindState.IDLE)
                doFindPeers(true);

        if (mAdvertisedNames.isEmpty())
            return;

        for (String name: mAdvertisedNames.keySet()) {
            LocalServiceInfo info = mAdvertisedNames.get(name);
            addLocalServiceName(info.encName, info.guid, info.timer);
        }
    }

    public void shutdown() {
        mHandler.removeCallbacks(mPeriodicDiscovery);
        mHandler.removeCallbacks(mPeriodicFind);
        mHandler.removeCallbacks(mRequestConnectionInfo);
        if (receiver != null) {
            context.unregisterReceiver(receiver);
        }
        if (manager != null && channel != null) {
            manager.clearLocalServices(channel, null);
            manager.clearServiceRequests(channel, null);
        }
        receiver = null;
        channel = null;
        manager = null;
    }

    private void doDiscoverServices(boolean start) {
        mHandler.removeCallbacks(mPeriodicDiscovery);
            if (start) {
                synchronized (mFindState) {
                    if (mFindState.get() == FindState.FIND_PEERS)
                        mHandler.removeCallbacks(mPeriodicFind);
                    mFindState.set(FindState.DISCOVERING);
                }
                mPeriodicDiscovery.run();
            } else {
                synchronized (mFindState) {
                    mFindState.set(FindState.IDLE);
                }
            }
    }

    private void doFindPeers(boolean start) {
        mHandler.removeCallbacks(mPeriodicFind);
        if (start) {
            mPeriodicFind.run();
        }
    }

    private boolean isForeground() {
        // Treat the case of preinstalled daemon as always in foreground
        if (!isStandalone)
            return true;

        String name = context.getPackageName();

        Log.d(TAG, "Check if foreground app " + name);

        if (name == null) return false;

        ActivityManager activityMgr = (ActivityManager)context.getSystemService(Activity.ACTIVITY_SERVICE);
        List<RunningTaskInfo> tasks = activityMgr.getRunningTasks(1);

        if (tasks.size() == 0) {
            return false;
        }

        boolean res = name.equals(tasks.get(0).baseActivity.getPackageName());

        Log.d(TAG, "Foreground app " + tasks.get(0).baseActivity.getPackageName());

        return res;
    }

    private void initiateConnect() {
        mPendingConnect = false;
        if (mPeerConfig == null)
            return;
        Log.d(TAG, "Initiate connection to " + mPeerConfig.deviceAddress);
        mPeerState.set(PeerState.INITIATED);
        manager.connect(channel, mPeerConfig,
                        new ActionListener()
                        {
                            public void onSuccess() {
                                Log.d(TAG, "connect initiated");
                                if (mPeerState.get() == PeerState.INITIATED) {
                                    mPeerState.set(PeerState.CONNECTING);
                                }
                                Log.d(TAG, "suspend discovery");
                                mFindState.set(FindState.IDLE);

                                // Suspend discovery while in active connection.
                                mHandler.removeCallbacks(mPeriodicFind);
                                mHandler.removeCallbacks(mPeriodicDiscovery);
                                mHandler.postDelayed(mRequestConnectionInfo, connectionTimeout);
                            }

                            public void onFailure(int reasonCode) {
                                Log.e(TAG, "connect failed: " + reasonCode);
                                int handle = getHandle(mPeerConfig.deviceAddress);
                                mPeerState.set(PeerState.DISCONNECTED);
                                isInitiator = false;
                                mPeerConfig = null;
                                busInterface.OnLinkError(handle, -mapError(reasonCode));
                            }
                        });
    }

    /*package*/ void setEnabled(boolean enabled) {
        isEnabled = enabled;
        isInitiator = false;
        mPendingConnect = false;
        mPeerConfig = null;

        // Restart service search and advertisement
        if (!isEnabled) {
            Log.d(TAG, "P2P disabled");
            synchronized (mFindState) {
                mFindState.set(FindState.IDLE);
            }
            mPeerState.set(PeerState.DISCONNECTED);
            doFindPeers(false);
            doDiscoverServices(false);
        } else {
            Log.d(TAG, "P2P enabled");
            startServiceSearch(true);
            startAdvertisements();
        }

        String p2pInterfaceName = getInterfaceNameFromHandle(0);

        Log.d(TAG, "Interface Name " + p2pInterfaceName);

    }

    private int getHandle(String addr) {
        if (addr == null || addr.isEmpty()) {
            return 0;
        } else {
            return addr.hashCode() >>> 1;
        }
    }

    synchronized public void onGroupInfoAvailable(WifiP2pGroup group) {
        Log.d(TAG, "onGroupInfoAvaialble");

        synchronized (mPeerState) {
                if (mPeerState.get() != PeerState.CONNECTED)
                    return;
        }

        int handle = 0;
        String p2pInterfaceName = group.getInterface();

        Log.d (TAG, "interface " + p2pInterfaceName);

        mGroupOwner =group.getOwner();
        if (mGroupOwner != null)
            Log.d(TAG, "Group owner " + mGroupOwner.deviceAddress);

        if (mPeerConfig != null)
            handle = getHandle(mPeerConfig.deviceAddress);

        busInterface.OnLinkEstablished(handle, p2pInterfaceName);

    }

    public void onConnectionInfoAvailable(WifiP2pInfo info) {
        Log.d(TAG, "onConnectionInfoAvailable()");
        Log.d(TAG, "Group Formed: " + info.groupFormed);
        Log.d(TAG, "Is Group Owner: " + info.isGroupOwner);
        if (info.groupOwnerAddress != null) {
            Log.d(TAG, "Group Owner Address: " + info.groupOwnerAddress.getHostAddress());
        }

        mHandler.removeCallbacks(mRequestConnectionInfo);

        if (!info.groupFormed)
            mGroupOwner = null;

        synchronized (mPeerState) {
        Log.d(TAG, "peerState: " + mPeerState.get());

        switch (mPeerState.get()) {
        case PeerState.INITIATED:
        case PeerState.CONNECTING:
            if (info.groupFormed) {
                mPeerState.set(PeerState.CONNECTED);
                manager.requestGroupInfo(channel, this);
            } else {
                int handle = 0;
                if (mPeerConfig != null) {
                    handle = getHandle(mPeerConfig.deviceAddress);
                }

                //Timeout: cancel link establishment.
                manager.cancelConnect(channel,
                                      new ActionListener()
                                      {
                                          public void onSuccess() {
                                              Log.d(TAG, "cancel  connection creation");
                                          }

                                          public void onFailure(int reasonCode) {
                                              Log.e(TAG, "cancel connection creation" + reasonCode);
                                          }
                                      });

                mPeerState.set(PeerState.DISCONNECTED);
                mPeerConfig = null;
                if (handle != 0) {
                    busInterface.OnLinkError(handle, -Status.P2P_TIMEOUT.getErrorCode());
                }
            }
            break;

        case PeerState.CONNECTED:
            if (!info.groupFormed) {
                int handle = 0;
                if (mPeerConfig != null) {
                    handle = getHandle(mPeerConfig.deviceAddress);
                }
                mPeerState.set(PeerState.DISCONNECTED);
                mPeerConfig = null;
                busInterface.OnLinkLost(handle);
            }
            break;

        case PeerState.DISCONNECTING:
        case PeerState.DISCONNECTED:
            if (info.groupFormed) {
                Log.d(TAG, "Incoming connection");
                mPeerState.set(PeerState.CONNECTED);
                manager.requestGroupInfo(channel, this);
                isInitiator = false;

                // Restart advertisements/discovery.
                synchronized (mFindState) {
                    Log.d(TAG, "Find State: " + mFindState.get());
                    if (mFindState.get() == FindState.IDLE && !mPendingConnect) {
                        startServiceSearch(true);
                        startAdvertisements();
                    }
                }

                if (mPendingConnect && mPeerConfig != null) {
                    // Race condition? Some other WFD entity connected while we were
                    // in a process of initiating pending connect request.
                    // TODO: Inform the daemon that initiated connection failed
                    // We might want to wait till we get group info and check if
                    // the uninitiated connection happened to be to the desired device.
                    mPeerConfig = null;
                }

                mPendingConnect = false;

            } else {
                int handle = 0;
                if (!mPendingConnect && mPeerConfig != null) {
                    handle = getHandle(mPeerConfig.deviceAddress);
                    mPeerConfig = null;
                }
                mPeerState.set(PeerState.DISCONNECTED);
                busInterface.OnLinkLost(handle);

                if (mPendingConnect)
                    initiateConnect();
            }
            break;

        default:
            Log.d(TAG, "Bad peer state: " + mPeerState.get());
            break;
        }
        }

        if (!info.groupFormed) {
            //Restart suspended discovery/advertisement.
            synchronized (mFindState) {
                Log.d(TAG, "Disconnected state: restart advertising/discovery");
                if (mFindState.get() == FindState.IDLE) {
                    startServiceSearch(true);
                    startAdvertisements();
                }
            }
        }
    }

    private void deviceLost(WifiP2pDevice lostPeer) {
        Log.d(TAG, "deviceLost: " + lostPeer.deviceAddress);

        ArrayList<FoundServiceInfo> services = null;
        String address = lostPeer.deviceAddress;

        synchronized (mDeviceServices) {
            if (!mDeviceServices.isEmpty() && mDeviceServices.containsKey(address)) {
                services = mDeviceServices.get(address);
                mDeviceServices.remove(address);
            }
        }

        if (services == null || services.isEmpty())
            return;

        Iterator<FoundServiceInfo> itr = services.iterator();

        while (itr.hasNext()) {
            FoundServiceInfo serviceInfo = itr.next();
            busInterface.OnLostAdvertisedName(serviceInfo.name, serviceInfo.prefix, serviceInfo.guid, address);
        }
    }

    public void onPeersAvailable(WifiP2pDeviceList newPeers) {
        Log.d(TAG, "onPeersAvailable");
        if (peerList != null) {
            // Naive iteration for now.  Could be more efficient by using a hash.
            for (WifiP2pDevice oldPeer : peerList.getDeviceList()) {
                boolean found = false;
                for (WifiP2pDevice newPeer : newPeers.getDeviceList()) {
                    if (oldPeer.deviceAddress.equals(newPeer.deviceAddress)) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    deviceLost(oldPeer);
                }
            }
        }

        peerList = newPeers;
    }

    /** AllJoyn API Implementation **/

    synchronized public void onDnsSdServiceAvailable(final String instanceName, String registrationType, WifiP2pDevice srcDevice) {
        Log.d(TAG, "onDnsSdServiceAvailable(): " + instanceName + ", " + registrationType);

        // There must be an pending service discovery request.
        // If the request list is empty, the logic is broken somewhere.
        if (mRequestedNames.isEmpty()) {
            Log.d(TAG, "Ignore found remote service, since there are no pending service requests");
            return;
        }

        // Cycle through the outstanding service requests in order to figure out
        // if we are interested in this particular AllJoyn service.
        for (int i = 0; i < mRequestedNames.size(); i++) {
            String prefix = mRequestedNames.get(i);
            Log.d(TAG, "Outstanding request for " + prefix);
            if (instanceName.startsWith(prefix) || prefix.equals("*")) {
                Log.d(TAG, "Matches");

                //Check if request for service info already exists
                if (!mServiceRequestList.isEmpty() && mServiceRequestList.contains(instanceName))
                    continue;

                addServiceRequest(instanceName);
            }
            continue;
        }
    }

    synchronized private void updateDeviceServiceList(String name, String prefix, String guid, int timer, String address) {
        ArrayList<FoundServiceInfo> services;
        FoundServiceInfo serviceInfo;
        boolean isNew = true;

        // TODO Change key to GUID rather than device address.
        // If this service haven't been seen before, add it to a discovered services list.
        // If timer is set to zero, this means that the service is no longer available:
        // remove from the discovered services list.

        if (mDeviceServices.containsKey(address)) {
            services = mDeviceServices.get(address);
            mDeviceServices.remove(address);
        } else {
            if (timer != 0)
                services = new ArrayList<FoundServiceInfo>();
            else
                return;
        }

        Iterator<FoundServiceInfo> itr = services.iterator();

        while (itr.hasNext()) {
            serviceInfo = itr.next();
            if (name.equals(serviceInfo.name)) {
                isNew = false;
                if (timer == 0)
                    services.remove(serviceInfo);
                break;
            }
        }

        if (isNew) {
            serviceInfo = new FoundServiceInfo(name, prefix, guid);
            services.add(serviceInfo);
        }

        //Check
        Log.d(TAG, "Device " + address + " has " + services.size() + " services");
        mDeviceServices.put(address, services);
    }

    public void onDnsSdTxtRecordAvailable(String fullDomainName, Map<String, String> txtRecordMap, WifiP2pDevice srcDevice) {
        Log.d(TAG, "onDnsSdTxtRecordAvailable(): " + fullDomainName);

        String guid = txtRecordMap.get("GUID");
        int timer =  255;
        String namePrefix = null;
        String name = fullDomainName;
        String decName = fullDomainName;

        if (txtRecordMap.containsKey("TIMER"))
            timer = Integer.parseInt(txtRecordMap.get("TIMER"));

        //Strip off the service type (suffix) to get the service name.
        int index = name.lastIndexOf(ServiceSuffix);

        if (index > 0)
            name = fullDomainName.substring(0, index);

        decName = decodeName(name);

        // Cycle through the outstanding name discovery requests to find the name prefix.
        synchronized (mServiceRequestList) {
            for (int i = 0; i < mServiceRequestList.size(); i++) {
                String current = mServiceRequestList.get(i);
                Log.d(TAG, "Found outstanding request for " + current);

                if (fullDomainName.regionMatches(false, 0, current, 0, current.length())) {
                    namePrefix = decodeName(current);
                    break;
                }
            }
        }

        if (namePrefix == null) {
            Log.e(TAG, "No request found for" + name + ". Ignore");
            return;
        }

        Log.d(TAG, "Matched service info request prefix " + namePrefix);

        for (Map.Entry entry : txtRecordMap.entrySet())
            Log.d(TAG, (entry.getKey() + ", " + entry.getValue()));

        updateDeviceServiceList(decName, namePrefix, guid, timer, srcDevice.deviceAddress);

        if (timer != 0)
            busInterface.OnFoundAdvertisedName(decName, namePrefix, guid, /*timer,*/ srcDevice.deviceAddress);
        else {
            removeServiceRequestFromList(name);
            busInterface.OnLostAdvertisedName(decName, namePrefix, guid, srcDevice.deviceAddress);
        }
    }

    /**
     * Register interest in a well-known name prefix for the purpose of discovery.
     *
     * @param namePrefix    Well-known name prefix.
     *
     * @return
     * <ul>
     * <li>O if the request to start discovery of the provided namePrefix was successful.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public int findAdvertisedName(final String name) {
        Log.d(TAG, "findAdvertisedName(): " + name);
        final String namePrefix;

        if (!isEnabled) {
            Log.e(TAG, "findAdvertisedName(): P2P is OFF");
            return -Status.P2P_DISABLED.getErrorCode();
        }

        // Strip the wildcard if it is present.
        int index = name.lastIndexOf("*");

        if (index > 0)
            namePrefix = name.substring(0, index);
        else
            namePrefix = name;

        final String encName = encodeName(namePrefix);

        synchronized (mRequestedNames) {
            if (!mRequestedNames.isEmpty() && mRequestedNames.contains(encName)) {
                Log.d(TAG, "Request for " + namePrefix + " already added");
                if (mFindState.get() != FindState.DISCOVERING)
                    doDiscoverServices(true);
                return Status.OK.getErrorCode();
            }

            // Keep track of outstanding Service Discovery requests.
            mRequestedNames.add(encName);
        }

        // Find all AllJoyn services first.  If we are already in discovery mode, then
        // we are already finding all AllJoyn services.
        // Do not create a new request, just keep it on RequestedNames.
        if (mFindState.get() == FindState.DISCOVERING)
            return Status.OK.getErrorCode();

        WifiP2pDnsSdServiceRequest serviceRequest = WifiP2pDnsSdServiceRequest.newInstance("_alljoyn._tcp");

        manager.addServiceRequest(channel, serviceRequest,
                                  new WifiP2pManager.ActionListener()
                                  {
                                      public void onSuccess() {
                                          Log.d(TAG, "addServiceRequest (find all AJN) success");
                                          doDiscoverServices(true);
                                      }

                                      public void onFailure(int reasonCode) {
                                          Log.e(TAG, "addServiceRequest (find all AJN) failed: " + reasonCode);
                                          synchronized (mRequestedNames) {
                                              mRequestedNames.remove(encName);
                                          }
                                      }
                                  });

        return Status.OK.getErrorCode();
    }

    synchronized private void removeServiceRequestFromList(String namePrefix) {

       for (int i = 0; !mServiceRequestList.isEmpty() && i < mServiceRequestList.size(); i++) {
            final String name = mServiceRequestList.get(i);
            Log.d(TAG, name);

            if (!name.startsWith(namePrefix))
                continue;

            WifiP2pDnsSdServiceRequest serviceRequest = WifiP2pDnsSdServiceRequest.newInstance(name, "_alljoyn._tcp");
            Log.d(TAG, "Remove service name request for" + name);
            manager.removeServiceRequest(channel, serviceRequest,
                                         new WifiP2pManager.ActionListener()
                                         {
                                             public void onSuccess() {
                                                 Log.d(TAG, "removeServiceRequest success for" + name);
                                             }

                                             public void onFailure(int reasonCode) {
                                                 Log.e(TAG, "removeServiceRequest for " + name + " failed: " + reasonCode);
                                             }
                                         });

            mServiceRequestList.remove(name);
            break;
        }

    }

    /**
     * Cancel interest in a well-known name prefix for the purpose of discovery.
     *
     * @param namePrefix    Well-known name prefix.
     *
     * @return
     * <ul>
     * <li>O if the request to stop discovery of the provided namePrefix was successful.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    synchronized public int cancelFindAdvertisedName(String namePrefix) {
        Log.d(TAG, "cancelFindAdvertisedName( "  + namePrefix + " )");

        if (!isEnabled) {
            Log.e(TAG, "cancelFindAdvertisedName(): P2P is OFF");
            return -Status.P2P_DISABLED.getErrorCode();
        }

        // Strip the wildcard if it is present.
        int index = namePrefix.lastIndexOf("*");

        if (index > 0)
            namePrefix = namePrefix.substring(0, index);

        // Remove all pending service requests associated with this prefix
        removeServiceRequestFromList(namePrefix);

        if (!mRequestedNames.isEmpty() && mRequestedNames.contains(namePrefix))
            mRequestedNames.remove(namePrefix);

        // If no pending "Find Advertised Name" requests, cleanup and stop discovery.
        if (mRequestedNames.isEmpty()) {
            Log.d(TAG, "Clear all service requests");
            mServiceRequestList.clear();
            manager.clearServiceRequests(channel, null);
            doDiscoverServices(false);
       }

        return Status.OK.getErrorCode();
    }

    /*package*/ void discoveryChanged(int state) {

        synchronized (mFindState) {
            if (!isEnabled) {
                mFindState.set(FindState.IDLE);
                return;
            }

            if (state == WifiP2pManager.WIFI_P2P_DISCOVERY_STARTED) {
                Log.d(TAG, "discoveryChanged: STARTED");
            } else {
                Log.d(TAG, "discoveryChanged: STOPPED");
                mFindState.set(FindState.IDLE);
            }
        }
    }

    /*package*/ void peersChanged() {
        manager.requestPeers(channel, this);
    }

    private String encodeName(String name) {
        StringBuffer encName = new StringBuffer();

        for (int i = 0; i < name.length(); i++) {
            char c = name.charAt(i);
            if (Character.isUpperCase(c) || (c == '-')) {
                encName.append('-');
                encName.append(Character.toLowerCase(c));
            } else
                encName.append(c);
        }
        return encName.toString();
    }

   private String decodeName(String name) {
        StringBuffer decName = new StringBuffer();
        char c;
        char next = name.charAt(0);
        int i;

        for (i = 0; i < name.length() - 1;) {
            c = name.charAt(i);
            next = name.charAt(i + 1);
            if ((c == '-')) {
                if (next != '-')
                    decName.append(Character.toUpperCase(next));
                else
                    decName.append(next);
                i += 2;
            } else {
                decName.append(c);;
                i++;
            }
        }
        // handle the last character
        if (i == name.length() - 1)
            decName.append(next);

        return decName.toString();
    }

    /**
     * Advertise the existence of a well-known name to other (possibly
     * disconnected) AllJoyn daemons.
     *
     * @param name The well-known name to advertise.
     *
     * @param guid The globally unique identifier of the AllJoyn daemon
     *             associated with the well-known name.
     *
     * @return
     * <ul>
     * <li>O if the request to advertise the provided name was successful.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public int advertiseName(final String name, final String guid) {
        Log.d(TAG, "advertiseName(): " + name + ", " + guid + " _alljoyn._tcp");

        if (!isEnabled) {
            Log.e(TAG, "advertisedName(): P2P is OFF");
            return -Status.P2P_DISABLED.getErrorCode();
        }

        int timer = 255;
        String encName = encodeName(name);

        LocalServiceInfo info = new LocalServiceInfo(guid, timer, encName);

        synchronized (mAdvertisedNames) {
            mAdvertisedNames.put(name, info);
        }

        addLocalServiceName(encName, guid, timer);

        return Status.OK.getErrorCode();
    }

    /**
     * Stop advertising the existence of the provided well-known name.
     *
     * @param name The well-known name to advertise.
     *
     * @param guid The globally unique identifier of the AllJoyn daemon
     *             associated with the well-known name.
     *
     * @return
     * <ul>
     * <li>O if the request to stop advertising the provided name was successful.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public int cancelAdvertiseName(final String name, String guid) {
        Log.d(TAG, "cancelAdvertiseName(" + name + ")");

        if (!isEnabled) {
            Log.e(TAG, "cancelAdvertisedName(): P2P is OFF");
            return -Status.P2P_DISABLED.getErrorCode();
        }

        synchronized (mAdvertisedNames) {
            if (mAdvertisedNames.isEmpty())
                return Status.OK.getErrorCode();
            if (!mAdvertisedNames.containsKey(name))
                return Status.OK.getErrorCode();
        }

        Map<String, String> txt = new HashMap<String, String>();
        txt.put("GUID", guid);
        txt.put("TIMER", Integer.toString(0));

        final WifiP2pDnsSdServiceInfo serviceInfo = WifiP2pDnsSdServiceInfo.newInstance(name, "_alljoyn._tcp", txt);
        manager.addLocalService(channel, serviceInfo,
                                new ActionListener()
                                {
                                    public void onSuccess() {
                                        Log.d(TAG, "addLocalService (timer 0) success");
                                    }

                                    public void onFailure(int reasonCode) {
                                        Log.e(TAG, "addLocalService (timer 0) failed: " + reasonCode);
                                    }
                                });


        synchronized (mAdvertisedNames) {
            mAdvertisedNames.remove(name);
        }

        Runnable removeService = new Runnable() {
            public void run() {
                if (!isEnabled || manager == null || channel == null) {
                    return;
                }

                // Catch the case when a service advertisement has been added back in
                // after canceling.
                synchronized(mAdvertisedNames) {
                    if (mAdvertisedNames.containsKey(name))
                        return;
                }

                manager.removeLocalService(channel, serviceInfo,
                                           new ActionListener()
                                           {
                                               public void onSuccess() {
                                                   Log.d(TAG, "removeLocalService success");
                                               }

                                               public void onFailure(int reasonCode) {
                                                   Log.e(TAG, "removeLocalService failed: " + reasonCode);
                                               }
                                           });

            }
        };
        mHandler.postDelayed(removeService, MAX_ADVERTISE_TIMEOUT);

        return Status.OK.getErrorCode();
    }

    /**
     * Establish a Wi-Fi P2P connection to the device described by the
     * corresponding parameter.  The strength of desire to act as the group
     * owner is communicated via the groupOwnerIntent variable.
     *
     * It is assumed that if a call to this method returns success, there will
     * be a corresponding network interface available corresponding to the
     * resulting Wi-Fi Direct device, and the name of this interface will be
     * discoverable via a call to getInterfaceNameFromHandle().
     *
     * @param device The well-known name to advertise.
     *
     * @param groupOwnerIntent The strength of desire for the device to assume
     *                         the role of group owner.  Similar to GO Intent
     *                         as described in the Wi-Fi P2P spec, but a value
     *                         of fifteen here indicates an absolute requirement
     *                         to act as group owner, and a zero indicates an
     *                         absolute requirement to act as STA.
     *
     * @return
     * <ul>
     * <li>A positive handle describing the resulting connection if successful.</li>
     * <li>Other negative error status codes indicating a failure.</li>
     * </ul>
     */
    public int establishLink(String deviceAddress, int groupOwnerIntent) {
        Log.d(TAG, "establishLink(): " + deviceAddress);

        if (!isEnabled) {
            Log.e(TAG, "establishLink(): P2P is OFF");
            return -Status.P2P_DISABLED.getErrorCode();
        }

        if (deviceAddress == null ||  deviceAddress.isEmpty()) {
                Log.e(TAG, "Device address empty");
                return  -Status.INVALID_ADDRESS.getErrorCode();
        }

        if (mPeerState.get() != PeerState.DISCONNECTED && isInitiator) {
            Log.e(TAG, "Already connected or in progress: " + mPeerState.get());
            return -Status.P2P.getErrorCode();
        }

        isInitiator = true;

        // if no connection, initiate connect;
        // else if connection exists to the same device, request group info,
        // else if connection exists to a different device and the current app is in foreground, 
        // tear the existing connection down and initiate a new one.

        // Remove pending  calls
        mHandler.removeCallbacks(mRequestConnectionInfo);

        mPeerConfig = new WifiP2pConfig();
        mPeerConfig.deviceAddress = deviceAddress;
        mPeerConfig.groupOwnerIntent = groupOwnerIntent;
        mPeerConfig.wps.setup = WpsInfo.PBC;

        // Check if connection to the target device already exists
        // (not initiated by this entity)
        if (mPeerState.get() == PeerState.CONNECTED && mGroupOwner != null) {
            if (mGroupOwner.deviceAddress.equals(deviceAddress)) {
                Log.d(TAG, "Connection to " + deviceAddress + " already exists");
                manager.requestGroupInfo(channel, this);
            } else {
                if (isForeground()) {
                    mPendingConnect = true;
                    releaseLink(0);
                } else {
                    Log.d(TAG, "Cannot override existing connection to " + mGroupOwner.deviceAddress);
                    mPeerConfig  = null;
                    return -Status.P2P.getErrorCode();
                }
            }
        } else {
            Log.d(TAG, "Start connection");
            initiateConnect();
        }
        Log.d(TAG, "establishLink(): Returning: " + getHandle(mPeerConfig.deviceAddress));

        return getHandle(mPeerConfig.deviceAddress);
    }

    /**
     * Tear down a no longer-required Wi-Fi P2P connection on the device
     * described by the handle parameter.
     *
     * @param handle A device handle previously returned by establishLink().
     *
     * @return
     * <ul>
     * <li>0 indicates that the connection has been torn down.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public int releaseLink(int handle) {
        Log.d(TAG, "releaseLink()");

        if (!isEnabled) {
            Log.e(TAG, "releaseLink(): P2P is OFF");
            return -Status.P2P_DISABLED.getErrorCode();
        }

        switch (mPeerState.get()) {
        case PeerState.INITIATED:
        case PeerState.CONNECTING:
            mHandler.removeCallbacks(mRequestConnectionInfo);
            // Restart suspended discovery.
            synchronized (mFindState) {
                Log.d(TAG, "Find State: " + mFindState.get());
                if (mFindState.get() == FindState.IDLE) {
                    startServiceSearch(true);
                    startAdvertisements();
                }
            }

            mPeerState.set(PeerState.DISCONNECTING);
            manager.cancelConnect(channel,
                                  new ActionListener()
                                  {
                                      public void onSuccess() {
                                          Log.d(TAG, "cancelConnect initiated");
                                          mPeerState.set(PeerState.DISCONNECTED);
                                          mPeerConfig = null;
                                      }

                                      public void onFailure(int reasonCode) {
                                          Log.e(TAG, "cancelConnect failed: " + reasonCode);

                                          /* Most likely failure is if the connection completed already,
                                           * but in that case onConnectionInfoAvailable would have called
                                           * removeGroup() and marked the peer as disconnected.  No harm
                                           * in doing it again.
                                           */
                                          mPeerState.set(PeerState.DISCONNECTED);
                                          mPeerConfig = null;
                                      }
                                  });

            break;

        case PeerState.CONNECTED:
            // OnLinkLost will be sent via onConnectionInfoAvailable()
            mPeerState.set(PeerState.DISCONNECTING);
            manager.removeGroup(channel,
                                new ActionListener()
                                {
                                    public void onSuccess() {
                                        Log.d(TAG, "removeGroup initiated");
                                    }

                                    public void onFailure(int reasonCode) {
                                        Log.d(TAG, "removeGroup failed: " + reasonCode);
                                        int handle = 0;
                                        if (mPeerConfig != null) {
                                            handle = getHandle(mPeerConfig.deviceAddress);
                                        }
                                        mPeerState.set(PeerState.DISCONNECTED);
                                        mPeerConfig = null;
                                        if (handle != 0) {
                                            busInterface.OnLinkError(handle, -mapError(reasonCode));
                                        }
                                    }
                                });
            break;

        default:
            Log.e(TAG, "No link to release");
            break;
        }

        return Status.OK.getErrorCode();
    }

    /**
     * Discover which network interface corresponds to a handle value returned
     * from establishlink().
     *
     * Useful for determining which interfaces to add in order to listen for
     * AllJoyn multicast name service packets.
     *
     * @param handle A device handle previously returned by establishLink().
     *
     * @return
     * <ul>
     * <li>A String containing the interface name (cf. wlan0)
     * <li>null indicating an interface name is not available for the given handle.</li>
     * </ul>
     */
    public String getInterfaceNameFromHandle(int handle) {
        Log.d(TAG, "getInterfaceNameFromHandle()");

        try {
            Enumeration<NetworkInterface> list = NetworkInterface.getNetworkInterfaces();

            while (list != null && list.hasMoreElements()) {
                NetworkInterface nIface = list.nextElement();
                byte[] macAddr = nIface.getHardwareAddress();

                if (macAddr == null)
                    continue;

                StringBuilder macStr = new StringBuilder();
                for (int i = 0; i < macAddr.length; i++) {
                    macStr.append(String.format("%02x%s", macAddr[i], (i < macAddr.length - 1) ? ":" : ""));
                }

                if (this.device != null && this.device.deviceAddress.equals(macStr.toString())) {
                    Log.d(TAG, "getInterfaceNameFromHandle(): Returning: " + nIface.getDisplayName());
                    return nIface.getDisplayName();
                }
            }
        } catch (SocketException e) {
            Log.e(TAG, "Could not get network interface name");
            return null;
        }

        return null;
    }

    /*package*/ void setDevice(WifiP2pDevice device) {
        this.device = device;
    }

    @Override
    protected void finalize() throws Throwable {
        Log.d(TAG, "Finalize");
        try {
            shutdown();
        } finally {
            super.finalize();
        }
    }
}
