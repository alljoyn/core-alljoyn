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
#include <qcc/Util.h>

using namespace std;
using namespace qcc;
using namespace ajn;

static const char* INTERFACE_NAME = "org.alljoyn.Bus.DescriptionInterface";
static const char* SERVICE_PATH = "/";

/* The elements in tags array must be placed in alphabetical order */
static const char* tags[] = { "de", "en" };
static const char* objId = "obj";
static const char* objDescription[] = { "DE: This is the object", "This is the object" };
static const char* ifcId = "ifc";
static const char* ifcDescription[] = { "<bold>DE:</bold> This is the interface", "This is the interface" };
static const char* propId = "prop";
static const char* namePropDescription[] = { "DE: This is the actual name", "This is the actual name" };
static const char* methId = "method";
static const char* pingMethodDescription[] = { "DE: This is the ping description", "This is the ping description" };

static const char* IntrospectWithDescriptionString1[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <description>This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>This is the interface</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>This is the actual name</description>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the actual name\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the actual name\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"&lt;bold&gt;DE:&lt;/bold&gt; This is the interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the interface\"/>\n"
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
    "  <description>DE: This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>&lt;bold&gt;DE:&lt;/bold&gt; This is the interface</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>DE: This is the actual name</description>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the actual name\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the actual name\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"&lt;bold&gt;DE:&lt;/bold&gt; This is the interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the interface\"/>\n"
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
    "  <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the object\"/>\n"
    "  <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the object\"/>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the actual name\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the actual name\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"&lt;bold&gt;DE:&lt;/bold&gt; This is the interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the interface\"/>\n"
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

static const char* IntrospectWithDescriptionString2[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>This is the interface</description>\n"
    "    <method name=\"Ping\">\n"
    "      <description>This is the ping description</description>\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the ping description\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the ping description\"/>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>This is the actual name</description>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the actual name\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the actual name\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"&lt;bold&gt;DE:&lt;/bold&gt; This is the interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the interface\"/>\n"
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
    "    <description>&lt;bold&gt;DE:&lt;/bold&gt; This is the interface</description>\n"
    "    <method name=\"Ping\">\n"
    "      <description>DE: This is the ping description</description>\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the ping description\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the ping description\"/>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>DE: This is the actual name</description>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the actual name\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the actual name\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"&lt;bold&gt;DE:&lt;/bold&gt; This is the interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the interface\"/>\n"
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
    "    <method name=\"Ping\">\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the ping description\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the ping description\"/>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the actual name\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the actual name\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"&lt;bold&gt;DE:&lt;/bold&gt; This is the interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the interface\"/>\n"
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

static const char* IntrospectWithDescriptionString3[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <description>This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>ifc</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString\" value=\"ifc\"/>\n"
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
    "  <description>DE: This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>ifc</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString\" value=\"ifc\"/>\n"
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
    "  <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the object\"/>\n"
    "  <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the object\"/>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString\" value=\"ifc\"/>\n"
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

static const char* IntrospectWithDescriptionString4[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <description>This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <signal name=\"globalBroadcastSignal\" sessionless=\"false\" globalbroadcast=\"true\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.GlobalBroadcast\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacyNonSessionlessSignal\" sessionless=\"false\">\n"
    "      <description>legacy non-sessionless signal</description>\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"legacy non-sessionless signal\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"legacy non-sessionless signal\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySessionlessSignal\" sessionless=\"true\">\n"
    "      <description>legacy sessionless signal</description>\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"legacy sessionless signal\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"legacy sessionless signal\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySignal\" sessionless=\"false\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessioncastSignal\" sessioncast=\"true\" sessionless=\"false\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessioncast\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"sessionlessSignal\" sessionless=\"true\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.Sessionless\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"unicastSignal\" sessionless=\"false\" unicast=\"true\">\n"
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
    ,

    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"DE: This is the object\"/>\n"
    "  <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"This is the object\"/>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <signal name=\"globalBroadcastSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.Signal.GlobalBroadcast\" value=\"true\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacyNonSessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"legacy non-sessionless signal\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"legacy non-sessionless signal\"/>\n"
    "    </signal>\n"
    "    <signal name=\"legacySessionlessSignal\">\n"
    "      <arg type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.de\" value=\"legacy sessionless signal\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"legacy sessionless signal\"/>\n"
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

static const char* IntrospectWithDescriptionLanguageString[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description language=\"en\">En interface</description>\n"
    "    <description language=\"fr\">Fr interface</description>\n"
    "    <method name=\"Ping\">\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\">\n"
    "        <description language=\"en\">En argument</description>\n"
    "        <description language=\"fr\">Fr argument</description>\n"
    "      </arg>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <description language=\"en\">En method</description>\n"
    "      <description language=\"fr\">Fr method</description>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description language=\"en\">En property</description>\n"
    "      <description language=\"fr\">Fr property</description>\n"
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
    "</node>\n"
    ,

    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>En interface</description>\n"
    "    <method name=\"Ping\">\n"
    "      <description>En method</description>\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\">\n"
    "        <description>En argument</description>\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En argument\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr argument\"/>\n"
    "      </arg>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En method\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr method\"/>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>En property</description>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En property\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr property\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr interface\"/>\n"
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
    "    <description>Fr interface</description>\n"
    "    <method name=\"Ping\">\n"
    "      <description>Fr method</description>\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\">\n"
    "        <description>Fr argument</description>\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En argument\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr argument\"/>\n"
    "      </arg>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En method\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr method\"/>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>Fr property</description>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En property\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr property\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr interface\"/>\n"
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
    "    <method name=\"Ping\">\n"
    "      <arg name=\"inStr\" type=\"s\" direction=\"in\">\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En argument\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr argument\"/>\n"
    "      </arg>\n"
    "      <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En method\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr method\"/>\n"
    "    </method>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En property\"/>\n"
    "      <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr property\"/>\n"
    "    </property>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.en\" value=\"En interface\"/>\n"
    "    <annotation name=\"org.alljoyn.Bus.DocString.fr\" value=\"Fr interface\"/>\n"
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

class MyTranslator : public Translator {
  public:

    virtual ~MyTranslator() { }

    virtual size_t NumTargetLanguages() {
        return 2;
    }

    virtual void GetTargetLanguage(size_t index, qcc::String& ret) {
        ret.assign(tags[index]);
    }

    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* source) {
        QCC_UNUSED(sourceLanguage);
        const char* tag = (*targetLanguage == '\0') ? "en" : targetLanguage;
        size_t i = 0;
        for (; i < ArraySize(tags); i++) {
            if (0 == strcasecmp(tag, tags[i])) {
                break;
            }
        }
        if (i == ArraySize(tags)) {
            return NULL;
        }

        if (0 == strcmp(source, objId)) {
            return objDescription[i];
        }
        if (0 == strcmp(source, ifcId)) {
            return ifcDescription[i];
        }
        if (0 == strcmp(source, propId)) {
            return namePropDescription[i];
        }
        if (0 == strcmp(source, methId)) {
            return pingMethodDescription[i];
        }
        return NULL;
    }

};

class DescriptionObject : public BusObject {
  public:
    DescriptionObject(InterfaceDescription& intf, const char* path) :
        BusObject(path),
        prop_name("Default name")
    {
        EXPECT_EQ(ER_OK, AddInterface(intf));
        SetDescription("", objId);
    }

  private:
    qcc::String prop_name;

};

class DescriptionObjectNoTranslate : public BusObject {
  public:
    DescriptionObjectNoTranslate(InterfaceDescription& intf, const char* path) :
        BusObject(path),
        prop_name("Default name")
    {
        EXPECT_EQ(ER_OK, AddInterface(intf));
        SetDescription("", objId);
    }

  private:
    qcc::String prop_name;
};

class DescriptionObjectNoIntfTranslate : public BusObject {
  public:
    DescriptionObjectNoIntfTranslate(InterfaceDescription& intf, const char* path) :
        BusObject(path),
        prop_name("Default name")
    {
        EXPECT_EQ(ER_OK, AddInterface(intf));
        SetDescription("", objId);
        SetDescriptionTranslator(&translator);
    }

  private:
    qcc::String prop_name;
    MyTranslator translator;
};

class DescriptionTest : public::testing::Test {
  protected:
    DescriptionTest() {
        m_remoteObj = NULL;
        s_msgBusServer = NULL;
        s_msgBusClient = NULL;
        m_testObj = NULL;
    }

    virtual void SetUp() {
        ASSERT_TRUE((s_msgBusServer = new BusAttachment("serviceMyApp", true)) != NULL);
        ASSERT_EQ(ER_OK, s_msgBusServer->Start());

        // Client part
        ASSERT_TRUE((s_msgBusClient = new BusAttachment("clientMyApp", true)) != NULL);
        ASSERT_EQ(ER_OK, s_msgBusClient->Start());
        ASSERT_EQ(ER_OK, s_msgBusClient->Connect());
    }

    virtual void TearDown() {
        delete m_remoteObj;
        m_remoteObj = NULL;

        delete s_msgBusClient;
        s_msgBusClient = NULL;

        delete s_msgBusServer;
        s_msgBusServer = NULL;

        delete m_testObj;
        m_testObj = NULL;
    }

    BusAttachment* s_msgBusServer;
    BusAttachment* s_msgBusClient;
    BusObject* m_testObj;
    ProxyBusObject* m_remoteObj;

    void DescriptionLanguages(ProxyBusObject* remoteObj)
    {
        const InterfaceDescription* testIntf = remoteObj->GetInterface("org.allseen.Introspectable");
        ASSERT_TRUE(testIntf != 0);
        const InterfaceDescription::Member* member = testIntf->GetMember("GetDescriptionLanguages");
        ASSERT_TRUE(member != 0);

        MyTranslator translator;

        Message replyMsg(*s_msgBusClient);
        EXPECT_EQ(ER_OK, remoteObj->MethodCall("org.allseen.Introspectable", "GetDescriptionLanguages", NULL, 0, replyMsg));
        MsgArg* asArray;
        size_t las;
        EXPECT_EQ(ER_OK, replyMsg->GetArg(0)->Get("as", &las, &asArray));

        ASSERT_EQ(translator.NumTargetLanguages(), las) << "Number of langugages not as expected.";
        for (size_t i = 0; i < las; ++i) {
            char* pas;
            EXPECT_EQ(ER_OK, asArray[i].Get("s", &pas));
            ASSERT_STREQ(tags[i], pas);
        }
    }

    void IntrospectWithDescription(ProxyBusObject* remoteObj, const char* languageTag, const char* IntrospectWithDescriptionStr)
    {
        const InterfaceDescription* testIntf = remoteObj->GetInterface("org.allseen.Introspectable");
        ASSERT_TRUE(testIntf != 0);
        const InterfaceDescription::Member* member = testIntf->GetMember("IntrospectWithDescription");
        ASSERT_TRUE(member != 0);

        MsgArg msgArg("s", languageTag);
        Message replyMsg(*s_msgBusClient);
        EXPECT_EQ(ER_OK, remoteObj->MethodCall("org.allseen.Introspectable", "IntrospectWithDescription", &msgArg, 1, replyMsg));
        char* str = NULL;
        const MsgArg* replyArg = replyMsg->GetArg(0);
        ASSERT_TRUE(replyArg != NULL);
        EXPECT_EQ(ER_OK, replyArg->Get("s", &str));
        ASSERT_STREQ(IntrospectWithDescriptionStr, str);
    }

    void Introspect(ProxyBusObject* remoteObj, const char* IntrospectStr)
    {
        const InterfaceDescription* testIntf = remoteObj->GetInterface("org.freedesktop.DBus.Introspectable");
        ASSERT_TRUE(testIntf != 0);
        const InterfaceDescription::Member* member = testIntf->GetMember("Introspect");
        ASSERT_TRUE(member != 0);

        MsgArg msgArg;
        Message replyMsg(*s_msgBusClient);
        EXPECT_EQ(ER_OK, remoteObj->MethodCall("org.freedesktop.DBus.Introspectable", "Introspect", &msgArg, 0, replyMsg));
        char* str = NULL;
        const MsgArg* replyArg = replyMsg->GetArg(0);
        ASSERT_TRUE(replyArg != NULL);
        EXPECT_EQ(ER_OK, replyArg->Get("s", &str));
        ASSERT_STREQ(IntrospectStr, str);
    }
};

TEST_F(DescriptionTest, IntrospectableDescriptionObject) {
    // Service part
    InterfaceDescription* intf = NULL;
    MyTranslator translator;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);
    intf->SetPropertyDescription("name", propId);
    intf->SetDescriptionTranslator(&translator);
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObject(*intf, SERVICE_PATH)) != NULL);
    m_testObj->SetDescriptionTranslator(&translator);
    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    DescriptionLanguages(m_remoteObj);
    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "en-US", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString1[1]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString1[2]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectGlobalTranslator)
{
    // Service part
    InterfaceDescription* intf = NULL;
    MyTranslator translator;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);
    intf->SetPropertyDescription("name", propId);
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObject(*intf, SERVICE_PATH)) != NULL);
    s_msgBusServer->SetDescriptionTranslator(&translator);
    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "en-US", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString1[1]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString1[2]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectDefaultTranslator)
{
    // Service part
    InterfaceDescription* intf = NULL;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);
    intf->SetPropertyDescription("name", propId);

    Translator* translator = intf->GetDescriptionTranslator();
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(ER_OK, translator->AddStringTranslation(objId, objDescription[i], tags[i]));
        EXPECT_EQ(ER_OK, translator->AddStringTranslation(ifcId, ifcDescription[i], tags[i]));
        EXPECT_EQ(ER_OK, translator->AddStringTranslation(propId, namePropDescription[i], tags[i]));
    }
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObject(*intf, SERVICE_PATH)) != NULL);
    m_testObj->SetDescriptionTranslator(translator);
    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    DescriptionLanguages(m_remoteObj);
    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "en-US", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString1[1]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString1[2]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectNoTranslate) {
    // Service part
    MyTranslator translator;
    InterfaceDescription* intf = NULL;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetPropertyDescription("name", propId);

    intf->AddMethod("Ping", "s", "s", "inStr,outStr", 0);
    intf->SetMemberDescription("Ping", methId);

    intf->SetDescriptionTranslator(&translator);
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObjectNoTranslate(*intf, SERVICE_PATH)) != NULL);

    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    DescriptionLanguages(m_remoteObj);
    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString2[0]);
    IntrospectWithDescription(m_remoteObj, "en-US", IntrospectWithDescriptionString2[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString2[1]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString2[2]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectNoIntfTranslate) {
    // Service part
    InterfaceDescription* intf = NULL;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);
    intf->SetPropertyDescription("name", propId);
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObjectNoIntfTranslate(*intf, SERVICE_PATH)) != NULL);

    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString3[0]);
    IntrospectWithDescription(m_remoteObj, "en-US", IntrospectWithDescriptionString3[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString3[1]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString3[2]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectWithAnnotations) {
    // Service part
    InterfaceDescription* intf = NULL;
    MyTranslator translator;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);
    intf->SetPropertyDescription("name", propId);
    intf->SetDescriptionTranslator(&translator);
    for (size_t i = 0; i < ArraySize(tags); i++) {
        qcc::String annotName = qcc::String("org.alljoyn.Bus.DocString.") + tags[i];
        qcc::String annotValue(namePropDescription[i]);
        intf->AddPropertyAnnotation("name", annotName, annotValue);
    }
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObject(*intf, SERVICE_PATH)) != NULL);
    m_testObj->SetDescriptionTranslator(&translator);
    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    DescriptionLanguages(m_remoteObj);
    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "en-US", IntrospectWithDescriptionString1[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString1[1]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString1[2]);
}

