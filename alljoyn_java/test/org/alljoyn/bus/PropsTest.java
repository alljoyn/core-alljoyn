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

import junit.framework.TestCase;

import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Properties;

public class PropsTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    public PropsTest(String name) {
        super(name);
    }

    public class Service implements PropsInterface, BusObject {

        private String stringProperty = "Hello";

        private int intProperty = 6;

        public String getStringProp() { return stringProperty; }

        public void setStringProp(String stringProperty) { this.stringProperty = stringProperty; }

        public int getIntProp() { return intProperty; }

        public void setIntProp(int intProperty) { this.intProperty = intProperty; }

        public String ping(String str) throws BusException {
            return str;
        }
    }

    public class CustomExceptionService implements PropsInterface, BusObject {
        public static final String GET_STRING_ERROR_NAME = "getStringPropFailed";
        public static final String GET_STRING_ERROR_MESSAGE = "getString customMessage";
        public static final String SET_STRING_ERROR_NAME = "setStringPropFailed set";
        public static final String SET_STRING_ERROR_MESSAGE = "setString customMessage set";
        public static final String GET_INT_ERROR_NAME = "getIntPropFailed";
        public static final String GET_INT_ERROR_MESSAGE = "getInt customMessage";
        public static final String SET_INT_ERROR_NAME = "setIntPropFailed set";
        public static final String SET_INT_ERROR_MESSAGE = "setInt customMessage set";

        private String stringProperty = "Hello";

        private int intProperty = 6;

        public String getStringProp() throws BusException {
            throw new ErrorReplyBusException(GET_STRING_ERROR_NAME, GET_STRING_ERROR_MESSAGE);
        }

        public void setStringProp(String stringProperty) throws BusException{
            this.stringProperty = stringProperty;
            throw new ErrorReplyBusException(SET_STRING_ERROR_NAME, SET_STRING_ERROR_MESSAGE);
        }

        public int getIntProp() throws BusException {
            throw new ErrorReplyBusException(GET_INT_ERROR_NAME, GET_INT_ERROR_MESSAGE);
        }

        public void setIntProp(int intProperty) throws BusException {
            this.intProperty = intProperty;
            throw new ErrorReplyBusException(SET_INT_ERROR_NAME, SET_INT_ERROR_MESSAGE);
        }

        public String ping(String str) throws BusException {
            return str;
        }
    }

    public class CustomExceptionService2 implements PropsInterface, BusObject {
        public static final String GET_STRING_ERROR_NAME = "";
        public static final String GET_STRING_ERROR_MESSAGE = "";
        public static final String SET_STRING_ERROR_NAME = "";
        public static final String SET_STRING_ERROR_MESSAGE = "";
        public static final String GET_INT_ERROR_NAME = "";
        public static final String GET_INT_ERROR_MESSAGE = "";
        public static final String SET_INT_ERROR_NAME = "";
        public static final String SET_INT_ERROR_MESSAGE = "test";

        private String stringProperty = "Hello";

        private int intProperty = 6;

        public String getStringProp() throws BusException {
            throw new ErrorReplyBusException(GET_STRING_ERROR_NAME, GET_STRING_ERROR_MESSAGE);
        }

        public void setStringProp(String stringProperty) throws BusException{
            this.stringProperty = stringProperty;
            throw new ErrorReplyBusException(SET_STRING_ERROR_NAME, SET_STRING_ERROR_MESSAGE);
        }

        public int getIntProp() throws BusException {
            throw new ErrorReplyBusException(GET_INT_ERROR_NAME, GET_INT_ERROR_MESSAGE);
        }

        public void setIntProp(int intProperty) throws BusException {
            this.intProperty = intProperty;
            throw new ErrorReplyBusException(SET_INT_ERROR_NAME, SET_INT_ERROR_MESSAGE);
        }

        public String ping(String str) throws BusException {
            return str;
        }
    }

    public class CustomExceptionService3 implements PropsInterface, BusObject {
        public static final String GET_INT_ERROR_MESSAGE = "retrieve int failed";
        public String getStringProp() throws BusException {
            throw new MarshalBusException();
        }

        public void setStringProp(String stringProperty) throws BusException {
            throw new AnnotationBusException();
        }

        public int getIntProp() throws BusException {
            throw new BusException(GET_INT_ERROR_MESSAGE);
        }

        public void setIntProp(int intProperty) throws BusException {
            throw new BusException();
        }

        public String ping(String str) throws BusException {
            return str;
        }
    }

    BusAttachment bus;
    BusAttachment clientBus;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());

        /* Register the service */
        Service service = new Service();
        Status status = bus.registerBusObject(service, "/testProperties");
        if (Status.OK != status) {
            throw new BusException("BusAttachment.registerBusObject() failed: " + status.toString());
        }

        CustomExceptionService customExceptionService = new CustomExceptionService();
        status = bus.registerBusObject(customExceptionService, "/testPropertiesCustomException");
        if (Status.OK != status) {
            throw new BusException("BusAttachment.registerBusObject() failed: " + status.toString());
        }

        CustomExceptionService2 customExceptionService2 = new CustomExceptionService2();
        status = bus.registerBusObject(customExceptionService2, "/testPropertiesCustomException2");
        if (Status.OK != status) {
            throw new BusException("BusAttachment.registerBusObject() failed: " + status.toString());
        }

        CustomExceptionService3 customExceptionService3 = new CustomExceptionService3();
        status = bus.registerBusObject(customExceptionService3, "/testPropertiesCustomException3");
        if (Status.OK != status) {
            throw new BusException("BusAttachment.registerBusObject() failed: " + status.toString());
        }
    }

    public void tearDown() throws Exception {
        if (bus != null) {
            bus.disconnect();
            bus.release();
            bus = null;
        }
    }

    public void testProps() throws Exception {
        /* Request a well-known name */
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.samples.props",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        if (res != DBusProxyObj.RequestNameResult.PrimaryOwner) {
            throw new BusException("Failed to obtain well-known name");
        }

        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.samples.props",
                                                         "/testProperties",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });
        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);

        /* Get a property */
        assertEquals("Hello", proxy.getStringProp());

        /* Set a property */
        proxy.setStringProp("MyNewValue");

        /* Get all of the properties of the interface */
        assertEquals("MyNewValue", proxy.getStringProp());
        assertEquals(6, proxy.getIntProp());

        /* Use the org.freedesktop.DBus.Properties interface to get all the properties */
        Properties properties = remoteObj.getInterface(Properties.class);
        Map<String, Variant> map = properties.GetAll("org.alljoyn.bus.PropsInterface");
        assertEquals("MyNewValue", map.get("StringProp").getObject(String.class));
        assertEquals(6, (int)map.get("IntProp").getObject(Integer.class));
   }

    public void testGetProperty() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class });
        Variant stringProp = remoteObj.getProperty(PropsInterface.class, "StringProp");
        assertEquals("Hello", stringProp.getObject(String.class));
    }

    public void testSetProperty() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class });
        remoteObj.setProperty(PropsInterface.class, "StringProp", new Variant("set"));
        Variant stringProp = remoteObj.getProperty(PropsInterface.class, "StringProp");
        assertEquals("set", stringProp.getObject(String.class));
    }

    public void testGetAllProperties() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class });
        Map<String, Variant> map = remoteObj.getAllProperties(PropsInterface.class);
        assertEquals("Hello", map.get("StringProp").getObject(String.class));
        assertEquals(6, (int)map.get("IntProp").getObject(Integer.class));
    }

    /* ALLJOYN-2043 */
    public void testGetAllThenMethodCall() throws Exception {
        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });

        /* Use the org.freedesktop.DBus.Properties interface to get all the properties */
        Properties properties = remoteObj.getInterface(Properties.class);
        Map<String, Variant> map = properties.GetAll("org.alljoyn.bus.PropsInterface");
        assertEquals("Hello", map.get("StringProp").getObject(String.class));
        assertEquals(6, (int)map.get("IntProp").getObject(Integer.class));

        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);
        assertEquals("World", proxy.ping("World"));
   }

    /* ALLJOYN-2924 */
    public void testCustomException() throws Exception {
        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testPropertiesCustomException",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });
        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);

        /* Get a String property */
        try {
            proxy.getStringProp();
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(errorReply.getErrorName(), CustomExceptionService.GET_STRING_ERROR_NAME);
            assertEquals(errorReply.getErrorMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService.GET_STRING_ERROR_MESSAGE);
        } catch (BusException busException) {
            assertEquals(true, false);
        }

        /* Set a String property */
        try {
            proxy.setStringProp("MyNewValue");
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(errorReply.getErrorName(), CustomExceptionService.SET_STRING_ERROR_NAME);
            assertEquals(errorReply.getErrorMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService.SET_STRING_ERROR_MESSAGE);
        } catch (BusException busException) {
            assertEquals(true, false);
        }

        /* get an int property */
        try {
            proxy.getIntProp();
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(errorReply.getErrorName(), CustomExceptionService.GET_INT_ERROR_NAME);
            assertEquals(errorReply.getErrorMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService.GET_INT_ERROR_MESSAGE);
        } catch (BusException busException) {
            assertEquals(true, false);
        }

        /* set an int property */
        try {
            proxy.setIntProp(1);
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(errorReply.getErrorName(), CustomExceptionService.SET_INT_ERROR_NAME);
            assertEquals(errorReply.getErrorMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService.SET_INT_ERROR_MESSAGE);
        } catch (BusException busException) {
            assertEquals(true, false);
        }
   }

    /* ALLJOYN-2924 */
    public void testCustomException2() throws Exception {
        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testPropertiesCustomException2",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });
        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);

        /* Get a String property */
        try {
            proxy.getStringProp();
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {
            assertEquals(busException.getMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService2.GET_STRING_ERROR_MESSAGE);
        }

        /* Set a String property */
        try {
            proxy.setStringProp("MyNewValue");
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {
            assertEquals(busException.getMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService2.SET_STRING_ERROR_MESSAGE);
        }

        /* get an int property */
        try {
            proxy.getIntProp();
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {
            assertEquals(busException.getMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService2.GET_INT_ERROR_MESSAGE);
        }

        /* set an int property */
        try {
            proxy.setIntProp(1);
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {
            assertEquals(busException.getMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE" + CustomExceptionService2.SET_INT_ERROR_MESSAGE);
        }
   }

    /* ALLJOYN-2924 */
    public void testCustomException3() throws Exception {
        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testPropertiesCustomException3",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });
        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);

        /* Get a String property */
        try {
            proxy.getStringProp();
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {}

        /* Set a String property */
        try {
            proxy.setStringProp("MyNewValue");
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {}

        /* get an int property */
        try {
            proxy.getIntProp();
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {
            assertEquals(busException.getMessage(), "ER_BUS_REPLY_IS_ERROR_MESSAGE");
        }

        /* set an int property */
        try {
            proxy.setIntProp(1);
        } catch (ErrorReplyBusException errorReply) {
            assertEquals(true, false);
        } catch (BusException busException) {
        }
   }
}
