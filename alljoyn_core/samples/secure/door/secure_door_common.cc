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

#include <qcc/Thread.h>
#include <alljoyn/PermissionPolicy.h>

#if defined(QCC_OS_GROUP_WINDOWS)
#include <Winsock2.h> // gethostname
#endif

using namespace ajn;
using namespace qcc;

namespace sample {
namespace secure {
namespace door {

void DoorCommonPCL::PolicyChanged()
{
    lock.Lock();
    QStatus status;
    PermissionConfigurator::ApplicationState appState;
    if (ER_OK == (status = ba.GetPermissionConfigurator().GetApplicationState(appState))) {
        if (PermissionConfigurator::ApplicationState::CLAIMED == appState) {
            qcc::Sleep(250); // Allow SecurityMgmtObj to send method reply (see ASACORE-2331)
            // Upon a policy update, existing connections are invalidated
            // and one needs to make them valid again.
            if (ER_OK != (status = ba.SecureConnectionAsync(nullptr, true))) {
                fprintf(stderr, "Attempt to secure the connection - status (%s)\n",
                        QCC_StatusText(status));
            }
            sem.Signal();
        }
    } else {
        fprintf(stderr, "Failed to GetApplicationState - status (%s)\n", QCC_StatusText(status));
    }
    lock.Unlock();
}

bool DoorCommonPCL::WaitForClaimedState()
{
    lock.Lock();
    PermissionConfigurator::ApplicationState appState;

    QStatus status = ba.GetPermissionConfigurator().GetApplicationState(appState);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to get application state - status (%s)\n",
                QCC_StatusText(status));
        lock.Unlock();
        return false;
    }

    if (PermissionConfigurator::ApplicationState::CLAIMED == appState) {
        printf("Already claimed !\n");
        lock.Unlock();
        return true;
    }

    printf("Waiting to be claimed...\n");
    status = sem.Wait(lock);
    if (ER_OK != status) {
        lock.Unlock();
        return false;
    }

