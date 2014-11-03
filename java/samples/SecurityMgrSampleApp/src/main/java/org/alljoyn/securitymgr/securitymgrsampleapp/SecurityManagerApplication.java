package org.alljoyn.securitymgr.securitymgrsampleapp;

import android.app.Application;
import android.util.Log;

public class SecurityManagerApplication extends Application implements SecurityManagerService.SecurityManagerServiceConnection
{
    private static final String TAG = "SecurityManagerApplication";

    @Override
    public void onCreate()
    {
        Log.d(TAG, "Creating application context");
        super.onCreate();

        //Keep a connection from the application context to the SecurityManagerService.
        //This to make sure this service is only instantiated once during the lifecycle of the app
        //This is very bad for:
        //* Memory usage
        //* Cpu usage
        //* Battery usage
        //But currently the native layer can't handle destruction and re-creation
        SecurityManagerService.getLocalBind(this, this);
    }


    @Override
    public void onLocalServiceConnected(SecurityManagerService service)
    {
        Log.d(TAG, "Local service connected");
    }

    @Override
    public void onLocalServiceDisconnected()
    {

    }
}
