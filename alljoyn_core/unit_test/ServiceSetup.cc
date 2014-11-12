/*******************************************************************************
 * Copyright (c) 2011,2014 AllSeen Alliance. All rights reserved.
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

/* namespace is defined in cc file instead of header file:
 *  avoid multiple definition compile error
 */
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.test_services.Interface";
namespace dummy {
const char*InterfaceName1 = "org.alljoyn.test_services.dummy.Interface1";
const char*InterfaceName2 = "org.alljoyn.test_services.dummy.Interface2";
const char*InterfaceName3 = "org.alljoyn.test_services.dummy.Interface3";
}
const char* ObjectPath = "/org/alljoyn/test_services";
namespace values {
const char* InterfaceName = "org.alljoyn.test_services.Interface.values";
namespace dummy {
const char*InterfaceName1 = "org.alljoyn.test_services.values.dummy.Interface1";
const char*InterfaceName2 = "org.alljoyn.test_services.values.dummy.Interface2";
const char*InterfaceName3 = "org.alljoyn.test_services.values.dummy.Interface3";
}
}
}

/* New set of names */
namespace service_test {
const char* InterfaceName = "org.alljoyn.service_test.Interface";
namespace dummy {
const char*InterfaceName1 = "org.alljoyn.service_test.dummy.Interface1";
const char*InterfaceName2 = "org.alljoyn.service_test.dummy.Interface2";
const char*InterfaceName3 = "org.alljoyn.service_test.dummy.Interface3";
}
const char* ObjectPath = "/org/alljoyn/service_test";
namespace values {
const char* InterfaceName = "org.alljoyn.service_test.Interface.values";
namespace dummy {
const char*InterfaceName1 = "org.alljoyn.service_test.values.dummy.Interface1";
const char*InterfaceName2 = "org.alljoyn.service_test.values.dummy.Interface2";
const char*InterfaceName3 = "org.alljoyn.service_test.values.dummy.Interface3";
}
}
}

}
}

bool MyBusListener::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    //printf("\n Inside Accept session Joiner  \n");
    return true;
}

QStatus MyAuthListener::RequestPwd(const qcc::String& authMechanism, uint8_t minLen, qcc::String& pwd)
{
    pwd = "123456";
    return ER_OK;
}

/* Service Object class */

ServiceObject::ServiceObject(BusAttachment& bus, const char* path) : BusObject(path), objectRegistered(false), myBus(bus),
    alljoynWellKnownName(genUniqueName(myBus)), serviceWellKnownName(genUniqueName(myBus))
{

}

ServiceObject::~ServiceObject()
{

}

void ServiceObject::ObjectRegistered()
{
    BusObject::ObjectRegistered();
    objectRegistered = true;
}

QStatus ServiceObject::AddInterfaceToObject(const InterfaceDescription* intf)
{
    assert(intf);
    QStatus status = AddInterface(*intf);
    return status;
}

void ServiceObject::PopulateSignalMembers()
{
    /* Register the signal handler with the bus */
    const InterfaceDescription* regTestIntf = myBus.GetInterface(
        ::org::alljoyn::alljoyn_test::InterfaceName);
    assert(regTestIntf);
    my_signal_member = regTestIntf->GetMember("my_signal");
    assert(my_signal_member);
    my_signal_member_2 = regTestIntf->GetMember("my_signal_string");
    assert(my_signal_member_2);
}

void ServiceObject::NameAcquiredCB(Message& msg, void* context)
{
    /* Check name acquired result */
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    if (args[0].v_uint32 == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        // output1++;
    } else {
        QCC_LogError(ER_FAIL, ("Failed to obtain name . RequestName returned %d",
                               args[0].v_uint32));
        //   output1 += 99;
    }
}

void ServiceObject::RequestName(const char* name)
{
    assert(name);
    /* Request a well-known name */
    /* Note that you cannot make a blocking method call here */
    const ProxyBusObject& dbusObj = myBus.GetDBusProxyObj();
    MsgArg args[2];
    args[0].Set("s", name);
    args[1].Set("u", 6);
    QStatus status = dbusObj.MethodCallAsync(ajn::org::freedesktop::DBus::InterfaceName,
                                             "RequestName",
                                             this,
                                             static_cast<MessageReceiver::ReplyHandler>(&ServiceObject::NameAcquiredCB),
                                             args,
                                             ArraySize(args));

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to request name %s", name));
    }
}

