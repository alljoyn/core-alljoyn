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
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/Message.h>
#include <qcc/Util.h>
#include "ajTestCommon.h"

/* Add canary values before and after output string buffers, to detect typical overruns/underruns */
#define CANARY_SIZE (sizeof(void*))

/* Using a macro rather than of a function allows EXPECT_EQ to print the correct source code line number of the VERIFY_CANARY_VALUES caller */
#define VERIFY_CANARY_VALUES(buffer, stringSize, canaryByte)            \
    {                                                                   \
        uint32_t byteIndex;                                             \
        char* localBuffer = (buffer);                                   \
                                                                        \
        /* Check for underruns */                                       \
        for (byteIndex = 0; byteIndex < CANARY_SIZE; byteIndex += 1) {  \
            EXPECT_EQ((canaryByte), *localBuffer);                      \
            localBuffer += 1;                                           \
        }                                                               \
                                                                        \
        /* Check for overruns */                                        \
        localBuffer += (stringSize);                                    \
                                                                        \
        for (byteIndex = 0; byteIndex < CANARY_SIZE; byteIndex += 1) {  \
            EXPECT_EQ((canaryByte), *localBuffer);                      \
            localBuffer += 1;                                           \
        }                                                               \
    }

static uint16_t translationCount = 0;

const char* translation_callback(const char* sourceLanguage, const char* targetLanguage, const char* sourceText)
{
    static qcc::String translated;

    translationCount++;
    if (sourceLanguage == targetLanguage) {
        return sourceText;
    } else if ((targetLanguage == nullptr) || (targetLanguage[0] == '\0')) {
        /* if target language is not specified, you may choose to use sourceText as a lookup id */
        translated = qcc::String("id:") + sourceText;
    } else {
        translated = qcc::String(targetLanguage) + ":" + sourceText;
    }
    return translated.c_str();
}

void introspect_and_compare(const alljoyn_interfacedescription& testIntf, const char* expectedIntrospect)
{
    char* introspect;
    size_t bufSize = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    introspect = (char*)malloc(sizeof(char) * bufSize);
    size_t bufSize2 = alljoyn_interfacedescription_introspect(testIntf, introspect, bufSize, 0);
    EXPECT_EQ(bufSize, bufSize2);
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);
}

