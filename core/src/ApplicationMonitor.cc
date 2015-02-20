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

#include <alljoyn/securitymgr/ApplicationState.h>
#include <alljoyn/BusAttachment.h>

#include "ApplicationMonitor.h"
#include "SecurityManagerImpl.h"
#include "Common.h"

#include <algorithm>
#include <iostream>

#include <SecLibDef.h>

#include <vector>
#include <qcc/Debug.h>

#define QCC_MODULE "SEC_MGR"

#define PM_NOTIF_IFN "org.allseen.Security.PermissionMgmt.Notification"
#define PM_NOTIF_SIG "qa(yv)ya(yv)ua(ayay)"
#define PM_NOTIF_ARGS "version,publicKeyInfo,claimableState,trustAnchors,serialNumber,memberships"
#define PM_NOTIF_MEMBER "NotifyConfig"

#define PM_STUB_NOTIF_IFN "org.allseen.Security.PermissionMgmt.Stub.Notification"

using namespace ajn;
using namespace ajn::services;
using namespace ajn::securitymgr;

ApplicationMonitor::ApplicationMonitor(ajn::BusAttachment* ba,
                                       qcc::String notifIfn) :
    pinger(new AutoPinger(*ba)), busAttachment(ba)
{
    QStatus status = ER_FAIL;

    do {
        if (NULL == busAttachment) {
            QCC_LogError(status, ("NULL busAttachment !"));
            return;
        }

        if (pinger == NULL) {
            QCC_LogError(status, ("Could not get a pinger !"));
            return;
        }
        pinger->AddPingGroup(qcc::String(AUTOPING_GROUPNAME), *this, 5);

        // TODO: move stub code outside production code
        InterfaceDescription* stubIntf;
        status = busAttachment->CreateInterface(PM_STUB_NOTIF_IFN, stubIntf);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface '%s' on securitymgr bus attachment",
                                  PM_STUB_NOTIF_IFN));
            return;
        }
        stubIntf->AddSignal(PM_NOTIF_MEMBER, PM_NOTIF_SIG, PM_NOTIF_ARGS, 0);
        stubIntf->Activate();
        // TODO: end of todo

        const InterfaceDescription* intf = busAttachment->GetInterface(notifIfn.c_str());
        if (NULL == intf) {
            QCC_LogError(status, ("Failed to get interface '%s' on securitymgr bus attachment",
                                  notifIfn.c_str()));
            return;
        }

        status = busAttachment->RegisterSignalHandler(
            this, static_cast<MessageReceiver::SignalHandler>(&ApplicationMonitor::StateChangedSignalHandler),
            intf->GetMember(PM_NOTIF_MEMBER), NULL);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register a security signal handler."));
            break;
        }

        qcc::String matchRule("type='signal',interface='");
        matchRule.append(notifIfn);
        matchRule.append("',member='" PM_NOTIF_MEMBER "',sessionless='t'");
        QCC_DbgPrintf(("matchrule = %s", matchRule.c_str()));
        status = busAttachment->AddMatch(matchRule.c_str());
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to add match rule for security info signal."));
            break;
        }
    } while (0);
}

ApplicationMonitor::~ApplicationMonitor()
{
    delete pinger;
}

QStatus ApplicationMonitor::UnmarshalSecuritySignal(Message& msg, SecurityInfo& info)
{
    MsgArg* keyArrayArg;
    size_t keyArrayLen = 0;
    QStatus status = msg->GetArg(1)->Get("a(yv)", &keyArrayLen, &keyArrayArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to retrieve public keys."));
        return status;
    }

    if (keyArrayLen != 1) {
        QCC_LogError(status, ("Wrong number of keys public keys (%d).", keyArrayLen));
        return ER_FAIL;
    }

    if (ER_OK != (status = SecurityManagerImpl::UnmarshalPublicKey(&keyArrayArg[0], info.publicKey))) {
        QCC_LogError(status, ("Unmarshalling to ECCPublicKey struct failed"));
        return status;
    }

    uint8_t claimableState;
    if (ER_OK != (status = msg->GetArg(2)->Get("y", &claimableState))) {
        QCC_LogError(status, ("Failed to unmarshal claimable state."));
        return status;
    }
    info.claimState = (ajn::PermissionConfigurator::ClaimableState)claimableState;
    QCC_DbgPrintf(("claimState = %s", ToString(info.claimState)));

    MsgArg* rotArrayArg;
    size_t rotArrayLen = 0;
    if (ER_OK != (status = msg->GetArg(3)->Get("a(yv)", &rotArrayLen, &rotArrayArg))) {
        QCC_LogError(status, ("Failed to unmarshal array of RoTs."));
        return status;
    }
    QCC_DbgPrintf(("numberOfRoTs = %i", rotArrayLen));

    if (rotArrayLen > 0) {
        for (size_t i = 0; i < rotArrayLen; i++) {
            uint8_t rotUsage;
            MsgArg* rotArg;
            if (ER_OK != (status = rotArrayArg[i].Get("(yv)", &rotUsage, &rotArg))) {
                QCC_LogError(status, ("Failed to unmarshal array of RoT %i.", i));
            }

            ECCPublicKey rot;
            if (ER_OK != (status = SecurityManagerImpl::UnmarshalPublicKey(rotArg, rot))) {
                QCC_LogError(status, ("Failed to unmarshal array of RoT %i.", i));
                return status;
            }
            info.rootsOfTrust.push_back(rot);
        }
    }

    if (ER_OK != (status = msg->GetArg(4)->Get("u", &(info.policySerialNum)))) {
        QCC_LogError(status, ("Failed to unmarshal policy serial number."));
        return status;
    }
    QCC_DbgPrintf(("policySerialNumber = %d", info.policySerialNum));

    info.runningState = STATE_RUNNING;
    QCC_DbgPrintf(("runningState = %s", ToString(info.runningState)));

    return status;
}

