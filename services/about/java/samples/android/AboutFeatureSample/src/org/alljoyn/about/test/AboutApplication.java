/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.about.test;

import java.util.Collection;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.about.AboutService;
import org.alljoyn.about.AboutServiceImpl;
import org.alljoyn.about.client.AboutClient;
import org.alljoyn.about.icon.AboutIconClient;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.PasswordManager;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.alljoyn.DaemonInit;
import org.alljoyn.services.common.AnnouncementHandler;
import org.alljoyn.services.common.BusObjectDescription;
import org.alljoyn.services.common.utils.GenericLogger;
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
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.HandlerThread;
import android.util.Log;
import android.widget.Toast;


/**
 * The AboutApplication class handles all the AllJoyn devices the application is announced on,
 * and enable the user to perform all the AllJoyn services methods. (get the about and the icon data)
 *
 */
public class AboutApplication extends Application
{

    public static final String TAG = "AboutClient";

    private BusAttachment m_Bus;
    private HashMap<String, SoftAPDetails> m_devicesMap;
    private AboutService m_aboutService;
    private AboutClient m_aboutClient;
    private AboutIconClient m_aboutIconClient;
    private String m_realmName;
    private BroadcastReceiver m_receiver;
    private WifiManager m_wifiManager;

    /**
     * The daemon should advertise itself "quietly" (directly to the calling port)
     * This is to reply directly to a TC looking for a daemon
     */
    private static final String DAEMON_QUIET_PREFIX= "quiet@";

    private final GenericLogger m_logger = new GenericLogger() {
        @Override
        public void debug(String TAG, String msg) {
            Log.d(TAG, msg);
        }

        @Override
        public void info(String TAG, String msg) {
            //To change body of implemented methods use File | Settings | File Templates.
            Log.i(TAG, msg);
        }

        @Override
        public void warn(String TAG, String msg) {
            //To change body of implemented methods use File | Settings | File Templates.
            Log.w(TAG, msg);
        }

        @Override
        public void error(String TAG, String msg) {
            //To change body of implemented methods use File | Settings | File Templates.
            Log.e(TAG, msg);
        }

        @Override
        public void fatal(String TAG, String msg) {
            //To change body of implemented methods use File | Settings | File Templates.
            Log.wtf(TAG, msg);
        }
    };

    static {
        try {
            System.loadLibrary("alljoyn_java");
        } catch (Exception e) {
            System.out.println("can't load library alljoyn_java");
        }
    }

