/*
 * This sample is identical to the simple sample, with the addition of security.  Refer to the
 * simple sample for further explanation of the AllJoyn code not called out here.
 *
 * Copyright (c) 2010-2011, 2014 AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.samples.secureservice;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.CountDownLatch;

import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class Service extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "SecureService";

    private static final int DIALOG_ONE_TIME_PASSWORD = 1;
    private static final int DIALOG_CREATE_PASSPHRASE = 2;
    private static final int DIALOG_ENTER_PASSPHRASE = 3;

    private static final int MESSAGE_PING = 1;
    private static final int MESSAGE_PING_REPLY = 2;
    private static final int MESSAGE_SHOW_ONE_TIME_PASSWORD_DIALOG = 3;
    private static final int MESSAGE_SHOW_PASSPHRASE_DIALOG = 4;
    private static final int MESSAGE_AUTH_COMPLETE = 5;
    private static final int MESSAGE_POST_TOAST = 6;

    private ArrayAdapter<String> mListViewArrayAdapter;
    private ListView mListView;
    private Menu menu;
    private int mCredentialsDialog;
    private CountDownLatch mLatch;
    private String mPassword;

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
            case MESSAGE_SHOW_ONE_TIME_PASSWORD_DIALOG:
                mCredentialsDialog = DIALOG_ONE_TIME_PASSWORD;
                showDialog(mCredentialsDialog);
                break;
            case MESSAGE_SHOW_PASSPHRASE_DIALOG:
                AuthListener.PasswordRequest request = (AuthListener.PasswordRequest) msg.obj;
                mCredentialsDialog = request.isNewPassword()
                        ? DIALOG_CREATE_PASSPHRASE : DIALOG_ENTER_PASSPHRASE;
                showDialog(mCredentialsDialog);
                break;
            case MESSAGE_AUTH_COMPLETE:
                if (mCredentialsDialog != 0) {
                    dismissDialog(mCredentialsDialog);
                    mCredentialsDialog = 0;
                }
                Boolean authenticated = (Boolean) msg.obj;
                if (authenticated.equals(Boolean.FALSE)) {
                    Toast.makeText(Service.this, "Authentication failed",
                                   Toast.LENGTH_LONG).show();
                }
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }
        }
    };

    /* The authentication listener for our bus attachment. */
    private AuthListeners mAuthListener;

    private SecureService mSecureService;

    private Handler mBusHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListViewArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        mListView = (ListView) findViewById(R.id.ListView);
        mListView.setAdapter(mListViewArrayAdapter);

        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());

        mAuthListener = new AuthListeners();
        mSecureService = new SecureService();
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
        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
        case DIALOG_ONE_TIME_PASSWORD: {
            LayoutInflater factory = LayoutInflater.from(this);
            View view = factory.inflate(R.layout.one_time_password_dialog, null);
            return new AlertDialog.Builder(this)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(R.string.one_time_password_dialog)
                .setCancelable(false)
                .setView(view)
                .create();
        }
        case DIALOG_CREATE_PASSPHRASE:
        case DIALOG_ENTER_PASSPHRASE: {
            int title = (id == DIALOG_CREATE_PASSPHRASE)
                    ? R.string.create_passphrase_dialog : R.string.enter_passphrase_dialog;
            LayoutInflater factory = LayoutInflater.from(this);
            View view = factory.inflate(R.layout.passphrase_dialog, null);
            EditText editText = (EditText) view.findViewById(R.id.PassphraseEditText);
            editText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                    if (actionId == EditorInfo.IME_NULL
                            && event.getAction() == KeyEvent.ACTION_UP) {
                        mPassword = view.getText().toString();
                        mLatch.countDown();
                        dismissDialog(mCredentialsDialog);
                    }
                    return true;
                }
            });
            return new AlertDialog.Builder(this)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(title)
                .setCancelable(false)
                .setView(view)
                .create();
        }
        default:
            return null;
        }
    }

    @Override
    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
        case DIALOG_ONE_TIME_PASSWORD: {
            TextView textView = (TextView) dialog.findViewById(R.id.PasswordText);
            textView.setText(mPassword);
            break;
        }
        default:
            break;
        }
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
     *
     * This class, registered with the BusAttachment, supports all the available authentication
     * mechanisms.
     */
    class AuthListeners implements AuthListener {

        /* Map from authentication mechanism to the specific listener. */
        private Map<String, AuthListener> mAuthListeners;

        public AuthListeners() {
            mAuthListeners = new HashMap<String, AuthListener>();
            mAuthListeners.put("ALLJOYN_SRP_KEYX", new SrpKeyXListener());
            mAuthListeners.put("ALLJOYN_SRP_LOGON", new SrpLogonListener());
            mAuthListeners.put("ALLJOYN_RSA_KEYX", new RsaKeyXListener());
            ECDHEKeyXListener ecdheListener = new ECDHEKeyXListener();
            mAuthListeners.put("ALLJOYN_ECDHE_NULL", ecdheListener);
            mAuthListeners.put("ALLJOYN_ECDHE_PSK", ecdheListener);
            mAuthListeners.put("ALLJOYN_ECDHE_ECDSA", ecdheListener);
        }

        /* Returns the list of supported mechanisms. */
        public String getMechanisms() {
            StringBuilder mechanisms = new StringBuilder();
            for (String mechanism : mAuthListeners.keySet()) {
                mechanisms.append(mechanism).append(" ");
            }
            return mechanisms.toString();
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
         * A true return value tells the BusAttachment that the requests have been handled.
         *
         * This simply defers to the specific listener based on the mechanism in use.
         */
        public boolean requested(String mechanism, String peer, int count, String userName,
                AuthRequest[] requests) {
            AuthListener listener = mAuthListeners.get(mechanism);
            if (listener != null) {
                return listener.requested(mechanism, peer, count, userName, requests);
            } else {
                return false;
            }
        }

        /*
         * An authentication attempt has completed, either successfully or unsuccessfully.
         *
         * This simply defers to the specific listener based on the mechanism in use.
         */
        public void completed(String mechanism, String authPeer, boolean authenticated) {
            AuthListener listener = mAuthListeners.get(mechanism);
            if (listener != null) {
                listener.completed(mechanism, authPeer, authenticated);
            }
            sendUiMessage(MESSAGE_AUTH_COMPLETE, authenticated);
        }
    }

    /*
     * This Secure Remote Password Key eXchange listener creates a random 6-digit pincode for each
     * authentication attempt.
     */
    class SrpKeyXListener implements AuthListener {
        private Random mPasswordGenerator;
        private boolean mGeneratePassword;

        public SrpKeyXListener() {
            mPasswordGenerator = new Random();
            mGeneratePassword = true;
        }

        public boolean requested(String mechanism, String peer, int count, String userName,
                AuthRequest[] requests) {
            if (mGeneratePassword) {
                mGeneratePassword = false;
                mPassword = String.format("%06d", mPasswordGenerator.nextInt(1000000));
                sendUiMessage(MESSAGE_SHOW_ONE_TIME_PASSWORD_DIALOG, null);
            }
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    ((PasswordRequest) request).setPassword(mPassword.toCharArray());
                }
            }
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {
            mGeneratePassword = true;
        }
    }

    /*
     * This Secure Remote Password Logon listener responds to authentication requests from a table
     * of username, password pairs similar to /etc/passwd.
     */
    class SrpLogonListener implements AuthListener {
        private Map<String, char[]> mUserNamePassword;

        /* Populate the username, password table used by this listener. */
        public SrpLogonListener() {
            mUserNamePassword = new HashMap<String, char[]>();
            mUserNamePassword.put("user1", "password1".toCharArray());
            mUserNamePassword.put("user2", "password2".toCharArray());
        }

        /*
         * Given the user name, lookup the password.  Returning true without setting the password
         * tells the authentication engine to ask the peer for the username again.
         *
         * A pre-computed token called the logon entry may also be supplied instead of the given
         * password.  See LogonEntryRequest.
         */
        public boolean requested(String mechanism, String peer, int count, String userName,
                AuthRequest[] requests) {
            char[] password = mUserNamePassword.get(userName);
            if (password != null) {
                for (AuthRequest request : requests) {
                    if (request instanceof PasswordRequest) {
                        ((PasswordRequest) request).setPassword(password);
                    }
                }
            }
            return true;
        }

        public void completed(String mechanism, String peer, boolean authenticated) {
        }
    }

    /* The RSA Key eXchange listener uses X.509 certificates to authenticate. */
    class RsaKeyXListener implements AuthListener {

        public boolean requested(String mechanism, String peer, int count, String userName,
                AuthRequest[] requests) {
            /* Collect the requests we're interested in to simplify processing below. */
            PasswordRequest passwordRequest = null;
            CertificateRequest certificateRequest = null;
            VerifyRequest verifyRequest = null;
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    passwordRequest = (PasswordRequest) request;
                } else if (request instanceof CertificateRequest) {
                    certificateRequest = (CertificateRequest) request;
                } else if (request instanceof VerifyRequest) {
                    verifyRequest = (VerifyRequest) request;
                }
            }

            if (verifyRequest != null) {
                /* Verify a certificate chain supplied by the peer. */
                return true;
            } else if (certificateRequest != null) {
                /*
                 * The engine is asking us for our certificate chain.
                 *
                 * If we return true and do not supply the certificate chain, then the engine will
                 * create a self-signed certificate for us.  It will ask for the passphrase to use
                 * for the private key via a PasswordRequest.
                 */
                return true;
            } else if (passwordRequest != null) {
                /*
                 * A password request under the ALLJOYN_RSA_KEYX mechanism is for the passphrase of the
                 * private key.
                 *
                 * PasswordRequest.isNewPassword() indicates if the engine has created a private key
                 * for us (as part of creating a self-signed certificate).  Otherwise it is
                 * expecting the passphrase for the existing private key.
                 */
                try {
                    if (count <= 3) {
                        /*
                         * Request the passphrase of our private key via the UI.  We need to wait
                         * here for the user to enter the passphrase before we can return.  The
                         * latch takes care of the synchronization for us.
                         */
                        mLatch = new CountDownLatch(1);
                        sendUiMessage(MESSAGE_SHOW_PASSPHRASE_DIALOG, passwordRequest);
                        mLatch.await();
                        passwordRequest.setPassword(mPassword.toCharArray());
                        return true;
                    }
                } catch (InterruptedException ex) {
                    Log.e(TAG, "Error waiting for password", ex);
                }
            }
            return false;
        }

        public void completed(String mechanism, String peer, boolean authenticated) {
        }
    }
    
    /* the ECDHEKeyXListener supports the Elliptic Curve Cryptography Diffie-Hellman Ephemeral 
     * key exchanges.
     */
    class ECDHEKeyXListener implements AuthListener {

        public ECDHEKeyXListener() {

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
                            String privateKeyPEM = SERVER_PK_PEM;
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
                        	boolean useCert1 = true;
                        	if (useCert1) {
                        		certChainPEM = SERVER_CERT1_PEM;
                        	}
                        	else {
                        		certChainPEM = SERVER_CERT2_PEM;
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
        private static final String SERVER_PK_PEM =
                "-----BEGIN PRIVATE KEY-----" +
                "tV/tGPp7kI0pUohc+opH1LBxzk51pZVM/RVKXHGFjAcAAAAA" +
                "-----END PRIVATE KEY-----";
        private static final String SERVER_CERT1_PEM =
                "-----BEGIN CERTIFICATE-----" +
                "AAAAAfUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n" + 
                "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n" + 
                "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n" + 
                "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3AABMa7uTLSqjDggO0t6TAgsxKNt\n" + 
                "+Zhu/jc3s242BE0drPcL4K+FOVJf+tlivskovQ3RfzTQ+zLoBH5ZCzG9ua/dAAAA\n" + 
                "ACt5bWBzbcaT0mUqwGOVosbMcU7SmhtE7vWNn/ECvpYFAAAAAA==\n" +
                "-----END CERTIFICATE-----";
        private static final String SERVER_CERT2_PEM =
        		"AAAAAvUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n" +
                "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n" +
                "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n" +
                "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3ABjeWi1/GbBcdnK0yJvL4X/UF0h\n" +
                "8plX3uAhOlF2vT2jfxe5U06zaWSXcs9kBEQvfOc+WvKloM7m5NFJNSd3qFFGUhfj\n" +
                "xx/0CCRJlk/jeIWmzQAAAAB8bexqa95eHEKTqdc8+qKFKggZZXlpaj9af/MFocIP\n" +
                "NQAAAAA=\n" +
                "-----END CERTIFICATE-----";
    }

    class SecureService implements SecureInterface, BusObject {

        public String Ping(String inStr) {
            sendUiMessage(MESSAGE_PING, inStr);
            sendUiMessage(MESSAGE_PING_REPLY, inStr);
            return inStr;
        }
    }

    class BusHandler extends Handler {

        private static final String SERVICE_NAME = "org.alljoyn.bus.samples.secure";
        private static final short CONTACT_PORT = 42;

        private BusAttachment mBus;

        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;

        public BusHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case CONNECT: {
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

                mBus.registerBusListener(new BusListener());

                status = mBus.registerBusObject(mSecureService, "/SecureService");
                logStatus("BusAttachment.registerBusObject()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * Create a new session listening on the contact port of the chat service.
                 */
                Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);

                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        logInfo(String.format("MyBusListener.acceptSessionJoiner(%d, %s, %s)", sessionPort, joiner,
                                              sessionOpts.toString()));
                        if (sessionPort == CONTACT_PORT) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                });
                logStatus("BusAttachment.bindSessionPort()", status);
                if (status != Status.OK) {
                    finish();
                    return;
                }

                /*
                 * request a well-known name from the bus
                 */
                int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;

                status = mBus.requestName(SERVICE_NAME, flag);
                logStatus(String.format("BusAttachment.requestName(%s, 0x%08x)", SERVICE_NAME, flag), status);
                if (status == Status.OK) {
                    /*
                     * If we successfully obtain a well-known name from the bus
                     * advertise the same well-known name
                     */
                    status = mBus.advertiseName(SERVICE_NAME, SessionOpts.TRANSPORT_ANY);
                    logStatus(String.format("BusAttachement.advertiseName(%s)", SERVICE_NAME), status);
                    if (status != Status.OK) {
                        /*
                         * If we are unable to advertise the name, release
                         * the well-known name from the local bus.
                         */
                        status = mBus.releaseName(SERVICE_NAME);
                        logStatus(String.format("BusAttachment.releaseName(%s)", SERVICE_NAME), status);
                        finish();
                        return;
                    }
                }
                break;
            }

            case DISCONNECT: {
                mBus.unregisterBusObject(mSecureService);
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
     * print the status or result to the Android log. If the result is the expected
     * result only print it to the log.  Otherwise print it to the error log and
     * Sent a Toast to the users screen.
     */
    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }
}
