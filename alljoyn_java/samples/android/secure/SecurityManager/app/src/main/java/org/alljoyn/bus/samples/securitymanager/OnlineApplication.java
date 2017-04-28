package org.alljoyn.bus.samples.securitymanager;

import org.alljoyn.bus.PermissionConfigurator;

/**
 * Created by George on 12/19/2016.
 */
public class OnlineApplication {

    private String busName;
    private PermissionConfigurator.ApplicationState appState;
    private ApplicationSyncState syncState;

    public String getBusName() {
        return busName;
    }

    public PermissionConfigurator.ApplicationState getAppState() {
        return appState;
    }

    public ApplicationSyncState getSyncState() {
        return syncState;
    }

    public enum ApplicationSyncState {
        SYNC_UNKNOWN,
        SYNC_UNMANAGED,
        SYNC_OK,
        SYNC_PENDING,
        SYNC_WILL_RESET,
        SYNC_RESET
    }

    public OnlineApplication(String bName,
                             PermissionConfigurator.ApplicationState appState,
                             ApplicationSyncState syncState) {
        busName = bName;
        this.appState = appState;
        this.syncState = syncState;
    }


}
