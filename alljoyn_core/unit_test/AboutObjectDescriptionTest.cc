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


#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/version.h>

#include <alljoyn/MsgArg.h>
#include <gtest/gtest.h>
#include <qcc/String.h>
#include <qcc/Thread.h>

using namespace ajn;
using namespace qcc;

TEST(AboutObjectDescriptionTest, Add)
{
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;

    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.Icon"));

    EXPECT_FALSE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.test"));
    EXPECT_FALSE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.game"));
    EXPECT_FALSE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.mediaplayer"));

    EXPECT_TRUE(aod.HasInterface("org.alljoyn.Icon"));

    EXPECT_TRUE(aod.HasInterface("org.alljoyn.test"));
    EXPECT_TRUE(aod.HasInterface("org.alljoyn.game"));
    EXPECT_TRUE(aod.HasInterface("org.alljoyn.mediaplayer"));

    EXPECT_FALSE(aod.HasInterface("org.alljoyn.IAmNotReal"));

//    MsgArg arg;
//    status = aod.GetMsgArg(&arg);
//    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
//
//    printf("******************\n%s\n*****************\n", arg.ToString().c_str());
}

TEST(AboutObjectDescriptionTest, Remove)
{
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;

    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    const char* remove_interfaces[] = { "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Remove("/org/alljoyn/test", remove_interfaces, 2);

    EXPECT_TRUE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_FALSE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_FALSE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    //MsgArg arg;
    //status = aod.GetMsgArg(&arg);
    //EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //printf("******************\n%s\n*****************\n", arg.ToString().c_str());
}

TEST(AboutObjectDescriptionTest, GetMsgArg)
{
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;

    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg arg;
    status = aod.GetMsgArg(&arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg* structarg;
    size_t s_size;
    status = arg.Get("a(oas)", &s_size, &structarg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(2u, s_size);

    char** object_path = new char*[s_size];
    size_t* number_itfs = new size_t[s_size];
    MsgArg** interfaces_arg = new MsgArg *[s_size];
    for (size_t i = 0; i < s_size; ++i) {
        status = structarg[i].Get("(oas)", &(object_path[i]), &number_itfs[i], &interfaces_arg[i]);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    EXPECT_STREQ("/About/DeviceIcon", object_path[0]);
    ASSERT_EQ(1u, number_itfs[0]);
    char* intf;
    interfaces_arg[0][0].Get("s", &intf);
    EXPECT_STREQ("org.alljoyn.Icon", intf);
    EXPECT_STREQ("/org/alljoyn/test", object_path[1]);
    ASSERT_EQ(3u, number_itfs[1]);

    // This test makes some assumptions about order that may not always be true
    // if we see failures that is a result of right values in the wrong order
    // then this test should be modified to account for that.
    interfaces_arg[1][0].Get("s", &intf);
    EXPECT_STREQ("org.alljoyn.game", intf);
    interfaces_arg[1][1].Get("s", &intf);
    EXPECT_STREQ("org.alljoyn.mediaplayer", intf);
    interfaces_arg[1][2].Get("s", &intf);
    EXPECT_STREQ("org.alljoyn.test", intf);

    delete [] object_path;
    delete [] number_itfs;
    delete [] interfaces_arg;
    //printf("******************\n%s\n*****************\n", arg.ToString().c_str());
}

TEST(AboutObjectDescriptionTest, Initilize)
{
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;

    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);

    MsgArg arg;
    status = aod.GetMsgArg(&arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescription aod1;
    status = aod1.Initialize(arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod1.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(aod1.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod1.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod1.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(aod1.HasInterface("/org/alljoyn/test", "org.alljoyn.Icon"));

    EXPECT_FALSE(aod1.HasInterface("/About/DeviceIcon", "org.alljoyn.test"));
    EXPECT_FALSE(aod1.HasInterface("/About/DeviceIcon", "org.alljoyn.game"));
    EXPECT_FALSE(aod1.HasInterface("/About/DeviceIcon", "org.alljoyn.mediaplayer"));

    MsgArg arg1;
    status = aod1.GetMsgArg(&arg1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //printf("******************\n%s\n*****************\n", arg1.ToString().c_str());
}

TEST(AboutObjectDescriptionTest, GetPaths) {
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;
    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    size_t numPaths = aod.GetPaths(NULL, 0);
    ASSERT_EQ(2, numPaths);
    const char** paths = new const char*[numPaths];
    aod.GetPaths(paths, numPaths);
    // We don't know what order the paths will be returned
    EXPECT_TRUE(strcmp(paths[0], "/About/DeviceIcon") == 0 || strcmp(paths[0], "/org/alljoyn/test") == 0) << paths[0];
    EXPECT_TRUE(strcmp(paths[1], "/About/DeviceIcon") == 0 || strcmp(paths[1], "/org/alljoyn/test") == 0) << paths[1];
    delete [] paths;
}

TEST(AboutObjectDescriptionTest, GetInterfaces) {
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;
    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    size_t numPaths = aod.GetPaths(NULL, 0);
    ASSERT_EQ(2, numPaths);

    size_t numInterfaces = aod.GetInterfaces("/About/DeviceIcon", NULL, 0);
    ASSERT_EQ(1, numInterfaces);
    const char** interfaces = new const char*[numInterfaces];
    aod.GetInterfaces("/About/DeviceIcon", interfaces, numInterfaces);
    EXPECT_STREQ("org.alljoyn.Icon", interfaces[0]);
    delete [] interfaces;

    numInterfaces = aod.GetInterfaces("/org/alljoyn/test", NULL, 0);
    ASSERT_EQ(3, numInterfaces);
    interfaces = new const char*[numInterfaces];
    aod.GetInterfaces("/org/alljoyn/test", interfaces, numInterfaces);
    // We don't konw what order the interfaces will be returned
    EXPECT_TRUE(strcmp(interfaces[0], "org.alljoyn.test") == 0 ||
                strcmp(interfaces[0], "org.alljoyn.game") == 0 ||
                strcmp(interfaces[0], "org.alljoyn.mediaplayer") == 0) << interfaces[0];
    EXPECT_TRUE(strcmp(interfaces[1], "org.alljoyn.test") == 0 ||
                strcmp(interfaces[1], "org.alljoyn.game") == 0 ||
                strcmp(interfaces[1], "org.alljoyn.mediaplayer") == 0) << interfaces[1];
    EXPECT_TRUE(strcmp(interfaces[2], "org.alljoyn.test") == 0 ||
                strcmp(interfaces[2], "org.alljoyn.game") == 0 ||
                strcmp(interfaces[2], "org.alljoyn.mediaplayer") == 0) << interfaces[2];
    delete [] interfaces;
}

TEST(AboutObjectDescriptionTest, Merge)
{
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;
    AboutObjectDescription aod2;
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aod2.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_FALSE(aod2.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_FALSE(aod2.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_FALSE(aod2.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_TRUE(aod2.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    aod2.Merge(aod);

    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(aod2.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod2.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod2.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_TRUE(aod2.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

}

TEST(AboutObjectDescriptionTest, Clear)
{
    QStatus status = ER_FAIL;
    AboutObjectDescription aod;

    status = aod.Add("/About/DeviceIcon", "org.alljoyn.Icon");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* interfaces2[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfaces2, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    aod.Clear();

    EXPECT_FALSE(aod.HasPath("/About/DeviceIcon"));
    EXPECT_FALSE(aod.HasPath("/org/alljoyn/test"));

    EXPECT_FALSE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_FALSE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_FALSE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_FALSE(aod.HasInterface("/org/alljoyn/test", "org.alljoyn.mediaplayer"));
}

class AboutObjectDescriptionTestBusObject1 : public BusObject {
  public:
    AboutObjectDescriptionTestBusObject1(BusAttachment& bus, const char* path)
        : BusObject(path) {
        AddInterface(*bus.GetInterface("test.about.objectdescription.interface1"), ANNOUNCED);
    }
};

class AboutObjectDescriptionTestBusObject2 : public BusObject {
  public:
    AboutObjectDescriptionTestBusObject2(BusAttachment& bus, const char* path)
        : BusObject(path) {
        AddInterface(*bus.GetInterface("test.about.objectdescription.interface2"), ANNOUNCED);
    }
};


TEST(AboutObjectDescriptionTest, PopulateAutomaticallyFromBusObject) {
    QStatus status = ER_FAIL;
    static const char* interface = "<interface name='test.about.objectdescription.interface1'>"
                                   "  <method name='Ping'>"
                                   "    <arg name='out_arg' type='s' direction='in' />"
                                   "    <arg name='return_arg' type='s' direction='out' />"
                                   "  </method>"
                                   "  <signal name='Chirp'>"
                                   "    <arg name='sound' type='s' />"
                                   "  </signal>"
                                   "  <property name='volume' type='i' access='readwrite'/>"
                                   "</interface>";
    BusAttachment bus("AboutObjectDescriptionTest");
    status = bus.CreateInterfacesFromXml(interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutObjectDescriptionTestBusObject1 busObject1(bus, "/test/path1");
    status = bus.RegisterBusObject(busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescription aod;
    status = bus.GetAboutObjectDescription(aod);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("test.about.objectdescription.interface1"));
    EXPECT_TRUE(aod.HasPath("/test/path1"));
    EXPECT_TRUE(aod.HasInterface("/test/path1", "test.about.objectdescription.interface1"));
}

TEST(AboutObjectDescriptionTest, PopulateAutomaticallyFromMultipleBusObjects) {
    QStatus status = ER_FAIL;
    static const char* interface1 = "<interface name='test.about.objectdescription.interface1'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    static const char* interface2 = "<interface name='test.about.objectdescription.interface2'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    BusAttachment bus("AboutObjectDescriptionTest");
    status = bus.CreateInterfacesFromXml(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.CreateInterfacesFromXml(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutObjectDescriptionTestBusObject1 busObject1(bus, "/test/path1");
    AboutObjectDescriptionTestBusObject2 busObject2(bus, "/test/path2");
    status = bus.RegisterBusObject(busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.RegisterBusObject(busObject2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescription aod;
    status = bus.GetAboutObjectDescription(aod);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("test.about.objectdescription.interface1"));
    EXPECT_TRUE(aod.HasPath("/test/path1"));
    EXPECT_TRUE(aod.HasInterface("/test/path1", "test.about.objectdescription.interface1"));

    EXPECT_TRUE(aod.HasInterface("test.about.objectdescription.interface2"));
    EXPECT_TRUE(aod.HasPath("/test/path2"));
    EXPECT_TRUE(aod.HasInterface("/test/path2", "test.about.objectdescription.interface2"));
}

TEST(AboutObjectDescriptionTest, PopulateAutomaticallyRemoveBusObject) {
    QStatus status = ER_FAIL;
    static const char* interface1 = "<interface name='test.about.objectdescription.interface1'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    static const char* interface2 = "<interface name='test.about.objectdescription.interface2'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    BusAttachment bus("AboutObjectDescriptionTest");
    status = bus.CreateInterfacesFromXml(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.CreateInterfacesFromXml(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutObjectDescriptionTestBusObject1 busObject1(bus, "/test/path1");
    AboutObjectDescriptionTestBusObject2 busObject2(bus, "/test/path2");
    status = bus.RegisterBusObject(busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.RegisterBusObject(busObject2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescription aod;
    status = bus.GetAboutObjectDescription(aod);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(aod.HasInterface("test.about.objectdescription.interface1"));
    EXPECT_TRUE(aod.HasPath("/test/path1"));
    EXPECT_TRUE(aod.HasInterface("/test/path1", "test.about.objectdescription.interface1"));

    EXPECT_TRUE(aod.HasInterface("test.about.objectdescription.interface2"));
    EXPECT_TRUE(aod.HasPath("/test/path2"));
    EXPECT_TRUE(aod.HasInterface("/test/path2", "test.about.objectdescription.interface2"));

    bus.UnregisterBusObject(busObject1);

    status = bus.GetAboutObjectDescription(aod);

    EXPECT_FALSE(aod.HasInterface("test.about.objectdescription.interface1"));
    EXPECT_FALSE(aod.HasPath("/test/path1"));
    EXPECT_FALSE(aod.HasInterface("/test/path1", "test.about.objectdescription.interface1"));

    EXPECT_TRUE(aod.HasInterface("test.about.objectdescription.interface2"));
    EXPECT_TRUE(aod.HasPath("/test/path2"));
    EXPECT_TRUE(aod.HasInterface("/test/path2", "test.about.objectdescription.interface2"));
}

TEST(AboutObjectDescriptionTest, GetInterfacePaths) {
    QStatus status = ER_FAIL;
    static const char* interface1 = "<interface name='test.about.objectdescription.interface1'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    BusAttachment bus("AboutObjectDescriptionTest");
    status = bus.CreateInterfacesFromXml(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescriptionTestBusObject1 busObject1(bus, "/test/path1");
    AboutObjectDescriptionTestBusObject1 busObject2(bus, "/test/path2");
    AboutObjectDescriptionTestBusObject1 busObject3(bus, "/test/path3");
    AboutObjectDescriptionTestBusObject1 busObject4(bus, "/test/path4");
    AboutObjectDescriptionTestBusObject1 busObject5(bus, "/test/path5");
    AboutObjectDescriptionTestBusObject1 busObject6(bus, "/test/path6");

    status = bus.RegisterBusObject(busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescription aod;
    status = bus.GetAboutObjectDescription(aod);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    size_t pathNum = aod.GetInterfacePaths("test.about.objectdescription.interface1", NULL, 0);
    EXPECT_EQ((size_t)1, pathNum);
    const char** paths = new const char*[pathNum];
    aod.GetInterfacePaths("test.about.objectdescription.interface1", paths, pathNum);
    EXPECT_STREQ("/test/path1", paths[0]);
    delete [] paths;
    paths = NULL;

    status = bus.RegisterBusObject(busObject2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.RegisterBusObject(busObject3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.RegisterBusObject(busObject4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.RegisterBusObject(busObject5);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.RegisterBusObject(busObject6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // now that we have added the interface 5 more times renew the AboutObjectDescription
    status = bus.GetAboutObjectDescription(aod);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    pathNum = aod.GetInterfacePaths("test.about.objectdescription.interface1", NULL, 0);
    EXPECT_EQ((size_t)6, pathNum);
    paths = new const char*[pathNum];
    aod.GetInterfacePaths("test.about.objectdescription.interface1", paths, pathNum);
    // This test may need to be modified there is nothing guaranteeing the return
    // order of the object paths. However, since the objects are added in numerical
    // order they will most likely be returned in numerical order.
    EXPECT_STREQ("/test/path1", paths[0]);
    EXPECT_STREQ("/test/path2", paths[1]);
    EXPECT_STREQ("/test/path3", paths[2]);
    EXPECT_STREQ("/test/path4", paths[3]);
    EXPECT_STREQ("/test/path5", paths[4]);
    EXPECT_STREQ("/test/path6", paths[5]);
    delete [] paths;
    paths = NULL;
}