    //======================================================================
    /* (non-Javadoc)
     * @see android.app.Application#onCreate()
     */
    @Override
    public void onCreate() {

        super.onCreate();
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        m_wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        m_devicesMap = new HashMap<String, SoftAPDetails>();

        //Receiver
        m_receiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {

                if(WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(intent.getAction())){

                    NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);

                    String str = "";
                    if(networkInfo.getState().equals(State.CONNECTED)){
                        WifiInfo wifiInfo = intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
                        str = wifiInfo.getSSID();
                    }
                    else{
                        str = networkInfo.getState().toString().toLowerCase(Locale.getDefault());
                    }
                    Intent networkIntent = new Intent(Keys.Actions.ACTION_CONNECTED_TO_NETWORK);
                    networkIntent.putExtra(Keys.Extras.EXTRA_NETWORK_SSID, str);
                    sendBroadcast(networkIntent);
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        registerReceiver(m_receiver, filter);

    }
    //======================================================================
    public String getCurrentSSID(){
        return m_wifiManager.getConnectionInfo().getSSID();
    }
    //======================================================================
    /**
     * @param msg Given a msg, create and display a toast on the screen.
     */
    public void makeToast(String msg){
        Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
    }
    //======================================================================
    /**
     * Sets the daemon realm name.
     * @param realmName The daemon realm name.
     */
    public void setRealmName(String realmName){
        m_realmName = realmName;
    }
    //======================================================================
    /* (non-Javadoc)
     * @see android.app.Application#onTerminate()
     */
    @Override
    public void onTerminate() {
        super.onTerminate();
        unregisterReceiver(m_receiver);
    }
    //======================================================================

    /**
     * Connect to the Alljoyn bus and register bus objects.
     */
    public void doConnect(){

        /*All communication through AllJoyn begins with a BusAttachment. */
        boolean b = DaemonInit.PrepareDaemon(getApplicationContext());
        System.out.println(b);
        String ss = getPackageName();
        m_Bus = new BusAttachment(ss , BusAttachment.RemoteMessage.Receive);

        //setting the password for the daemon to allow thin clients to connect
        Log.d(TAG, "Setting daemon password");
        Status pasStatus = PasswordManager.setCredentials("ALLJOYN_PIN_KEYX", "000000");

        if ( pasStatus != Status.OK ) {
            Log.e(TAG, "Failed to set password for daemon, Error: " + pasStatus);
        }

        Status status = m_Bus.connect();
        Log.d(TAG, "bus.connect status: "+status);

        // Pump up the daemon debug level
        m_Bus.setDaemonDebug("ALL", 7);
        m_Bus.setLogLevels("ALL=7");
        m_Bus.useOSLogging(true);

        try
        {
            AnnouncementHandler receiver = new AnnouncementHandler(){

                @Override
                public void onAnnouncement(String busName, short port, BusObjectDescription[] interfaces, Map<String, Variant> aboutMap) {

                    Map<String, Object> newMap = new HashMap<String, Object>();
                    try {
                        newMap = TransportUtil.fromVariantMap(aboutMap);
                        String deviceId = (String) (newMap.get(AboutKeys.ABOUT_APP_ID).toString());
                        String deviceFriendlyName = (String) newMap.get(AboutKeys.ABOUT_DEVICE_NAME);
                        Log.d(TAG, "onAnnouncement received: with parameters: busName:"+busName+", port:"+port+", deviceid"+deviceId+", deviceName:"+deviceFriendlyName);
                        addDevice(deviceId, busName, port, deviceFriendlyName, interfaces, newMap);

                    } catch (BusException e) {
                        e.printStackTrace();
                    }
                }

                @Override
                public void onDeviceLost(String serviceName)
                {
                    //remove the device from the spinner
                    Log.d(TAG, "onDeviceLost received: device with busName: "+serviceName+" was lost");
                    removeDevice(serviceName);
                }
            };

            m_aboutService = AboutServiceImpl.getInstance();
            m_aboutService.setLogger(m_logger);
            m_aboutService.startAboutClient(m_Bus);
            m_aboutService.addAnnouncementHandler(receiver, null);

        } catch (Exception e){
            e.printStackTrace();
        }

        //request the name
        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        String DAEMON_NAME = m_realmName;//"org.alljoyn.BusNode.Dashboard2";
        Status reqStatus = m_Bus.requestName(DAEMON_NAME, flag);
        if (reqStatus == Status.OK) {
            //advertise the name
            //advertise the name with a quite prefix for TC to find it
            Status adStatus = m_Bus.advertiseName(DAEMON_QUIET_PREFIX + DAEMON_NAME, SessionOpts.TRANSPORT_ANY);
            if (adStatus != Status.OK){
                m_Bus.releaseName(DAEMON_NAME);
                Log.w(TAG, "failed to advertise daemon name " + DAEMON_NAME);
            }
            else{
                Log.d(TAG, "Succefully advertised daemon name " + DAEMON_NAME);
            }
        }
    }

    //======================================================================
    /**
     * Disconnect from Alljoyn bus and unregister bus objects.
     */
    public void doDisconnect(){
        /* It is important to unregister the BusObject before disconnecting from the bus.
         * Failing to do so could result in a resource leak.
         */
        try {
            if(m_aboutService != null)
                m_aboutService.stopAboutClient();
            if(m_Bus != null){
                m_Bus.cancelAdvertiseName(DAEMON_QUIET_PREFIX + m_realmName, SessionOpts.TRANSPORT_ANY);
                m_Bus.releaseName(m_realmName);
                m_Bus.disconnect();
                m_Bus = null;
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    //======================================================================

    // Add an AllJoym device to the application.
    private void addDevice(String deviceId, String busName, short port, String deviceFriendlyName, BusObjectDescription[] interfaces, Map<String, Object> aboutMap)
    {
        SoftAPDetails oldDevice = m_devicesMap.get(deviceId);

        if(oldDevice != null){//device already exist. update the fields that might have changed.
            oldDevice.busName = busName;
            oldDevice.aboutMap = aboutMap;
            oldDevice.deviceFriendlyName = deviceFriendlyName;
            oldDevice.port = port;
            oldDevice.interfaces = interfaces;
            oldDevice.updateSupportedServices();

        }
        else{
            //add the device to the map
            SoftAPDetails sad = new SoftAPDetails(deviceId, busName, deviceFriendlyName, port, interfaces, aboutMap);
            m_devicesMap.put(deviceId, sad);
        }
        //notify the activity to come and get it
        Intent intent = new Intent(Keys.Actions.ACTION_DEVICE_FOUND);
        Bundle extras = new Bundle();
        extras.putString(Keys.Extras.EXTRA_DEVICE_ID, deviceId);
        intent.putExtras(extras);
        sendBroadcast(intent);
    }
    //======================================================================

    // Remove an AllJoyn device from the application.
    private void removeDevice(String busName){

        Collection<SoftAPDetails> devices = m_devicesMap.values();
        Object[] array = devices.toArray();
        for(int i = 0; i < array.length; i++){
            SoftAPDetails d = (SoftAPDetails) array[i];
            if(d.busName.equals(busName)){
                m_devicesMap.remove(d.appId);
            }
        }
        Intent intent = new Intent(Keys.Actions.ACTION_DEVICE_LOST);
        Bundle extras = new Bundle();
        extras.putString(Keys.Extras.EXTRA_BUS_NAME, busName);
        intent.putExtras(extras);
        sendBroadcast(intent);
    }
    //======================================================================

    // Send an intent indicating an error has occurred.
    private void updateTheUiAboutError(String error){

        Intent intent = new Intent(Keys.Actions.ACTION_ERROR);
        intent.putExtra(Keys.Extras.EXTRA_ERROR, error);
        sendBroadcast(intent);
    }
    //======================================================================

    // Retrieve a device by its device is.
    public SoftAPDetails getDevice(String deviceId){
        return m_devicesMap.get(deviceId);
    }
    //======================================================================

    // Display a dialog with the given errorMsg and displays it.
    public void showAlert(Context context, String errorMsg) {

        AlertDialog.Builder alert = new AlertDialog.Builder(context);
        alert.setTitle("Error");
        alert.setMessage(errorMsg);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
            }
        });
        alert.show();
    }

    //======================================================================
    public void startAboutSession(SoftAPDetails device){
        try {
            m_aboutClient =  m_aboutService.createAboutClient(device.busName, null, device.port);
        }
        catch (Exception e) {
            String m = e.getMessage();
            Log.d(TAG, "startSession: Exception: "+m);
            e.printStackTrace();
            updateTheUiAboutError("startSession: Exception: "+m);
        }
        if (m_aboutClient != null)
            m_aboutClient.connect();
    }

    //======================================================================
    public void stopAboutSession(){
        if (m_aboutClient != null){
            m_aboutClient.disconnect();
        }
    }
    //======================================================================
    public void startIconSession(SoftAPDetails device){
        try {
            m_aboutIconClient =  m_aboutService.createAboutIconClient(device.busName, null, device.port);
        }
        catch (Exception e) {
            String m = e.getMessage();
            Log.d(TAG, "startSession: Exception: "+m);
            e.printStackTrace();
            updateTheUiAboutError("startSession: Exception: "+m);
        }
        if (m_aboutIconClient != null)
            m_aboutIconClient.connect();
    }

    //======================================================================
    public void stopIconSession(){
        if (m_aboutIconClient != null){
            m_aboutIconClient.disconnect();
        }
    }
    //======================================================================
    /**
     * Returns the about service version.
     * @return the about service version.
     */
    public Short getAboutVersion() {

        short aboutVersion = -1;
        try {
            if (m_aboutClient != null){
                if(!m_aboutClient.isConnected()){
                    m_aboutClient.connect();
                }
                aboutVersion = m_aboutClient.getVersion();
            }
        }catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("GET ABOUT VERSION: "+e.getMessage());
        }
        return Short.valueOf(aboutVersion);
    }
    //======================================================================
    /**
     * Returns the about service fields.
     * @return the about service fields.
     */
    public Map<String, Object> getAbout(String lang) {

        Map<String, Object> aboutMap = null;
        try {
            if (m_aboutClient != null){
                if(!m_aboutClient.isConnected()){
                    m_aboutClient.connect();
                }
                aboutMap = m_aboutClient.getAbout(lang);
            }
            Log.d(TAG, "GET_ABOUT: 'About' information has returned");

        } catch (Exception e){
            String m = e.getMessage();
            Log.d(TAG, "GET_ABOUT: Exception: "+m);
            e.printStackTrace();
            updateTheUiAboutError("GET_ABOUT: Exception: "+m);
        }
        return aboutMap;
    }
    //======================================================================
    /**
     * Return the about service bus object description, meaning the supported interfaces.
     * @return the about service bus object description, meaning the supported interfaces.
     */
    public BusObjectDescription[] getBusObjectDescription(){

        BusObjectDescription[] busObjectDescription = null;
        try {

            busObjectDescription = m_aboutClient.getBusObjectDescriptions();
        } catch (Exception e){
            e.printStackTrace();
            updateTheUiAboutError("GET BUS OBJECT DESCRIPTION: "+e.getMessage());
        }
        return busObjectDescription;
    }

