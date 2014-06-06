/*
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.rawservice;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

import java.lang.reflect.Field;

import java.io.InputStream;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.BufferedReader;
import java.io.IOException;

public class Service extends Activity {
    /*
     * Load the native alljoyn_java library.  The actual AllJoyn code is
     * written in C++ and the alljoyn_java library provides the language
     * bindings from Java to C++ and vice versa.
     */
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "RawService";

    private static final int MESSAGE_PING = 1;
    private static final int MESSAGE_PING_REPLY = 2;
    private static final int MESSAGE_POST_TOAST = 3;

    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;
    private Menu menu;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_PING:
                String ping = (String) msg.obj;
                mListViewArrayAdapter.add("Ping:  " + ping);
                break;
            case MESSAGE_PING_REPLY:
                String reply = (String) msg.obj;
                mListViewArrayAdapter.add("Reply:  " + reply);
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }
        }
    };

    /*
     * An event loop used to make calls to AllJoyn methods.
     */
    private Handler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        /*
         * You always need to make all AllJoyn calls through a separate handler
         * thread to prevent blocking the Activity.
         */
        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        /*
         * Create a ervice bus object and send a message to the AllJoyn bus
         * handler asking it to do whatever it takes to start the service
         * and make it available out to the rest of the world.
         */
        mRawService = new RawService();
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        this.menu = menu;
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
        case R.id.quit:
            finish();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        /*
         * Explicitly disconnect from the AllJoyn bus to prevent any resource
         * leaks.
         */
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    /*
     * Our service is actually an AllJoyn bus object that will be located at
     * a certain object path in a bus attachment.  This bus object implements
     * a named interface.  The name of the interface implies a contract that
     * certain methods, signals and properties will be available at the object.
     * The fact that there is a bus object implementing a certain named
     * interface is implied by virtue of the fact that the attachment has
     * requested a particular well-known bus name.  The presence of the well-
     * known name also implies the existence of a well-known or contact session
     * port which clients can use to join sessions.
     *
     * The contact ports, bus objects, methods, etc., are all implied by the
     * associated names.  In the case of the "org.alljoyn.bus.samples.raw" well
     * known name, the presence of an interface is implied.  This interrace is
     * named "org.alljoyn.bus.samples.raw.RawInterface", and you can find its
     * definition in RawInterface.java as the specified @BusInterface.  The
     * well-known name also implies that the interface is implemented by a bus
     * object which can be found at a particular location defined by an object
     * path.  Also implied by the well-known name is the session contact port.
     *
     * In order to export an AllJoyn service, you then must declare a named
     * interface that you want your service to support, for example:
     *
     *   @BusInterface(name = "org.alljoyn.bus.samples.raw.RawInterface")
     *   public interface RawInterface {
     *       @BusMethod
     *      void YourMethod() throws BusException;
     *   }
     *
     * You must provide an implementation of this interface as an AllJoyn bus
     * object (that inherits from class BusObject), for example:
     *
     *   class YourService implements YourInterface, BusObject {
     *     public void YourMethod() { }
     *   }
     *
     * You must create an instance of your bus obejct and register it at the
     * object path which will be implied by the well-known name, for example:
     *
     *   YourService yourService;
     *   mBus.registerBusObject(yourService, "/YourServiceObjectPath");
     *
     * You must request that the AllJoyn bus grant you the well-known name.
     * This is done since there can be at most one service on the distributed
     * bus with a given name.  If you require more than one instance of your
     * service, you can append an instance number to your name.
     *
     *   mBus.requestName(YOUR_SERVICE_WELL_KNOWN_NAME, flags);
     *
     * Now that you have your object all set up, you need to bind the well
     * known session port (also known as the contact port).  This turns the
     * provided session port number into a half-association (associated with
     * your service) which clients can connect to.
     *
     *   mBus.bindSessionPort(YOUR_CONTACT_SESSION_PORT, sessionOptions);
     *
     * The last step in the service creation process is to advertise the
     * existence of your service to the outside world.
     *
     *   mBus.advertiseName(YOUR_SERVICE_WELL_KNOWN_NAME, sessionOptions);
     *
     * You can find these steps in the message loop dedicated to handling the
     * AllJoyn bus events, below.
     */
    class RawService implements RawInterface, BusObject {
        /*
         * This is the code run when the client makes a call to the
         * RequestRawSession method of the RawInterface.  This implementation
         * just returns the previously bound RAW_PORT.
         *
         * The actual mapping of the bus object to the declared interface
         * and finally to this method is handled somewhat magically using
         * Java reflection based on annotations at the various levels.
         */
        public short RequestRawSession() {
            return BusHandler.RAW_PORT;
        }
    }

    /*
     * The bus object that actually implements the service we are providing
     * to the world.
     */
    private RawService mRawService;

    /*
     * This class will handle all AllJoyn calls.  It implements an event loop
     * used to avoid blocking the Android Activity when interacting with the
     * bus.  See onCreate().
     */
    class BusHandler extends Handler {
        /*
         * Name used as the well-known name and the advertised name.  This name must be a unique name
         * both to the bus and to the network as a whole.  The name uses reverse URL style of naming.
         */
        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.raw";
        private static final short CONTACT_PORT = 88;

        /*
         * TODO: Remove this when we ephemeral session ports is fully implemented.
         */
        private static final String RAW_SERVICE_NAME = "org.alljoyn.bus.samples.yadda888";
        private static final short RAW_PORT = 888;

        private BusAttachment mBus = null;
        private int mRawSessionId = -1;
        public BufferedReader mInputStream = null;
        boolean mStreamUp = false;

        private Thread mReader = null;

        /*
         * These are the (event) messages sent to the BusHandler to tell it
         * what to do.
         */
        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;
        public static final int JOINED = 3;

        public BusHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            /*
             * When we recieve the CONNECT message, it instructs the handler to
             * build a bus attachment and service and connect it to the AllJoyn
             * bus to start our service.
             */
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                /*
                 * All communication through AllJoyn begins with a BusAttachment.
                 * A BusAttachment needs a name. The actual name is unimportant
                 * except for internal security. As a default we use the class
                 * name.
                 *
                 * By default AllJoyn does not allow communication between
                 * physically remote bus attachments.  The second argument must
                 * be set to Receive to enable this communication.
                 */
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * If using the debug version of the AllJoyn libraries, tell
                 * them to write debug output to the OS log so we can see it
                 * using adb logcat.  Turn on all of the debugging output from
                 * the Java language bindings (module ALLJOYN_JAVA).  When one
                 * builds the samples in alljoyn_java, the library appropriate
                 * to the build variant is copied in; so if the variant is
                 * debug, the following will work.
                 */
                mBus.useOSLogging(true);
                mBus.setDebugLevel("ALLJOYN_JAVA", 7);

                /*
                 * To make a service available to other AllJoyn peers, first
                 * register a BusObject with the BusAttachment at a specific
                 * object path.  Our service is implemented by the RawService
                 * BusObject found at the "/RawService" object path.
                 */
                Status status = mBus.registerBusObject(mRawService, "/RawService");
                logStatus("BusAttachment.registerBusObject()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * The next step in making a service available to other peers
                 * is to connect the BusAttachment to the AllJoyn bus.
                 */
                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * We now request a well-known name from the bus.  This is an
                 * alias for the unique name which we are automatically given
                 * when we hook up to the bus.  This name must be unique across
                 * the distributed bus and acts as the human-readable name of
                 * our service.
                 *
                 * We have the oppportunity to ask the underlying system to
                 * queue our request to be granted the well-known name, but we
                 * decline this opportunity so we will fail if another instance
                 * of this service is already running.
                 */
                status = mBus.requestName(SERVICE_NAME, BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE);
                logStatus(String.format("BusAttachment.requestName(%s, 0x%08x)", SERVICE_NAME,
                                        BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE), status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * TODO: Remove this second advertisement which is used until
                 * we can fix the ephemeral session port problem.
                 */
                status = mBus.requestName(RAW_SERVICE_NAME, BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE);
                logStatus(String.format("BusAttachment.requestName(%s, 0x%08x)", RAW_SERVICE_NAME,
                                        BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE), status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * If we sucessfully receive permission to use the requested
                 * service name, we need to Create a new session listening on
                 * the contact port of the raw service.  The default session
                 * options are sufficient for our purposes
                 */
                Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);
                SessionOpts sessionOpts = new SessionOpts();

                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    /**
                     * Callback indicating a joiner has requested to be admitted
                     * to the raw session.  We always allow this.
                     */
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        logInfo(String.format("BusListener.acceptSessionJoiner(%d, %s, %s): accepted on CONTACT_PORT",
                                              sessionPort, joiner, sessionOpts.toString()));
                        return true;
                    }
                });
                logStatus("BusAttachment.bindSessionPort()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Create a new raw session listening on the another port.
                 * This can be done on-demand in a "real" application, but it
                 * is convenient to do the job as a well-known contact port
                 * here.  We will just return this well-known port as if it
                 * were an ephemeral session port when our (single) client asks
                 * for a new raw session.
                 *
                 * We have to change the session options object to reflect that
                 * it is a raw session we want to bind.  The difference is in
                 * the traffic flowing across the session, so we need to change
                 * the traffic type to RAW_RELIABLE, which will imply TCP, for
                 * example, if we are using an IP based transport mechanism.
                 */
                contactPort.value = RAW_PORT;
                sessionOpts.traffic = SessionOpts.TRAFFIC_RAW_RELIABLE;

                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    /**
                     * Callback indicating a joiner has requested to be admitted
                     * to the raw session.  We always allow this.
                     */
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        logInfo(String.format("BusListener.acceptSessionJoiner(%d, %s, %s): accepted on RAW_PORT",
                                              sessionPort, joiner, sessionOpts.toString()));
                        return true;
                    }

                    /**
                     * Notification callback indicating the raw session has
                     * been successfully joined.
                     *
                     * We need a cue as to when there is a TCP link up between
                     * the client and server.  This is when the RAW_RELIABLE
                     * session has been established and is indicated by this
                     * callback.
                     *
                     * When we get this callback, we need to pull the underlying
                     * socket FD out of the session specified by the provided
                     * sessionId and begin listening on it for client data.  As
                     * usual we arrange for this by sending a message to the
                     * AllJoyn bus handler.
                     */
                    @Override
                    public void sessionJoined(short sessionPort, int sessionId, String joiner) {
                        logInfo(String.format("BusListener.sessionJoined(%d, %d, %s): on RAW_PORT",
                                              sessionPort, sessionId, joiner));
                        mRawSessionId = sessionId;
                        mBusHandler.sendEmptyMessage(BusHandler.JOINED);
                    }
                });

                logStatus("BusAttachment.bindSessionPort()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Advertise the same well-known name so future clients can
                 * discover our existence.
                 */
                status = mBus.advertiseName(SERVICE_NAME, SessionOpts.TRANSPORT_ANY);
                logStatus(String.format("BusAttachement.advertiseName(%s)", SERVICE_NAME), status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                status = mBus.advertiseName(RAW_SERVICE_NAME, SessionOpts.TRANSPORT_ANY);
                logStatus(String.format("BusAttachement.advertiseName(%s)", RAW_SERVICE_NAME), status);
                if (status != Status.OK) {
                    finish();
                    return;
                }
                break;
            }

            /*
             * We have a new joiner with a given mRawSessionId that has joined
             * our raw session.  This is provided to us in the sessionJoined
             * callback we provide during binding.
             *
             * The session we got is in raw mode, but we need to get a socket
             * file descriptor back from AllJoyn that represents the TCP
             * established connection.  Once we have the sock FD we are then
             * free to do whatever we want with it.
             *
             * What we are going to do with the sock FD is to create a Java
             * file descriptor out of it, and then use that Java FD to create
             * an input stream.
             */
            case JOINED: {
                /*
                 * Get the socket FD from AllJoyn.  It is a socket FD just
                 * any other file descriptor.
                 */
                Mutable.IntegerValue sockFd = new Mutable.IntegerValue();
                Status status = mBus.getSessionFd(mRawSessionId, sockFd);
                logStatus("BusAttachment.getSession()", status);
                try {
                    /*
                     * We have a socked FD, but now we need a Java file
                     * descriptor.  There is no appropriate constructor,
                     * public or not in the Dalvik FileDescriptor, so
                     * we new up a file descriptor and set its internal
                     * descriptor field using reflection.
                     */
                    Field field = FileDescriptor.class.getDeclaredField("descriptor");
                    field.setAccessible(true);
                    FileDescriptor fd = new FileDescriptor();
                    field.set(fd, sockFd.value);

                    /*
                     * Now that we have a FileDescriptor with an AllJoyn
                     * raw session socket FD in it, we can use it like
                     * any other "normal" FileDescriptor.
                     */
                    InputStream is = new FileInputStream(fd);
                    final Reader reader = new BufferedReader(new InputStreamReader(is), 80);

                    /*
                     * We don't want to block the AllJoyn message handler so we
                     * spin up a thread to read characters from the raw reliable
                     * stream.  This is far from a real example of something
                     * useful to do with the stream, but we do show bytes moving
                     * across the underlying TCP connection.
                     */
                    logInfo("Spinning up read thread");
                    mReader = new Thread(new Runnable() {
                        public void run() {
                            logInfo("Thread running");
                            Looper.myLooper().prepare();
                            StringBuilder stringBuilder = new StringBuilder();
                            try {
                                while (true) {
                                    int c;
                                    /*
                                     * Wait until a character is available.
                                     */
                                    if (reader.ready() == false) {
                                        Thread.sleep(100);
                                        continue;
                                    }

                                    /*
                                     * Build a string out of the characters and
                                     * when we see a newline, cook up a toast
                                     * do display them.
                                     */
                                    try {
                                        c = reader.read();
                                        logInfo(String.format("Read %d from stream", c));
                                        stringBuilder.append((char)c);
                                        if (c == 10) {
                                            String s = stringBuilder.toString();
                                            logInfo(String.format("Read %s from stream", s));
                                            Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, s);
                                            mHandler.sendMessage(toastMsg);
                                            stringBuilder.delete(0, stringBuilder.length() - 1);
                                        }
                                    } catch (IOException ex) {
                                    }
                                }
                            } catch (Throwable ex) {
                                logInfo(String.format("Exception reading raw Java stream: %s", ex.toString()));
                            }
                            logInfo("Thread exiting");
                        }
                    }, "reader");
                    mReader.start();

                } catch (Throwable ex) {
                    logInfo("Exception Bringing up raw Java stream");
                }
                break;
            }

            /*
             * Release all of the bus-related resources acquired during and
             * after the case CONNECT.
             */
            case DISCONNECT: {
                /*
                 * It is important to unregister the BusObject before disconnecting from the bus.
                 * Failing to do so could result in a resource leak.
                 */
                mBus.unregisterBusObject(mRawService);
                mBus.disconnect();
                mBusHandler.getLooper().quit();
                break;
            }

            default:
                break;
            }
        }
    }

    private void logStatus(String msg, Status status) {
        String log = String.format("%s: %s", msg, status);
        if (status == Status.OK) {
            Log.i(TAG, log);
        } else {
            Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
            mHandler.sendMessage(toastMsg);
            Log.e(TAG, log);
        }
    }

    /*
     * Print the status or result to the Android log. If the result is the expected
     * result only print it to the log.  Otherwise print it to the error log and
     * Sent a Toast to the users screen.
     */
    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }
}