TEST(InterfaceDescriptionTest, addmember) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getmember) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "s", NULL, "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_member member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &member));

    EXPECT_EQ(testIntf, member.iface);
    EXPECT_EQ(ALLJOYN_MESSAGE_METHOD_CALL, member.memberType);
    EXPECT_STREQ("ping", member.name);
    EXPECT_STREQ("s", member.signature);
    EXPECT_STREQ("s", member.returnSignature);
    EXPECT_STREQ("in,out", member.argNames);

    size_t annotation_count;
    annotation_count = alljoyn_interfacedescription_member_getannotationscount(member);
    EXPECT_EQ((size_t)0, annotation_count);

    alljoyn_interfacedescription_member member2;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "chirp", &member2));

    EXPECT_EQ(testIntf, member2.iface);
    EXPECT_EQ(ALLJOYN_MESSAGE_SIGNAL, member2.memberType);
    EXPECT_STREQ("chirp", member2.name);
    EXPECT_STREQ("s", member2.signature);
    EXPECT_STREQ("", member2.returnSignature);
    EXPECT_STREQ("chirp", member2.argNames);

    annotation_count = alljoyn_interfacedescription_member_getannotationscount(member2);
    EXPECT_EQ((size_t)0, annotation_count);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getmembers) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "s", NULL, "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_member member[6];
    size_t size;
    size = alljoyn_interfacedescription_getmembers(testIntf, NULL, 0);
    EXPECT_EQ((size_t)2, size);

    size = alljoyn_interfacedescription_getmembers(testIntf, member, 6);
    EXPECT_EQ((size_t)2, size);

    /*
     * NOTE there is nothing that specifies the order the members are organized
     * when they are added to the interface.  As can be seen here even though
     * the 'chirp' signal was added to the interface after 'ping' it comes out
     * of the interface before 'ping'. This result is based on actual program
     * behaver.
     */
    EXPECT_EQ(testIntf, member[0].iface);
    EXPECT_EQ(ALLJOYN_MESSAGE_SIGNAL, member[0].memberType);
    EXPECT_STREQ("chirp", member[0].name);
    EXPECT_STREQ("s", member[0].signature);
    EXPECT_STREQ("", member[0].returnSignature);
    EXPECT_STREQ("chirp", member[0].argNames);

    size_t annotation_count;
    annotation_count = alljoyn_interfacedescription_member_getannotationscount(member[0]);
    EXPECT_EQ((size_t)0, annotation_count);

    EXPECT_EQ(testIntf, member[1].iface);
    EXPECT_EQ(ALLJOYN_MESSAGE_METHOD_CALL, member[1].memberType);
    EXPECT_STREQ("ping", member[1].name);
    EXPECT_STREQ("s", member[1].signature);
    EXPECT_STREQ("s", member[1].returnSignature);
    EXPECT_STREQ("in,out", member[1].argNames);

    annotation_count = alljoyn_interfacedescription_member_getannotationscount(member[1]);
    EXPECT_EQ((size_t)0, annotation_count);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, hasmembers) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "s", NULL, "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_interfacedescription_hasmember(testIntf, "ping", "s", "s"));
    EXPECT_TRUE(alljoyn_interfacedescription_hasmember(testIntf, "chirp", "s", NULL));

    /*
     * expected to be false even though the members exist the signatures do not
     * match what is expected.
     */
    EXPECT_FALSE(alljoyn_interfacedescription_hasmember(testIntf, "ping", "i", "s"));
    EXPECT_FALSE(alljoyn_interfacedescription_hasmember(testIntf, "chirp", "b", NULL));

    EXPECT_FALSE(alljoyn_interfacedescription_hasmember(testIntf, "invalid", "s", NULL));

    alljoyn_busattachment_destroy(bus);
}
TEST(InterfaceDescriptionTest, activate) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);
    /* once the interface has been activated we should not be able to add new members */
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "pong", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_BUS_INTERFACE_ACTIVATED, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, introspect) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* introspect;
    size_t bufSize  = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    introspect = (char*)malloc((sizeof(char) * bufSize) + (2 * CANARY_SIZE));

    memset(introspect, 'A', bufSize + (2 * CANARY_SIZE));
    size_t bufSize2 = alljoyn_interfacedescription_introspect(testIntf, introspect + CANARY_SIZE, bufSize, 0);
    EXPECT_EQ(bufSize, bufSize2);

    /*
     * NOTE there is nothing that specifies the order the members are organized
     * when they are added to the interface.  As can be seen here even though
     * the 'chirp' signal was added to the interface after 'ping' it is listed
     * before 'ping'. This result is based on actual program behaver.
     */
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <signal name=\"chirp\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "  </signal>\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect + CANARY_SIZE);
    VERIFY_CANARY_VALUES(introspect, bufSize, 'A');

    memset(introspect, 'B', bufSize + (2 * CANARY_SIZE));
    size_t badBufSize = 0;
    bufSize2 = alljoyn_interfacedescription_introspect(testIntf, introspect + CANARY_SIZE, badBufSize, 0);
    EXPECT_EQ(bufSize, bufSize2);
    VERIFY_CANARY_VALUES(introspect, bufSize, 'B');

    memset(introspect, 'C', bufSize + (2 * CANARY_SIZE));
    size_t tooSmallSize = 1;
    bufSize2 = alljoyn_interfacedescription_introspect(testIntf, introspect + CANARY_SIZE, tooSmallSize, 0);
    EXPECT_EQ(bufSize, bufSize2);
    EXPECT_STREQ("", introspect + CANARY_SIZE); //empty string instead of expectedIntrospect
    VERIFY_CANARY_VALUES(introspect, bufSize, 'C');

    memset(introspect, 'D', bufSize + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    bufSize2 = alljoyn_interfacedescription_introspect(testIntf, introspect + CANARY_SIZE, tooSmallSize, 0);
    EXPECT_EQ(bufSize, bufSize2);
    EXPECT_STREQ("<", introspect + CANARY_SIZE); //first character of expectedIntrospect
    VERIFY_CANARY_VALUES(introspect, bufSize, 'D');

    free(introspect);
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, issecure) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface_secure(bus, "org.alljoyn.test.InterfaceDescription", &testIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    EXPECT_EQ(QCC_TRUE, alljoyn_interfacedescription_issecure(testIntf));
    status = alljoyn_busattachment_deleteinterface(bus, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    EXPECT_EQ(QCC_FALSE, alljoyn_interfacedescription_issecure(testIntf));
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, addproperty) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntf, introspect, buf, 0);
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <property name=\"prop1\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"prop2\" type=\"i\" access=\"write\"/>\n"
        "  <property name=\"prop3\" type=\"u\" access=\"readwrite\"/>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, hasproperty) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_interfacedescription_hasproperty(testIntf, "prop1"));
    EXPECT_TRUE(alljoyn_interfacedescription_hasproperty(testIntf, "prop2"));
    EXPECT_TRUE(alljoyn_interfacedescription_hasproperty(testIntf, "prop3"));
    EXPECT_FALSE(alljoyn_interfacedescription_hasproperty(testIntf, "invalid_prop"));
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, hasproperties) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    /*
     * At this point this is an empty interface the call to hasproperties should
     * return false.
     */
    EXPECT_FALSE(alljoyn_interfacedescription_hasproperties(testIntf));
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*
     * At this point the interface only contains a method call the call to
     * hasproperties should return false.
     */
    EXPECT_FALSE(alljoyn_interfacedescription_hasproperties(testIntf));
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /*
     * At this point the interface only contains a property the call to
     * hasproperties should return true.
     */
    EXPECT_TRUE(alljoyn_interfacedescription_hasproperties(testIntf));
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*
     * At this point the interface only contains multiple properties the call to
     * hasproperties should return true.
     */
    EXPECT_TRUE(alljoyn_interfacedescription_hasproperties(testIntf));

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getname) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("org.alljoyn.test.InterfaceDescription", alljoyn_interfacedescription_getname(testIntf));

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, addmethod) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmethod(testIntf, "method1", "ss", "b", "string1,string2,bool", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntf, introspect, buf, 0);
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <method name=\"method1\">\n"
        "    <arg name=\"string1\" type=\"s\" direction=\"in\"/>\n"
        "    <arg name=\"string2\" type=\"s\" direction=\"in\"/>\n"
        "    <arg name=\"bool\" type=\"b\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getmethod) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmethod(testIntf, "method1", "ss", "b", "string1,string2,bool", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_member member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmethod(testIntf, "method1", &member));

    EXPECT_EQ(testIntf, member.iface);
    EXPECT_EQ(ALLJOYN_MESSAGE_METHOD_CALL, member.memberType);
    EXPECT_STREQ("method1", member.name);
    EXPECT_STREQ("ss", member.signature);
    EXPECT_STREQ("b", member.returnSignature);
    EXPECT_STREQ("string1,string2,bool", member.argNames);

    size_t annotation_count;
    annotation_count = alljoyn_interfacedescription_member_getannotationscount(member);
    EXPECT_EQ((size_t)0, annotation_count);

    EXPECT_FALSE(alljoyn_interfacedescription_getmethod(testIntf, "invalid", &member));

    /*
     * since we have not called alljoyn_interfacedescription_activate it is
     * possible to continue to add new members to the interface.
     */
    status = alljoyn_interfacedescription_addsignal(testIntf, "signal1", "s", "string", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*
     * get method should return false when trying to get a signal
     */
    EXPECT_FALSE(alljoyn_interfacedescription_getmethod(testIntf, "signal1", &member));

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, addsignal) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addsignal(testIntf, "signal1", "s", "string", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntf, introspect, buf, 0);
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <signal name=\"signal1\">\n"
        "    <arg name=\"string\" type=\"s\" direction=\"out\"/>\n"
        "  </signal>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getsignal) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addsignal(testIntf, "signal1", "s", "string", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_member member;
    EXPECT_TRUE(alljoyn_interfacedescription_getsignal(testIntf, "signal1", &member));

    EXPECT_EQ(testIntf, member.iface);
    EXPECT_EQ(ALLJOYN_MESSAGE_SIGNAL, member.memberType);
    EXPECT_STREQ("signal1", member.name);
    EXPECT_STREQ("s", member.signature);
    EXPECT_STREQ("", member.returnSignature);
    EXPECT_STREQ("string", member.argNames);

    size_t annotation_count;
    annotation_count = alljoyn_interfacedescription_member_getannotationscount(member);
    EXPECT_EQ((size_t)0, annotation_count);

    EXPECT_FALSE(alljoyn_interfacedescription_getsignal(testIntf, "invalid", &member));

    /*
     * since we have not called alljoyn_interfacedescription_activate it is
     * possible to continue to add new members to the interface.
     */
    status = alljoyn_interfacedescription_addmethod(testIntf, "method1", "ss", "b", "string1,string2,bool", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_FALSE(alljoyn_interfacedescription_getsignal(testIntf, "method1", &member));

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getproperty) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_property propa;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop1", &propa));
    EXPECT_STREQ("prop1", propa.name);
    EXPECT_STREQ("s", propa.signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_READ, propa.access);
    alljoyn_interfacedescription_property propb;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop2", &propb));
    EXPECT_STREQ("prop2", propb.name);
    EXPECT_STREQ("i", propb.signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_WRITE, propb.access);
    alljoyn_interfacedescription_property propc;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop3", &propc));
    EXPECT_STREQ("prop3", propc.name);
    EXPECT_STREQ("u", propc.signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_RW, propc.access);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, getproperties) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_property prop[6];
    size_t size = 0;
    size = alljoyn_interfacedescription_getproperties(testIntf, NULL, 0);
    EXPECT_EQ((size_t)3, size);

    size = alljoyn_interfacedescription_getproperties(testIntf, prop, 6);
    EXPECT_EQ((size_t)3, size);
    EXPECT_STREQ("prop1", prop[0].name);
    EXPECT_STREQ("s", prop[0].signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_READ, prop[0].access);

    EXPECT_STREQ("prop2", prop[1].name);
    EXPECT_STREQ("i", prop[1].signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_WRITE, prop[1].access);

    EXPECT_STREQ("prop3", prop[2].name);
    EXPECT_STREQ("u", prop[2].signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_RW, prop[2].access);

    /*
     * testing to see if it will not cause a problem if the array does not have
     * enough room for all of the properties.
     */
    alljoyn_interfacedescription_property prop2[2];
    size = alljoyn_interfacedescription_getproperties(testIntf, prop2, 2);
    EXPECT_EQ((size_t)2, size);
    EXPECT_STREQ("prop1", prop2[0].name);
    EXPECT_STREQ("s", prop2[0].signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_READ, prop2[0].access);

    EXPECT_STREQ("prop2", prop2[1].name);
    EXPECT_STREQ("i", prop2[1].signature);
    EXPECT_EQ(ALLJOYN_PROP_ACCESS_WRITE, prop2[1].access);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, alljoyn_interfacedescription_member_eql)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "s", NULL, "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_member member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &member));

    alljoyn_interfacedescription_member other_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &other_member));

    alljoyn_interfacedescription_member other_member2;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "chirp", &other_member2));

    EXPECT_TRUE(alljoyn_interfacedescription_member_eql(member, other_member));

    EXPECT_FALSE(alljoyn_interfacedescription_member_eql(member, other_member2));

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, alljoyn_interfacedescription_property_eql)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_property propa;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop1", &propa));

    alljoyn_interfacedescription_property propa2;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop1", &propa2));

    alljoyn_interfacedescription_property propb;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop2", &propb));

    EXPECT_TRUE(alljoyn_interfacedescription_property_eql(propa, propa2));

    EXPECT_FALSE(alljoyn_interfacedescription_property_eql(propa, propb));
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, interface_annotations)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addannotation(testIntf, "org.alljoyn.test.annotation", "foo");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    size_t annotation_count = alljoyn_interfacedescription_getannotationscount(testIntf);
    EXPECT_EQ((size_t)1, annotation_count);

    size_t name_size;
    size_t value_size;
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, NULL, &name_size, NULL, &value_size);
    EXPECT_EQ((size_t)28, name_size); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, value_size); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}

    char* name = (char*)malloc((sizeof(char) * name_size) + (2 * CANARY_SIZE));
    char* value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(name, '9', name_size + (2 * CANARY_SIZE));
    memset(value, '8', value_size + (2 * CANARY_SIZE));
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)28, name_size); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, value_size); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    EXPECT_STREQ("org.alljoyn.test.annotation", name + CANARY_SIZE);
    EXPECT_STREQ("foo", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, '9');
    VERIFY_CANARY_VALUES(value, value_size, '8');

    memset(name, '7', name_size + (2 * CANARY_SIZE));
    memset(value, '6', value_size + (2 * CANARY_SIZE));
    size_t badNameSize = 0;
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)28, badNameSize); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, value_size); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    VERIFY_CANARY_VALUES(name, name_size, '7');
    VERIFY_CANARY_VALUES(value, value_size, '6');

    memset(name, '5', name_size + (2 * CANARY_SIZE));
    memset(value, '4', value_size + (2 * CANARY_SIZE));
    size_t badValueSize = 0;
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ((size_t)28, name_size); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, badValueSize); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    VERIFY_CANARY_VALUES(name, name_size, '5');
    VERIFY_CANARY_VALUES(value, value_size, '4');

    memset(name, '3', name_size + (2 * CANARY_SIZE));
    memset(value, '2', value_size + (2 * CANARY_SIZE));
    badNameSize = 0;
    badValueSize = 0;
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ((size_t)28, badNameSize); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, badValueSize); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    VERIFY_CANARY_VALUES(name, name_size, '3');
    VERIFY_CANARY_VALUES(value, value_size, '2');

    memset(name, '1', name_size + (2 * CANARY_SIZE));
    memset(value, '0', value_size + (2 * CANARY_SIZE));
    size_t tooSmallSize = 1;
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)28, tooSmallSize); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, value_size); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    EXPECT_STREQ("", name + CANARY_SIZE); //empty string instead of 'org.alljoyn.test.annotation'
    EXPECT_STREQ("foo", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, '1');
    VERIFY_CANARY_VALUES(value, value_size, '0');

    memset(name, 'a', name_size + (2 * CANARY_SIZE));
    memset(value, 'b', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    alljoyn_interfacedescription_getannotationatindex(testIntf, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_EQ((size_t)28, name_size); //the size of 'org.alljoyn.test.annotation' + nul
    EXPECT_EQ((size_t)4, tooSmallSize); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    EXPECT_STREQ("org.alljoyn.test.annotation", name + CANARY_SIZE); //empty string instead of 'org.alljoyn.test.annotation'
    EXPECT_STREQ("f", value + CANARY_SIZE); //first character of 'foo'
    VERIFY_CANARY_VALUES(name, name_size, 'a');
    VERIFY_CANARY_VALUES(value, value_size, 'b');

    free(name);
    free(value);

    QCC_BOOL success = alljoyn_interfacedescription_getannotation(testIntf, "org.alljoyn.test.annotation", NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)4, value_size); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}

    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 'c', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_getannotation(testIntf, "org.alljoyn.test.annotation", value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)4, value_size); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    EXPECT_STREQ("foo", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'c');

    memset(value, 'd', value_size + (2 * CANARY_SIZE));
    size_t badSize = 0;
    success = alljoyn_interfacedescription_getannotation(testIntf, "org.alljoyn.test.annotation", value + CANARY_SIZE, &badSize);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)4, badSize); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    VERIFY_CANARY_VALUES(value, value_size, 'd');

    memset(value, 'e', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_getannotation(testIntf, "org.alljoyn.test.annotation", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)4, tooSmallSize); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of 'foo'
    VERIFY_CANARY_VALUES(value, value_size, 'e');

    memset(value, 'f', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_getannotation(testIntf, "org.alljoyn.test.annotation", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)4, tooSmallSize); //the size of 'foo' + nul {'f', 'o', 'o', '\0'}
    EXPECT_STREQ("f", value + CANARY_SIZE); //first character of 'foo'
    VERIFY_CANARY_VALUES(value, value_size, 'f');

    free(value);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, method_annotations)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "one", "black_cat");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    alljoyn_interfacedescription_member method_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &method_member));

    size_t annotation_count = alljoyn_interfacedescription_member_getannotationscount(method_member);
    EXPECT_EQ((size_t)1, annotation_count);
    size_t name_size;
    size_t value_size;

    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, NULL, &name_size, NULL, &value_size);
    EXPECT_EQ((size_t)4, name_size); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul

    char* name = (char*)malloc((sizeof(char) * name_size) + (2 * CANARY_SIZE));
    char* value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(name, 'A', name_size + (2 * CANARY_SIZE));
    memset(value, 'B', value_size + (2 * CANARY_SIZE));
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)4, name_size); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    EXPECT_STREQ("one", name + CANARY_SIZE);
    EXPECT_STREQ("black_cat", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'A');
    VERIFY_CANARY_VALUES(value, value_size, 'B');

    memset(name, 'C', name_size + (2 * CANARY_SIZE));
    memset(value, 'D', value_size + (2 * CANARY_SIZE));
    size_t badNameSize = 0;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)4, badNameSize); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    VERIFY_CANARY_VALUES(name, name_size, 'C');
    VERIFY_CANARY_VALUES(value, value_size, 'D');

    memset(name, 'E', name_size + (2 * CANARY_SIZE));
    memset(value, 'F', value_size + (2 * CANARY_SIZE));
    size_t badValueSize = 0;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ((size_t)4, name_size); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, badValueSize); //the size of black_cat + nul
    VERIFY_CANARY_VALUES(name, name_size, 'E');
    VERIFY_CANARY_VALUES(value, value_size, 'F');

    memset(name, 'G', name_size + (2 * CANARY_SIZE));
    memset(value, 'H', value_size + (2 * CANARY_SIZE));
    badNameSize = 0;
    badValueSize = 0;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ((size_t)4, badNameSize); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, badValueSize); //the size of black_cat + nul
    VERIFY_CANARY_VALUES(name, name_size, 'G');
    VERIFY_CANARY_VALUES(value, value_size, 'H');

    memset(name, 'I', name_size + (2 * CANARY_SIZE));
    memset(value, 'J', value_size + (2 * CANARY_SIZE));
    size_t tooSmallSize = 1;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)4, tooSmallSize); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    EXPECT_STREQ("", name + CANARY_SIZE); //empty string instead of 'one'
    EXPECT_STREQ("black_cat", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'I');
    VERIFY_CANARY_VALUES(value, value_size, 'J');

    memset(name, 'L', name_size + (2 * CANARY_SIZE));
    memset(value, 'M', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_EQ((size_t)4, name_size); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, tooSmallSize); //the size of black_cat + nul
    EXPECT_STREQ("one", name + CANARY_SIZE);
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of 'black_cat'
    VERIFY_CANARY_VALUES(name, name_size, 'L');
    VERIFY_CANARY_VALUES(value, value_size, 'M');

    memset(name, 'N', name_size + (2 * CANARY_SIZE));
    memset(value, 'O', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)4, tooSmallSize); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    EXPECT_STREQ("o", name + CANARY_SIZE); //first character of 'one'
    EXPECT_STREQ("black_cat", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'N');
    VERIFY_CANARY_VALUES(value, value_size, 'O');

    memset(name, 'P', name_size + (2 * CANARY_SIZE));
    memset(value, 'R', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 3;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_EQ((size_t)4, name_size); //the size of 'one' {'o', 'n', 'e', '\0'}
    EXPECT_EQ((size_t)10, tooSmallSize); //the size of black_cat + nul
    EXPECT_STREQ("one", name + CANARY_SIZE);
    EXPECT_STREQ("bl", value + CANARY_SIZE); //first two characters of 'black_cat'
    VERIFY_CANARY_VALUES(name, name_size, 'P');
    VERIFY_CANARY_VALUES(value, value_size, 'R');

    free(name);
    free(value);

    QCC_BOOL success = alljoyn_interfacedescription_member_getannotation(method_member, "one", NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 'K', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_member_getannotation(method_member, "one", value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    EXPECT_STREQ("black_cat", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'K');

    memset(value, 'L', value_size + (2 * CANARY_SIZE));
    badValueSize = 0;
    success = alljoyn_interfacedescription_member_getannotation(method_member, "one", value + CANARY_SIZE, &badValueSize);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)10, badValueSize); //the size of black_cat + nul
    VERIFY_CANARY_VALUES(value, value_size, 'L');

    memset(value, 'M', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_member_getannotation(method_member, "one", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)10, tooSmallSize); //the size of black_cat + nul
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of black_cat
    VERIFY_CANARY_VALUES(value, value_size, 'M');

    memset(value, 'N', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_member_getannotation(method_member, "one", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)10, tooSmallSize); //the size of black_cat + nul
    EXPECT_STREQ("b", value + CANARY_SIZE); //first character of black_cat
    VERIFY_CANARY_VALUES(value, value_size, 'N');

    free(value);

    success = alljoyn_interfacedescription_getmemberannotation(testIntf, "ping", "one", NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul

    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 'O', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_getmemberannotation(testIntf, "ping", "one", value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)10, value_size); //the size of black_cat + nul
    EXPECT_STREQ("black_cat", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'O');

    memset(value, 'P', value_size + (2 * CANARY_SIZE));
    badValueSize = 0;
    success = alljoyn_interfacedescription_getmemberannotation(testIntf, "ping", "one", value + CANARY_SIZE, &badValueSize);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)10, badValueSize); //the size of black_cat + nul
    VERIFY_CANARY_VALUES(value, value_size, 'P');

    memset(value, 'Q', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_getmemberannotation(testIntf, "ping", "one", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)10, tooSmallSize); //the size of black_cat + nul
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of black_cat
    VERIFY_CANARY_VALUES(value, value_size, 'Q');

    memset(value, 'R', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_getmemberannotation(testIntf, "ping", "one", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)10, tooSmallSize); //the size of black_cat + nul
    EXPECT_STREQ("b", value + CANARY_SIZE); //first character of black_cat
    VERIFY_CANARY_VALUES(value, value_size, 'R');

    free(value);

    alljoyn_busattachment_destroy(bus);
}


