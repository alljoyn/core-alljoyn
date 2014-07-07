/*
 * Copyright (c) 2009-2011,2013-2014 AllSeen Alliance. All rights reserved.
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

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;

public class AllJoynDaemon {
    private String daemonAddress;
    private String daemonRemoteAddress;
    private Process daemon;
    private String pid;
    private Thread errorReader;
    private Thread inputReader;

    private class StreamReader extends Thread {
        private BufferedReader br;

        public StreamReader(InputStream stream) {
            br = new BufferedReader(new InputStreamReader(stream));
        }

        public void run() {
            try {
                String line = null;
                while ((line = br.readLine()) != null) {
                    //System.err.println(line);
                    String[] split = line.split(" *= *");
                    if (split.length == 2 && split[0].matches(".*PID")) {
                        pid = split[1];
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public AllJoynDaemon() {
        daemonAddress = System.getProperty("org.alljoyn.bus.daemonaddress",
            "unix:abstract=AllJoynDaemonTest");
        daemonRemoteAddress = System.getProperty("org.alljoyn.bus.daemonremoteaddress",
            "tcp:addr=127.0.0.1,port=5343");
        if ("The Android Project".equals(System.getProperty("java.vendor"))) {
            return;
        }
        try {
            String address = "BUS_SERVER_ADDRESSES=" + daemonAddress + ";" + daemonRemoteAddress;
            daemon = Runtime.getRuntime().exec("bbdaemon", new String[] { address });
            errorReader = new StreamReader(daemon.getErrorStream());
            inputReader = new StreamReader(daemon.getInputStream());
            errorReader.start();
            inputReader.start();
            /* Wait a bit for bbdaemon to get initialized */
            while (pid == null) {
                Thread.sleep(100);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void stop() {
        try {
            if (daemon != null) {
                Runtime.getRuntime().exec("kill -s SIGINT " + pid);
                daemon.waitFor();
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public String address() {
        return daemonAddress;
    }

    public String remoteAddress() {
        return daemonRemoteAddress;
    }
}
