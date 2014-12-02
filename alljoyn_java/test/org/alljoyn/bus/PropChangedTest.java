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

    private static final ShortValue SERVICE_PORT = new ShortValue((short) 12345);
    private static final SessionOpts SESSION_OPTS = new SessionOpts();
    private static final String INTERFACE_NAME = "org.alljoyn.test.PropChangedTest";
    private static final String OBJECT_PATH = "/org/alljoyn/test/PropChangedTest";
    private static final String PROP_NOT_SIGNALED = "NotSignaled";

    private static int uniquifier = 0;

    private String serviceName;
    private String peerName;

    private BusAttachment serviceBus;
    private BusAttachment clientBus;

    private int sessionId;

    private final Semaphore sessionSema = new Semaphore(0);

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
        serviceBus = new BusAttachment(getClass().getName() + "_service");
        assertEquals(Status.OK, serviceBus.connect());
        assertEquals(Status.OK, serviceBus.bindSessionPort(SERVICE_PORT, SESSION_OPTS, new MySessionPortListener()));
        serviceName = "test.x" + serviceBus.getGlobalGUIDString() + ".x" + uniquifier++;
        int flags =
            BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING
                | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, serviceBus.requestName(serviceName, flags));
        assertEquals(Status.OK, serviceBus.advertiseName(serviceName, SessionOpts.TRANSPORT_ANY));

        // client
        clientBus = new BusAttachment(getClass().getName() + "_client");
        assertEquals(Status.OK, clientBus.connect());
        clientBus.registerBusListener(new MyBusListener());
        clientBus.findAdvertisedName(serviceName);
    }

    @Override
    public void tearDown()
        throws Exception
    {
        // client
        clientBus.cancelFindAdvertisedName(serviceName);
        clientBus.disconnect();

        // service
        serviceBus.cancelAdvertiseName(serviceName, SessionOpts.TRANSPORT_ANY);
        serviceBus.releaseName(serviceName);
        serviceBus.disconnect();
    }

    // Main test logic

    protected ServiceBusObject obj;
    protected SampleStore store;
    protected ProxyBusObject proxy;

    static public class Range
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

        boolean isIn(int num)
        {
            return ((num >= first) && (num <= last));
        }
    }

    public static Range Pall = new Range(1, 0); // match all (size of 0)
    public static Range P1 = new Range(1, 1);
    public static Range P2 = new Range(2, 2);
    public static Range P1to2 = new Range(1, 2);
    public static Range P1to3 = new Range(1, 3);
    public static Range P1to4 = new Range(1, 4);
    public static Range P2to3 = new Range(2, 3);
    public static Range P3to4 = new Range(3, 4);

    public class InterfaceParameters
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
            BusInterface ann = intf.getAnnotation(BusInterface.class);
            this.name = ann.name();
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

    public class TestParameters
    {
        public boolean newEmit; // use new (true) or old (false) emitter method
        public Range rangePropEmit; // number of properties to emit (newEmit only)
        public List<Range> rangePropListen; // number of properties to listen for on each interface
        public List<Range> rangePropListenExp; // number of properties expected to be received for each interface
        public List<InterfaceParameters> intfParams;

        public TestParameters()
        {
            rangePropListen = new ArrayList<>();
            rangePropListenExp = new ArrayList<>();
            intfParams = new ArrayList<>();
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

    private class SampleStore
    {
        private final List<ProxyBusObject> proxySamples;
        private final HashMap<String, List<Map<String, Variant>>> changedSamples;
        private final HashMap<String, List<String[]>> invalidatedSamples;
        private final Semaphore signalSema;

        public SampleStore()
        {
            proxySamples = new ArrayList<>();
            changedSamples = new HashMap<>();
            invalidatedSamples = new HashMap<>();
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

        void waitForSignals(TestParameters tp)
            throws InterruptedException
        {
            waitForSignals(tp, 0);
        }
    }

    private void validateSignals(TestParameters tp, InterfaceParameters ip)
        throws BusException
    {
        int emitChanged = 0;
        int emitInvalidated = 0;
        int n;

        // ensure correct number of samples
        assertEquals(tp.rangePropListenExp.size(), store.changedSamples.get(ip.name).size());
        assertEquals(tp.rangePropListenExp.size(), store.invalidatedSamples.get(ip.name).size());

        if (ip.emitsChanged == "true") {
            emitChanged = tp.rangePropEmit.size();
        }
        else if (ip.emitsChanged == "invalidates") {
            emitInvalidated = tp.rangePropEmit.size();
        }

        // loop over all samples
        List<Map<String, Variant>> changedSamples = store.changedSamples.get(ip.name);
        List<String[]> invalidatedSamples = store.invalidatedSamples.get(ip.name);
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

    public Set<String> buildPropertyNameList(Range range)
    {
        Set<String> props = new HashSet<String>();
        for (int num = range.first; num <= range.last; num++) {
            props.add("P" + num);
        }
        return props;
    }

    protected void doSetup(TestParameters tp)
        throws BusException
    {
        obj = new ServiceBusObject();
        assertEquals(Status.OK, serviceBus.registerBusObject(obj, OBJECT_PATH));
        // TODO different sessions

        store = new SampleStore();
        // create proxy
        Class<?>[] intf = new Class<?>[tp.intfParams.size()];
        int i = 0;
        for (InterfaceParameters ip : tp.intfParams) {
            intf[i++] = ip.intf;
        }
        proxy = clientBus.getProxyBusObject(peerName, OBJECT_PATH, sessionId, intf);
        assertNotNull(proxy);
        // register listeners (listeners * interfaces)
        for (Range listenRange : tp.rangePropListen) {
            for (InterfaceParameters ip : tp.intfParams) {
                PropertiesChangedListener l = new MyPropertiesChangedListener(store);
                if (listenRange.size() > 0) {
                    Set<String> propSet = buildPropertyNameList(listenRange);
                    String[] prop = propSet.toArray(new String[propSet.size()]);
                    assertEquals(Status.OK, proxy.registerPropertiesChangedListener(ip.name, prop, l));
                }
                else {
                    assertEquals(Status.OK, proxy.registerPropertiesChangedListener(ip.name, new String[] {}, l));
                }
            }
        }
    }

    protected void doTest(TestParameters tp)
        throws BusException, InterruptedException
    {
        waitForSession();
        doSetup(tp);
        Thread.sleep(TIMEOUT_BEFORE_SIGNAL); // otherwise we might miss the signal
        // test
        obj.emitSignals(tp);
        // TODO wait for multiple, move to store?
        store.waitForSignals(tp); // wait for signal(s)
        validateSignals(tp);
    }

    // Listener implementations

    private class MySessionPortListener
        extends SessionPortListener
    {
        @Override
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts)
        {
            return true;
        }
    }

    private class MyBusListener
        extends BusListener
    {
        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix)
        {
            peerName = name;
            assertEquals(Status.OK, clientBus.joinSession(namePrefix, SERVICE_PORT.value, SESSION_OPTS,
                new SessionListener(), new MyOnJoinSessionListener(), null));
        }
    }

    private class MyOnJoinSessionListener
        extends OnJoinSessionListener
    {
        @Override
        public void onJoinSession(Status status, int id, SessionOpts opts, Object context)
        {
            assertEquals(Status.OK, status);
            sessionId = id;
            sessionSema.release();
        }

    }

    private class MyPropertiesChangedListener
        extends PropertiesChangedListener
    {
        private final SampleStore store;

        public MyPropertiesChangedListener(SampleStore store)
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

    private void waitForSession()
    {
        sessionSema.acquireUninterruptibly();
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

    public class ServiceBusObject
        implements InterfaceP1true, InterfaceP1true_bis, InterfaceP1invalidates, InterfaceP1to3true,
        InterfaceP1to4true, InterfaceP1to4invalidates, BusObject
    {
        private final PropertyChangedEmitter emitter;

        public ServiceBusObject()
        {
            // TODO multi-session support needs multiple emitters
            emitter = new PropertyChangedEmitter(this);
        }

        public void emitSignal(TestParameters tp, InterfaceParameters ip)
            throws BusException
        {
            if (tp.newEmit) {
                Set<String> props = buildPropertyNameList(tp.rangePropEmit);
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

        public void emitSignals(TestParameters tp)
            throws BusException
        {
            for (InterfaceParameters ip : tp.intfParams) {
                emitSignal(tp, ip);
            }
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
}
