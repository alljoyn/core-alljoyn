/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus;

import org.apache.cordova.api.Plugin;
import org.apache.cordova.api.PluginResult;
import org.json.JSONArray;

public class AllJoyn extends Plugin { // TODO look at extending CordovaPlugin instead...

    public AllJoyn() {
        create();
    }

    @Override
    protected void finalize() throws Throwable {
        destroy();
    }

    public PluginResult execute(String action, JSONArray args, String callbackId) {
        execute(action, args.toString(), callbackId);
        PluginResult result = new PluginResult(PluginResult.Status.NO_RESULT);
        result.setKeepCallback(true);
        return result;
    }

    public boolean isSynch(String action) {
        return false;
    }

    static {
        System.loadLibrary("npalljoyn");
    }

    private long handle;

    private String getKeyStoreFilename() {
        return ctx.getContext().getFileStreamPath("alljoyn_keystore").getAbsolutePath();
    }
    private native void create();
    private synchronized native void destroy();
    private native void execute(String action, String args, String callbackId);
}