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
#include <alljoyn_c/AboutIcon.h>
#include <alljoyn_c/AboutIconObj.h>
#include <alljoyn_c/AboutIconProxy.h>
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/AjAPI.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include "ajTestCommon.h"
#include <BusInternal.h>

#include <alljoyn/AboutIcon.h>
#include <alljoyn/AboutIconObj.h>

using namespace ajn;

static const size_t MAX_ICON_SIZE_IN_BYTES = ALLJOYN_MAX_ARRAY_LEN;
static const uint8_t ICON_BYTE = 0x11;

typedef struct large_icon_t {
    uint8_t* iconByteArrayPtr;
    size_t iconSize;
} large_icon;

static large_icon* create_large_icon(size_t iconsize)
{
    large_icon* result = (large_icon*) malloc(sizeof(large_icon));
    size_t iconByte = 0;
    result->iconSize = iconsize;
    result->iconByteArrayPtr = (uint8_t*) malloc(sizeof(uint8_t) * iconsize);
    if (!(result->iconByteArrayPtr)) {
        free(result);
        return NULL;
    }
    for (; iconByte < iconsize; iconByte++) {
        result->iconByteArrayPtr[iconByte] = ICON_BYTE;
    }
    return result;
}

static void destroy_large_icon(large_icon* icon)
{
    if (!icon) {
        return;
    }

    if (icon->iconByteArrayPtr) {
        free(icon->iconByteArrayPtr);
        icon->iconByteArrayPtr = NULL;
    }

    free(icon);
    icon = NULL;
}

