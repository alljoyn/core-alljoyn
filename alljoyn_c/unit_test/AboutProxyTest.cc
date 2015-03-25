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

#include <gtest/gtest.h>

#include <alljoyn_c/AboutData.h>
#include <alljoyn_c/AboutListener.h>
#include <alljoyn_c/AboutObj.h>
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/AboutProxy.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/AjAPI.h>
#include "ajTestCommon.h"
#include "alljoyn/DBusStd.h"
#include <qcc/Thread.h>
#include <qcc/GUID.h>
#include <stdlib.h>

#define WAIT_TIME 5

using namespace ajn;
using namespace qcc;

static const char* ENGLISH_TAG = "en";
static const char* SPANISH_TAG = "es";
static const char* FRENCH_TAG = "fr";

typedef struct about_proxy_test_bus_object_t {
    alljoyn_busobject aboutobject;
    int isannounce;
}about_proxy_test_bus_object;

typedef struct lang_data_t {
    char devicename[256];
    char appname[256];
    char manufacturer[256];
    char description[256];
    char modelnumber[64];
    char dateofmanufacture[64];
    char softwareversion[32];
}lang_data;

typedef struct about_obj_test_about_listener_2_t {
    int announceListenerFlag;
    int aboutObjectPartOfAnnouncement;
    char* busName;
    alljoyn_sessionport port;
    alljoyn_aboutlistener listener;
    uint16_t version;
}about_obj_test_about_listener_2;

static about_proxy_test_bus_object* create_about_proxy_test_bus_object(alljoyn_busattachment bus,
                                                                       const char* path,
                                                                       const char* interfaceName,
                                                                       int announce)
{
    about_proxy_test_bus_object* result =
        (about_proxy_test_bus_object*) malloc(sizeof(about_proxy_test_bus_object));
    result->aboutobject = alljoyn_busobject_create(path, 0, NULL, NULL);
    alljoyn_interfacedescription iface = alljoyn_busattachment_getinterface(bus, interfaceName);
    EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;
    result->isannounce = announce;
    if (iface == NULL) {
        printf("The interfaceDescription pointer for %s was NULL    \
                when it should not have been.\n", interfaceName);
        return NULL;
    }

    if (result->isannounce) {
        alljoyn_busobject_addinterface(result->aboutobject, iface);
        alljoyn_busobject_setannounceflag(result->aboutobject, iface, ANNOUNCED);
    } else {
        alljoyn_busobject_addinterface(result->aboutobject, iface);
        alljoyn_busobject_setannounceflag(result->aboutobject, iface, UNANNOUNCED);
    }
    return result;
}

static void destroy_about_proxy_test_bus_object(about_proxy_test_bus_object* aboutobject)
{
    if (aboutobject) {
        if (aboutobject->aboutobject) {
            alljoyn_busobject_destroy(aboutobject->aboutobject);
            aboutobject->aboutobject = NULL;
        }
        free(aboutobject);
        aboutobject = NULL;
    }
}

static lang_data* create_english_lang_data()
{
    lang_data* data = (lang_data*) malloc(sizeof(lang_data));
    static const char devicename[] = "Dish Washer";
    static const char appname[] = "Controller";
    static const char manufacturer[] = "Alliance";
    static const char description[] = "Smart dish washer";
    static const char modelnumber[] = "HDW-1111";
    static const char dom[] = "2014-20-24";
    static const char softver[] = "0.2.2";
    snprintf(data->devicename, strlen(devicename) + 1, "%s", devicename);
    snprintf(data->appname, strlen(appname) + 1, "%s", appname);
    snprintf(data->manufacturer, strlen(manufacturer) + 1, "%s", manufacturer);
    snprintf(data->description, strlen(description) + 1, "%s", description);
    snprintf(data->modelnumber, strlen(modelnumber) + 1, "%s", modelnumber);
    snprintf(data->dateofmanufacture, strlen(dom) + 1, "%s", dom);
    snprintf(data->softwareversion, strlen(softver) + 1, "%s", softver);
    return data;
}

