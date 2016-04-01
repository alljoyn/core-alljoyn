/******************************************************************************
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
 ******************************************************************************/
#include <qcc/platform.h>

#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/String.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>
#include <alljoyn/Status.h>

#include <gtest/gtest.h>
#include "ajTestCommon.h"
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

using namespace std;
using namespace qcc;
using namespace ajn;

static const char* INTERFACE_NAME = "org.alljoyn.Bus.DescriptionInterface";
static const char* SERVICE_PATH = "/";

static const char* IntrospectionXmlWithVersion =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"Bar\" type=\"s\" access=\"read\">\n"
    "      <annotation name=\"org.gtk.GDBus.Since\" value=\"1\"/>\n"
    "    </property>\n"
    "    <property name=\"Baz\" type=\"s\" access=\"read\">\n"
    "      <annotation name=\"org.gtk.GDBus.Since\" value=\"2\"/>\n"
    "    </property>\n"
    "    <property name=\"Foo\" type=\"s\" access=\"read\"/>\n"
    "    <annotation name=\"org.gtk.GDBus.Since\" value=\"2\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithTypeSignatures =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <signal name=\"Announce\">\n"
    "      <arg name=\"objectDescription\" type=\"a(sas)\" direction=\"out\">\n"
    "        <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"a[ObjectDescription]\"/>\n"
    "      </arg>\n"
    "      <arg name=\"metaData\" type=\"a{sv}\" direction=\"out\">\n"
    "        <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"[ApplicationMetadata]\"/>\n"
    "      </arg>\n"
    "    </signal>\n"
    "    <method name=\"GetObjectDescription\">\n"
    "      <arg name=\"objectDescription\" type=\"a(sas)\" direction=\"out\">\n"
    "        <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"a[ObjectDescription]\"/>\n"
    "      </arg>\n"
    "    </method>\n"
    "    <property name=\"ObjectDescriptions\" type=\"a(sas)\" access=\"read\">\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"a[ObjectDescription]\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.Dict.ApplicationMetadata.Key.Type\" value=\"s\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.Dict.ApplicationMetadata.Value.Type\" value=\"v\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.Struct.ObjectDescription.Field.implementedInterfaces.Type\" value=\"as\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.Struct.ObjectDescription.Field.path.Type\" value=\"o\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithTypeSignatureAnnotations =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"SupplySource\" type=\"y\" access=\"read\">\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"The supply source of water\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"[WaterSupplySource]\"/>\n"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.Enum.WaterSupplySource.Value.NotSupported\" value=\"255\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.Enum.WaterSupplySource.Value.Pipe\" value=\"1\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.Enum.WaterSupplySource.Value.Tank\" value=\"0\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithDescriptions =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"Method En.\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.Nl\" value=\"Method Nl.\"/>\n"
    "    </method>\n"
    "    <signal name=\"LightOn\">\n"
    "      <arg name=\"metaData\" type=\"a{sv}\" direction=\"out\">\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"Light metadata\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.Nl\" value=\"Licht metadata\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"[ApplicationMetadata]\"/>\n"
    "      </arg>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"Light has been turned on\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.Nl\" value=\"Licht is aangestoken\"/>\n"
    "    </signal>\n"
    "    <property name=\"SupplySource\" type=\"y\" access=\"read\">\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"The supply source of water\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.Nl\" value=\"Het aanbod bron van water\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Name\" value=\"[WaterSupplySource]\"/>\n"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"Hello interface\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithSignalEmissionBehaviors =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <signal name=\"globalBroadcastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.GlobalBroadcast\" value=\"true\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"false\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacyNonSessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"false\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessioncastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessioncast\" value=\"true\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"false\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"signal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"unicastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"false\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Unicast\" value=\"true\"/>\n"
    "    </signal>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithConstants =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"WaterTemperature\" type=\"u\" access=\"readwrite\">\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Default\" value=\"0\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.DisplayHint\" value=\"Value range\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Max\" value=\"100\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Min\" value=\"0\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Reference\" value=\"Reference here\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Type.Units\" value=\"degrees Celsius\"/>\n"
    "    </property>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithConstantProperties =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"MaxTemperature\" type=\"u\" access=\"read\">\n"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"const\"/>\n"
    "    </property>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlWithCustomAnnotations =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"theProperty\" type=\"u\" access=\"readwrite\">\n"
    "      <annotation name=\"xyz\" value=\"123\"/>\n"
    "    </property>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n";

