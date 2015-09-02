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

#include "secure_door_common.h"

#include <alljoyn/PermissionPolicy.h>

#if defined(QCC_OS_GROUP_WINDOWS)
#include <Winsock2.h> // gethostname
#endif

using namespace ajn;
using namespace qcc;

namespace sample {
namespace secure {
namespace door {

Door::Door(BusAttachment* ba) :
    BusObject(DOOR_OBJECT_PATH), autoSignal(false), open(false)
{
    const InterfaceDescription* secPermIntf = ba->GetInterface(DOOR_INTERFACE);
    assert(secPermIntf);
    AddInterface(*secPermIntf, ANNOUNCED);

    /* Register the method handlers with the door bus object */
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

void Door::SendDoorEvent()
{
    printf("Sending door event ...\n");
    MsgArg outArg;
    outArg.Set("b", open);
    Signal(nullptr, SESSION_ID_ALL_HOSTED, *stateSignal, &outArg, 1, 0, 0,  nullptr);
}

void Door::ReplyWithBoolean(bool answer, Message& msg)
{
    MsgArg outArg;
    outArg.Set("b", answer);

    if (ER_OK != MethodReply(msg, &outArg, 1)) {
        printf("ReplyWithBoolean: Error sending reply.\n");
    }
}

void Door::Open(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    printf("Door Open method was called\n");
    if (open == false) {
        open = true;
        if (autoSignal) {
            SendDoorEvent();
        }
    }
    ReplyWithBoolean(true, msg);
}

void Door::Close(const InterfaceDescription::Member* member,
                 Message& msg)
{
    QCC_UNUSED(member);

    printf("Door Close method called\n");
    if (open) {
        open = false;
        if (autoSignal) {
            SendDoorEvent();
        }
    }
    ReplyWithBoolean(true, msg);
}

QStatus Door::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    printf("Door::Get(%s)@%s\n", propName, ifcName);
    // Only one property is available
    if (strcmp(ifcName, DOOR_INTERFACE) == 0 && strcmp(propName, DOOR_STATE) == 0) {
        val.Set("b", open);
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

void Door::GetState(const InterfaceDescription::Member* member,
                    Message& msg)
{
    QCC_UNUSED(member);

    printf("Door GetState method was called\n");
    ReplyWithBoolean(open, msg);
}

QStatus DoorCommon::CreateInterface()
{
    InterfaceDescription* doorIntf = nullptr;
    // Create a secure door interface on the bus attachment
    QStatus status = ba->CreateInterface(DOOR_INTERFACE, doorIntf, AJ_IFC_SECURITY_REQUIRED);
    if (ER_OK == status) {
        printf("Secure door interface was created.\n");
        doorIntf->AddMethod(DOOR_OPEN, nullptr, "b", "success");
        doorIntf->AddMethod(DOOR_CLOSE, nullptr, "b", "success");
        doorIntf->AddMethod(DOOR_GET_STATE, nullptr, "b", "state");
        doorIntf->AddSignal(DOOR_STATE_CHANGED, "b", "state", 0);
        doorIntf->AddProperty(DOOR_STATE, "b", PROP_ACCESS_RW);
        doorIntf->Activate();
    } else {
        printf("Failed to create Secure PermissionMgmt interface.\n");
    }

    return status;
}

QStatus DoorCommon::SetAboutData()
{
    GUID128 appId;
    aboutData.SetAppId(appId.ToString().c_str());

    char buf[64];
    gethostname(buf, sizeof(buf));
    aboutData.SetDeviceName(buf);

    GUID128 deviceId;
    aboutData.SetDeviceId(deviceId.ToString().c_str());

    aboutData.SetAppName(appName.c_str());
    aboutData.SetManufacturer("Manufacturer");
    aboutData.SetModelNumber("1");
    aboutData.SetDescription(appName.c_str());
    aboutData.SetDateOfManufacture("2015-04-14");
    aboutData.SetSoftwareVersion("0.1");
    aboutData.SetHardwareVersion("0.0.1");
    aboutData.SetSupportUrl("https://allseenalliance.org/");

    if (!aboutData.IsValid()) {
        printf("Invalid about data.\n");
        return ER_FAIL;
    }
    return ER_OK;
}

QStatus DoorCommon::HostSession()
{
    SessionOpts opts;
    SessionPort port = DOOR_APPLICATION_PORT;

    QStatus status = ba->BindSessionPort(port, opts, spl);

    if (ER_OK != status) {
        printf("Failed to BindSesssionPort\n");
    }

    return status;
}

QStatus DoorCommon::AnnounceAbout()
{
    QStatus status = SetAboutData();
    if (ER_OK != status) {
        printf("Failed to set about data - status(%s)\n", QCC_StatusText(status));
    }

    status = aboutObj->Announce(DOOR_APPLICATION_PORT, aboutData);
    if (ER_OK != status) {
        printf("Announcing about failed - status(%s)\n", QCC_StatusText(status));
    }
    return status;
}

QStatus DoorCommon::Init(bool provider)
{
    QStatus status = CreateInterface();

    if (status != ER_OK) {
        return status;
    }

    status = ba->Start();
    if (status != ER_OK) {
        return status;
    }

    status = ba->Connect();
    if (status != ER_OK) {
        return status;
    }

    GUID128 psk;
    status = ba->EnablePeerSecurity(KEYX_ECDHE_DSA " " KEYX_ECDHE_NULL " " KEYX_ECDHE_PSK,
                                    provider ? new DefaultECDHEAuthListener(
                                        psk.GetBytes(), GUID128::SIZE) : new DefaultECDHEAuthListener());
    if (status != ER_OK) {
        return status;
    }

    if (provider) {
        printf("Allow doors to be claimable using a PSK.\n");
        status = ba->GetPermissionConfigurator().SetClaimCapabilities(
            PermissionConfigurator::CAPABLE_ECDHE_PSK | PermissionConfigurator::CAPABLE_ECDHE_NULL);
        if (status != ER_OK) {
            printf("Failed to SetClaimCapabilities - status(%s)\n", QCC_StatusText(status));
        }
        status = ba->GetPermissionConfigurator().SetClaimCapabilityAdditionalInfo(
            PermissionConfigurator::PSK_GENERATED_BY_APPLICATION);
        if (status != ER_OK) {
            printf("Failed to SetClaimCapabilityAdditionalInfo - status(%s)\n", QCC_StatusText(status));
        }
        PermissionConfigurator::ApplicationState state;
        if (ER_OK == ba->GetPermissionConfigurator().GetApplicationState(state)) {
            if (PermissionConfigurator::CLAIMABLE == state) {
                printf("Door provider is not claimed.\n");
                printf("The provider can be claimed using PSK with an application generated secret.\n");
                printf("PSK = (%s)\n", psk.ToString().c_str());
            }
        }
    }

    PermissionPolicy::Rule manifestRule;
    manifestRule.SetInterfaceName(DOOR_INTERFACE);

    if (provider) {
        // Set a very flexible default manifest for the door provider
        PermissionPolicy::Rule::Member members[2];
        members[0].SetMemberName("*");
        members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        members[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        members[1].SetMemberName("*");
        members[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        members[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        manifestRule.SetMembers(2, members);
    } else {
        // Set a very flexible default manifest for the door consumer
        PermissionPolicy::Rule::Member member;
        member.SetMemberName("*");
        member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY |
                             PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
        manifestRule.SetMembers(1, &member);
    }

    status = ba->GetPermissionConfigurator().SetPermissionManifest(&manifestRule, 1);
    if (status != ER_OK) {
        return status;
    }

    status = HostSession();
    if (status != ER_OK) {
        return status;
    }

    return status;
}

void DoorCommon::UpdateManifest(const PermissionPolicy::Acl& manifest)
{
    PermissionPolicy::Rule* rules = const_cast<PermissionPolicy::Rule*> (manifest.GetRules());

    ba->GetPermissionConfigurator().SetPermissionManifest(rules, manifest.GetRulesSize());
    ba->GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::NEED_UPDATE);
}

QStatus DoorCommon::Fini()
{
    // Empty string first parameter (authMechanism) to avoid resetting keyStore.
    QStatus status = ba->EnablePeerSecurity("", nullptr, nullptr, true);
    if (status != ER_OK) {
        printf("Failed to disable peer security at destruction...\n");
    }

    delete aboutObj;
    aboutObj = nullptr;

    ba->Disconnect();
    ba->Stop();
    ba->Join();

    delete ba;
    ba = nullptr;

    return status;
}

DoorCommon::~DoorCommon()
{
}
}
}
}