void ApplicationMonitor::StateChangedSignalHandler(const InterfaceDescription::Member* member,
                                                   const char* sourcePath,
                                                   Message& msg)
{
    QCC_DbgPrintf(("Received NotifyConfig signal!!!"));

    QStatus status = ER_OK;

    SecurityInfo info;
    info.busName = qcc::String(msg->GetSender());
    QCC_DbgPrintf(("busname = %s", info.busName.c_str()));

    qcc::String localBusName = busAttachment->GetUniqueName();
    // ignore signals of local security manager
    if (info.busName == localBusName) {
        QCC_DbgPrintf(("Ignoring NotifyConfig signal of local Security Manager."));
        return;
    }

    if (ER_OK != (status = UnmarshalSecuritySignal(msg, info))) {
        QCC_LogError(status, ("Failed to unmarshal NotifyConfig signal."));
        return;
    }

    std::map<qcc::String, SecurityInfo>::iterator it = applications.find(info.busName);
    if (it != applications.end()) {
        // known bus name
        SecurityInfo oldInfo = it->second;
        it->second = info;

        NotifySecurityInfoListeners(&oldInfo, &info);
    } else {
        // new bus name
        applications[info.busName] = info;

        if (ER_OK != (status = pinger->AddDestination(AUTOPING_GROUPNAME, info.busName))) {
            QCC_LogError(status, ("Failed to add destination to AutoPinger."));
        }
        QCC_DbgPrintf(("Added destination %s", info.busName.c_str()));

        NotifySecurityInfoListeners(NULL, &info);
    }
}

std::vector<SecurityInfo> ApplicationMonitor::GetApplications() const
{
    if (!applications.empty()) {
        std::vector<SecurityInfo> apps;
        std::map<qcc::String, SecurityInfo>::const_iterator it = applications.begin();
        for (; it != applications.end(); ++it) {
            const SecurityInfo& appInfo = it->second;
            apps.push_back(appInfo);
        }
        return apps;
    }
    return std::vector<SecurityInfo>();
}

void ApplicationMonitor::RegisterSecurityInfoListener(SecurityInfoListener* al)
{
    if (NULL != al) {
        securityListenersMutex.Lock(__FILE__, __LINE__);
        listeners.push_back(al);
        securityListenersMutex.Unlock(__FILE__, __LINE__);
    }
}

void ApplicationMonitor::UnregisterSecurityInfoListener(SecurityInfoListener* al)
{
    securityListenersMutex.Lock(__FILE__, __LINE__);
    std::vector<SecurityInfoListener*>::iterator it = std::find(listeners.begin(), listeners.end(), al);
    if (listeners.end() != it) {
        listeners.erase(it);
    }
    securityListenersMutex.Unlock(__FILE__, __LINE__);
}

void ApplicationMonitor::NotifySecurityInfoListeners(const SecurityInfo* oldSecInfo,
                                                     const SecurityInfo* newSecInfo)
{
    securityListenersMutex.Lock(__FILE__, __LINE__);
    for (size_t i = 0; i < listeners.size(); ++i) {
        listeners[i]->OnSecurityStateChange(oldSecInfo, newSecInfo);
    }
    securityListenersMutex.Unlock(__FILE__, __LINE__);
}

void ApplicationMonitor::DestinationLost(const qcc::String& group, const qcc::String& destination)
{
    QCC_DbgPrintf(("DestinationLost %s\n", destination.data()));
    std::map<qcc::String, SecurityInfo>::iterator it = applications.find(destination);

    if (it != applications.end()) {
        /* we already know this application */
        NotifySecurityInfoListeners(&it->second, NULL);
        applications.erase(it);
    } else {
        /* We are monitoring an app not in the list. Remove it. */
        pinger->RemoveDestination(AUTOPING_GROUPNAME, destination);
    }
}

void ApplicationMonitor::DestinationFound(const qcc::String& group, const qcc::String& destination)
{
    QCC_DbgPrintf(("DestinationFound %s\n", destination.data()));
    std::map<qcc::String, SecurityInfo>::iterator it = applications.find(destination);

    if (it != applications.end()) {
        /* we already know this application */
        if (it->second.runningState != STATE_RUNNING) {
            SecurityInfo old = it->second;
            it->second.runningState = STATE_RUNNING;
            NotifySecurityInfoListeners(&old, &it->second);
        }
    } else {
        /* We are monitoring an app not in the list. Remove it. */
        pinger->RemoveDestination(AUTOPING_GROUPNAME, destination);
    }
}

#undef QCC_MODULE
