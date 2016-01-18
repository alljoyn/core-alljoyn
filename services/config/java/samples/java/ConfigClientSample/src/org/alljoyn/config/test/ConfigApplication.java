/******************************************************************************
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
 ******************************************************************************/

package org.alljoyn.config.test;

import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.bus.AboutListener;
import org.alljoyn.bus.AboutObjectDescription;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable.StringValue;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.alljoyn.DaemonInit;
import org.alljoyn.config.ConfigService;
import org.alljoyn.config.ConfigServiceImpl;
import org.alljoyn.config.client.ConfigClient;
import org.alljoyn.config.transport.ConfigTransport;
import org.alljoyn.services.android.security.AuthPasswordHandler;
import org.alljoyn.services.android.security.SrpAnonymousKeyListener;
import org.alljoyn.services.android.utils.AndroidLogger;
import org.alljoyn.services.common.utils.TransportUtil;

import android.app.AlertDialog;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.HandlerThread;
import android.util.Log;

/**
 * Handles all the AllJoyn devices that it received announcements from, and
 * enables the user to perform all the config service methods
 * 
 */
public class ConfigApplication extends Application implements AuthPasswordHandler, AboutListener {

    public static final String TAG = "ConfigClient";
    public static final String TAG_PASSWORD = "ConfigApplication_password";

    private static final String[] ANNOUNCEMENT_IFACES = new String[] { ConfigTransport.INTERFACE_NAME };

    private BusAttachment busAttachment;
    private HashMap<UUID, Device> devicesMap;
    private ConfigService configService;
    private ConfigClient configClient;
    private String daemonName = null;
    private BroadcastReceiver mainReceiver;

    // private SoftAPDetails m_curentPeer;
    private WifiManager wifiManager;

    /**
     * The daemon should advertise itself "quietly" (directly to the calling
     * port) This is to reply directly to a TC looking for a daemon
     */
    private static final String DAEMON_QUIET_PREFIX = "quiet@";

    static {
        try {
            System.loadLibrary("alljoyn_java");
        } catch (Exception e) {
            System.out.println("can't load library alljoyn_java");
        }
    }

    public class Device {

        /**
         * The device application id
         */
        UUID appId;

        /**
         * The device bus name
         */
        String busName;

        /**
         * The device friendly name
         */
        String deviceName;

        /**
         * The device port
         */
        short port;

        /**
         * The device supported interfaces
         */
        AboutObjectDescription[] objectDescriptions;

        /**
         * Represents the device about fields as we got them in the announcement
         */
        Map<String, Object> aboutData;

        /**
         * Represents the device config languages - the language in which the
         * device requests the config data.
         */
        String configLanguage = "";

        /**
         * Represents the device password from the client side.
         */
        char[] password = SrpAnonymousKeyListener.DEFAULT_PINCODE;

        /**
         * Create a device according to the following parameters.
         * 
         * @param appId
         *            The device application id
         * @param busName
         *            The device bus name
         * @param deviceFriendlyName
         *            The device friendly name
         * @param port
         *            The device port
         * @param interfaces
         *            The device supported interfaces
         * @param aboutMap
         *            The device about fields
         * @param password
         *            The device password
         */
        public Device(UUID appId, String busName, String deviceFriendlyName, short port, AboutObjectDescription[] objectDescriptions, Map<String, Object> aboutData) {

            this.appId = appId;
            this.busName = busName;
            this.deviceName = deviceFriendlyName;
            this.port = port;
            this.objectDescriptions = objectDescriptions;
            this.aboutData = aboutData;
            if (aboutData != null) {
                this.configLanguage = (String) aboutData.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
            }
        }

        @Override
        public String toString() {
            return String.format(getString(R.string.main_screen_row_text), deviceName, busName);
        }

        /**
         * @return Returns the device announcement
         */
        public String getAnnounce() {
            StringBuilder sb = new StringBuilder();
            // device name
            sb.append("BusName: " + busName + "\n\n");
            // port
            sb.append("Port: " + port + "\n\n");
            // about map
            if (aboutData == null) {
                sb.append(R.string.get_announce_about_map_null);
            } else {
                Set<String> set = aboutData.keySet();
                sb.append("About map:\n");
                Iterator<String> iterator = set.iterator();
                while (iterator.hasNext()) {
                    String current = iterator.next();
                    Object value = aboutData.get(current);
                    sb.append(current + " : " + value.toString() + "\n");
                }
                sb.append("\n");
            }
            // object descriptions
            sb.append("Bus Object Description:\n");
            for (int i = 0; i < objectDescriptions.length; i++) {
                sb.append(busObjectDescriptionString(objectDescriptions[i]));
                if (i != objectDescriptions.length - 1) {
                    sb.append("\n");
                }
            }
            return sb.toString();
        }