static lang_data* create_spanish_lang_data()
{
    lang_data* data = (lang_data*) malloc(sizeof(lang_data));
    static const char devicename[] = "dispositivo";
    static const char appname[] = "aplicacion";
    static const char manufacturer[] = "manufactura";
    static const char description[] = "Una descripcion poetica de esta aplicacion";
    static const char modelnumber[] = "HDW-1111";
    static const char dom[] = "2014-20-24";
    static const char softver[] = "0.2.2";
    snprintf(data->devicename, strlen(devicename) + 1, "%s", devicename);
    snprintf(data->appname, strlen(appname) + 1, "%s", appname);
    snprintf(data->manufacturer, strlen(manufacturer) + 1, "%s", manufacturer);
    snprintf(data->description, strlen(description) + 1, "%s", description);
    snprintf(data->modelnumber, strlen(modelnumber) + 1, "%s", modelnumber);
    snprintf(data->dateofmanufacture, strlen(dom) + 1, "%s", dom);
    snprintf(data->softwareversion, strlen(softver) + 1, "%s", softver);
    return data;
}

static int destroy_lang_data(lang_data* data)
{
    if (data) {
        free(data);
        data = NULL;
    }
    return 0;
}

static QCC_BOOL my_sessionportlistener_acceptsessionjoiner(const void* context,
                                                           alljoyn_sessionport sessionPort,
                                                           const char* joiner,
                                                           const alljoyn_sessionopts opts)
{
    return QCC_TRUE;
}

static void about_obj_test_about_listener_announced_cb(const void* context,
                                                       const char* busName,
                                                       uint16_t version,
                                                       alljoyn_sessionport port,
                                                       const alljoyn_msgarg objectDescriptionArg,
                                                       const alljoyn_msgarg aboutDataArg)
{

    about_obj_test_about_listener_2* listener =
        (about_obj_test_about_listener_2*)(context);
    EXPECT_FALSE(listener->announceListenerFlag == 1)
        << "We don't expect the flag to already be  \
            true when an AnnouceSignal is received.";
    size_t busNameLen = strlen(busName);
    if (listener->busName) {
        free(listener->busName);
        listener->busName = NULL;
    }
    listener->busName = (char*) malloc(sizeof(char) * (busNameLen + 1));
    strncpy(listener->busName, busName, busNameLen);
    listener->busName[busNameLen] = '\0';
    listener->port = port;
    listener->announceListenerFlag = QCC_TRUE;
    listener->version = version;
}

static about_obj_test_about_listener_2* create_about_obj_test_about_listener_2()
{
    about_obj_test_about_listener_2* result =
        (about_obj_test_about_listener_2*) malloc(sizeof(about_obj_test_about_listener_2));
    alljoyn_aboutlistener_callback callbacks = {
        &about_obj_test_about_listener_announced_cb
    };
    result->listener = alljoyn_aboutlistener_create(&callbacks, result);
    result->port = 0;
    result->busName = NULL;
    result->announceListenerFlag = 0;
    result->aboutObjectPartOfAnnouncement = 0;
    return result;
}

static void destroy_about_obj_test_about_listener_2(about_obj_test_about_listener_2* listener)
{
    if (listener->busName) {
        free(listener->busName);
        listener->busName = NULL;
    }
    if (listener->listener) {
        alljoyn_aboutlistener_destroy(listener->listener);
        listener = NULL;
    }
}

class AboutProxyTest : public testing::Test {
  public:
    AboutProxyTest()
    {
        port = 25;
        aboutEnglishData = alljoyn_aboutdata_create(ENGLISH_TAG);
        aboutSpanishData = alljoyn_aboutdata_create(SPANISH_TAG);
        serviceBus = NULL;
        listener = NULL;
    }