TEST(AboutIconTest, isAnnounced) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment g_msgBus =
        alljoyn_busattachment_create("AboutIconTest", QCC_TRUE);
    status = alljoyn_busattachment_start(g_msgBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(g_msgBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_abouticon icon = alljoyn_abouticon_create();

    status = alljoyn_abouticon_seturl(icon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_abouticonobj aboutIcon = alljoyn_abouticonobj_create(g_msgBus, icon);

    alljoyn_msgarg aodArg = alljoyn_msgarg_create();
    status = (*(ajn::BusAttachment*) g_msgBus).GetInternal().GetAnnouncedObjectDescription(*(ajn::MsgArg*)aodArg);

    alljoyn_aboutobjectdescription aod =
        alljoyn_aboutobjectdescription_create_full(aodArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/About/DeviceIcon"));

    alljoyn_abouticonobj_destroy(aboutIcon);
    status = alljoyn_busattachment_stop(g_msgBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(g_msgBus);
    alljoyn_busattachment_destroy(g_msgBus);
}

TEST(AboutIconTest, GetVersion) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment serviceBus =
        alljoyn_busattachment_create("AboutIconTest service", QCC_TRUE);
    status = alljoyn_busattachment_start(serviceBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(serviceBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticon icon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(icon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIcon = alljoyn_abouticonobj_create(serviceBus, icon);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutIconTest client", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(clientBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* serviceBusName = alljoyn_busattachment_getuniquename(serviceBus);
    alljoyn_abouticonproxy aiProxy = alljoyn_abouticonproxy_create(clientBus,
                                                                   serviceBusName,
                                                                   0);
    uint16_t version;
    status = alljoyn_abouticonproxy_getversion(aiProxy, &version);
    EXPECT_EQ(((ajn::AboutIconObj*)aboutIcon)->VERSION, version);

    alljoyn_abouticon_destroy(icon);
    alljoyn_abouticonobj_destroy(aboutIcon);
    alljoyn_abouticonproxy_destroy(aiProxy);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    alljoyn_busattachment_destroy(clientBus);
    status = alljoyn_busattachment_stop(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(serviceBus);
    alljoyn_busattachment_destroy(serviceBus);
}

TEST(AboutIconTest, GetUrl) {
    QStatus status = ER_FAIL;

    alljoyn_busattachment serviceBus =
        alljoyn_busattachment_create("AboutIconTest Service", QCC_TRUE);
    status = alljoyn_busattachment_start(serviceBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(serviceBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_abouticon icon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(icon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_abouticonobj aboutIcon = alljoyn_abouticonobj_create(serviceBus, icon);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutIconTest Client", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* serviceBusName = alljoyn_busattachment_getuniquename(serviceBus);
    alljoyn_abouticonproxy aiProxy =
        alljoyn_abouticonproxy_create(clientBus, serviceBusName, 0);
    alljoyn_abouticon iconReceived = alljoyn_abouticon_create();
    status = alljoyn_abouticonproxy_geticon(aiProxy, iconReceived);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* url = ((ajn::AboutIcon*)iconReceived)->url.c_str();
    EXPECT_STREQ("http://www.example.com", url);

    alljoyn_abouticon_destroy(icon);
    alljoyn_abouticon_destroy(iconReceived);
    alljoyn_abouticonobj_destroy(aboutIcon);
    alljoyn_abouticonproxy_destroy(aiProxy);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    status = alljoyn_busattachment_stop(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(serviceBus);
    alljoyn_busattachment_destroy(clientBus);
    alljoyn_busattachment_destroy(serviceBus);
}

TEST(AboutIconTest, GetIcon) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment serviceBus =
        alljoyn_busattachment_create("AboutIconTest Service", QCC_TRUE);
    status = alljoyn_busattachment_start(serviceBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(serviceBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t aboutIconContent[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
                                   0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0A,
                                   0x00, 0x00, 0x00, 0x0A, 0x08, 0x02, 0x00, 0x00, 0x00, 0x02,
                                   0x50, 0x58, 0xEA, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D,
                                   0x41, 0x00, 0x00, 0xAF, 0xC8, 0x37, 0x05, 0x8A, 0xE9, 0x00,
                                   0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66,
                                   0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62,
                                   0x65, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61,
                                   0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x18,
                                   0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x62, 0xFC, 0x3F, 0x95,
                                   0x9F, 0x01, 0x37, 0x60, 0x62, 0xC0, 0x0B, 0x46, 0xAA, 0x34,
                                   0x40, 0x80, 0x01, 0x00, 0x06, 0x7C, 0x01, 0xB7, 0xED, 0x4B,
                                   0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
                                   0xAE, 0x42, 0x60, 0x82 };

    alljoyn_abouticon icon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_setcontent(icon, "image/png", aboutIconContent,
                                          sizeof(aboutIconContent) / sizeof(aboutIconContent[0]),
                                          false);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_abouticonobj aboutIcon = alljoyn_abouticonobj_create(serviceBus, icon);

    alljoyn_busattachment clientBus = alljoyn_busattachment_create("AboutIconTest Client", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(clientBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* serviceBusName = alljoyn_busattachment_getuniquename(serviceBus);

    alljoyn_abouticonproxy aiProxy =
        alljoyn_abouticonproxy_create(clientBus, serviceBusName, 0);

    alljoyn_abouticon iconReceived = alljoyn_abouticon_create();

    status = alljoyn_abouticonproxy_geticon(aiProxy, iconReceived);

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("image/png", ((ajn::AboutIcon*)iconReceived)->mimetype.c_str());
    ASSERT_EQ(sizeof(aboutIconContent) / sizeof(aboutIconContent[0]),
              ((ajn::AboutIcon*)iconReceived)->contentSize);

    for (size_t i = 0; i < ((ajn::AboutIcon*)iconReceived)->contentSize; ++i) {
        EXPECT_EQ(aboutIconContent[i], ((ajn::AboutIcon*)iconReceived)->content[i]);
    }

    EXPECT_STREQ(((ajn::AboutIcon*)icon)->mimetype.c_str(),
                 ((ajn::AboutIcon*)iconReceived)->mimetype.c_str());

    alljoyn_abouticon_destroy(icon);
    alljoyn_abouticon_destroy(iconReceived);
    alljoyn_abouticonobj_destroy(aboutIcon);
    alljoyn_abouticonproxy_destroy(aiProxy);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    alljoyn_busattachment_destroy(clientBus);
    status = alljoyn_busattachment_stop(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(serviceBus);
    alljoyn_busattachment_destroy(serviceBus);
}

TEST(AboutIconTest, GetLargeIcon) {
    QStatus status = ER_FAIL;
    large_icon* myIcon = create_large_icon(MAX_ICON_SIZE_IN_BYTES);

    ASSERT_TRUE(myIcon &&
                NULL != myIcon->iconByteArrayPtr &&
                myIcon->iconSize == MAX_ICON_SIZE_IN_BYTES);

    alljoyn_busattachment serviceBus =
        alljoyn_busattachment_create("AboutLargeIconTest Service", QCC_TRUE);
    status = alljoyn_busattachment_start(serviceBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(serviceBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* mimeType = "image/png";
    alljoyn_abouticon icon = alljoyn_abouticon_create();
    alljoyn_abouticon_setcontent(icon, mimeType, myIcon->iconByteArrayPtr,
                                 myIcon->iconSize, QCC_FALSE);

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIcon = alljoyn_abouticonobj_create(serviceBus, icon);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutLargeIconTest Client", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(clientBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* serviceBusName = alljoyn_busattachment_getuniquename(serviceBus);
    alljoyn_abouticonproxy aiProxy =
        alljoyn_abouticonproxy_create(clientBus, serviceBusName, 0);
    alljoyn_abouticon iconOut = alljoyn_abouticon_create();
    status = alljoyn_abouticonproxy_geticon(aiProxy, iconOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("image/png", ((ajn::AboutIcon*)iconOut)->mimetype.c_str());
    ASSERT_EQ(MAX_ICON_SIZE_IN_BYTES, ((ajn::AboutIcon*)iconOut)->contentSize);

    for (size_t i = 0; i < ((ajn::AboutIcon*)iconOut)->contentSize; ++i) {
        EXPECT_EQ(myIcon->iconByteArrayPtr[i],
                  ((ajn::AboutIcon*)iconOut)->content[i]);
    }

    EXPECT_EQ(MAX_ICON_SIZE_IN_BYTES, ((ajn::AboutIcon*)iconOut)->contentSize);

    EXPECT_STREQ(mimeType, ((ajn::AboutIcon*)iconOut)->mimetype.c_str());

    destroy_large_icon(myIcon);
    alljoyn_abouticon_destroy(icon);
    alljoyn_abouticonobj_destroy(aboutIcon);
    alljoyn_abouticonproxy_destroy(aiProxy);
    alljoyn_abouticon_destroy(iconOut);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    alljoyn_busattachment_destroy(clientBus);
    status = alljoyn_busattachment_stop(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(serviceBus);
    alljoyn_busattachment_destroy(serviceBus);
}

TEST(AboutIconTest, GetLargeIcon_Negative) {
    QStatus status = ER_FAIL;
    large_icon* myIcon = create_large_icon(MAX_ICON_SIZE_IN_BYTES + 1);

    ASSERT_TRUE(myIcon &&
                NULL != myIcon->iconByteArrayPtr &&
                myIcon->iconSize == MAX_ICON_SIZE_IN_BYTES + 1);

    alljoyn_busattachment serviceBus =
        alljoyn_busattachment_create("AboutLargeIconTest Service", QCC_TRUE);
    status = alljoyn_busattachment_start(serviceBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(serviceBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* mimeType = "image/png";
    alljoyn_abouticon iconIn = alljoyn_abouticon_create();
    status =
        alljoyn_abouticon_setcontent(iconIn, mimeType,
                                     myIcon->iconByteArrayPtr,
                                     (MAX_ICON_SIZE_IN_BYTES + 1),
                                     QCC_FALSE);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIcon = alljoyn_abouticonobj_create(serviceBus, iconIn);

    alljoyn_busattachment clientBus = alljoyn_busattachment_create("AboutLargeIconTest Client", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(clientBus, NULL);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* serviceBusName = alljoyn_busattachment_getuniquename(serviceBus);
    alljoyn_abouticonproxy aiProxy =
        alljoyn_abouticonproxy_create(clientBus, serviceBusName, 0);

    alljoyn_abouticon iconOut = alljoyn_abouticon_create();
    status = alljoyn_abouticonproxy_geticon(aiProxy, iconOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("", ((ajn::AboutIcon*)iconOut)->mimetype.c_str());

    ASSERT_EQ(0u, ((ajn::AboutIcon*)iconOut)->contentSize);

    destroy_large_icon(myIcon);
    alljoyn_abouticon_destroy(iconIn);
    alljoyn_abouticonobj_destroy(aboutIcon);
    alljoyn_abouticonproxy_destroy(aiProxy);
    alljoyn_abouticon_destroy(iconOut);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(clientBus);
    alljoyn_busattachment_destroy(clientBus);
    status = alljoyn_busattachment_stop(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(serviceBus);
    alljoyn_busattachment_destroy(serviceBus);
}