        private String busObjectDescriptionString(AboutObjectDescription bus) {
            String s = "";
            s += "path: " + bus.path + "\n";
            s += "interfaces: ";
            String[] tmp = bus.interfaces;
            for (int i = 0; i < tmp.length; i++) {
                s += tmp[i];
                if (i != tmp.length - 1) {
                    s += ",";
                } else {
                    s += "\n";
                }
            }
            return s;
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see android.app.Application#onCreate()
     */
    @Override
    public void onCreate() {
        super.onCreate();
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        devicesMap = new HashMap<UUID, Device>();

        // Receives intents from the wifi manager to display the current network
        // we are connected to
        mainReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {

                if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(intent.getAction())) {

                    NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);

                    String str = "";
                    if (networkInfo.getState().equals(State.CONNECTED)) {

                        WifiManager wifiMgr = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
                        str = wifiMgr.getConnectionInfo().getSSID();
                    } else {
                        str = networkInfo.getState().toString().toLowerCase(Locale.getDefault());
                    }
                    Intent networkIntent = new Intent(ACTION_CONNECTED_TO_NETWORK);
                    networkIntent.putExtra(EXTRA_NETWORK_SSID, str);
                    sendBroadcast(networkIntent);
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        registerReceiver(mainReceiver, filter);

    }

    public String getCurrentSSID() {
        return wifiManager.getConnectionInfo().getSSID();
    }

    /**
     * Sets the daemon realm name.
     * 
     * @param realmName
     *            The daemon realm name.
     */
    public void setRealmName(String daemonName) {
        this.daemonName = daemonName;
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
        unregisterReceiver(mainReceiver);
    }

    /**
     * Connect to the Alljoyn bus and register bus objects
     */
    public void doConnect() {

        /* All communication through AllJoyn begins with a BusAttachment. */
        DaemonInit.PrepareDaemon(getApplicationContext());
        String ss = getPackageName();
        busAttachment = new BusAttachment(ss, BusAttachment.RemoteMessage.Receive);

        // Connecting to the bus
        Status status = busAttachment.connect();
        Log.d(TAG, "bus.connect status: '" + status + "', BusName: '" + busAttachment.getUniqueName() + "'");

        String keyStoreFileName = null;
        try {
            busAttachment.registerAboutListener(this);
            busAttachment.whoImplements(ANNOUNCEMENT_IFACES);
            configService = ConfigServiceImpl.getInstance();
            configService.startConfigClient(busAttachment);

            keyStoreFileName = getApplicationContext().getFileStreamPath("alljoyn_keystore").getAbsolutePath();

        } catch (Exception e) {
            e.printStackTrace();
        }

        // request the name
        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        // request the name for the daemon and advertise it.
        daemonName = "org.alljoyn.BusNode.d" + busAttachment.getGlobalGUIDString();
        Status reqStatus = busAttachment.requestName(daemonName, flag);
        if (reqStatus == Status.OK) {
            // advertise the name with a quite prefix for TC to find it
            Status adStatus = busAttachment.advertiseName(DAEMON_QUIET_PREFIX + daemonName, SessionOpts.TRANSPORT_ANY);
            if (adStatus != Status.OK) {
                busAttachment.releaseName(daemonName);
                Log.w(TAG, "Failed to advertise daemon name " + daemonName);
            } else {
                Log.d(TAG, "Succefully advertised daemon name " + daemonName);
            }
        }

        // Set keyListener
        if (keyStoreFileName != null && keyStoreFileName.length() > 0) {

            final String[] authMechanisms = new String[] { "ALLJOYN_SRP_KEYX", "ALLJOYN_ECDHE_PSK" };

            SrpAnonymousKeyListener authListener = new SrpAnonymousKeyListener(ConfigApplication.this, new AndroidLogger(), authMechanisms);
            Status authStatus = busAttachment.registerAuthListener(authListener.getAuthMechanismsAsString(), authListener, keyStoreFileName);
            Log.d(TAG, "BusAttachment.registerAuthListener status = " + authStatus);
            if (authStatus != Status.OK) {
                Log.d(TAG, "Failed to register Auth listener status = " + authStatus.toString());
            }
        }
    }

    /**
     * Disconnect from Alljoyn bus and unregister bus objects.
     */
    public void doDisconnect() {

        /*
         * It is important to unregister the BusObject before disconnecting from
         * the bus. Failing to do so could result in a resource leak.
         */
        Log.d(TAG, "Disconnecting from AllJoyn");
        try {
            if (configService != null) {
                stopConfigSession();
                configService.stopConfigClient();
            }
            if (busAttachment != null) {
                busAttachment.cancelWhoImplements(ANNOUNCEMENT_IFACES);
                busAttachment.unregisterAboutListener(this);
                busAttachment.clearKeyStore();
                Log.i(TAG_PASSWORD, "Bus attachment clear key store");
                busAttachment.cancelAdvertiseName(DAEMON_QUIET_PREFIX + daemonName, SessionOpts.TRANSPORT_ANY);
                busAttachment.releaseName(daemonName);
                busAttachment.disconnect();
                busAttachment = null;
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Add an AllJoyn device to the application
    private void addDevice(UUID deviceId, String busName, short port, String deviceFriendlyName, String defaultLang, AboutObjectDescription[] objectDescriptions, Map<String, Object> aboutData) {

        Device oldDevice = devicesMap.get(deviceId);

        if (oldDevice != null) {// device already exist. update the fields that
                                // might have changed.

            oldDevice.busName = busName;
            oldDevice.aboutData = aboutData;
            oldDevice.deviceName = deviceFriendlyName;
            oldDevice.configLanguage = defaultLang;
            oldDevice.port = port;
            oldDevice.objectDescriptions = objectDescriptions;
        } else {
            // add the device to the map
            Device sad = new Device(deviceId, busName, deviceFriendlyName, port, objectDescriptions, aboutData);
            sad.configLanguage = defaultLang;
            devicesMap.put(deviceId, sad);
        }
        // notify the activity to come and get it
        Intent intent = new Intent(ACTION_DEVICE_FOUND);
        Bundle extras = new Bundle();
        extras.putSerializable(EXTRA_DEVICE_ID, deviceId);
        intent.putExtras(extras);
        sendBroadcast(intent);
    }

    // Remove an AllJoyn device from the application
    private void removeDevice(String busName) {
        Collection<Device> devices = devicesMap.values();
        Object[] array = devices.toArray();
        for (int i = 0; i < array.length; i++) {
            Device d = (Device) array[i];
            if (d.busName.equals(busName)) {
                devicesMap.remove(d.appId);
            }
        }
        Intent intent = new Intent(ACTION_DEVICE_LOST);
        Bundle extras = new Bundle();
        extras.putString(EXTRA_BUS_NAME, busName);
        intent.putExtras(extras);
        sendBroadcast(intent);
    }

    // Send an intent indicating an error has occurred
    private void updateTheUiAboutError(String error) {
        Intent intent = new Intent(ACTION_ERROR);
        intent.putExtra(EXTRA_ERROR, error);
        sendBroadcast(intent);
    }

    public HashMap<UUID, Device> getDeviceList() {
        return devicesMap;
    }

    // Retrieve a device by its device id
    public Device getDevice(UUID deviceId) {
        return devicesMap.get(deviceId);
    }

    // Display a dialog with the given errorMsg and displays it.
    public void showAlert(Context context, String errorMsg) {

        AlertDialog.Builder alert = new AlertDialog.Builder(context);
        alert.setTitle("Error");
        alert.setMessage(errorMsg);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
            }
        });
        alert.show();
    }

    /**
     * Create config client and create a session
     * 
     * @param peer
     *            The {@link Device} to connect
     * @throws Exception
     *             If failed to create the configClient or to establish session
     */
    public void startConfigSession(Device peer) throws Exception {

        if (peer == null || peer.busName == null) {

            throw new IllegalArgumentException("Received peer or peer.busName is undefined");
        }

        // If the configClient is already connected with the received peer, no
        // need to create an additional session
        if (configClient != null && configClient.getPeerName().equals(peer.busName)) {

            if (configClient.isConnected()) {

                Log.d(TAG, "ConfigClient is already in the session with the peer: '" + peer.busName + "', sid: '" + configClient.getSessionId() + "'");

                return;
            }
        }
        // Need to create a new configClient
        else {
            try {

                // Try to close the previous connection, if exists
                stopConfigSession();
                configClient = configService.createFeatureConfigClient(peer.busName, null, peer.port);
            } catch (Exception e) {

                String m = e.getMessage();
                Log.d(TAG, "startSession: Exception: " + m);
                e.printStackTrace();
                updateTheUiAboutError("startSession: Exception: " + m);
                throw e;
            }
        }

        // Instead of checking the returned status, we check the isConnected()
        // method.
        // It takes into account both the Status.Ok and the
        // Status.ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED
        Status status = configClient.connect();
        if (!configClient.isConnected()) {

            throw new Exception("Failed to connect to the peer: '" + peer.busName + "', Status: '" + status + "'");
        }

        Log.d(TAG, "The session with the peer: '" + peer.busName + "', has been established successfully, sid: '" + configClient.getSessionId() + "'");
    }

    /**
     * End the session with the config server
     */
    public void stopConfigSession() {

        if (configClient != null) {

            Log.d(TAG, "Closing connection with the peer: '" + configClient.getPeerName() + "'");
            configClient.disconnect();
        }

        configClient = null;
    }

    /**
     * Returns the config service version.
     * 
     * @return the config service version.
     */
    public Short getConfigVersion(UUID deviceId) {

        short configVersion = -1;
        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return configVersion;
        }

        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return configVersion;
        }

