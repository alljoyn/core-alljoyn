/*
 * Copyright AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.samples;

import java.io.FileOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.File;
import java.io.IOException;

import org.alljoyn.bus.KeyStoreListener;
import org.alljoyn.bus.BusException;

public class DoorKeyStoreListener implements KeyStoreListener {
    private File file;

    public DoorKeyStoreListener() {
        String home = System.getProperty("user.home");
        File dir = new File(home + "/.alljoyn_keystore");
        dir.mkdir();
        file = new File(home + "/.alljoyn_keystore/JavaSecureDoorService");

        try {
            file.createNewFile();
        } catch (IOException e) {
        }
    }

    public String getKeyStorePath() {
        return file.getAbsolutePath();
    }

    public byte[] getKeys() throws BusException {
        ByteArrayOutputStream ous = null;
        InputStream ios = null;

        try {
            byte[] buffer = new byte[4096];
            ous = new ByteArrayOutputStream();
            ios = new FileInputStream(file);
            int read = 0;
            while ((read = ios.read(buffer)) != -1) {
                ous.write(buffer, 0, read);
            }
        } catch (IOException e) {
        } finally {
            try {
                if (ous != null) {
                    ous.close();
                }
            } catch (IOException e) {
                // swallow, since not that important
            }
            try {
                if (ios != null) {
                    ios.close();
                }
            } catch (IOException e) {
                // swallow, since not that important
            }
        }

        return ous.toByteArray();
    }

    private static final String PASSWORD = "password";
    public char[] getPassword() throws BusException {
        return PASSWORD.toCharArray();
    }

    public void putKeys(byte[] keys) throws BusException {
        FileOutputStream stream = null;
        try {
            stream = new FileOutputStream(file.getAbsolutePath());
            stream.write(keys);
        } catch (IOException e) {
        } finally {
            try {
                if (stream != null) {
                    stream.close();
                }
            } catch (IOException e) {
                // swallow, since not that important
            }
        }
    }
}
