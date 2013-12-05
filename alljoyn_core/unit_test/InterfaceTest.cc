/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/
#include "ajTestCommon.h"
#include "ServiceSetup.h"

#include <gtest/gtest.h>

#include <qcc/Thread.h>

const char* SERVICE_OBJECT_PATH = "/org/alljoyn/test_services";

class InterfaceTest : public testing::Test {
  public:
    BusAttachment* g_msgBus;

    virtual void SetUp() {
        g_msgBus = new BusAttachment("testservices", true);
        QStatus status = g_msgBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        if (g_msgBus) {
            BusAttachment* deleteMe = g_msgBus;
            g_msgBus = NULL;
            delete deleteMe;
        }
    }
    //Common setup function for all service tests
    QStatus ServiceBusSetup() {
        QStatus status = ER_OK;

        if (!g_msgBus->IsConnected()) {
            /* Connect to the daemon and wait for the bus to exit */
            status = g_msgBus->Connect(ajn::getConnectArg().c_str());
        }

        return status;
    }

};

TEST_F(InterfaceTest, SUCCESS_AddInterfacestoBus_NoActivation) {
    QStatus status = ER_OK;
    InterfaceDescription* testIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add org.alljoyn.alljoyn_test interface */
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(InterfaceTest, SUCCESS_AddSameInterfacestoBus_NoActivation) {
    QStatus status = ER_OK;
    InterfaceDescription* testIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add org.alljoyn.alljoyn_test interface */
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Add org.alljoyn.alljoyn_test interface */
    /* Add same interfaces again should be successfull since we have not Activated */
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(InterfaceTest, SUCCESS_AddInterfacestoBus_Activation) {
    QStatus status = ER_OK;
    InterfaceDescription* testIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add org.alljoyn.alljoyn_test interface */
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    testIntf->Activate();
}

TEST_F(InterfaceTest, FAIL_AddInterfacestoBus_AfterActivation_NoActivate) {
    QStatus status = ER_OK;
    InterfaceDescription* testIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup()) << "  Actual Status: " << QCC_StatusText(status);

    /* Add org.alljoyn.alljoyn_test interface */
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();

    /* Add org.alljoyn.alljoyn_test interface */
    /* Add same interfaces after activation should not be successfull */
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_BUS_IFACE_ALREADY_EXISTS, status) << "  Actual Status: " << QCC_StatusText(status);
    //testIntf should be NULL if call to CreateInterface fails
    ASSERT_EQ(NULL, testIntf);
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_BUS_IFACE_ALREADY_EXISTS, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(NULL, testIntf);
}

TEST_F(InterfaceTest, SUCCESS_RegisterBusObject) {
    QStatus status = ER_OK;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add the test interface to this object */
    //Register service object
    status =  g_msgBus->RegisterBusObject(myService);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    // wait for signal upto 1.0 sec (5 * 200 ms)
    for (int i = 0; i < 200; ++i) {
        if (true == myService.getobjectRegistered()) {
            break;
        }
        qcc::Sleep(5);
    }
    ASSERT_TRUE(myService.getobjectRegistered());
}