        try {

            startConfigSession(device);
            configVersion = configClient.getVersion();
        } catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("GET CONFIG VERSION: " + e.getMessage());
        }
        return Short.valueOf(configVersion);
    }

    /**
     * Return the config service fields.
     * 
     * @return the config service fields.
     */
    public void getConfig(String lang, UUID deviceId) {

        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return;
        }
        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return;
        }
        try {

            startConfigSession(device);
            Map<String, Object> configMap = configClient.getConfig(lang);
            Log.d(TAG, "GET_CONFIG: Config information was received");

            device.configLanguage = (String) configMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
            device.deviceName = (String) configMap.get(AboutKeys.ABOUT_DEVICE_NAME);
        } catch (Exception e) {
            Log.d(TAG, "GET_CONFIG: Exception: " + e.toString());
            e.printStackTrace();
            updateTheUiAboutError("GET_CONFIG: Exception: " + e.toString());
        }

    }

    /**
     * Factory reset the alljoyn device.
     */
    public void doFactoryReset(UUID deviceId) {

        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return;
        }

        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return;
        }
        try {

            startConfigSession(device);
            configClient.factoryReset();
        } catch (Exception e) {

            e.printStackTrace();
            updateTheUiAboutError("FACTORY RESET: Exception: " + e.toString());
        }
    }

    /**
     * Restart the alljoyn device
     */
    public void doRestart(UUID deviceId) {

        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return;
        }
        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return;
        }
        try {

            startConfigSession(device);
            configClient.restart();
        } catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("RESTART: Exception: " + e.toString());
        }
    }

    /**
     * Sets the AllJoyn device password.
     * 
     * @param newpassword
     *            the password to set.
     */
    public void setPasscodeOnServerSide(String newpassword, UUID deviceId) {

        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return;
        }
        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return;
        }
        try {

            startConfigSession(device);
            configClient.setPasscode(daemonName, newpassword.toCharArray());
            Log.i(TAG_PASSWORD, "calling setPasscode on the server side");
            device.password = newpassword.toCharArray();

            StringValue s = new StringValue();
            Status status = busAttachment.getPeerGUID(device.busName, s);
            if (status.equals(Status.OK)) {
                Log.i(TAG_PASSWORD, "Bus attachment clear its keys");
                busAttachment.clearKeys(s.value);
            } else {
                Log.i(TAG_PASSWORD, "getPeerGUID status is not OK");
            }
        } catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("SET PASSCODE: Exception: " + e.toString());
        }
    }

    /**
     * Sets the config fields appear in the given configMap.
     * 
     * @param configMap
     *            the map containing all the fields to be set.
     */
    public void setConfig(Map<String, Object> configMap, UUID deviceId, String lang) {

        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return;
        }

        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return;
        }

        try {

            startConfigSession(device);
            configClient.setConfig(configMap, lang);
        } catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("SET_CONFIG: Exception: " + e.toString());
        }
    }

    /**
     * Reset the config fields that appear in the given configMap to the default
     * value.
     * 
     * @param fieldsToRemove
     *            the map containing all the fields to be reset.
     */
    public void resetConfiguration(String[] fieldsToRemove, String lang, UUID deviceId) {

        if (deviceId == null) {
            updateTheUiAboutError(getString(R.string.no_peer_selected));
            return;
        }

        Device device = getDevice(deviceId);
        if (device == null) {
            updateTheUiAboutError(getString(R.string.no_device_available));
            return;
        }

        try {

            startConfigSession(device);
            configClient.ResetConfigurations(lang, fieldsToRemove);
        } catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("RESET CONFIGURATION: Exception: " + e.toString());
        }
    }

    // ***************************** PASSWORD *******************************

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.alljoyn.services.android.security.AuthPasswordHandler#getPassword
     * (java.lang.String)
     */
    @Override
    public char[] getPassword(String peerName) {

        Log.i(TAG_PASSWORD, "AuthPasswordHandler asked for password");
        if (peerName != null) {
            Collection<Device> devices = devicesMap.values();
            Iterator<Device> iterator = devices.iterator();
            for (; iterator.hasNext();) {
                Device device = iterator.next();
                if (device.busName.equals(peerName)) {
                    char[] password = device.password;
                    Log.i(TAG_PASSWORD, "Client password = " + String.valueOf(password) + " for peerName " + peerName + ", Name=" + device.deviceName);
                    if (password != null) {
                        return password;
                    }
                }
            }
        } else {
            Log.e(TAG_PASSWORD, "peerName == null, retrn null");
        }
        return null;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.alljoyn.services.android.security.AuthPasswordHandler#completed(java
     * .lang.String, java.lang.String, boolean)
     */
    @Override
    public void completed(String mechanism, String authPeer, boolean authenticated) {
        if (!authenticated) {
            Log.i(TAG_PASSWORD, " ** " + authPeer + " failed to authenticate");
            Intent intent = new Intent(ACTION_PASSWORD_IS_INCORRECT);
            sendBroadcast(intent);
        } else {
            Log.i(TAG_PASSWORD, " ** " + authPeer + " successfully authenticated");
        }
    }

    public static final String ACTION_PREFIX = ConfigApplication.class.getPackage().getName() + "_";

    /**
     * An action indicating an error has occurred. An extra string called
     * Keys.Extras.EXTRA_ERROR provides the error msg.
     */
    public static final String ACTION_ERROR = ACTION_PREFIX + "ACTION_ERROR";

    /**
     * An action indicating a new device was found. An extra string called
     * Keys.Extras.EXTRA_DEVICE_ID provides the device id.
     */
    public static final String ACTION_DEVICE_FOUND = ACTION_PREFIX + "ACTION_DEVICE_FOUND";

    /**
     * An action indicating a device was lost. An extra string called
     * Keys.Extras.EXTRA_BUS_NAME provides the device bus name.
     */
    public static final String ACTION_DEVICE_LOST = ACTION_PREFIX + "ACTION_DEVICE_LOST";

    /**
     * An action indicating we are connected to a network. An extra string
     * called Keys.Extras.EXTRA_NETWORK_SSID provides the network ssid. Id we
     * are connecting/connected to a network, the EXTRA_NETWORK_SSID indicates
     * that.
     */
    public static final String ACTION_CONNECTED_TO_NETWORK = ACTION_PREFIX + "ACTION_CONNECTED_TO_NETWORK";

    /**
     * An action indicating the password we gave does not match the device
     * password.
     */
    public static final String ACTION_PASSWORD_IS_INCORRECT = ACTION_PREFIX + "ACTION_PASSWORD_IS_INCORRECT";

    /**
     * The lookup key for a String object indicating the error message that
     * occurred. Retrieve with getStringExtra(String).
     */
    public static final String EXTRA_ERROR = ACTION_PREFIX + "extra_error";

    /**
     * The lookup key for a String object indicating a device id. Retrieve with
     * intent.getExtras().getString(String). To get the device itself call
     * getApplication().getDevice(String);
     */
    public static final String EXTRA_DEVICE_ID = ACTION_PREFIX + "extra_deviceId";

    /**
     * The lookup key for a String object indicating a device bus name. Retrieve
     * with getExtras().getString(String).
     */
    public static final String EXTRA_BUS_NAME = ACTION_PREFIX + "extra_BusName";

    /**
     * The lookup key for a String object indicating the network ssid to which
     * we are connected. Retrieve with intent.getStringExtra(String).
     */
    public static final String EXTRA_NETWORK_SSID = ACTION_PREFIX + "extra_network_ssid";

    @Override
    public void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData) {
        Map<String, Object> newMap = new HashMap<String, Object>();
        try {

            newMap = TransportUtil.fromVariantMap(aboutData);
            UUID deviceId = (UUID) newMap.get(AboutKeys.ABOUT_APP_ID);
            String deviceFriendlyName = (String) newMap.get(AboutKeys.ABOUT_DEVICE_NAME);
            String defaultLanguage = (String) newMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);

            Log.d(TAG, "onAnnouncement received: with parameters: busName:" + busName + ", port:" + port + ", deviceid" + deviceId + ", deviceName:" + deviceFriendlyName);
            addDevice(deviceId, busName, port, deviceFriendlyName, defaultLanguage, objectDescriptions, newMap);

        } catch (BusException e) {
            e.printStackTrace();
        }
    }
}
