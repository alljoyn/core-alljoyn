package org.alljoyn.securitymgr.securitymgrsampleapp;

import org.alljoyn.securitymgr.ApplicationInfo;
import org.alljoyn.securitymgr.DefaultSecurityHandler;
import org.alljoyn.securitymgr.ManifestApprover;
import org.alljoyn.securitymgr.SecurityMngtException;
import org.alljoyn.securitymgr.access.Manifest;
import org.alljoyn.securitymgr.securitymgrsampleapp.helper.ApplicationsTracker;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class SecurityManagerService extends Service implements ManifestApprover
{
    private static final String TAG = "SecurityManagerService";
    private LocalBinder mLocalBinder;
    private WifiManager.MulticastLock mMulticastLock;
    private DefaultSecurityHandler mSecurityHandler = null;
    private ApplicationsTracker mApplicationsTracker = null;
    private ManifestApprover mManifestApprover = null;

    @Override
    public void onCreate()
    {
        super.onCreate();
        Log.d(TAG, "Creating SecurityManagerService");
        mLocalBinder = new LocalBinder(this);

        //take wifi multicast lock
        final WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        if (wifi != null) {
            // take multicast lock. Not required on all devices as some have
            // this by default enabled in their drivers.
            mMulticastLock = wifi.createMulticastLock("SecurityManagerService");
            mMulticastLock.acquire();
        }

        mApplicationsTracker = new ApplicationsTracker();

        //Initialize native component
        String userName = "Demo";
        String keystoreUser = "Demo";
        String keystorePass = "DemoPass";
        String appPath = getFilesDir().getAbsolutePath();

        try {
            mSecurityHandler = new DefaultSecurityHandler(userName, mApplicationsTracker, this, appPath,
                keystoreUser, keystorePass);
        }
        catch (SecurityMngtException e) {
            Log.e(TAG, "Error initializing native SecurityHandler", e);
            stopSelf();
        }

        if (mSecurityHandler == null) {
            Log.e(TAG, "Could not create default security manager!");
            stopSelf();
        }
        mApplicationsTracker.addApplications(mSecurityHandler.getApplications());
    }

    @Override
    public void onDestroy()
    {
        Log.d(TAG, "Destroying SecurityManagerService");
        if (mMulticastLock != null && mMulticastLock.isHeld()) {
            mMulticastLock.release();
        }
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        return mLocalBinder;
    }

    public static LocalServiceConnection getLocalBind(Context ctx, SecurityManagerServiceConnection conn)
    {
        Log.d(TAG, "Creating local bound connection");
        Intent i = new Intent(ctx, SecurityManagerService.class);
        LocalServiceConnection serviceConnection = new LocalServiceConnection(ctx, conn);
        ctx.bindService(i, serviceConnection, Service.BIND_AUTO_CREATE);
        return serviceConnection;
    }

    @Override
    public boolean approveManifest(ApplicationInfo applicationInfo, Manifest manifest)
    {
        Log.d(TAG, "Need to approve manifest for " + applicationInfo);
        if (mManifestApprover == null) {
            Log.e(TAG, "No manifest approver set");
            return false; //always reject
        }
        return mManifestApprover.approveManifest(applicationInfo, manifest);
    }

    public ApplicationsTracker getApplicationsTracker()
    {
        return mApplicationsTracker;
    }

    public DefaultSecurityHandler getSecurityHandler()
    {
        return mSecurityHandler;
    }

    public void setManifestApprover(ManifestApprover manifestApprover)
    {
        mManifestApprover = manifestApprover;
    }

    //////////////////////////////////////
    // helper classes below
    //////////////////////////////////////

    private static class LocalBinder extends Binder
    {
        private final SecurityManagerService mService;

        public LocalBinder(SecurityManagerService service)
        {
            mService = service;
        }

        public SecurityManagerService getService()
        {
            return mService;
        }
    }

    public static class LocalServiceConnection implements ServiceConnection
    {
        private final SecurityManagerServiceConnection mConn;
        private final Context mCtx;

        public LocalServiceConnection(Context ctx, SecurityManagerServiceConnection conn)
        {
            mConn = conn;
            mCtx = ctx;
        }

        @Override
        public final void onServiceConnected(ComponentName name, IBinder iBinder)
        {
            Log.d(TAG, "Notify onServiceConnected");
            LocalBinder localBinder = (LocalBinder) iBinder;
            mConn.onLocalServiceConnected(localBinder.getService());
        }

        @Override
        public final void onServiceDisconnected(ComponentName name)
        {
            mConn.onLocalServiceDisconnected();
        }

        public void unBind()
        {
            mCtx.unbindService(this);
        }
    }

    public interface SecurityManagerServiceConnection
    {
        void onLocalServiceConnected(SecurityManagerService service);

        void onLocalServiceDisconnected();
    }


}
