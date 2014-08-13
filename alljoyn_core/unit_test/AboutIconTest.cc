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
#include <alljoyn/AboutIconObj.h>
#include <alljoyn/AboutIconProxy.h>
#include <alljoyn/BusAttachment.h>

using namespace ajn;

TEST(AboutIconTest, isAnnounced) {
    QStatus status = ER_FAIL;
    BusAttachment busAttachment("AboutIconTest", true);
    status = busAttachment.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busAttachment.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutIconObj aboutIcon(busAttachment, "", "http://www.test.com", NULL, (size_t)0);
    AboutObjectDescription aod;
    status = busAttachment.GetAboutObjectDescription(aod);
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

    AboutIconObj aboutIcon(serviceBus, "", "http://www.test.com", NULL, (size_t)0);

    BusAttachment clientBus("AboutIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus);
    qcc::String url;
    aiProxy.GetUrl(serviceBus.GetUniqueName().c_str(), url, 0);
    EXPECT_STREQ("http://www.test.com", url.c_str());
}

TEST(AboutIconTest, GetVersion) {
    QStatus status = ER_FAIL;
    BusAttachment serviceBus("AboutIconTest Service");
    status = serviceBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconObj aboutIcon(serviceBus, "", "http://www.test.com", NULL, (size_t)0);

    BusAttachment clientBus("AboutIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus);
    uint16_t version;
    aiProxy.GetVersion(serviceBus.GetUniqueName().c_str(), version, 0);
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
    qcc::String mimeType("image/png");

    AboutIconObj aboutIcon(serviceBus, mimeType, "", aboutIconContent, sizeof(aboutIconContent) / sizeof(aboutIconContent[0]));

    BusAttachment clientBus("AboutIconTest Client");
    status = clientBus.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientBus.Connect();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutIconProxy aiProxy(clientBus);

    AboutIconProxy::Icon icon;
    aiProxy.GetIcon(serviceBus.GetUniqueName().c_str(), icon, 0);
    EXPECT_STREQ("image/png", icon.mimetype.c_str());
    ASSERT_EQ(sizeof(aboutIconContent) / sizeof(aboutIconContent[0]), icon.contentSize);
    for (size_t i = 0; i < icon.contentSize; ++i) {
        EXPECT_EQ(aboutIconContent[i], icon.content[i]);
    }

    size_t icon_size;
    aiProxy.GetSize(serviceBus.GetUniqueName().c_str(), icon_size, 0);
    EXPECT_EQ(sizeof(aboutIconContent) / sizeof(aboutIconContent[0]), icon_size);

    qcc::String retMimeType;
    aiProxy.GetMimeType(serviceBus.GetUniqueName().c_str(), retMimeType, 0);
    EXPECT_STREQ(mimeType.c_str(), retMimeType.c_str());
}
