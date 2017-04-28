package org.alljoyn.bus.samples.securitymanager;

import android.app.ActivityManager;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import org.alljoyn.bus.ApplicationStateListener;
import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.PermissionConfigurationListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.SecurityApplicationProxy;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.KeyInfoNISTP256;

import java.io.File;
import java.lang.reflect.Array;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.jar.Manifest;

public class MainActivity extends AppCompatActivity {

    private static final String MANIFEST_XML =
            "<manifest>" +
                    "<node>" +
                    "<interface>" +
                    "<any>" +
                    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
                    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" +
                    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" +
                    "</any>" +
                    "</interface>" +
                    "</node>" +
                    "</manifest>";

    private static final String MANIFEST_ALL_INCLUSIVE =
            "<manifest>" +
                    "<node name=\"*\">" +
                    "<interface name=\"*\">" +
                    "<method name=\"*\">" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
                    "</method>" +
                    "<signal name=\"*\">" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
                    "</signal>" +
                    "<property name=\"*\">" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
                    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
                    "</property>" +
                    "</interface>" +
                    "</node>" +
                    "</manifest>";

    private static final String TAG = "MainActivity";

    static {
        System.loadLibrary("alljoyn_java");
    }

    private ListView listView;
    private SecurityApplicationStateListener secAppStateListener;

    private BusAttachment bus;

    private MainListAdapter adapter;

    private UUID manager_uuid;

    private KeyInfoNISTP256 manager_key;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        manager_uuid = UUID.randomUUID();

        bus = new BusAttachment("SecurityManager");
        bus.connect();

        bus.registerAboutListener(new AboutAppNameListener());
        bus.whoImplements(null);