    virtual void SetUp()
    {
        QStatus status = ER_FAIL;
        engLangData = create_english_lang_data();
        spLangData = create_spanish_lang_data();
        serviceBus = alljoyn_busattachment_create("AboutProxyTestServiceBus", QCC_TRUE);
        status = alljoyn_busattachment_start(serviceBus);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_busattachment_connect(serviceBus, NULL);

        /* Initialize English data */
        setUpEnglishData();
        status = alljoyn_aboutdata_setsupportedlanguage(aboutEnglishData, SPANISH_TAG);

        /* Initialize Spanish data */
        setUpSpanishData();
        status = alljoyn_aboutdata_setsupportedlanguage(aboutSpanishData, ENGLISH_TAG);

        alljoyn_sessionportlistener_callbacks callbacks = {
            &my_sessionportlistener_acceptsessionjoiner
        };

        alljoyn_sessionopts opts =
            alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                       ALLJOYN_PROXIMITY_ANY,
                                       ALLJOYN_TRANSPORT_ANY);

        listener = alljoyn_sessionportlistener_create(&callbacks, NULL);
        status = alljoyn_busattachment_bindsessionport(serviceBus, &port, opts, listener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown()
    {
        destroy_lang_data(engLangData);
        destroy_lang_data(spLangData);
        if (serviceBus) {
            alljoyn_busattachment_stop(serviceBus);
            alljoyn_busattachment_join(serviceBus);
            alljoyn_busattachment_destroy(serviceBus);
            serviceBus = NULL;
        }
        if (aboutEnglishData) {
            alljoyn_aboutdata_destroy(aboutEnglishData);
            aboutEnglishData = NULL;
        }
        if (aboutSpanishData) {
            alljoyn_aboutdata_destroy(aboutSpanishData);
            aboutSpanishData = NULL;
        }
        if (listener) {
            alljoyn_sessionportlistener_destroy(listener);
            listener = NULL;
        }
    }

    alljoyn_busattachment serviceBus;
    alljoyn_aboutdata aboutEnglishData;
    alljoyn_aboutdata aboutSpanishData;
    alljoyn_sessionportlistener listener;
    alljoyn_sessionport port;
    lang_data* engLangData;
    lang_data* spLangData;
  private:
    void setUpEnglishData()
    {
        QStatus status = ER_OK;
        qcc::GUID128 appId;
        status = alljoyn_aboutdata_setappid(aboutEnglishData,
                                            appId.GetBytes(),
                                            qcc::GUID128::SIZE);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        qcc::GUID128 deviceId;
        status = alljoyn_aboutdata_setdeviceid(aboutEnglishData, deviceId.ToString().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_aboutdata_setdevicename(aboutEnglishData,
                                                 engLangData->devicename,
                                                 ENGLISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setdateofmanufacture(aboutEnglishData,
                                                        engLangData->dateofmanufacture);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setappname(aboutEnglishData,
                                              engLangData->appname, ENGLISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setmanufacturer(aboutEnglishData,
                                                   engLangData->manufacturer,
                                                   ENGLISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setmodelnumber(aboutEnglishData,
                                                  engLangData->modelnumber);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setdescription(aboutEnglishData,
                                                  engLangData->description,
                                                  ENGLISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setsoftwareversion(aboutEnglishData,
                                                      engLangData->softwareversion);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutEnglishData, ENGLISH_TAG));
    }

    void setUpSpanishData()
    {
        QStatus status = ER_OK;
        uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        status = alljoyn_aboutdata_setappid(aboutSpanishData, appId, 16);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_aboutdata_setdeviceid(aboutSpanishData, "fakeId");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutSpanishData, SPANISH_TAG));

        status = alljoyn_aboutdata_setdevicename(aboutSpanishData,
                                                 spLangData->devicename,
                                                 SPANISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setappname(aboutSpanishData,
                                              spLangData->appname,
                                              SPANISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setmanufacturer(aboutSpanishData,
                                                   spLangData->manufacturer,
                                                   SPANISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setdescription(aboutSpanishData,
                                                  spLangData->description,
                                                  SPANISH_TAG);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutSpanishData, SPANISH_TAG));

        status = alljoyn_aboutdata_setmodelnumber(aboutSpanishData,
                                                  spLangData->modelnumber);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setsoftwareversion(aboutSpanishData,
                                                      spLangData->softwareversion);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutSpanishData, SPANISH_TAG));
    }
};

TEST_F(AboutProxyTest, GetObjectDescription) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceNameQcc = "test.about.a" + interface_rand_string.ToString();
    qcc::String interfaceQcc =  "<node>"
                               "<interface name='" + ifaceNameQcc + "'>"
                               "</interface>"
                               "</node>";

    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_proxy_test_bus_object* busObject =
        create_about_proxy_test_bus_object(serviceBus,
                                           "/test/alljoyn/AboutProxy",
                                           ifaceName, QCC_TRUE);

