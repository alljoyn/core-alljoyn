/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.alljoyn.config.samples.configservice;

import java.lang.Override;
import java.util.Arrays;
import java.util.Map;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.AboutObj;
import org.alljoyn.config.ConfigServiceImpl;
import org.alljoyn.config.server.ConfigChangeListener;
import org.alljoyn.config.server.FactoryResetHandler;
import org.alljoyn.config.server.PassphraseChangedListener;
import org.alljoyn.config.server.RestartHandler;
import org.alljoyn.config.server.SetPasswordHandler;
import org.alljoyn.config.samples.configservice.security.AuthPasswordHandler;
import org.alljoyn.config.samples.configservice.security.SrpAnonymousKeyListener;
import org.alljoyn.config.samples.configservice.storage.AboutDataImpl;
import org.alljoyn.config.samples.configservice.storage.AboutDataStoreImpl;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

/**
 * Android service that supports the AllJoyn Config service.
 */
public class ConfigService extends Service implements AuthPasswordHandler, SetPasswordHandler {

    public static final String TAG = ConfigService.class.getSimpleName();

    // Strings indicating a notification event to broadcast, and its message field within the broadcast bundle.
    public static final String BROADCAST_NOTIFICATION = "org.alljoyn.config.sample.BROADCAST_NOTIFICATION";
    public static final String BROADCAST_NOTIFICATION_MESSAGE = "message";

    // The session port to be bound
    private static final short SESSION_PORT = 1080;

    // Reference to the AllJoyn bus
    BusAttachment mBus;

    // Reference to AboutConfigService
    private AsyncHandler mAsyncHandler;

    // the about data used by About service
    private AboutDataImpl mAboutData;

    // the property store of the Config service
    private AboutDataStoreImpl mAboutDataStore;

    public String mKeyStoreFileName;

    // listener for bus security requests. Config services are secure
    public SrpAnonymousKeyListener mAuthListener;

    // the password for secured sessions
    private char[] mMyPass;

    // load the native alljoyn_java library.
    static {
        System.loadLibrary("alljoyn_java");
    }

    public ConfigService() {
    }