static const char* IntrospectionXmlLegacy[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <signal name=\"globalBroadcastSignal\" sessionless=\"false\" globalbroadcast=\"true\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacyNonSessionlessSignal\" sessionless=\"false\">\n"
    "      <description>legacy non-sessionless signal</description>\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySessionlessSignal\" sessionless=\"true\">\n"
    "      <description>legacy sessionless signal</description>\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySignal\" sessionless=\"false\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessioncastSignal\" sessioncast=\"true\" sessionless=\"false\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessionlessSignal\" sessionless=\"true\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"unicastSignal\" sessionless=\"false\" unicast=\"true\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n"
    ,

    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <signal name=\"globalBroadcastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.GlobalBroadcast\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacyNonSessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString\" value=\"legacy non-sessionless signal\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString\" value=\"legacy sessionless signal\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessioncastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessioncast\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"unicastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Unicast\" value=\"true\"/>\n"
    "    </signal>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.allseen.Introspectable\">\n"
    "    <method name=\"GetDescriptionLanguages\">\n"
    "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"IntrospectWithDescription\">\n"
    "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "  </interface>\n"
    "</node>\n"
};

class IntrospectionXmlTestBusObject : public BusObject {
  public:
    IntrospectionXmlTestBusObject(BusAttachment& bus, const char* path, const char* interfaceName) :
        BusObject(path)
    {
        const InterfaceDescription* iface = bus.GetInterface(interfaceName);
        EXPECT_TRUE(iface != nullptr);
        QStatus status = AddInterface(*iface, UNANNOUNCED);
        EXPECT_EQ(ER_OK, status);
    }
};

class IntrospectionXmlTest : public::testing::Test {
  protected:
    IntrospectionXmlTest() {
        m_remoteObj = nullptr;
        m_msgBusServer = nullptr;
        m_msgBusClient = nullptr;
        m_testObj = nullptr;
    }

    virtual void SetUp() {
        ASSERT_TRUE((m_msgBusServer = new BusAttachment("serviceMyApp", true)) != nullptr);
        ASSERT_EQ(ER_OK, m_msgBusServer->Start());

        // Client part
        ASSERT_TRUE((m_msgBusClient = new BusAttachment("clientMyApp", true)) != nullptr);
        ASSERT_EQ(ER_OK, m_msgBusClient->Start());
        ASSERT_EQ(ER_OK, m_msgBusClient->Connect());
    }

    virtual void TearDown() {
        delete m_remoteObj;
        m_remoteObj = nullptr;

        delete m_msgBusClient;
        m_msgBusClient = nullptr;

        delete m_msgBusServer;
        m_msgBusServer = nullptr;

        delete m_testObj;
        m_testObj = nullptr;
    }

    BusAttachment* m_msgBusServer;
    BusAttachment* m_msgBusClient;
    BusObject* m_testObj;
    ProxyBusObject* m_remoteObj;

    void CreateFromXmlAndIntrospect(const char* input, const char* output = nullptr)
    {
        if (output == nullptr) {
            output = input;
        }

        EXPECT_EQ(ER_OK, m_msgBusServer->CreateInterfacesFromXml(input));
        ASSERT_TRUE((m_testObj = new IntrospectionXmlTestBusObject(*m_msgBusServer, SERVICE_PATH, INTERFACE_NAME)) != nullptr);
        ASSERT_EQ(ER_OK, m_msgBusServer->RegisterBusObject(*m_testObj));
        ASSERT_EQ(ER_OK, m_msgBusServer->Connect());
        ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*m_msgBusClient, m_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != nullptr);
        EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

        const InterfaceDescription* testIntf = m_remoteObj->GetInterface("org.freedesktop.DBus.Introspectable");
        ASSERT_TRUE(testIntf != 0);
        const InterfaceDescription::Member* member = testIntf->GetMember("Introspect");
        ASSERT_TRUE(member != 0);