QStatus ServiceObject::InstallMethodHandlers()
{
    const InterfaceDescription* regTestIntf = myBus.GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
    assert(regTestIntf);
    /* Register the method handlers with the object */
    const MethodEntry methodEntries[] = {
        { regTestIntf->GetMember("my_ping"), static_cast<MessageReceiver::MethodHandler>(&ServiceObject::Ping) },
        { regTestIntf->GetMember("my_sing"), static_cast<MessageReceiver::MethodHandler>(&ServiceObject::Sing) },
        { regTestIntf->GetMember("my_param_test"), static_cast<MessageReceiver::MethodHandler>(&ServiceObject::ParamTest) }

    };

    QStatus status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
    return status;
}

void ServiceObject::Ping(const InterfaceDescription::Member* member, Message& msg)
{
    /* Reply with same string that was sent to us */
    MsgArg arg(*(msg->GetArg(0)));
    //printf("Pinged with: %s\n", msg->GetArg(0)->ToString().c_str());
    QStatus status = MethodReply(msg, &arg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("Ping: Error sending reply"));
    }
}

void ServiceObject::Sing(const InterfaceDescription::Member* member, Message& msg)
{
    /* Reply with same string that was sent to us */
    MsgArg arg(*(msg->GetArg(0)));
    char temp_str[15];
    strcpy(temp_str, msg->GetArg(0)->v_string.str);

    QStatus status = MethodReply(msg, &arg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("Sing: Error sending reply"));
    }

    MsgArg arg1[2];

    if (0 == strcmp(temp_str, "Huge String")) {
        //qcc::String hugeA(4096, 'a');
        //printf("%d\n%s\n", hugeA.length(), hugeA.c_str());
        //EXPECT_EQ(hugeA[4096], '\0');
        //status = arg1[1].Set("s", hugeA.c_str());
        arg1[1].Set("s", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        arg1[0].Set("u", 4096);
    } else {
        arg1[0].Set("u", 5);
        arg1[1].Set("s", "hello");
    }

    status = Signal(NULL, 0, *my_signal_member_2, arg1, 2);
}


void ServiceObject::ParamTest(const InterfaceDescription::Member* member, Message& msg)
{
    /* Reply has multiple */
    MsgArg arg[10];

    arg[0] = *(msg->GetArg(0));
    arg[1] = *(msg->GetArg(1));
    arg[2] = *(msg->GetArg(2));
    arg[3] = *(msg->GetArg(3));
    arg[4] = *(msg->GetArg(4));
    arg[5] = *(msg->GetArg(5));
    arg[6] = *(msg->GetArg(6));
    arg[7] = *(msg->GetArg(7));
    arg[8] = *(msg->GetArg(8));
    arg[9] = *(msg->GetArg(9));

    QStatus status = MethodReply(msg, arg, 10);
    if (ER_OK != status) {
        printf("\n Error sending reply");
        QCC_LogError(status, ("ParamTest: Error sending reply"));
    }
}