    //***************************** ICON ***********************************
    //======================================================================
    /**
     * Return the icon service fields.
     * @return the icon service fields.
     */
    public Short getIconVersion() {

        short iconVersion = -1;
        try {
            if (m_aboutIconClient != null){
                if(!m_aboutIconClient.isConnected()){
                    m_aboutIconClient.connect();
                }
                iconVersion = m_aboutIconClient.getVersion();
            }
        }catch (Exception e) {
            e.printStackTrace();
            updateTheUiAboutError("GET ICON VERSION: Exception: "+e.toString());
        }
        return Short.valueOf(iconVersion);
    }
    //======================================================================
    /**
     * Return the icon details: size, url, mime type and the byte array of the icon content.
     * @return the icon details: size, url, mime type and the byte array of the icon content.
     */
    public IconDetails getIconDetails(){

        String url = "";
        int size = 0;
        byte[] content = new byte[]{};
        String mimeType = "";

        try {
            if (m_aboutIconClient != null){
                if(!m_aboutIconClient.isConnected()){
                    m_aboutIconClient.connect();
                }
                url = m_aboutIconClient.GetUrl();
                size = m_aboutIconClient.getSize();
                content = m_aboutIconClient.GetContent();
                mimeType = m_aboutIconClient.getMimeType();
                Log.d(TAG, "GET_ABOUT_ICON: 'Icon' information has returned");
            }
        } catch (Exception e)
        {
            Log.d(TAG, "GET_ABOUT_ICON: Exception: "+e.toString());
            updateTheUiAboutError("GET ICON DETAILS: Exception: "+e.getMessage());
            e.printStackTrace();
        }
        return new IconDetails(url, size, content, mimeType);
    }
    //======================================================================
    class AboutProperty
    {
        String key;
        Object value;
        public AboutProperty(String key, Object value)
        {
            this.key = key;
            this.value = value;
        }
    }
    //======================================================================
    class IconDetails
    {
        String url;
        int size;
        byte[] content;
        String mimeType;
        public IconDetails(String url, int size, byte[] content, String mimeType)
        {
            this.url = url;
            this.size = size;
            this.content = content;
            this.mimeType = mimeType;
        }
    }
}

