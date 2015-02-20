/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include "door_common.h"

#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <alljoyn/PermissionPolicy.h>

#if defined(QCC_OS_GROUP_WINDOWS)
#include <Winsock2.h> // gethostname
#endif

using namespace ajn;

namespace sample {
namespace securitymgr {
namespace door {
bool DoorAuthListener::RequestCredentials(const char* authMechanism,
                                          const char* authPeer,
                                          uint16_t authCount,
                                          const char* userId,
                                          uint16_t credMask,
                                          Credentials& creds)
{
    printf("RequestCredentials %s\n", authMechanism);

    // allow ECDHE_NULL and  DSA sessions
    if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0 || strcmp(authMechanism, KEYX_ECDHE_DSA)) {
        creds.SetExpiration(100);          /* set the master secret expiry time to 100 seconds */
        return true;
    }
    return false;
}

bool DoorAuthListener::VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
{
    printf("VerifyCredentials %s\n", authMechanism);
    return false;
}

void DoorAuthListener::AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success)
{
    printf("AuthenticationComplete %s\n", authMechanism);
}

Door::Door(BusAttachment& ba) :
    ajn::BusObject(DOOR_OBJECT_PATH), open(false)
{
    const InterfaceDescription* secPermIntf = ba.GetInterface(DOOR_INTERFACE);
    assert(secPermIntf);
    AddInterface(*secPermIntf);

    /* Register the method handlers with the object */
    const MethodEntry methodEntries[] = {
        { secPermIntf->GetMember(DOOR_OPEN), static_cast<MessageReceiver::MethodHandler>(&Door::Open) },
        { secPermIntf->GetMember(DOOR_CLOSE), static_cast<MessageReceiver::MethodHandler>(&Door::Close) },
        { secPermIntf->GetMember(DOOR_GET_STATE), static_cast<MessageReceiver::MethodHandler>(&Door::GetState) }
    };
    QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
        printf("Failed to register method handlers for the Door.\n");
    }

    stateSignal = secPermIntf->GetMember(DOOR_STATE_CHANGED);
}

QStatus Door::RegisterToAbout()
{
    std::vector<qcc::String> intfs;
    intfs.push_back(DOOR_INTERFACE);
    QStatus status =  ajn::services::AboutServiceApi::getInstance()->AddObjectDescription(DOOR_OBJECT_PATH, intfs);
    if (status == ER_OK) {
        status = ajn::services::AboutServiceApi::getInstance()->Announce();
    }
    return status;
}

void Door::SendDoorEvent(bool newState)
{
/*
    MsgArg outArg;
    outArg.Set("b", &newState);
    Signal(NULL, SESSION_ID_ALL_HOSTED, *stateSignal, &outArg, 1, 0, 0,  NULL);
 */
}

void Door::ReplyWithBoolean(bool answer, Message& msg)
{
    MsgArg outArg;
    outArg.Set("b", &answer);

    if (ER_OK != MethodReply(msg, &outArg, 1)) {
        printf("ReplyWithBoolean: Error sending reply.\n");
    }
}

void Door::Open(const InterfaceDescription::Member* member, Message& msg)
{
    printf("Door Open called\n");
    if (open == false) {
        open = true;
        SendDoorEvent(true);
    }
    ReplyWithBoolean(true, msg);
}

void Door::Close(const ajn::InterfaceDescription::Member* member,
                 ajn::Message& msg)
{
    printf("Door Close called\n");
    if (open) {
        open = false;
        SendDoorEvent(true);
    }
    ReplyWithBoolean(true, msg);
}

