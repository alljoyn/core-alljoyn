package org.alljoyn.securitymgr.securitymgrsampleapp;

import org.alljoyn.securitymgr.*;
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

/**
 * Service that contains all logic to interact with the native layer.
 */
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


        //Object that will get the callbacks if a new application is discovered.
        mApplicationsTracker = new ApplicationsTracker();

        String appPath = getFilesDir().getAbsolutePath();

        try {
            //initialize the native layer. This must only be done once in the entire lifecycle of the application!
            mSecurityHandler = new DefaultSecurityHandler(mApplicationsTracker, this, appPath);
        }
        catch (SecurityMngtException e) {
            Log.e(TAG, "Error initializing native SecurityHandler", e);
            stopSelf();
        }

        if (mSecurityHandler == null) {
            Log.e(TAG, "Could not create default security manager!");
            stopSelf();
        }

        //Fetch the list of applications that are stored in the database.
        mApplicationsTracker.addApplications(mSecurityHandler.getApplications());
    }

    @Override
    public void onDestroy()
    {
        Log.d(TAG, "Destroying SecurityManagerService");
        if (mMulticastLock != null && mMulticastLock.isHeld()) {
            //release multicast lock
            mMulticastLock.release();
        }
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        return mLocalBinder;
    }

    /**
     * Create a local bound connection to this service.
     * @param ctx The android context.
     * @param conn The callback interface that will be called when the service is bound.
     * @return A connnection manager. Should be used for closing the connection.
     */
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

    /**
     * Get the ApplicationsTracker object.
     * @return The ApplicationsTracker
     */
    public ApplicationsTracker getApplicationsTracker()
    {
        return mApplicationsTracker;
    }

    /**
     * Claim an application. @see DefaultSecurityHandler#claimApplication(ApplicationInfo) <br>
     * The @link setManifestApprover(ManifestApprover) function must be called upfront.<br>
     * This call is blocking and should never be done from the UI thread.
     * @param applicationInfo The application to claim.
     * @throws SecurityMngtException On exception.
     */
    public void claimApplication(ApplicationInfo applicationInfo) throws SecurityMngtException
    {
        if (mManifestApprover == null) {
            throw new IllegalStateException("ManifestApprover not yet set");
        }
        mSecurityHandler.claimApplication(applicationInfo);
    }

    /**
     * Unclaim an application. @see DefaultSecurityHandler#unclaimApplication(ApplicationInfo)
     * @param applicationInfo The application to claim.
     * @throws SecurityMngtException On exception.
     */
    public void unClaimApplication(ApplicationInfo applicationInfo) throws SecurityMngtException
    {
        mSecurityHandler.unclaimApplication(applicationInfo);
    }

    /**
     * Set the ManifestApprover callbacks.
     * @param manifestApprover the ManifestApprover. Set to null to unset.
     */
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

        private LocalServiceConnection(Context ctx, SecurityManagerServiceConnection conn)
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

        /**
         * Close/unbind the serviceconnection.
         */
        public void unBind()
        {
            mCtx.unbindService(this);
        }
    }

    /**
     * Interface to receive callbacks when the connection to the SecurityManagerService is bound/lost
     */
    public interface SecurityManagerServiceConnection
    {
        /**
         * Called when the service is connected.
         * @param service The service object.
         */
        void onLocalServiceConnected(SecurityManagerService service);

        /**
         * Called when the service is lost unexpected.
         */
        void onLocalServiceDisconnected();
    }


}
