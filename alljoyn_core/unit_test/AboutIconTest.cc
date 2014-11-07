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

#include <gtest/gtest.h>
#include <alljoyn/AboutIcon.h>
#include <alljoyn/AboutIconObj.h>
#include <alljoyn/AboutIconProxy.h>
#include <alljoyn/BusAttachment.h>
#include <BusInternal.h>

using namespace ajn;

TEST(AboutIconTest, isAnnounced) {
    QStatus status = ER_FAIL;
    BusAttachment busAttachment("AboutIconTest", true);
    status = busAttachment.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busAttachment.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIcon icon;
    status = icon.SetUrl("image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(busAttachment, icon);

    MsgArg aodArg;
    status = busAttachment.GetInternal().GetAnnouncedObjectDescription(aodArg);
    AboutObjectDescription aod;
    aod.CreateFromMsgArg(aodArg);

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aod.HasPath("/About/DeviceIcon"));
    EXPECT_TRUE(aod.HasInterface("org.alljoyn.Icon"));
    EXPECT_TRUE(aod.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon"));

    busAttachment.Stop();
    busAttachment.Join();
}

TEST(AboutIconTest, GetUrl) {
    QStatus status = ER_FAIL;
    BusAttachment serviceBus("AboutIconTest Service");
    status = serviceBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIcon icon;
    status = icon.SetUrl("image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(serviceBus, icon);

    BusAttachment clientBus("AboutIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus, serviceBus.GetUniqueName().c_str());
    AboutIcon icon_url;
    aiProxy.GetIcon(icon_url);
    EXPECT_STREQ("http://www.example.com", icon_url.url.c_str());
}

TEST(AboutIconTest, GetVersion) {
    QStatus status = ER_FAIL;
    BusAttachment serviceBus("AboutIconTest Service");
    status = serviceBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIcon icon;
    status = icon.SetUrl("image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(serviceBus, icon);

    BusAttachment clientBus("AboutIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus, serviceBus.GetUniqueName().c_str());
    uint16_t version;
    aiProxy.GetVersion(version);
    EXPECT_EQ(aboutIcon.VERSION, version);
}

TEST(AboutIconTest, GetIcon) {
    QStatus status = ER_FAIL;
    BusAttachment serviceBus("AboutIconTest Service");
    status = serviceBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.Connect();
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

    AboutIcon icon;
    status = icon.SetContent("image/png", aboutIconContent, sizeof(aboutIconContent) / sizeof(aboutIconContent[0]));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(serviceBus, icon);

    BusAttachment clientBus("AboutIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus, serviceBus.GetUniqueName().c_str());

    AboutIcon retIcon;
    status = aiProxy.GetIcon(retIcon);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("image/png", retIcon.mimetype.c_str());
    ASSERT_EQ(sizeof(aboutIconContent) / sizeof(aboutIconContent[0]), retIcon.contentSize);
    for (size_t i = 0; i < retIcon.contentSize; ++i) {
        EXPECT_EQ(aboutIconContent[i], retIcon.content[i]);
    }

    EXPECT_STREQ(icon.mimetype.c_str(), retIcon.mimetype.c_str());
}

static const size_t MAX_ICON_SIZE_IN_BYTES = ALLJOYN_MAX_ARRAY_LEN;
static const uint8_t ICON_BYTE = 0x11;

class LargeIcon {
  private:
    uint8_t* iconByteArrayPtr;
    size_t iconSize;

  public:

    LargeIcon() : iconSize(0)
    {

        iconByteArrayPtr = new uint8_t[MAX_ICON_SIZE_IN_BYTES];
        if (iconByteArrayPtr) {
            for (size_t iconByte = 0; iconByte < MAX_ICON_SIZE_IN_BYTES; iconByte++) {
                iconByteArrayPtr[iconByte] = ICON_BYTE;
            }
            iconSize = MAX_ICON_SIZE_IN_BYTES;
        }
    }

    LargeIcon(size_t icon_size) : iconSize(0)
    {

        iconByteArrayPtr = new uint8_t[icon_size];
        if (iconByteArrayPtr) {
            for (size_t iconByte = 0; iconByte < icon_size; iconByte++) {
                iconByteArrayPtr[iconByte] = ICON_BYTE;
            }
            iconSize = icon_size;
        }
    }

    ~LargeIcon()
    {
        if (iconByteArrayPtr) {
            delete [] iconByteArrayPtr;
            iconByteArrayPtr = NULL;
            iconSize = 0;
        }

    }

    uint8_t* getLargeIconArray()
    {
        return iconByteArrayPtr;
    }

    size_t getIconSize()
    {
        return iconSize;
    }
};

TEST(AboutIconTest, GetLargeIcon) {
    QStatus status = ER_FAIL;

    LargeIcon myIcon;

    uint8_t* aboutIconContent = myIcon.getLargeIconArray();
    size_t icon_size = myIcon.getIconSize();

    ASSERT_TRUE(NULL != aboutIconContent && icon_size == MAX_ICON_SIZE_IN_BYTES);

    BusAttachment serviceBus("AboutLargeIconTest Service");
    status = serviceBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    qcc::String mimeType("image/png");

    AboutIcon icon;
    status = icon.SetContent(mimeType.c_str(), aboutIconContent, MAX_ICON_SIZE_IN_BYTES);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(serviceBus, icon);

    BusAttachment clientBus("AboutLargeIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus, serviceBus.GetUniqueName().c_str(), 0);

    AboutIcon iconOut;
    status = aiProxy.GetIcon(iconOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("image/png", iconOut.mimetype.c_str());
    ASSERT_EQ(MAX_ICON_SIZE_IN_BYTES, iconOut.contentSize);

    for (size_t i = 0; i < iconOut.contentSize; ++i) {
        EXPECT_EQ(aboutIconContent[i], iconOut.content[i]);
    }

    EXPECT_EQ(MAX_ICON_SIZE_IN_BYTES, iconOut.contentSize);

    EXPECT_STREQ(mimeType.c_str(), iconOut.mimetype.c_str());

}
//ASACORE-944
TEST(AboutIconTest, GetLargeIcon_Negative) {
    QStatus status = ER_FAIL;

    // icon over array limit
    LargeIcon myIcon((MAX_ICON_SIZE_IN_BYTES + 1));

    uint8_t* aboutIconContent = myIcon.getLargeIconArray();
    size_t icon_size = myIcon.getIconSize();

    ASSERT_TRUE((NULL != aboutIconContent) && (icon_size == MAX_ICON_SIZE_IN_BYTES + 1));

    BusAttachment serviceBus("AboutLargeIconTest Service");
    status = serviceBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    qcc::String mimeType("image/png");

    AboutIcon iconIn;
    status = iconIn.SetContent(mimeType.c_str(), aboutIconContent, (MAX_ICON_SIZE_IN_BYTES + 1));
    EXPECT_EQ(ER_BUS_BAD_VALUE, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(serviceBus, iconIn);

    BusAttachment clientBus("AboutLargeIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus, serviceBus.GetUniqueName().c_str(), 0);

    AboutIcon iconOut;
    // Should be an empty icon since the SetContent failed.
    status = aiProxy.GetIcon(iconOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("", iconOut.mimetype.c_str());

    ASSERT_EQ(0u, iconOut.contentSize);
}