TEST_F(InterfaceTest, AddInterfacesToObject) {
    QStatus status = ER_OK;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add org.alljoyn.alljoyn_test interface  using one service object*/
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();

    /* Use a different service object to GetInterface and AddInterfaceToObject */
    ServiceObject myService2(*g_msgBus, SERVICE_OBJECT_PATH);

    const InterfaceDescription* regTestIntf = g_msgBus->GetInterface(myService2.getAlljoynDummyInterfaceName1());
    ASSERT_TRUE(regTestIntf);
    status =  myService2.AddInterfaceToObject(regTestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const InterfaceDescription* regTestIntfval = g_msgBus->GetInterface(myService2.getAlljoynValuesDummyInterfaceName1());
    ASSERT_TRUE(regTestIntfval);
    status = myService2.AddInterfaceToObject(regTestIntfval);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(InterfaceTest, AddInterfaceToObjectAgain) {
    QStatus status = ER_OK;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add org.alljoyn.alljoyn_test interface  using one service object*/
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();

    const InterfaceDescription* regTestIntf = g_msgBus->GetInterface(myService.getAlljoynDummyInterfaceName1());
    ASSERT_TRUE(regTestIntf);
    /* Adding interface to the object for the first time - successfull */
    status = myService.AddInterfaceToObject(regTestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* trying to add the added interface again  -  error*/
    status = myService.AddInterfaceToObject(regTestIntf);
    EXPECT_EQ(ER_BUS_IFACE_ALREADY_EXISTS, status) << "  Actual Status: " << QCC_StatusText(status);

    /* trying top add val interface  to the object for the first time - successfull*/
    const InterfaceDescription* regTestIntfval = g_msgBus->GetInterface(myService.getAlljoynValuesDummyInterfaceName1());
    ASSERT_TRUE(regTestIntfval);
    status = myService.AddInterfaceToObject(regTestIntfval);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* trying top add val interface  to the object again - error*/
    status = myService.AddInterfaceToObject(regTestIntfval);
    EXPECT_EQ(ER_BUS_IFACE_ALREADY_EXISTS, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(InterfaceTest, AddInterfaceAgainToRegisteredObject) {
    QStatus status = ER_OK;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    status =  g_msgBus->RegisterBusObject(myService);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait for a maximum of 2 sec for object to be registered
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (myService.getobjectRegistered()) {
            break;
        }
    }
    ASSERT_TRUE(myService.getobjectRegistered());

    /* Add org.alljoyn.alljoyn_test interface  using one service object*/
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();
    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName1(), testIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    testIntf->Activate();

    const InterfaceDescription* regTestIntf = g_msgBus->GetInterface(myService.getAlljoynDummyInterfaceName1());
    ASSERT_TRUE(regTestIntf);
    /* trying to add the added interface again  -  error*/
    status = myService.AddInterfaceToObject(regTestIntf);
    EXPECT_EQ(ER_BUS_CANNOT_ADD_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);

    /* trying top add val interface  to the object for the first time - successfull*/
    const InterfaceDescription* regTestIntfval = g_msgBus->GetInterface(myService.getAlljoynValuesDummyInterfaceName1());
    ASSERT_TRUE(regTestIntfval);
    /* trying top add val interface  to the object again - error*/
    status = myService.AddInterfaceToObject(regTestIntfval);
    EXPECT_EQ(ER_BUS_CANNOT_ADD_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);
    g_msgBus->UnregisterBusObject(myService);

    /* Adding interface to the object for the first time - successfull */
    status = myService.AddInterfaceToObject(regTestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = myService.AddInterfaceToObject(regTestIntfval);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(InterfaceTest, AddSignalToInterface_AfterItIsActivated) {
    QStatus status = ER_OK;
    InterfaceDescription* regTestIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName2(), regTestIntf);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    assert(regTestIntf);
    regTestIntf->Activate();

    /* Adding a signal to the activated interface -- Error */
    status = regTestIntf->AddSignal("my_signal", "s", NULL, 0);
    ASSERT_EQ(ER_BUS_INTERFACE_ACTIVATED, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(InterfaceTest, GetSignal) {
    QStatus status = ER_OK;
    InterfaceDescription* regTestIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    status =  g_msgBus->RegisterBusObject(myService);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait for a maximum of 2 sec for object to be registered
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (myService.getobjectRegistered()) {
            break;
        }
    }
    ASSERT_TRUE(myService.getobjectRegistered());

    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName3(), regTestIntf);
    ASSERT_TRUE(regTestIntf);

    /* Test for ALLJOYN-333: Get non exist signal crash */
    ASSERT_EQ(NULL, regTestIntf->GetSignal("nonExist_signal"));

    /* Adding a signal to no */
    status = regTestIntf->AddSignal("my_signal1", "s", NULL, 0);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Get defined signal should return non NULL */
    ASSERT_TRUE(regTestIntf->GetSignal("my_signal1"));
}


TEST_F(InterfaceTest, AddSameSignalToInterface_AndActivateItLater) {
    QStatus status = ER_OK;
    InterfaceDescription* regTestIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* After Activation nothing can be added to the interface - this test should throw an error*/
    status =  g_msgBus->RegisterBusObject(myService);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait for a maximum of 2 sec for object to be registered
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (myService.getobjectRegistered()) {
            break;
        }
    }
    ASSERT_TRUE(myService.getobjectRegistered());

    status = g_msgBus->CreateInterface(myService.getAlljoynDummyInterfaceName3(), regTestIntf);
    ASSERT_TRUE(regTestIntf);

    /* Adding a signal to no */
    status = regTestIntf->AddSignal("my_signal", "s", NULL, 0);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Adding a signal to the activated interface -- Error */
    status = regTestIntf->AddSignal("my_signal", "s", NULL, 0);
    ASSERT_EQ(ER_BUS_MEMBER_ALREADY_EXISTS, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Adding a signal to the activated interface -- Error */
    status = regTestIntf->AddSignal("my_signal", "s", "s", 0);
    ASSERT_EQ(ER_BUS_MEMBER_ALREADY_EXISTS, status) << "  Actual Status: " << QCC_StatusText(status);

    status = myService.AddInterfaceToObject(regTestIntf);
    ASSERT_EQ(ER_BUS_CANNOT_ADD_INTERFACE, status);
    regTestIntf->Activate();

}

TEST_F(InterfaceTest, AddSamePropertyToInterface_AndActivateItLater) {
    QStatus status = ER_OK;
    InterfaceDescription* valuesIntf = NULL;
    ServiceObject myService(*g_msgBus, SERVICE_OBJECT_PATH);

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    status =  g_msgBus->RegisterBusObject(myService);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait for a maximum of 2 sec for object to be registered
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (myService.getobjectRegistered()) {
            break;
        }
    }
    ASSERT_TRUE(myService.getobjectRegistered());

    status = g_msgBus->CreateInterface(myService.getAlljoynValuesDummyInterfaceName3(), valuesIntf);
    assert(valuesIntf);

    /* Adding properties to the interface */
    status = valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Add the same property - diff signatures - diff accesses - all should give the same error */
    status = valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
    ASSERT_EQ(ER_BUS_PROPERTY_ALREADY_EXISTS, status);
    status = valuesIntf->AddProperty("int_val", "m", PROP_ACCESS_RW);
    ASSERT_EQ(ER_BUS_PROPERTY_ALREADY_EXISTS, status);
    status = valuesIntf->AddProperty("int_val", "m", PROP_ACCESS_READ);
    ASSERT_EQ(ER_BUS_PROPERTY_ALREADY_EXISTS, status);

    status = valuesIntf->AddProperty("str_val", "s", PROP_ACCESS_RW);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = valuesIntf->AddProperty("ro_str", "s", PROP_ACCESS_READ);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = valuesIntf->AddProperty("prop_signal", "s", PROP_ACCESS_RW);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = myService.AddInterfaceToObject(valuesIntf);
    ASSERT_EQ(ER_BUS_CANNOT_ADD_INTERFACE, status);
    valuesIntf->Activate();
}

/* Interace xml with annotations */
static const char ifcXML[] =
    "  <interface name=\"org.alljoyn.xmlTest\">\n"
    "    <method name=\"Deprecated\">\n"
    "      <arg name=\"sock\" type=\"h\" direction=\"in\"/>\n"
    "      <annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>\n"
    "    </method>\n"
    "    <method name=\"NoReply\">\n"
    "      <arg name=\"sock\" type=\"h\" direction=\"out\"/>\n"
    "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.freedesktop.DBus.Method.MyAnnotation\" value=\"someValue\"/>\n"
    "  </interface>\n";

static const char ifcXML2[] =
    "  <interface name=\"org.alljoyn.xmlTest\">\n"
    "    <method name=\"Deprecated\">\n"
    "      <arg name=\"sock\" type=\"h\" direction=\"in\"/>\n"
    "      <annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>\n"
    "    </method>\n"
    "    <method name=\"NoReply\">\n"
    "      <arg name=\"sock\" type=\"h\" direction=\"out\"/>\n"
    "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>\n"
    "    </method>\n"
    "    <property name=\"myproperty\" type=\"i\" access=\"readwrite\">\n"
    "      <annotation name=\"prop_annotation\" value=\"unused\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.freedesktop.DBus.Method.MyAnnotation\" value=\"someValue\"/>\n"
    "  </interface>\n";


TEST_F(InterfaceTest, FullAnnotationsXmlTest) {
    QStatus status = ER_OK;

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    InterfaceDescription* testIntf = NULL;
    ServiceObject myService(*g_msgBus, "/org/alljoyn/xmlTest");

    ASSERT_EQ(ER_OK, ServiceBusSetup());

    /* Add org.alljoyn.alljoyn_test interface */
    status = g_msgBus->CreateInterface("org.alljoyn.xmlTest", testIntf);
    EXPECT_EQ(status, ER_OK);
    ASSERT_TRUE(testIntf != NULL);

    testIntf->AddAnnotation("org.freedesktop.DBus.Method.MyAnnotation", "someValue");

    testIntf->AddMethod("Deprecated", "h", NULL, "sock");
    testIntf->AddMemberAnnotation("Deprecated", org::freedesktop::DBus::AnnotateDeprecated, "true");

    testIntf->AddMethod("NoReply", NULL, "h", "sock");
    testIntf->AddMemberAnnotation("NoReply", org::freedesktop::DBus::AnnotateNoReply, "true");


    testIntf->AddProperty("myproperty", "i", PROP_ACCESS_RW);
    testIntf->AddPropertyAnnotation("myproperty", "prop_annotation", "unused");

    testIntf->Activate();
    qcc::String xml = testIntf->Introspect(2);
    EXPECT_STREQ(ifcXML2, xml.c_str());
}


// Test for ALLJOYN-397
TEST_F(InterfaceTest, AnnotationXMLTest) {
    QStatus status = ER_OK;

    EXPECT_EQ(ER_OK, ServiceBusSetup());

    status = g_msgBus->CreateInterfacesFromXml(ifcXML);
    EXPECT_EQ(status, ER_OK);

    const InterfaceDescription* iface = g_msgBus->GetInterface("org.alljoyn.xmlTest");
    ASSERT_TRUE(iface != NULL);

    const InterfaceDescription::Member* deprecatedMem = iface->GetMember("Deprecated");
    ASSERT_TRUE(deprecatedMem != NULL);

    //ASSERT_EQ(deprecatedMem->annotation, MEMBER_ANNOTATE_DEPRECATED);
    qcc::String val;
    EXPECT_TRUE(deprecatedMem->GetAnnotation(org::freedesktop::DBus::AnnotateDeprecated, val));
    EXPECT_STREQ("true", val.c_str());

    const InterfaceDescription::Member* noreplyMem = iface->GetMember("NoReply");
    ASSERT_TRUE(noreplyMem != NULL);

    //ASSERT_EQ(noreplyMem->annotation, MEMBER_ANNOTATE_NO_REPLY);
    EXPECT_TRUE(noreplyMem->GetAnnotation(org::freedesktop::DBus::AnnotateNoReply, val));
    EXPECT_STREQ("true", val.c_str());
}

/* Interace xml with optional argument name permutations */
static const char ifcXMLArgNames[] =
    "  <interface name=\"org.alljoyn.xmlTest\">\n"
    "    <method name=\"Method0\">\n"
    "      <arg type=\"s\" direction=\"in\"/>\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Method1\">\n"
    "      <arg name=\"arg0\" type=\"s\" direction=\"in\"/>\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"Method2\">\n"
    "      <arg type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"arg1\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n";

// Test for ALLJOYN-953
TEST_F(InterfaceTest, ArgNamesTest) {
    QStatus status = ER_OK;

    status = g_msgBus->CreateInterfacesFromXml(ifcXMLArgNames);
    EXPECT_EQ(status, ER_OK);

    const InterfaceDescription* iface = g_msgBus->GetInterface("org.alljoyn.xmlTest");
    ASSERT_TRUE(iface != NULL);

    const InterfaceDescription::Member* member = iface->GetMember("Method0");
    ASSERT_TRUE(member != NULL);
    EXPECT_STREQ("", member->argNames.c_str());

    member = iface->GetMember("Method1");
    ASSERT_TRUE(member != NULL);
    EXPECT_STREQ("arg0,", member->argNames.c_str());

    member = iface->GetMember("Method2");
    ASSERT_TRUE(member != NULL);
    EXPECT_STREQ(",arg1", member->argNames.c_str());
}