    /* (non-Javadoc)
     * @see android.app.Service#onBind(android.content.Intent)
     */
    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not implemented");
    }

    /* (non-Javadoc)
     * @see android.app.Service#onCreate()
     */
    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");
        super.onCreate();

        // create a handler
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mAsyncHandler = new AsyncHandler(busThread.getLooper());

        // initialize the state machine
        mAsyncHandler.sendEmptyMessage(AsyncHandler.CONNECT);
    }

    /* (non-Javadoc)
     * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Continue running until explicitly being stopped.
        return START_STICKY;
    }

    /* (non-Javadoc)
     * @see android.app.Service#onDestroy()
     */
    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");

        // Disconnect to prevent any resource leaks
        mAsyncHandler.shutdown();
        mAsyncHandler.getLooper().quit();

        super.onDestroy();
    }

    /**
     * Handles all callbacks from UI and AllJoyn
     */
    class AsyncHandler extends Handler implements RestartHandler, ConfigChangeListener, FactoryResetHandler, PassphraseChangedListener {
        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;

        public AsyncHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

            // connect to the bus and start our service
            case CONNECT: {
                connect();
                break;
            }

            // release all resources acquired in connect
            case DISCONNECT: {
                disconnect();
                break;
            }

            default:
                break;
            }
        }

        // ------------------------ Connect --------------------------------

        /**
         * Connect to the bus and start our services.
         */
        private void connect() {
            mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);
            Status status = mBus.connect();
            Log.d(TAG, "connect: " + status.name());

            // this is not a TCP/IP port. It's AllJoyn specific
            bindSessionPort(SESSION_PORT);

            // initialize the about data
            mAboutData = new AboutDataImpl(getApplicationContext());
            AboutObj aboutObj = new AboutObj(mBus);

            // initialize ConfigService (exposes the service's configurable properties to clients)
            try {
                mAboutDataStore = new AboutDataStoreImpl(getApplicationContext());
                ConfigServiceImpl.getInstance().startConfigServer(mAboutDataStore, this, this, this, this, mBus);
                ConfigServiceImpl.getInstance().setSetPasswordHandler(ConfigService.this);
            } catch (Exception e) {
                e.printStackTrace();
            }

            registerAuthenticationListener();

            Log.d(TAG, "Announcing service");
            aboutObj.announce(SESSION_PORT, mAboutData);
        }

        // ------------------------ Disconnect --------------------------------

        /**
         * Release all resources acquired in connect.
         */
        private void disconnect() {
            Log.d(TAG, "disconnect");
            try {
                ConfigServiceImpl.getInstance().stopConfigServer();
            } catch (Exception e) {
                Log.e(TAG, "disconnect failed: " + e.getMessage());
            }
            mBus.disconnect();
            mBus.release();
            mBus = null;
        }

        // ------------------------ Shutdown --------------------------------

        private void shutdown() {
            try {
                ConfigServiceImpl.getInstance().stopConfigServer();
                mBus.disconnect();
                mBus.release();
            } catch (Exception e) {
                Log.e(TAG, "Shutdown failed to stop service, Error: " + e.getMessage());
            }
        }

        // ------------------------ ConfigService callbacks --------------------------------

        /* (non-Javadoc)
         * @see org.alljoyn.config.server.PassphraseChangedListener#onPassphraseChanged(byte[])
         */
        @Override
        public void onPassphraseChanged(byte[] passphrase) {
            sendBroadcastNotification("Passphrase Changed: " + new String(passphrase));
        }

        /* (non-Javadoc)
         * @see org.alljoyn.config.server.FactoryResetHandler#doFactoryReset()
         */
        @Override
        public void doFactoryReset() {
            mMyPass = SrpAnonymousKeyListener.DEFAULT_PINCODE; // reset the password to default
            mBus.clearKeyStore();
            sendBroadcastNotification("Factory Reset");
        }

        /* (non-Javadoc)
         * @see org.alljoyn.config.server.ConfigChangeListener#onConfigChanged(java.util.Map, java.lang.String)
         */
        @Override
        public void onConfigChanged(Map<String, Variant> configuration, String languageTag) {
            sendBroadcastNotification("Configuration Changed");
        }

        /* (non-Javadoc)
         * @see org.alljoyn.config.server.RestartHandler#restart()
         */
        @Override
        public void restart() {
            try {
                mAsyncHandler.sendEmptyMessage(AsyncHandler.DISCONNECT);
                mAsyncHandler.sendEmptyMessage(AsyncHandler.CONNECT);
                sendBroadcastNotification("Restart (AJ reconnected)");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        /* (non-Javadoc)
         * @see org.alljoyn.config.server.ConfigChangeListener#onResetConfiguration(java.lang.String, java.lang.String[])
         */
        @Override
        public void onResetConfiguration(String language, String[] fieldsToRemove) {
            sendBroadcastNotification("Reset Configuration on: " + Arrays.toString(fieldsToRemove));
        }
    }

    private void registerAuthenticationListener() {
        // Register authentication listener. This is needed as Config service is secure.
        mKeyStoreFileName = getFileStreamPath("alljoyn_keystore").getAbsolutePath();
        mAuthListener = new SrpAnonymousKeyListener(ConfigService.this);
        Status authStatus = mBus.registerAuthListener(mAuthListener.getAuthMechanismsAsString(), mAuthListener, mKeyStoreFileName);
        Log.d(TAG, "BusAttachment.registerAuthListener status = " + authStatus);
        if (authStatus != Status.OK) {
            Log.d(TAG, "Failed to register Auth listener status = " + authStatus.toString());
        }
    }

    /**
     * Send a broadcast indicating a message from the Config Service.
     */
    private void sendBroadcastNotification(String message) {
        Bundle bundle = new Bundle();
        bundle.putString(BROADCAST_NOTIFICATION_MESSAGE, message);
        sendBroadcast(BROADCAST_NOTIFICATION, bundle);
    }

    /**
     * Helper method to send broadcasts.
     */
    private void sendBroadcast(String action, Bundle extras) {
        Intent intent = new Intent(action);
        if (extras != null && !extras.isEmpty()) {
            intent.putExtras(extras);
        }
        getApplicationContext().sendBroadcast(intent);
    }

    private String bindSessionPort(final short port) {
        /*
         * Create a new session listening on the contact port of the about/config service.
         */
        Mutable.ShortValue contactPort = new Mutable.ShortValue(port);

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = true;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        Status status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
            @Override
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                return sessionPort == port;
            }
            @Override
            public void sessionJoined(short sessionPort, int id, String joiner){
                Log.i(TAG, String.format("SessionPortListener.sessionJoined(%d, %d, %s)", sessionPort, id, joiner));
            }
        });

        String logMessage = String.format("BusAttachment.bindSessionPort(%d, %s): %s",
                contactPort.value, sessionOpts.toString(),status);
        Log.d(TAG, logMessage);
        if (status != Status.OK) {
            return "";
        }

        String serviceName = mBus.getUniqueName();

        status = mBus.advertiseName(serviceName, SessionOpts.TRANSPORT_ANY);
        Log.d(TAG, String.format("BusAttachment.advertiseName(%s): %s", serviceName, status));

        return serviceName;
    }

    // ------------------------ Authentication callbacks --------------------------------
    // Called by AllJoyn when we access the Config service (a secure interface)

    /* (non-Javadoc)
     * @see org.alljoyn.config.server.SetPasswordHandler#setPassword(java.lang.String, char[])
     */
    @Override
    public void setPassword(String realmName, char[] password) {
        mMyPass = password;
        mBus.clearKeyStore(); //remove all encryption keys.
    }

    /* (non-Javadoc)
     * @see org.alljoyn.config.samples.configservice.security.AuthPasswordHandler#getPassword(java.lang.String)
     */
    @Override
    public char[] getPassword(String peerName) {
        return mMyPass != null ? mMyPass : SrpAnonymousKeyListener.DEFAULT_PINCODE;
    }

    /* (non-Javadoc)
     * @see org.alljoyn.config.samples.configservice.security.AuthPasswordHandler#completed(java.lang.String, java.lang.String, boolean)
     */
    @Override
    public void completed(String mechanism, String authPeer, boolean authenticated) {
        if (!authenticated) {
            Log.w(TAG, " ** " + authPeer + " failed to authenticate");
        } else {
            Log.i(TAG, " ** " + authPeer + " successfully authenticated");
        }
    }

}
