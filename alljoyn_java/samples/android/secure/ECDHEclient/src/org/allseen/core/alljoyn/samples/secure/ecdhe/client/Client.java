/*
 * This sample is identical to the simple sample, with the addition of security.  Refer to the
 * simple sample for further explanation of the AllJoyn code not called out here.
 *
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

package org.allseen.core.alljoyn.samples.secure.ecdhe.client;

import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class Client extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }


    private static final int MESSAGE_PING = 1;
    private static final int MESSAGE_PING_REPLY = 2;
    private static final int MESSAGE_AUTH_COMPLETE = 3;
    private static final int MESSAGE_POST_TOAST = 4;
    private static final int MESSAGE_START_PROGRESS_DIALOG = 5;
    private static final int MESSAGE_STOP_PROGRESS_DIALOG = 6;

    private static final String TAG = "ECDHEClient";

    private EditText mEditText;
    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;
    private Menu menu;
    private ProgressDialog mDialog;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_PING:
                String ping = (String) msg.obj;
                mListViewArrayAdapter.add("Ping:  " + ping);
                break;
            case MESSAGE_PING_REPLY:
                String ret = (String) msg.obj;
                mListViewArrayAdapter.add("Reply:  " + ret);
                mEditText.setText("");
                break;

            case MESSAGE_AUTH_COMPLETE:
                Boolean authenticated = (Boolean) msg.obj;
                if (authenticated.equals(Boolean.FALSE)) {
                    Toast.makeText(Client.this, "Authentication failed", Toast.LENGTH_LONG).show();
                }
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            case MESSAGE_START_PROGRESS_DIALOG:
                mDialog = ProgressDialog.show(Client.this, "",
                                              "Finding Security Service.\nPlease wait...", true, true);
                break;
            case MESSAGE_STOP_PROGRESS_DIALOG:
                mDialog.dismiss();
                break;
            default:
                break;
            }
        }
    };

    /* The authentication listener for our bus attachment. */
    private ECDHEKeyXListener mAuthListener;

    private BusHandler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        mEditText = (EditText) findViewById(R.id.EditText);
        mEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_NULL
                        && event.getAction() == KeyEvent.ACTION_UP) {
                    Message msg = mBusHandler.obtainMessage(BusHandler.PING,
                                                            view.getText().toString());
                    mBusHandler.sendMessage(msg);
                }
                return true;
            }
        });

        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        mAuthListener = new ECDHEKeyXListener();
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
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

        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }



    private void sendUiMessage(int what, Object obj) {
        mHandler.sendMessage(mHandler.obtainMessage(what, obj));
    }



    /*
     * The main differences between a secure application and a plain application, besides the
     * @Secure annotations of the interfaces, are encapsulated in the AuthListener.  The
     * BusAttachment calls the listener with various authentication requests in the process of
     * authenticating a peer.  The requests made are dependent on the specific authentication
     * mechanism negotiated between the peers.
     */
    class ECDHEKeyXListener implements AuthListener {

        public ECDHEKeyXListener() {

        }
        /* Returns the list of supported mechanisms. The valid list is a combo of
         * ALLJOYN_ECDHE_NULL
         * ALLJOYN_ECDHE_PSK
         * ALLJOYN_ECDHE_ECDSA
         *
         */
        public String getMechanisms() {
            return  "ALLJOYN_ECDHE_ECDSA";
        }

        /*
         * Persistent authentication and encryption data is stored at this location.
         *
         * This uses the private file area associated with the application package.
         */
        public String getKeyStoreFileName() {
            return getFileStreamPath("alljoyn_keystore").getAbsolutePath();
        }

        /*
         * Authentication requests are being made.  Contained in this call are the mechanism in use,
         * the number of attempts made so far, the desired user name for the requests, and the
         * specific credentials being requested in the form of AuthRequests.
         *
         * The ECDHE key exchange can request for private key, public key, and to verify the peer
         */
        public boolean requested(String authMechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
            try {
                Log.d(TAG, "Listener requested with mechanism " + authMechanism);
                if (!authMechanism.equals("ALLJOYN_ECDHE_NULL") &&
                        !authMechanism.equals("ALLJOYN_ECDHE_PSK") &&
                        !authMechanism.equals("ALLJOYN_ECDHE_ECDSA")) {
                    return false;
                }

                for (AuthRequest rqst: requests) {
                    if (rqst instanceof PrivateKeyRequest) {
                        /*
                         * Only the ALLJOYN_ECDHE_ECDSA requests for DSA private key.
                         * The application may provide the DSA private key and public key in the certificate.
                         * AllJoyn stores the keys in the key store for future use.
                         * If the application does not provide the private key, AllJoyn will
                         * generate the DSA key pair.
                         */
                        if (sendBackKeys) {
                            PrivateKeyRequest pkRqst = (PrivateKeyRequest) rqst;
                            String privateKeyPEM = CLIENT_PK_PEM;
                            pkRqst.setPrivateKey(privateKeyPEM);
                            Log.d(TAG, "Listener sends back private key " + privateKeyPEM);
                        }
                    }
                    else if (rqst instanceof CertificateRequest) {
                        /*
                         * Only the ALLJOYN_ECDHE_ECDSA requests for DSA private key.
                         * The application may provide the DSA private key and public key in the certificate.
                         * AllJoyn stores the keys in the key store for future use.
                         * If the application does not provide the private key, AllJoyn will
                         * generate the DSA key pair.
                         */
                        if (sendBackKeys) {
                        	String certChainPEM;
                        	boolean useCert1 = false;
                        	if (useCert1) {
                        		certChainPEM = CLIENT_CERT1_PEM;
                        	}
                        	else {
                        		certChainPEM = CLIENT_CERT2_PEM;
                        	}
                            CertificateRequest certChainRqst = (CertificateRequest) rqst;
                            certChainRqst.setCertificateChain(certChainPEM);
                            Log.d(TAG, "Listener sends back cert chain " + certChainPEM);
                        }
                    }
                    else if (rqst instanceof PasswordRequest) {
                        /*
                         * Only the ECDHE_PSK key exchange requests for the pre shared secret.
                         * Based on the pre shared secret id given the username input variable,
                         * the application can retrieve the pre shared secret
                         * from storage or from the end user.
                         * In this example, the pre shared secret is a hard coded string
                         */
                        PasswordRequest pskRqst = (PasswordRequest) rqst;
                        String psk = "123456";
                        pskRqst.setPassword(psk.toCharArray());
                        Log.d(TAG, "Listener sends back PSK " + psk);
                    }
                    else if (rqst instanceof VerifyRequest) {
                        /*
                         * Only the ECDHE_ECDSA key exchange sends a VerifyRequest
                         * with the certificate chain from the peer for the
                         * application to verify.
                         * The application has to option to verify the certificate chain.
                         * If the cert chain is validated and trusted then return true; otherwise, return false.
                         */
                        VerifyRequest verifyRqst = (VerifyRequest) rqst;
                        String certPEM = verifyRqst.getCertificateChain();
                        Log.d(TAG, "Listener: verifying cert " + certPEM);
                    }
                    else if (rqst instanceof ExpirationRequest) {
                        ExpirationRequest er = (ExpirationRequest) rqst;
                        er.setExpiration(100); /* expired 100 seconds from now */
                    }
                }

                return true;

            } catch (Exception ex) {
                Log.e(TAG, "Error processing auth request", ex);
            }
            return false;
        }

        public void completed(String authMechanism, String authPeer, boolean authenticated) {
            sendUiMessage(MESSAGE_AUTH_COMPLETE, authenticated);
        }


        private boolean sendBackKeys = true;  /* toggle the send back keys */
        /* the followings are same data to try out the ECDHE_ECDSA key exchange */
        private static final String CLIENT_PK_PEM =
                "-----BEGIN PRIVATE KEY-----" +
                "CkzgQdvZSOQMmqOnddsw0BRneCNZhioNMyUoJwec9rMAAAAA" +
                "-----END PRIVATE KEY-----";
        private static final String CLIENT_CERT1_PEM =
                "-----BEGIN CERTIFICATE-----" +
                "AAAAAZ1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n" +
                "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n" +
                "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n" +
                "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQABMa7uTLSqjDggO0t6TAgsxKNt\n" +
                "+Zhu/jc3s242BE0drNFJAiGa/u6AX5qdR+7RFxVuqm251vKPgWjfwN2AesHrAAAA\n" +
                "ANsNwJl8Z1v5jbqo077qdQIT6aM1jc+pKXdgNMk6loqFAAAAAA==\n" +
                "-----END CERTIFICATE-----";
        private static final String CLIENT_CERT2_PEM =
                "-----BEGIN CERTIFICATE-----" +
                "AAAAAp1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n" +
                "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n" +
                "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n" +
                "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQCJ9dkuY0Z6jjx+a8azIQh4UF0h\n" +
                "8plX3uAhOlF2vT2jfxe5U06zaWSXcs9kBEQvfOeMM4sUtoXPArUA+TNahfOS9Bbf\n" +
                "0Hh08SvDJSDgM2OetQAAAAAYUr2pw2kb90fWblBWVKnrddtrI5Zs8BYx/EodpMrS\n" +
                "twAAAAA=\n" +
                "-----END CERTIFICATE-----";
    }

    class BusHandler extends Handler {
        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.secure";
        private static final short CONTACT_PORT=42;

        private BusAttachment mBus;
        private ProxyBusObject mProxyObj;
        private SecureInterface mSecureInterface;

        private int     mSessionId;
        private boolean mIsConnected;
        private boolean mIsStoppingDiscovery;

        public static final int CONNECT = 1;
        public static final int JOIN_SESSION = 2;
        public static final int DISCONNECT = 3;
        public static final int PING = 4;

        public BusHandler(Looper looper) {
            super(looper);

            mIsConnected = false;
            mIsStoppingDiscovery = false;
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case CONNECT: {
                // Acquire multicast lock
                boolean doMulticastLock = true;
                MulticastLock multicastLock = null;
                if (doMulticastLock) {
                    WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
                    multicastLock = wifi.createMulticastLock("multicastLock");
                    multicastLock.setReferenceCounted(/* true */ false);
                    multicastLock.acquire();
                }
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                /*
                 * Register the AuthListener before calling connect() to ensure that everything is
                 * in place before any remote peers access the service.
                 */
                Status status = mBus.registerAuthListener(mAuthListener.getMechanisms(),
                                                          mAuthListener,
                                                          mAuthListener.getKeyStoreFileName());
                logStatus("BusAttachment.registerAuthListener()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                mBus.registerBusListener(new BusListener() {
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                        logInfo(String.format("MyBusListener.foundAdvertisedName(%s, 0x%04x, %s)", name, transport, namePrefix));
                        /*
                         * This client will only join the first service that it sees advertising
                         * the indicated well-known name.  If the program is already a member of
                         * a session (i.e. connected to a service) we will not attempt to join
                         * another session.
                         * It is possible to join multiple session however joining multiple
                         * sessions is not shown in this sample.
                         */
                        if(!mIsConnected){
                            Message msg = obtainMessage(JOIN_SESSION, name);
                            sendMessage(msg);
                        }
                    }
                });

                /* The following block enables the ECDHE key exchange debug messages to show in the Android log */
                boolean daemonDebugEnabled = false;
                if (daemonDebugEnabled) {
                    mBus.useOSLogging(true);
                    mBus.setDaemonDebug("ALLJOYN_AUTH", 3);
                    mBus.setDebugLevel("ALLJOYN_AUTH", 3);
                    mBus.setDaemonDebug("AUTH_KEY_EXCHANGER", 3);
                    mBus.setDebugLevel("AUTH_KEY_EXCHANGER", 3);
                }
                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (Status.OK != status) {
                    finish();
                    return;
                }

                /*
                 * Now find an instance of the AllJoyn object we want to call.  We start by looking for
                 * a name, then connecting to the device that is advertising that name.
                 *
                 * In this case, we are looking for the well-known SERVICE_NAME.
                 */
                status = mBus.findAdvertisedName(SERVICE_NAME);
                logStatus(String.format("BusAttachement.findAdvertisedName(%s)", SERVICE_NAME), status);
                if (Status.OK != status) {
                    finish();
                    return;
                }
                break;
            }

            case (JOIN_SESSION): {
                /*
                 * If discovery is currently being stopped don't join to any other sessions.
                 */
                if (mIsStoppingDiscovery) {
                    break;
                }

                /*
                 * In order to join the session, we need to provide the well-known
                 * contact port.  This is pre-arranged between both sides as part
                 * of the definition of the chat service.  As a result of joining
                 * the session, we get a session identifier which we must use to
                 * identify the created session communication channel whenever we
                 * talk to the remote side.
                 */
                short contactPort = CONTACT_PORT;
                SessionOpts sessionOpts = new SessionOpts();
                Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

                Status status = mBus.joinSession((String) msg.obj, contactPort, sessionId, sessionOpts, new SessionListener(){
                    @Override
                    public void sessionLost(int sessionId) {
                        mIsConnected = false;
                        logInfo(String.format("MyBusListener.sessionLost(%d)", sessionId));
                        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
                    }
                });
                logStatus("BusAttachment.joinSession()", status);

                if (status == Status.OK) {
                    /*
                     * To communicate with an AllJoyn object, we create a ProxyBusObject.
                     * A ProxyBusObject is composed of a name, path, sessionID and interfaces.
                     *
                     * This ProxyBusObject is located at the well-known SERVICE_NAME, under path
                     * "/SimpleService", uses sessionID of CONTACT_PORT, and implements the SimpleInterface.
                     */
                    mProxyObj =  mBus.getProxyBusObject(SERVICE_NAME,
                                                        "/SecureService",
                                                        sessionId.value,
                                                        new Class[] { SecureInterface.class });

                    /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
                    mSecureInterface = mProxyObj.getInterface(SecureInterface.class);

                    mSessionId = sessionId.value;
                    mIsConnected = true;
                    mHandler.sendEmptyMessage(MESSAGE_STOP_PROGRESS_DIALOG);
                }
                break;
            }
            case DISCONNECT: {
                mIsStoppingDiscovery = true;
                if (mIsConnected) {
                    Status status = mBus.leaveSession(mSessionId);
                    logStatus("BusAttachment.leaveSession()", status);
                }
                mBus.disconnect();
                getLooper().quit();
                break;
            }

            case PING: {
                if (mSecureInterface != null) {
                    try {
                        sendUiMessage(MESSAGE_PING, msg.obj);
                        String reply = mSecureInterface.Ping((String) msg.obj);
                        sendUiMessage(MESSAGE_PING_REPLY, reply);
                    } 
                    catch (ErrorReplyBusException erbe) {
                    	logException("SecureInterface.Ping() " + erbe.getErrorName() + ": " +
                           erbe.getErrorMessage() + " status: " + erbe.getErrorStatus().getErrorCode(), erbe);
                    }
                    catch (BusException ex) {
                        logException("SecureInterface.Ping()", ex);
                    }
                }
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

    private void logException(String msg, BusException ex) {
        String log = String.format("%s: %s", msg, ex);
        Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
        mHandler.sendMessage(toastMsg);
        Log.e(TAG, log, ex);
    }

    /*
     * print the status or result to the Android log. If the result is the expected
     * result only print it to the log.  Otherwise print it to the error log and
     * Sent a Toast to the users screen.
     */
    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }
}
