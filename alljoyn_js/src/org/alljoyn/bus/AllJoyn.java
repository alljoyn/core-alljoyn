/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
