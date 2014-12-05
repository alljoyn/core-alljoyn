/******************************************************************************
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
 ******************************************************************************/
#include <qcc/platform.h>

#include <assert.h>
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

static const char* tags[] = { "en", "de" };
static const char* objId = "obj";
static const char* objDescription[] = { "This is the object", "DE: This is the object" };
static const char* ifcId = "ifc";
static const char* ifcDescription[] = { "This is the interface", "<bold>DE:</bold> This is the interface" };
static const char* propId = "prop";
static const char* namePropDescription[] = { "This is the actual name", "DE: This is the actual name" };

static const char* IntrospectWithDescriptionString1[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <description>This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>This is the interface</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>This is the actual name</description>\n"
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

    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <description>DE: This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>&lt;bold&gt;DE:&lt;/bold&gt; This is the interface</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>DE: This is the actual name</description>\n"
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
};

static const char* IntrospectWithDescriptionString2[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>This is the interface</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>This is the actual name</description>\n"
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

    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <description>&lt;bold&gt;DE:&lt;/bold&gt; This is the interface</description>\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\">\n"
    "      <description>DE: This is the actual name</description>\n"
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
};

static const char* IntrospectWithDescriptionString3[] = {
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <description>This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\"/>\n"
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

    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <description>DE: This is the object</description>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"org.alljoyn.Bus.DescriptionInterface\">\n"
    "    <property name=\"name\" type=\"s\" access=\"readwrite\"/>\n"
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
        const char* tag = (*targetLanguage == '\0') ? "en" : targetLanguage;
        size_t i = 0;

        if (0 == strcmp(tag, "en")) {
            i = 0;
        } else if (0 == strcmp(tag, "de")) {
            i = 1;
        } else {
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
            ASSERT_STREQ(tags[1 - i], pas);
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
        EXPECT_EQ(ER_OK, replyMsg->GetArg(0)->Get("s", &str));
        ASSERT_STREQ(IntrospectWithDescriptionStr, str);
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
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString1[1]);
}

TEST_F(DescriptionTest, IntrospectableDescriptionObjectNoTranslate) {
    // Service part
    MyTranslator translator;
    InterfaceDescription* intf = NULL;
    EXPECT_EQ(ER_OK, s_msgBusServer->CreateInterface(INTERFACE_NAME, intf));
    ASSERT_TRUE(intf != NULL);

    intf->AddProperty("name", "s", PROP_ACCESS_RW);
    intf->SetDescriptionLanguage("");
    intf->SetDescription(ifcId);
    intf->SetPropertyDescription("name", propId);
    intf->SetDescriptionTranslator(&translator);
    intf->Activate();

    ASSERT_TRUE((m_testObj = new DescriptionObjectNoTranslate(*intf, SERVICE_PATH)) != NULL);

    ASSERT_EQ(ER_OK, s_msgBusServer->RegisterBusObject(*m_testObj));
    ASSERT_EQ(ER_OK, s_msgBusServer->Connect());

    ASSERT_TRUE((m_remoteObj = new ProxyBusObject(*s_msgBusClient, s_msgBusServer->GetUniqueName().c_str(), SERVICE_PATH, 0)) != NULL);
    EXPECT_EQ(ER_OK, m_remoteObj->IntrospectRemoteObject());

    DescriptionLanguages(m_remoteObj);
    IntrospectWithDescription(m_remoteObj, "en", IntrospectWithDescriptionString2[0]);
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString2[1]);
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
    IntrospectWithDescription(m_remoteObj, "de", IntrospectWithDescriptionString3[1]);
}
