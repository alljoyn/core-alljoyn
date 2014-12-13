/*
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

package org.alljoyn.bus;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import junit.framework.TestCase;

import org.alljoyn.bus.Mutable.ShortValue;
import org.alljoyn.bus.annotation.BusAnnotation;
import org.alljoyn.bus.annotation.BusAnnotations;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusProperty;

public class PropChangedTest
    extends TestCase
{
    /* client/service synchronization time-out */
    private static final int TIMEOUT = 5000;
    /* time-out before emitting signal */
    private static final int TIMEOUT_BEFORE_SIGNAL = 100;
    /* time-out te be used for tests that expect to time-out */
    private static final int TIMEOUT_EXPECTED = 500;

    private static int UNIQUIFIER = 0;

    private static final ShortValue SERVICE_PORT = new ShortValue((short) 12345);
    private static final SessionOpts SESSION_OPTS = new SessionOpts();
    private static final String INTERFACE_NAME = "org.alljoyn.test.PropChangedTest";
    private static final String OBJECT_PATH = "/org/alljoyn/test/PropChangedTest";
    private static final String PROP_NOT_SIGNALED = "NotSignaled";

    private ServiceBusAttachment mServiceBus;
    private ClientBusAttachment mClientBus;

    private PropChangedTestBusObject mObj;
    private PropChangedTestProxyBusObject mProxy;

    public PropChangedTest(String name)
    {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    @Override
    public void setUp()
        throws Exception
    {
        // service
        mServiceBus = new ServiceBusAttachment(getClass().getName() + "_service");
        mServiceBus.start();

        // client
        mClientBus = new ClientBusAttachment(getClass().getName() + "_client", mServiceBus.getServiceName());
        mClientBus.start();
    }

    @Override
    public void tearDown()
        throws Exception
    {
        if (null != mProxy) {
            mProxy.stop();
            mProxy = null;
        }

        mObj = null;

        // client
        mClientBus.stop();
        mClientBus = null;

        // service
        mServiceBus.stop();
        mServiceBus = null;
    }

    // Bus attachments

    private class ServiceBusAttachment
        extends BusAttachment
    {
        private final String mServiceName;

        public ServiceBusAttachment(String busName)
        {
            super(busName);
            mServiceName = "test.x" + getGlobalGUIDString() + ".x" + UNIQUIFIER++;
        }

        private String getServiceName()
        {
            return mServiceName;
        }

        public void start()
        {
            assertEquals(Status.OK, mServiceBus.connect());
            assertEquals(Status.OK,
                mServiceBus.bindSessionPort(SERVICE_PORT, SESSION_OPTS, new MySessionPortListener()));
            int flags =
                BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING
                    | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
            assertEquals(Status.OK, mServiceBus.requestName(mServiceName, flags));
            assertEquals(Status.OK, mServiceBus.advertiseName(mServiceName, SessionOpts.TRANSPORT_ANY));
        }

        public void stop()
        {
            mServiceBus.cancelAdvertiseName(mServiceName, SessionOpts.TRANSPORT_ANY);
            mServiceBus.releaseName(mServiceName);
            mServiceBus.unbindSessionPort(SERVICE_PORT.value);
            mServiceBus.disconnect();
        }

        private class MySessionPortListener
            extends SessionPortListener
        {
            @Override
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts)
            {
                return true;
            }
        }
    }

    private class ClientBusAttachment
        extends BusAttachment
    {
        private final Semaphore mSessionSema = new Semaphore(0);
        private final String mServiceName;
        private String mPeerName;
        private int mSessionId;
        private BusListener mBusListener;

        public ClientBusAttachment(String busName, String serviceName)
        {
            super(busName);
            mServiceName = serviceName;
        }

        public void start()
        {
            mBusListener = new MyBusListener();
            assertEquals(Status.OK, connect());
            registerBusListener(mBusListener);
            findAdvertisedName(mServiceName);
        }

        public void stop()
        {
            cancelFindAdvertisedName(mServiceName);
            leaveSession(mSessionId);
            unregisterBusListener(mBusListener);
            disconnect();
        }

        public void waitForSession()
        {
            mSessionSema.acquireUninterruptibly();
        }

        public String getPeerName()
        {
            return mPeerName;
        }

        public int getSessionId()
        {
            return mSessionId;
        }

        private class MyBusListener
            extends BusListener
        {
            @Override
            public void foundAdvertisedName(String name, short transport, String namePrefix)
            {
                mPeerName = name;
                assertEquals(
                    Status.OK,
                    joinSession(name, SERVICE_PORT.value, SESSION_OPTS, new SessionListener(),
                        new MyOnJoinSessionListener(), null));
            }
        }

        private class MyOnJoinSessionListener
            extends OnJoinSessionListener
        {
            @Override
            public void onJoinSession(Status status, int id, SessionOpts opts, Object context)
            {
                assertEquals(Status.OK, status);
                mSessionId = id;
                mSessionSema.release();
            }

        }
    }

    // Main test logic

    static private class Range
    {
        public int first;
        public int last;

        Range(int first, int last)
        {
            this.first = first;
            this.last = last;
        }

        int size()
        {
            return last - first + 1;
        }
    }

    private static Range Pall = new Range(1, 0); // match all (size of 0)
    private static Range P1 = new Range(1, 1);
    private static Range P2 = new Range(2, 2);
    private static Range P1to2 = new Range(1, 2);
    private static Range P1to3 = new Range(1, 3);
    private static Range P1to4 = new Range(1, 4);
    private static Range P2to3 = new Range(2, 3);
    private static Range P2to4 = new Range(2, 4);
    private static Range P3to4 = new Range(3, 4);

    private class InterfaceParameters
    {
        Range rangeProp; // number of properties in interface and published
        public String emitsChanged; // value for EmitsChanged annotation
        public String name; // interface name
        Class<?> intf; // interface
        boolean emitsFalse; // add/emit

        public InterfaceParameters(Class<?> intf, Range rangeProp)
        {
            this.intf = intf;
            this.rangeProp = rangeProp;
            this.emitsChanged = "true";
            this.emitsFalse = false;
            this.name = getInterfaceName(intf);
        }

        public InterfaceParameters(Class<?> intf, Range rangeProp, String emitsChanged)
        {
            this(intf, rangeProp);
            this.emitsChanged = emitsChanged;
        }

        public InterfaceParameters(Class<?> intf, Range rangeProp, String emitsChanged, boolean emitsFalse)
        {
            this(intf, rangeProp, emitsChanged);
            this.emitsFalse = emitsFalse;
        }
    }

    private class TestParameters
    {
        public boolean newEmit; // use new (true) or old (false) emitter method
        public Range rangePropEmit; // number of properties to emit (newEmit only)
        public List<Range> rangePropListen; // number of properties to listen for on each interface
        public List<Range> rangePropListenExp; // number of properties expected to be received for each interface
        public List<InterfaceParameters> intfParams;

        public TestParameters()
        {
            rangePropListen = new ArrayList<Range>();
            rangePropListenExp = new ArrayList<Range>();
            intfParams = new ArrayList<InterfaceParameters>();
        }

        public TestParameters(Boolean newEmit, InterfaceParameters ip)
        {
            this();
            this.newEmit = newEmit;
            this.rangePropEmit = ip.rangeProp;
            this.rangePropListen.add(ip.rangeProp);
            this.rangePropListenExp.add(ip.rangeProp); // same as listen
            this.intfParams.add(ip);
        }

        public TestParameters(Boolean newEmit, Range rangePropEmit)
        {
            this();
            this.newEmit = newEmit;
            this.rangePropEmit = rangePropEmit;
        }

        public TestParameters(Boolean newEmit, Range rangePropListen, Range rangePropEmit, InterfaceParameters ip)
        {
            this();
            this.newEmit = newEmit;
            this.rangePropEmit = rangePropEmit;
            this.rangePropListen.add(rangePropListen);
            this.rangePropListenExp.add(rangePropListen); // same as listen
            this.intfParams.add(ip);
        }

        public TestParameters(Boolean newEmit, Range rangePropListen, Range rangePropEmit)
        {
            this();
            this.newEmit = newEmit;
            this.rangePropEmit = rangePropEmit;
            this.rangePropListen.add(rangePropListen);
            this.rangePropListenExp.add(rangePropListen); // same as listen
        }

        TestParameters addInterfaceParameters(InterfaceParameters ip)
        {
            this.intfParams.add(ip);
            return this;
        }

        TestParameters addListener(Range rangePropListen)
        {
            this.rangePropListen.add(rangePropListen);
            this.rangePropListenExp.add(rangePropListen); // same as listen
            return this;
        }

        TestParameters addListener(Range rangePropListen, Range rangePropListenExp)
        {
            this.rangePropListen.add(rangePropListen);
            this.rangePropListenExp.add(rangePropListenExp);
            return this;
        }
    }

    static private class SampleStore
    {
        private final List<ProxyBusObject> proxySamples;
        private final HashMap<String, List<Map<String, Variant>>> changedSamples;
        private final HashMap<String, List<String[]>> invalidatedSamples;
        private final Semaphore signalSema;

        public SampleStore()
        {
            proxySamples = new ArrayList<ProxyBusObject>();
            changedSamples = new HashMap<String, List<Map<String, Variant>>>();
            invalidatedSamples = new HashMap<String, List<String[]>>();
            signalSema = new Semaphore(0);
        }

        public synchronized void addSample(ProxyBusObject proxy, String ifaceName, Map<String, Variant> changed,
            String[] invalidated)
        {
            proxySamples.add(proxy);
            if (null == changedSamples.get(ifaceName)) {
                changedSamples.put(ifaceName, new ArrayList<Map<String, Variant>>());
                invalidatedSamples.put(ifaceName, new ArrayList<String[]>());
            }
            changedSamples.get(ifaceName).add(changed);
            invalidatedSamples.get(ifaceName).add(invalidated);
            signalSema.release();
        }

        public synchronized void clear()
        {
            proxySamples.clear();
            changedSamples.clear();
            invalidatedSamples.clear();
        }

        boolean timedWait(int timeout)
            throws InterruptedException
        {
            return signalSema.tryAcquire(timeout, TimeUnit.MILLISECONDS);
        }

        /*
         * By default we do not expect a time-out when waiting for the signals. If the expectTimeout argument is
         * provided an different from 0 we will wait for that amount of milliseconds and will NOT expect any signals.
         */
        void waitForSignals(TestParameters tp, int expectTimeout)
            throws InterruptedException
        {
            int timeout = (0 != expectTimeout) ? expectTimeout : TIMEOUT;
            boolean expStatus = (0 != expectTimeout) ? false : true;

            /* wait for signals for all interfaces on all listeners */
            int num = tp.intfParams.size() * tp.rangePropListen.size();
            for (int i = 0; i < num; i++) {
                assertEquals(expStatus, timedWait(timeout)); // wait for property changed signal
            }
        }
    }

    private class PropChangedTestListener
        extends PropertiesChangedListener
    {
        private final SampleStore store;

        public PropChangedTestListener(SampleStore store)
        {
            this.store = store;
        }

        @Override
        public void propertiesChanged(ProxyBusObject pObj, String ifaceName, Map<String, Variant> changed,
            String[] invalidated)
        {
            store.addSample(pObj, ifaceName, changed, invalidated);
        }
    }

    private class PropChangedTestProxyBusObject
        extends ProxyBusObject
    {
        private final SampleStore mStore;
        private final HashMap<String, PropChangedTestListener> mListeners;

        public PropChangedTestProxyBusObject(ClientBusAttachment bus, TestParameters tp, Class<?>[] busInterfaces,
            String path)
            throws BusException
        {
            super(bus, bus.getPeerName(), path, bus.getSessionId(), busInterfaces);
            mStore = new SampleStore();
            mListeners = new HashMap<String, PropChangedTestListener>();
            // register listeners (listeners * interfaces)
            for (Range listenRange : tp.rangePropListen) {
                for (InterfaceParameters ip : tp.intfParams) {
                    PropChangedTestListener l = new PropChangedTestListener(mStore);
                    registerListener(l, ip.name, listenRange);
                    mListeners.put(ip.name, l);
                }
            }
        }

        public void stop()
            throws BusException
        {
            for (String intf : mListeners.keySet()) {
                unregisterPropertiesChangedListener(intf, mListeners.get(intf));
            }
        }

        public void registerListener(PropChangedTestListener listener, String ifaceName, Range props)
            throws BusException
        {
            if (props.size() > 0) {
                Set<String> propSet = getPropertyNameList(props);
                String[] prop = propSet.toArray(new String[propSet.size()]);
                assertEquals(Status.OK, registerPropertiesChangedListener(ifaceName, prop, listener));
            }
            else {
                assertEquals(Status.OK, registerPropertiesChangedListener(ifaceName, new String[] {}, listener));
            }
        }

        public void waitForSignals(TestParameters tp, int expectTimeout)
            throws InterruptedException
        {
            mStore.waitForSignals(tp, expectTimeout);
        }

        private void validateSignals(TestParameters tp, InterfaceParameters ip)
            throws BusException
        {
            int emitChanged = 0;
            int emitInvalidated = 0;
            int n;

            // ensure correct number of samples
            List<Map<String, Variant>> changedSamples = mStore.changedSamples.get(ip.name);
            List<String[]> invalidatedSamples = mStore.invalidatedSamples.get(ip.name);
            assertEquals(tp.rangePropListenExp.size(), (null == changedSamples ? 0 : changedSamples.size()));
            assertEquals(tp.rangePropListenExp.size(), (null == invalidatedSamples ? 0 : invalidatedSamples.size()));

            if (ip.emitsChanged == "true") {
                emitChanged = tp.rangePropEmit.size();
            }
            else if (ip.emitsChanged == "invalidates") {
                emitInvalidated = tp.rangePropEmit.size();
            }

            // loop over all samples
            for (int sample = 0; sample < tp.rangePropListenExp.size(); sample++) {
                Map<String, Variant> changed = changedSamples.get(sample);
                String[] invalidated = invalidatedSamples.get(sample);

                n = changed.size();
                int numListen = tp.rangePropListen.get(sample).size();
                if (0 == numListen) {
                    numListen = tp.rangePropEmit.size(); // listen to all
                }
                assertEquals(Math.min(emitChanged, numListen), n);
                for (int i = 0; i < n; i++) {
                    int num = tp.rangePropListenExp.get(sample).first + i;
                    String expName = "P" + num;
                    Variant v = changed.get(expName);
                    assertNotNull(v);
                    assertEquals("i", v.getSignature());
                    int val = v.getObject(Integer.class);
                    assertEquals(num, val);
                }

                n = invalidated.length;
                assertEquals(emitInvalidated, n);
                for (int i = 0; i < n; i++) {
                    int num = tp.rangePropListenExp.get(sample).first + i;
                    String expName = "P" + num;
                    assertTrue(Arrays.asList(invalidated).contains(expName));
                }
            }
        }

        private void validateSignals(TestParameters tp)
            throws BusException
        {
            for (InterfaceParameters ip : tp.intfParams) {
                validateSignals(tp, ip);
            }
        }
    }

    public PropChangedTestProxyBusObject createProxy(ClientBusAttachment bus, TestParameters tp, String path)
        throws BusException
    {
        Class<?>[] intf = new Class<?>[tp.intfParams.size()];
        int i = 0;
        for (InterfaceParameters ip : tp.intfParams) {
            intf[i++] = ip.intf;
        }
        return new PropChangedTestProxyBusObject(bus, tp, intf, path);
    }

    public PropChangedTestProxyBusObject createProxy(ClientBusAttachment bus, TestParameters tp)
        throws BusException
    {
        return createProxy(bus, tp, OBJECT_PATH);
    }

    private String getInterfaceName(Class<?> intf)
    {
        BusInterface ann = intf.getAnnotation(BusInterface.class);
        return ann.name();
    }

    static private Set<String> getPropertyNameList(Range range)
    {
        Set<String> props = new HashSet<String>();
        for (int num = range.first; num <= range.last; num++) {
            props.add("P" + num);
        }
        return props;
    }

    private void doSetup(TestParameters tpService, TestParameters tpClient)
        throws BusException, InterruptedException
    {
        mClientBus.waitForSession();

        mObj = new PropChangedTestBusObject(mServiceBus);
        assertNotNull(mObj);

        // create proxy
        mProxy = createProxy(mClientBus, tpClient);
        assertNotNull(mProxy);

        Thread.sleep(TIMEOUT_BEFORE_SIGNAL); // otherwise we might miss the signal
    }

    private void doTest(TestParameters tpService, TestParameters tpClient, int expectTimeout)
        throws BusException, InterruptedException
    {
        doSetup(tpService, tpClient);
        // test
        mObj.emitSignals(tpService);
        mProxy.waitForSignals(tpClient, expectTimeout); // wait for signal(s)
        if (0 == expectTimeout) {
            mProxy.validateSignals(tpClient);
        }
    }

    private void doTest(TestParameters tpService, TestParameters tpClient)
        throws BusException, InterruptedException
    {
        doTest(tpService, tpClient, 0);
    }

    private void doTest(TestParameters tp)
        throws BusException, InterruptedException
    {
        doTest(tp, tp);
    }

    // Interfaces and service

    @BusInterface(name = INTERFACE_NAME + ".P1true")
    public interface InterfaceP1true
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP1()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "false")})
        public int getNotSignaled()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".P1true_bis")
    public interface InterfaceP1true_bis
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP1()
            throws BusException;

    };

    @BusInterface(name = INTERFACE_NAME + ".P1false")
    public interface InterfaceP1false
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "false")})
        public int getP1()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".P1to4true")
    public interface InterfaceP1to4true
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP1()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP2()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP3()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP4()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "false")})
        public int getNotSignaled()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".P1to2true")
    public interface InterfaceP1to2true
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP1()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP2()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".P1to3true")
    public interface InterfaceP1to3true
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP1()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP2()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP3()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".P1invalidates")
    public interface InterfaceP1invalidates
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal",
            value = "invalidates")})
        public int getP1()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "false")})
        public int getNotSignaled()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".P1to4invalidates")
    public interface InterfaceP1to4invalidates
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal",
            value = "invalidates")})
        public int getP1()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal",
            value = "invalidates")})
        public int getP2()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal",
            value = "invalidates")})
        public int getP3()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal",
            value = "invalidates")})
        public int getP4()
            throws BusException;

        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "false")})
        public int getNotSignaled()
            throws BusException;
    }

    @BusInterface(name = INTERFACE_NAME + ".Invalid")
    public interface InterfaceInvalid
    {
        @BusProperty
        @BusAnnotations({@BusAnnotation(name = "org.freedesktop.DBus.Property.EmitsChangedSignal", value = "true")})
        public int getP1()
            throws BusException;
    }

    private class PropChangedTestBusObject
        implements InterfaceP1true, InterfaceP1true_bis, InterfaceP1invalidates, InterfaceP1false, InterfaceP1to2true,
        InterfaceP1to3true, InterfaceP1to4true, InterfaceP1to4invalidates, BusObject
    {
        private final PropertyChangedEmitter mEmitter;

        public PropChangedTestBusObject(BusAttachment bus, String path)
        {
            assertEquals(Status.OK, bus.registerBusObject(this, path));
            mEmitter = new PropertyChangedEmitter(this);
        }

        public PropChangedTestBusObject(BusAttachment bus)
        {
            this(bus, OBJECT_PATH);
        }

        public void emitSignals(TestParameters tp, InterfaceParameters ip, PropertyChangedEmitter emitter)
            throws BusException
        {
            if (tp.newEmit) {
                Set<String> props = getPropertyNameList(tp.rangePropEmit);
                if (ip.emitsFalse) {
                    props.add(PROP_NOT_SIGNALED);
                }
                emitter.PropertiesChanged(ip.intf, props);
            }
            else {
                for (int num = tp.rangePropEmit.first; num <= tp.rangePropEmit.last; num++) {
                    String name = "P" + num;
                    Variant val = new Variant(num);
                    emitter.PropertyChanged(ip.name, name, val);
                }
            }
        }

        public void emitSignals(TestParameters tp, InterfaceParameters ip)
            throws BusException
        {
            emitSignals(tp, ip, mEmitter);
        }

        public void emitSignals(TestParameters tp)
            throws BusException
        {
            for (InterfaceParameters ip : tp.intfParams) {
                emitSignals(tp, ip);
            }
        }

        public void emitPropChanged(Class<?> iface, Set<String> properties)
            throws BusException
        {
            mEmitter.PropertiesChanged(iface, properties);
        }

        @Override
        public int getP1()
            throws BusException
        {
            return 1;
        }

        @Override
        public int getP2()
            throws BusException
        {
            return 2;
        }

        @Override
        public int getP3()
            throws BusException
        {
            return 3;
        }

        @Override
        public int getP4()
            throws BusException
        {
            return 4;
        }

        @Override
        public int getNotSignaled()
            throws BusException
        {
            assertTrue(false); // should never be called
            return -1;
        }
    }

    // Tests

    /*
     * Functional tests for the newly added EmitPropChanged function for multiple properties (independent of
     * RegisterPropertiesChangedListener). For BusObject containing interfaces created with three different annotations
     * of PropertiesChanged (true, invalidated, false).
     *
     * Note: Property with annotation "false" is part of all tests and validation is done that it is not sent over.
     */

    /*
     * Create a BusObject containing an interface with single property, P1. Invoke newly added EmitPropChanged function
     * for multiple properties to indicate a change to P1. Verify that the signal sent across contains the P1 and its
     * value.
     *
     * Note: Property with annotation "true".
     */
    public void testEmitPropChanged_1()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1, "true", true);
        TestParameters tp = new TestParameters(true, ip);
        doTest(tp);
    }

    /* See above but with annotation "invalidates". */
    public void testEmitPropChanged_2()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1invalidates.class, P1, "invalidates", true);
        TestParameters tp = new TestParameters(true, ip);
        doTest(tp);
    }

    /*
     * Create a BusObject containing an interface with multiple properties, P1, P2, P3, and P4. Invoke newly added
     * EmitPropChanged function for multiple properties to indicate a change to P1, P2, P3 and P4. Verify that the
     * signal sent across contains the P1, P2, P3 and P4.
     *
     * Note: Properties with annotation "true".
     */
    public void testEmitPropChanged_3()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to4true.class, P1to4, "true", true);
        TestParameters tp = new TestParameters(true, ip);
        doTest(tp);
    }

    /* See above but with annotation "invalidates". */
    public void testEmitPropChanged_4()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to4invalidates.class, P1to4, "invalidates", true);
        TestParameters tp = new TestParameters(true, ip);
        doTest(tp);
    }

    /*
     * Functional tests for the newly added RegisterPropertiesChangedListener. For ProxyBusObject created in three
     * different ways (via Introspection, via raw xml, programmatically).
     *
     * Note: using java Interface class only (-> programmatically)
     */

    /*
     * Register a single listener for a property P of interface I. EmitPropChanged existing signal for the single
     * Property P1. Verify that listener is called with P1.
     */
    public void testPropertiesChangedListener_1()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(false, P1, P1, ip);
        doTest(tp);
    }

    /*
     * Register a single listener for a property P of interface I. EmitPropChanged the newly added signal for the
     * multiple properties with Property P1. Verify that listener is called with P1.
     */
    public void testPropertiesChangedListener_2()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, P1, P1, ip);
        doTest(tp);
    }

    /*
     * Run the previous test twice with a clean up of the objects in between. The listeners are not explicitly
     * unregistered.
     */
    public void testPropertiesChangedListener_2_twice()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, P1, P1, ip);
        doTest(tp);
        // destroy everything ...
        mProxy = null;
        mObj = null;
        mClientBus.stop();
        mClientBus = null;
        mServiceBus.stop();
        mServiceBus = null;
        // ... and redo test on different bus attachments
        setUp();
        doTest(tp);
    }

    /*
     * Register two listeners for the same property P of interface I. Emit PropChanged signal for the property P of
     * interface I. Verify that both listeners get called with P.
     */
    public void testPropertiesChangedListener_3()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, P1, P1, ip);
        tp.addListener(P1);
        doTest(tp);
    }

    /*
     * Register a single listener for a property P of interface I. EmitPropChanged for the property P of interface I
     * marked as true PropertiesChanged annotation, changed to value v. Verify that the listener is called with P with
     * v. Note: same as 2nd test above
     */
    public void testPropertiesChangedListener_4()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, P1, P1, ip);
        doTest(tp);
    }

    /*
     * Register a single listener for a property P of interface I. EmitPropChanged for the property P of interface I
     * marked as invalidates PropertiesChanged annotation, changed to value v. Verify that the listener is called with
     * P.
     */
    public void testPropertiesChangedListener_5()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1invalidates.class, P1, "invalidates");
        TestParameters tp = new TestParameters(true, P1, P1, ip);
        doTest(tp);
    }

    /*
     * Register a single listener for specific properties P1, P2 and P3 of interface I. EmitPropChanged for the single
     * Property P1. Verify that listener is called with P1.
     */
    public void testPropertiesChangedListener_6()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to3true.class, P1to3);
        TestParameters tp = new TestParameters(true, P1to3, P1, ip);
        doTest(tp);
    }

    /*
     * Register a single listener for specific properties P1, P2 and P3 of interface I. EmitPropChanged for properties
     * P1, P2 and P3. Verify that listener is called with P1, P2 and P3.
     */
    public void testPropertiesChangedListener_7()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to3true.class, P1to3);
        TestParameters tp = new TestParameters(true, P1to3, P1to3, ip);
        doTest(tp);
    }

    /*
     * Register a single listener for all properties of interface I using NULL as argument. EmitPropChanged for all
     * properties of I. Verify that listener is called with all the properties.
     */
    public void testPropertiesChangedListener_8()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to3true.class, P1to3);
        TestParameters tp = new TestParameters(true, Pall, P1to3, ip);
        doTest(tp);
    }

    /*
     * Register two listeners L1 and L2 for all properties of I1 and I2 respectively. EmitPropChanged for all properties
     * of I1 and I2 (two separate signals). Verify that both listeners get called with all the properties of appropriate
     * interfaces.
     */
    public void testPropertiesChangedListener_9()
        throws Exception
    {
        TestParameters tp = new TestParameters(true, P1, P1);
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true_bis.class, P1));
        doTest(tp);
    }

    /*
     * Register two listeners L1 and L2 for two mutually exclusive halves of properties in I. EmitPropChanged for all
     * properties of I. Verify that both listeners get called with appropriate properties.
     */
    public void testPropertiesChangedListener_10()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to4true.class, P1to4);
        TestParameters tp = new TestParameters(true, P1to2, P1to4, ip);
        tp.addListener(P3to4);
        doTest(tp);
    }

    /*
     * Register listener L1 for properties P1 and P2. Register listener L2 with properties P2 and P3. EmitPropChanged
     * for P2. Verify that both listeners get called with P2.
     */
    public void testPropertiesChangedListener_11()
        throws Exception
    {
        TestParameters tp = new TestParameters(true, P2);
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1to3true.class, P1to3));
        tp.addListener(P1to2, P2);
        tp.addListener(P2to3, P2);
        doTest(tp);
    }

    /*
     * Functional test for partially created ProxyBusObject run the following test (a scenario where for example the
     * BusObject could have 20 different interfaces while the ProxyBusObject only has 1 out of 20):
     *
     * Create a ProxyBusObject with only one of interfaces I1 as compared the full list of interfaces in BusObject.
     * Register listeners L1 for all properties of I1. EmitPropChanged signal for properties in I1. Verify that L1 does
     * get invoked.
     */
    public void testPropertiesChangedListener_PartialProxy()
        throws Exception
    {
        // one interface for client
        TestParameters tpClient = new TestParameters(true, P1, P1);
        tpClient.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        // an extra interface for service
        TestParameters tpService = new TestParameters(true, P1, P1);
        tpService.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        tpService.addInterfaceParameters(new InterfaceParameters(InterfaceP1true_bis.class, P1));

        doTest(tpService, tpClient);
    }

    /*
     * Functional test for the same Listener being registered for different ProxyBusObjects (listener is tied to an
     * interface and a set of properties).
     *
     * Create three different proxy bus objects PB1, PB2 and PB3. PB1 and PB2 are proxies for the same bus object BobA
     * over different session ids (S1 and S2). PB3 is a proxy for a different bus object BobB. Both the BusObjects BobA
     * and BobB implement interface I. Register the same Listener L for all three proxy bus objects for all properties
     * of I. - Emit PropChanged signal from BobA over S1. Verify that L gets invoked with PB1. - Emit PropChanged signal
     * from BobA over S2. Verify that L gets invoked with PB2. - EmitPropChanged signal from BobB. Verify that L gets
     * invoked with PB3.
     *
     * Negative tests also included: - Using the same listener L for all. - Emit PropChanged signal from BobA over S1.
     * Verify that L does NOT get invoked with PB2 and PB3. - Emit PropChanged signal from BobA over S2. Verify that L
     * does NOT get invoked with PB1 and PB3. - EmitPropChanged signal from BobB. Verify that L does NOT get invoked
     * with PB1 and PB2. - Using different listeners L and L2 on S1 and S2 respectively. - Emit a PropChanged signal for
     * P1 only over S1. Verify that listener of PB2 does NOT get called. - and vice versa
     */
    public void testPropertiesChangedListener_MultiSession()
        throws Exception
    {
        TestParameters tp = new TestParameters(true, P1);
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));

        mClientBus.waitForSession(); // S1
        // second session setup

        ClientBusAttachment mClientBus2 =
            new ClientBusAttachment("PropChangedTestClient2", mServiceBus.getServiceName());
        mClientBus2.start();
        mClientBus2.waitForSession(); // S2

        // set up bus objects
        PropChangedTestBusObject bobA = new PropChangedTestBusObject(mServiceBus, OBJECT_PATH + "/BobA");
        assertNotNull(bobA);
        PropChangedTestBusObject bobB = new PropChangedTestBusObject(mServiceBus, OBJECT_PATH + "/BobB");

        // set up signal emitters for S1 and S2
        PropertyChangedEmitter emitS1A = new PropertyChangedEmitter(bobA, mClientBus.getSessionId());
        PropertyChangedEmitter emitS2A = new PropertyChangedEmitter(bobA, mClientBus2.getSessionId());
        PropertyChangedEmitter emitS1B = new PropertyChangedEmitter(bobB, mClientBus.getSessionId());

        // set up proxy bus objects
        PropChangedTestProxyBusObject pb1 = createProxy(mClientBus, tp, OBJECT_PATH + "/BobA");
        PropChangedTestProxyBusObject pb2 = createProxy(mClientBus2, tp, OBJECT_PATH + "/BobA");
        PropChangedTestProxyBusObject pb3 = createProxy(mClientBus, tp, OBJECT_PATH + "/BobB");

        // set up listeners
        SampleStore store = new SampleStore();
        PropChangedTestListener l = new PropChangedTestListener(store);
        pb1.registerListener(l, tp.intfParams.get(0).name, P1);
        pb2.registerListener(l, tp.intfParams.get(0).name, P1);
        pb3.registerListener(l, tp.intfParams.get(0).name, P1);

        PropChangedTestListener l2 = new PropChangedTestListener(store);
        pb2.registerListener(l2, tp.intfParams.get(0).name, P1);

        // test for pb1 (only l)
        store.clear();
        assertEquals(0, store.proxySamples.size());
        bobA.emitSignals(tp, tp.intfParams.get(0), emitS1A);
        Thread.sleep(500);
        assertEquals(1, store.proxySamples.size());
        assertEquals(pb1, store.proxySamples.get(0));

        // test for pb2 (both l and l2)
        store.clear();
        assertEquals(0, store.proxySamples.size());
        bobA.emitSignals(tp, tp.intfParams.get(0), emitS2A);
        Thread.sleep(500);
        assertEquals(2, store.proxySamples.size());
        assertEquals(pb2, store.proxySamples.get(0));
        assertEquals(pb2, store.proxySamples.get(1));

        // test for pb3 (only l)
        store.clear();
        assertEquals(0, store.proxySamples.size());
        bobB.emitSignals(tp, tp.intfParams.get(0), emitS1B);
        Thread.sleep(500);
        assertEquals(1, store.proxySamples.size());
        assertEquals(pb3, store.proxySamples.get(0));

        // clean up
        pb2.unregisterPropertiesChangedListener(tp.intfParams.get(0).name, l2);
        pb3.unregisterPropertiesChangedListener(tp.intfParams.get(0).name, l);
        pb2.unregisterPropertiesChangedListener(tp.intfParams.get(0).name, l);
        pb1.unregisterPropertiesChangedListener(tp.intfParams.get(0).name, l);
    }

    /*
     * The following are the tests that check the return codes of EmitPropChanged.
     *
     * 1. Invoke the newly added EmitPropChanged with NULL as the interface name. ER_OK should not be returned.
     *
     * 2. Invoke the newly added EmitPropChanged with an invalid interface name. ER_OK should not be returned.
     *
     * 3. Invoke the newly added EmitPropChanged with an invalid property name. ER_OK should not be returned.
     *
     * 4. Invoke the newly added EmitPropChanged with a mixture of valid and invalid properties. ER_OK should not be
     * returned.
     *
     * Note: Tests have been adapted to fit the Java API. The wording that is used is still the one from the
     * "Test Approach ASACORE-47" document.
     */
    public void testEmitPropChanged_Negative()
    {
        Exception ex;

        Set<String> okProps = new HashSet<String>(Arrays.asList("P1"));
        Set<String> nokProps = new HashSet<String>(Arrays.asList("P2"));
        Set<String> mixProps = new HashSet<String>(Arrays.asList("P1", "P2"));

        mClientBus.waitForSession();
        mObj = new PropChangedTestBusObject(mServiceBus);

        try {
            ex = null;
            mObj.emitPropChanged(null, okProps); // 1
        }
        catch (Exception e) {
            ex = e;
        }
        assertNotNull(ex);

        try {
            ex = null;
            mObj.emitPropChanged(InterfaceInvalid.class, okProps); // 2
        }
        catch (Exception e) {
            ex = e;
        }
        assertNotNull(ex);

        try {
            ex = null;
            mObj.emitPropChanged(InterfaceP1true.class, nokProps); // 3
        }
        catch (Exception e) {
            ex = e;
        }
        assertNotNull(ex);

        try {
            ex = null;
            mObj.emitPropChanged(InterfaceP1true.class, mixProps); // 4
        }
        catch (Exception e) {
            ex = e;
        }
        assertNotNull(ex);
    }

    /*
     * The following are the tests that check the return codes of RegisterPropertiesChangedListener.
     *
     * 5. Create a ProxyBusObject and invoke RegisterPropertiesChangedListener with NULL as the interface parameter. The
     * return code should be ER_BUS_OBJECT_NO_SUCH_INTERFACE.
     *
     * 6. Create a ProxyBusObject and invoke RegisterPropertiesChangedListener with an invalid string as an interface
     * parameter. The return code should be ER_BUS_OBJECT_NO_SUCH_INTERFACE.
     *
     * 7. Create a ProxyBusObject and invoke RegisterPropertiesChangedListener with a non-existent property. The return
     * code should be ER_BUS_NO_SUCH_PROPERTY.
     *
     * 8. Create a ProxyBusObject and invoke RegisterPropertiesChangedListener with an array of properties that contains
     * a mix of valid properties and invalid / non-existent properties. The return code should be
     * ER_BUS_NO_SUCH_PROPERTY.
     *
     * Note: Tests have been adapted to fit the Java API. The wording that is used is still the one from the
     * "Test Approach ASACORE-47" document.
     */
    public void testRegisterPropertiesChangedListener_Negative()
        throws BusException
    {
        Exception ex;
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, ip);

        String[] okProps = new String[] {"P1"};
        String[] nokProps = new String[] {"P2"};
        String[] mixProps = new String[] {"P1", "P2"};

        mClientBus.waitForSession();
        mObj = new PropChangedTestBusObject(mServiceBus);
        mProxy = createProxy(mClientBus, tp);
        // extra listener for testing
        PropertiesChangedListener listener = new PropChangedTestListener(new SampleStore());

        // 5
        try {
            ex = null;
            mProxy.registerPropertiesChangedListener(null, okProps, listener);
        }
        catch (NullPointerException e) {
            ex = e;
        }
        assertNotNull(ex);

        // 6
        assertEquals(Status.BUS_NO_SUCH_INTERFACE,
            mProxy.registerPropertiesChangedListener(getInterfaceName(InterfaceInvalid.class), okProps, listener));
        // 7
        assertEquals(Status.BUS_NO_SUCH_PROPERTY,
            mProxy.registerPropertiesChangedListener(getInterfaceName(InterfaceP1true.class), nokProps, listener));
        // 8
        assertEquals(Status.BUS_NO_SUCH_PROPERTY,
            mProxy.registerPropertiesChangedListener(getInterfaceName(InterfaceP1true.class), mixProps, listener));
    }

    /*
     * The following are the tests that check the return codes of RegisterPropertiesChangedListener.
     *
     * 9. Create a ProxyBusObject and register a listener. Invoke UnregisterPropertiesChangedListener with NULL as
     * interface parameter. The return code should be ER_BUS_OBJECT_NO_SUCH_INTERFACE.
     *
     * 10. Create a ProxyBusObject and register a listener. Invoke UnregisterPropertiesChangedListener with a
     * non-existent random string as interface parameter. The return code should be ER_BUS_OBJECT_NO_SUCH_INTERFACE.
     *
     * Note: Tests have been adapted to fit the Java API. The wording that is used is still the one from the
     * "Test Approach ASACORE-47" document.
     *
     * This test is DISABLED until ASACORE-1294 is resolved.
     */
    public void DISABLED_testUnregisterPropertiesChangedListener_Negative()
        throws BusException
    {
        Exception ex;
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, ip);

        mClientBus.waitForSession();
        mObj = new PropChangedTestBusObject(mServiceBus);
        mProxy = createProxy(mClientBus, tp);
        PropertiesChangedListener invalidListener = new PropChangedTestListener(new SampleStore());

        // 9
        try {
            ex = null;
            mProxy.unregisterPropertiesChangedListener(null, mProxy.mListeners.get(ip.name));
        }
        catch (NullPointerException e) {
            ex = e;
        }
        assertNotNull(ex);
        // 10
        mProxy.unregisterPropertiesChangedListener(getInterfaceName(InterfaceInvalid.class),
            mProxy.mListeners.get(ip.name));
        // extra
        mProxy.unregisterPropertiesChangedListener(getInterfaceName(InterfaceP1true.class), invalidListener);
    }

    /*
     * Create a ProxyBusObject and register a listener to look for PropertiesChanged for property P1. EmitPropChanged
     * signal for P1. Ensure that listener gets called. Unregister the listener. EmitPropChanged signal for P1. Ensure
     * that the listener does NOT get called.
     */
    public void testListenerNotCalled_1()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1true.class, P1);
        TestParameters tp = new TestParameters(true, ip);

        // test that listener works
        doTest(tp);
        // now unregister
        mProxy.unregisterPropertiesChangedListener(getInterfaceName(InterfaceP1true.class),
            mProxy.mListeners.get(ip.name));
        mProxy.mListeners.remove(ip.name);
        // fire signal again and expect no callback to be called
        mObj.emitSignals(tp);
        assertEquals(false, mProxy.mStore.timedWait(TIMEOUT_EXPECTED));
    }

    /*
     * Create a ProxyBusObject and register a listener to look for PropertiesChanged for property P1. The BusObject
     * contains two properties P1 and P2, where the names of the properties differ by just one character.
     * EmitPropChanged signal for P2. Ensure that listener does NOT get called.
     */
    public void testListenerNotCalled_2()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to2true.class, P1to2);
        TestParameters tp = new TestParameters(true, P1, P2, ip);

        doTest(tp, tp, TIMEOUT_EXPECTED);
    }

    /*
     * Create a ProxyBusObject and register a listener to look for PropertiesChanged for all properties of I1 interface.
     * The BusObject contains two interfaces I1 and I2, where the names of the interfaces differ by just one character,
     * while the names of properties are identical. EmitPropChanged signal for all properties of I2. Ensure that
     * listener does NOT get called.
     */
    public void testListenerNotCalled_3()
        throws Exception
    {
        TestParameters tp = new TestParameters(true, P1);
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true_bis.class, P1));
        tp.addListener(P1);

        doSetup(tp, tp);
        // remove listener for I2
        mProxy.unregisterPropertiesChangedListener(getInterfaceName(InterfaceP1true_bis.class),
            mProxy.mListeners.get(tp.intfParams.get(1).name));
        mProxy.mListeners.remove(tp.intfParams.get(1).name);
        // fire signal for I2 and expect time-out
        mObj.emitSignals(tp, tp.intfParams.get(1));
        assertEquals(false, mProxy.mStore.timedWait(TIMEOUT_EXPECTED));
    }

    /*
     * Create a ProxyBusObject and register a listener to look for PropertiesChanged for property P1. EmitPropChanged
     * signal for P1 where P1 is marked as false with PropertyChanged annotation. Ensure that the listener does NOT get
     * called.
     */
    public void testListenerNotCalled_4()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1false.class, P1, "false");
        TestParameters tp = new TestParameters(true, ip);

        doTest(tp, tp, TIMEOUT_EXPECTED);
    }

    /*
     * Create a ProxyBusObject and register a listener to look for PropertiesChanged for property P1. Emit a PropChanged
     * signal for property P2. Use EmitPropChanged signal for single property. Ensure that the listener does NOT get
     * called.
     */
    public void testListenerNotCalled_5()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to2true.class, P1to2);
        TestParameters tp = new TestParameters(false, P1, P2, ip);

        doTest(tp, tp, TIMEOUT_EXPECTED);
    }

    /*
     * Create a ProxyBusObject and register a listener to look for PropertiesChanged for property P1. Emit a PropChanged
     * signal for properties P2, P3 and P4. Use EmitPropChanged for multiple properties. Ensure that the listener does
     * NOT get called.
     */
    public void testListenerNotCalled_6()
        throws Exception
    {
        InterfaceParameters ip = new InterfaceParameters(InterfaceP1to4true.class, P1to4);
        TestParameters tp = new TestParameters(true, P1, P2to4, ip);

        doTest(tp, tp, TIMEOUT_EXPECTED);
    }

    /*
     * Create a ProxyBusObject and register two listeners, L1 and L2 for properties P1 and P2 respectively. Emit a
     * PropChangedSignal for P1. Ensure that L1 gets called. Ensure that L2 does NOT get called.
     */
    public void testListenerNotCalled_7()
        throws Exception
    {
        TestParameters tp = new TestParameters(true, P1);
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1to2true.class, P1to2));
        tp.addListener(P1);
        tp.addListener(P2);

        doSetup(tp, tp);
        // emit
        mObj.emitSignals(tp);
        // wait for a single signal
        assertEquals(true, mProxy.mStore.timedWait(TIMEOUT));
        assertEquals(false, mProxy.mStore.timedWait(TIMEOUT_EXPECTED));
        // remove L2 from TestParameters because nothing is expected on it
        tp.rangePropListenExp.remove(1);
        // validate that only signal for P1 was seen
        mProxy.validateSignals(tp);
    }

    /*
     * Create a ProxyBusObject and register two listeners, L1 and L2 for the same property P belonging to two different
     * interfaces I1 and I2 respectively. Emit a PropChangedSignal for P of I1. Ensure that L1 gets called and L2 does
     * NOT get called.
     */
    public void testListenerNotCalled_8()
        throws Exception
    {
        TestParameters tp = new TestParameters(true, P1);
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        tp.addInterfaceParameters(new InterfaceParameters(InterfaceP1true_bis.class, P1));
        tp.addListener(P1); // creates a listener per interface

        doSetup(tp, tp);
        // emit
        mObj.emitSignals(tp, tp.intfParams.get(0));
        // wait for a single signal
        assertEquals(true, mProxy.mStore.timedWait(TIMEOUT));
        assertEquals(false, mProxy.mStore.timedWait(TIMEOUT_EXPECTED));
        // validate signal for I1 was seen
        mProxy.validateSignals(tp, tp.intfParams.get(0));
        // nothing expected for I2
        tp.rangePropListenExp.clear();
        mProxy.validateSignals(tp, tp.intfParams.get(1));
    }

    /*
     * Partially created ProxyBusObject scenario, where the ProxyBusObject only has one interface I1 out of the
     * interfaces on BusObject. Register a listener for all properties of I1. Emit a PropChanged signal for properties
     * belonging to another interface I2 (I2 is not even present in ProxyBusObject). Verify that the listener does NOT
     * get called.
     */
    public void testListenerNotCalled_10()
        throws Exception
    {
        // one interface for client
        TestParameters tpClient = new TestParameters(true, P1, P1);
        tpClient.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        // an extra interface for service
        TestParameters tpService = new TestParameters(true, P1, P1);
        tpService.addInterfaceParameters(new InterfaceParameters(InterfaceP1true.class, P1));
        tpService.addInterfaceParameters(new InterfaceParameters(InterfaceP1true_bis.class, P1));

        doSetup(tpService, tpClient);
        // emit on I2
        mObj.emitSignals(tpService, tpService.intfParams.get(1));
        // expect time-out
        assertEquals(false, mProxy.mStore.timedWait(TIMEOUT_EXPECTED));
    }
}