QStatus ServiceObject::EmitTestSignal(String newName)
{
    MsgArg arg("s", newName.c_str());
    QStatus status = Signal(NULL, 0, *my_signal_member, &arg, 1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //printf("\n Signal Sent");
    return status;
}


QStatus ServiceObject::Get(const char*ifcName, const char*propName, MsgArg& val)
{
    QStatus status = ER_OK;
    if (0 == strcmp("int_val", propName)) {
        //val.Set("i", prop_int_val);
        val.typeId = ALLJOYN_INT32;
        val.v_int32 = prop_int_val;
    } else if (0 == strcmp("str_val", propName)) {
        //val.Set("s", prop_str_val.c_str());
        val.typeId = ALLJOYN_STRING;
        val.v_string.str = prop_str_val.c_str();
        val.v_string.len = prop_str_val.size();
    } else if (0 == strcmp("ro_str", propName)) {
        //val.Set("s", prop_ro_str_val.c_str());
        val.typeId = ALLJOYN_STRING;
        val.v_string.str = prop_ro_str.c_str();
        val.v_string.len = prop_ro_str.size();
    } else if (0 == strcmp("prop_signal", propName)) {
        val.typeId = ALLJOYN_STRING;
        val.v_string.str = prop_signal.c_str();
        val.v_string.len = prop_signal.size();
    } else {
        status = ER_BUS_NO_SUCH_PROPERTY;
    }

    return status;
}

QStatus ServiceObject::Set(const char*ifcName, const char*propName, MsgArg& val)
{
    QStatus status = ER_OK;
    if ((0 == strcmp("int_val", propName)) && (val.typeId == ALLJOYN_INT32)) {
        prop_int_val = val.v_int32;
    } else if ((0 == strcmp("str_val", propName)) && (val.typeId == ALLJOYN_STRING)) {
        prop_str_val = val.v_string.str;
    } else if (0 == strcmp("ro_str", propName)) {
        status = ER_BUS_PROPERTY_ACCESS_DENIED;
    } else if (0 == strcmp("prop_signal", propName) && (val.typeId == ALLJOYN_STRING)) {
        prop_signal = val.v_string.str;
        //printf("\n\n\n Sending a signal \n\n\n");
        status = EmitTestSignal(prop_signal);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    } else {
        status = ER_BUS_NO_SUCH_PROPERTY;
    }

    return status;
}

bool ServiceObject::getobjectRegistered()
{
    return objectRegistered;
}


void ServiceObject::setobjectRegistered(bool value)
{
    objectRegistered = value;
}

const char* ServiceObject::getAlljoynInterfaceName() const
{
    return ::org::alljoyn::alljoyn_test::InterfaceName;
}

const char* ServiceObject::getServiceInterfaceName() const
{
    return ::org::alljoyn::service_test::InterfaceName;
}

const char* ServiceObject::getAlljoynWellKnownName() const
{
    return alljoynWellKnownName.c_str();
}

const char* ServiceObject::getServiceWellKnownName() const
{
    return serviceWellKnownName.c_str();
}

const char* ServiceObject::getAlljoynObjectPath() const
{
    return ::org::alljoyn::alljoyn_test::ObjectPath;
}

const char* ServiceObject::getServiceObjectPath() const
{
    return ::org::alljoyn::service_test::ObjectPath;
}

const char* ServiceObject::getAlljoynValuesInterfaceName() const
{
    return ::org::alljoyn::alljoyn_test::values::InterfaceName;
}

const char* ServiceObject::getServiceValuesInterfaceName() const
{
    return ::org::alljoyn::service_test::values::InterfaceName;
}

const char* ServiceObject::getAlljoynDummyInterfaceName1() const
{
    return ::org::alljoyn::alljoyn_test::dummy::InterfaceName1;
}

const char* ServiceObject::getAlljoynDummyInterfaceName2() const
{
    return ::org::alljoyn::alljoyn_test::dummy::InterfaceName2;
}

const char* ServiceObject::getAlljoynDummyInterfaceName3() const
{
    return ::org::alljoyn::alljoyn_test::dummy::InterfaceName3;
}

const char* ServiceObject::getAlljoynValuesDummyInterfaceName1() const
{
    return ::org::alljoyn::alljoyn_test::values::dummy::InterfaceName1;
}

const char* ServiceObject::getAlljoynValuesDummyInterfaceName2() const
{
    return ::org::alljoyn::alljoyn_test::values::dummy::InterfaceName2;
}

const char* ServiceObject::getAlljoynValuesDummyInterfaceName3() const
{
    return ::org::alljoyn::alljoyn_test::values::dummy::InterfaceName3;
}

const char* ServiceObject::getServiceDummyInterfaceName1() const
{
    return ::org::alljoyn::service_test::dummy::InterfaceName1;
}

const char* ServiceObject::getServiceDummyInterfaceName2() const
{
    return ::org::alljoyn::service_test::dummy::InterfaceName2;
}

const char* ServiceObject::getServiceDummyInterfaceName3() const
{
    return ::org::alljoyn::service_test::dummy::InterfaceName3;
}

const char* ServiceObject::getServiceValuesDummyInterfaceName1() const
{
    return ::org::alljoyn::service_test::values::dummy::InterfaceName1;
}

const char* ServiceObject::getServiceValuesDummyInterfaceName2() const
{
    return ::org::alljoyn::service_test::values::dummy::InterfaceName2;
}

const char* ServiceObject::getServiceValuesDummyInterfaceName3() const
{
    return ::org::alljoyn::service_test::values::dummy::InterfaceName3;
}