    status = alljoyn_busattachment_registerbusobject(serviceBus, busObject->aboutobject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutProxyTestClient", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* aboutListener =
        create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutEnglishData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(alljoyn_busattachment_getuniquename(serviceBus),
                 aboutListener->busName);
    EXPECT_EQ(port, aboutListener->port);

    alljoyn_sessionid sessionId;
    alljoyn_sessionopts sessionOpts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    alljoyn_busattachment_enableconcurrentcallbacks(clientBus);
    status = alljoyn_busattachment_joinsession(clientBus, aboutListener->busName,
                                               aboutListener->port, NULL,
                                               &sessionId, sessionOpts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutproxy proxy =
        alljoyn_aboutproxy_create(clientBus, aboutListener->busName, sessionId);
    uint16_t aboutVersion;
    status = alljoyn_aboutproxy_getversion(proxy, &aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);
    EXPECT_EQ(aboutListener->version, aboutVersion) << "Version mismatch!";

    alljoyn_msgarg objDescriptionArg = alljoyn_msgarg_create();
    status = alljoyn_aboutproxy_getobjectdescription(proxy, objDescriptionArg);
    EXPECT_EQ(ER_OK, status) << "  GetObjectDescription Status: " << QCC_StatusText(status);

    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create();

    alljoyn_aboutobjectdescription_createfrommsgarg(aod, objDescriptionArg);
    size_t numPaths = 0;
    numPaths = alljoyn_aboutobjectdescription_getpaths(aod, NULL, 0);
    ASSERT_EQ(1u, numPaths);

    const char** paths = new const char*[numPaths];
    alljoyn_aboutobjectdescription_getpaths(aod, paths, numPaths);
    /* Object path must match sender */
    EXPECT_TRUE(strcmp(paths[0], "/test/alljoyn/AboutProxy") == 0) << paths[0];
    size_t numInterfaces =
        alljoyn_aboutobjectdescription_getinterfaces(aod, paths[0], NULL, 0);

    ASSERT_EQ(1u, numInterfaces);
    const char** supportedInterfaces = new const char*[numInterfaces];
    alljoyn_aboutobjectdescription_getinterfaces(aod, paths[0],
                                                 supportedInterfaces,
                                                 numInterfaces);
    EXPECT_STREQ(ifaceName, supportedInterfaces[0]) << "Interface mismatch!";

    delete [] supportedInterfaces;
    delete [] paths;
    status = alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);
    alljoyn_aboutproxy_destroy(proxy);
    alljoyn_msgarg_destroy(objDescriptionArg);
    alljoyn_aboutobjectdescription_destroy(aod);
    alljoyn_sessionopts_destroy(sessionOpts);
    destroy_about_proxy_test_bus_object(busObject);
    destroy_about_obj_test_about_listener_2(aboutListener);
    alljoyn_aboutobj_destroy(aboutObj);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutProxyTest, GetAboutdata_English) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceNameQcc = "test.about.b" + interface_rand_string.ToString();
    qcc::String interfaceQcc =  "<node>"
                               "<interface name='" + ifaceNameQcc + "'>"
                               "</interface>"
                               "</node>";

    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_proxy_test_bus_object* busObject =
        create_about_proxy_test_bus_object(serviceBus, "/test/alljoyn/English",
                                           ifaceName, QCC_TRUE);