TEST_F(DescriptionTest, SignalTypes)
{
    InterfaceDescription* intf = NULL;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
    ASSERT_EQ(ER_OK, intf->AddSignal("legacySignal", "s", NULL));

    intf->SetDescriptionLanguage("en");
    ASSERT_EQ(ER_OK, intf->AddSignal("legacyNonSessionlessSignal", "s", NULL));
    ASSERT_EQ(ER_OK, intf->SetMemberDescription("legacyNonSessionlessSignal", "legacy non-sessionless signal", false));

    ASSERT_EQ(ER_OK, intf->AddSignal("legacySessionlessSignal", "s", NULL));
    ASSERT_EQ(ER_OK, intf->SetMemberDescription("legacySessionlessSignal", "legacy sessionless signal", true));
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    ASSERT_EQ(ER_OK, intf->AddSignal("sessioncastSignal", "s", NULL, MEMBER_ANNOTATE_SESSIONCAST));
    ASSERT_EQ(ER_OK, intf->AddSignal("sessionlessSignal", "s", NULL, MEMBER_ANNOTATE_SESSIONLESS));
    ASSERT_EQ(ER_OK, intf->AddSignal("unicastSignal", "s", NULL, MEMBER_ANNOTATE_UNICAST));
    ASSERT_EQ(ER_OK, intf->AddSignal("globalBroadcastSignal", "s", NULL, MEMBER_ANNOTATE_GLOBAL_BROADCAST));

    MyTranslator translator;
    intf->SetDescriptionTranslator(&translator);
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObject(*intf, SERVICE_PATH)) != NULL);
    m_testObj->SetDescriptionTranslator(&translator);
    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString4[0]);
    Introspect(m_remoteObj, IntrospectWithDescriptionString4[1]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectWithMultipleLanguages) {
    // Service part
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterfacesFromXml(IntrospectWithDescriptionLanguageString[0]));
    const InterfaceDescription* intf = s_msgBusServer->GetInterface(INTERFACE_NAME);
    ASSERT_TRUE(intf != NULL);

    ASSERT_TRUE((m_testObj = new DescriptionObject((InterfaceDescription&)*intf, SERVICE_PATH)) != NULL);
    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionLanguageString[1]);
    IntrospectWithDescription(m_remoteObj, "fr", IntrospectWithDescriptionLanguageString[2]);
    Introspect(m_remoteObj, IntrospectWithDescriptionLanguageString[3]);
}
