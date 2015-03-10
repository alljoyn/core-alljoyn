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

import java.util.Map;
import java.util.HashMap;
import java.util.UUID;
import java.util.ArrayList;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Observer;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.AboutDataListener;
import org.alljoyn.bus.AboutObj;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.Version;

import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;

import junit.framework.TestCase;

public class ObserverTest extends TestCase {
    public ObserverTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private int uniquifier = 0;
    private String genUniqueName(BusAttachment bus) {
        return "test.x" + bus.getGlobalGUIDString() + ".x" + uniquifier++;
    }

    private BusAttachment bus;

    public class SessionAccepter extends SessionPortListener {
        public boolean accept;
        public HashMap<String, Integer> sessions;

        public SessionAccepter(boolean acc) {
            accept = acc;
            sessions = new HashMap<String, Integer>();
        }

        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {return accept;}
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
            aboutObj.unannounce();
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

        public String methodA() throws BusException {
            return bus.getUniqueName() + "@" + path;
        }

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
            aboutData.put("DefaultLanguage", new Variant(new String("en")));
            aboutData.put("DeviceId", new Variant(new String(
                            "93c06771-c725-48c2-b1ff-6a2a59d445b8")));
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] { "en" }));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(Version.get()));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            aboutData.put("SupportUrl", new Variant(new String(
                            "http://www.example.com/support")));
            aboutData.put("DeviceName", new Variant(new String("A device name")));
            aboutData.put("AppName", new Variant(new String("An application name")));
            aboutData.put("Manufacturer", new Variant(new String(
                            "A mighty manufacturing company")));
            aboutData.put("Description",
                    new Variant( new String("Sample showing the about feature in a service application")));
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException {
            Map<String, Variant> aboutData = new HashMap<String, Variant>();
            aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage", new Variant(new String("en")));
            aboutData.put("DeviceName", new Variant(new String("A device name")));
            aboutData.put("DeviceId", new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
            aboutData.put("AppName", new Variant( new String("An application name")));
            aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            return aboutData;
        }

    }
    public void setUp() throws Exception {
    }

    public void tearDown() throws Exception {
    }

    public String makePath(String name) {
        return "/TEST/" + name;
     }

    public class Lambda {
        public boolean func() { return false; }
    }

    public boolean waitForLambda(long waitMs, Lambda expression) {
        try {
            boolean ret = expression.func();
            long endMs = System.currentTimeMillis() + waitMs;
            while (!ret && (System.currentTimeMillis() <= endMs)) {
                Thread.sleep(5);
                ret = expression.func();
            }
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
        private BusAttachment bus;
        public int counter;
        private ArrayList<ProxyBusObject> proxies;

        public ObserverListener(BusAttachment b) {
            bus = b;
            counter = 0;
            proxies = new ArrayList<ProxyBusObject>();
        }

        public void objectDiscovered(ProxyBusObject proxy) {
            boolean found = proxies.contains(proxy);
            assertFalse(found);
            proxies.add(proxy);
            checkReentrancy(proxy);
            --counter;
        }

        public void objectLost(ProxyBusObject proxy) {
            boolean found = proxies.contains(proxy);
            assertTrue(found);
            proxies.remove(proxy);
            --counter;
        }

        public void expectInvocations(int n) {
            /* check previous invocation count */
            assertEquals(0, counter);
            counter = n;
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

        final Observer obsA = new Observer(consumer.bus, new Class<?>[] {InterfaceA.class});
        final ObserverListener listenerA = new ObserverListener(consumer.bus);
        obsA.registerListener(listenerA);
        final Observer obsB = new Observer(consumer.bus, new Class<?>[] {InterfaceB.class});
        final ObserverListener listenerB = new ObserverListener(consumer.bus);
        obsB.registerListener(listenerB);
        final Observer obsAB = new Observer(consumer.bus, new Class<?>[] {InterfaceA.class, InterfaceB.class});
        final ObserverListener listenerAB = new ObserverListener(consumer.bus);
        obsAB.registerListener(listenerAB);

        Lambda allDone = new Lambda() {
            public boolean func() {
                return listenerA.counter == 0 &&
                    listenerB.counter == 0 &&
                    listenerAB.counter == 0;
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
        final ObserverListener listenerB2 = new ObserverListener(consumer.bus);
        listenerB2.expectInvocations(0);
        obsB.registerListener(listenerB2, false);

        Lambda allDone2 = new Lambda() {
            public boolean func() {
                return listenerA.counter == 0 &&
                    listenerB.counter == 0 &&
                    listenerB2.counter == 0 &&
                    listenerAB.counter == 0;
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
        final Observer obsB2 = new Observer(consumer.bus, new Class<?>[]{InterfaceB.class});
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

        obsA.unregisterAllListeners();
        obsB.unregisterAllListeners();
        obsB2.unregisterAllListeners();
        obsAB.unregisterAllListeners();
    }

    public void testSimple() {
        Participant provider = new Participant("prov");
        Participant consumer = new Participant("cons");
        simpleScenario(provider, consumer);
        provider.stop();
        consumer.stop();
    }

    public void testSimpleSelf() {
        Participant both = new Participant("both");
        simpleScenario(both, both);
        both.stop();
    }

    public void testRejection() {
        Participant willing = new Participant("willing");
        Participant doubtful = new Participant("doubtful");
        Participant unwilling = new Participant("unwilling");
        Participant consumer = new Participant("consumer");

        willing.createA("a");
        doubtful.createAB("a");
        unwilling.createAB("a");

        unwilling.accepter.accept = false;

        final ObserverListener listener = new ObserverListener(consumer.bus);
        final Observer obs = new Observer(consumer.bus, new Class<?>[] {InterfaceA.class});
        obs.registerListener(listener);

        listener.expectInvocations(2);
        willing.registerObject("a");
        doubtful.registerObject("a");
        unwilling.registerObject("a");

        Lambda ok = new Lambda() { public boolean func() { return listener.counter == 0; }};
        assertTrue(waitForLambda(3000, ok));

        /* now let doubtful kill the connection */
        try {
            Thread.sleep(100); // This sleep is necessary to make sure the provider knows it has a session.
        } catch (InterruptedException iex) {
        }
        listener.expectInvocations(1);
        doubtful.bus.leaveHostedSession(doubtful.accepter.sessions.get(consumer.bus.getUniqueName()));
        assertTrue(waitForLambda(3000, ok));

        /* there should only be one object left */
        assertEquals(1, countProxies(obs));

        /* unannounce and reannounce, connection should be restored */
        listener.expectInvocations(1);
        doubtful.unregisterObject("a");
        doubtful.registerObject("a");
        assertTrue(waitForLambda(3000, ok));

        /* now there should only be two objects */
        assertEquals(2, countProxies(obs));

        willing.stop();
        doubtful.stop();
        unwilling.stop();
        consumer.stop();

        obs.unregisterAllListeners();
    }

    public void testListenTwice() {
        /* reuse the same listener for two observers */
        Participant provider = new Participant("prov");
        Participant consumer = new Participant("cons");

        provider.createA("a");
        provider.createAB("ab");
        provider.createAB("ab2");

        final ObserverListener listener = new ObserverListener(consumer.bus);
        final Observer obs = new Observer(consumer.bus, new Class<?>[] {InterfaceA.class});
        obs.registerListener(listener);
        final Observer obs2 = new Observer(consumer.bus, new Class<?>[] {InterfaceA.class});
        obs2.registerListener(listener);

        Lambda ok = new Lambda() { public boolean func() { return listener.counter == 0; }};

        /* use listener for 2 observers, so we expect to
         * see all events twice */
        listener.expectInvocations(6);
        provider.registerObject("a");
        provider.registerObject("ab");
        provider.registerObject("ab2");

        assertTrue(waitForLambda(3000, ok));

        obs2.unregisterListener(listener);
        /* one observer is gone, so we expect to see
         * every event just once. */
        listener.expectInvocations(3);
        provider.unregisterObject("a");
        provider.unregisterObject("ab");
        provider.unregisterObject("ab2");

        assertTrue(waitForLambda(3000, ok));

        provider.stop();
        consumer.stop();

        obs.unregisterAllListeners();
        obs2.unregisterAllListeners();
    }

    public void testMulti() {
        /* multiple providers, multiple consumers */
        Participant one = new Participant("one");
        Participant two = new Participant("two");

        one.createA("a");
        one.createB("b");
        one.createAB("ab");
        two.createA("a");
        two.createB("b");
        two.createAB("ab");

        final Observer obsAone = new Observer(one.bus, new Class<?>[] {InterfaceA.class});
        final ObserverListener lisAone = new ObserverListener(one.bus);
        obsAone.registerListener(lisAone);
        final Observer obsBone = new Observer(one.bus, new Class<?>[] {InterfaceB.class});
        final ObserverListener lisBone = new ObserverListener(one.bus);
        obsBone.registerListener(lisBone);
        final Observer obsABone = new Observer(one.bus, new Class<?>[] {InterfaceA.class, InterfaceB.class});
        final ObserverListener lisABone = new ObserverListener(one.bus);
        obsABone.registerListener(lisABone);

        final Observer obsAtwo = new Observer(two.bus, new Class<?>[] {InterfaceA.class});
        final ObserverListener lisAtwo = new ObserverListener(two.bus);
        obsAtwo.registerListener(lisAtwo);
        final Observer obsBtwo = new Observer(two.bus, new Class<?>[] {InterfaceB.class});
        final ObserverListener lisBtwo = new ObserverListener(two.bus);
        obsBtwo.registerListener(lisBtwo);
        final Observer obsABtwo = new Observer(two.bus, new Class<?>[] {InterfaceA.class, InterfaceB.class});
        final ObserverListener lisABtwo = new ObserverListener(two.bus);
        obsABtwo.registerListener(lisABtwo);


        Lambda ok = new Lambda () {
            public boolean func() {
                return lisAone.counter == 0 &&
                    lisBone.counter == 0 &&
                    lisABone.counter == 0 &&
                    lisAtwo.counter == 0 &&
                    lisBtwo.counter == 0 &&
                    lisABtwo.counter == 0;
            }
        };

        /* put objects on the bus */
        lisAone.expectInvocations(4);
        lisBone.expectInvocations(4);
        lisABone.expectInvocations(2);
        lisAtwo.expectInvocations(4);
        lisBtwo.expectInvocations(4);
        lisABtwo.expectInvocations(2);

        one.registerObject("a");
        one.registerObject("b");
        one.registerObject("ab");
        two.registerObject("a");
        two.registerObject("b");
        two.registerObject("ab");

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

        one.unregisterObject("a");
        one.unregisterObject("b");
        one.unregisterObject("ab");
        two.unregisterObject("a");
        two.unregisterObject("b");
        two.unregisterObject("ab");

        assertTrue(waitForLambda(3000, ok));
        assertEquals(0, countProxies(obsAone));
        assertEquals(0, countProxies(obsBone));
        assertEquals(0, countProxies(obsABone));
        assertEquals(0, countProxies(obsAtwo));
        assertEquals(0, countProxies(obsBtwo));
        assertEquals(0, countProxies(obsABtwo));

        one.stop();
        two.stop();

        obsAone.unregisterAllListeners();
        obsBone.unregisterAllListeners();
        obsABone.unregisterAllListeners();
        obsAtwo.unregisterAllListeners();
        obsBtwo.unregisterAllListeners();
        obsABtwo.unregisterAllListeners();
    }
}