QStatus Door::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    printf("Door::Get(%s)@%s\n", propName, ifcName);

    if (strcmp(ifcName, DOOR_INTERFACE) == 0 && strcmp(propName, DOOR_STATE) == 0) {
        val.Set("b", open);
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

void Door::GetState(const ajn::InterfaceDescription::Member* member,
                    ajn::Message& msg)
{
    printf("Door GetState called\n");
    ReplyWithBoolean(open, msg);
}

QStatus DoorCommon::CreateInterface()
{
    InterfaceDescription* doorIntf = NULL;
    QStatus status = ba.CreateInterface(DOOR_INTERFACE, doorIntf, DOOR_INTF_SECURE ? AJ_IFC_SECURITY_REQUIRED : 0);
    if (ER_OK == status) {
        printf("Interface created.\n");
        doorIntf->AddMethod(DOOR_OPEN, NULL, "b", "success");
        doorIntf->AddMethod(DOOR_CLOSE, NULL, "b", "success");
        doorIntf->AddMethod(DOOR_GET_STATE, NULL, "b", "state");
        doorIntf->AddSignal(DOOR_STATE_CHANGED, "b", "state");
        doorIntf->AddProperty(DOOR_STATE, "b", PROP_ACCESS_RW);
        doorIntf->Activate();
    } else {
        printf("Failed to create Secure PermissionMgmt interface.\n");
    }

    return status;
}

QStatus DoorCommon::FillAboutPropertyStore()
{
    QStatus status = ER_OK;

    status = aboutPropertyStore.setDeviceId("Linux");
    if (status != ER_OK) {
        return status;
    }

    qcc::GUID128 appId;
    status = aboutPropertyStore.setAppId(appId.ToString(), "en");
    if (status != ER_OK) {
        return status;
    }

    std::vector<qcc::String> languages(1);
    languages[0] = "en";
    status = aboutPropertyStore.setSupportedLangs(languages);
    if (status != ER_OK) {
        return status;
    }
    status = aboutPropertyStore.setDefaultLang("en");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setAppName(appName, "en");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setModelNumber("1");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setSoftwareVersion("");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setAjSoftwareVersion(ajn::GetVersion());
    if (status != ER_OK) {
        return status;
    }

    char buf[64];
    gethostname(buf, sizeof(buf));
    status = aboutPropertyStore.setDeviceName(buf, "en");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setDescription(appName, "en");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setManufacturer("QEO LLC", "en");
    if (status != ER_OK) {
        return status;
    }

    status = aboutPropertyStore.setSupportUrl("http://www.alljoyn.org");
    if (status != ER_OK) {
        return status;
    }
    return status;
}

QStatus DoorCommon::AdvertiseApplication()
{
    QStatus status = FillAboutPropertyStore();
    if (status != ER_OK) {
        std::cerr << "Could not fill propertystore" << std::endl;
        return status;
    }

    AboutServiceApi::Init(ba, aboutPropertyStore);
    if (!AboutServiceApi::getInstance()) {
        std::cerr << "Could not get instance" << std::endl;
        return ER_FAIL;
    }

    AboutServiceApi::getInstance()->Register(DOOR_APPLICATION_PORT);
    status = ba.RegisterBusObject(*AboutServiceApi::getInstance());
    if (status != ER_OK) {
        std::cerr << "Could not register about bus object" << std::endl;
        return status;
    }

    status = AboutServiceApi::getInstance()->Announce();
    if (status != ER_OK) {
        std::cerr << "Could not announce" << std::endl;
        return status;
    }

    return ER_OK;
}

QStatus DoorCommon::init(const char* keyStoreName, bool provider)
{
    QStatus status = CreateInterface();

    if (status != ER_OK) {
        return status;
    }

    status = ba.Start();
    if (status != ER_OK) {
        return status;
    }

    status = ba.Connect();
    if (status != ER_OK) {
        return status;
    }

    status = ba.EnablePeerSecurity(KEYX_ECDHE_DSA " " KEYX_ECDHE_NULL, new DoorAuthListener(), keyStoreName);
    if (status != ER_OK) {
        return status;
    }

    PermissionPolicy::Rule::Member* member = new  ajn::PermissionPolicy::Rule::Member[1];
    member->SetMemberName("*");
    uint8_t mask = (provider ? PermissionPolicy::Rule::Member::ACTION_PROVIDE | 0 // | 0 was added to avoid an undefined reference at link time
                    : PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    member->SetActionMask(mask);
    member->SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);

    PermissionPolicy::Rule* manifestRule = new PermissionPolicy::Rule[1];
    manifestRule->SetInterfaceName(DOOR_INTERFACE);
    manifestRule->SetMembers(1, member);
    status = ba.GetPermissionConfigurator().SetPermissionManifest(manifestRule, 1);
    if (status != ER_OK) {
        return status;
    }

    status = AdvertiseApplication();

    return status;
}

DoorCommon::~DoorCommon()
{
}
}
}
}
