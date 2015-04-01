/**
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

package org.alljoyn.bus;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import junit.framework.TestCase;

import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;

public class ObserverTest extends TestCase {
    private static final String A = "a";
    private static final String B = "b";
    private static final String C = "c";
    private static final String AB = "ab";
    private static final String TEST_PATH_PREFIX = "/TEST/";
    private static final int WAIT_TIMEOUT = 3000;

    static class SingleLambda extends Lambda {
        private ObserverListener listener;

        public SingleLambda(ObserverListener l) {
            listener = l;
        }

        @Override
        public boolean func() {
            return listener.getCounter() == 0;
        }
    }

    private final ArrayList<Participant> participants = new ArrayList<Participant>();
    private final ArrayList<Observer> observers = new ArrayList<Observer>();
    public boolean failure;

    public ObserverTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class SessionAccepter extends SessionPortListener {
        public boolean accept;
        public HashMap<String, Integer> sessions;

        public SessionAccepter(boolean acc) {
            accept = acc;
            sessions = new HashMap<String, Integer>();
        }

        @Override
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {return accept;}
        @Override
        public void sessionJoined(short port, int id, String joiner) {
            sessions.put(joiner, id);
        }
    }

    public class Participant {
        private String name;
        private AboutDataListener aboutData;
        private AboutObj aboutObj;

        private Map<String, BusObject> objects;
        private Map<String, Boolean> enabled;

        public  BusAttachment bus;
        public SessionAccepter accepter;

        public Participant(String _name) {
            participants.add(this);
            try {
                name = _name;
                objects = new HashMap<String, BusObject>();
                enabled = new HashMap<String, Boolean>();

                bus = new BusAttachment(getClass().getName() + name);
                Status status = bus.connect();
                assertEquals(Status.OK, status);

                SessionOpts sessionOpts = new SessionOpts();
                Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);
                accepter = new SessionAccepter(true);
                assertEquals(Status.OK, bus.bindSessionPort(sessionPort, sessionOpts, accepter));

                aboutObj = new AboutObj(bus);
                aboutData = new MyAboutData();
                aboutObj.announce((short)42, aboutData);
            } catch (Exception e) {
                fail("creation of participant failed: " + e);
            }
        }

        public void createA(String name) {
            String path = makePath(name);
            objects.put(name, new ObjectA(bus, path));
            enabled.put(name, false);
        }

        public void createB(String name) {
            String path = makePath(name);
            objects.put(name, new ObjectB(bus, path));
            enabled.put(name, false);
        }

        public void createAB(String name) {
            String path = makePath(name);
            objects.put(name, new ObjectAB(bus, path));
            enabled.put(name, false);
        }

        public void registerObject(String name) {
            BusObject o = objects.get(name);
            if (o != null) {
                bus.registerBusObject(o, makePath(name));
                enabled.put(name, true);
                aboutObj.announce((short)42, aboutData);
            }
        }

        public void unregisterObject(String name) {
            BusObject o = objects.get(name);
            if (o != null) {
                bus.unregisterBusObject(o);
                enabled.put(name, false);
                aboutObj.announce((short)42, aboutData);
            }
        }

        public void stop() {
            for (String name : objects.keySet()) {
                if (enabled.get(name)) {
                    bus.unregisterBusObject(objects.get(name));
                }
            }
            objects.clear();
            if (aboutObj != null) {
                aboutObj.unannounce();
                aboutObj = null;
            }
        }

        public String getName() {
            return name;
        }
    }

    @BusInterface(name="org.allseen.test.A", announced="true")
    public interface InterfaceA {
        @BusMethod
        String methodA() throws BusException;
    }

    @BusInterface(name="org.allseen.test.B", announced="true")
    public interface InterfaceB {
        @BusMethod
        String methodB() throws BusException;
    }

    public class ObjectA implements InterfaceA, BusObject {
        public String path;
        public BusAttachment bus;

        public ObjectA(BusAttachment _bus, String _path) {
            bus = _bus;
            path = _path;
        }

        @Override
        public String methodA() throws BusException {
            return bus.getUniqueName() + "@" + path;
        }
    }

    public class ObjectB implements InterfaceB, BusObject {
        public String path;
        public BusAttachment bus;

        public ObjectB(BusAttachment _bus, String _path) {
            bus = _bus;
            path = _path;
        }

        @Override
        public String methodB() throws BusException {
            return bus.getUniqueName() + "@" + path;
        }
    }

    public class ObjectAB implements InterfaceA, InterfaceB, BusObject {
        public String path;
        public BusAttachment bus;

        public ObjectAB(BusAttachment _bus, String _path) {
            bus = _bus;
            path = _path;
        }

        @Override
        public String methodA() throws BusException {
            return bus.getUniqueName() + "@" + path;
        }

        @Override
        public String methodB() throws BusException {
            return bus.getUniqueName() + "@" + path;
        }
    }

    public class MyAboutData implements AboutDataListener {

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException {
            Map<String, Variant> aboutData = new HashMap<String, Variant>();
            // nonlocalized values
            aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage", new Variant("en"));
            aboutData.put("DeviceId", new Variant("93c06771-c725-48c2-b1ff-6a2a59d445b8"));
            aboutData.put("ModelNumber", new Variant("A1B2C3"));
            aboutData.put("SupportedLanguages", new Variant(new String[] { "en" }));
            aboutData.put("DateOfManufacture", new Variant("2014-09-23"));
            aboutData.put("SoftwareVersion", new Variant("1.0"));
            aboutData.put("AJSoftwareVersion", new Variant(Version.get()));
            aboutData.put("HardwareVersion", new Variant("0.1alpha"));
            aboutData.put("SupportUrl", new Variant("http://www.example.com/support"));
            aboutData.put("DeviceName", new Variant("A device name"));
            aboutData.put("AppName", new Variant("An application name"));
            aboutData.put("Manufacturer", new Variant("A mighty manufacturing company"));
            aboutData.put("Description",
                          new Variant("Sample showing the about feature in a service application"));
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException {
            Map<String, Variant> aboutData = new HashMap<String, Variant>();
            aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage", new Variant("en"));
            aboutData.put("DeviceName", new Variant("A device name"));
            aboutData.put("DeviceId", new Variant("93c06771-c725-48c2-b1ff-6a2a59d445b8"));
            aboutData.put("AppName", new Variant("An application name"));
            aboutData.put("Manufacturer", new Variant("A mighty manufacturing company"));
            aboutData.put("ModelNumber", new Variant("A1B2C3"));
            return aboutData;
        }

    }

    @Override
    public void tearDown() throws Exception {
        for (Observer observer : observers) {
            observer.close();
        }
        for (Participant participant : participants) {
            participant.stop();
        }
        participants.clear();
        assertFalse(failure);
        System.gc();
        System.gc();
        System.runFinalization();
        System.runFinalization();

        Thread.sleep(100);
    }

    public String makePath(String name) {
        return TEST_PATH_PREFIX + name;
    }

    public static class Lambda {
        public boolean func() { return false; }
    }

    public boolean waitForLambda(long waitMs, Lambda expression) {
        try {
            boolean ret = expression.func();
            long endMs = System.currentTimeMillis() + waitMs;
            while (!failure && !ret && (System.currentTimeMillis() <= endMs)) {
                Thread.sleep(5);
                ret = expression.func();
            }
            assertFalse(failure);
            return ret;
        } catch (Exception e) {
            fail("exception during lambda evaluation");
            return false;
        }
    }

    public int countProxies(Observer obs) {
        ProxyBusObject proxy;
        int count = 0;
        for (proxy = obs.getFirst(); proxy != null; proxy = obs.getNext(proxy)) {
            ++count;
        }
        return count;
    }

    public class ObserverListener implements Observer.Listener {
        private final BusAttachment bus;
        private final AtomicInteger counter;
        private final ArrayList<ProxyBusObject> proxies;
        private boolean allowDuplicates;

        public ObserverListener(Participant p) {
            bus = p.bus;
            counter = new AtomicInteger();
            proxies = new ArrayList<ProxyBusObject>();
        }

        public final int getCounter() {
            return counter.get();
        }

        @Override
        public void objectDiscovered(ProxyBusObject proxy) {
            boolean found = proxies.contains(proxy);
            if (found && !allowDuplicates) {
                failure = true;
                assertFalse(found);
            }
            proxies.add(proxy);
            checkReentrancy(proxy);
            counter.decrementAndGet();
        }


        @Override
        public void objectLost(ProxyBusObject proxy) {
            boolean found = proxies.contains(proxy);
            assertTrue(found);
            proxies.remove(proxy);
            counter.decrementAndGet();
        }

        public void expectInvocations(int n) {
            /* check previous invocation count */
            assertEquals(0, counter.get());
            counter.set(n);
        }

        private void checkReentrancy(ProxyBusObject proxy) {
            try {
                /* does the proxy implement InterfaceB? */
                InterfaceB intf = proxy.getInterface(InterfaceB.class);
                try {
                    String id = intf.methodB();
                    assertEquals(proxy.getBusName() + "@" + proxy.getObjPath(), id);
                } catch (BusException e) {
                    // OK we might get ER_BUS_BLOCKING_CALL_NOT_ALLOWED here
                    if (!e.getMessage().equals("ER_BUS_BLOCKING_CALL_NOT_ALLOWED")) {
                        e.printStackTrace();
                        fail("recieved an unexpected BusException.");
                    }
                }
                bus.enableConcurrentCallbacks();
                try {
                    String id = intf.methodB();
                    assertEquals(proxy.getBusName() + "@" + proxy.getObjPath(), id);
                } catch (BusException e) {
                    fail("method invocation failed: " + e);
                }
            } catch (ClassCastException cce) {
                /* no it doesn't -> then it must implement InterfaceA, right? */
                InterfaceA intf = proxy.getInterface(InterfaceA.class);
                try {
                    String id = intf.methodA();
                    assertEquals(proxy.getBusName() + "@" + proxy.getObjPath(), id);
                } catch (BusException e) {
                    // OK we might get ER_BUS_BLOCKING_CALL_NOT_ALLOWED here
                    if (!e.getMessage().equals("ER_BUS_BLOCKING_CALL_NOT_ALLOWED")) {
                        e.printStackTrace();
                        fail("recieved an unexpected BusException.");
                    }
                }
                bus.enableConcurrentCallbacks();
                try {
                    String id = intf.methodA();
                    assertEquals(proxy.getBusName() + "@" + proxy.getObjPath(), id);
                } catch (BusException e) {
                    fail("method invocation failed: " + e);
                }
            }
        }
    }

    public void simpleScenario(Participant provider, Participant consumer) {
        provider.createA("justA");
        provider.createB("justB");
        provider.createAB("both");

        final Observer obsA = newObserver(consumer, InterfaceA.class);
        final ObserverListener listenerA = new ObserverListener(consumer);
        obsA.registerListener(listenerA);
        final Observer obsB = newObserver(consumer, InterfaceB.class);
        final ObserverListener listenerB = new ObserverListener(consumer);
        obsB.registerListener(listenerB);
        final Observer obsAB = newObserver(consumer, InterfaceA.class,
                InterfaceB.class);
        final ObserverListener listenerAB = new ObserverListener(consumer);
        obsAB.registerListener(listenerAB);

        Lambda allDone = new Lambda() {
            @Override
            public boolean func() {
                return listenerA.getCounter() == 0
                        && listenerB.getCounter() == 0
                        && listenerAB.getCounter() == 0;
            }
        };
        /* let provider publish objects on the bus */
        listenerA.expectInvocations(2);
        listenerB.expectInvocations(2);
        listenerAB.expectInvocations(1);

        provider.registerObject("justA");
        provider.registerObject("justB");
        provider.registerObject("both");
        assertTrue(waitForLambda(3000, allDone));

        /* remove justA from the bus */
        listenerA.expectInvocations(1);
        listenerB.expectInvocations(0);
        listenerAB.expectInvocations(0);

        provider.unregisterObject("justA");
        assertTrue(waitForLambda(3000, allDone));

        /* remove "both" from the bus */
        listenerA.expectInvocations(1);
        listenerB.expectInvocations(1);
        listenerAB.expectInvocations(1);

        provider.unregisterObject("both");
        assertTrue(waitForLambda(3000, allDone));

        /* count the number of proxies left in the Observers.
         * There should be 0 in A, 1 in B, 0 in AB */
        assertEquals(0, countProxies(obsA));
        assertEquals(1, countProxies(obsB));
        assertEquals(0, countProxies(obsAB));

        /* remove all listeners */
        obsA.unregisterAllListeners();
        obsB.unregisterAllListeners();
        obsAB.unregisterListener(listenerAB);

        /* remove "justB" and reinstate the other objects */
        listenerA.expectInvocations(0);
        listenerB.expectInvocations(0);
        listenerAB.expectInvocations(0);
        provider.unregisterObject("justB");
        provider.registerObject("justA");
        provider.registerObject("both");

        /* busy-wait for a second at most */
        assertTrue(waitForLambda(1000, new Lambda() {
            @Override
            public boolean func() {
                return 2 == countProxies(obsA) &&
                        1 == countProxies(obsB) &&
                        1 == countProxies(obsAB);
            }}));

        /* reinstate listeners & test triggerOnExisting functionality */
        listenerA.expectInvocations(2);
        listenerB.expectInvocations(1);
        listenerAB.expectInvocations(1);
        obsA.registerListener(listenerA);
        obsB.registerListener(listenerB);
        obsAB.registerListener(listenerAB);

        assertTrue(waitForLambda(3000, allDone));

        /* test multiple listeners for same observer */
        final ObserverListener listenerB2 = new ObserverListener(consumer);
        listenerB2.expectInvocations(0);
        obsB.registerListener(listenerB2, false);

        Lambda allDone2 = new Lambda() {
            @Override
            public boolean func() {
                return listenerA.getCounter() == 0
                        && listenerB.getCounter() == 0
                        && listenerB2.getCounter() == 0
                        && listenerAB.getCounter() == 0;
            }
        };

        listenerA.expectInvocations(0);
        listenerB.expectInvocations(1);
        listenerB2.expectInvocations(1);
        listenerAB.expectInvocations(0);
        provider.registerObject("justB");
        assertTrue(waitForLambda(1000, allDone2));

        /* are all objects back where they belong? */
        assertEquals(2, countProxies(obsA));
        assertEquals(2, countProxies(obsB));
        assertEquals(1, countProxies(obsAB));

        /* test multiple observers for the same set of interfaces */
        final Observer obsB2 = newObserver(consumer, InterfaceB.class);
        obsB.unregisterListener(listenerB2); /* unregister listenerB2 from obsB so we can reuse it here */
        listenerA.expectInvocations(0);
        listenerB.expectInvocations(0);
        listenerB2.expectInvocations(2);
        listenerAB.expectInvocations(0);
        obsB2.registerListener(listenerB2);
        assertTrue(waitForLambda(1000, allDone2));

        /* test Observer::Get() and the proxy creation functionality */
        ProxyBusObject proxy;
        proxy = obsA.get(provider.bus.getUniqueName(), makePath("justA"));
        assertTrue(null != proxy);

        proxy = obsA.get(provider.bus.getUniqueName(), makePath("both"));
        assertTrue(null != proxy);

        /* verify that we can indeed perform method calls */
        InterfaceA intfA = proxy.getInterface(InterfaceA.class);
        try {
            String id = intfA.methodA();
            assertEquals(provider.bus.getUniqueName() + "@" + makePath("both"), id);
        } catch (BusException e) {
            fail("method call failed: " + e);
        }
    }

    public void testClose() throws Exception {
        Participant provider = new Participant("prov");
        Participant consumer = new Participant("cons");
        final Observer obs = newObserver(consumer, InterfaceA.class);
        final ObserverListener listener = new ObserverListener(consumer);
        obs.registerListener(listener);
        provider.createA(A);
        waitForEvent(listener, provider, A);

        ProxyBusObject pbo = obs.getFirst();
        assertNotNull(pbo);

        obs.close();
        obs.close();
        obs.registerListener(listener);
        assertNull(obs.getFirst());
        assertNull(obs.get(pbo.getBusName(), pbo.getObjPath()));
        assertNull(obs.getNext(pbo));
        obs.unregisterListener(listener);
        obs.unregisterAllListeners();
        obs.close();
        Thread.sleep(100);
        listener.expectInvocations(0);
    }

    public void testSimple() {
        Participant provider = new Participant("prov");
        Participant consumer = new Participant("cons");
        simpleScenario(provider, consumer);
    }

    public void testSimpleSelf() {
        Participant both = new Participant("both");
        simpleScenario(both, both);
    }

    public void testRejection() throws Exception {
        Participant willing = new Participant("willing");
        Participant doubtful = new Participant("doubtful");
        Participant unwilling = new Participant("unwilling");
        Participant consumer = new Participant("consumer");

        willing.createA(A);
        doubtful.createAB(A);
        unwilling.createAB(A);

        unwilling.accepter.accept = false;

        final ObserverListener listener = new ObserverListener(consumer);
        final Observer obs = newObserver(consumer, InterfaceA.class);
        obs.registerListener(listener);

        listener.expectInvocations(2);
        willing.registerObject(A);
        doubtful.registerObject(A);
        unwilling.registerObject(A);

        Lambda ok = new SingleLambda(listener);
        assertTrue(waitForLambda(3000, ok));

        /* now let doubtful kill the connection */
        Thread.sleep(100); // This sleep is necessary to make sure the provider knows it has a session.
        listener.expectInvocations(1);

        Integer sessionid = doubtful.accepter.sessions.get(consumer.bus.getUniqueName());
        assertNotNull(sessionid);
        doubtful.bus.leaveHostedSession(sessionid);

        assertTrue(waitForLambda(3000, ok));

        /* there should only be one object left */
        assertEquals(1, countProxies(obs));

        /* unannounce and reannounce, connection should be restored */
        listener.expectInvocations(1);
        doubtful.unregisterObject(A);
        doubtful.registerObject(A);
        assertTrue(waitForLambda(3000, ok));

        /* now there should only be two objects */
        assertEquals(2, countProxies(obs));
    }

    public void testInvalidArgs() throws Exception {
        Participant test = new Participant("invArgs");
        Observer obs = null;
        /* null bus pointer is invalid expect a NullPointerException */
        try {
            obs = newObserver(null, InterfaceA.class);
            fail("bus can't be null");
        } catch (NullPointerException rt) { /* OK */
            assertNull(obs);
        }
        /* must pass in a valid interface Class<?>[] array */
        try {
            obs = new Observer(test.bus, null);
            fail("null array value is not allowed");
        } catch (NullPointerException rt) { /* OK */
            assertNull(obs);
        }
        /* array can not contain a null value. */
        try {
            obs = newObserver(test, new Class<?>[] { null });
            fail("Array with null value is not allowed");
        } catch (NullPointerException rt) { /* OK */
            assertNull(obs);
        }
        /* empty array is not allowed */
        try {
            obs = newObserver(test);
            fail("Empty array is not allowed");
        } catch (IllegalArgumentException iae) { /* OK */
            assertNull(obs);
        }
        /* class must be a BusInterface */
        try {
            obs = newObserver(test, String.class);
            fail("Class is not allowed");
        } catch (IllegalArgumentException iae) { /* OK */
            assertNull(obs);
        }

        /* all classes must be BusInterfaces */
        try {
            obs = new Observer(test.bus, new Class<?>[] { InterfaceA.class },
                    new Class<?>[] { String.class });
            fail("Class is not allowed");
        } catch (IllegalArgumentException iae) { /* OK */
            assertNull(obs);
        }
        assertNull(obs);
        obs = new Observer(test.bus, new Class<?>[] { InterfaceA.class }, null); // null
        // allowed.

        try {
            obs.registerListener(null);
            fail("null not allowed");
        } catch (IllegalArgumentException rt) { /* OK */
            assertNotNull(obs);
        }

        final ObserverListener listener = new ObserverListener(test);
        obs.registerListener(listener);
        listener.expectInvocations(1);
        test.createA(A);
        test.registerObject(A);
        assertTrue(waitForLambda(3000, new SingleLambda(listener)));

        // null values are harmless for the object.
        try {
            obs.get(null, A);
            fail("Expected Observer.get(null, path) to throw a NullPointerException.");
        } catch (NullPointerException npe) { /* OK */
            assertNotNull(obs);
        }
        try {
            obs.get(test.bus.getUniqueName(), null);
            fail("Expected Observer.get(bus, null) to throw a NullPointerException.");
        } catch (NullPointerException npe) { /* OK */
            assertNotNull(obs);
        }
        assertNull(obs.get(test.bus.getUniqueName(), AB));
        assertNull(obs.get(A, A));
        assertNotNull(obs.get(test.bus.getUniqueName(), makePath(A)));


        Method finalize = obs.getClass().getDeclaredMethod("finalize");
        finalize.setAccessible(true);
        finalize.invoke(obs);
        finalize.invoke(obs);
        finalize.invoke(obs);
    }

    final AtomicInteger exceptionCount = new AtomicInteger();

    public void testExceptionInListener() {
        Participant consumer = new Participant("one");
        Participant provider = new Participant("two");

        final Observer obs = newObserver(consumer, InterfaceA.class);
        final ObserverListener listener = new ObserverListener(consumer);
        final ObserverListener lateJoiner = new ObserverListener(consumer);
        final Observer.Listener badListener = new Observer.Listener() {
            @Override
            public void objectLost(ProxyBusObject object) {
                exceptionCount.incrementAndGet();
                throw new RuntimeException();
            }

            @Override
            public void objectDiscovered(ProxyBusObject object) {
                exceptionCount.incrementAndGet();
                throw new RuntimeException();
            }
        };
        obs.registerListener(badListener);
        obs.registerListener(listener);
        provider.createA(A);
        waitForEvent(listener, provider, A);
        obs.registerListener(badListener, false);
        obs.registerListener(badListener, true);
        lateJoiner.expectInvocations(1);
        obs.registerListener(lateJoiner, true);
        waitForLambda(WAIT_TIMEOUT, new SingleLambda(lateJoiner));
        waitForEvent(listener, provider, A, false);
        assertTrue(waitForLambda(WAIT_TIMEOUT, new Lambda() {
            @Override
            public boolean func() {

                return 5 == exceptionCount.get();
            }
        }));
        assertEquals(5, exceptionCount.get());
    }

    public void testListenTwice() {
        /* reuse the same listener for two observers */
        Participant provider = new Participant("prov");
        Participant consumer = new Participant("cons");

        provider.createA(A);
        provider.createAB(AB);
        provider.createAB("ab2");

        final ObserverListener listener = new ObserverListener(consumer);
        final Observer obs = newObserver(consumer, InterfaceA.class);
        obs.registerListener(listener);
        final Observer obs2 = newObserver(consumer, InterfaceA.class);
        obs2.registerListener(listener);

        Lambda ok = new SingleLambda(listener);

        /* use listener for 2 observers, so we expect to
         * see all events twice */
        listener.expectInvocations(6);
        provider.registerObject(A);
        provider.registerObject(AB);
        provider.registerObject("ab2");

        assertTrue(waitForLambda(3000, ok));

        obs2.unregisterListener(listener);
        /* one observer is gone, so we expect to see
         * every event just once. */
        listener.expectInvocations(3);
        provider.unregisterObject(A);
        provider.unregisterObject(AB);
        provider.unregisterObject("ab2");

        assertTrue(waitForLambda(3000, ok));
    }

    public void testListenTwiceOnSameObserver() throws Exception {
        /* reuse the same listener for two observers */
        Participant provider = new Participant("prov");
        Participant consumer = new Participant("cons");

        provider.createA(A);

        final ObserverListener listener = new ObserverListener(consumer);
        final Observer obs = newObserver(consumer, InterfaceA.class);
        obs.registerListener(listener);
        Lambda ok = new SingleLambda(listener);
        /*
         * use listener twice on observer, so we expect to see all events twice
         */
        listener.expectInvocations(1);
        provider.registerObject(A);

        assertTrue(waitForLambda(3000, ok));

        /*
         * one observer is gone, so we expect to see every event just once.
         */
        listener.allowDuplicates = true;
        listener.expectInvocations(1);
        obs.registerListener(listener, true);

        boolean result = waitForLambda(3000, ok);
        assertTrue("Counter value = " + listener.getCounter(), result);

        listener.expectInvocations(2);
        provider.createAB(AB);
        provider.registerObject(AB);
        result = waitForLambda(3000, ok);
        assertTrue("Counter value = " + listener.getCounter(), result);
        obs.unregisterListener(listener);
        listener.expectInvocations(1);
        provider.unregisterObject(AB);
        assertTrue(waitForLambda(3000, ok));
        obs.unregisterListener(listener);
        provider.unregisterObject(A);
        Thread.sleep(100);
        listener.expectInvocations(0);

        obs.unregisterListener(listener); // the listener is not registered
        // anymore, but this should work.
        obs.unregisterListener(null); // Does nothing sensible, but should not
        // trigger an exception.

        obs.registerListener(listener);
        obs.registerListener(new ObserverListener(consumer));

        // We should be able to unregisterAll multiple times
        obs.unregisterAllListeners();
        obs.unregisterAllListeners();
        obs.unregisterAllListeners();
        obs.unregisterAllListeners();
        obs.unregisterAllListeners();
    }

    public void testMultipleListenersOnSingleObserver() {
        Participant one = new Participant("one");
        Participant two = new Participant("two");

        final Observer obsA = newObserver(one, InterfaceA.class);
        final ObserverListener lisAone = new ObserverListener(one);
        final ObserverListener lisAtwo = new ObserverListener(one);

        obsA.registerListener(lisAone);
        obsA.registerListener(lisAtwo);
        lisAone.expectInvocations(2);
        lisAtwo.expectInvocations(2);

        two.createA(A);
        two.createAB(AB);
        two.registerObject(A);
        two.registerObject(AB);

        Lambda ok = new Lambda() {
            @Override
            public boolean func() {
                return lisAone.getCounter() == 0 && lisAtwo.getCounter() == 0;
            }
        };
        boolean result = waitForLambda(3000, ok);
        assertTrue("Count one =  " + lisAone.getCounter() + ", two = "
                + lisAtwo.getCounter(), result);
    }

    public void testNativeCreate() throws Exception {
        final Participant consumer = new Participant("one");
        final Observer obs = newObserver(consumer, InterfaceA.class);
        Method create = obs.getClass().getDeclaredMethod("create",
                                                         BusAttachment.class, String[].class);
        create.setAccessible(true);
        try {
            create.invoke(obs, null, new String[0]);
            fail("Expected call to throw InvocationTargetException.");
        } catch (InvocationTargetException npe) {
            assertTrue(npe.getCause() instanceof NullPointerException);
        }
        try {
            create.invoke(obs, consumer.bus, null);
            fail("Expected call to throw InvocationTargetException.");
        } catch (InvocationTargetException npe) {
            assertTrue(npe.getCause() instanceof NullPointerException);
        }
        try {
            create.invoke(obs, consumer.bus, new String[1]);
            fail("Expected call to throw InvocationTargetException.");
        } catch (InvocationTargetException npe) {
            assertTrue(npe.getCause() instanceof NullPointerException);
        }
    }

    public void testGetFirstNext() {
        Participant consumer = new Participant("one");
        Participant provider = new Participant("two");

        final Observer obs = newObserver(consumer, InterfaceA.class);
        final ObserverListener listener = new ObserverListener(consumer);
        obs.registerListener(listener);

        assertNull(obs.getFirst());
        assertNull(obs.getNext(null));
        ArrayList<ProxyBusObject> objects = null;

        String[] names = new String[] { A, AB, B, C };
        for (int i = 0; i < names.length;) {
            provider.createA(names[i]);
            waitForEvent(listener, provider, names[i]);
            objects = checkObjects(++i, obs);
        }
        assertNotNull(objects);
        ProxyBusObject obj = objects.get(1);
        unregisterObject(provider, obj, listener);
        assertSame(objects.get(2), obs.getNext(obj));
        assertSame(objects.get(2), obs.getNext(objects.get(0)));
        objects = checkObjects(3, obs);
        assertFalse(objects.contains(obj));

        obj = objects.get(2);
        unregisterObject(provider, obj, listener);
        assertNull(obs.getNext(obj));
        assertNull(obs.getNext(objects.get(1)));
        objects = checkObjects(2, obs);
        assertFalse(objects.contains(obj));

        obj = objects.get(0);
        unregisterObject(provider, obj, listener);
        assertSame(objects.get(1), obs.getNext(obj));
        assertSame(objects.get(1), obs.getFirst());
        objects = checkObjects(1, obs);
        assertFalse(objects.contains(obj));

        obj = objects.get(0);
        unregisterObject(provider, obj, listener);
        assertNull(obs.getNext(obj));
        assertNull(obs.getFirst());
    }

    private void unregisterObject(Participant provider, ProxyBusObject obj,
            ObserverListener listener) {
        waitForEvent(listener, provider,
                obj.getObjPath().substring(TEST_PATH_PREFIX.length()), false);
    }

    private ArrayList<ProxyBusObject> checkObjects(int nrOfObjects, Observer obs) {
        ArrayList<ProxyBusObject> objects = new ArrayList<ProxyBusObject>();
        objects.add(obs.getFirst());
        ProxyBusObject obj = obs.getFirst();
        checkGet(obj, obs);
        assertNotNull(obj);
        assertSame(objects.get(0), obj);
        for (int i = 1; i < nrOfObjects; i++) {
            obj = obs.getNext(objects.get(i - 1));
            assertNotNull(obj);
            assertSame(obj, obs.getNext(objects.get(i - 1)));
            assertFalse(objects.contains(obj));
            checkGet(obj, obs);
            objects.add(obj);
        }
        assertNull(obs.getNext(obj));
        assertNull(obs.getNext(obj));
        return objects;
    }

    private void checkGet(ProxyBusObject obj, Observer obs) {
        assertSame(obj, obs.get(obj.getBusName(), obj.getObjPath()));
    }

    private void waitForEvent(ObserverListener listener, Participant provider,
            String name) {
        waitForEvent(listener, provider, name, true);
    }

    private void waitForEvent(ObserverListener listener, Participant provider,
            String name, boolean register) {
        listener.expectInvocations(1);
        if (register) {
            provider.registerObject(name);
        } else {
            provider.unregisterObject(name);
        }
        Lambda ok = new SingleLambda(listener);
        assertTrue(waitForLambda(WAIT_TIMEOUT, ok));
    }

    public void testMulti() {
        /* multiple providers, multiple consumers */
        Participant one = new Participant("one");
        Participant two = new Participant("two");

        one.createA(A);
        one.createB(B);
        one.createAB(AB);
        two.createA(A);
        two.createB(B);
        two.createAB(AB);

        final Observer obsAone = newObserver(one, InterfaceA.class);
        final ObserverListener lisAone = new ObserverListener(one);
        obsAone.registerListener(lisAone);
        final Observer obsBone = newObserver(one, InterfaceB.class);
        final ObserverListener lisBone = new ObserverListener(one);
        obsBone.registerListener(lisBone);
        final Observer obsABone = newObserver(one, InterfaceA.class,
                InterfaceB.class);
        final ObserverListener lisABone = new ObserverListener(one);
        obsABone.registerListener(lisABone);

        final Observer obsAtwo = newObserver(two, InterfaceA.class);
        final ObserverListener lisAtwo = new ObserverListener(two);
        obsAtwo.registerListener(lisAtwo);
        final Observer obsBtwo = newObserver(two, InterfaceB.class);
        final ObserverListener lisBtwo = new ObserverListener(two);
        obsBtwo.registerListener(lisBtwo);
        final Observer obsABtwo = newObserver(two, InterfaceA.class,
                InterfaceB.class);
        final ObserverListener lisABtwo = new ObserverListener(two);
        obsABtwo.registerListener(lisABtwo);


        Lambda ok = new Lambda () {
            @Override
            public boolean func() {
                return lisAone.getCounter() == 0 && lisBone.getCounter() == 0
                        && lisABone.getCounter() == 0
                        && lisAtwo.getCounter() == 0
                        && lisBtwo.getCounter() == 0
                        && lisABtwo.getCounter() == 0;
            }
        };

        /* put objects on the bus */
        lisAone.expectInvocations(4);
        lisBone.expectInvocations(4);
        lisABone.expectInvocations(2);
        lisAtwo.expectInvocations(4);
        lisBtwo.expectInvocations(4);
        lisABtwo.expectInvocations(2);

        one.registerObject(A);
        one.registerObject(B);
        one.registerObject(AB);
        two.registerObject(A);
        two.registerObject(B);
        two.registerObject(AB);

        assertTrue(waitForLambda(3000, ok));
        assertEquals(4, countProxies(obsAone));
        assertEquals(4, countProxies(obsBone));
        assertEquals(2, countProxies(obsABone));
        assertEquals(4, countProxies(obsAtwo));
        assertEquals(4, countProxies(obsBtwo));
        assertEquals(2, countProxies(obsABtwo));

        /* now drop all objects */
        lisAone.expectInvocations(4);
        lisBone.expectInvocations(4);
        lisABone.expectInvocations(2);
        lisAtwo.expectInvocations(4);
        lisBtwo.expectInvocations(4);
        lisABtwo.expectInvocations(2);

        one.unregisterObject(A);
        one.unregisterObject(B);
        one.unregisterObject(AB);
        two.unregisterObject(A);
        two.unregisterObject(B);
        two.unregisterObject(AB);

        assertTrue(waitForLambda(3000, ok));
        assertEquals(0, countProxies(obsAone));
        assertEquals(0, countProxies(obsBone));
        assertEquals(0, countProxies(obsABone));
        assertEquals(0, countProxies(obsAtwo));
        assertEquals(0, countProxies(obsBtwo));
        assertEquals(0, countProxies(obsABtwo));
    }

    private Observer newObserver(Participant p, Class<?>... classes) {
        Observer obs = new Observer(p.bus, classes);
        observers.add(obs);
        return obs;
    }
}