        MsgArg msgArg;
        Message replyMsg(*m_msgBusClient);
        EXPECT_EQ(ER_OK, m_remoteObj->MethodCall("org.freedesktop.DBus.Introspectable", "Introspect", &msgArg, 0, replyMsg));
        char* str = nullptr;
        const MsgArg* replyArg = replyMsg->GetArg(0);
        ASSERT_TRUE(replyArg != nullptr);
        EXPECT_EQ(ER_OK, replyArg->Get("s", &str));
        ASSERT_STREQ(output, str);
    }
};

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithVersion) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithVersion);
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithTypeSignatures) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithTypeSignatures);
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithTypeSignaturesAnnotations) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithTypeSignatureAnnotations);
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithDescriptions) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithDescriptions);

    ASSERT_TRUE(m_remoteObj != nullptr);
    const InterfaceDescription* testIntf = m_remoteObj->GetInterface(INTERFACE_NAME);
    ASSERT_TRUE(testIntf != 0);
    const InterfaceDescription::Member* member = testIntf->GetSignal("LightOn");
    EXPECT_STREQ("Licht is aangestoken", member->description.c_str());
    const InterfaceDescription::Property* property = testIntf->GetProperty("SupplySource");
    EXPECT_STREQ("Het aanbod bron van water", property->description.c_str());
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithSignalEmissionBehaviors) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithSignalEmissionBehaviors);

    ASSERT_TRUE(m_remoteObj != nullptr);
    const InterfaceDescription* testIntf = m_remoteObj->GetInterface(INTERFACE_NAME);
    ASSERT_TRUE(testIntf != 0);
    const InterfaceDescription::Member* member = testIntf->GetSignal("signal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);
    EXPECT_FALSE(member->isGlobalBroadcastSignal);
    EXPECT_FALSE(member->isSessioncastSignal);
    EXPECT_FALSE(member->isUnicastSignal);

    member = testIntf->GetSignal("globalBroadcastSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);
    EXPECT_TRUE(member->isGlobalBroadcastSignal);

    member = testIntf->GetSignal("legacyNonSessionlessSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);

    member = testIntf->GetSignal("legacySignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);

    member = testIntf->GetSignal("sessioncastSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_TRUE(member->isSessioncastSignal);
    EXPECT_FALSE(member->isSessionlessSignal);

    member = testIntf->GetSignal("sessionlessSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_TRUE(member->isSessionlessSignal);

    member = testIntf->GetSignal("unicastSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);
    EXPECT_TRUE(member->isUnicastSignal);
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithConstants) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithConstants);

    ASSERT_NE(nullptr, m_remoteObj);
    const InterfaceDescription* testIntf = m_remoteObj->GetInterface(INTERFACE_NAME);
    ASSERT_NE(nullptr, testIntf);
    EXPECT_TRUE(testIntf->HasCacheableProperties());
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithConstantProperties) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithConstantProperties);

    ASSERT_NE(nullptr, m_remoteObj);
    const InterfaceDescription* testIntf = m_remoteObj->GetInterface(INTERFACE_NAME);
    ASSERT_NE(nullptr, testIntf);
    EXPECT_TRUE(testIntf->HasCacheableProperties());
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlWithCustomAnnotations) {

    CreateFromXmlAndIntrospect(IntrospectionXmlWithCustomAnnotations);
}

TEST_F(IntrospectionXmlTest, IntrospectionXmlCreateInterfacesFromXmlLegacy) {

    CreateFromXmlAndIntrospect(IntrospectionXmlLegacy[0], IntrospectionXmlLegacy[1]);

    ASSERT_TRUE(m_remoteObj != nullptr);
    const InterfaceDescription* testIntf = m_remoteObj->GetInterface(INTERFACE_NAME);
    ASSERT_TRUE(testIntf != 0);
    const InterfaceDescription::Member* member = testIntf->GetSignal("globalBroadcastSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);
    EXPECT_TRUE(member->isGlobalBroadcastSignal);
    EXPECT_FALSE(member->isSessioncastSignal);
    EXPECT_FALSE(member->isUnicastSignal);

    member = testIntf->GetSignal("legacyNonSessionlessSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);
    EXPECT_STREQ("legacy non-sessionless signal", member->description.c_str());

    member = testIntf->GetSignal("legacySessionlessSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_TRUE(member->isSessionlessSignal);
    EXPECT_STREQ("legacy sessionless signal", member->description.c_str());

    member = testIntf->GetSignal("legacySignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);

    member = testIntf->GetSignal("sessioncastSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_TRUE(member->isSessioncastSignal);
    EXPECT_FALSE(member->isSessionlessSignal);

    member = testIntf->GetSignal("sessionlessSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_TRUE(member->isSessionlessSignal);

    member = testIntf->GetSignal("unicastSignal");
    ASSERT_TRUE(member != 0);
    EXPECT_FALSE(member->isSessionlessSignal);
    EXPECT_TRUE(member->isUnicastSignal);
}