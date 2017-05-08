package org.alljoyn.bus.samples.securitymanager;

import android.content.Context;
import android.util.Log;
import android.os.Message;

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
import org.alljoyn.bus.samples.securitymanager.SecurityApplicationStateListener;

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

public class AlljoynActions {

    static {
        System.loadLibrary("alljoyn_java");
    }

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
    private static final String TAG = "AlljoynAction";

    private BusAttachment bus;
    private UUID managerUuid;
    private UUID managerGroupUuid;
    private KeyInfoNISTP256 managerKey;
    protected SecurityApplicationStateListener secAppStateListener;

    private MainActivity main;
    public AlljoynActions(MainActivity context) {
        main = context;
    }

    protected Runnable busStart = new Runnable() {
        public void run() {
            bus = new BusAttachment("SecurityManager");
            bus.connect();

            bus.registerAboutListener(new AboutAppNameListener());
            bus.whoImplements(null);

            managerGroupUuid = UUID.randomUUID();
            managerUuid = UUID.randomUUID();

            Status status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                    mAuthListener,
                    main.getFileStreamPath("alljoyn_keystore").getAbsolutePath(),
                    false,
                    mPcListener);
            
            if (status != Status.OK) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to registerAuthListener " + status.toString() + " Restart Application.");
                main.errorHandler.sendMessage(msg);
                return;
            }

            PermissionConfigurator pc = bus.getPermissionConfigurator();
            try {
                managerKey = pc.getSigningPublicKey();
                managerKey.setKeyId(CertificateX509.generateAuthorityKeyId(managerKey.getPublicKey()));
                pc.setManifestTemplateFromXml(MANIFEST_XML);

                CertificateX509 identityCert = generateSignedIdentityCertificate(managerKey.getPublicKey());
                String signedManifest = pc.computeThumbprintAndSignManifestXml(identityCert, MANIFEST_XML);
                pc.claim(managerKey,
                        UUID.randomUUID(),
                        managerKey,
                        new CertificateX509[]{identityCert},
                        new String[]{signedManifest});

                CertificateX509 membershipCert = generateSignedMembershipCertificate(managerKey.getPublicKey(), managerUuid);
                pc.installMembership(new CertificateX509[]{membershipCert});
            } catch (BusException e) {
                e.printStackTrace();
            }
            secAppStateListener = new SecurityApplicationStateListener(main);
            bus.registerApplicationStateListener(secAppStateListener);

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

    public void setPosition(int position) {
        this.position = position;
    }

    private int position;

    private SessionOpts sessionOpts = new SessionOpts();
    protected Runnable claimAndAddMembership = new Runnable() {

        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) 101,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to joinSession " + status.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            List<KeyInfoNISTP256> tempKeys = new ArrayList<>(secAppStateListener.getMap().keySet());

            SecurityApplicationProxy sap = null;
            try {
                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                CertificateX509 identity = generateSignedIdentityCertificate(tempKeys.get(position).getPublicKey());
                String signedManifest = bus.getPermissionConfigurator().computeThumbprintAndSignManifestXml(
                        identity,
                        sap.getManifestTemplate());

                sap.claim(managerKey,
                        managerUuid,
                        managerKey,
                        new CertificateX509[]{identity},
                        new String[]{signedManifest});
                bus.secureConnection(null, true);
            } catch (Exception e) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to claim " + e.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            try {
                sap.endManagement();
            } catch (Exception e) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to endManagement " + e.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            Message msg = main.errorHandler.obtainMessage(MainActivity.SUCCESS,
                    "Claiming Succeeded");
            main.errorHandler.sendMessage(msg);
        }
    };

    protected Runnable installMembership = new Runnable() {
        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) 101,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to joinSession " + status.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            SecurityApplicationProxy sap = null;
            try {
                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                if (status != Status.OK) {
                    Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                            "Failed to secureConnection " + status.toString());
                    main.errorHandler.sendMessage(msg);
                    return;
                }

                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                sap.startManagement();

                CertificateX509 membership = generateSignedMembershipCertificate(sap.getEccPublicKey(), managerGroupUuid);
                sap.installMembership(new CertificateX509[]{membership});
                sap.endManagement();
                bus.leaveSession(sessionId.value);
            } catch (BusException e) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to installMembership " + e.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }
            Message msg = main.errorHandler.obtainMessage(MainActivity.SUCCESS,
                    "install Membership succeeded");
            main.errorHandler.sendMessage(msg);
        }
    };

    protected Runnable installPolicy = new Runnable() {
        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) 101,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to joinSession " + status.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            SecurityApplicationProxy sap = null;
            try {
                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                if (status != Status.OK) {
                    Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                            "Failed to secureConnection " + status.toString());
                    main.errorHandler.sendMessage(msg);
                    return;
                }

                sap.startManagement();
                String policy = PolicyHelper.POLICY_BEGIN +
                        PolicyHelper.CA_ACL +
                        PolicyHelper.ACL_ANY +
                        PolicyHelper.ACL_END +
                        PolicyHelper.MEMBER_ACL +
                        PolicyHelper.ACL_ANY +
                        PolicyHelper.ACL_END +
                        PolicyHelper.POLICY_END;

                policy = policy.replace(PolicyHelper.REPLACE_CA,
                        CertificateX509.encodePublicKeyPEM(managerKey.getPublicKey()));
                policy = policy.replace(PolicyHelper.REPLACE_MEMBER,
                        CertificateX509.encodePublicKeyPEM(managerKey.getPublicKey()));
                policy = policy.replace(PolicyHelper.REPLACE_MEMBER_GUID,
                        managerGroupUuid.toString().replace("-", ""));
                sap.updatePolicy(policy);
                sap.endManagement();
                bus.leaveSession(sessionId.value);
            } catch (BusException e) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to installPolicy " + e.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            Message msg = main.errorHandler.obtainMessage(MainActivity.SUCCESS,
                    "installPolicy succeeded");
            main.errorHandler.sendMessage(msg);
        }
    };

    protected Runnable unclaim = new Runnable() {
        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) 101,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to joinSession " + status.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            SecurityApplicationProxy sap = null;
            try {

                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                if (status != Status.OK) {
                    Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                            "Failed to secureConnection " + status.toString());
                    main.errorHandler.sendMessage(msg);
                    return;
                }

                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);
                sap.startManagement();
                sap.reset();

            } catch (BusException e) {
                Message msg = main.errorHandler.obtainMessage(MainActivity.FAIL,
                        "Failed to unclaim " + e.toString());
                main.errorHandler.sendMessage(msg);
                return;
            }

            Message msg = main.errorHandler.obtainMessage(MainActivity.SUCCESS,
                    "Unclaim Succeeded"); 
            main.errorHandler.sendMessage(msg);
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