TEST(InterfaceDescriptionTest, signal_annotations)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "s", NULL, "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "chirp", "two", "apples");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    alljoyn_interfacedescription_member signal_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "chirp", &signal_member));

    size_t annotation_count = alljoyn_interfacedescription_member_getannotationscount(signal_member);
    EXPECT_EQ((size_t)1, annotation_count);
    size_t name_size;
    size_t value_size;
    alljoyn_interfacedescription_member_getannotationatindex(signal_member, 0, NULL, &name_size, NULL, &value_size);
    EXPECT_EQ((size_t)4, name_size); //the size of 'two' {'t', 'w', 'o', '\0'}
    EXPECT_EQ((size_t)7, value_size); //the size of 'apples' + nul

    char* name = (char*)malloc(sizeof(char) * name_size);
    char* value = (char*)malloc(sizeof(char) * value_size);

    alljoyn_interfacedescription_member_getannotationatindex(signal_member, 0, name, &name_size, value, &value_size);


    EXPECT_STREQ("two", name);
    EXPECT_STREQ("apples", value);

    free(name);
    free(value);

    alljoyn_interfacedescription_member_getannotation(signal_member, "two", NULL, &value_size);
    EXPECT_LT((size_t)0, value_size);

    value = (char*)malloc(sizeof(char) * value_size);
    QCC_BOOL success = alljoyn_interfacedescription_member_getannotation(signal_member, "two", value, &value_size);
    EXPECT_TRUE(success);

    EXPECT_STREQ("apples", value);

    free(value);

    alljoyn_interfacedescription_getmemberannotation(testIntf, "chirp", "two", NULL, &value_size);
    EXPECT_LT((size_t)0, value_size);

    value = (char*)malloc(sizeof(char) * value_size);

    success = alljoyn_interfacedescription_getmemberannotation(testIntf, "chirp", "two", value, &value_size);
    EXPECT_TRUE(success);

    EXPECT_STREQ("apples", value);

    free(value);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, property_annotations)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addpropertyannotation(testIntf, "prop", "three", "people");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    alljoyn_interfacedescription_property property;
    EXPECT_TRUE(alljoyn_interfacedescription_getproperty(testIntf, "prop", &property));

    size_t annotation_count = alljoyn_interfacedescription_property_getannotationscount(property);
    EXPECT_EQ((size_t)1, annotation_count);

    size_t name_size;
    size_t value_size;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, NULL, &name_size, NULL, &value_size);
    EXPECT_EQ((size_t)6, name_size); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul

    char* name = (char*)malloc((sizeof(char) * name_size) + (2 * CANARY_SIZE));
    char* value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(name, 'a', name_size + (2 * CANARY_SIZE));
    memset(value, 'b', value_size + (2 * CANARY_SIZE));
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)6, name_size); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul
    EXPECT_STREQ("three", name + CANARY_SIZE);
    EXPECT_STREQ("people", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'a');
    VERIFY_CANARY_VALUES(value, value_size, 'b');

    memset(name, 'c', name_size + (2 * CANARY_SIZE));
    memset(value, 'd', value_size + (2 * CANARY_SIZE));
    size_t badNameSize = 0;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)6, badNameSize); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul
    VERIFY_CANARY_VALUES(name, name_size, 'c');
    VERIFY_CANARY_VALUES(value, value_size, 'd');

    memset(name, 'e', name_size + (2 * CANARY_SIZE));
    memset(value, 'f', value_size + (2 * CANARY_SIZE));
    size_t badValueSize = 0;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ((size_t)6, name_size); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, badValueSize); //the size of 'people' + nul
    VERIFY_CANARY_VALUES(name, name_size, 'e');
    VERIFY_CANARY_VALUES(value, value_size, 'f');

    memset(name, 'g', name_size + (2 * CANARY_SIZE));
    memset(value, 'h', value_size + (2 * CANARY_SIZE));
    badNameSize = 0;
    badValueSize = 0;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ((size_t)6, badNameSize); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, badValueSize); //the size of 'people' + nul
    VERIFY_CANARY_VALUES(name, name_size, 'g');
    VERIFY_CANARY_VALUES(value, value_size, 'h');

    memset(name, 'i', name_size + (2 * CANARY_SIZE));
    memset(value, 'j', value_size + (2 * CANARY_SIZE));
    size_t tooSmallSize = 1;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ((size_t)6, tooSmallSize); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul
    EXPECT_STREQ("", name + CANARY_SIZE); //empty string instead of 'three'
    EXPECT_STREQ("people", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'i');
    VERIFY_CANARY_VALUES(value, value_size, 'j');

    memset(name, 'k', name_size + (2 * CANARY_SIZE));
    memset(value, 'l', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_EQ((size_t)6, name_size); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, tooSmallSize); //the size of 'people' + nul
    EXPECT_STREQ("three", name + CANARY_SIZE);
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of 'people'
    VERIFY_CANARY_VALUES(name, name_size, 'k');
    VERIFY_CANARY_VALUES(value, value_size, 'l');

    memset(name, 'm', name_size + (2 * CANARY_SIZE));
    memset(value, 'n', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 3;
    size_t tooSmallSize2 = 2;
    alljoyn_interfacedescription_property_getannotationatindex(property, 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &tooSmallSize2);
    EXPECT_EQ((size_t)6, tooSmallSize); //the size of 'three' {'t', 'h', 'r', 'e', 'e', '\0'}
    EXPECT_EQ((size_t)7, tooSmallSize2); //the size of 'people' + nul
    EXPECT_STREQ("th", name + CANARY_SIZE); //first two characters of 'three'
    EXPECT_STREQ("p", value + CANARY_SIZE); //first character of 'people'
    VERIFY_CANARY_VALUES(name, name_size, 'm');
    VERIFY_CANARY_VALUES(value, value_size, 'n');

    free(name);
    free(value);

    QCC_BOOL success = alljoyn_interfacedescription_property_getannotation(property, "three", NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul

    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 'o', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_property_getannotation(property, "three", value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul
    EXPECT_STREQ("people", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'o');

    memset(value, 'p', value_size + (2 * CANARY_SIZE));
    badValueSize = 0;
    success = alljoyn_interfacedescription_property_getannotation(property, "three", value + CANARY_SIZE, &badValueSize);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)7, badValueSize); //the size of 'people' + nul
    VERIFY_CANARY_VALUES(value, value_size, 'p');

    memset(value, 'q', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_property_getannotation(property, "three", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)7, tooSmallSize); //the size of 'people' + nul
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of 'people'
    VERIFY_CANARY_VALUES(value, value_size, 'q');

    memset(value, 'r', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_property_getannotation(property, "three", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)7, tooSmallSize); //the size of 'people' + nul
    EXPECT_STREQ("p", value + CANARY_SIZE); //first character of 'people'
    VERIFY_CANARY_VALUES(value, value_size, 'r');

    free(value);

    success = alljoyn_interfacedescription_getpropertyannotation(testIntf, "prop", "three", NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul

    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 's', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_getpropertyannotation(testIntf, "prop", "three", value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)7, value_size); //the size of 'people' + nul
    EXPECT_STREQ("people", value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 's');

    memset(value, 't', value_size + (2 * CANARY_SIZE));
    badValueSize = 0;
    success = alljoyn_interfacedescription_getpropertyannotation(testIntf, "prop", "three", value + CANARY_SIZE, &badValueSize);
    EXPECT_FALSE(success);
    EXPECT_EQ((size_t)7, badValueSize); //the size of 'people' + nul
    VERIFY_CANARY_VALUES(value, value_size, 't');

    memset(value, 'u', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_getpropertyannotation(testIntf, "prop", "three", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)7, tooSmallSize); //the size of 'people' + nul
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of 'people'
    VERIFY_CANARY_VALUES(value, value_size, 'u');

    memset(value, 'u', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_getpropertyannotation(testIntf, "prop", "three", value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ((size_t)7, tooSmallSize); //the size of 'people' + nul
    EXPECT_STREQ("p", value + CANARY_SIZE); //first character of 'people'
    VERIFY_CANARY_VALUES(value, value_size, 'u');

    free(value);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, member_argument_annotations)
{
    QStatus status = ER_OK;
    char argName1[] = "in_name1";
    char argValue1[] = "in_value1";
    char argName2[] = "in_name2";
    char argValue2[] = "in_value 2";
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_NE(nullptr, bus);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_NE(nullptr, testIntf);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addargannotation(testIntf, "ping", "in", argName1, argValue1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addargannotation(testIntf, "ping", "in", argName2, argValue2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    alljoyn_interfacedescription_member method_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &method_member));

    size_t annotation_count = alljoyn_interfacedescription_member_getargannotationscount(method_member, "in");
    EXPECT_EQ((size_t)2, annotation_count);
    size_t name_size;
    size_t value_size;

    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 1, NULL, &name_size, NULL, &value_size);
    EXPECT_EQ(sizeof(argName2), name_size); //the size of name + nul
    EXPECT_EQ(sizeof(argValue2), value_size); //the size of value + nul
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, NULL, &name_size, NULL, &value_size);
    EXPECT_EQ(sizeof(argName1), name_size); //the size of name + nul
    EXPECT_EQ(sizeof(argValue1), value_size); //the size of value + nul

    char* name = (char*)malloc((sizeof(char) * name_size) + (2 * CANARY_SIZE));
    char* value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(name, 'A', name_size + (2 * CANARY_SIZE));
    memset(value, 'B', value_size + (2 * CANARY_SIZE));
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &value_size);
    EXPECT_EQ(sizeof(argName1), name_size); //the size of name + nul
    EXPECT_EQ(sizeof(argValue1), value_size); //the size of value + nul
    EXPECT_STREQ(argName1, name + CANARY_SIZE);
    EXPECT_STREQ(argValue1, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'A');
    VERIFY_CANARY_VALUES(value, value_size, 'B');

    memset(name, 'C', name_size + (2 * CANARY_SIZE));
    memset(value, 'D', value_size + (2 * CANARY_SIZE));
    size_t badNameSize = 0;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ(sizeof(argName1), badNameSize); //the size of name + nul
    EXPECT_EQ(sizeof(argValue1), value_size); //the size of value + nul
    VERIFY_CANARY_VALUES(name, name_size, 'C');
    VERIFY_CANARY_VALUES(value, value_size, 'D');

    memset(name, 'E', name_size + (2 * CANARY_SIZE));
    memset(value, 'F', value_size + (2 * CANARY_SIZE));
    size_t badValueSize = 0;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ(sizeof(argName1), name_size); //the size of name + nul
    EXPECT_EQ(sizeof(argValue1), badValueSize); //the size of value + nul
    VERIFY_CANARY_VALUES(name, name_size, 'E');
    VERIFY_CANARY_VALUES(value, value_size, 'F');

    memset(name, 'G', name_size + (2 * CANARY_SIZE));
    memset(value, 'H', value_size + (2 * CANARY_SIZE));
    badNameSize = 0;
    badValueSize = 0;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &badNameSize, value + CANARY_SIZE, &badValueSize);
    EXPECT_EQ(sizeof(argName1), badNameSize);
    EXPECT_EQ(sizeof(argValue1), badValueSize);
    VERIFY_CANARY_VALUES(name, name_size, 'G');
    VERIFY_CANARY_VALUES(value, value_size, 'H');

    memset(name, 'I', name_size + (2 * CANARY_SIZE));
    memset(value, 'J', value_size + (2 * CANARY_SIZE));
    size_t tooSmallSize = 1;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ(sizeof(argName1), tooSmallSize);
    EXPECT_EQ(sizeof(argValue1), value_size);
    EXPECT_STREQ("", name + CANARY_SIZE); //empty string instead of 'one'
    EXPECT_STREQ(argValue1, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'I');
    VERIFY_CANARY_VALUES(value, value_size, 'J');

    memset(name, 'L', name_size + (2 * CANARY_SIZE));
    memset(value, 'M', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_EQ(sizeof(argName1), name_size);
    EXPECT_EQ(sizeof(argValue1), tooSmallSize);
    EXPECT_STREQ(argName1, name + CANARY_SIZE);
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of arg value
    VERIFY_CANARY_VALUES(name, name_size, 'L');
    VERIFY_CANARY_VALUES(value, value_size, 'M');

    memset(name, 'N', name_size + (2 * CANARY_SIZE));
    memset(value, 'O', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &tooSmallSize, value + CANARY_SIZE, &value_size);
    EXPECT_EQ(sizeof(argName1), tooSmallSize);
    EXPECT_EQ(sizeof(argValue1), value_size);
    char s1[] = { argName1[0], '\0' }; //first character of arg name
    EXPECT_STREQ(s1, name + CANARY_SIZE);
    EXPECT_STREQ(argValue1, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(name, name_size, 'N');
    VERIFY_CANARY_VALUES(value, value_size, 'O');

    memset(name, 'P', name_size + (2 * CANARY_SIZE));
    memset(value, 'R', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 3;
    alljoyn_interfacedescription_member_getargannotationatindex(method_member, "in", 0, name + CANARY_SIZE, &name_size, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_EQ(sizeof(argName1), name_size);
    EXPECT_EQ(sizeof(argValue1), tooSmallSize);
    EXPECT_STREQ(argName1, name + CANARY_SIZE);
    char s2[] = { argValue1[0], argValue1[1], '\0' };
    EXPECT_STREQ(s2, value + CANARY_SIZE); //first two characters of arg value
    VERIFY_CANARY_VALUES(name, name_size, 'P');
    VERIFY_CANARY_VALUES(value, value_size, 'R');

    free(name);
    free(value);

    QCC_BOOL success = alljoyn_interfacedescription_member_getargannotation(method_member, "in", argName1, NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ(sizeof(argValue1), value_size);
    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 'K', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_member_getargannotation(method_member, "in", argName1, value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ(sizeof(argValue1), value_size);
    EXPECT_STREQ(argValue1, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'K');

    memset(value, 'L', value_size + (2 * CANARY_SIZE));
    badValueSize = 0;
    success = alljoyn_interfacedescription_member_getargannotation(method_member, "in", argName1, value + CANARY_SIZE, &badValueSize);
    EXPECT_FALSE(success);
    EXPECT_EQ(sizeof(argValue1), badValueSize);
    VERIFY_CANARY_VALUES(value, value_size, 'L');

    memset(value, 'M', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_member_getargannotation(method_member, "in", argName1, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ(sizeof(argValue1), tooSmallSize);
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of arg value
    VERIFY_CANARY_VALUES(value, value_size, 'M');

    memset(value, 'N', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_member_getargannotation(method_member, "in", argName1, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ(sizeof(argValue1), tooSmallSize);
    char s3[] = { argValue1[0], '\0' }; //first character of arg value
    EXPECT_STREQ(s3, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'N');

    free(value);

    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "ping", "in", argName1, NULL, &value_size);
    EXPECT_FALSE(success);
    EXPECT_EQ(sizeof(argValue1), value_size);

    value = (char*)malloc((sizeof(char) * value_size) + (2 * CANARY_SIZE));

    memset(value, 'O', value_size + (2 * CANARY_SIZE));
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "ping", "in", argName1, value + CANARY_SIZE, &value_size);
    EXPECT_TRUE(success);
    EXPECT_EQ(sizeof(argValue1), value_size);
    EXPECT_STREQ(argValue1, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'O');

    memset(value, 'P', value_size + (2 * CANARY_SIZE));
    badValueSize = 0;
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "ping", "in", argName1, value + CANARY_SIZE, &badValueSize);
    EXPECT_FALSE(success);
    EXPECT_EQ(sizeof(argValue1), badValueSize);
    VERIFY_CANARY_VALUES(value, value_size, 'P');

    memset(value, 'Q', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 1;
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "ping", "in", argName1, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ(sizeof(argValue1), tooSmallSize);
    EXPECT_STREQ("", value + CANARY_SIZE); //empty string instead of arg value
    VERIFY_CANARY_VALUES(value, value_size, 'Q');

    memset(value, 'R', value_size + (2 * CANARY_SIZE));
    tooSmallSize = 2;
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "ping", "in", argName1, value + CANARY_SIZE, &tooSmallSize);
    EXPECT_TRUE(success);
    EXPECT_EQ(sizeof(argValue1), tooSmallSize);
    char s4[] = { argValue1[0], '\0' }; //first character of arg value
    EXPECT_STREQ(s4, value + CANARY_SIZE);
    VERIFY_CANARY_VALUES(value, value_size, 'R');

    free(value);

    size_t allocSize = 16;
    value = (char*)malloc(allocSize);
    value_size = allocSize;
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "notexist", "notexist", "notexist", value, &value_size);
    EXPECT_FALSE(success);
    value_size = allocSize;
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "ping", "notexist", "notexist", value, &value_size);
    EXPECT_FALSE(success);
    value_size = allocSize;
    success = alljoyn_interfacedescription_getmemberargannotation(testIntf, "chirp", "chirp", "notexist", value, &value_size);
    EXPECT_FALSE(success);
    free(value);

    alljoyn_busattachment_destroy(bus);
}

/*
 * check to see that we are still backward compatible with the annotation flags
 */
TEST(InterfaceDescriptionTest, annotation_flags)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", ALLJOYN_MEMBER_ANNOTATE_NO_REPLY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "s", NULL, "chirp", ALLJOYN_MEMBER_ANNOTATE_DEPRECATED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    alljoyn_interfacedescription_member method_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &method_member));

    size_t annotation_count = alljoyn_interfacedescription_member_getannotationscount(method_member);
    EXPECT_EQ((size_t)1, annotation_count);
    size_t name_size;
    size_t value_size;
    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, NULL, &name_size, NULL, &value_size);

    char* name = (char*)malloc(sizeof(char) * name_size);
    char* value = (char*)malloc(sizeof(char) * value_size);

    alljoyn_interfacedescription_member_getannotationatindex(method_member, 0, name, &name_size, value, &value_size);


    EXPECT_STREQ("org.freedesktop.DBus.Method.NoReply", name);
    EXPECT_STREQ("true", value);

    free(name);
    free(value);

    alljoyn_interfacedescription_member signal_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "chirp", &signal_member));

    annotation_count = alljoyn_interfacedescription_member_getannotationscount(signal_member);
    EXPECT_EQ((size_t)1, annotation_count);

    alljoyn_interfacedescription_member_getannotationatindex(signal_member, 0, NULL, &name_size, NULL, &value_size);

    name = (char*)malloc(sizeof(char) * name_size);
    value = (char*)malloc(sizeof(char) * value_size);

    alljoyn_interfacedescription_member_getannotationatindex(signal_member, 0, name, &name_size, value, &value_size);


    EXPECT_STREQ("org.freedesktop.DBus.Deprecated", name);
    EXPECT_STREQ("true", value);

    free(name);
    free(value);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, multiple_annotations)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", ALLJOYN_MEMBER_ANNOTATE_NO_REPLY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "org.alljoyn.test.one", "black_cat");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "org.alljoyn.test.two", "broken_mirror");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "org.alljoyn.test.three", "latter");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "org.alljoyn.test.four", "umbrella");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "org.alljoyn.test.five", "luck");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmemberannotation(testIntf, "ping", "org.alljoyn.test.six", "bad");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    alljoyn_interfacedescription_member method_member;
    EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &method_member));

    size_t annotation_count = alljoyn_interfacedescription_member_getannotationscount(method_member);
    EXPECT_EQ((size_t)7, annotation_count); //six annotations added plus the NoReply annotation
    size_t name_size;
    size_t value_size;

    for (size_t i = 0; i < annotation_count; i++) {
        alljoyn_interfacedescription_member_getannotationatindex(method_member, i, NULL, &name_size, NULL, &value_size);
        char* name = (char*)malloc(sizeof(char) * name_size);
        char* value = (char*)malloc(sizeof(char) * value_size);
        ASSERT_TRUE(name != NULL);
        ASSERT_TRUE(value != NULL);

        alljoyn_interfacedescription_member_getannotationatindex(method_member, i, name, &name_size, value, &value_size);

        /*
         * order that the annotations are returned is not known we only know that
         * the key must match with the value.
         * for windows the order returned is (this order could differ by OS or compiler)
         * 0 : org.alljoyn.test.five = luck
         * 1 : org.alljoyn.test.four = umbrella
         * 2 : org.alljoyn.test.one = black_cat
         * 3 : org.alljoyn.test.six = bad
         * 4 : org.alljoyn.test.three = latter
         * 5 : org.alljoyn.test.two = broken_mirror
         * 6 : org.freedesktop.DBus.Method.NoReply = true
         */
        EXPECT_TRUE((strcmp("org.alljoyn.test.one", name) == 0 && strcmp("black_cat", value) == 0) ||
                    (strcmp("org.alljoyn.test.two", name) == 0 && strcmp("broken_mirror", value) == 0) ||
                    (strcmp("org.alljoyn.test.three", name) == 0 && strcmp("latter", value) == 0) ||
                    (strcmp("org.alljoyn.test.four", name) == 0 && strcmp("umbrella", value) == 0) ||
                    (strcmp("org.alljoyn.test.five", name) == 0 && strcmp("luck", value) == 0) ||
                    (strcmp("org.alljoyn.test.six", name) == 0 && strcmp("bad", value) == 0) ||
                    (strcmp("org.freedesktop.DBus.Method.NoReply", name) == 0 && strcmp("true", value) == 0))
            << "Expected annotation not found : " << name << " = " << value << "\n";

        free(name);
        free(value);
    }
    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, description_language)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_NE(nullptr, bus);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_NE(nullptr, testIntf);
    char language[] = "En";
    alljoyn_interfacedescription_setdescriptionlanguage(testIntf, language);
    alljoyn_interfacedescription_setdescription(testIntf, "Hello");
    alljoyn_interfacedescription_activate(testIntf);

    size_t count = alljoyn_interfacedescription_getdescriptionlanguages(testIntf, nullptr, 0);
    EXPECT_EQ((size_t)1, count);
    const char* languages[1];
    count = alljoyn_interfacedescription_getdescriptionlanguages(testIntf, languages, ArraySize(languages));
    EXPECT_EQ((size_t)1, count);
    EXPECT_STREQ(language, languages[0]);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, member_property_arg_description)
{
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_NE(nullptr, bus);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_NE(nullptr, testIntf);

    QCC_BOOL hasdescription = alljoyn_interfacedescription_hasdescription(testIntf);
    EXPECT_FALSE(hasdescription);

    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "ping", "my member description");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_interfacedescription_setargdescription(testIntf, "ping", "in", "my member in argument description");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setargdescription(testIntf, "ping", "out", "my member out argument description");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setpropertydescription(testIntf, "prop1", "my property description");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);

    hasdescription = alljoyn_interfacedescription_hasdescription(testIntf);
    EXPECT_TRUE(hasdescription);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, description_translator_en_en) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_NE(nullptr, bus);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_NE(nullptr, testIntf);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // Set description language to "En", introspect returns "En" descriptions
    alljoyn_interfacedescription_setdescriptionlanguage(testIntf, "En");
    alljoyn_interfacedescription_setdescription(testIntf, "in_desc");
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "ping", "me_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setargdescription(testIntf, "ping", "in", "ar_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "chirp", "si_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setpropertydescription(testIntf, "prop1", "pr_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <signal name=\"chirp\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"si_desc\"/>\n"
        "  </signal>\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\">\n"
        "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"ar_desc\"/>\n"
        "    </arg>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"me_desc\"/>\n"
        "  </method>\n"
        "  <property name=\"prop1\" type=\"s\" access=\"read\">\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"pr_desc\"/>\n"
        "  </property>\n"
        "  <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"in_desc\"/>\n"
        "</interface>\n";
    translationCount = 0;
    introspect_and_compare(testIntf, expectedIntrospect);
    EXPECT_TRUE(translationCount == 0);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, description_translator_en_null) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_NE(nullptr, bus);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_NE(nullptr, testIntf);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_setdescriptionlanguage(testIntf, "En");
    alljoyn_interfacedescription_setdescription(testIntf, "in_desc");
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "ping", "me_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setargdescription(testIntf, "ping", "in", "ar_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "chirp", "si_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setpropertydescription(testIntf, "prop1", "pr_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_translation_callback_ptr p = alljoyn_interfacedescription_getdescriptiontranslationcallback(testIntf);
    EXPECT_EQ(nullptr, p);
    alljoyn_interfacedescription_setdescriptiontranslationcallback(testIntf, translation_callback);
    p = alljoyn_interfacedescription_getdescriptiontranslationcallback(testIntf);
    EXPECT_TRUE(p == translation_callback);

    // Set translation callback and set description language to "", translation callback is invoked and introspect returns translated "En" descriptions
    alljoyn_interfacedescription_setdescriptionlanguage(testIntf, "");
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <signal name=\"chirp\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:si_desc\"/>\n"
        "  </signal>\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\">\n"
        "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:ar_desc\"/>\n"
        "    </arg>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:me_desc\"/>\n"
        "  </method>\n"
        "  <property name=\"prop1\" type=\"s\" access=\"read\">\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:pr_desc\"/>\n"
        "  </property>\n"
        "  <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:in_desc\"/>\n"
        "</interface>\n";
    translationCount = 0;
    introspect_and_compare(testIntf, expectedIntrospect);
    EXPECT_TRUE(translationCount > 0);

    alljoyn_busattachment_destroy(bus);
}