        Status status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                mAuthListener,
                Environment.getExternalStorageDirectory().getAbsolutePath() + "/alljoyn_security_manager.ks",
                false,
                mPcListener);
        Log.i(TAG, "registerAuthListener Status " + status.toString());

        PermissionConfigurator pc = bus.getPermissionConfigurator();
        try {
            manager_key = pc.getSigningPublicKey();
            manager_key.setKeyId(CertificateX509.generateAuthorityKeyId(manager_key.getPublicKey()));
            pc.setManifestTemplateFromXml(MANIFEST_XML);

            CertificateX509 identityCert = generateSignedIdentityCertificate(manager_key.getPublicKey());
            String signedManifest = pc.computeThumbprintAndSignManifestXml(identityCert, MANIFEST_ALL_INCLUSIVE);
            pc.claim(manager_key,
                    UUID.randomUUID(),
                    manager_key,
                    new CertificateX509[]{identityCert},
                    new String[]{signedManifest});

            CertificateX509 membershipCert = generateSignedMembershipCertificate(manager_key.getPublicKey(), manager_uuid);
            pc.installMembership(new CertificateX509[]{membershipCert});
        } catch (BusException e) {
            e.printStackTrace();
        }

        listView = (ListView) findViewById(R.id.listView);

        secAppStateListener = new SecurityApplicationStateListener(this);
        bus.registerApplicationStateListener(secAppStateListener);

        listView.setOnItemClickListener(onItemClickListener);
        listView.setOnItemLongClickListener(onItemLongClickListener);

        adapter = new MainListAdapter(this, secAppStateListener);
        listView.setAdapter(adapter);

    }

    public void refresh() {
        runOnUiThread(new setText());
    }

    public class setText implements Runnable {
        @Override
        public void run() {
            adapter.notifyDataSetChanged();
        }
    }

    private AdapterView.OnItemClickListener onItemClickListener = new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            Thread thread = new Thread(new ClaimAndAddMembership(position));
            thread.start();
        }
    };

    private AdapterView.OnItemLongClickListener onItemLongClickListener = new AdapterView.OnItemLongClickListener() {
        @Override
        public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
            Thread thread = new Thread(new Unclaim(position));
            thread.start();
            return true;
        }
    };

    private class ClaimAndAddMembership implements Runnable {
        private int position;

        private ClaimAndAddMembership(int pos) {
            position = pos;
        }

        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();
            SessionOpts sessionOpts = new SessionOpts();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) 101,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            Log.i(TAG, "Join Session Status " + status.toString());

            SecurityApplicationProxy sap = null;
            try {
                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                List<KeyInfoNISTP256> tempKeys = new ArrayList<>(secAppStateListener.getMap().keySet());
                CertificateX509 identity = generateSignedIdentityCertificate(tempKeys.get(position).getPublicKey());
                String signedManifest = bus.getPermissionConfigurator().computeThumbprintAndSignManifestXml(
                        identity,
                        sap.getManifestTemplate());

                sap.claim(manager_key,
                        manager_uuid,
                        manager_key,
                        new CertificateX509[]{identity},
                        new String[]{signedManifest});

                Thread.sleep(10000);
                bus.leaveSession(sessionId.value);

                status = bus.joinSession(tempList.get(position).getBusName(),
                        (short) 101,
                        sessionId,
                        sessionOpts,
                        new SessionListener());
                Log.i(TAG, "Join Session Status " + status.toString());

                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                Log.i(TAG, "secureConnection " + status.toString());

                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);


                Log.i(TAG, "claim guid " + manager_uuid.toString());

                sap.endManagement();

                /*CertificateX509 membership = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);


                UUID groupUUID = UUID.randomUUID();

                membership.setSubjectPublicKey(tempKeys.get(position).getPublicKey());
                membership.setCA(false);
                membership.setSubjectCN(CertificateX509.generateAuthorityKeyId(tempKeys.get(position).getPublicKey()));
                membership.setValidity(start, end);
                membership.setSerial("1".getBytes());
                membership.setIssuerCN(caKey.getKeyId());
                membership.signAndGenerateAuthorityKeyId(cryptoECC.getDSAPrivateKey(), caKey.getPublicKey());
                sap.startManagement();
                sap.installMembership(new CertificateX509[]{membership});*/
            } catch (BusException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    };

    private class Unclaim implements Runnable {
        private int position;

        private Unclaim(int pos) {
            position = pos;
        }

        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();
            SessionOpts sessionOpts = new SessionOpts();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) 101,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            Log.i(TAG, "Join Session Status " + status.toString());

            SecurityApplicationProxy sap = null;
            try {

                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                Log.i(TAG, "secureConnection " + status.toString());

                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);
                sap.reset();

            } catch (BusException e) {
                e.printStackTrace();
            }
        }
    };

    private AuthListener mAuthListener = new AuthListener() {
        @Override
        public boolean requested(String mechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
            return true;
        }

        @Override
        public void completed(String mechanism, String authPeer, boolean authenticated) {
        }
    };

    private PermissionConfigurationListener mPcListener = new PermissionConfigurationListener() {
        @Override
        public Status factoryReset() {
            Log.i(TAG, "factoryReset()");
            return Status.OK;
        }

        @Override
        public void policyChanged() {
            Log.i(TAG, "policyChanged()");
        }

        @Override
        public void startManagement() {
            Log.i(TAG, "startManagement()");
        }

        @Override
        public void endManagement() {
            Log.i(TAG, "endManagement()");
        }
    };

    public CertificateX509 generateSignedIdentityCertificate(ECCPublicKey subjectPublicKey) throws BusException{
        CertificateX509 retCert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        retCert.setSubjectAltName(("abcedf1234567").getBytes());
        retCert.setSubjectPublicKey(subjectPublicKey);
        retCert.setCA(false);
        retCert.setSubjectOU("MyTestIdentity".getBytes());
        long start = System.currentTimeMillis() / 1000;
        long end = start + 3600 * 24 * 10 * 365;
        start -= 3600;
        retCert.setValidity(start, end);
        retCert.setSerial("0".getBytes());
        retCert.setIssuerCN("Issuer".getBytes());
        bus.getPermissionConfigurator().signCertificate(retCert);
        return retCert;
    }

    public CertificateX509 generateSignedMembershipCertificate(ECCPublicKey subjectPublicKey, UUID groupID) throws BusException{
        CertificateX509 retCert = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
        bb.putLong(groupID.getMostSignificantBits());
        bb.putLong(groupID.getLeastSignificantBits());
        retCert.setSubjectAltName(bb.array());
        retCert.setCA(false);

        byte[] arr = bb.array();
        Log.i(TAG, "generateSignedMembershipCertificate " + Arrays.toString(arr));
        retCert.setSubjectPublicKey(subjectPublicKey);
        retCert.setSubjectCN("MyTestIdentity".getBytes());

        long start = System.currentTimeMillis() / 1000;
        long end = start + 3600 * 24 * 10 * 365;
        start -= 3600;

        retCert.setValidity(start, end);
        retCert.setSerial("10".getBytes());
        retCert.setIssuerCN("Issuer".getBytes());
        bus.getPermissionConfigurator().signCertificate(retCert);
        return retCert;
    }

}