    status = alljoyn_busattachment_registerbusobject(serviceBus,
                                                     busObject->aboutobject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutProxyTestClient", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* aboutListener = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    status = alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutEnglishData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_TRUE(aboutListener->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(alljoyn_busattachment_getuniquename(serviceBus), aboutListener->busName);
    EXPECT_EQ(port, aboutListener->port);

    alljoyn_sessionid sessionId;
    alljoyn_sessionopts sessionOpts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    alljoyn_busattachment_enableconcurrentcallbacks(clientBus);
    status = alljoyn_busattachment_joinsession(clientBus, aboutListener->busName,
                                               aboutListener->port, NULL,
                                               &sessionId, sessionOpts);
    alljoyn_aboutproxy proxy =
        alljoyn_aboutproxy_create(clientBus, aboutListener->busName, sessionId);
    uint16_t aboutVersion;
    status = alljoyn_aboutproxy_getversion(proxy, &aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);
    EXPECT_EQ(aboutListener->version, aboutVersion) << "Version mismatch!";

    /* AboutData for English */
    alljoyn_msgarg dataArg = alljoyn_msgarg_create();
    status = alljoyn_aboutproxy_getaboutdata(proxy, ENGLISH_TAG, dataArg);
    EXPECT_EQ(ER_OK, status) << "  GetAboutData Status: " << QCC_StatusText(status);
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create_empty();
    status = alljoyn_aboutdata_createfrommsgarg(aboutData, dataArg, ENGLISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  GetAboutData Status: " << QCC_StatusText(status);

    char* appName;
    status = alljoyn_aboutdata_getappname(aboutData, &appName, ENGLISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->appname, appName);

    char* deviceName;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, ENGLISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->devicename, deviceName);

    char* dateOfManufacture;
    status = alljoyn_aboutdata_getdateofmanufacture(aboutData, &dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->dateofmanufacture, dateOfManufacture);

    char* manufacturer;
    status = alljoyn_aboutdata_getmanufacturer(aboutData, &manufacturer, ENGLISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->manufacturer, manufacturer);

    char* description;
    status = alljoyn_aboutdata_getdescription(aboutData, &description, ENGLISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->description, description);

    char* modelNumber;
    status = alljoyn_aboutdata_getmodelnumber(aboutData, &modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->modelnumber, modelNumber);

    char* softwareVersion;
    status = alljoyn_aboutdata_getsoftwareversion(aboutData, &softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(engLangData->softwareversion, softwareVersion);

    status = alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    alljoyn_aboutproxy_destroy(proxy);
    alljoyn_msgarg_destroy(dataArg);
    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_sessionopts_destroy(sessionOpts);
    destroy_about_proxy_test_bus_object(busObject);
    destroy_about_obj_test_about_listener_2(aboutListener);
    alljoyn_aboutobj_destroy(aboutObj);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutProxyTest, GetAboutdata_Spanish) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceNameQcc = "test.about.c" + interface_rand_string.ToString();
    qcc::String interfaceQcc =  "<node>"
                               "<interface name='" + ifaceNameQcc + "'>"
                               "</interface>"
                               "</node>";

    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_proxy_test_bus_object* busObject =
        create_about_proxy_test_bus_object(serviceBus, "/test/alljoyn/Spanish",
                                           ifaceName, QCC_TRUE);

    status = alljoyn_busattachment_registerbusobject(serviceBus,
                                                     busObject->aboutobject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutProxyTestClient", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* aboutListener = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    status = alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutSpanishData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_TRUE(aboutListener->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(alljoyn_busattachment_getuniquename(serviceBus), aboutListener->busName);
    EXPECT_EQ(port, aboutListener->port);

    alljoyn_sessionid sessionId;
    alljoyn_sessionopts sessionOpts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    alljoyn_busattachment_enableconcurrentcallbacks(clientBus);
    status = alljoyn_busattachment_joinsession(clientBus, aboutListener->busName,
                                               aboutListener->port, NULL,
                                               &sessionId, sessionOpts);
    alljoyn_aboutproxy proxy = alljoyn_aboutproxy_create(clientBus, aboutListener->busName, sessionId);
    uint16_t aboutVersion;
    status = alljoyn_aboutproxy_getversion(proxy, &aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);
    EXPECT_EQ(aboutListener->version, aboutVersion) << "Version mismatch!";

    /* AboutData for Spanish */
    alljoyn_msgarg dataArg = alljoyn_msgarg_create();
    status = alljoyn_aboutproxy_getaboutdata(proxy, SPANISH_TAG, dataArg);
    EXPECT_EQ(ER_OK, status) << "  GetAboutData Status: " << QCC_StatusText(status);
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create_empty();
    /* About data for Spanish */
    status = alljoyn_aboutdata_createfrommsgarg(aboutData, dataArg, SPANISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  GetAboutData Status: " << QCC_StatusText(status);
    char* appName;
    status = alljoyn_aboutdata_getappname(aboutData, &appName, SPANISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spLangData->appname, appName);

    char* deviceName;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, SPANISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spLangData->devicename, deviceName);

    char* manufacturer;
    status = alljoyn_aboutdata_getmanufacturer(aboutData, &manufacturer, SPANISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spLangData->manufacturer, manufacturer);

    char* description;
    status = alljoyn_aboutdata_getdescription(aboutData, &description, SPANISH_TAG);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spLangData->description, description);

    status = alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    alljoyn_aboutproxy_destroy(proxy);
    alljoyn_msgarg_destroy(dataArg);
    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_sessionopts_destroy(sessionOpts);
    destroy_about_proxy_test_bus_object(busObject);
    destroy_about_obj_test_about_listener_2(aboutListener);
    alljoyn_aboutobj_destroy(aboutObj);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutProxyTest, GetAboutdata_Unsupported) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceNameQcc = "test.about.d" + interface_rand_string.ToString();
    qcc::String interfaceQcc =  "<node>"
                               "<interface name='" + ifaceNameQcc + "'>"
                               "</interface>"
                               "</node>";

    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_proxy_test_bus_object* busObject =
        create_about_proxy_test_bus_object(serviceBus,
                                           "/test/alljoyn/Unsupported",
                                           ifaceName, QCC_TRUE);