TEST(InterfaceDescriptionTest, description_translator_en_de) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("InterfaceDescriptionTest", QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("InterfaceDescriptionTest"));
    ASSERT_NE(nullptr, bus);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.InterfaceDescription", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_NE(nullptr, testIntf);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_setdescriptionlanguage(testIntf, "En");
    alljoyn_interfacedescription_setdescription(testIntf, "in_desc");
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "ping", "me_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setargdescription(testIntf, "ping", "in", "ar_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setmemberdescription(testIntf, "chirp", "si_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_setpropertydescription(testIntf, "prop1", "pr_desc");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription_setdescriptiontranslationcallback(testIntf, translation_callback);

    // Set description language to "De", translation callback is invoked and introspect returns "En" and translated "De" descriptions
    alljoyn_interfacedescription_setdescriptionlanguage(testIntf, "De");
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.InterfaceDescription\">\n"
        "  <signal name=\"chirp\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.De\" value=\"De:si_desc\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:si_desc\"/>\n"
        "  </signal>\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\">\n"
        "      <annotation name=\"org.alljoyn.Bus.DocString.De\" value=\"De:ar_desc\"/>\n"
        "      <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:ar_desc\"/>\n"
        "    </arg>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.De\" value=\"De:me_desc\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:me_desc\"/>\n"
        "  </method>\n"
        "  <property name=\"prop1\" type=\"s\" access=\"read\">\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.De\" value=\"De:pr_desc\"/>\n"
        "    <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:pr_desc\"/>\n"
        "  </property>\n"
        "  <annotation name=\"org.alljoyn.Bus.DocString.De\" value=\"De:in_desc\"/>\n"
        "  <annotation name=\"org.alljoyn.Bus.DocString.En\" value=\"En:in_desc\"/>\n"
        "</interface>\n";
    translationCount = 0;
    introspect_and_compare(testIntf, expectedIntrospect);
    EXPECT_TRUE(translationCount > 0);

    alljoyn_busattachment_destroy(bus);
}
