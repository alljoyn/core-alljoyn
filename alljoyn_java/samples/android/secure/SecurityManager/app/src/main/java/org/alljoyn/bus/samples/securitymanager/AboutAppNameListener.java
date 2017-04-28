package org.alljoyn.bus.samples.securitymanager;

import android.util.Log;

import org.alljoyn.bus.AboutData;
import org.alljoyn.bus.AboutListener;
import org.alljoyn.bus.AboutObjectDescription;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Variant;

import java.util.Map;

/**
 * Created by George on 1/2/2017.
 */
public class AboutAppNameListener implements AboutListener {
    private static final String TAG = "AboutAppNameListener";
    @Override
    public void announced(String busName, int version, short port, AboutObjectDescription[] aboutObjectDescriptions, Map<String, Variant> map) {
        Log.i(TAG, "busName " + busName);

        try {
            AboutData aboutData = new AboutData(map);
            Log.i(TAG, "ApplicationName " + aboutData.getAppName());
        } catch (BusException e) {
            e.printStackTrace();
        }
    }
}