    printf("Claimed !\n");
    lock.Unlock();
    return true;
}

Door::Door(BusAttachment* ba) :
    BusObject(DOOR_OBJECT_PATH), autoSignal(false), open(false)
{
    const InterfaceDescription* secPermIntf = ba->GetInterface(DOOR_INTERFACE);
    assert(secPermIntf);
    QStatus status = AddInterface(*secPermIntf, ANNOUNCED);
    assert(ER_OK == status);

    /* Register the method handlers with the door bus object */
    const MethodEntry methodEntries[] = {
        { secPermIntf->GetMember(DOOR_OPEN), static_cast<MessageReceiver::MethodHandler>(&Door::Open) },
        { secPermIntf->GetMember(DOOR_CLOSE), static_cast<MessageReceiver::MethodHandler>(&Door::Close) },
        { secPermIntf->GetMember(DOOR_GET_STATE), static_cast<MessageReceiver::MethodHandler>(&Door::GetState) }
    };
    status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    assert(ER_OK == status);

    stateSignal = secPermIntf->GetMember(DOOR_STATE_CHANGED);
}

void Door::SendDoorEvent()
{
    printf("Sending door event ...\n");
    MsgArg outArg;
    outArg.Set("b", open);

    QStatus status = Signal(nullptr, SESSION_ID_ALL_HOSTED, *stateSignal, &outArg, 1, 0, 0,  nullptr);
    if (status != ER_OK) {
        fprintf(stderr, "Failed to send Signal - status (%s)\n", QCC_StatusText(status));
    }
}

void Door::ReplyWithBoolean(bool answer, Message& msg)
{
    MsgArg outArg;
    outArg.Set("b", answer);

    QStatus status = MethodReply(msg, &outArg, 1);
    if (status != ER_OK) {
        fprintf(stderr, "Failed to send MethodReply - status (%s)\n", QCC_StatusText(status));
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

void DoorCommon::CreateInterface()
{
    InterfaceDescription* doorIntf = nullptr;
    // Create a secure door interface on the bus attachment
    QStatus status = ba->CreateInterface(DOOR_INTERFACE, doorIntf, AJ_IFC_SECURITY_REQUIRED);
    assert(ER_OK == status);

    status = doorIntf->AddMethod(DOOR_OPEN, nullptr, "b", "success");
    assert(ER_OK == status);
    status = doorIntf->AddMethod(DOOR_CLOSE, nullptr, "b", "success");
    assert(ER_OK == status);
    status = doorIntf->AddMethod(DOOR_GET_STATE, nullptr, "b", "state");
    assert(ER_OK == status);
    status = doorIntf->AddSignal(DOOR_STATE_CHANGED, "b", "state", 0);
    assert(ER_OK == status);
    status = doorIntf->AddProperty(DOOR_STATE, "b", PROP_ACCESS_RW);
    assert(ER_OK == status);
    doorIntf->Activate();

    printf("Secure door interface was created.\n");
}

void DoorCommon::SetAboutData()
{
    GUID128 appId;
    QStatus status = aboutData.SetAppId(appId.ToString().c_str());
    assert(ER_OK == status);

    char buf[64];
    gethostname(buf, sizeof(buf));
    status = aboutData.SetDeviceName(buf);
    assert(ER_OK == status);

    GUID128 deviceId;
    status = aboutData.SetDeviceId(deviceId.ToString().c_str());
    assert(ER_OK == status);

    status = aboutData.SetAppName(appName.c_str());
    assert(ER_OK == status);
    status = aboutData.SetManufacturer("Manufacturer");
    assert(ER_OK == status);
    status = aboutData.SetModelNumber("1");
    assert(ER_OK == status);
    status = aboutData.SetDescription(appName.c_str());
    assert(ER_OK == status);
    status = aboutData.SetDateOfManufacture("2015-04-14");
    assert(ER_OK == status);
    status = aboutData.SetSoftwareVersion("0.1");
    assert(ER_OK == status);
    status = aboutData.SetHardwareVersion("0.0.1");
    assert(ER_OK == status);
    status = aboutData.SetSupportUrl("https://allseenalliance.org/");
    assert(ER_OK == status);

    assert(aboutData.IsValid());
}

void DoorCommon::HostSession()
{
    SessionOpts opts;
    SessionPort port = DOOR_APPLICATION_PORT;

    QStatus status = ba->BindSessionPort(port, opts, spl);
    assert(ER_OK == status);
}

void DoorCommon::AnnounceAbout()
{
    SetAboutData();

    QStatus status = aboutObj->Announce(DOOR_APPLICATION_PORT, aboutData);
    assert(ER_OK == status);
}

void DoorCommon::Init(bool provider, PermissionConfigurationListener* pcl)
{
    CreateInterface();

    QStatus status = ba->Start();
    assert(ER_OK == status);

    status = ba->Connect();
    assert(ER_OK == status);

    GUID128 psk;
    status = ba->EnablePeerSecurity(KEYX_ECDHE_DSA " " KEYX_ECDHE_NULL " " KEYX_ECDHE_PSK,
                                    provider ? new DefaultECDHEAuthListener(
                                        psk.GetBytes(), GUID128::SIZE) : new DefaultECDHEAuthListener(), nullptr, false, pcl);
    assert(ER_OK == status);

    if (provider) {
        printf("Allow doors to be claimable using a PSK.\n");
        status = ba->GetPermissionConfigurator().SetClaimCapabilities(
            PermissionConfigurator::CAPABLE_ECDHE_PSK | PermissionConfigurator::CAPABLE_ECDHE_NULL);
        assert(ER_OK == status);

        status = ba->GetPermissionConfigurator().SetClaimCapabilityAdditionalInfo(
            PermissionConfigurator::PSK_GENERATED_BY_APPLICATION);
        assert(ER_OK == status);
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
    assert(ER_OK == status);

    if (provider) {
        PermissionConfigurator::ApplicationState state;
        status = ba->GetPermissionConfigurator().GetApplicationState(state);
        assert(ER_OK == status);

        if (PermissionConfigurator::CLAIMABLE == state) {
            printf("Door provider is not claimed.\n");
            printf("The provider can be claimed using PSK with an application generated secret.\n");
            printf("PSK = (%s)\n", psk.ToString().c_str());
        }
    }

    HostSession();
}

void DoorCommon::UpdateManifest(const PermissionPolicy::Acl& manifest)
{
    PermissionPolicy::Rule* rules = const_cast<PermissionPolicy::Rule*> (manifest.GetRules());

    QStatus status = ba->GetPermissionConfigurator().SetPermissionManifest(rules, manifest.GetRulesSize());
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetPermissionManifest - status (%s)\n", QCC_StatusText(status));
        return;
    }

    status = ba->GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::NEED_UPDATE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetApplicationState - status (%s)\n", QCC_StatusText(status));
    }
}

void DoorCommon::Fini()
{
    /**
     * This is needed to make sure that the authentication listener is removed before the
     * bus attachment is destructed.
     * Use an empty string as a first parameter (authMechanism) to avoid resetting the keyStore
     * so previously claimed apps can still be so after restarting.
     **/
    ba->EnablePeerSecurity("", nullptr, nullptr, true);

    delete aboutObj;
    aboutObj = nullptr;

    ba->Disconnect();
    ba->Stop();
    ba->Join();

    delete ba;
    ba = nullptr;
}

DoorCommon::~DoorCommon()
{
}
}
}
}