    status = alljoyn_busattachment_registerbusobject(serviceBus, busObject->aboutobject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutProxyTestClient", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* aboutListener = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    status = alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutEnglishData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_TRUE(aboutListener->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(alljoyn_busattachment_getuniquename(serviceBus), aboutListener->busName);
    EXPECT_EQ(port, aboutListener->port);

    alljoyn_sessionid sessionId;
    alljoyn_sessionopts sessionOpts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    alljoyn_busattachment_enableconcurrentcallbacks(clientBus);
    status = alljoyn_busattachment_joinsession(clientBus, aboutListener->busName,
                                               aboutListener->port, NULL,
                                               &sessionId, sessionOpts);
    alljoyn_aboutproxy proxy = alljoyn_aboutproxy_create(clientBus,
                                                         aboutListener->busName,
                                                         sessionId);
    uint16_t aboutVersion;
    status = alljoyn_aboutproxy_getversion(proxy, &aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);
    EXPECT_EQ(aboutListener->version, aboutVersion) << "Version mismatch!";

    /* AboutData for English */
    alljoyn_msgarg dataArg = alljoyn_msgarg_create();
    status = alljoyn_aboutproxy_getaboutdata(proxy, FRENCH_TAG, dataArg);
    EXPECT_EQ(ER_LANGUAGE_NOT_SUPPORTED, status)
        << "  GetAboutData Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    alljoyn_aboutproxy_destroy(proxy);
    alljoyn_msgarg_destroy(dataArg);
    alljoyn_sessionopts_destroy(sessionOpts);
    destroy_about_proxy_test_bus_object(busObject);
    destroy_about_obj_test_about_listener_2(aboutListener);
    alljoyn_aboutobj_destroy(aboutObj);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(clientBus);
}
