package org.alljoyn.bus.samples.securitymanager;

import android.content.Context;
import android.util.Log;

import org.alljoyn.bus.ApplicationStateListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.common.KeyInfoNISTP256;

import java.security.Key;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by George on 12/19/2016.
 */
public class SecurityApplicationStateListener implements ApplicationStateListener {

    private static final String TAG = "SAppStateListener";

    private Map<KeyInfoNISTP256, OnlineApplication> map;
    public Map<KeyInfoNISTP256, OnlineApplication> getMap() {
        return map;
    }

    private MainActivity context;

    public SecurityApplicationStateListener(MainActivity context) {
        this.context = context;
        map = new HashMap<KeyInfoNISTP256, OnlineApplication>();
    }

    @Override
    public void state(String busName,
                      KeyInfoNISTP256 key,
                      PermissionConfigurator.ApplicationState applicationState) {

        Log.i(TAG, "State Callback " + applicationState);
        OnlineApplication app = new OnlineApplication(busName, applicationState, OnlineApplication.ApplicationSyncState.SYNC_OK);
        map.put(key, app);
        context.refresh();

    }
}
