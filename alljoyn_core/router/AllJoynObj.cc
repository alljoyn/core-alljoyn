/**
 * @file
 * * This file implements the org.alljoyn.Bus and org.alljoyn.Daemon interfaces
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <algorithm>
#include <assert.h>
#include <limits>
#include <map>
#include <set>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/SocketStream.h>
#include <qcc/StreamPump.h>
#include <qcc/STLContainer.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>
#include <BusEndpoint.h>

#include "DaemonRouter.h"
#include "AllJoynObj.h"
#include "TransportList.h"
#include "SessionInternal.h"
#include "BusController.h"
#include "VirtualEndpoint.h"
#include "EndpointHelper.h"
#include "ns/IpNameService.h"
#include "AllJoynPeerObj.h"
#include "ConfigDB.h"
#include "NameTable.h"

#define QCC_MODULE "ALLJOYN_OBJ"

using namespace std;
using namespace qcc;

namespace ajn {

void* AllJoynObj::NameMapEntry::truthiness = reinterpret_cast<void*>(true);
int AllJoynObj::JoinSessionThread::jstCount = 0;
struct AllJoynObj::PingAlarmContext {
    enum Type {
        TRANSPORT_CONTEXT,
        REPLY_CONTEXT
    };
    Type type;

    String name;
    String sender;
    PingAlarmContext(Type type, String name, String sender = "") : type(type), name(name), sender(sender) { };
};

void AllJoynObj::AcquireLocks()
{
    /*
     * Locks must be acquired in the following order since tha caller of
     * this method may already have the name table lock
     */
    router.LockNameTable();
}

void AllJoynObj::ReleaseLocks()
{
    router.UnlockNameTable();
}

AllJoynObj::AllJoynObj(Bus& bus, BusController* busController) :
    BusObject(org::alljoyn::Bus::ObjectPath, false),
    bus(bus),
    router(reinterpret_cast<DaemonRouter&>(bus.GetInternal().GetRouter())),
    foundNameSignal(NULL),
    lostAdvNameSignal(NULL),
    sessionLostSignal(NULL),
    mpSessionChangedSignal(NULL),
    mpSessionChangedWithReason(NULL),
    mpSessionJoinedSignal(NULL),
    guid(bus.GetInternal().GetGlobalGUID()),
    exchangeNamesSignal(NULL),
    detachSessionSignal(NULL),
    timer("NameReaper"),
    isStopping(false),
    busController(busController)
{
}

AllJoynObj::~AllJoynObj()
{
    IpNameService::Instance().UnregisterListener(*this);
    bus.UnregisterBusObject(*this);
    router.RemoveBusNameListener(this);

    Stop();
    Join();
    outgoingPingMap.clear();
    incomingPingMap.clear();
}

QStatus AllJoynObj::Init()
{
    QStatus status;

    /* Make this object implement org.alljoyn.Bus */
    const InterfaceDescription* alljoynIntf = bus.GetInterface(org::alljoyn::Bus::InterfaceName);
    if (!alljoynIntf) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Failed to get %s interface", org::alljoyn::Bus::InterfaceName));
        return status;
    }

    /* Hook up the methods to their handlers */
    const MethodEntry methodEntries[] = {
        { alljoynIntf->GetMember("AdvertiseName"),            static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::AdvertiseName) },
        { alljoynIntf->GetMember("CancelAdvertiseName"),      static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::CancelAdvertiseName) },
        { alljoynIntf->GetMember("FindAdvertisedName"),       static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::FindAdvertisedName) },
        { alljoynIntf->GetMember("FindAdvertisedNameByTransport"),       static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::FindAdvertisedNameByTransport) },
        { alljoynIntf->GetMember("CancelFindAdvertisedName"), static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::CancelFindAdvertisedName) },
        { alljoynIntf->GetMember("CancelFindAdvertisedNameByTransport"), static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::CancelFindAdvertisedNameByTransport) },
        { alljoynIntf->GetMember("BindSessionPort"),          static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::BindSessionPort) },
        { alljoynIntf->GetMember("UnbindSessionPort"),        static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::UnbindSessionPort) },
        { alljoynIntf->GetMember("JoinSession"),              static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::JoinSession) },
        { alljoynIntf->GetMember("LeaveSession"),             static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::LeaveSession) },
        { alljoynIntf->GetMember("LeaveHostedSession"),       static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::LeaveHostedSession) },
        { alljoynIntf->GetMember("LeaveJoinedSession"),       static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::LeaveJoinedSession) },
        { alljoynIntf->GetMember("GetSessionFd"),             static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::GetSessionFd) },
        { alljoynIntf->GetMember("SetLinkTimeout"),           static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::SetLinkTimeout) },
        { alljoynIntf->GetMember("AliasUnixUser"),            static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::AliasUnixUser) },
        { alljoynIntf->GetMember("OnAppSuspend"),             static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::OnAppSuspend) },
        { alljoynIntf->GetMember("OnAppResume"),              static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::OnAppResume) },
        { alljoynIntf->GetMember("CancelSessionlessMessage"), static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::CancelSessionlessMessage) },
        { alljoynIntf->GetMember("RemoveSessionMember"),      static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::RemoveSessionMember) },
        { alljoynIntf->GetMember("GetHostInfo"),              static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::GetHostInfo) },
        { alljoynIntf->GetMember("ReloadConfig"),             static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::ReloadConfig) },
        { alljoynIntf->GetMember("Ping"),                     static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::Ping) },
        { alljoynIntf->GetMember("FindAdvertisementByTransport"),        static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::FindAdvertisementByTransport) },
        { alljoynIntf->GetMember("CancelFindAdvertisementByTransport"),  static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::CancelFindAdvertisementByTransport) },
        { alljoynIntf->GetMember("SetIdleTimeouts"),  static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::SetIdleTimeouts) }
    };

    AddInterface(*alljoynIntf);
    status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
    if (ER_OK != status) {
        QCC_LogError(status, ("AddMethods for %s failed", org::alljoyn::Bus::InterfaceName));
    }

    foundNameSignal = alljoynIntf->GetMember("FoundAdvertisedName");
    lostAdvNameSignal = alljoynIntf->GetMember("LostAdvertisedName");
    sessionLostSignal = alljoynIntf->GetMember("SessionLost");
    sessionLostWithReasonSignal = alljoynIntf->GetMember("SessionLostWithReason");
    sessionLostWithReasonAndDispositionSignal = alljoynIntf->GetMember("SessionLostWithReasonAndDisposition");
    mpSessionChangedSignal = alljoynIntf->GetMember("MPSessionChanged");
    mpSessionChangedWithReason = alljoynIntf->GetMember("MPSessionChangedWithReason");

    const InterfaceDescription* busSessionIntf = bus.GetInterface(org::alljoyn::Bus::Peer::Session::InterfaceName);
    if (!busSessionIntf) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Failed to get %s interface", org::alljoyn::Bus::Peer::Session::InterfaceName));
        return status;
    }

    mpSessionJoinedSignal = busSessionIntf->GetMember("SessionJoined");

    /* Make this object implement org.alljoyn.Daemon */
    daemonIface = bus.GetInterface(org::alljoyn::Daemon::InterfaceName);
    if (!daemonIface) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Failed to get %s interface", org::alljoyn::Daemon::InterfaceName));
        return status;
    }

    /* Hook up the methods to their handlers */
    const MethodEntry daemonMethodEntries[] = {
        { daemonIface->GetMember("AttachSession"),     static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::AttachSession) },
        { daemonIface->GetMember("GetSessionInfo"),    static_cast<MessageReceiver::MethodHandler>(&AllJoynObj::GetSessionInfo) }
    };
    AddInterface(*daemonIface);
    status = AddMethodHandlers(daemonMethodEntries, ArraySize(daemonMethodEntries));
    if (ER_OK != status) {
        QCC_LogError(status, ("AddMethods for %s failed", org::alljoyn::Daemon::InterfaceName));
    }

    exchangeNamesSignal = daemonIface->GetMember("ExchangeNames");
    assert(exchangeNamesSignal);
    detachSessionSignal = daemonIface->GetMember("DetachSession");
    assert(detachSessionSignal);

    /* Register a signal handler for ExchangeNames */
    if (ER_OK == status) {
        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&AllJoynObj::ExchangeNamesSignalHandler),
                                           daemonIface->GetMember("ExchangeNames"),
                                           NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register ExchangeNamesSignalHandler"));
        }
    }

    /* Register a signal handler for NameChanged bus-to-bus signal */
    if (ER_OK == status) {
        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&AllJoynObj::NameChangedSignalHandler),
                                           daemonIface->GetMember("NameChanged"),
                                           NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register NameChangedSignalHandler"));
        }
    }

    /* Register a signal handler for DetachSession bus-to-bus signal */
    if (ER_OK == status) {
        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&AllJoynObj::DetachSessionSignalHandler),
                                           daemonIface->GetMember("DetachSession"),
                                           NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register DetachSessionSignalHandler"));
        }
    }


    /* Register a name table listener */
    router.AddBusNameListener(this);

    /* Register as a listener for all the remote transports */
    if (ER_OK == status) {
        TransportList& transList = bus.GetInternal().GetTransportList();
        status = transList.RegisterListener(this);
    }

    /* Start the name reaper */
    if (ER_OK == status) {
        status = timer.Start();
    }

    if (ER_OK == status) {
        status = bus.RegisterBusObject(*this);
    }

    IpNameService::Instance().RegisterListener(*this);

    return status;
}

QStatus AllJoynObj::Stop()
{
    /* Stop any outstanding JoinSessionThreads */
    joinSessionThreadsLock.Lock(MUTEX_CONTEXT);
    isStopping = true;
    vector<JoinSessionThread*>::iterator it = joinSessionThreads.begin();
    while (it != joinSessionThreads.end()) {
        (*it)->Stop();
        ++it;
    }
    joinSessionThreadsLock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

QStatus AllJoynObj::Join()
{
    /* Wait for any outstanding JoinSessionThreads */
    joinSessionThreadsLock.Lock(MUTEX_CONTEXT);
    while (!joinSessionThreads.empty()) {
        joinSessionThreadsLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(50);
        joinSessionThreadsLock.Lock(MUTEX_CONTEXT);
    }
    joinSessionThreadsLock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

void AllJoynObj::ObjectRegistered(void)
{
    QStatus status;
    LocalEndpoint localEndpoint = bus.GetInternal().GetLocalEndpoint();


    /* Acquire org.alljoyn.Bus name */
    uint32_t disposition = DBUS_REQUEST_NAME_REPLY_EXISTS;
    status = router.AddAlias(org::alljoyn::Bus::WellKnownName,
                             localEndpoint->GetUniqueName(),
                             DBUS_NAME_FLAG_DO_NOT_QUEUE,
                             disposition,
                             NULL,
                             NULL);
    if ((ER_OK != status) || (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != disposition)) {
        status = (ER_OK == status) ? ER_FAIL : status;
        QCC_LogError(status, ("Failed to register well-known name \"%s\" (disposition=%d)", org::alljoyn::Bus::WellKnownName, disposition));
    }

    /* Acquire org.alljoyn.Daemon name */
    disposition = DBUS_REQUEST_NAME_REPLY_EXISTS;
    status = router.AddAlias(org::alljoyn::Daemon::WellKnownName,
                             localEndpoint->GetUniqueName(),
                             DBUS_NAME_FLAG_DO_NOT_QUEUE,
                             disposition,
                             NULL,
                             NULL);
    if ((ER_OK != status) || (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != disposition)) {
        status = (ER_OK == status) ? ER_FAIL : status;
        QCC_LogError(status, ("Failed to register well-known name \"%s\" (disposition=%d)", org::alljoyn::Daemon::WellKnownName, disposition));
    }

    /* Add a broadcast Rule rule to receive org.alljoyn.Daemon signals */
    if (status == ER_OK) {
        status = bus.AddMatch("type='signal',interface='org.alljoyn.Daemon'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add match rule for org.alljoyn.Daemon"));
        }
    }

    if (status == ER_OK) {
        /* Must call base class */
        BusObject::ObjectRegistered();

        /* Notify parent */
        busController->ObjectRegistered(this);
    }
}

void AllJoynObj::BindSessionPort(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_BINDSESSIONPORT_REPLY_SUCCESS;
    size_t numArgs;
    const MsgArg* args;
    SessionOpts opts;

    msg->GetArgs(numArgs, args);
    SessionPort sessionPort = args[0].v_uint16;
    QStatus status = GetSessionOpts(args[1], opts);

    /* Get the sender */
    String sender = msg->GetSender();

    if (status == ER_OK) {
        BusEndpoint srcEp = router.FindEndpoint(sender);
        if (srcEp->IsValid()) {
            status = TransportPermission::FilterTransports(srcEp, sender, opts.transports, "BindSessionPort");
            if (status == ER_OK) {
                if (PermissionMgr::GetDaemonBusCallPolicy(srcEp) == PermissionMgr::STDBUSCALL_SHOULD_REJECT) {
                    QCC_DbgPrintf(("The sender endpoint is not allowed to call BindSessionPort()"));
                    status = ER_BUS_NOT_ALLOWED;
                } else if (PermissionMgr::GetDaemonBusCallPolicy(srcEp) == PermissionMgr::STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL) {
                    opts.transports &= TRANSPORT_LOCAL;
                    QCC_DbgPrintf(("The sender endpoint is only allowed to use local transport"));
                }
            }
        } else {
            status = ER_BUS_NO_ENDPOINT;
        }
    }

    if (status != ER_OK) {
        QCC_DbgTrace(("AllJoynObj::BindSessionPort(<bad args>) from %s", sender.c_str()));
        replyCode = ALLJOYN_BINDSESSIONPORT_REPLY_FAILED;
    } else {
        QCC_DbgTrace(("AllJoynObj::BindSession(%s, %d, %s, <%x, %x, %x>)", sender.c_str(), sessionPort,
                      opts.isMultipoint ? "true" : "false", opts.traffic, opts.proximity, opts.transports));

        /* Validate some Session options */
        if ((opts.traffic == SessionOpts::TRAFFIC_RAW_UNRELIABLE) ||
            ((opts.traffic == SessionOpts::TRAFFIC_RAW_RELIABLE) && opts.isMultipoint)) {
            replyCode = ALLJOYN_BINDSESSIONPORT_REPLY_INVALID_OPTS;
        }
    }

    /* At least one existing transport must also be capable of doing what is being asked */
    if (replyCode == ALLJOYN_BINDSESSIONPORT_REPLY_SUCCESS) {
        bool supports = false;
        TransportList& transList = bus.GetInternal().GetTransportList();
        for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
            Transport* trans = transList.GetTransport(i);
            if (trans && trans->SupportsOptions(opts)) {
                supports = true;
                break;
            }
        }
        if (supports == false) {
            replyCode = ALLJOYN_BINDSESSIONPORT_REPLY_INVALID_OPTS;
        }
    }

    if (replyCode == ALLJOYN_BINDSESSIONPORT_REPLY_SUCCESS) {
        /* Assign or check uniqueness of sessionPort */
        AcquireLocks();
        if (sessionPort == SESSION_PORT_ANY) {
            sessionPort = 9999;
            while (++sessionPort) {
                SessionMapType::iterator it = SessionMapLowerBound(sender, 0);
                while ((it != sessionMap.end()) && (it->first.first == sender)) {
                    if (it->second.sessionPort == sessionPort) {
                        break;
                    }
                    ++it;
                }
                /* If no existing sessionMapEntry for sessionPort, then we are done */
                if ((it == sessionMap.end()) || (it->first.first != sender)) {
                    break;
                }
            }
            if (sessionPort == 0) {
                replyCode = ALLJOYN_BINDSESSIONPORT_REPLY_FAILED;
            }
        } else {
            SessionMapType::iterator it = SessionMapLowerBound(sender, 0);
            while ((it != sessionMap.end()) && (it->first.first == sender) && (it->first.second == 0)) {
                if (it->second.sessionPort == sessionPort) {
                    replyCode = ALLJOYN_BINDSESSIONPORT_REPLY_ALREADY_EXISTS;
                    break;
                }
                ++it;
            }
        }

        if (replyCode == ALLJOYN_BINDSESSIONPORT_REPLY_SUCCESS) {
            /* Assign a session id and store the session information */
            SessionMapEntry entry;
            entry.sessionHost = sender;
            entry.sessionPort = sessionPort;
            entry.endpointName = sender;
            entry.fd = qcc::INVALID_SOCKET_FD;
            entry.opts = opts;
            entry.id = 0;
            SessionMapInsert(entry);
        }
        ReleaseLocks();
    }

    /* Reply to request */
    MsgArg replyArgs[2];
    replyArgs[0].Set("u", replyCode);
    replyArgs[1].Set("q", sessionPort);
    status = MethodReply(msg, replyArgs, ArraySize(replyArgs));
    QCC_DbgPrintf(("AllJoynObj::BindSessionPort(%s, %d) returned %d (status=%s)", sender.c_str(), sessionPort, replyCode, QCC_StatusText(status)));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.BindSessionPort"));
    }
}

void AllJoynObj::UnbindSessionPort(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_UNBINDSESSIONPORT_REPLY_FAILED;
    size_t numArgs;
    const MsgArg* args;
    SessionOpts opts;

    msg->GetArgs(numArgs, args);
    SessionPort sessionPort = args[0].v_uint16;

    QCC_DbgTrace(("AllJoynObj::UnbindSession(%d)", sessionPort));

    /* Remove session map entry */
    String sender = msg->GetSender();
    AcquireLocks();
    SessionMapType::iterator it = SessionMapLowerBound(sender, 0);
    while ((it != sessionMap.end()) && (it->first.first == sender) && (it->first.second == 0)) {
        if (it->second.sessionPort == sessionPort) {
            sessionMap.erase(it);
            replyCode = ALLJOYN_UNBINDSESSIONPORT_REPLY_SUCCESS;
            break;
        }
        ++it;
    }
    ReleaseLocks();

    /* Reply to request */
    MsgArg replyArgs[1];
    replyArgs[0].Set("u", replyCode);
    QStatus status = MethodReply(msg, replyArgs, ArraySize(replyArgs));
    QCC_DbgPrintf(("AllJoynObj::UnbindSessionPort(%s, %d) returned %d (status=%s)", sender.c_str(), sessionPort, replyCode, QCC_StatusText(status)));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.UnbindSessionPort"));
    }
}

ThreadReturn STDCALL AllJoynObj::JoinSessionThread::Run(void* arg)
{
    if (isJoin) {
        QCC_DbgTrace(("JoinSessionThread::RunJoin()"));
        return RunJoin();
    } else {
        QCC_DbgTrace(("JoinSessionThread::RunAttach()"));
        return RunAttach();
    }
}

bool AllJoynObj::IsSelfJoinSupported(BusEndpoint& joinerEp) const {

    if (joinerEp->GetEndpointType() == ENDPOINT_TYPE_NULL) {
        return true;
    }

    if (joinerEp->GetEndpointType() == ENDPOINT_TYPE_REMOTE) {
        RemoteEndpoint rep = RemoteEndpoint::cast(joinerEp);
        /* self-join is only supported from protocol version 11 and higher */
        if (rep->GetRemoteProtocolVersion() >= 11) {
            return true;
        }
    }

    return false;
}

ThreadReturn STDCALL AllJoynObj::JoinSessionThread::RunJoin()
{
    QCC_DbgTrace(("JoinSessionThread::RunJoin()"));

    uint32_t replyCode = ALLJOYN_JOINSESSION_REPLY_SUCCESS;
    SessionId id = 0;
    SessionOpts optsOut(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, 0);
    size_t numArgs;
    const MsgArg* args;
    SessionMapEntry sme;
    String sender = msg->GetSender();
    RemoteEndpoint b2bEp;
    BusEndpoint joinerEp = ajObj.router.FindEndpoint(sender);

    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): joinerEp=\"%s\"", joinerEp->GetUniqueName().c_str()));

    /* Parse the message args */
    msg->GetArgs(numArgs, args);
    const char* sessionHost = NULL;
    SessionPort sessionPort;
    SessionOpts optsIn;
    QStatus status = MsgArg::Get(args, 2, "sq", &sessionHost, &sessionPort);
    BusEndpoint rSessionEp;
    bool isSelfJoin = false;

    if (status == ER_OK) {
        status = GetSessionOpts(args[2], optsIn);
        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): optsIn=\"%s\"", optsIn.ToString().c_str()));
    }

    if (status == ER_OK) {
        BusEndpoint srcEp = ajObj.router.FindEndpoint(sender);
        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): srcEp=\"%s\"", srcEp->GetUniqueName().c_str()));
        if (srcEp->IsValid()) {
            status = TransportPermission::FilterTransports(srcEp, sender, optsIn.transports, "JoinSessionThread.Run");
        }
    }

    if (status == ER_OK) {
        PermissionMgr::DaemonBusCallPolicy policy = PermissionMgr::GetDaemonBusCallPolicy(joinerEp);
        bool rejectCall = false;
        if (policy == PermissionMgr::STDBUSCALL_SHOULD_REJECT) {
            rejectCall = true;
        } else if (policy == PermissionMgr::STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL) {
            optsIn.transports &= TRANSPORT_LOCAL;
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): The sender endpoint is only allowed to use local transport."));
        }

        if (rejectCall) {
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): The sender endpoint is not allowed to call JoinSession()"));
            replyCode = ALLJOYN_JOINSESSION_REPLY_REJECTED;
            /* Reply to request */
            MsgArg replyArgs[3];
            replyArgs[0].Set("u", replyCode);
            replyArgs[1].Set("u", id);
            SetSessionOpts(optsOut, replyArgs[2]);
            status = ajObj.MethodReply(msg, replyArgs, ArraySize(replyArgs));
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): JoinSession(%d) returned (%d,%u) (status=%s)",
                           sessionPort, replyCode, id, QCC_StatusText(status)));
            return 0;
        }
    }

    ajObj.AcquireLocks();

    SessionMapType::iterator it = ajObj.SessionMapLowerBound(sender, 0);
    BusEndpoint hostEp = ajObj.router.FindEndpoint(sessionHost);
    if (hostEp->IsValid()) {
        while ((it != ajObj.sessionMap.end()) && (it->first.first == sender) && (it->first.second == 0)) {
            if (ajObj.router.FindEndpoint(it->second.sessionHost) == hostEp) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): self-join!"));
                isSelfJoin = true;
                break;
            }
            ++it;
        }
    }

    if (ajObj.IsSelfJoinSupported(joinerEp) == false && isSelfJoin == true) {
        /* Do not let a session creator join itself */
        replyCode = ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED;
    }

    if (status != ER_OK) {
        if (replyCode != ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): bad args"));
        }
    } else if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
        QCC_DbgPrintf(("JoinSessionThread::RunJoin() sessionPort=%d, opts=<%u, 0x%x, 0x%x>)",
                       sessionPort, optsIn.traffic, optsIn.proximity, optsIn.transports));

        /* Decide how to proceed based on the session endpoint existence/type */
        VirtualEndpoint vSessionEp;
        BusEndpoint ep;

        if (sessionHost) {
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): sessionHost=\"%s\"", sessionHost));
            ep = ajObj.router.FindEndpoint(sessionHost);
            if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                vSessionEp = VirtualEndpoint::cast(ep);
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): vSessionEp=\"%s\"", sessionHost));
            } else if ((ep->GetEndpointType() == ENDPOINT_TYPE_REMOTE) || (ep->GetEndpointType() == ENDPOINT_TYPE_NULL) || (ep->GetEndpointType() == ENDPOINT_TYPE_LOCAL)) {
                rSessionEp = ep;
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): rSessionEp=\"%s\"", rSessionEp->GetUniqueName().c_str()));
            }
        }

        if (rSessionEp->IsValid()) {
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): session is with another locally connected attachment"));

            /* Find creator in session map */
            String creatorName = rSessionEp->GetUniqueName();
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): creatorName=\"%s\"", creatorName.c_str()));
            bool foundSessionMapEntry = false;
            SessionMapType::iterator sit = ajObj.SessionMapLowerBound(creatorName, 0);
            while ((sit != ajObj.sessionMap.end()) && (creatorName == sit->first.first)) {
                if ((sit->second.sessionHost == creatorName) && (sit->second.sessionPort == sessionPort)) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): found \"%s\" in sessionMap with expected port %d.",
                                   creatorName.c_str(), sessionPort));
                    if (sit->first.second == 0) {
                        sme = sit->second;
                        foundSessionMapEntry = true;
                        if (!sme.opts.isMultipoint) {
                            break;
                        }
                    } else {
                        /* Check if this joiner has already joined and reject in that case */
                        vector<String>::iterator mit = sit->second.memberNames.begin();
                        while (mit != sit->second.memberNames.end()) {
                            if (*mit == sender) {
                                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): joiner already joined"));
                                foundSessionMapEntry = false;
                                replyCode = ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED;
                                break;
                            }
                            ++mit;
                        }
                        sme = sit->second;
                    }
                }
                ++sit;
            }

            if (joinerEp->IsValid() && foundSessionMapEntry) {
                bool isAccepted = false;
                SessionId newSessionId = sme.id;
                if (!sme.opts.IsCompatible(optsIn)) {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS;
                } else {
                    /* Create a new sessionId if needed */
                    while (newSessionId == 0) {
                        newSessionId = qcc::Rand32();
                    }

                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): newsessinoId=%d.", newSessionId));

                    /* Add an entry to sessionMap here (before sending accept session) since accept session
                     * may trigger a call to GetSessionFd or LeaveSession which must be aware of the new session's
                     * existence in order to complete successfully.
                     */
                    bool hasSessionMapPlaceholder = false;
                    sme.id = newSessionId;

                    if (!ajObj.SessionMapFind(sme.endpointName, sme.id)) {
                        /* Set isInitializing to true, to ensure that this entry is not deleted
                         * while the join session is in progress
                         */
                        sme.isInitializing = true;
                        ajObj.SessionMapInsert(sme);
                        hasSessionMapPlaceholder = true;
                    }

                    /* Ask creator to accept session */
                    ajObj.ReleaseLocks();
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): SendAcceptSession()"));
                    status = ajObj.SendAcceptSession(sme.sessionPort, newSessionId, sessionHost, sender.c_str(), optsIn, isAccepted);
                    if (status != ER_OK) {
                        QCC_LogError(status, ("SendAcceptSession failed"));
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    }
                    ajObj.AcquireLocks();

                    /* Check the session didn't go away during the join attempt */
                    if (!joinerEp->IsValid()) {
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                        QCC_LogError(ER_FAIL, ("Joiner %s disappeared while joining", sender.c_str()));
                    }

                    /* Cleanup failed raw session entry in sessionMap */
                    if (hasSessionMapPlaceholder && ((status != ER_OK) || !isAccepted)) {
                        ajObj.SessionMapErase(sme);
                    }
                }
                if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                    if (!isAccepted) {
                        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Join session request rejected"));
                        replyCode = ALLJOYN_JOINSESSION_REPLY_REJECTED;
                    } else if (sme.opts.traffic == SessionOpts::TRAFFIC_MESSAGES) {
                        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Join session request accepted"));
                        /* setup the forward and reverse routes through the local daemon */
                        RemoteEndpoint tEp;
                        status = ajObj.router.AddSessionRoute(newSessionId, joinerEp, NULL, rSessionEp, tEp);
                        if (status != ER_OK) {
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                            QCC_LogError(status, ("AddSessionRoute(%u, %s, NULL, %s, tEp) failed", newSessionId, sender.c_str(), rSessionEp->GetUniqueName().c_str()));
                        }
                        if (status == ER_OK) {
                            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Add local joiner to member list"));
                            /* Add (local) joiner to list of session members since no AttachSession will be sent */
                            SessionMapEntry* smEntry = ajObj.SessionMapFind(sme.endpointName, newSessionId);
                            if (smEntry) {
                                smEntry->memberNames.push_back(sender);
                                smEntry->isInitializing = false;
                                sme = *smEntry;
                            } else {
                                replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                                status = ER_FAIL;
                                QCC_LogError(status, ("Failed to find sessionMap entry"));
                            }
                            /* Create a joiner side entry in sessionMap */
                            if (!isSelfJoin) {
                                SessionMapEntry joinerSme = sme;
                                joinerSme.endpointName = sender;
                                joinerSme.id = newSessionId;
                                ajObj.SessionMapInsert(joinerSme);
                                id = joinerSme.id;
                            } else {
                                id = newSessionId;
                            }

                            optsOut = sme.opts;
                            optsOut.transports &= optsIn.transports;
                            sme.id = newSessionId;
                        }
                    } else if ((sme.opts.traffic != SessionOpts::TRAFFIC_MESSAGES) && !sme.opts.isMultipoint) {
                        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Raw socket"));
                        /* Create a raw socket pair for the two local session participants */
                        SocketFd fds[2];
                        status = SocketPair(fds);
                        if (status == ER_OK) {
                            /* Update the creator-side entry in sessionMap */
                            SessionMapEntry* smEntry = ajObj.SessionMapFind(sme.endpointName, sme.id);
                            if (smEntry) {
                                smEntry->fd = fds[0];
                                smEntry->memberNames.push_back(sender);

                                /* Create a joiner side entry in sessionMap */
                                if (!isSelfJoin) {
                                    SessionMapEntry sme2 = sme;
                                    sme2.memberNames.push_back(sender);
                                    sme2.endpointName = sender;
                                    sme2.fd = fds[1];
                                    ajObj.SessionMapInsert(sme2);
                                    id = sme2.id;
                                } else {
                                    id = sme.id;
                                }
                                optsOut = sme.opts;
                                optsOut.transports &= optsIn.transports;
                            } else {
                                qcc::Close(fds[0]);
                                qcc::Close(fds[1]);
                                status = ER_FAIL;
                                QCC_LogError(status, ("Failed to find sessionMap entry"));
                                replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                            }
                        } else {
                            QCC_LogError(status, ("SocketPair failed"));
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                        }
                    } else {
                        /* QosInfo::TRAFFIC_RAW_UNRELIABLE is not currently supported */
                        replyCode = ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS;
                    }
                }
            } else {
                if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_NO_SESSION;
                }
            }
        } else {
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): session is with a remote attachment"));
            /* Session is with a connected or unconnected remote device */
            MsgArg membersArg;

            /* Check for existing multipoint session */
            if (vSessionEp->IsValid()) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Existing virtual endpoint IsValid() and isMultipoint"));
                SessionMapType::iterator it = ajObj.sessionMap.begin();
                while (it != ajObj.sessionMap.end()) {
                    if ((it->second.sessionHost == vSessionEp->GetUniqueName()) && (it->second.sessionPort == sessionPort)) {
                        if (it->second.opts.IsCompatible(optsIn)) {
                            if (it->second.opts.isMultipoint) {
                                b2bEp = vSessionEp->GetBusToBusEndpoint(it->second.id);
                                if (b2bEp->IsValid()) {
                                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): IncrementRef() on existing mp session"));
                                    b2bEp->IncrementRef();
                                    replyCode = ALLJOYN_JOINSESSION_REPLY_SUCCESS;
                                }
                            }
                        } else {
                            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Blocked multiple connections to same dest with same session ID"));
                            /* Cannot support more than one connection to the same destination with the same sessionId */
                            replyCode = ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS;
                        }
                        break;
                    }
                    ++it;
                }
            }

            String busAddr;
            if (!b2bEp->IsValid()) {
                /* Step 1a: If there is a busAddr from advertisement use it to (possibly) create a physical connection */
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Look for busaddr corresponding to sessionHost"));
                set<JoinSessionEntry> advertisements;
                vector<String> busAddrs;
                multimap<String, NameMapEntry>::iterator nmit = ajObj.nameMap.lower_bound(sessionHost);
                while (nmit != ajObj.nameMap.end() && (nmit->first == sessionHost)) {
                    if (nmit->second.transport & optsIn.transports) {
                        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Found busaddr in name map: \"%s\"", nmit->second.busAddr.c_str()));
                        JoinSessionEntry joinSessionEntry(nmit->first, nmit->second.transport, nmit->second.busAddr);
                        advertisements.insert(joinSessionEntry);
                    }
                    ++nmit;
                }

                set<JoinSessionEntry>::iterator sit = advertisements.begin();
                while (sit != advertisements.end()) {
                    busAddrs.push_back((*sit).busAddr);
                    sit++;
                }

                /* Step 1b: If no busAddr, see if one exists in the adv alias map */
                if (busAddrs.empty() && (sessionHost[0] == ':')) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): look for busaddr in adv alias map"));
                    String rguidStr = String(sessionHost).substr(1, GUID128::SHORT_SIZE);
                    map<String, set<AdvAliasEntry> >::iterator ait = ajObj.advAliasMap.find(rguidStr);
                    if (ait != ajObj.advAliasMap.end()) {
                        set<AdvAliasEntry>::iterator bit = ait->second.begin();
                        while ((bit != ait->second.end())) {
                            if (((*bit).transport & optsIn.transports) != 0) {
                                multimap<String, NameMapEntry>::iterator nmit2 = ajObj.nameMap.lower_bound((*bit).name);
                                while (nmit2 != ajObj.nameMap.end() && (nmit2->first == (*bit).name)) {
                                    if ((nmit2->second.transport & (*bit).transport & optsIn.transports) != 0) {
                                        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Found busaddr in adv alias map: \"%s\"",
                                                       nmit2->second.busAddr.c_str()));
                                        busAddrs.push_back(nmit2->second.busAddr);
                                    }
                                    ++nmit2;
                                }
                            }
                            ++bit;
                        }
                    }
                }
                ajObj.ReleaseLocks();
                /*
                 * Step 1c: If still no advertisement (busAddr) and we are connected to the sesionHost, then ask it directly
                 * for the busAddr
                 */
                if (vSessionEp->IsValid() && busAddrs.empty()) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): no busaddr.  SendGetSessionInfo() directly."));
                    status = ajObj.SendGetSessionInfo(sessionHost, sessionPort, optsIn, busAddrs);
                    if (status != ER_OK) {
                        busAddrs.clear();
                        QCC_LogError(status, ("GetSessionInfo failed"));
                    }
                }

                if (!busAddrs.empty()) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Have busaddrs to try."));
                    /* Try busAddrs in priority order until connect succeeds */
                    for (size_t i = 0; i < busAddrs.size(); ++i) {
                        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Trying busaddr=\"%s\"", busAddrs[i].c_str()));
                        /* Ask the transport that provided the advertisement for an endpoint */
                        TransportList& transList = ajObj.bus.GetInternal().GetTransportList();
                        Transport* trans = transList.GetTransport(busAddrs[i]);
                        if (trans != NULL) {
                            if ((optsIn.transports & trans->GetTransportMask()) == 0) {
                                QCC_DbgPrintf(("AllJoynObj:JoinSessionThread(): skip unpermitted transport(%s)", trans->GetTransportName()));
                                continue;
                            }

                            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Connect(\"%s\")", busAddrs[i].c_str()));

                            BusEndpoint newEp;
                            status = trans->Connect(busAddrs[i].c_str(), optsIn, newEp);
                            if (status == ER_OK) {
                                b2bEp = RemoteEndpoint::cast(newEp);
                                if (b2bEp->IsValid()) {
                                    b2bEp->IncrementRef();
                                }
                                busAddr = busAddrs[i];
                                replyCode = ALLJOYN_JOINSESSION_REPLY_SUCCESS;
                                optsIn.transports  = trans->GetTransportMask();
                                break;
                            } else {
                                QCC_LogError(status, ("trans->Connect(%s) failed", busAddrs[i].c_str()));
                                replyCode = ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
                            }
                        }
                    }
                } else {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): No advertisement. No existing route.  Nothing we can do."));
                    /* No advertisment or existing route to session creator */
                    replyCode = ALLJOYN_JOINSESSION_REPLY_NO_SESSION;
                }

                if (busAddr.empty()) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Tried to Connect() but failed.  Nothing we can do."));
                    replyCode = ALLJOYN_JOINSESSION_REPLY_UNREACHABLE;
                }
                ajObj.AcquireLocks();
            }

            /* Step 2: Wait for the new b2b endpoint to have a virtual ep for nextController */
            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Wait for virtual endpoint."));
            uint64_t startTime = GetTimestamp64();
            while (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                /* Do we route through b2bEp? If so, we're done */
                if (!b2bEp->IsValid()) {
                    QCC_LogError(ER_FAIL, ("B2B endpoint %s disappeared during JoinSession", b2bEp->GetUniqueName().c_str()));
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    break;
                }

                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Remote name of new b2b endpoint is \"%s\"", b2bEp->GetRemoteName().c_str()));
                VirtualEndpoint vep;
                if (ajObj.router.FindEndpoint(b2bEp->GetRemoteName(), vep) && vep->CanUseRoute(b2bEp)) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Found virtual endpoint for route"));
                    /* Got a virtual endpoint we can route through */
                    break;
                }
                /* Otherwise wait */
                uint64_t now = GetTimestamp64();
                if (now > (startTime + 30000LL)) {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    QCC_DbgPrintf(("JoinSession timed out waiting for %s to appear on %s", sessionHost, b2bEp->GetUniqueName().c_str()));
                    break;
                }
                /* Give up the locks while waiting */
                ajObj.ReleaseLocks();
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Sleep"));
                qcc::Sleep(10);
                ajObj.AcquireLocks();
            }

            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Done waiting for virtual endpoint."));

            /* Step 3: Send a session attach */
            if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                const String nextControllerName = b2bEp->GetRemoteName();

                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): SendAttachSession()"));
                ajObj.ReleaseLocks();
                status = ajObj.SendAttachSession(sessionPort, sender.c_str(), sessionHost, sessionHost, b2bEp,
                                                 nextControllerName.c_str(), 0, busAddr.c_str(), optsIn, replyCode,
                                                 id, optsOut, membersArg);
                if (status != ER_OK) {
                    QCC_LogError(status, ("AttachSession to %s failed", nextControllerName.c_str()));
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                }
                /* Re-acquire locks */
                ajObj.AcquireLocks();
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): FindEndpoint(\"%s\")", sessionHost));
                ajObj.router.FindEndpoint(sessionHost, vSessionEp);
                if (!vSessionEp->IsValid()) {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    QCC_LogError(ER_BUS_NO_ENDPOINT, ("SessionHost endpoint (%s) not found", sessionHost));
                }
            }

            /* If session was successful, Add two-way session routes to the table */
            if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Attach session success(\"%s\")", sessionHost));
                if (joinerEp->IsValid()) {
                    BusEndpoint busEndpoint = BusEndpoint::cast(vSessionEp);
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): AddSessionRoute() for session ID %d.", id));
                    status = ajObj.router.AddSessionRoute(id, joinerEp, NULL, busEndpoint, b2bEp, b2bEp->IsValid() ? NULL : &optsOut);
                    if (status != ER_OK) {
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                        QCC_LogError(status, ("AddSessionRoute(%u, %s, NULL, %s, %s, %s) failed", id, sender.c_str(), vSessionEp->GetUniqueName().c_str(), b2bEp->GetUniqueName().c_str(), b2bEp->IsValid() ? "NULL" : "opts"));
                    }
                } else {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    QCC_LogError(ER_BUS_NO_ENDPOINT, ("Cannot find joiner endpoint %s", sender.c_str()));
                }
            }

            /* Create session map entry */
            bool sessionMapEntryCreated = false;
            if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Add session map entry for sender=\"%s\", id=%d., sessionHost=\"%s\", sessionPort=%d.",
                               sender.c_str(), id, vSessionEp->GetUniqueName().c_str(), sessionPort));
                const MsgArg* sessionMembers;
                size_t numSessionMembers = 0;
                membersArg.Get("as", &numSessionMembers, &sessionMembers);
                sme.endpointName = sender;
                sme.id = id;
                sme.sessionHost = vSessionEp->GetUniqueName();
                sme.sessionPort = sessionPort;
                sme.opts = optsOut;
                for (size_t i = 0; i < numSessionMembers; ++i) {
                    sme.memberNames.push_back(sessionMembers[i].v_string.str);
                }
                ajObj.SessionMapInsert(sme);
                sessionMapEntryCreated = true;
            }

            /* If a raw sesssion was requested, then teardown the new b2bEp to use it for a raw stream */
            if ((replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) && (optsOut.traffic != SessionOpts::TRAFFIC_MESSAGES)) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Raw session.  Tear down new endpoint"));
                SessionMapEntry* smEntry = ajObj.SessionMapFind(sender, id);
                if (smEntry) {
                    ajObj.ReleaseLocks();
                    status = ajObj.ShutdownEndpoint(b2bEp, smEntry->fd);
                    ajObj.AcquireLocks();
                    smEntry = ajObj.SessionMapFind(sender, id);
                    if (smEntry) {
                        smEntry->isRawReady = true;
                    } else {
                        status = ER_FAIL;
                        QCC_LogError(status, ("Failed to find SessionMapEntry"));
                    }

                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to shutdown remote endpoint for raw usage"));
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    }
                } else {
                    QCC_LogError(ER_FAIL, ("Failed to find session id=%u for %s, %d", id, sender.c_str(), id));
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                }
            }
            /* If session was unsuccessful, cleanup sessionMap */
            if (sessionMapEntryCreated && (replyCode != ALLJOYN_JOINSESSION_REPLY_SUCCESS)) {
                ajObj.SessionMapErase(sme);
            }

            /* Cleanup b2bEp if its ref hasn't been incremented */
            if (b2bEp->IsValid()) {
                b2bEp->DecrementRef();
            }
        }
    }

    /* Send AttachSession to all other members of the multicast session */
    if ((replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) && sme.opts.isMultipoint &&
        sme.sessionHost != sender /* test if we now just selfjoined */) {
        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Multicast session joined."));
        for (size_t i = 0; i < sme.memberNames.size(); ++i) {
            const String& member = sme.memberNames[i];
            /* Skip this joiner since it is attached already */
            if (member == sender || member == sme.sessionHost) {
                continue;
            }

            QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Member \"%s\"", sme.memberNames[i].c_str()));

            BusEndpoint memberEp = ajObj.router.FindEndpoint(member);
            RemoteEndpoint memberB2BEp;
            if (memberEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Member \"%s\" is virtual", sme.memberNames[i].c_str()));
                /* Endpoint is not served directly by this daemon so forward the attach using existing b2bEp connection with session creator */
                if (!b2bEp->IsValid()) {
                    VirtualEndpoint vMemberEp = VirtualEndpoint::cast(memberEp);
                    /* Local session creator */
                    memberB2BEp = vMemberEp->GetBusToBusEndpoint(id);
                } else {
                    /* Remote session creator */
                    memberB2BEp = b2bEp;
                }
                if (memberB2BEp->IsValid()) {
                    MsgArg tMembersArg;
                    SessionId tId;
                    SessionOpts tOpts;
                    const String nextControllerName = memberB2BEp->GetRemoteName();
                    uint32_t tReplyCode;
                    ajObj.ReleaseLocks();
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): SendAttachSession()"));
                    status = ajObj.SendAttachSession(sessionPort,
                                                     sender.c_str(),
                                                     sessionHost,
                                                     member.c_str(),
                                                     memberB2BEp,
                                                     nextControllerName.c_str(),
                                                     id,
                                                     "",
                                                     sme.opts,
                                                     tReplyCode,
                                                     tId,
                                                     tOpts,
                                                     tMembersArg);
                    ajObj.AcquireLocks();
                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to attach session %u to %s", id, member.c_str()));
                    } else if (tReplyCode != ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("Failed to attach session %u to %s (reply=%d)", id, member.c_str(), tReplyCode));
                    } else if (id != tId) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("Session id mismatch (expected=%u, actual=%u)", id, tId));
                    } else if (!joinerEp->IsValid() || !memberB2BEp->IsValid() || !memberB2BEp->IsValid()) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("joiner, memberEp or memberB2BEp disappeared during join"));
                    }
                } else {
                    status = ER_BUS_BAD_SESSION_OPTS;
                    QCC_LogError(status, ("Unable to add existing member %s to session %u", memberEp->GetUniqueName().c_str(), id));
                }
            } else if (memberEp->IsValid()) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Local (non-virtual) endpoint"));
                /* Add joiner to any local member's sessionMap entry  since no AttachSession is sent */
                SessionMapEntry* smEntry = ajObj.SessionMapFind(member, id);
                if (smEntry) {
                    smEntry->memberNames.push_back(sender);
                }
                /* Multipoint session member is local to this daemon. Send MPSessionChanged */
                if (optsOut.isMultipoint) {
                    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Local (non-virtual) MPSessionChanged"));
                    ajObj.ReleaseLocks();
                    ajObj.SendMPSessionChanged(id, sender.c_str(), true, member.c_str(), ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_ADDED);
                    ajObj.AcquireLocks();
                }
            }
            /* Add session routing */
            if (memberEp->IsValid() && joinerEp->IsValid() && (status == ER_OK)) {
                QCC_DbgPrintf(("JoinSessionThread::RunJoin(): AddSessionRoute()"));
                status = ajObj.router.AddSessionRoute(id, joinerEp, NULL, memberEp, memberB2BEp);
                if (status != ER_OK) {
                    QCC_LogError(status, ("AddSessionRoute(%u, %s, NULL, %s, %s) failed", id, sender.c_str(), memberEp->GetUniqueName().c_str(), memberB2BEp->GetUniqueName().c_str()));
                }
            }
        }
    }
    ajObj.ReleaseLocks();

    QCC_DbgPrintf(("JoinSessionThread::RunJoin(): Reply to request"));

    /* Reply to request */
    MsgArg replyArgs[3];
    replyArgs[0].Set("u", replyCode);
    replyArgs[1].Set("u", id);
    SetSessionOpts(optsOut, replyArgs[2]);
    status = ajObj.MethodReply(msg, replyArgs, ArraySize(replyArgs));
    QCC_DbgPrintf(("AllJoynObj::JoinSession(%d) returned (%d,%u) (status=%s)", sessionPort, replyCode, id, QCC_StatusText(status)));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.JoinSession"));
    }

    /* Send SessionJoined to creator if creator is local since RunAttach does not run in this case */
    if ((status == ER_OK) && (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) && rSessionEp->IsValid()) {
        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): SendSessionJoined() to local endpoint"));
        ajObj.SendSessionJoined(sme.sessionPort, sme.id, sender.c_str(), sme.endpointName.c_str());
        /* If session is multipoint, send MPSessionChanged to sessionHost */
        if (sme.opts.isMultipoint) {
            ajObj.SendMPSessionChanged(sme.id, sender.c_str(), true, sme.endpointName.c_str(), ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_ADDED);
        }
    }

    /* Send a series of MPSessionChanged to "catch up" the new joiner */
    if ((replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) && optsOut.isMultipoint) {
        QCC_DbgPrintf(("JoinSessionThread::RunJoin(): SendMPSessionChanged() series to local endpoint"));
        ajObj.AcquireLocks();
        SessionMapEntry* smEntry = ajObj.SessionMapFind(sender, id);
        if (smEntry) {
            String sessionHost = smEntry->sessionHost;

            vector<String> memberVector = smEntry->memberNames;
            ajObj.ReleaseLocks();
            /* Already sent MPSessionChanged to session creator, so skip it here if sessionHost (aka session creator) is equal to the sender. */
            if (!isSelfJoin) {
                ajObj.SendMPSessionChanged(id, sessionHost.c_str(), true, sender.c_str(), ALLJOYN_MPSESSIONCHANGED_LOCAL_MEMBER_ADDED);
            }
            vector<String>::const_iterator mit = memberVector.begin();
            while (mit != memberVector.end()) {
                if (sender != *mit && sessionHost != *mit) {
                    ajObj.SendMPSessionChanged(id, mit->c_str(), true, sender.c_str(), ALLJOYN_MPSESSIONCHANGED_LOCAL_MEMBER_ADDED);
                }
                mit++;
            }
        } else {
            ajObj.ReleaseLocks();
        }
    }

    return 0;
}

void AllJoynObj::JoinSessionThread::ThreadExit(Thread* thread)
{
    ajObj.joinSessionThreadsLock.Lock(MUTEX_CONTEXT);
    vector<JoinSessionThread*>::iterator it = ajObj.joinSessionThreads.begin();
    JoinSessionThread* deleteMe = NULL;
    while (it != ajObj.joinSessionThreads.end()) {
        if (*it == thread) {
            deleteMe = *it;
            ajObj.joinSessionThreads.erase(it);
            break;
        }
        ++it;
    }
    ajObj.joinSessionThreadsLock.Unlock(MUTEX_CONTEXT);
    if (deleteMe) {
        deleteMe->Join();
        delete deleteMe;
    } else {
        QCC_LogError(ER_FAIL, ("Internal error: JoinSessionThread not found on list"));
    }
}

void AllJoynObj::JoinSession(const InterfaceDescription::Member* member, Message& msg)
{
    /* Handle JoinSession on another thread since JoinThread can block waiting for NameOwnerChanged */
    joinSessionThreadsLock.Lock(MUTEX_CONTEXT);
    if (!isStopping) {
        JoinSessionThread* jst = new JoinSessionThread(*this, msg, true);
        QStatus status = jst->Start(NULL, jst);
        if (status == ER_OK) {
            joinSessionThreads.push_back(jst);
        } else {
            QCC_LogError(status, ("Join: Failed to start JoinSessionThread"));
            delete jst;
        }
    }
    joinSessionThreadsLock.Unlock(MUTEX_CONTEXT);
}

void AllJoynObj::AttachSession(const InterfaceDescription::Member* member, Message& msg)
{
    /* Handle AttachSession on another thread since AttachSession can block when connecting through an intermediate node */
    joinSessionThreadsLock.Lock(MUTEX_CONTEXT);
    if (!isStopping) {
        JoinSessionThread* jst = new JoinSessionThread(*this, msg, false);
        QStatus status = jst->Start(NULL, jst);
        if (status == ER_OK) {
            joinSessionThreads.push_back(jst);
        } else {
            QCC_LogError(status, ("Attach: Failed to start JoinSessionThread"));
            delete jst;
        }
    }
    joinSessionThreadsLock.Unlock(MUTEX_CONTEXT);
}


void AllJoynObj::LeaveHostedSession(const InterfaceDescription::Member* member, Message& msg)
{
    LeaveSessionCommon(member, msg, LEAVE_HOSTED_SESSION);

}

void AllJoynObj::LeaveJoinedSession(const InterfaceDescription::Member* member, Message& msg)
{
    LeaveSessionCommon(member, msg, LEAVE_JOINED_SESSION);

}


void AllJoynObj::LeaveSession(const InterfaceDescription::Member* member, Message& msg)
{
    LeaveSessionCommon(member, msg, LEAVE_SESSION);

}

uint32_t AllJoynObj::CheckLeaveSession(const SessionMapEntry*smEntry, const char*sender, SessionId id, LeaveSessionType lst, bool& senderWasSelfJoined) const {

    if (!smEntry || (id == 0)) {
        return ALLJOYN_LEAVESESSION_REPLY_NO_SESSION;
    }

    String senderstr(sender);
    senderWasSelfJoined = false;

    switch (lst) {
    case LEAVE_HOSTED_SESSION:
        if (smEntry->sessionHost != sender) {
            QCC_DbgPrintf(("Sender was not the host\n"));
            return ALLJOYN_LEAVESESSION_REPLY_NO_SESSION;
        }
        senderWasSelfJoined = smEntry->IsSelfJoin();
        break;

    case LEAVE_JOINED_SESSION:
        if (find(smEntry->memberNames.begin(), smEntry->memberNames.end(), senderstr) == smEntry->memberNames.end()) {
            QCC_DbgPrintf(("Sender was not a joiner\n"));
            return ALLJOYN_LEAVESESSION_REPLY_NO_SESSION;
        }
        if (smEntry->sessionHost == sender) {
            senderWasSelfJoined = true;
            assert(smEntry->IsSelfJoin());
        }
        break;

    case LEAVE_SESSION:
        {
            if (smEntry->IsSelfJoin() == true && (smEntry->sessionHost == sender)) {
                QCC_DbgPrintf(("Cannot use LeaveSession() on self-join session: ambiguous\n"));
                return ALLJOYN_LEAVESESSION_REPLY_NO_SESSION;
            }
        }
        break;
    }

    return ALLJOYN_LEAVESESSION_REPLY_SUCCESS;
}

void AllJoynObj::LeaveSessionCommon(const InterfaceDescription::Member* member, Message& msg, LeaveSessionType lst)
{
    uint32_t replyCode = ALLJOYN_LEAVESESSION_REPLY_SUCCESS;

    size_t numArgs;
    const MsgArg* args;
    bool senderWasSelfJoined;

    /* Parse the message args */
    msg->GetArgs(numArgs, args);
    assert(numArgs == 1);
    SessionId id = static_cast<SessionId>(args[0].v_uint32);

    QCC_DbgTrace(("AllJoynObj::LeaveSession(%u)", id));

    /* Find the session with that id */
    AcquireLocks();
    SessionMapEntry* smEntry = SessionMapFind(msg->GetSender(), id);
    if ((replyCode = CheckLeaveSession(smEntry, msg->GetSender(), id, lst, senderWasSelfJoined)) != ALLJOYN_LEAVESESSION_REPLY_SUCCESS) {
        ReleaseLocks();
    } else {

        /* Send DetachSession signal to daemons of all session participants */
        if (senderWasSelfJoined == false) {
            MsgArg detachSessionArgs[2];
            detachSessionArgs[0].Set("u", id);
            detachSessionArgs[1].Set("s", msg->GetSender());

            QStatus status = Signal(NULL, 0, *detachSessionSignal, detachSessionArgs, ArraySize(detachSessionArgs), 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
            if (status != ER_OK) {
                QCC_LogError(status, ("Error sending org.alljoyn.Daemon.DetachSession signal"));
            }

            /* Close any open fd for this session */
            if (smEntry->fd != qcc::INVALID_SOCKET_FD) {
                qcc::Shutdown(smEntry->fd);
                qcc::Close(smEntry->fd);
            }
        }


        /* Locks must be released before calling RemoveSessionRefs since that method calls out to user (SessionLost) */
        ReleaseLocks();

        /* Remove entries from sessionMap, but dont send a SessionLost back to the caller of this method */
        if (RemoveSessionRefs(msg->GetSender(), id, false, lst) == false) {

            /* Remove session routes */
            router.RemoveSessionRoutes(msg->GetSender(), id);
        } else {
            router.RemoveSelfJoinSessionRoute(msg->GetSender(), id);
        }
    }

    /* Reply to request */
    MsgArg replyArgs[1];
    replyArgs[0].Set("u", replyCode);
    QStatus status = MethodReply(msg, replyArgs, ArraySize(replyArgs));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.LeaveSession"));
    }
}

void AllJoynObj::RemoveSessionMember(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_SUCCESS;

    size_t numArgs;
    const MsgArg* args;
    /* Parse the message args */
    msg->GetArgs(numArgs, args);
    assert(numArgs == 2);

    SessionId id;
    const char* sessionMemberName;
    bool sessionMemberSelfJoined = false;

    QStatus status = MsgArg::Get(args, numArgs, "us", &id, &sessionMemberName);
    if (status != ER_OK) {
        replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_FAILED;
    }

    QCC_DbgPrintf(("AllJoynObj::RemoveSessionMember(%u, %s)", id, sessionMemberName));

    AcquireLocks();
    if (replyCode == ALLJOYN_REMOVESESSIONMEMBER_REPLY_SUCCESS) {
        /* Find the session with the sender and specified session id */
        SessionMapEntry* smEntry = SessionMapFind(msg->GetSender(), id);
        if (!smEntry || (id == 0)) {
            replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_NO_SESSION;
        } else if (!smEntry->opts.isMultipoint) {
            replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_NOT_MULTIPOINT;
        } else if (smEntry->sessionHost != msg->GetSender()) {
            replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_NOT_BINDER;
        } else {
            /* Search for this member in the member names. */
            vector<String>::iterator mit = smEntry->memberNames.begin();
            bool found = false;
            String srcStr = sessionMemberName;
            while (mit != smEntry->memberNames.end()) {
                if (*mit == srcStr) {
                    found = true;
                    break;
                }
                mit++;
            }

            if (!found) {
                replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_NOT_FOUND;
            } else {
                if (smEntry->sessionHost == srcStr) {
                    sessionMemberSelfJoined = true;
                }
                /* Find the virtual endpoint associated with the remote daemon
                 * for the session member we want to remove.
                 * If a virtual endpoint was not found, the destination is local
                 * to this daemon.
                 */
                VirtualEndpoint vep;
                router.FindEndpoint(sessionMemberName, vep);
                if (vep->IsValid()) {
                    RemoteEndpoint rep = vep->GetBusToBusEndpoint(id);
                    /* Check the Remote daemon version */
                    if (rep->GetRemoteProtocolVersion() < 7) {
                        /* Lower versions of the daemon do not support the RemoveSessionMember
                         * feature. So, if the remote daemon is older, then do not allow this
                         * method call.
                         */
                        replyCode = ALLJOYN_REMOVESESSIONMEMBER_REPLY_INCOMPATIBLE_REMOTE_DAEMON;
                    }
                }
            }
        }
    }
    if (replyCode == ALLJOYN_REMOVESESSIONMEMBER_REPLY_SUCCESS) {

        /* Send DetachSession signal to daemons of all session participants.
         * Send a detachSessionSignal to be sent with the
         * member name we want to remove and the session ID to remove from. */
        if (sessionMemberSelfJoined == false) {
            MsgArg detachSessionArgs[2];
            detachSessionArgs[0].Set("u", id);
            detachSessionArgs[1].Set("s", sessionMemberName);

            QStatus status = Signal(NULL, 0, *detachSessionSignal, detachSessionArgs, ArraySize(detachSessionArgs), 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
            if (status != ER_OK) {
                QCC_LogError(status, ("Error sending org.alljoyn.Daemon.DetachSession signal"));
            }
        }

        /* Locks must be released before calling RemoveSessionRefs since that method calls out to user (SessionLost) */
        ReleaseLocks();

        /* Remove entries from sessionMap, send a SessionLost to the session member being removed. */
        if (RemoveSessionRefs(sessionMemberName, id, true) == false) {
            /* Remove session routes */
            router.RemoveSessionRoutes(sessionMemberName, id);
        } else {
            router.RemoveSelfJoinSessionRoute(sessionMemberName, id);
        }

    } else {
        ReleaseLocks();
    }

    /* Reply to request */
    MsgArg replyArgs[1];
    replyArgs[0].Set("u", replyCode);
    status = MethodReply(msg, replyArgs, ArraySize(replyArgs));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.RemoveSessionMember"));
    }
}

void AllJoynObj::GetHostInfo(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_GETHOSTINFO_REPLY_SUCCESS;

    size_t numArgs;
    const MsgArg* args;

    /* Parse the message args */
    msg->GetArgs(numArgs, args);
    assert(numArgs == 1);
    SessionId id = static_cast<SessionId>(args[0].v_uint32);

    QCC_DbgPrintf(("AllJoynObj::GetHostInfo(%u)", id));

    String remoteIpAddrStr = "";
    String localIpAddrStr = "";
    /* Find the session with that id */
    AcquireLocks();
    SessionMapEntry* smEntry = SessionMapFind(msg->GetSender(), id);
    if (!smEntry || (id == 0)) {
        replyCode = ALLJOYN_GETHOSTINFO_REPLY_NO_SESSION;
        ReleaseLocks();
    } else if (smEntry->sessionHost == msg->GetSender()) {
        replyCode = ALLJOYN_GETHOSTINFO_REPLY_IS_BINDER;
        ReleaseLocks();
    } else {
        /* get the vep to the sessionhost.
         */
        VirtualEndpoint vep;
        router.FindEndpoint(smEntry->sessionHost, vep);
        if (vep->IsValid()) {
            RemoteEndpoint rep = vep->GetBusToBusEndpoint(id);
            QStatus status = rep->GetRemoteIp(remoteIpAddrStr);
            if (status != ER_OK) {
                replyCode = ALLJOYN_GETHOSTINFO_REPLY_NOT_SUPPORTED_ON_TRANSPORT;
            }
            status = rep->GetLocalIp(localIpAddrStr);
            if (status != ER_OK) {
                replyCode = ALLJOYN_GETHOSTINFO_REPLY_NOT_SUPPORTED_ON_TRANSPORT;
            }
        } else {
            replyCode = ALLJOYN_GETHOSTINFO_REPLY_FAILED;
        }

        ReleaseLocks();
    }

    const char* remoteIpAddr = remoteIpAddrStr.c_str();
    const char* localIpAddr = localIpAddrStr.c_str();

    /* Reply to request */
    MsgArg replyArgs[3];
    replyArgs[0].Set("u", replyCode);
    replyArgs[1].Set("s", localIpAddr);
    replyArgs[2].Set("s", remoteIpAddr);
    QStatus status = MethodReply(msg, replyArgs, ArraySize(replyArgs));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.GetHostInfo"));
    }
}

void AllJoynObj::ReloadConfig(const InterfaceDescription::Member* member, Message& msg)
{
    ConfigDB* config = ConfigDB::GetConfigDB();
    bool loaded = config->LoadConfig(&bus);
    MsgArg replyArg;

    replyArg.Set("b", loaded);

    QStatus status = MethodReply(msg, &replyArg, 1);

    /* Log error if reply could not be sent */
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.ReloadConfig"));
    }
}

qcc::ThreadReturn STDCALL AllJoynObj::JoinSessionThread::RunAttach()
{
    QCC_DbgTrace(("JoinSessionThread::RunAttach()"));

    SessionId id = 0;
    String creatorName;
    MsgArg replyArgs[4];
    SessionOpts optsOut;
    uint32_t replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
    bool destIsLocal = false;

    /* Default member list to empty */
    replyArgs[3].Set("as", 0, NULL);

    /* Received a daemon request to establish a session route */

    /* Parse message args */
    SessionPort sessionPort;
    const char* src;
    const char* sessionHost;
    const char* dest;
    const char* srcB2B;
    const char* busAddr;
    SessionOpts optsIn;
    RemoteEndpoint srcB2BEp;
    RemoteEndpoint b2bEp;
    String srcStr;
    String destStr;
    bool newSME = false;
    SessionMapEntry sme;

    size_t na;
    const MsgArg* args;
    msg->GetArgs(na, args);
    QStatus status = MsgArg::Get(args, 6, "qsssss", &sessionPort, &src, &sessionHost, &dest, &srcB2B, &busAddr);
    const String srcB2BStr = srcB2B;

    QCC_DbgPrintf(("JoinSessionThread::RunAttach(): sessionPort=%d, src=\"%s\", sessionHost=\"%s\", dest=\"%s\", srcB2B=\"%s\", busAddr=\"%s\"",
                   sessionPort, src, sessionHost, dest, srcB2B, busAddr));

    bool sendSessionJoined = false;
    if (status == ER_OK) {
        status = GetSessionOpts(args[6], optsIn);

        QCC_DbgPrintf(("AllJoynObj::RunAttach(): optsIn.traffic=0x%x, optisIn.proximity=0x%x, optsIn.transports=0x%x",
                       optsIn.traffic, optsIn.proximity, optsIn.transports));
    }

    if (status != ER_OK) {
        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Bad args"));
        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
        ajObj.AcquireLocks();
    } else {
        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Good request.  Starting."));
        srcStr = src;
        destStr = dest;

        ajObj.AcquireLocks();
        /*
         * If there is an outstanding join involving (sessionHost,port), then destEp may not be valid yet.
         * Essentially, someone else might know we are a multipoint session member before we do.
         */
        BusEndpoint destEp = ajObj.router.FindEndpoint(destStr);
        if ((destEp->GetEndpointType() != ENDPOINT_TYPE_REMOTE) && (destEp->GetEndpointType() != ENDPOINT_TYPE_NULL) && (destEp->GetEndpointType() != ENDPOINT_TYPE_LOCAL)) {
            /* Release locks while waiting */
            ajObj.ReleaseLocks();
            QCC_DbgPrintf(("AllJoynObj::RunAttach(): Endpoint for destStr=\"%s\" exists but is invalid.  Sleeping.", destStr.c_str()));
            qcc::Sleep(500);
            ajObj.AcquireLocks();
            destEp = ajObj.router.FindEndpoint(destStr);
        }

        /* Determine if the dest is local to this daemon */
        if ((destEp->GetEndpointType() == ENDPOINT_TYPE_REMOTE) || (destEp->GetEndpointType() == ENDPOINT_TYPE_NULL) || (destEp->GetEndpointType() == ENDPOINT_TYPE_LOCAL)) {

            QCC_DbgPrintf(("AllJoynObj::RunAttach(): destStr=\"%s\" served directly.", destStr.c_str()));

            /* This daemon serves dest directly */
            /* Check for a session in the session map */
            bool foundSessionMapEntry = false;
            String destUniqueName = destEp->GetUniqueName();
            BusEndpoint sessionHostEp = ajObj.router.FindEndpoint(sessionHost);
            SessionMapType::iterator sit = ajObj.SessionMapLowerBound(destUniqueName, 0);
            replyCode = ALLJOYN_JOINSESSION_REPLY_SUCCESS;
            while ((sit != ajObj.sessionMap.end()) && (sit->first.first == destUniqueName)) {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Found destUniqueName=\"%s\" in session map.", destUniqueName.c_str()));
                BusEndpoint creatorEp = ajObj.router.FindEndpoint(sit->second.sessionHost);
                sme = sit->second;
                if ((sme.sessionPort == sessionPort) && sessionHostEp->IsValid() && (creatorEp == sessionHostEp)) {

                    QCC_DbgPrintf(("AllJoynObj::RunAttach(): Valid session map entry for sessionPort=%d", sessionPort));

                    if (sit->second.opts.isMultipoint && (sit->first.second == 0)) {
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Session is multipoint"));

                        /* Session is multipoint. Look for an existing (already joined) session */
                        while ((sit != ajObj.sessionMap.end()) && (sit->first.first == destUniqueName)) {
                            creatorEp = ajObj.router.FindEndpoint(sit->second.sessionHost);
                            if ((sit->first.second != 0) && (sit->second.sessionPort == sessionPort) && (creatorEp == sessionHostEp)) {
                                sme = sit->second;
                                foundSessionMapEntry = true;
                                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Found session map entry"));
                                /* make sure session is not already joined by this joiner */
                                vector<String>::const_iterator mit = sit->second.memberNames.begin();
                                while (mit != sit->second.memberNames.end()) {
                                    if (*mit++ == srcStr) {
                                        replyCode = ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED;
                                        foundSessionMapEntry = false;
                                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Already joined"));
                                        break;
                                    }
                                }
                                break;
                            }
                            ++sit;
                        }
                    } else if (sme.opts.isMultipoint && (sit->first.second == msg->GetSessionId())) {
                        /* joiner to joiner multipoint attach message */
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Joiner to Joiner multipoint attach message"));
                        foundSessionMapEntry = true;
                    } else if (!sme.opts.isMultipoint && (sit->first.second != 0)) {
                        /* Cannot join a non-multipoint session more than once */
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Multiple joins to non-multipoint session detected"));
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    }
                    if ((replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) && !foundSessionMapEntry) {
                        /* Assign a session id and insert entry */
                        while (sme.id == 0) {
                            sme.id = qcc::Rand32();
                        }
                        sme.isInitializing = true;
                        foundSessionMapEntry = true;
                        ajObj.SessionMapInsert(sme);
                        newSME = true;
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Allocate new session id: %d", sme.id));
                    }
                    break;
                }
                ++sit;
            }
            if (!foundSessionMapEntry) {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Unable to find a session map entry"));
                if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_NO_SESSION;
                }
            } else if (!sme.opts.IsCompatible(optsIn)) {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Incompatible options"));
                replyCode = ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS;
                optsOut = sme.opts;
            } else {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Valid session map entry"));

                optsOut = sme.opts;
                optsOut.transports &= optsIn.transports;

                BusEndpoint tempEp = ajObj.router.FindEndpoint(srcStr);
                VirtualEndpoint srcEp = VirtualEndpoint::cast(tempEp);
                tempEp = ajObj.router.FindEndpoint(srcB2BStr);
                srcB2BEp = RemoteEndpoint::cast(tempEp);
                if (srcB2BEp->IsValid() && srcEp->IsValid()) {
                    QCC_DbgPrintf(("AllJoynObj::RunAttach(): srcB2BEp IsValid(), srcEp IsValid()"));
                    uint32_t protoVer = srcB2BEp->GetFeatures().protocolVersion;
                    QCC_DbgPrintf(("AllJoynObj::RunAttach(): protoVer=%d.", protoVer));
                    if (protoVer < 9) {
                        srcB2BEp->GetFeatures().nameTransfer = sme.opts.nameTransfer;
                    }

                    /* Store ep for raw sessions (for future close and fd extract) */
                    if (optsOut.traffic != SessionOpts::TRAFFIC_MESSAGES) {
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): traffic != TRAFFIC_MESSAGES"));
                        SessionMapEntry* smEntry = ajObj.SessionMapFind(sme.endpointName, sme.id);
                        if (smEntry) {
                            smEntry->streamingEp = srcB2BEp;
                        }
                    }

                    /* If this node is the session creator, give it a chance to accept or reject the new member */
                    bool isAccepted = true;
                    BusEndpoint creatorEp = ajObj.router.FindEndpoint(sme.sessionHost);

                    if (creatorEp->IsValid() && (destEp == creatorEp)) {
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): SendAcceptSession()"));
                        ajObj.ReleaseLocks();
                        status = ajObj.SendAcceptSession(sme.sessionPort, sme.id, dest, src, optsIn, isAccepted);

                        if (ER_OK != status) {
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                            QCC_LogError(status, ("SendAcceptSession failed"));
                        }

                        /* Re-lock and re-acquire */
                        ajObj.AcquireLocks();
                        if (!destEp->IsValid() || !srcEp->IsValid()) {
                            QCC_LogError(ER_FAIL, ("%s (%s) disappeared during JoinSession", !destEp->IsValid() ? "destEp" : "srcB2BEp", !destEp->IsValid() ? destStr.c_str() : srcB2BStr.c_str()));
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                        }
                    }

                    /* Add new joiner to members */
                    if (isAccepted && creatorEp->IsValid() && (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS)) {

                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Joinee accepted.  Adding joiner"));

                        SessionMapEntry* smEntry = ajObj.SessionMapFind(sme.endpointName, sme.id);
                        /* Update sessionMap */
                        if (smEntry) {
                            QCC_DbgPrintf(("AllJoynObj::RunAttach(): Adding srcStr=\"%s\" to session map entry", srcStr.c_str()));
                            smEntry->memberNames.push_back(srcStr);
                            id = smEntry->id;
                            destIsLocal = true;
                            creatorName = creatorEp->GetUniqueName();
                            /* create the list of members for the AttachSession reply.
                             * Include every member from this session map entry, apart from a self-joined host.
                             * We can't include that one because it would confuse legacy routers. They'd end up
                             * creating double session routes and corrupting their session cast set */
                            vector<String> replyMembers;
                            for (vector<String>::const_iterator mit = smEntry->memberNames.begin();
                                 mit != smEntry->memberNames.end(); ++mit) {
                                if (*mit != smEntry->sessionHost) {
                                    replyMembers.push_back(*mit);
                                }
                            }
                            replyArgs[3].Set("a$", replyMembers.size(), &replyMembers.front());

                        } else {
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                        }

                        /* Add routes for new session */
                        if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                            if (optsOut.traffic == SessionOpts::TRAFFIC_MESSAGES) {
                                BusEndpoint busEndpoint = BusEndpoint::cast(srcEp);
                                QCC_DbgPrintf(("AllJoynObj::RunAttach(): AddSessionRoute() for id=%d.", id));
                                status = ajObj.router.AddSessionRoute(id, destEp, NULL, busEndpoint, srcB2BEp);
                                if (ER_OK != status) {
                                    QCC_LogError(status, ("AddSessionRoute(%u, %s, NULL, %s, %s) failed", id, dest, srcEp->GetUniqueName().c_str(), srcB2BEp->GetUniqueName().c_str()));
                                }
                            }

                            /* Send SessionJoined to creator */
                            if (ER_OK == status && creatorEp->IsValid() && (destEp == creatorEp)) {
                                sendSessionJoined = true;
                            }
                        }
                    } else {
                        replyCode =  ALLJOYN_JOINSESSION_REPLY_REJECTED;
                    }
                } else {
                    status = ER_FAIL;
                    replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    if (!srcB2BEp->IsValid()) {
                        QCC_LogError(status, ("Cannot locate srcB2BEp(%s)", srcB2BStr.c_str()));
                    }
                    if (!srcEp->IsValid()) {
                        QCC_LogError(status, ("Cannot locate srcEp(%s)", srcStr.c_str()));
                    }
                }
            }
        } else {
            QCC_DbgPrintf(("AllJoynObj::RunAttach(): destStr=\"%s\" routes indirectly", destStr.c_str()));
            /* This daemon will attempt to route indirectly to dest */
            if ((busAddr[0] == '\0') && (msg->GetSessionId() != 0) && (destEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Secondary (multipoint) attach.  Increment reference"));
                /* This is a secondary (multipoint) attach.
                 * Forward the attach to the dest over the existing session id's B2BEp */
                VirtualEndpoint vep = VirtualEndpoint::cast(destEp);
                b2bEp = vep->GetBusToBusEndpoint(msg->GetSessionId());
                if (b2bEp->IsValid()) {
                    b2bEp->IncrementRef();
                }
            } else if (busAddr[0] != '\0') {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Indirect route.  Find transport for busAddr=\"%s\"", busAddr));
                /* Ask the transport for an endpoint */
                TransportList& transList = ajObj.bus.GetInternal().GetTransportList();
                Transport* trans = transList.GetTransport(busAddr);
                if (trans == NULL) {
                    replyCode = ALLJOYN_JOINSESSION_REPLY_UNREACHABLE;
                } else {
                    ajObj.ReleaseLocks();
                    BusEndpoint ep;
                    QCC_DbgPrintf(("AllJoynObj::RunAttach(): Indirect route. Connect() to  busAddr=\"%s\"", busAddr));
                    status = trans->Connect(busAddr, optsIn, ep);
                    ajObj.AcquireLocks();
                    if (status == ER_OK) {
                        b2bEp = RemoteEndpoint::cast(ep);
                        if (b2bEp->IsValid()) {
                            b2bEp->IncrementRef();
                        }
                    } else {
                        QCC_LogError(status, ("trans->Connect(%s) failed", busAddr));
                        replyCode = ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
                    }
                }
            }

            if (!b2bEp->IsValid()) {
                replyCode = ALLJOYN_JOINSESSION_REPLY_NO_SESSION;
            } else {
                /* Forward AttachSession to next hop */
                SessionId tempId;
                SessionOpts tempOpts;
                const String nextControllerName = b2bEp->GetRemoteName();

                QCC_DbgPrintf(("AllJoynObj::RunAttach(): Forward AttachSession to  busAddr=\"%s\" at nextControllerName=\"%s\"",
                               busAddr, nextControllerName.c_str()));

                /* Send AttachSession */
                ajObj.ReleaseLocks();
                status = ajObj.SendAttachSession(sessionPort, src, sessionHost, dest, b2bEp, nextControllerName.c_str(),
                                                 msg->GetSessionId(), busAddr, optsIn, replyCode, tempId, tempOpts, replyArgs[3]);
                ajObj.AcquireLocks();

                /* If successful, add bi-directional session routes */
                if ((status == ER_OK) && (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS)) {

                    QCC_DbgPrintf(("AllJoynObj::RunAttach(): SendAttachSession() success"));

                    /* Wait for dest to appear with a route through b2bEp */
                    uint64_t startTime = GetTimestamp64();
                    VirtualEndpoint vDestEp;
                    while (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                        /* Does vSessionEp route through b2bEp? If so, we're done */
                        if (!b2bEp->IsValid()) {
                            QCC_LogError(ER_FAIL, ("B2B endpoint disappeared during AttachSession"));
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                            break;
                        }
                        if (ajObj.router.FindEndpoint(destStr, vDestEp) && vDestEp->CanUseRoute(b2bEp)) {
                            QCC_DbgPrintf(("AllJoynObj::RunAttach(): Indirect route appeared"));
                            break;
                        }
                        /* Otherwise wait */
                        uint64_t now = GetTimestamp64();
                        if (now > (startTime + 30000LL)) {
                            replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                            QCC_LogError(ER_FAIL, ("AttachSession timed out waiting for destination to appear"));
                            break;
                        } else {
                            /* Give up the locks while waiting */
                            ajObj.ReleaseLocks();
                            qcc::Sleep(10);
                            ajObj.AcquireLocks();
                        }
                    }

                    BusEndpoint tempEp = ajObj.router.FindEndpoint(srcStr);
                    VirtualEndpoint srcEp = VirtualEndpoint::cast(tempEp);
                    tempEp = ajObj.router.FindEndpoint(srcB2BStr);
                    srcB2BEp = RemoteEndpoint::cast(tempEp);
                    /* Add bi-directional session routes */
                    if (srcB2BEp->IsValid() && srcEp->IsValid() && vDestEp->IsValid() && b2bEp->IsValid()) {
                        id = tempId;
                        optsOut = tempOpts;
                        BusEndpoint busEndpointDest = BusEndpoint::cast(vDestEp);
                        BusEndpoint busEndpointSrc = BusEndpoint::cast(srcEp);
                        status = ajObj.router.AddSessionRoute(id, busEndpointDest, &b2bEp, busEndpointSrc, srcB2BEp);
                        if (status != ER_OK) {
                            QCC_LogError(status, ("AddSessionRoute(%u, %s, %s, %s) failed",
                                                  id, dest, b2bEp->GetUniqueName().c_str(), srcEp->GetUniqueName().c_str(), srcB2BEp->GetUniqueName().c_str()));
                        }
                    } else {
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    }
                } else {
                    QCC_LogError(status, ("AttachSession failed (reply=%d)", replyCode));
                    if (status == ER_OK) {
                        status = ER_BUS_REPLY_IS_ERROR_MESSAGE;
                    }
                    if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
                    }
                }
            }
            if (b2bEp->IsValid()) {
                b2bEp->DecrementRef();
            }
        }
    }

    /* Reply to request */
    QCC_DbgPrintf(("AllJoynObj::RunAttach(): Reply to request"));
    replyArgs[0].Set("u", replyCode);
    replyArgs[1].Set("u", id);
    SetSessionOpts(optsOut, replyArgs[2]);

    /*
     * On success, ensure that reply goes over the new b2b connection. Otherwise a race condition
     * related to shutting down endpoints that are to become raw will occur.
     */
    /* Obtain the srcB2BEp */
    BusEndpoint tempEp = ajObj.router.FindEndpoint(srcB2BStr);
    srcB2BEp = RemoteEndpoint::cast(tempEp);
    if (srcB2BEp->IsValid()) {
        ajObj.ReleaseLocks();
        status = msg->ReplyMsg(msg, replyArgs, ArraySize(replyArgs));
        if (status == ER_OK) {
            status = srcB2BEp->PushMessage(msg);
        }
    } else {
        ajObj.ReleaseLocks();
        status = ajObj.MethodReply(msg, replyArgs, ArraySize(replyArgs));
    }

    /* Send SessionJoined to creator */
    if (sendSessionJoined) {
        QCC_DbgPrintf(("AllJoynObj::RunAttach(): SendSessionJoined(port=%d., id=%d, source=\"%s\", endpoint=\"%s\")",
                       sme.sessionPort, sme.id, srcStr.c_str(), sme.endpointName.c_str()));
        ajObj.SendSessionJoined(sme.sessionPort, sme.id, srcStr.c_str(), sme.endpointName.c_str());
    }
    ajObj.AcquireLocks();

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Daemon.AttachSession."));
    }

    /* Special handling for successful raw session creation. (Must occur after reply is sent) */
    if (srcB2BEp->IsValid() && (optsOut.traffic != SessionOpts::TRAFFIC_MESSAGES)) {
        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Direct raw session handling"));
        if (!b2bEp->IsValid()) {
            if (!creatorName.empty()) {
                /* Destination for raw session. Shutdown endpoint and preserve the fd for future call to GetSessionFd */
                SessionMapEntry* smEntry = ajObj.SessionMapFind(creatorName, id);
                if (smEntry) {
                    if (smEntry->streamingEp->IsValid()) {
                        ajObj.ReleaseLocks();
                        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Direct raw session handling.  ShutdownEndpoint()"));
                        status = ajObj.ShutdownEndpoint(smEntry->streamingEp, smEntry->fd);

                        ajObj.AcquireLocks();
                        smEntry = ajObj.SessionMapFind(creatorName, id);
                        if (smEntry) {
                            if (status != ER_OK) {
                                QCC_LogError(status, ("Failed to shutdown raw endpoint"));
                            }
                            smEntry->streamingEp->Invalidate();
                            smEntry->isRawReady = true;
                        }
                    }
                }
                if (!smEntry) {
                    QCC_LogError(ER_FAIL, ("Failed to find SessionMapEntry \"%s\",%08x", creatorName.c_str(), id));
                }
            }
        } else {
            QCC_DbgPrintf(("AllJoynObj::RunAttach(): indirect raw session handling"));
            /* Indirect raw route (middle-man). Create a pump to copy raw data between endpoints */
            QStatus tStatus;
            SocketFd srcB2bFd, b2bFd;
            ajObj.ReleaseLocks();
            status = ajObj.ShutdownEndpoint(srcB2BEp, srcB2bFd);
            tStatus = ajObj.ShutdownEndpoint(b2bEp, b2bFd);

            ajObj.AcquireLocks();
            status = (status == ER_OK) ? tStatus : status;
            if (status == ER_OK) {
                QCC_DbgPrintf(("AllJoynObj::RunAttach(): indirect raw session handling. Create message pump."));
                SocketStream* ss1 = new SocketStream(srcB2bFd);
                SocketStream* ss2 = new SocketStream(b2bFd);
                size_t chunkSize = 4096;
                String threadNameStr = id;
                threadNameStr.append("-pump");
                const char* threadName = threadNameStr.c_str();
                bool isManaged = true;
                ManagedObj<StreamPump> pump(ss1, ss2, chunkSize, threadName, isManaged);
                status = pump->Start();
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("Raw relay creation failed"));
            }
        }
    }

    /* Clear the initializing state (or cleanup) any initializing sessionMap entry */
    if (newSME) {
        QCC_DbgPrintf(("AllJoynObj::RunAttach(): Cleanup"));
        SessionMapEntry* smEntry = ajObj.SessionMapFind(sme.endpointName, sme.id);
        if (smEntry) {
            if (replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) {
                smEntry->isInitializing = false;
            } else {
                ajObj.SessionMapErase(sme);
            }
        } else {
            QCC_LogError(ER_BUS_NO_SESSION, ("Error clearing initializing entry in sessionMap"));
        }
    }

    ajObj.ReleaseLocks();

    /* Send SessionChanged if multipoint */
    if ((replyCode == ALLJOYN_JOINSESSION_REPLY_SUCCESS) && optsOut.isMultipoint && (id != 0) && destIsLocal) {
        QCC_DbgPrintf(("AllJoynObj::RunAttach(): SendMPSessionChanged(id=%d, source=\"%s, true, dest)", id, srcStr.c_str(), destStr.c_str()));
        ajObj.SendMPSessionChanged(id, srcStr.c_str(), true, destStr.c_str(), ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_ADDED);
    }

    QCC_DbgPrintf(("AllJoynObj::RunAttach(%d) returned (%d,%u) (status=%s)", sessionPort, replyCode, id, QCC_StatusText(status)));

    return 0;
}

void AllJoynObj::AddAdvNameAlias(const String& guid, const TransportMask mask, const String& advName)
{
    QCC_DbgTrace(("AllJoynObj::AddAdvNameAlias(%s, 0x%x, %s)", guid.c_str(), mask, advName.c_str()));

    AcquireLocks();
    std::map<qcc::String, set<AdvAliasEntry> >::iterator it = advAliasMap.find(guid);
    if (it == advAliasMap.end()) {
        set<AdvAliasEntry> temp;
        AdvAliasEntry entry(advName, mask);
        temp.insert(entry);
        advAliasMap.insert(pair<String, set<AdvAliasEntry> >(guid, temp));
    } else {
        AdvAliasEntry entry(advName, mask);
        it->second.insert(entry);
    }
    ReleaseLocks();
}

bool AllJoynObj::RemoveSessionRefs(const char* epName, SessionId id, bool sendSessionLost, LeaveSessionType lst)
{
    QCC_DbgTrace(("AllJoynObj::RemoveSessionRefs(%s, %u, %u)", epName, id, sendSessionLost));

    AcquireLocks();

    BusEndpoint endpoint = router.FindEndpoint(epName);

    if (!endpoint->IsValid()) {
        ReleaseLocks();
        return false;
    }

    String epNameStr = endpoint->GetUniqueName();
    vector<pair<String, SessionId> > changedSessionMembers;
    vector<SessionMapEntry> sessionsLost;
    vector<String> epChangedSessionMembers;
    SessionMapEntry smeRemoved;
    bool foundSME = false;
    bool epPresentInAnyEntry = false;
    QStatus sessionLostReason = ER_FAIL;

    SessionMapType::iterator it = sessionMap.begin();
    /* Look through sessionMap for entries matching id */
    while (it != sessionMap.end()) {
        bool toRemove = false;
        if (it->first.second == id) {
            if (it->first.first == epNameStr) {
                bool selfJoinEntry = false;
                /* Exact key matches are removed */

                /* special logic in the case this exact match entry was about self-join
                 * This logic is largely the same as in non exact match branch
                 */
                if (it->second.IsSelfJoin() && it->second.sessionHost == epNameStr) {
                    selfJoinEntry = true;
                    bool endPointIsMember = false;
                    if ((lst == LEAVE_SESSION) ||
                        (lst == LEAVE_JOINED_SESSION)) {
                        /* Remove matching session members */
                        vector<String>::iterator mit = it->second.memberNames.begin();
                        while (mit != it->second.memberNames.end()) {
                            if (epNameStr == *mit) {
                                endPointIsMember = true;
                                mit = it->second.memberNames.erase(mit);
                                if (it->second.opts.isMultipoint) {
                                    changedSessionMembers.push_back(it->first);
                                }
                            } else {
                                ++mit;
                            }
                        }
                    }

                    if (((lst == LEAVE_SESSION && endPointIsMember == false) || lst == LEAVE_HOSTED_SESSION)) {
                        if (endpoint == router.FindEndpoint(it->second.sessionHost)) {
                            /* Modify entry to remove matching sessionHost */
                            it->second.sessionHost.clear();
                            if (it->second.opts.isMultipoint) {
                                changedSessionMembers.push_back(it->first);
                            }
                        }
                    }

                    /* Session is lost when members + sessionHost together contain only one entry */
                    if ((it->second.fd == qcc::INVALID_SOCKET_FD) && (it->second.memberNames.empty() || ((it->second.memberNames.size() == 1) && it->second.sessionHost.empty()))) {
                        SessionMapEntry tsme = it->second;
                        sessionsLost.push_back(tsme);
                        toRemove = true;
                    }
                } else {
                    toRemove = true;
                }

                if (sendSessionLost) {
                    smeRemoved = it->second;
                    epChangedSessionMembers.push_back(smeRemoved.sessionHost);
                    vector<String>::iterator mit = smeRemoved.memberNames.begin();
                    while (mit != smeRemoved.memberNames.end()) {
                        if (epNameStr != *mit) {
                            epChangedSessionMembers.push_back(*mit++);
                        } else {
                            ++mit;
                        }
                    }
                }

                if (selfJoinEntry == true) {
                    sessionLostReason = ER_BUS_REMOVED_BY_BINDER_SELF;
                } else {
                    sessionLostReason = ER_BUS_REMOVED_BY_BINDER;
                }

                foundSME = true;
            } else {
                bool endPointIsMember = false;

                if ((lst == LEAVE_SESSION) ||
                    (lst == LEAVE_JOINED_SESSION)) {
                    /* Remove matching session members */
                    vector<String>::iterator mit = it->second.memberNames.begin();
                    while (mit != it->second.memberNames.end()) {
                        if (epNameStr == *mit) {
                            endPointIsMember = true;
                            mit = it->second.memberNames.erase(mit);
                            if (it->second.opts.isMultipoint) {
                                changedSessionMembers.push_back(it->first);
                            }
                        } else {
                            ++mit;
                        }
                    }
                }

                if ((lst == LEAVE_SESSION && endPointIsMember == false) || lst == LEAVE_HOSTED_SESSION) {
                    if (endpoint == router.FindEndpoint(it->second.sessionHost)) {
                        /* Modify entry to remove matching sessionHost */
                        it->second.sessionHost.clear();
                        if (it->second.opts.isMultipoint) {
                            changedSessionMembers.push_back(it->first);
                        }
                    }
                }

                /* Session is lost when members + sessionHost together contain only one entry */
                if ((it->second.fd == qcc::INVALID_SOCKET_FD) && (it->second.memberNames.empty() || ((it->second.memberNames.size() == 1) && it->second.sessionHost.empty()))) {
                    SessionMapEntry tsme = it->second;
                    if (!it->second.isInitializing) {
                        toRemove = true;
                    }
                    sessionsLost.push_back(tsme);
                }
            }

            if (!toRemove && !epPresentInAnyEntry &&
                ((endpoint == router.FindEndpoint(it->second.sessionHost)) ||
                 (find(it->second.memberNames.begin(), it->second.memberNames.end(), epNameStr) != it->second.memberNames.end()))) {
                epPresentInAnyEntry = true;
            }

        }
        if (toRemove) {
            sessionMap.erase(it++);
        } else {
            ++it;
        }
    }
    ReleaseLocks();

    /* Send MPSessionChanged for each changed session involving alias */
    vector<pair<String, SessionId> >::const_iterator csit = changedSessionMembers.begin();
    while (csit != changedSessionMembers.end()) {
        /* We should not send the MPSessionChanged if this member was still present (selfjoin case), with the exception of its own leaf */
        if (epPresentInAnyEntry == false || epNameStr == csit->first) {
            SendMPSessionChanged(csit->second, epNameStr.c_str(), false, csit->first.c_str(), ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_REMOVED);
        }
        csit++;
    }
    /* Send MPSessionChanged to the member being removed by the binder */
    vector<String>::const_iterator csitEp = epChangedSessionMembers.begin();
    while (csitEp != epChangedSessionMembers.end()) {
        SendMPSessionChanged(id, (*csitEp).c_str(), false, epNameStr.c_str(), ALLJOYN_MPSESSIONCHANGED_LOCAL_MEMBER_REMOVED);
        csitEp++;
    }
    /* Send session lost signals */
    vector<SessionMapEntry>::iterator slit = sessionsLost.begin();
    while (slit != sessionsLost.end()) {
        if (slit->memberNames.size() == 1) {
            SendSessionLost(*slit++, ER_OK, ALLJOYN_SESSIONLOST_DISPOSITION_MEMBER);
        } else {
            SendSessionLost(*slit++, ER_OK, ALLJOYN_SESSIONLOST_DISPOSITION_HOST);
        }
    }
    if (foundSME && sendSessionLost) {
        SendSessionLost(smeRemoved, sessionLostReason, ALLJOYN_SESSIONLOST_DISPOSITION_MEMBER);
    }

    return epPresentInAnyEntry;
}

void AllJoynObj::RemoveSessionRefs(const String& vepName, const String& b2bEpName)
{
    QCC_DbgTrace(("AllJoynObj::RemoveSessionRefs(%s, %s)",  vepName.c_str(), b2bEpName.c_str()));

    VirtualEndpoint vep;
    RemoteEndpoint b2bEp;

    AcquireLocks();

    if (!router.FindEndpoint(vepName, vep)) {
        QCC_LogError(ER_FAIL, ("Virtual endpoint %s disappeared during RemoveSessionRefs", vepName.c_str()));
        ReleaseLocks();
        return;
    }
    if (!router.FindEndpoint(b2bEpName, b2bEp)) {
        QCC_LogError(ER_FAIL, ("B2B endpoint %s disappeared during RemoveSessionRefs", b2bEpName.c_str()));
        ReleaseLocks();
        return;
    }

    QStatus disconnectReason = b2bEp->GetDisconnectStatus();

    vector<pair<String, SessionId> > changedSessionMembers;
    vector<SessionMapEntry> sessionsLost;
    SessionMapType::iterator it = sessionMap.begin();
    while (it != sessionMap.end()) {
        int count;
        /* Skip binding reservations */
        if (it->first.second == 0) {
            ++it;
            continue;
        }
        /* Examine sessions with ids that are affected by removal of vep through b2bep */
        /* Only sessions that route through a single (matching) b2bEp are affected */
        if ((vep->GetBusToBusEndpoint(it->first.second, &count) == b2bEp) && (count == 1)) {
            if (it->first.first == vepName) {
                /* Key matches can be removed from sessionMap */
                sessionMap.erase(it++);
            } else {
                if (BusEndpoint::cast(vep) == router.FindEndpoint(it->second.sessionHost)) {
                    /* If the session's sessionHost is vep, then clear it out of the session */
                    it->second.sessionHost.clear();
                    if (it->second.opts.isMultipoint) {
                        changedSessionMembers.push_back(it->first);
                    }
                } else {
                    /* Clear vep from any session members */
                    vector<String>::iterator mit = it->second.memberNames.begin();
                    while (mit != it->second.memberNames.end()) {
                        if (vepName == *mit) {
                            mit = it->second.memberNames.erase(mit);
                            if (it->second.opts.isMultipoint) {
                                changedSessionMembers.push_back(it->first);
                            }
                        } else {
                            ++mit;
                        }
                    }
                }
                /* A session with only one member and no sessionHost or only a sessionHost are "lost" */
                if ((it->second.fd == qcc::INVALID_SOCKET_FD) && (it->second.memberNames.empty() || ((it->second.memberNames.size() == 1) && it->second.sessionHost.empty()))) {
                    SessionMapEntry tsme = it->second;
                    pair<String, SessionId> key = it->first;
                    if (!it->second.isInitializing) {
                        sessionMap.erase(it++);
                    } else {
                        ++it;
                    }
                    sessionsLost.push_back(tsme);
                } else {
                    ++it;
                }
            }
        } else {
            ++it;
        }
    }
    ReleaseLocks();

    /* Send MPSessionChanged for each changed session involving alias */
    vector<pair<String, SessionId> >::const_iterator csit = changedSessionMembers.begin();
    while (csit != changedSessionMembers.end()) {
        SendMPSessionChanged(csit->second, vepName.c_str(), false, csit->first.c_str(), ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_REMOVED);
        csit++;
    }
    /* Send session lost signals */
    vector<SessionMapEntry>::iterator slit = sessionsLost.begin();
    while (slit != sessionsLost.end()) {
        if (slit->memberNames.size() == 1) {
            SendSessionLost(*slit++, disconnectReason, ALLJOYN_SESSIONLOST_DISPOSITION_MEMBER);
        } else {
            SendSessionLost(*slit++, disconnectReason, ALLJOYN_SESSIONLOST_DISPOSITION_HOST);
        }
    }
}

void AllJoynObj::GetSessionInfo(const InterfaceDescription::Member* member, Message& msg)
{
    /* Received a daemon request for session info */

    /* Parse message args */
    const char* creatorName;
    SessionPort sessionPort;
    SessionOpts optsIn;
    vector<String> busAddrs;

    size_t na;
    const MsgArg* args;
    msg->GetArgs(na, args);
    QStatus status = MsgArg::Get(args, 2, "sq", &creatorName, &sessionPort);
    if (status == ER_OK) {
        status = GetSessionOpts(args[2], optsIn);
    }

    if (status == ER_OK) {
        QCC_DbgTrace(("AllJoynObj::GetSessionInfo(%s, %u, <%x, %x, %x>)", creatorName, sessionPort, optsIn.traffic, optsIn.proximity, optsIn.transports));

        /* Ask the appropriate transport for the listening busAddr */
        TransportList& transList = bus.GetInternal().GetTransportList();
        for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
            Transport* trans = transList.GetTransport(i);
            if (trans && (trans->GetTransportMask() & optsIn.transports)) {
                trans->GetListenAddresses(optsIn, busAddrs);
            } else if (!trans) {
                QCC_LogError(ER_BUS_TRANSPORT_NOT_AVAILABLE, ("NULL transport pointer found in transportList"));
            }
        }
    } else {
        QCC_LogError(status, ("AllJoynObj::GetSessionInfo cannot parse args"));
    }

    if (busAddrs.empty()) {
        status = MethodReply(msg, ER_BUS_NO_SESSION);
    } else {
        MsgArg replyArg("a$", busAddrs.size(), &busAddrs[0]);
        status = MethodReply(msg, &replyArg, 1);
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("GetSessionInfo failed"));
    }
}

QStatus AllJoynObj::SendAttachSession(SessionPort sessionPort,
                                      const char* src,
                                      const char* sessionHost,
                                      const char* dest,
                                      RemoteEndpoint& b2bEp,
                                      const char* remoteControllerName,
                                      SessionId outgoingSessionId,
                                      const char* busAddr,
                                      const SessionOpts& optsIn,
                                      uint32_t& replyCode,
                                      SessionId& id,
                                      SessionOpts& optsOut,
                                      MsgArg& members)
{
    QStatus status = ER_OK;
    Message reply(bus);
    MsgArg attachArgs[7];
    attachArgs[0].Set("q", sessionPort);
    attachArgs[1].Set("s", src);
    attachArgs[2].Set("s", sessionHost);
    attachArgs[3].Set("s", dest);
    attachArgs[4].Set("s", b2bEp->GetUniqueName().c_str());
    attachArgs[5].Set("s", busAddr);
    SetSessionOpts(optsIn, attachArgs[6]);
    ProxyBusObject controllerObj(bus, remoteControllerName, org::alljoyn::Daemon::ObjectPath, outgoingSessionId);
    controllerObj.AddInterface(*daemonIface);

    /* If the new session is raw, then arm the endpoint's RX thread to stop after reading one more message */
    if ((status == ER_OK) && (optsIn.traffic != SessionOpts::TRAFFIC_MESSAGES)) {
        status = b2bEp->PauseAfterRxReply();
    }

    /* Make the method call */
    if (status == ER_OK) {
        QCC_DbgPrintf(("Sending AttachSession(%u, %s, %s, %s, %s, %s, <%x, %x, %x>) to %s",
                       attachArgs[0].v_uint16,
                       attachArgs[1].v_string.str,
                       attachArgs[2].v_string.str,
                       attachArgs[3].v_string.str,
                       attachArgs[4].v_string.str,
                       attachArgs[5].v_string.str,
                       optsIn.proximity, optsIn.traffic, optsIn.transports,
                       remoteControllerName));

        controllerObj.SetB2BEndpoint(b2bEp);
        status = controllerObj.MethodCall(org::alljoyn::Daemon::InterfaceName,
                                          "AttachSession",
                                          attachArgs,
                                          ArraySize(attachArgs),
                                          reply,
                                          30000);
    }

    if (status != ER_OK) {
        replyCode = ALLJOYN_JOINSESSION_REPLY_FAILED;
        QCC_LogError(status, ("SendAttachSession failed"));
    } else {
        const MsgArg* replyArgs;
        size_t numReplyArgs;
        reply->GetArgs(numReplyArgs, replyArgs);
        replyCode = replyArgs[0].v_uint32;
        id = replyArgs[1].v_uint32;
        status = GetSessionOpts(replyArgs[2], optsOut);
        if (status == ER_OK) {
            members = *reply->GetArg(3);
            QCC_DbgPrintf(("Received AttachSession response: replyCode=%d, sessionId=%u, opts=<%x, %x, %x>",
                           replyCode, id, optsOut.proximity, optsOut.traffic, optsOut.transports));
        } else {
            QCC_DbgPrintf(("Received AttachSession response: <bad_args>"));
        }
    }

    return status;
}

QStatus AllJoynObj::SendSessionJoined(SessionPort sessionPort,
                                      SessionId sessionId,
                                      const char* joinerName,
                                      const char* creatorName)
{
    MsgArg args[3];
    args[0].Set("q", sessionPort);
    args[1].Set("u", sessionId);
    args[2].Set("s", joinerName);

    QCC_DbgPrintf(("SendSessionJoined(%u, %u, %s) to %s",
                   args[0].v_uint16,
                   args[1].v_uint32,
                   args[2].v_string.str,
                   creatorName));

    AllJoynPeerObj* peerObj = bus.GetInternal().GetLocalEndpoint()->GetPeerObj();
    QStatus status = peerObj->Signal(creatorName, sessionId, *mpSessionJoinedSignal, args, ArraySize(args));
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to send SessionJoined to %s", creatorName));
    }

    return status;
}

QStatus AllJoynObj::SendAcceptSession(SessionPort sessionPort,
                                      SessionId sessionId,
                                      const char* creatorName,
                                      const char* joinerName,
                                      const SessionOpts& inOpts,
                                      bool& isAccepted)
{
    /* Give the receiver a chance to accept or reject the new member */
    Message reply(bus);
    MsgArg acceptArgs[4];
    acceptArgs[0].Set("q", sessionPort);
    acceptArgs[1].Set("u", sessionId);
    acceptArgs[2].Set("s", joinerName);
    SetSessionOpts(inOpts, acceptArgs[3]);
    ProxyBusObject peerObj(bus, creatorName, org::alljoyn::Bus::Peer::ObjectPath, 0);
    const InterfaceDescription* sessionIntf = bus.GetInterface(org::alljoyn::Bus::Peer::Session::InterfaceName);
    assert(sessionIntf);
    peerObj.AddInterface(*sessionIntf);

    QCC_DbgPrintf(("Calling AcceptSession(%d, %u, %s, <%x, %x, %x> to %s",
                   acceptArgs[0].v_uint16,
                   acceptArgs[1].v_uint32,
                   acceptArgs[2].v_string.str,
                   inOpts.proximity, inOpts.traffic, inOpts.transports,
                   creatorName));

    QStatus status = peerObj.MethodCall(org::alljoyn::Bus::Peer::Session::InterfaceName,
                                        "AcceptSession",
                                        acceptArgs,
                                        ArraySize(acceptArgs),
                                        reply);
    if (status == ER_OK) {
        size_t na;
        const MsgArg* replyArgs;
        reply->GetArgs(na, replyArgs);
        replyArgs[0].Get("b", &isAccepted);
    } else {
        isAccepted = false;
    }
    return status;
}

SessionListener::SessionLostReason AllJoynObj::ConvertReasonToSessionLostReason(QStatus reason) const {

    SessionListener::SessionLostReason replyCode;
    switch (reason) {
    case ER_OK:
        replyCode = SessionListener::ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION;
        break;

    case ER_SOCK_OTHER_END_CLOSED:
    case ER_BUS_ENDPOINT_CLOSING:
        replyCode = SessionListener::ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY;
        break;

    case ER_BUS_REMOVED_BY_BINDER:
        replyCode = SessionListener::ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER;
        break;

    case ER_BUS_REMOVED_BY_BINDER_SELF:
        replyCode = SessionListener::ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER_SELF;
        break;

    case ER_TIMEOUT:
        replyCode = SessionListener::ALLJOYN_SESSIONLOST_LINK_TIMEOUT;
        break;

    default:
        replyCode = SessionListener::ALLJOYN_SESSIONLOST_REASON_OTHER;
        break;
    }

    return replyCode;

}

void AllJoynObj::SendSessionLost(const SessionMapEntry& sme, QStatus reason, unsigned int disposition)
{
    /* Send SessionLost to the endpoint mentioned in sme */
    Message sigMsg(bus);

    AcquireLocks();
    BusEndpoint ep = router.FindEndpoint(sme.endpointName);


    if (ep->GetEndpointType() == ENDPOINT_TYPE_REMOTE && RemoteEndpoint::cast(ep)->GetRemoteProtocolVersion() < 7) {
        ReleaseLocks();
        /* For older clients i.e. protocol version < 7, emit SessionLost(u) signal */
        MsgArg args[1];
        args[0].Set("u", sme.id);
        QCC_DbgPrintf(("Sending SessionLost(%u) to %s", sme.id, sme.endpointName.c_str()));
        QStatus status = Signal(sme.endpointName.c_str(), sme.id, *sessionLostSignal, args, ArraySize(args));
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to send SessionLost(%d) to %s", sme.id, sme.endpointName.c_str()));
        }
    } else if (ep->GetEndpointType() == ENDPOINT_TYPE_REMOTE && RemoteEndpoint::cast(ep)->GetRemoteProtocolVersion() < 11) {
        ReleaseLocks();
        /* For newer clients i.e. protocol version [8:10], emit SessionLostWithReason(uu) signal */
        MsgArg args[2];
        args[0].Set("u", sme.id);
        args[1].Set("u", ConvertReasonToSessionLostReason(reason));
        QCC_DbgPrintf(("Sending sessionLostWithReason(%u, %s) to %s", sme.id, QCC_StatusText(reason), sme.endpointName.c_str()));

        QStatus status = Signal(sme.endpointName.c_str(), sme.id, *sessionLostWithReasonSignal, args, ArraySize(args));

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to send sessionLostWithReason(%u, %s) to %s", sme.id, QCC_StatusText(reason), sme.endpointName.c_str()));
        }
    } else {
        ReleaseLocks();
        /* For newer clients i.e. protocol version >= 11, emit SessionLostWithReason(uub) signal */
        MsgArg args[3];
        args[0].Set("u", sme.id);
        args[1].Set("u", ConvertReasonToSessionLostReason(reason));
        args[2].Set("u", disposition);
        QCC_DbgPrintf(("Sending sessionLostWithReasonAndDisposition(%u, %s, %d) to %s", sme.id, QCC_StatusText(reason), disposition, sme.endpointName.c_str()));

        QStatus status = Signal(sme.endpointName.c_str(), sme.id, *sessionLostWithReasonAndDispositionSignal, args, ArraySize(args));

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to send sessionLostWithReason(%u, %s) to %s", sme.id, QCC_StatusText(reason), sme.endpointName.c_str()));
        }
    }
}

void AllJoynObj::SendMPSessionChanged(SessionId sessionId, const char* name, bool isAdd, const char* dest, unsigned int reason)
{
    Message msg(bus);

    AcquireLocks();
    BusEndpoint ep = router.FindEndpoint(dest);

    if (ep->GetEndpointType() == ENDPOINT_TYPE_REMOTE && RemoteEndpoint::cast(ep)->GetRemoteProtocolVersion() <= 10) {
        ReleaseLocks();
        MsgArg args[3];
        args[0].Set("u", sessionId);
        args[1].Set("s", name);
        args[2].Set("b", isAdd);
        QCC_DbgPrintf(("Sending MPSessionChanged(%u, %s, %s) to %s", sessionId, name, isAdd ? "true" : "false", dest));
        QStatus status = Signal(dest, sessionId, *mpSessionChangedSignal, args, ArraySize(args));
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to send MPSessionChanged to %s", dest));
        }
    } else {
        ReleaseLocks();
        MsgArg args[4];
        args[0].Set("u", sessionId);
        args[1].Set("s", name);
        args[2].Set("b", isAdd);
        args[3].Set("u", reason);
        QCC_DbgPrintf(("Sending MPSessionChanged(%u, %s, %s, %u) to %s", sessionId, name, isAdd ? "true" : "false", reason, dest));
        QStatus status = Signal(dest, sessionId, *mpSessionChangedWithReason, args, ArraySize(args));
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to send MPSessionChangedWithReason to %s", dest));
        }
    }
}

QStatus AllJoynObj::SendGetSessionInfo(const char* creatorName,
                                       SessionPort sessionPort,
                                       const SessionOpts& opts,
                                       vector<String>& busAddrs)
{
    QStatus status = ER_BUS_NO_ENDPOINT;

    /* Send GetSessionInfo to creatorName */
    Message reply(bus);
    MsgArg sendArgs[3];
    sendArgs[0].Set("s", creatorName);
    sendArgs[1].Set("q", sessionPort);
    SetSessionOpts(opts, sendArgs[2]);

    BusEndpoint creatorEp = router.FindEndpoint(creatorName);
    if (creatorEp->IsValid()) {
        String controllerName = creatorEp->GetControllerUniqueName();
        ProxyBusObject rObj(bus, controllerName.c_str(), org::alljoyn::Daemon::ObjectPath, 0);
        const InterfaceDescription* intf = bus.GetInterface(org::alljoyn::Daemon::InterfaceName);
        assert(intf);
        rObj.AddInterface(*intf);
        QCC_DbgPrintf(("Calling GetSessionInfo(%s, %u, <%x, %x, %x>) on %s",
                       sendArgs[0].v_string.str,
                       sendArgs[1].v_uint16,
                       opts.proximity, opts.traffic, opts.transports,
                       controllerName.c_str()));

        status = rObj.MethodCall(org::alljoyn::Daemon::InterfaceName,
                                 "GetSessionInfo",
                                 sendArgs,
                                 ArraySize(sendArgs),
                                 reply);
        if (status == ER_OK) {
            size_t na;
            const MsgArg* replyArgs;
            const MsgArg* busAddrArgs;
            size_t numBusAddrs;
            reply->GetArgs(na, replyArgs);
            replyArgs[0].Get("as", &numBusAddrs, &busAddrArgs);
            for (size_t i = numBusAddrs; i > 0; --i) {
                busAddrs.push_back(busAddrArgs[i - 1].v_string.str);
            }
        }
    }
    return status;
}

QStatus AllJoynObj::ShutdownEndpoint(RemoteEndpoint& b2bEp, SocketFd& sockFd)
{
    SocketStream& ss = static_cast<SocketStream&>(b2bEp->GetStream());
    /* Grab the file descriptor for the B2B endpoint and close the endpoint */
    ss.DetachSocketFd();
    SocketFd epSockFd = ss.GetSocketFd();
    if (!epSockFd) {
        return ER_BUS_NOT_CONNECTED;
    }
    QStatus status = SocketDup(epSockFd, sockFd);
    if (status == ER_OK) {
        status = b2bEp->StopAfterTxEmpty();
        if (status == ER_OK) {
            status = b2bEp->Join();
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to join RemoteEndpoint used for streaming"));
                sockFd = qcc::INVALID_SOCKET_FD;
            }
        } else {
            QCC_LogError(status, ("Failed to stop RemoteEndpoint used for streaming"));
            sockFd = qcc::INVALID_SOCKET_FD;
        }
    } else {
        QCC_LogError(status, ("Failed to dup remote endpoint's socket"));
        sockFd = qcc::INVALID_SOCKET_FD;
    }
    return status;
}

void AllJoynObj::DetachSessionSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;

    /* Parse message args */
    msg->GetArgs(numArgs, args);
    assert(numArgs == 2);
    SessionId id = args[0].v_uint32;
    const char* src = args[1].v_string.str;

    QCC_DbgTrace(("AllJoynObj::DetachSessionSignalHandler(src=%s, id=%u)", src, id));

    /* Do not process our own detach message signals */
    if (::strncmp(guid.ToShortString().c_str(), msg->GetSender() + 1, qcc::GUID128::SHORT_SIZE) == 0) {
        return;
    }

    /* Remove session info from sessionmapentry, send a SessionLost to the member being removed. */
    if (RemoveSessionRefs(src, id, true) == false) {
        /* Remove session info from router */
        router.RemoveSessionRoutes(src, id);
    }
}

void AllJoynObj::GetSessionFd(const InterfaceDescription::Member* member, Message& msg)
{
    /* Parse args */
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);
    SessionId id = args[0].v_uint32;
    QStatus status;
    SocketFd sockFd = qcc::INVALID_SOCKET_FD;

    QCC_DbgTrace(("AllJoynObj::GetSessionFd(%u)", id));

    /* Wait for any join related operations to complete before returning fd */
    AcquireLocks();
    SessionMapEntry* smEntry = SessionMapFind(msg->GetSender(), id);
    if (smEntry && (smEntry->opts.traffic != SessionOpts::TRAFFIC_MESSAGES)) {
        uint64_t ts = GetTimestamp64();
        while (smEntry && !smEntry->isRawReady && ((ts + 5000LL) > GetTimestamp64())) {
            ReleaseLocks();
            qcc::Sleep(5);
            AcquireLocks();
            smEntry = SessionMapFind(msg->GetSender(), id);
        }
        /* sessionMap entry removal was delayed waiting for sockFd to become available. Delete it now. */
        if (smEntry) {
            sockFd = smEntry->fd;
            SessionMapErase(*smEntry);
        }
    }
    ReleaseLocks();

    if (sockFd != qcc::INVALID_SOCKET_FD) {
        /* Send the fd and transfer ownership */
        MsgArg replyArg;
        replyArg.Set("h", sockFd);
        status = MethodReply(msg, &replyArg, 1);
        qcc::Close(sockFd);
    } else {
        /* Send an error */
        status = MethodReply(msg, ER_BUS_NO_SESSION);
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.GetSessionFd"));
    }
}

AllJoynObj::SessionMapEntry* AllJoynObj::SessionMapFind(const qcc::String& name, SessionId session)
{
    pair<String, SessionId> key(name, session);
    AllJoynObj::SessionMapType::iterator it = sessionMap.find(key);
    if (it == sessionMap.end()) {
        return NULL;
    } else {
        return &(it->second);
    }
}

AllJoynObj::SessionMapType::iterator AllJoynObj::SessionMapLowerBound(const qcc::String& name, SessionId session)
{
    pair<String, SessionId> key(name, session);
    return sessionMap.lower_bound(key);
}

AllJoynObj::SessionMapType::iterator AllJoynObj::SessionMapUpperBound(const qcc::String& name, SessionId session)
{
    pair<String, SessionId> key(name, session);
    return sessionMap.upper_bound(key);
}

void AllJoynObj::SessionMapInsert(SessionMapEntry& sme)
{
    pair<String, SessionId> key(sme.endpointName, sme.id);
    sessionMap.insert(pair<pair<String, SessionId>, SessionMapEntry>(key, sme));
}

void AllJoynObj::SessionMapErase(SessionMapEntry& sme)
{
    pair<String, SessionId> key(sme.endpointName, sme.id);
    sessionMap.erase(key);
}

void AllJoynObj::SetLinkTimeout(const InterfaceDescription::Member* member, Message& msg)
{
    /* Parse args */
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);
    SessionId id = args[0].v_uint32;
    uint32_t reqLinkTimeout = args[1].v_uint32;
    uint32_t actLinkTimeout = reqLinkTimeout;
    bool foundEp = false;
    uint32_t disposition;
    QStatus status = ER_OK;

    /* Set the link timeout on all endpoints that are involved in this session */
    AcquireLocks();
    SessionMapType::iterator it = SessionMapLowerBound(msg->GetSender(), id);

    while ((it != sessionMap.end()) && (it->first.first == msg->GetSender()) && (it->first.second == id)) {
        SessionMapEntry& entry = it->second;
        if (entry.opts.traffic == SessionOpts::TRAFFIC_MESSAGES) {
            vector<String> memberNames = entry.memberNames;
            memberNames.push_back(entry.sessionHost);
            for (size_t i = 0; i < memberNames.size(); ++i) {
                BusEndpoint memberEp = router.FindEndpoint(memberNames[i]);
                if (memberEp->IsValid() && (memberEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
                    VirtualEndpoint vMemberEp = VirtualEndpoint::cast(memberEp);
                    RemoteEndpoint b2bEp = vMemberEp->GetBusToBusEndpoint(id);
                    if (b2bEp->IsValid()) {
                        uint32_t tTimeout = reqLinkTimeout;
                        QStatus tStatus = b2bEp->SetLinkTimeout(tTimeout);
                        status = (status == ER_OK) ? tStatus : status;
                        actLinkTimeout = ((tTimeout == 0) || (actLinkTimeout == 0)) ? 0 : max(actLinkTimeout, tTimeout);
                        foundEp = true;
                    }
                } else if ((memberEp->GetEndpointType() == ENDPOINT_TYPE_REMOTE) || (memberEp->GetEndpointType() == ENDPOINT_TYPE_NULL)) {
                    /*
                     * This is a locally connected client. These clients do not have per-session connecions
                     * therefore we silently allow this as if we had granted the user's request
                     */
                    foundEp = true;
                }
            }
        }
        ++it;
    }
    ReleaseLocks();

    /* Set disposition */
    if (status == ER_ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT) {
        disposition = ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT;
    } else if (!foundEp) {
        disposition = ALLJOYN_SETLINKTIMEOUT_REPLY_NO_SESSION;
        actLinkTimeout = 0;
    } else if (status != ER_OK) {
        disposition = ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED;
        actLinkTimeout = 0;
    } else {
        disposition = ALLJOYN_SETLINKTIMEOUT_REPLY_SUCCESS;
    }

    /* Send response */
    MsgArg replyArgs[2];
    replyArgs[0].Set("u", disposition);
    replyArgs[1].Set("u", actLinkTimeout);
    status = MethodReply(msg, replyArgs, ArraySize(replyArgs));
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.SetLinkTimeout"));
    }
    QCC_DbgTrace(("AllJoynObj::SetLinkTimeout(%u, %d) (status=%s, disp=%d, lto=%d)", id, reqLinkTimeout,
                  QCC_StatusText(status), disposition, actLinkTimeout));
}
void AllJoynObj::SetIdleTimeouts(const InterfaceDescription::Member* member, Message& msg)
{
    /* Parse args */
    uint32_t disposition = ALLJOYN_SETIDLETIMEOUTS_REPLY_FAILED;
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);
    uint32_t actIdleTimeout = 0;
    uint32_t actProbeTimeout = 0;
    uint32_t reqIdleTimeout = 0;
    uint32_t reqProbeTimeout = 0;

    if (numArgs == 2) {
        reqIdleTimeout = args[0].v_uint32;
        reqProbeTimeout = args[1].v_uint32;
        actIdleTimeout = reqIdleTimeout;
        actProbeTimeout = reqProbeTimeout;
        AcquireLocks();
        BusEndpoint senderEp = router.FindEndpoint(msg->GetSender());
        if (senderEp->IsValid()) {
            if (senderEp->GetEndpointType() == ENDPOINT_TYPE_REMOTE) {
                RemoteEndpoint rep = RemoteEndpoint::cast(senderEp);
                rep->SetIdleTimeouts(actIdleTimeout, actProbeTimeout);
                disposition = ALLJOYN_SETIDLETIMEOUTS_REPLY_SUCCESS;
            } else {
                disposition = ALLJOYN_SETIDLETIMEOUTS_REPLY_NOT_ALLOWED;
            }
        }
        ReleaseLocks();
    }

    /* Send response */
    MsgArg replyArgs[3];
    replyArgs[0].Set("u", disposition);
    replyArgs[1].Set("u", actIdleTimeout);
    replyArgs[2].Set("u", actProbeTimeout);

    QStatus status = MethodReply(msg, replyArgs, ArraySize(replyArgs));
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.SetIdleTimeouts"));
    }

    QCC_DbgPrintf(("SetIdleTimeouts(%u,%u) (disposition=%u, actIdleTo=%u, actProbeTo=%u)",
                   reqIdleTimeout, reqProbeTimeout, disposition, actIdleTimeout, actProbeTimeout));
}
void AllJoynObj::AliasUnixUser(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_ALIASUNIXUSER_REPLY_SUCCESS;
    /* Parse args */
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);
    uint32_t aliasUID = args[0].v_uint32;
    uint32_t origUID = 0;
    qcc::String sender = msg->GetSender();
    BusEndpoint srcEp = router.FindEndpoint(sender);
    replyCode = PermissionMgr::AddAliasUnixUser(srcEp, sender, origUID, aliasUID);

    /* Send response */
    MsgArg replyArg;
    replyArg.Set("u", replyCode);
    MethodReply(msg, &replyArg, 1);
    QCC_DbgPrintf(("AllJoynObj::AliasUnixUser(%d) returned %d", aliasUID, replyCode));
}

void AllJoynObj::OnAppSuspend(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_ONAPPSUSPEND_REPLY_SUCCESS;
    qcc::String sender = msg->GetSender();
    BusEndpoint srcEp = router.FindEndpoint(sender);
    if (srcEp->IsValid()) {
        // Only allow NullEndpoint to make this call
        if (srcEp->GetEndpointType() == ENDPOINT_TYPE_NULL) {
            if (ER_OK != IpNameService::Instance().OnProcSuspend()) {
                replyCode = ALLJOYN_ONAPPSUSPEND_REPLY_FAILED;
            }
        } else {
            QCC_DbgPrintf(("OnAppSuspend() is only supported for bundled daemon"));
            replyCode = ALLJOYN_ONAPPSUSPEND_REPLY_NO_SUPPORT;
        }
    } else {
        QCC_LogError(ER_BUS_NO_ENDPOINT, ("AllJoynObj::OnAppSuspend() sender endpoint is invalid"));
        replyCode = ALLJOYN_ONAPPSUSPEND_REPLY_FAILED;
    }

    /* Reply to request */
    MsgArg replyArg;
    replyArg.Set("u", replyCode);
    QStatus status = MethodReply(msg, &replyArg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("AllJoynObj::OnAppSuspend() failed to send reply message"));
    }
}

void AllJoynObj::OnAppResume(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_ONAPPRESUME_REPLY_SUCCESS;
    qcc::String sender = msg->GetSender();
    BusEndpoint srcEp = router.FindEndpoint(sender);
    if (srcEp->IsValid()) {
        // Only allow NullEndpoint to make this call
        if (srcEp->GetEndpointType() == ENDPOINT_TYPE_NULL) {
            if (ER_OK != IpNameService::Instance().OnProcResume()) {
                replyCode = ALLJOYN_ONAPPRESUME_REPLY_FAILED;
            }
        } else {
            QCC_DbgPrintf(("OnAppResume() is only supported for bundled daemon"));
            replyCode = ALLJOYN_ONAPPRESUME_REPLY_NO_SUPPORT;
        }
    } else {
        QCC_LogError(ER_BUS_NO_ENDPOINT, ("AllJoynObj::OnAppResume() sender endpoint is invalid"));
        replyCode = ALLJOYN_ONAPPRESUME_REPLY_FAILED;
    }

    /* Reply to request */
    MsgArg replyArg;
    replyArg.Set("u", replyCode);
    QStatus status = MethodReply(msg, &replyArg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("AllJoynObj::OnAppResume() failed to send reply message"));
    }
}

TransportMask AllJoynObj::GetCompleteTransportMaskFilter() {
    TransportList& transList = bus.GetInternal().GetTransportList();
    Transport* tcpTransport = transList.GetTransport("tcp:");
    Transport* udpTransport = transList.GetTransport("udp:");
    TransportMask filterComplete = (tcpTransport && tcpTransport->IsRunning()) ? TRANSPORT_TCP : 0;
    filterComplete |= (udpTransport && udpTransport->IsRunning()) ? TRANSPORT_UDP : 0;
    return filterComplete;
}
void AllJoynObj::AdvertiseName(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t replyCode = ALLJOYN_ADVERTISENAME_REPLY_SUCCESS;
    size_t numArgs;
    const MsgArg* args;
    MsgArg replyArg;
    const char* advertiseName;
    TransportMask transports = 0;
    bool quietly = false;

    /* Get AdvertiseName args */
    msg->GetArgs(numArgs, args);
    QStatus status = MsgArg::Get(args, numArgs, "sq", &advertiseName, &transports);
    QCC_DbgTrace(("AllJoynObj::AdvertiseName(%s, %x)", (status == ER_OK) ? advertiseName : "", transports));
    if (ER_OK != status) {
        QCC_LogError(status, ("Fail to parse msg parameters"));
        replyCode = ALLJOYN_ADVERTISENAME_REPLY_FAILED;
    }

    /* Get the sender name */
    qcc::String sender = msg->GetSender();
    BusEndpoint srcEp = router.FindEndpoint(sender);

    if (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS == replyCode) {
        if (PermissionMgr::GetDaemonBusCallPolicy(srcEp) == PermissionMgr::STDBUSCALL_SHOULD_REJECT) {
            QCC_DbgPrintf(("The sender endpoint is not allowed to call AdvertiseName()"));
            replyCode = ALLJOYN_ADVERTISENAME_REPLY_FAILED;
        } else if (PermissionMgr::GetDaemonBusCallPolicy(srcEp) == PermissionMgr::STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL) {
            transports &= TRANSPORT_LOCAL;
            QCC_DbgPrintf(("The sender endpoint is only allowed to use local transport"));
        }
    }

    if (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS == replyCode) {
        status = TransportPermission::FilterTransports(srcEp, sender, transports, "AdvertiseName");
        if (ER_OK != status) {
            QCC_LogError(status, ("Filter transports failed"));
        }
    }

    if (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS == replyCode) {
        qcc::String adNameStr = advertiseName;
        // If this is a quiet advertisement, the name has a prefix of "quiet@".
        size_t pos = adNameStr.find_first_of('@');
        if (pos != qcc::String::npos && (0 == adNameStr.compare(0, pos, "quiet"))) {
            quietly = true;
            advertiseName += (pos + 1);
        }

        /* Check to see if the advertise name is valid and well formed */
        if (IsLegalBusName(advertiseName)) {

            /* Check to see if advertiseName is already being advertised */
            AcquireLocks();
            String advertiseNameStr = advertiseName;
            multimap<qcc::String, pair<TransportMask, qcc::String> >::iterator it = advertiseMap.lower_bound(advertiseNameStr);

            bool foundEntry = false;
            while ((it != advertiseMap.end()) && (it->first == advertiseNameStr)) {
                if (it->second.second == sender) {
                    if ((it->second.first & transports) != 0) {
                        replyCode = ALLJOYN_ADVERTISENAME_REPLY_ALREADY_ADVERTISING;
                    }
                    foundEntry = true;
                    break;
                }
                ++it;
            }

            if (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS == replyCode) {
                bool transportsProcessed = false;
                TransportList& transList = bus.GetInternal().GetTransportList();
                for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
                    Transport* trans = transList.GetTransport(i);
                    if (trans && trans->IsBusToBus() && (trans->GetTransportMask() & transports)) {
                        transportsProcessed = true;
                    } else if (!trans) {
                        QCC_LogError(ER_BUS_TRANSPORT_NOT_AVAILABLE, ("NULL transport pointer found in transportList"));
                    }
                }
                /* Add to advertise map */
                if (transportsProcessed || (transports & TRANSPORT_LOCAL)) {
                    if (!foundEntry) {
                        advertiseMap.insert(pair<qcc::String, pair<TransportMask, qcc::String> >(advertiseNameStr, pair<TransportMask, String>(transports, sender)));
                    } else {
                        it->second.first |= transports;
                    }
                } else {
                    replyCode = ALLJOYN_ADVERTISENAME_REPLY_TRANSPORT_NOT_AVAILABLE;
                }
                ReleaseLocks();

                /* Advertise on transports specified */
                if (transportsProcessed) {
                    status = ER_BUS_BAD_SESSION_OPTS;
                    for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
                        Transport* trans = transList.GetTransport(i);
                        if (trans && trans->IsBusToBus() && (trans->GetTransportMask() & transports)) {
                            status = trans->EnableAdvertisement(advertiseNameStr, quietly, transports & GetCompleteTransportMaskFilter());
                            if ((status != ER_OK) && (status != ER_NOT_IMPLEMENTED)) {
                                QCC_LogError(status, ("EnableAdvertisment failed for transport %s - mask=0x%x", trans->GetTransportName(), transports));
                            }
                        } else if (!trans) {
                            QCC_LogError(ER_BUS_TRANSPORT_NOT_AVAILABLE, ("NULL transport pointer found in transportList"));
                        }
                    }
                }

            } else {
                ReleaseLocks();
            }
        } else {
            replyCode = ALLJOYN_ADVERTISENAME_REPLY_FAILED;
        }
    }

    /* Reply to request */
    String advNameStr = advertiseName;                         /* Needed since advertiseName will be corrupt after MethodReply */
    replyArg.Set("u", replyCode);
    status = MethodReply(msg, &replyArg, 1);

    QCC_DbgPrintf(("AllJoynObj::Advertise(%s) returned %d (status=%s)", advNameStr.c_str(), replyCode, QCC_StatusText(status)));

    /* Add advertisement to local nameMap so local discoverers can see this advertisement
       even if the advertisement is not enabled on the local transport. Note however that
       discoverers might see this advertisement as being advertised on the local transport
       even though it is not enabled on the local transport.
     */
    if (replyCode == ALLJOYN_ADVERTISENAME_REPLY_SUCCESS) {
        vector<String> names;
        names.push_back(advNameStr);
        FoundNames("local:", bus.GetGlobalGUIDString(), TRANSPORT_LOCAL, &names, numeric_limits<uint32_t>::max());
    }

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.Advertise"));
    }
}

void AllJoynObj::CancelAdvertiseName(const InterfaceDescription::Member* member, Message& msg)
{
    const MsgArg* args;
    size_t numArgs;

    /* Get the name being advertised */
    msg->GetArgs(numArgs, args);
    const char* advertiseName;
    TransportMask transports = 0;
    QStatus status = MsgArg::Get(args, numArgs, "sq", &advertiseName, &transports);
    if (status != ER_OK) {
        QCC_LogError(status, ("CancelAdvertiseName: bad arg types"));
        return;
    }

    // Strip off name prefix "quiet@" if exists
    qcc::String adNameStr = advertiseName;
    size_t pos = adNameStr.find_first_of('@');
    if (pos != qcc::String::npos && (0 == adNameStr.compare(0, pos, "quiet"))) {
        advertiseName += (pos + 1);
    }

    QCC_DbgTrace(("AllJoynObj::CancelAdvertiseName(%s, 0x%x)", advertiseName, transports));

    /* Cancel advertisement */
    status = ProcCancelAdvertise(msg->GetSender(), advertiseName, transports);
    uint32_t replyCode = (ER_OK == status) ? ALLJOYN_CANCELADVERTISENAME_REPLY_SUCCESS : ALLJOYN_CANCELADVERTISENAME_REPLY_FAILED;

    /* Reply to request */
    String advNameStr = advertiseName;                         /* Needed since advertiseName will be corrupt after MethodReply */
    MsgArg replyArg("u", replyCode);
    status = MethodReply(msg, &replyArg, 1);


    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.CancelAdvertise"));
    }
}

QStatus AllJoynObj::ProcCancelAdvertise(const qcc::String& sender, const qcc::String& advertiseName, TransportMask transports)
{
    QCC_DbgTrace(("AllJoynObj::ProcCancelAdvertise(%s, %s, %x)",
                  sender.c_str(),
                  advertiseName.c_str(),
                  transports));

    QStatus status = ER_OK;

    /* Check to see if this advertised name exists and delete it */
    bool foundAdvert = false;
    TransportMask refMask = 0;
    TransportMask cancelMask = 0;
    TransportMask origMask = 0;

    AcquireLocks();
    multimap<qcc::String, pair<TransportMask, qcc::String> >::iterator it = advertiseMap.find(advertiseName);
    while ((it != advertiseMap.end()) && (it->first == advertiseName)) {
        if (it->second.second == sender) {
            foundAdvert = true;
            origMask = it->second.first;
            it->second.first &= ~transports;
            if (it->second.first == 0) {
                advertiseMap.erase(it++);
                continue;
            }
        }
        refMask |= it->second.first;
        ++it;
    }

    cancelMask = transports & ~refMask;
    if (foundAdvert) {
        cancelMask &= origMask;
    }

    ReleaseLocks();

    /* Cancel transport advertisement if no other refs exist */
    if (foundAdvert && cancelMask) {
        TransportList& transList = bus.GetInternal().GetTransportList();
        for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
            Transport* trans = transList.GetTransport(i);
            if (trans && (trans->GetTransportMask() & cancelMask)) {
                trans->DisableAdvertisement(advertiseName, cancelMask & GetCompleteTransportMaskFilter());
            } else if (!trans) {
                QCC_LogError(ER_BUS_TRANSPORT_NOT_AVAILABLE, ("NULL transport pointer found in transportList"));
            }
        }

    } else if (!foundAdvert) {
        status = ER_FAIL;
    }

    /* Remove advertisement from local nameMap so local discoverers are notified of advertisement going away */
    if ((status == ER_OK) && (transports & TRANSPORT_LOCAL)) {
        vector<String> names;
        names.push_back(advertiseName);
        FoundNames("local:", bus.GetGlobalGUIDString(), TRANSPORT_LOCAL, &names, 0);
    }

    return status;
}

void AllJoynObj::FindAdvertisedName(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    qcc::String matchingStr;
    const char* str;
    QStatus status = MsgArg::Get(args, numArgs, "s", &str);
    if (status == ER_OK) {
        /*
         * When a bus name is advertised, the source may append a string that
         * identifies a specific instance of advertised name.  For example, one
         * might advertise something like
         *
         *   com.mycompany.myproduct.0123456789ABCDEF
         *
         * as a specific instance of the bus name,
         *
         *   com.mycompany.myproduct
         *
         * Clients of the system will want to be able to discover all specific
         * instances, so they need to do a wildcard search for bus name strings
         * that match the non-specific name, for example,
         *
         *   com.mycompany.myproduct*
         *
         * We automatically append the name service wildcard character to the end
         * of the provided string (which we call the namePrefix) before sending it
         * to the name service which forwards the request out over the net.
         */
        matchingStr = String("name='") + str + "*'";
    }

    ProcFindAdvertisement(status, msg, matchingStr, TRANSPORT_ANY);
}

void AllJoynObj::FindAdvertisedNameByTransport(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    qcc::String matchingStr;
    const char* str;
    TransportMask transports;
    QStatus status = MsgArg::Get(args, numArgs, "sq", &str, &transports);
    if (status == ER_OK) {
        matchingStr = String("name='") + str + "*'";
    }

    ProcFindAdvertisement(status, msg, matchingStr, transports);
}

void AllJoynObj::FindAdvertisementByTransport(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    qcc::String matchingStr;
    const char* str;
    TransportMask transports;
    QStatus status = MsgArg::Get(args, numArgs, "sq", &str, &transports);
    if (status == ER_OK) {
        matchingStr = str;
    }

    ProcFindAdvertisement(status, msg, matchingStr, transports);
}

void AllJoynObj::ProcFindAdvertisement(QStatus status, Message& msg, const qcc::String& matchingStr, TransportMask transports)
{
    uint32_t replyCode = ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS;
    TransportMask enableMask = 0;
    TransportMask origMask = 0;

    QCC_DbgTrace(("AllJoynObj::FindAdvertiseNameProc(%s)", matchingStr.c_str()));
    if (ER_OK != status) {
        QCC_LogError(status, ("Fail to parse msg parameters"));
        replyCode = ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED;
    }

    MatchMap matching;
    if (ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS == replyCode) {
        status = ParseMatchRule(matchingStr, matching);
        if (ER_OK != status) {
            QCC_LogError(status, ("Fail to parse matching paramter"));
            replyCode = ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED;
        }
    }

    qcc::String sender = msg->GetSender();

    AcquireLocks();
    BusEndpoint srcEp = router.FindEndpoint(sender);

    if (ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS == replyCode) {
        if (PermissionMgr::GetDaemonBusCallPolicy(srcEp) == PermissionMgr::STDBUSCALL_SHOULD_REJECT) {
            QCC_DbgPrintf(("The sender endpoint is not allowed to call FindAdvertisedName()"));
            replyCode = ER_ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED;
        } else if (PermissionMgr::GetDaemonBusCallPolicy(srcEp) == PermissionMgr::STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL) {
            QCC_DbgPrintf(("The sender endpoint is only allowed to use local transport."));
            transports &= TRANSPORT_LOCAL;
        }
    }

    if (ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS == replyCode) {
        status = TransportPermission::FilterTransports(srcEp, sender, transports, "AllJoynObj::FindAdvertisedName");
    }

    MatchMap::const_iterator namePrefix = matching.find("name");
    if (ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS == replyCode) {
        bool transportsProcessed = false;
        TransportList& transList = bus.GetInternal().GetTransportList();
        for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
            Transport* trans = transList.GetTransport(i);
            if (trans && trans->IsBusToBus() && (trans->GetTransportMask() & transports)) {
                transportsProcessed = true;
            } else if (!trans) {
                QCC_LogError(ER_BUS_TRANSPORT_NOT_AVAILABLE, ("NULL transport pointer found in transportList"));
            }
        }

        /* Check to see if this endpoint is already discovering this prefix */
        bool foundEntry = false;
        DiscoverMapType::iterator it = discoverMap.lower_bound(matchingStr);
        while ((it != discoverMap.end()) && (it->first == matchingStr)) {
            /* This is the transportMask of the transports that this name was being discovered prior to this FindAdvertisedName call. */
            origMask |= it->second.transportMask;
            if (it->second.sender == sender) {
                if ((it->second.transportMask & transports) != 0) {
                    replyCode = ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING;
                } else {
                    it->second.transportMask = it->second.transportMask | transports;
                }
                foundEntry = true;
            }
            ++it;
        }
        if (transportsProcessed || (transports & TRANSPORT_LOCAL)) {
            if (!foundEntry) {
                /* This is the fix for multiple found names issue.
                 * If this is a name-based query, set initComplete to false and set it to true after
                 * the calls to the transports are complete.
                 */
                discoverMap.insert(std::make_pair(matchingStr, DiscoverMapEntry(transports, sender, matching, namePrefix == matching.end())));
            }
        } else {
            replyCode = ALLJOYN_FINDADVERTISEDNAME_REPLY_TRANSPORT_NOT_AVAILABLE;
        }
    }
    /* Find out the transports on which discovery needs to be enabled for this name.
     * i.e. The ones that are set in the requested transport mask and not set in the origMask.
     */
    ReleaseLocks();
    enableMask = transports & ~origMask;
    if (ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS == replyCode) {
        /* Find name on all remote transports */
        TransportList& transList = bus.GetInternal().GetTransportList();
        for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
            Transport* trans = transList.GetTransport(i);
            if (trans && (trans->GetTransportMask() & enableMask)) {
                trans->EnableDiscovery(matchingStr.c_str(), enableMask & GetCompleteTransportMaskFilter());
            } else if (!trans) {
                QCC_LogError(ER_BUS_TRANSPORT_NOT_AVAILABLE, ("NULL transport pointer found in transportList"));
            }
        }
    }

    /* Reply to request */
    MsgArg replyArg("u", replyCode);
    status = MethodReply(msg, &replyArg, 1);
    QCC_DbgPrintf(("AllJoynObj::FindAdvertisedName(%s) returned %d (status=%s)", matchingStr.c_str(), replyCode, QCC_StatusText(status)));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.Discover"));
    }

    /* Send FoundAdvertisedName signals if there are existing matches for matching */
    if ((ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS == replyCode) && (namePrefix != matching.end())) {
        AcquireLocks();
        String prefix = namePrefix->second.substr(0, namePrefix->second.find_last_of('*'));
        multimap<String, NameMapEntry>::iterator it = nameMap.lower_bound(prefix);
        set<SentSetEntry> sentSet;
        while ((it != nameMap.end()) && !WildcardMatch(it->first, namePrefix->second)) {
            if ((it->second.transport & transports) == 0) {
                ++it;
                continue;
            }

            SentSetEntry sentSetEntry(it->first, it->second.transport);
            if (sentSet.find(sentSetEntry) == sentSet.end()) {
                String foundName = it->first;
                NameMapEntry nme = it->second;
                it = nameMap.lower_bound(prefix);
                sentSet.insert(sentSetEntry);
            } else {
                ++it;
            }
        }


        //Set initComplete to true
        DiscoverMapType::iterator dit = discoverMap.lower_bound(matchingStr);
        while ((dit != discoverMap.end()) && (dit->first == matchingStr)) {
            if (dit->second.sender == sender) {
                dit->second.initComplete = true;
                break;
            }
            ++dit;
        }
        ReleaseLocks();
        set<SentSetEntry>::iterator sit = sentSet.begin();
        while (sit != sentSet.end()) {
            status = SendFoundAdvertisedName(sender, (*sit).name, (*sit).transport, namePrefix->second);
            if (ER_OK != status) {
                QCC_LogError(status, ("Cannot send FoundAdvertisedName to %s for name=%s", sender.c_str(), (*sit).name.c_str()));
            }
            sit++;
        }
    }
}

void AllJoynObj::CancelFindAdvertisedName(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    qcc::String matchingStr;
    const char* str;
    QStatus status = MsgArg::Get(args, numArgs, "s", &str);
    if (status == ER_OK) {
        matchingStr = String("name='") + str + "*'";
    }

    HandleCancelFindAdvertisement(status, msg, matchingStr, TRANSPORT_ANY);
}

void AllJoynObj::CancelFindAdvertisedNameByTransport(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    qcc::String matchingStr;
    const char* str;
    TransportMask transports;
    QStatus status = MsgArg::Get(args, numArgs, "sq", &str, &transports);
    if (status == ER_OK) {
        matchingStr = String("name='") + str + "*'";
    }

    HandleCancelFindAdvertisement(status, msg, matchingStr, transports);
}

void AllJoynObj::CancelFindAdvertisementByTransport(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    qcc::String matchingStr;
    const char* str;
    TransportMask transports;
    QStatus status = MsgArg::Get(args, numArgs, "sq", &str, &transports);
    if (status == ER_OK) {
        matchingStr = str;
    }

    HandleCancelFindAdvertisement(status, msg, matchingStr, transports);
}

void AllJoynObj::HandleCancelFindAdvertisement(QStatus status, Message& msg, const qcc::String& matchingStr, TransportMask transports)
{
    uint32_t replyCode = ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_SUCCESS;

    /* Cancel advertisement */
    QCC_DbgPrintf(("Calling ProcCancelFindAdvertisement from HandleCancelFindAdvertisedment [%s]", Thread::GetThread()->GetName()));
    if (ER_OK == status) {
        status = ProcCancelFindAdvertisement(msg->GetSender(), matchingStr, transports);
        replyCode = (ER_OK == status) ? ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_SUCCESS : ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED;
    } else {
        QCC_LogError(status, ("HandleCancelFindAdvertisement() parse message arguments error"));
        replyCode = ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED;
    }
    /* Reply to request */
    MsgArg replyArg("u", replyCode);
    status = MethodReply(msg, &replyArg, 1);
    QCC_DbgPrintf(("AllJoynObj::CancelFindAdvertisement(%s) returned %d (status=%s)", matchingStr.c_str(), replyCode, QCC_StatusText(status)));

    /* Log error if reply could not be sent */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.CancelFindAdvertisement"));
    }
}

QStatus AllJoynObj::ProcCancelFindAdvertisement(const qcc::String& sender, const qcc::String& matchingStr, TransportMask transports)
{
    QCC_DbgTrace(("AllJoynObj::ProcCancelFindAdvertisement(sender = %s, matching = %s, transports = %d)", sender.c_str(), matchingStr.c_str(), transports));
    QStatus status = ER_OK;
    AcquireLocks();
    bool foundFinder = false;
    TransportMask refMask = 0;
    TransportMask origMask = 0;
    TransportMask cancelMask = 0;
    DiscoverMapType::iterator it = discoverMap.lower_bound(matchingStr);
    while ((it != discoverMap.end()) && (it->first == matchingStr)) {
        if (it->second.sender == sender) {
            foundFinder = true;
            origMask = it->second.transportMask;
            it->second.transportMask &= ~transports;
            if (it->second.transportMask == 0) {
                discoverMap.erase(it++);
                continue;
            }
        }
        refMask |= it->second.transportMask;
        ++it;
    }

    cancelMask = transports & ~refMask;
    if (foundFinder) {
        cancelMask &= origMask;
    }

    ReleaseLocks();

    /* Disable discovery if certain transports are no longer referenced for the name prefix */
    if (foundFinder && cancelMask) {
        TransportList& transList = bus.GetInternal().GetTransportList();
        for (size_t i = 0; i < transList.GetNumTransports(); ++i) {
            Transport* trans =  transList.GetTransport(i);
            if (trans && (trans->GetTransportMask() & cancelMask)) {
                trans->DisableDiscovery(matchingStr.c_str(), refMask & GetCompleteTransportMaskFilter());
            }
        }
    } else if (!foundFinder) {
        status = ER_FAIL;
    }
    return status;
}

QStatus AllJoynObj::AddBusToBusEndpoint(RemoteEndpoint& endpoint)
{
    QCC_DbgTrace(("AllJoynObj::AddBusToBusEndpoint(%s)", endpoint->GetUniqueName().c_str()));

    const qcc::String& shortGuidStr = endpoint->GetRemoteGUID().ToShortString();

    /* Add b2b endpoint */
    AcquireLocks();
    b2bEndpoints[endpoint->GetUniqueName()] = endpoint;
    ReleaseLocks();

    /* Create a virtual endpoint for talking to the remote bus control object */
    /* This endpoint will also carry broadcast messages for the remote bus */
    String remoteControllerName(":", 1, 16);
    remoteControllerName.append(shortGuidStr);
    remoteControllerName.append(".1");
    AddVirtualEndpoint(remoteControllerName, endpoint->GetUniqueName());

    /* Exchange existing bus names if connected to another daemon */
    return ExchangeNames(endpoint);
}

void AllJoynObj::RemoveBusToBusEndpoint(RemoteEndpoint& endpoint)
{
    QCC_DbgTrace(("AllJoynObj::RemoveBusToBusEndpoint(%s)", endpoint->GetUniqueName().c_str()));

    qcc::String guidToBeChecked;
    /* Be careful to lock the name table before locking the virtual endpoints since both locks are needed
     * and doing it in the opposite order invites deadlock
     */
    AcquireLocks();
    String b2bEpName = endpoint->GetUniqueName();

    /* Remove the B2B endpoint before removing virtual endpoints to ensure
     * that another thread does not try to re-add the B2B endpoint to a
     * virtual endpoint while this function is in progress.
     */
    b2bEndpoints.erase(endpoint->GetUniqueName());

    /* Remove any virtual endpoints associated with a removed bus-to-bus endpoint */
    map<qcc::String, VirtualEndpoint>::iterator it = virtualEndpoints.begin();
    while (it != virtualEndpoints.end()) {
        String vepName = it->first;
        /* Check if this virtual endpoint has a route through this bus-to-bus endpoint.
         * If not, no cleanup is required for this virtual endpoint.
         */
        if (!it->second->CanUseRoute(endpoint)) {
            it = virtualEndpoints.upper_bound(vepName);
            continue;
        }
        /* Clean sessionMap and report lost sessions */

        /*
         * Remove the sessionMap entries involving endpoint
         * This call must be made without holding locks since it can trigger LostSession callback
         */

        ReleaseLocks();
        RemoveSessionRefs(vepName, b2bEpName);
        AcquireLocks();
        it = virtualEndpoints.find(vepName);
        if (it == virtualEndpoints.end()) {
            /* If the virtual endpoint was lost, continue to the next virtual endpoint */
            it = virtualEndpoints.upper_bound(vepName);
            continue;
        }

        /* Remove endpoint (b2b) reference from this vep.
         * Note: If IsStopping() is true, then there is another thread that is in the process
         * of deleting this virtual endpoint. In this case, skip this virtual endpoint. */
        if (!it->second->IsStopping()) {
            if (it->second->RemoveBusToBusEndpoint(endpoint)) {
                /* The last b2b endpoint was removed from this vep. */
                String exitingEpName = it->second->GetUniqueName();

                /* Let directly connected daemons know that this virtual endpoint is gone. */
                map<qcc::StringMapKey, RemoteEndpoint>::iterator it2 = b2bEndpoints.begin();
                const qcc::GUID128& otherSideGuid = endpoint->GetRemoteGUID();
                guidToBeChecked = otherSideGuid.ToString();
                while ((it2 != b2bEndpoints.end()) && (it != virtualEndpoints.end())) {
                    if ((it2->second != endpoint) && (it2->second->GetRemoteGUID() != otherSideGuid) && (it2->second->GetFeatures().nameTransfer == SessionOpts::ALL_NAMES)) {
                        Message sigMsg(bus);
                        MsgArg args[3];
                        args[0].Set("s", exitingEpName.c_str());
                        args[1].Set("s", exitingEpName.c_str());
                        args[2].Set("s", "");

                        QStatus status = sigMsg->SignalMsg("sss",
                                                           org::alljoyn::Daemon::WellKnownName,
                                                           0,
                                                           org::alljoyn::Daemon::ObjectPath,
                                                           org::alljoyn::Daemon::InterfaceName,
                                                           "NameChanged",
                                                           args,
                                                           ArraySize(args),
                                                           0,
                                                           0);
                        if (ER_OK == status) {
                            String key = it->first;
                            String key2 = it2->first.c_str();
                            RemoteEndpoint ep = it2->second;
                            ReleaseLocks();
                            status = ep->PushMessage(sigMsg);
                            if (ER_OK != status) {
                                QCC_LogError(status, ("Failed to send NameChanged to %s", ep->GetUniqueName().c_str()));
                            }
                            AcquireLocks();
                            it2 = b2bEndpoints.lower_bound(key2);
                            if ((it2 != b2bEndpoints.end()) && (it2->first == key2)) {
                                ++it2;
                            }
                            it = virtualEndpoints.find(key);
                        } else {
                            ++it2;;
                        }
                    } else {
                        ++it2;
                    }
                }

                /* Remove virtual endpoint with no more b2b eps */
                if (it != virtualEndpoints.end()) {
                    String vepName = it->first;
                    ReleaseLocks();
                    RemoveVirtualEndpoint(vepName);
                    AcquireLocks();
                    it = virtualEndpoints.upper_bound(vepName);
                }

            } else {
                /* Need to hit NameTable here since name ownership of a vep alias may have changed */
                ReleaseLocks();
                router.UpdateVirtualAliases(vepName);
                AcquireLocks();
                it = virtualEndpoints.upper_bound(vepName);
            }
        }
    }

    ReleaseLocks();
    //
    // Check if guid for this name is eligible for removal from PeerInfoMap in NameService
    //
    if (!IsGuidLongStringKnown(guidToBeChecked)) {
        IpNameService::Instance().RemoveFromPeerInfoMap(guidToBeChecked);
    }

}

QStatus AllJoynObj::ExchangeNames(RemoteEndpoint& endpoint)
{
    QCC_DbgTrace(("AllJoynObj::ExchangeNames(endpoint = %s)", endpoint->GetUniqueName().c_str()));

    vector<pair<qcc::String, vector<qcc::String> > > names;
    QStatus status;

    /* Send local name table info to remote bus controller */
    AcquireLocks();
    router.GetUniqueNamesAndAliases(names);

    MsgArg argArray(ALLJOYN_ARRAY);
    MsgArg* entries = new MsgArg[names.size()];
    size_t numEntries = 0;
    vector<pair<qcc::String, vector<qcc::String> > >::const_iterator it = names.begin();
    LocalEndpoint localEndpoint = bus.GetInternal().GetLocalEndpoint();

    /* Send all endpoint info except for endpoints related to destination */
    while (it != names.end()) {
        BusEndpoint ep = router.FindEndpoint(it->first);
        bool isLocalInfo = (0 == ::strncmp(guid.ToShortString().c_str(), it->first.c_str() + 1, guid.ToShortString().size()));

        if ((ep->IsValid() && ((endpoint->GetFeatures().nameTransfer == SessionOpts::ALL_NAMES) || isLocalInfo) && ((ep->GetEndpointType() != ENDPOINT_TYPE_VIRTUAL) || VirtualEndpoint::cast(ep)->CanRouteWithout(endpoint->GetRemoteGUID())))) {
            MsgArg* aliasNames = new MsgArg[it->second.size()];
            vector<qcc::String>::const_iterator ait = it->second.begin();
            size_t numAliases = 0;
            while (ait != it->second.end()) {
                /* Send exportable endpoints */
                aliasNames[numAliases++].Set("s", ait->c_str());
                ++ait;
            }
            if (0 < numAliases) {
                entries[numEntries].Set("(sa*)", it->first.c_str(), numAliases, aliasNames);
                /*
                 * Set ownwership flag so entries array destructor will free inner message args.
                 */
                entries[numEntries].SetOwnershipFlags(MsgArg::OwnsArgs, true);
            } else {
                entries[numEntries].Set("(sas)", it->first.c_str(), 0, NULL);
                delete[] aliasNames;
            }
            ++numEntries;
        }
        ++it;
    }
    status = argArray.Set("a(sas)", numEntries, entries);
    if (ER_OK == status) {
        Message exchangeMsg(bus);
        status = exchangeMsg->SignalMsg("a(sas)",
                                        org::alljoyn::Daemon::WellKnownName,
                                        0,
                                        org::alljoyn::Daemon::ObjectPath,
                                        org::alljoyn::Daemon::InterfaceName,
                                        "ExchangeNames",
                                        &argArray,
                                        1,
                                        0,
                                        0);
        if (ER_OK == status) {
            ReleaseLocks();
            status = endpoint->PushMessage(exchangeMsg);
            AcquireLocks();
        }
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to send ExchangeName signal"));
    }
    ReleaseLocks();

    /*
     * This will also free the inner MsgArgs.
     */
    delete [] entries;
    return status;
}

void AllJoynObj::ExchangeNamesSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    QCC_DbgTrace(("AllJoynObj::ExchangeNamesSignalHandler(msg sender = \"%s\")", msg->GetSender()));

    bool madeChanges = false;
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);
    assert((1 == numArgs) && (ALLJOYN_ARRAY == args[0].typeId));
    const MsgArg* items = args[0].v_array.GetElements();
    const String& shortGuidStr = guid.ToShortString();

    /* Create a virtual endpoint for each unique name in args */
    /* Be careful to lock the name table before locking the virtual endpoints since both locks are needed
     * and doing it in the opposite order invites deadlock
     */
    AcquireLocks();

    map<qcc::StringMapKey, RemoteEndpoint>::iterator bit = b2bEndpoints.find(msg->GetRcvEndpointName());
    const size_t numItems = args[0].v_array.GetNumElements();
    if (bit != b2bEndpoints.end()) {
        qcc::GUID128 otherGuid = bit->second->GetRemoteGUID();
        const String& shortOtherGuidStr = otherGuid.ToShortString();
        StringMapKey key = bit->first;
        for (size_t i = 0; i < numItems; ++i) {
            if (bit == b2bEndpoints.end()) {
                QCC_DbgPrintf(("b2bEp %s disappeared during ExchangeNamesSignalHandler", key.c_str()));
                break;
            }
            assert(items[i].typeId == ALLJOYN_STRUCT);
            qcc::String uniqueName = items[i].v_struct.members[0].v_string.str;
            if (!IsLegalUniqueName(uniqueName.c_str())) {
                QCC_LogError(ER_FAIL, ("Invalid unique name \"%s\" in ExchangeNames message", uniqueName.c_str()));
                continue;
            } else if (0 == ::strncmp(uniqueName.c_str() + 1, shortGuidStr.c_str(), shortGuidStr.size())) {
                /* Cant accept a request to change a local name */
                continue;
            } else if ((bit->second->GetFeatures().nameTransfer != SessionOpts::ALL_NAMES) &&
                       (0 != ::strncmp(uniqueName.c_str() + 1, shortOtherGuidStr.c_str(), shortOtherGuidStr.size()))) {
                /* Filter out names from routers that predate the DAEMON_NAMES flag (if not ALL_NAMES) */
                continue;
            }

            /* Add a virtual endpoint */
            bool madeChange;
            String b2bName = bit->second->GetUniqueName();
            ReleaseLocks();
            AddVirtualEndpoint(uniqueName, b2bName, &madeChange);

            /* Relock and reacquire */
            AcquireLocks();
            BusEndpoint tempEp = router.FindEndpoint(uniqueName);
            VirtualEndpoint vep = VirtualEndpoint::cast(tempEp);
            bit = b2bEndpoints.find(key);
            if (bit == b2bEndpoints.end()) {
                QCC_DbgPrintf(("b2bEp %s disappeared during ExchangeNamesSignalHandler", key.c_str()));
                break;
            }

            if (madeChange) {
                madeChanges = true;
            }

            /* Add virtual aliases (remote well-known names) */
            const MsgArg* aliasItems = items[i].v_struct.members[1].v_array.GetElements();
            const size_t numAliases = items[i].v_struct.members[1].v_array.GetNumElements();
            for (size_t j = 0; j < numAliases; ++j) {
                assert(ALLJOYN_STRING == aliasItems[j].typeId);
                if (vep->IsValid()) {
                    ReleaseLocks();
                    bool madeChange = router.SetVirtualAlias(aliasItems[j].v_string.str, &vep, vep);
                    AcquireLocks();
                    bit = b2bEndpoints.find(key);
                    if (bit == b2bEndpoints.end()) {
                        QCC_DbgPrintf(("b2bEp %s disappeared during ExchangeNamesSignalHandler", key.c_str()));
                        break;
                    }
                    if (madeChange) {
                        madeChanges = true;
                    }
                }
            }
        }
    } else {
        QCC_LogError(ER_BUS_NO_ENDPOINT, ("Cannot find b2b endpoint %s", msg->GetRcvEndpointName()));
    }
    ReleaseLocks();

    /* If there were changes, forward message to all directly connected controllers except the one that
     * sent us this ExchangeNames
     */
    if (madeChanges) {
        AcquireLocks();
        map<qcc::StringMapKey, RemoteEndpoint>::const_iterator bit = b2bEndpoints.find(msg->GetRcvEndpointName());
        map<qcc::StringMapKey, RemoteEndpoint>::iterator it = b2bEndpoints.begin();
        while (it != b2bEndpoints.end()) {
            if ((it->second->GetFeatures().nameTransfer == SessionOpts::ALL_NAMES) && ((bit == b2bEndpoints.end()) || (bit->second->GetRemoteGUID() != it->second->GetRemoteGUID()))) {
                QCC_DbgPrintf(("Propagating ExchangeName signal to %s", it->second->GetUniqueName().c_str()));
                StringMapKey key = it->first;
                RemoteEndpoint ep = it->second;
                ReleaseLocks();
                QStatus status = ep->PushMessage(msg);
                if (ER_OK != status) {
                    QCC_LogError(status, ("Failed to forward ExchangeNames to %s", ep->GetUniqueName().c_str()));
                }
                AcquireLocks();
                bit = b2bEndpoints.find(msg->GetRcvEndpointName());
                it = b2bEndpoints.lower_bound(key);
                if ((it != b2bEndpoints.end()) && (it->first == key)) {
                    ++it;
                }
            } else {
                ++it;
            }
        }
        ReleaseLocks();
    }
}


void AllJoynObj::NameChangedSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    assert(daemonIface);

    const qcc::String alias = args[0].v_string.str;
    const qcc::String oldOwner = args[1].v_string.str;
    const qcc::String newOwner = args[2].v_string.str;

    const String& shortGuidStr = guid.ToShortString();
    bool madeChanges = false;

    QCC_DbgPrintf(("AllJoynObj::NameChangedSignalHandler: alias = \"%s\"   oldOwner = \"%s\"   newOwner = \"%s\"  sent from \"%s\"",
                   alias.c_str(), oldOwner.c_str(), newOwner.c_str(), msg->GetSender()));

    /* Don't allow a NameChange that attempts to change a local name */
    if ((!oldOwner.empty() && (0 == ::strncmp(oldOwner.c_str() + 1, shortGuidStr.c_str(), shortGuidStr.size()))) ||
        (!newOwner.empty() && (0 == ::strncmp(newOwner.c_str() + 1, shortGuidStr.c_str(), shortGuidStr.size())))) {
        return;
    }

    /* Ignore a NameChange for non-local names from routers that predate the DAEMON_NAMES flag (if not ALL_NAMES) */
    AcquireLocks();
    map<qcc::StringMapKey, RemoteEndpoint>::iterator bit = b2bEndpoints.find(msg->GetRcvEndpointName());
    if (bit != b2bEndpoints.end() && (bit->second->GetFeatures().nameTransfer != SessionOpts::ALL_NAMES)) {
        qcc::GUID128 otherGuid = bit->second->GetRemoteGUID();
        const String& shortOtherGuidStr = otherGuid.ToShortString();
        if ((!oldOwner.empty() && (0 != ::strncmp(oldOwner.c_str() + 1, shortOtherGuidStr.c_str(), shortOtherGuidStr.size()))) ||
            (!newOwner.empty() && (0 != ::strncmp(newOwner.c_str() + 1, shortOtherGuidStr.c_str(), shortOtherGuidStr.size())))) {
            ReleaseLocks();
            return;
        }
    }
    ReleaseLocks();

    if (alias[0] == ':') {
        AcquireLocks();
        map<qcc::StringMapKey, RemoteEndpoint>::iterator bit = b2bEndpoints.find(msg->GetRcvEndpointName());
        if (bit != b2bEndpoints.end()) {
            /* Change affects a remote unique name (i.e. a VirtualEndpoint) */
            if (newOwner.empty()) {
                VirtualEndpoint vep = FindVirtualEndpoint(oldOwner.c_str());
                if (vep->IsValid()) {
                    madeChanges = vep->CanUseRoute(bit->second);
                    /* Note: If IsStopping() is true, then there is another thread that is in the process
                     * of deleting this virtual endpoint. In this case, skip this virtual endpoint. */
                    if (madeChanges && !vep->IsStopping()) {
                        if (vep->RemoveBusToBusEndpoint(bit->second)) {
                            /* The last b2b endpoint was removed from this vep. */
                            String vepName = vep->GetUniqueName();
                            ReleaseLocks();
                            RemoveVirtualEndpoint(vepName);
                        } else {
                            /* Need to hit NameTable here since name ownership of a vep alias may have changed */
                            String vepName = vep->GetUniqueName();
                            ReleaseLocks();
                            router.UpdateVirtualAliases(vepName);
                        }
                    } else {
                        ReleaseLocks();
                    }
                } else {
                    ReleaseLocks();
                }
            } else {
                /* Add a new virtual endpoint */
                if (bit != b2bEndpoints.end()) {
                    String b2bEpName = bit->second->GetUniqueName();
                    ReleaseLocks();
                    AddVirtualEndpoint(alias, b2bEpName, &madeChanges);
                } else {
                    ReleaseLocks();
                }
            }
        } else {
            ReleaseLocks();
            QCC_LogError(ER_BUS_NO_ENDPOINT, ("Cannot find bus-to-bus endpoint %s", msg->GetRcvEndpointName()));
        }
    } else {
        AcquireLocks();
        /* Change affects a well-known name (name table only) */
        VirtualEndpoint remoteController = FindVirtualEndpoint(msg->GetSender());
        if (remoteController->IsValid()) {
            ReleaseLocks();
            if (newOwner.empty()) {
                madeChanges = router.SetVirtualAlias(alias, NULL, remoteController);
            } else {
                VirtualEndpoint newOwnerEp = FindVirtualEndpoint(newOwner.c_str());
                madeChanges = router.SetVirtualAlias(alias, &newOwnerEp, remoteController);
            }
            AcquireLocks();
        } else {
            QCC_LogError(ER_BUS_NO_ENDPOINT, ("Cannot find virtual endpoint %s", msg->GetSender()));
        }
        ReleaseLocks();
    }

    if (madeChanges) {
        /* Forward message to all directly connected controllers except the one that sent us this NameChanged */
        AcquireLocks();
        map<qcc::StringMapKey, RemoteEndpoint>::const_iterator bit = b2bEndpoints.find(msg->GetRcvEndpointName());
        map<qcc::StringMapKey, RemoteEndpoint>::iterator it = b2bEndpoints.begin();
        while (it != b2bEndpoints.end()) {
            if ((it->second->GetFeatures().nameTransfer == SessionOpts::ALL_NAMES) && ((bit == b2bEndpoints.end()) || (bit->second->GetRemoteGUID() != it->second->GetRemoteGUID()))) {
                String key = it->first.c_str();
                RemoteEndpoint ep = it->second;
                ReleaseLocks();
                QStatus status = ep->PushMessage(msg);
                if (ER_OK != status) {
                    QCC_LogError(status, ("Failed to forward NameChanged to %s", ep->GetUniqueName().c_str()));
                }
                AcquireLocks();
                bit = b2bEndpoints.find(msg->GetRcvEndpointName());
                it = b2bEndpoints.lower_bound(key);
                if ((it != b2bEndpoints.end()) && (it->first == key)) {
                    ++it;
                }
            } else {
                ++it;
            }
        }
        ReleaseLocks();
    }
}

void AllJoynObj::AddVirtualEndpoint(const qcc::String& uniqueName, const String& b2bEpName, bool* wasAdded)
{
    QCC_DbgTrace(("AllJoynObj::AddVirtualEndpoint(name=%s, b2b=%s)", uniqueName.c_str(), b2bEpName.c_str()));

    bool added = false;

    AcquireLocks();
    BusEndpoint tempEp = router.FindEndpoint(b2bEpName);
    RemoteEndpoint busToBusEndpoint = RemoteEndpoint::cast(tempEp);

    map<qcc::String, VirtualEndpoint>::iterator it = virtualEndpoints.find(uniqueName);

    /* If there is a VirtualEndpoint with the desired unique name in the virtualEndpoints map
     * and its state is EP_STOPPING, it means that another thread is trying to remove this
     * VirtualEndpoint (As a part of cleanup in AllJoynObj::RemoveBusToBusEndpoint or
     * AllJoynObj::NameChangedSignalHandler.)
     * In that case, wait for that thread to finish removing this virtual endpoint.
     * Also, if the busToBusEndpoint becomes invalid, we just return.
     */
    while (busToBusEndpoint->IsValid() && it != virtualEndpoints.end() && it->second->IsStopping()) {
        ReleaseLocks();
        qcc::Sleep(10);
        AcquireLocks();
        it = virtualEndpoints.find(uniqueName);
    }

    if (busToBusEndpoint->IsValid()) {
        VirtualEndpoint vep;
        if (it == virtualEndpoints.end()) {
            vep = VirtualEndpoint(uniqueName, busToBusEndpoint);
            /* Add new virtual endpoint */
            virtualEndpoints.insert(pair<qcc::String, VirtualEndpoint>(uniqueName, vep));
            added = true;
            /* Register the endpoint with the router */
            ReleaseLocks();
            BusEndpoint busEndpoint = BusEndpoint::cast(vep);
            router.RegisterEndpoint(busEndpoint);

        } else {
            /* Add the busToBus endpoint to the existing virtual endpoint */
            vep = it->second;
            added = vep->AddBusToBusEndpoint(busToBusEndpoint);
            ReleaseLocks();
        }
    } else {
        ReleaseLocks();
    }

    if (wasAdded) {
        *wasAdded = added;
    }
}

void AllJoynObj::RemoveVirtualEndpoint(const String& vepName)
{
    QCC_DbgTrace(("RemoveVirtualEndpoint: %s", vepName.c_str()));

    /* Remove virtual endpoint along with any aliases that exist for this uniqueName */
    router.RemoveVirtualAliases(vepName);
    router.UnregisterEndpoint(vepName, ENDPOINT_TYPE_VIRTUAL);
    AcquireLocks();
    map<qcc::String, VirtualEndpoint>::iterator it = virtualEndpoints.find(vepName);
    if (it != virtualEndpoints.end()) {
        VirtualEndpoint vep = it->second;
        virtualEndpoints.erase(it);
        ReleaseLocks();
    } else {
        ReleaseLocks();
    }
}

VirtualEndpoint AllJoynObj::FindVirtualEndpoint(const qcc::String& uniqueName)
{
    VirtualEndpoint ret;
    AcquireLocks();
    map<qcc::String, VirtualEndpoint>::iterator it = virtualEndpoints.find(uniqueName);
    if (it != virtualEndpoints.end()) {
        ret = it->second;
    }
    ReleaseLocks();
    return ret;
}

void AllJoynObj::NameOwnerChanged(const qcc::String& alias,
                                  const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                                  const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    QStatus status;
    const String& shortGuidStr = guid.ToShortString();

    /* When newOwner and oldOwner are the same, only the name transfer changed. */
    if (newOwner == oldOwner) {
        return;
    }

    /* Validate that there is either a new owner or an old owner */
    const qcc::String* un = oldOwner ? oldOwner : newOwner;
    if (NULL == un) {
        QCC_LogError(ER_BUS_NO_ENDPOINT, ("Invalid NameOwnerChanged without oldOwner or newOwner"));
        return;
    }

    /* Validate format of unique name */
    size_t guidLen = un->find_first_of('.');
    if ((qcc::String::npos == guidLen) || (guidLen < 3)) {
        QCC_LogError(ER_FAIL, ("Invalid unique name \"%s\"", un->c_str()));
    }

    /* Ignore well-known name changes that involve any bus controller endpoint */
    if ((::strcmp(un->c_str() + guidLen, ".1") == 0) && (alias[0] != ':')) {
        return;
    }

    /* Remove unique names from sessionMap entries */
    if (!newOwner && (alias[0] == ':')) {
        AcquireLocks();
        vector<pair<String, SessionId> > changedSessionMembers;
        vector<SessionMapEntry> sessionsLost;
        SessionMapType::iterator it = sessionMap.begin();
        while (it != sessionMap.end()) {
            if (it->first.first == alias) {
                /* If endpoint has gone then just delete the session map entry */
                sessionMap.erase(it++);
            } else if (it->first.second != 0) {
                /* Remove member entries from existing sessions */
                if (it->second.sessionHost == alias) {
                    if (it->second.opts.isMultipoint) {
                        changedSessionMembers.push_back(it->first);
                    }
                    it->second.sessionHost.clear();

                    /* Check for self-joined member */
                    vector<String>::iterator mit = it->second.memberNames.begin();
                    while (mit != it->second.memberNames.end()) {
                        if (*mit == alias) {
                            it->second.memberNames.erase(mit);
                            break;
                        }
                        ++mit;
                    }

                } else {
                    vector<String>::iterator mit = it->second.memberNames.begin();
                    while (mit != it->second.memberNames.end()) {
                        if (*mit == alias) {
                            it->second.memberNames.erase(mit);
                            if (it->second.opts.isMultipoint) {
                                changedSessionMembers.push_back(it->first);
                            }
                            break;
                        }
                        ++mit;
                    }
                }
                /*
                 * Remove empty session entry.
                 * Preserve raw sessions until GetSessionFd is called.
                 */
                /*
                 * If the session is point-to-point and the memberNames are empty.
                 * if the sessionHost is not empty (implied) and there are no member names send
                 * the  sessionLost signal as long as the session is not a raw session
                 */
                bool noMemberSingleHost = it->second.memberNames.empty();
                /*
                 * If the session is a Multipoint session it will list its own unique
                 * name in the list of memberNames. If There is only one name in the
                 * memberNames list and there is no session host it is safe to send
                 * the session lost signal as long as the session does not contain a
                 * raw session.
                 */
                bool singleMemberNoHost = ((it->second.memberNames.size() == 1) && it->second.sessionHost.empty());
                /*
                 * as long as the file descriptor is -1 this is not a raw session
                 */
                bool noRawSession = (it->second.fd == qcc::INVALID_SOCKET_FD);
                if ((noMemberSingleHost || singleMemberNoHost) && noRawSession) {
                    SessionMapEntry tsme = it->second;
                    pair<String, SessionId> key = it->first;
                    if (!it->second.isInitializing) {
                        sessionMap.erase(it++);
                    } else {
                        ++it;
                    }
                    sessionsLost.push_back(tsme);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
        ReleaseLocks();

        /* Send MPSessionChanged for each changed session involving alias */
        vector<pair<String, SessionId> >::const_iterator csit = changedSessionMembers.begin();
        while (csit != changedSessionMembers.end()) {
            SendMPSessionChanged(csit->second, alias.c_str(), false, csit->first.c_str(), ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_REMOVED);
            csit++;
        }
        /* Send session lost signals */
        vector<SessionMapEntry>::iterator slit = sessionsLost.begin();
        while (slit != sessionsLost.end()) {
            if (slit->memberNames.size() == 1) {
                SendSessionLost(*slit++, ER_BUS_ENDPOINT_CLOSING, ALLJOYN_SESSIONLOST_DISPOSITION_MEMBER);
            } else {
                SendSessionLost(*slit++, ER_BUS_ENDPOINT_CLOSING, ALLJOYN_SESSIONLOST_DISPOSITION_HOST);
            }
        }
    }

    /* Only if local name */
    if (0 == ::strncmp(shortGuidStr.c_str(), un->c_str() + 1, shortGuidStr.size())) {

        /* Send NameChanged to all directly connected controllers */
        AcquireLocks();
        map<qcc::StringMapKey, RemoteEndpoint>::iterator it = b2bEndpoints.begin();
        while (it != b2bEndpoints.end()) {
            Message sigMsg(bus);
            MsgArg args[3];
            args[0].Set("s", alias.c_str());
            args[1].Set("s", oldOwner ? oldOwner->c_str() : "");
            args[2].Set("s", newOwner ? newOwner->c_str() : "");

            status = sigMsg->SignalMsg("sss",
                                       org::alljoyn::Daemon::WellKnownName,
                                       0,
                                       org::alljoyn::Daemon::ObjectPath,
                                       org::alljoyn::Daemon::InterfaceName,
                                       "NameChanged",
                                       args,
                                       ArraySize(args),
                                       0,
                                       0);
            if (ER_OK == status) {
                StringMapKey key = it->first;
                RemoteEndpoint ep = it->second;
                ReleaseLocks();
                status = ep->PushMessage(sigMsg);
                AcquireLocks();
                it = b2bEndpoints.lower_bound(key);
                if ((it != b2bEndpoints.end()) && (it->first == key)) {
                    ++it;
                }
            } else {
                ++it;
            }
            // if the endpoint is closing we don't don't expect the NameChanged signal to send
            if (ER_OK != status && ER_BUS_ENDPOINT_CLOSING != status) {
                QCC_LogError(status, ("Failed to send NameChanged"));
            }
        }
        ReleaseLocks();

        /* If a local unique name dropped, then remove any refs it had in the connnect, advertise and discover maps */
        if ((NULL == newOwner) && (alias[0] == ':')) {
            /* Remove endpoint refs from connect map */
            qcc::String last;
            AcquireLocks();
            multimap<qcc::String, qcc::String>::iterator it = connectMap.begin();
            while (it != connectMap.end()) {
                if (it->second == *oldOwner) {
                    bool isFirstSpec = (last != it->first);
                    qcc::String lastOwner;
                    do {
                        last = it->first;
                        connectMap.erase(it++);
                    } while ((connectMap.end() != it) && (last == it->first) && (*oldOwner == it->second));
                    if (isFirstSpec && ((connectMap.end() == it) || (last != it->first))) {
                        QStatus status = bus.Disconnect(last.c_str());
                        if (ER_OK != status) {
                            QCC_LogError(status, ("Failed to disconnect connect spec %s", last.c_str()));
                        }
                    }
                } else {
                    last = it->first;
                    ++it;
                }
            }

            /* Remove endpoint refs from advertise map */
            multimap<String, pair<TransportMask, String> >::const_iterator ait = advertiseMap.begin();
            while (ait != advertiseMap.end()) {
                if (ait->second.second == *oldOwner) {
                    String name = ait->first;
                    TransportMask mask = ait->second.first;
                    ReleaseLocks();

                    QStatus status = ProcCancelAdvertise(*oldOwner, name, mask);
                    AcquireLocks();
                    ait = advertiseMap.upper_bound(name);
                    if (ER_OK != status) {
                        QCC_LogError(status, ("Failed to cancel advertise for name \"%s\"", name.c_str()));
                    }
                } else {
                    ++ait;
                }
            }

            /* Remove endpoint refs from discover map */
            DiscoverMapType::const_iterator dit = discoverMap.begin();
            while (dit != discoverMap.end()) {
                if (dit->second.sender == *oldOwner) {
                    last = dit->first;
                    TransportMask mask = dit->second.transportMask;

                    QCC_DbgPrintf(("Calling ProcCancelFindAdvertisement from NameOwnerChanged [%s]", Thread::GetThread()->GetName()));
                    ReleaseLocks();

                    QStatus status = ProcCancelFindAdvertisement(*oldOwner, last, mask);
                    AcquireLocks();
                    dit = discoverMap.upper_bound(last);
                    if (ER_OK != status) {
                        QCC_LogError(status, ("Failed to cancel discover for name \"%s\"", last.c_str()));
                    }
                } else {
                    ++dit;
                }
            }
            ReleaseLocks();
        }
    }
}

struct FoundNameEntry {
  public:
    String name;
    String prefix;
    String dest;
    FoundNameEntry(const String& name, const String& prefix, const String& dest) : name(name), prefix(prefix), dest(dest) {
    }
    bool operator<(const FoundNameEntry& other) const {
        return (name < other.name) || ((name == other.name) && ((prefix < other.prefix) || ((prefix == other.prefix) && (dest < other.dest))));
    }
};

void AllJoynObj::FoundNames(const qcc::String& busAddr,
                            const qcc::String& guid,
                            TransportMask transport,
                            const vector<qcc::String>* names,
                            uint32_t ttl)
{
    QCC_DbgTrace(("AllJoynObj::FoundNames(busAddr = \"%s\", guid = \"%s\", names = %s, ttl = %d)",
                  busAddr.c_str(), guid.c_str(), StringVectorToString(names, ",").c_str(), ttl));

    if (NULL == foundNameSignal) {
        return;
    }
    set<FoundNameEntry> foundNameSet;
    set<String> lostNameSet;
    AcquireLocks();
    if (names == NULL) {
        /* If name is NULL expire all names for the given bus address. */
        if (ttl == 0) {
            multimap<String, NameMapEntry>::iterator it = nameMap.begin();
            while (it != nameMap.end()) {
                NameMapEntry& nme = it->second;
                if ((nme.guid == guid) && (nme.busAddr == busAddr)) {
                    lostNameSet.insert(it->first);
                    timer.RemoveAlarm(nme.alarm, false);
                    nameMap.erase(it++);
                } else {
                    it++;
                }
            }
        }
    } else {
        /* Generate a list of name deltas */
        vector<String>::const_iterator nit = names->begin();
        while (nit != names->end()) {
            multimap<String, NameMapEntry>::iterator it = nameMap.find(*nit);
            bool sendSignal = true;
            bool isNew = true;
            /* Send a FoundAdvertisedName signal if this is the first namemap entry of this transport for this name. */
            /* Send a LostAdvertisedName signal if this is the last namemap entry of this transport for this name. */
            while ((it != nameMap.end()) && (*nit == it->first)) {
                if ((it->second.guid == guid) && (it->second.transport & transport) && (busAddr != it->second.busAddr)) {
                    sendSignal = false;
                    break;
                }
                it++;
            }

            it = nameMap.find(*nit);
            while ((it != nameMap.end()) && (*nit == it->first)) {
                if ((it->second.guid == guid) && (it->second.transport & transport) && (busAddr == it->second.busAddr)) {
                    isNew = false;
                    break;

                }
                ++it;
            }

            if (0 < ttl) {

                if (isNew) {
                    QCC_DbgPrintf(("Adding new entry %d  %d", (1000LL * ttl), (1000LL * ttl * 80 / 100)));
                    /* Add new name to map */
                    NameMapType::iterator it = nameMap.insert(NameMapType::value_type(*nit, NameMapEntry(busAddr,
                                                                                                         guid,
                                                                                                         transport,
                                                                                                         (ttl == numeric_limits<uint32_t>::max()) ? numeric_limits<uint64_t>::max() : (1000LL * ttl),
                                                                                                         this)));
                    QCC_DbgPrintf(("TTL set to %ld", it->second.ttl));
                    /* Don't schedule an alarm which will never expire or multiple timers for the same set */
                    if (ttl != numeric_limits<uint32_t>::max()) {
                        NameMapEntry& nme = it->second;
                        // We need the alarm to be triggered off at 80% time to enable cache refresh
                        const uint32_t timeout = ttl * 1000 * 80 / 100;
                        AllJoynObj* pObj = this;
                        Alarm newAlarm(timeout, pObj, NameMapEntry::truthiness);
                        nme.alarm = newAlarm;
                        QStatus status = timer.AddAlarm(nme.alarm);
                        if (ER_OK != status && ER_TIMER_EXITING != status) {
                            QCC_LogError(status, ("Failed to add alarm"));
                        }
                    }
                    /* Send FoundAdvertisedName to anyone who is discovering *nit */
                    if (sendSignal) {
                        if (0 < discoverMap.size()) {
                            for (DiscoverMapType::const_iterator dit = discoverMap.begin(); dit != discoverMap.end(); ++dit) {
                                MatchMap::const_iterator namePrefix = dit->second.matching.find("name");
                                if (namePrefix == dit->second.matching.end()) {
                                    continue;
                                }

                                if (!dit->second.initComplete) {
                                    continue;
                                }

                                if (!WildcardMatch(*nit, namePrefix->second) && (transport & dit->second.transportMask)) {
                                    foundNameSet.insert(FoundNameEntry(*nit, namePrefix->second, dit->second.sender));
                                }
                            }
                        }
                    }
                } else {
                    /*
                     * If the busAddr doesn't match, then this is actually a new but redundant advertisement.
                     * Don't track it. Don't updated the TTL for the existing advertisement with the same name
                     * and don't tell clients about this alternate way to connect to the name
                     * since it will look like a duplicate to the client (that doesn't receive busAddr).
                     */
                    if (busAddr == it->second.busAddr) {
                        NameMapEntry& nme = it->second;
                        nme.timestamp = GetTimestamp64();
                        /* need to move the alarm ttl seconds into the future. */
                        const uint32_t timeout = ttl * 1000 * 80 / 100;
                        AllJoynObj* pObj = this;
                        Alarm newAlarm(timeout, pObj, NameMapEntry::truthiness);
                        QStatus status = timer.ReplaceAlarm(nme.alarm, newAlarm, false);
                        nme.alarm = newAlarm;
                        QCC_DbgPrintf(("Resetting alarm for name %s to %d", it->first.c_str(), timeout));
                        if (ER_OK != status) {
                            /* This is expected if a prior name set changed in any way (order, removed entry, etc) */
                            status = timer.AddAlarm(nme.alarm);
                            if (ER_OK != status && ER_TIMER_EXITING != status) {
                                QCC_LogError(status, ("Failed to update alarm"));
                            }
                        }
                    }
                }
            } else {

                /* 0 == ttl means flush the record */
                if (!isNew) {
                    NameMapEntry& nme = it->second;
                    qcc::String guidToBeChecked = it->second.guid;
                    if (sendSignal) {
                        lostNameSet.insert(it->first);
                    }
                    timer.RemoveAlarm(nme.alarm, false);
                    nameMap.erase(it);
                    //
                    // Check if guid for this name is eligible for removal from PeerInfoMap in Name service
                    //
                    if (!IsGuidLongStringKnown(guidToBeChecked)) {
                        QCC_DbgPrintf(("TTl=0. Removing GUID %s", guidToBeChecked.c_str()));
                        IpNameService::Instance().RemoveFromPeerInfoMap(guidToBeChecked);
                    }
                }
            }
            ++nit;
        }
    }
    ReleaseLocks();

    /* Send FoundAdvertisedName signals without holding locks */
    set<FoundNameEntry>::const_iterator fit = foundNameSet.begin();
    while (fit != foundNameSet.end()) {
        QStatus status = SendFoundAdvertisedName(fit->dest, fit->name, transport, fit->prefix);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to send FoundAdvertisedName to %s (name=%s)", fit->dest.c_str(), fit->name.c_str()));
        }
        ++fit;
    }

    set<String>::const_iterator lit = lostNameSet.begin();
    while (lit != lostNameSet.end()) {
        /* Send LostAdvetisedName signals */
        SendLostAdvertisedName(*lit, transport);
        /* Clean advAliasMap */
        CleanAdvAliasMap(*lit, transport);
        lit++;
    }
}

bool AllJoynObj::IsGuidShortStringKnown(qcc::String& guid)
{
    //
    //Check if there any other name in the NameMap from this guid
    //
    AcquireLocks();
    std::multimap<qcc::String, NameMapEntry>::iterator it = nameMap.begin();
    while (it != nameMap.end()) {
        if (qcc::GUID128(it->second.guid).ToShortString() == guid) {
            ReleaseLocks();
            return true;
        }
        ++it;
    }
    //
    //Check if there is any active session with this guid
    //
    BusEndpoint bep = router.FindEndpoint(":" + guid + ".1");
    if (bep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
        QCC_DbgPrintf(("Session found for %s", guid.c_str()));
        ReleaseLocks();
        return true;
    } else {
        QCC_DbgPrintf(("EndpoinType = %d,  Session not found for %s", bep->GetEndpointType(), guid.c_str()));
    }

    ReleaseLocks();
    return false;
}


bool AllJoynObj::IsGuidLongStringKnown(qcc::String& guid)
{
    //
    //Check if there any other name in the NameMap from this guid
    //
    AcquireLocks();
    std::multimap<qcc::String, NameMapEntry>::iterator it = nameMap.begin();
    while (it != nameMap.end()) {
        if (it->second.guid == guid) {
            ReleaseLocks();
            return true;
        }
        ++it;
    }
    //
    //Check if there is any active session with this guid
    //
    GUID128 wellFormedBusName(guid);
    QCC_DbgPrintf(("wellFormedBusName.ToShortString()(%s)", wellFormedBusName.ToShortString().c_str()));
    BusEndpoint bep = router.FindEndpoint(":" + wellFormedBusName.ToShortString() + ".1");
    if (bep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
        QCC_DbgPrintf(("Session found ", wellFormedBusName.ToShortString().c_str()));
        ReleaseLocks();
        return true;
    } else {
        QCC_DbgPrintf(("EndpoinType = %d,  Session not found for %s", bep->GetEndpointType(), wellFormedBusName.ToShortString().c_str()));
    }

    ReleaseLocks();
    return false;
}

void AllJoynObj::CleanAdvAliasMap(const String& name, const TransportMask mask)
{
    QCC_DbgTrace(("AllJoynObj::CleanAdvAliasMap(%s, 0x%x): size=%d", name.c_str(), mask, advAliasMap.size()));

    /* Clean advAliasMap */
    AcquireLocks();
    map<String, set<AdvAliasEntry> >::iterator ait = advAliasMap.begin();
    while (ait != advAliasMap.end()) {
        set<AdvAliasEntry>::iterator bit = ait->second.begin();
        while (bit != ait->second.end()) {
            if (((*bit).name == name) && (((*bit).transport & mask) != 0)) {
                ait->second.erase(bit++);
            } else {
                ++bit;
            }
        }
        if (ait->second.size() == 0) {
            advAliasMap.erase(ait++);
        } else {
            ++ait;
        }
    }
    ReleaseLocks();
}

QStatus AllJoynObj::SendFoundAdvertisedName(const String& dest,
                                            const String& name,
                                            TransportMask transport,
                                            const String& namePrefix)
{
    QCC_DbgTrace(("AllJoynObj::SendFoundAdvertisedName(%s, %s, 0x%x, %s)", dest.c_str(), name.c_str(), transport, namePrefix.c_str()));

    MsgArg args[3];
    args[0].Set("s", name.c_str());
    args[1].Set("q", transport);
    String prefix = namePrefix.substr(0, namePrefix.find_last_of('*'));
    args[2].Set("s", prefix.c_str());
    return Signal(dest.c_str(), 0, *foundNameSignal, args, ArraySize(args));
}

QStatus AllJoynObj::SendLostAdvertisedName(const String& name, TransportMask transport)
{
    QCC_DbgTrace(("AllJoynObj::SendLostAdvertisdName(%s, 0x%x)", name.c_str(), transport));

    QStatus status = ER_OK;

    /* Send LostAdvertisedName to anyone who is discovering name */
    AcquireLocks();
    vector<pair<String, String> > sigVec;
    if (0 < discoverMap.size()) {
        for (DiscoverMapType::const_iterator dit = discoverMap.begin(); dit != discoverMap.end(); ++dit) {
            MatchMap::const_iterator namePrefix = dit->second.matching.find("name");
            if (namePrefix == dit->second.matching.end()) {
                continue;
            }
            if (!WildcardMatch(name, namePrefix->second) && (dit->second.transportMask & transport)) {
                sigVec.push_back(pair<String, String>(namePrefix->second, dit->second.sender));
            }
        }
    }
    ReleaseLocks();

    /* Send the signals now that we aren't holding the lock */
    vector<pair<String, String> >::const_iterator it = sigVec.begin();
    while (it != sigVec.end()) {
        MsgArg args[3];
        args[0].Set("s", name.c_str());
        args[1].Set("q", transport);
        String prefix = it->first.substr(0, it->first.find_last_of('*'));
        args[2].Set("s", prefix.c_str());
        QCC_DbgPrintf(("Sending LostAdvertisedName(%s, 0x%x, %s) to %s", name.c_str(), transport, prefix.c_str(), it->second.c_str()));
        QStatus tStatus = Signal(it->second.c_str(), 0, *lostAdvNameSignal, args, ArraySize(args));
        if (ER_OK != tStatus) {
            status = (ER_OK == status) ? tStatus : status;
            if (status != ER_BUS_NO_ROUTE) {
                QCC_LogError(tStatus, ("Failed to send LostAdvertisedName to %s (name=%s)", it->second.c_str(), name.c_str()));
            }
        }
        ++it;
    }
    return status;
}
void AllJoynObj::SendIPNSResponse(String name, uint32_t replyCode) {
    AcquireLocks();
    multimap<String, IncomingPingInfo>::iterator it = incomingPingMap.lower_bound(name);
    list<IncomingPingInfo> temp;
    while (it != incomingPingMap.end() && it->first == name) {
        temp.push_back(it->second);
        incomingPingMap.erase(it++);
    }
    ReleaseLocks();
    list<IncomingPingInfo>::iterator it1 = temp.begin();
    while (it1 != temp.end()) {
        PingResponse((*it1).transport, (*it1).ns4, name, replyCode);
        it1++;
    }

}

void AllJoynObj::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgPrintf(("AlarmTriggered"));
    if (alarm->GetContext() != NameMapEntry::truthiness) {
        assert(alarm->GetContext());

        PingAlarmContext* ctx = static_cast<PingAlarmContext*>(alarm->GetContext());
        if (ctx->type == PingAlarmContext::TRANSPORT_CONTEXT) {
            if (ctx->name[0] == ':' && !router.IsValidLocalUniqueName(ctx->name)) {
                SendIPNSResponse(ctx->name, ALLJOYN_PING_REPLY_UNKNOWN_NAME);
                delete ctx;
                return;
            }
            ProxyBusObject peerObj(bus, ctx->name.c_str(), "/", 0);
            const InterfaceDescription* intf = bus.GetInterface(org::freedesktop::DBus::Peer::InterfaceName);
            assert(intf);
            peerObj.AddInterface(*intf);

            QStatus status = peerObj.MethodCallAsync(org::freedesktop::DBus::Peer::InterfaceName,
                                                     "Ping",
                                                     this, static_cast<MessageReceiver::ReplyHandler>(&AllJoynObj::PingReplyTransportHandler),
                                                     NULL, 0,
                                                     ctx);
            if (status != ER_OK) {
                /*
                 * UNREACHABLE may not be the correct reply code here, a failure
                 * status indicates only that setting up the async method call
                 * failed.
                 */
                SendIPNSResponse(ctx->name, ALLJOYN_PING_REPLY_UNREACHABLE);
                delete ctx;
            }
        } else {

            //REPLY_CONTEXT
            AcquireLocks();
            multimap<pair<String, String>, OutgoingPingInfo>::iterator it = outgoingPingMap.find(pair<String, String>(ctx->name, ctx->sender));
            if (it != outgoingPingMap.end()) {
                OutgoingPingInfo opi = it->second;
                outgoingPingMap.erase(it);
                ReleaseLocks();
                PingReplyMethodHandlerUsingCode(opi.message, ALLJOYN_PING_REPLY_TIMEOUT);
            } else {

                ReleaseLocks();
            }
            delete ctx;
        }
        return;
    }
    //
    // Check if TTL is 80% of time or 90% of time
    // If 80%
    //    Initiate a search query for "*"
    //    Set the alarm to 90% of time
    // If 90%
    //    Initiate a search query for "*"
    //    Set the alarm for full TTL time out

    AcquireLocks();
    uint64_t now = GetTimestamp64();
    uint64_t timePassed;
    uint64_t ttl;
    multimap<String, NameMapEntry>::iterator it = nameMap.begin();
    set<String> guidSet;
    while (it != nameMap.end()) {
        NameMapEntry& nme = it->second;
        timePassed = now - nme.timestamp;
        ttl = nme.ttl;
        QCC_DbgPrintf(("Time Passed %ld ttl-80 : %ld ttl-90 : %ld", timePassed, (uint64_t)(ttl * 80 / 100), (uint64_t)(ttl * 90 / 100)));
        if ((timePassed >= (ttl * 80 / 100)) &&
            (timePassed < ttl)) {
            if (discoverMap.size() > 0) {
                //
                // Send Unicast search query only if there are discoverers
                //
                QCC_DbgPrintf(("AlarmTriggered sending query \"*\" Name : %s GUID : %s nme.transport %x", it->first.c_str(), it->second.guid.c_str(), nme.transport));
                guidSet.insert(nme.guid);
            }
        }

        if ((timePassed >= (ttl * 80 / 100)) &&
            (timePassed < (ttl * 90 / 100))) {
            //
            // Set the next alarm of this entry to 90% of ttl
            //
            const uint32_t timeout = (ttl * 90 / 100) - timePassed;
            QCC_DbgPrintf(("AlarmTriggered for 80 time for Name : %s GUID : %s Next alarm in %d", it->first.c_str(), it->second.guid.c_str(), timeout));
            AllJoynObj* pObj = this;
            Alarm newAlarm(timeout, pObj, NameMapEntry::truthiness);
            QStatus status = timer.ReplaceAlarm(nme.alarm, newAlarm, false);
            nme.alarm = newAlarm;
            QCC_DbgPrintf(("Resetting alarm for name %s", it->first.c_str()));
            if (ER_OK != status) {
                /* This is expected if a prior name set changed in any way (order, removed entry, etc) */
                status = timer.AddAlarm(nme.alarm);
                if (ER_OK != status && ER_TIMER_EXITING != status) {
                    QCC_LogError(status, ("Failed to update alarm"));
                }
            }
        }

        if ((timePassed >= (ttl * 90 / 100)) &&
            (timePassed < ttl)) {
            //
            // Set the next alarm to TTL - timePassed
            //
            const uint32_t timeout = (ttl) - timePassed;
            QCC_DbgPrintf(("AlarmTriggered for 90 time for Name : %s GUID : %s Next alarm in %ld", it->first.c_str(), it->second.guid.c_str(), timeout));
            AllJoynObj* pObj = this;
            Alarm newAlarm(timeout, pObj, NameMapEntry::truthiness);
            QStatus status = timer.ReplaceAlarm(nme.alarm, newAlarm, false);
            nme.alarm = newAlarm;
            QCC_DbgPrintf(("Resetting alarm for name %s", it->first.c_str()));
            if (ER_OK != status) {
                /* This is expected if a prior name set changed in any way (order, removed entry, etc) */
                status = timer.AddAlarm(nme.alarm);
                if (ER_OK != status && ER_TIMER_EXITING != status) {
                    QCC_LogError(status, ("Failed to update alarm"));
                }
            }
        }
        ++it;
    }
    ReleaseLocks();
    set<String>::const_iterator git = guidSet.begin();
    while (git != guidSet.end()) {

        QStatus status = IpNameService::Instance().RefreshCache(TRANSPORT_TCP | TRANSPORT_UDP, *git, "name='*'");
        if (ER_OK != status) {
            QCC_LogError(status, ("Error while sending query for Cache refresh"));
        }
        git++;
    }

    // if 100* of time
    //     do all the things below
    //     AND if not in a session
    //         Remove from PeerInfoMap
    //
    if (ER_OK == reason) {
        set<pair<String, TransportMask> > lostNameSet;
        AcquireLocks();
        if ((bool)alarm->GetContext()) {
            multimap<String, NameMapEntry>::iterator it = nameMap.begin();
            uint64_t now = GetTimestamp64();
            while (it != nameMap.end()) {
                NameMapEntry& nme = it->second;
                qcc::String guidToBeChecked = it->second.guid;
                if ((now - nme.timestamp) >= nme.ttl) {
                    bool sendSignal = true;
                    multimap<String, NameMapEntry>::iterator it1 = nameMap.find(it->first);
                    /* Send a LostAdvertisedName signal if this is the last namemap entry of this transport for this name. */
                    while ((it1 != nameMap.end()) && (it->first == it1->first)) {
                        if ((nme.guid == it1->second.guid) && (nme.transport & it1->second.transport) && (nme.busAddr != it1->second.busAddr)) {
                            sendSignal = false;
                            break;
                        }
                        it1++;
                    }
                    if (sendSignal) {
                        QCC_DbgPrintf(("Expiring discovered name %s for guid %s", it->first.c_str(), nme.guid.c_str()));
                        lostNameSet.insert(pair<String, TransportMask>(it->first, nme.transport));
                    }
                    /* Remove alarm */
                    timer.RemoveAlarm(nme.alarm, false);
                    nme.alarm->SetContext((void*)false);
                    nameMap.erase(it++);
                    //
                    // Check if guid for this name is eligible for removal from PeerInfoMap in Name service
                    //
                    if (!IsGuidLongStringKnown(guidToBeChecked)) {
                        IpNameService::Instance().RemoveFromPeerInfoMap(guidToBeChecked);
                    }
                } else {
                    ++it;
                }
            }
        }
        ReleaseLocks();
        set<pair<String, TransportMask> >::const_iterator lit = lostNameSet.begin();
        while (lit != lostNameSet.end()) {
            /* Send LostAdvetisedName signals */
            SendLostAdvertisedName(lit->first, lit->second);
            /* Clean advAliasMap */
            CleanAdvAliasMap(lit->first, lit->second);
            lit++;
        }
    }
}

void AllJoynObj::CancelSessionlessMessage(const InterfaceDescription::Member* member, Message& msg)
{
    size_t numArgs;
    const MsgArg* args;
    msg->GetArgs(numArgs, args);

    uint32_t serialNum = args[0].v_uint32;
    qcc::String sender = msg->GetSender();

    SessionlessObj& sessionlessObj = busController->GetSessionlessObj();
    QStatus status = sessionlessObj.CancelMessage(sender, serialNum);
    if (status != ER_OK) {
        QCC_LogError(status, ("SessionlessObj::CancelMessage failed"));
    }

    /* Form response and send it */
    MsgArg replyArg;
    uint32_t replyCode;
    switch (status) {
    case ER_OK:
        replyCode = ALLJOYN_CANCELSESSIONLESS_REPLY_SUCCESS;
        break;

    case ER_BUS_NO_SUCH_MESSAGE:
        replyCode = ALLJOYN_CANCELSESSIONLESS_REPLY_NO_SUCH_MSG;
        break;

    case ER_BUS_NOT_ALLOWED:
        replyCode = ALLJOYN_CANCELSESSIONLESS_REPLY_NOT_ALLOWED;
        break;

    default:
        replyCode = ALLJOYN_CANCELSESSIONLESS_REPLY_FAILED;
        break;
    }
    replyArg.Set("u", replyCode);
    status = MethodReply(msg, &replyArg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("AllJoynObj::CancelSessionlessMessage() failed to send reply message"));
    }
}

void AllJoynObj::BusConnectionLost(const qcc::String& busAddr)
{
    /* Clear the connection map of this busAddress */
    AcquireLocks();
    multimap<String, String>::iterator it = connectMap.lower_bound(busAddr);
    while ((it != connectMap.end()) && (0 == busAddr.compare(it->first))) {
        connectMap.erase(it++);
    }
    ReleaseLocks();
}

void AllJoynObj::Ping(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_DbgTrace(("AllJoynObj::Ping()"));

    uint32_t replyCode = ALLJOYN_PING_REPLY_SUCCESS;
    TransportMask transports = TRANSPORT_ANY;
    size_t numArgs;
    const MsgArg* args;
    String sender = msg->GetSender();
    BusEndpoint senderEp = router.FindEndpoint(sender);

    /* Parse the message args */
    msg->GetArgs(numArgs, args);
    const char* name = NULL;
    uint32_t timeout;
    QStatus status = MsgArg::Get(args, numArgs, "su", &name, &timeout);

    if (status == ER_OK && senderEp->IsValid()) {
        status = TransportPermission::FilterTransports(senderEp, sender, transports, "AllJoynObj::Ping");
    }
    if (status == ER_OK) {
        PermissionMgr::DaemonBusCallPolicy policy = PermissionMgr::GetDaemonBusCallPolicy(senderEp);
        bool rejectCall = false;
        if (policy == PermissionMgr::STDBUSCALL_SHOULD_REJECT) {
            rejectCall = true;
        } else if (policy == PermissionMgr::STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL) {
            transports &= TRANSPORT_LOCAL;
            QCC_DbgPrintf(("The sender endpoint is only allowed to use local transport."));
        }

        if (rejectCall) {
            QCC_DbgPrintf(("The sender endpoint is not allowed to call Ping()"));
            replyCode = ALLJOYN_PING_REPLY_FAILED;
            /* Reply to request */
            MsgArg replyArg("u", replyCode);
            status = MethodReply(msg, &replyArg, 1);
            QCC_DbgPrintf(("AllJoynObj::Ping(%s) returned %d (status=%s)", name, replyCode, QCC_StatusText(status)));
            return;
        }
    }

    if (status != ER_OK) {
        replyCode = ALLJOYN_PING_REPLY_FAILED;
        QCC_DbgTrace(("Ping(<bad_args>"));
    } else {
        assert(name);
        QCC_DbgTrace(("Ping(%s)", name));

        /* Decide how to proceed based on the endpoint existence/type */
        BusEndpoint ep = router.FindEndpoint(name);
        if ((ep->GetEndpointType() == ENDPOINT_TYPE_REMOTE) || (ep->GetEndpointType() == ENDPOINT_TYPE_NULL) || (ep->GetEndpointType() == ENDPOINT_TYPE_LOCAL) || (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
            AcquireLocks();
            if (dbusPingsInProgress.find(pair<String, String>(msg->GetSender(), name)) != dbusPingsInProgress.end()) {
                replyCode = ALLJOYN_PING_REPLY_IN_PROGRESS;
                ReleaseLocks();
            } else {

                dbusPingsInProgress.insert(pair<String, String>(msg->GetSender(), name));
                ReleaseLocks();
                /* Ping is to a locally connected or remote in session attachment */
                ProxyBusObject peerObj(bus, name, "/", 0);
                const InterfaceDescription* intf = bus.GetInterface(org::freedesktop::DBus::Peer::InterfaceName);
                assert(intf);
                peerObj.AddInterface(*intf);
                Message* ctx = new Message(msg);
                status = peerObj.MethodCallAsync(org::freedesktop::DBus::Peer::InterfaceName,
                                                 "Ping",
                                                 this, static_cast<MessageReceiver::ReplyHandler>(&AllJoynObj::PingReplyMethodHandler),
                                                 NULL, 0,
                                                 ctx);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Send Ping failed"));
                    delete ctx;
                    replyCode = ALLJOYN_PING_REPLY_UNREACHABLE;
                    AcquireLocks();
                    dbusPingsInProgress.erase(pair<String, String>(msg->GetSender(), name));
                    ReleaseLocks();
                }
            }
        } else {
            /* Ping is to a connected or unconnected remote device */

            /*
             * First order of business is to locate a guid corresponding to the name.
             * The logic below follows the same logic as joining a session.
             */

            // Check if the name is advertised
            TransportMask transport = TRANSPORT_TCP | TRANSPORT_UDP | TRANSPORT_LOCAL;                         // TODO transport hard-coded
            String guid;
            AcquireLocks();
            for (multimap<String, NameMapEntry>::iterator nmit = nameMap.lower_bound(name);
                 nmit != nameMap.end() && (nmit->first == name); ++nmit) {
                if (nmit->second.transport & transport) {
                    guid = qcc::GUID128(nmit->second.guid).ToShortString();
                    break;
                }
            }

            if (!ep->IsValid()) {

                if (name[0] != ':') {
                    // Well known name
                    if (guid.empty()) {
                        // No guid found for well known name
                        replyCode = ALLJOYN_PING_REPLY_UNKNOWN_NAME;
                    } else if (guid == bus.GetInternal().GetGlobalGUID().ToShortString()) {
                        // Locally advertised not requested
                        replyCode = ALLJOYN_PING_REPLY_UNREACHABLE;
                    }
                } else {
                    // Unique name
                    String nameStr(name);
                    String guidStr = nameStr.substr(1, GUID128::SHORT_SIZE);
                    if (guidStr == bus.GetInternal().GetGlobalGUID().ToShortString()) {
                        // Guid matches our guid but endpoint is invalid.
                        // Check NameTable to find out if this is a name that has been assigned.
                        if (!router.IsValidLocalUniqueName(nameStr)) {
                            replyCode = ALLJOYN_PING_REPLY_UNKNOWN_NAME;
                        } else {
                            replyCode = ALLJOYN_PING_REPLY_UNREACHABLE;
                        }
                    } else if (IsGuidShortStringKnown(guidStr)) {
                        guid = guidStr;
                    } else {
                        replyCode = ALLJOYN_PING_REPLY_UNKNOWN_NAME;
                    }
                }
            } else if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                VirtualEndpoint vep = VirtualEndpoint::cast(ep);
                guid = String(vep->GetUniqueName()).substr(1, GUID128::SHORT_SIZE);
                QCC_DbgPrintf(("Session found %s", name));
            }

            if (!guid.empty() && replyCode == ALLJOYN_PING_REPLY_SUCCESS) {
                QCC_DbgPrintf(("Pinging GUID %s", guid.c_str()));
                if (outgoingPingMap.find(pair<String, String>(name, msg->GetSender())) != outgoingPingMap.end()) {
                    replyCode = ALLJOYN_PING_REPLY_IN_PROGRESS;
                    ReleaseLocks();
                } else {
                    PingAlarmContext* ctx = new PingAlarmContext(PingAlarmContext::REPLY_CONTEXT, name, msg->GetSender());
                    AllJoynObj* pObj = this;

                    Alarm alarm(timeout, pObj, ctx);
                    OutgoingPingInfo ogi(alarm, msg);
                    pair<String, String> key(name, msg->GetSender());
                    outgoingPingMap.insert(pair<pair<String, String>, OutgoingPingInfo>(key, ogi));
                    timer.AddAlarm(alarm);
                    ReleaseLocks();
                    status = IpNameService::Instance().Ping(transport, guid, name);
                    if (status != ER_OK) {
                        QCC_DbgPrintf(("Query failed status %s", QCC_StatusText(status)));
                        AcquireLocks();
                        multimap<pair<String, String>, OutgoingPingInfo>::iterator it = outgoingPingMap.find(key);
                        if (it != outgoingPingMap.end()) {
                            replyCode = (status == ER_ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE) ? ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE : ALLJOYN_PING_REPLY_FAILED;
                            outgoingPingMap.erase(it);
                        }
                        if (timer.RemoveAlarm(alarm, false)) {
                            //Delete ctx if alarm was successfully removed.
                            delete ctx;
                        }
                        ReleaseLocks();
                    }
                }
            } else {
                if (replyCode != ALLJOYN_PING_REPLY_UNREACHABLE) {
                    replyCode = ALLJOYN_PING_REPLY_UNKNOWN_NAME;

                }
                ReleaseLocks();
            }
        }
    }

    /* Reply to request if something went wrong.  The success case is handled asynchronously. */
    if (replyCode != ALLJOYN_PING_REPLY_SUCCESS) {
        MsgArg replyArg("u", replyCode);
        status = MethodReply(msg, &replyArg, 1);
        QCC_DbgPrintf(("AllJoynObj::Ping(%s) returned %d (status=%s)", name, replyCode, QCC_StatusText(status)));

        /* Log error if reply could not be sent */
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to respond to org.alljoyn.Bus.Ping"));
        }
    }

    return;
}

void AllJoynObj::PingReplyMethodHandler(Message& reply, void* context)
{
    QCC_DbgTrace(("AllJoynObj::PingReplyMethodHandler()"));
    Message* msg = static_cast<Message*>(context);
    uint32_t replyCode = (ajn::MESSAGE_ERROR == reply->GetType()) ? ALLJOYN_PING_REPLY_UNREACHABLE : ALLJOYN_PING_REPLY_SUCCESS;

    /* Parse the message args */
    size_t numArgs;
    const MsgArg* args;

    (*msg)->GetArgs(numArgs, args);
    const char* name = NULL;
    uint32_t timeout;
    MsgArg::Get(args, numArgs, "su", &name, &timeout);

    AcquireLocks();
    dbusPingsInProgress.erase(pair<String, String>((*msg)->GetSender(), name));
    ReleaseLocks();

    PingReplyMethodHandlerUsingCode(*msg, replyCode);
    delete msg;
}

/* From IpNameServiceListener */
bool AllJoynObj::ResponseHandler(TransportMask transport, MDNSPacket response, uint16_t recvPort)
{
    /*
     * Note that we always return false here so that other name service
     * listeners can process other records in the response.
     */

    MDNSResourceRecord* pingReplyRecord;
    if ((recvPort == IpNameService::MULTICAST_MDNS_PORT) ||
        !response->GetAdditionalRecord("ping-reply.*", MDNSResourceRecord::TXT, &pingReplyRecord)) {
        return false;
    }

    MDNSPingReplyRData* pingRData = static_cast<MDNSPingReplyRData*>(pingReplyRecord->GetRData());
    if (!pingRData) {
        QCC_DbgPrintf(("Ignoring response with invalid ping info"));
        return false;
    }

    QCC_DbgPrintf(("Recieved a ping response for name %s", pingRData->GetWellKnownName().c_str()));
    const String& name = pingRData->GetWellKnownName();
    uint32_t replyCode;
    if (pingRData->GetReplyCode() == "ALLJOYN_PING_REPLY_SUCCESS") {
        replyCode = ALLJOYN_PING_REPLY_SUCCESS;
    } else if (pingRData->GetReplyCode() == "ALLJOYN_PING_REPLY_UNREACHABLE") {
        replyCode = ALLJOYN_PING_REPLY_UNREACHABLE;
    } else {
        replyCode = ALLJOYN_PING_REPLY_UNKNOWN_NAME;
    }

    AcquireLocks();

    std::multimap<pair<String, String>, OutgoingPingInfo>::iterator it = outgoingPingMap.lower_bound(pair<String, String>(name, ""));
    list<Message> replyMsgs;

    while (it != outgoingPingMap.end() && it->first.first == name) {
        // TODO May need to filter on transport
        OutgoingPingInfo opi = it->second;
        outgoingPingMap.erase(it++);
        replyMsgs.push_back(opi.message);
        PingAlarmContext* ctx = static_cast<PingAlarmContext*>(opi.alarm->GetContext());
        if (timer.RemoveAlarm(opi.alarm, false)) {
            //Delete context if alarm was successfully removed.
            delete ctx;
        }
    }

    ReleaseLocks();
    for (list<Message>::iterator it = replyMsgs.begin(); it != replyMsgs.end(); it++) {
        PingReplyMethodHandlerUsingCode(*it, replyCode);
    }
    return false;
}

void AllJoynObj::PingReplyMethodHandlerUsingCode(Message& msg, uint32_t replyCode)
{
    QCC_DbgTrace(("AllJoynObj::PingReplyMethodHandlerUsingCode()"));
    const char* name = NULL;
    msg->GetArgs("s", &name);
    QCC_DbgPrintf(("AllJoynObj::Ping(%s) returned %d", name, replyCode));

    MsgArg replyArg("u", replyCode);
    MethodReply(msg, &replyArg, 1);
}


/* From IpNameServiceListener */
bool AllJoynObj::QueryHandler(TransportMask transport, MDNSPacket query, uint16_t recvPort,
                              const qcc::IPEndpoint& ns4)
{
    MDNSResourceRecord* pingRecord;
    if ((recvPort == IpNameService::MULTICAST_MDNS_PORT) ||
        !query->GetAdditionalRecord("ping.*", MDNSResourceRecord::TXT, &pingRecord)) {
        QCC_DbgPrintf(("Received Unicast packet which is not PING"));
        return false;
    }

    MDNSPingRData* pingRData = static_cast<MDNSPingRData*>(pingRecord->GetRData());
    if (!pingRData) {
        QCC_DbgPrintf(("Ignoring query with invalid ping info"));
        return true;
    }

    const String& name = pingRData->GetWellKnownName();

    //If incomingPingMap does not have this key, then add an alarm
    //in any case add it to incomingPingMap.
    AcquireLocks();
    bool alarmFound = (incomingPingMap.find(name) != incomingPingMap.end());
    IncomingPingInfo ipi(transport, ns4);

    incomingPingMap.insert(pair<String, IncomingPingInfo>(name, ipi));
    if (!alarmFound) {
        uint32_t timeout = 0;
        PingAlarmContext* ctx = new PingAlarmContext(PingAlarmContext::TRANSPORT_CONTEXT, name);
        AllJoynObj* pObj = this;
        Alarm alarm(timeout, pObj, ctx);
        timer.AddAlarm(alarm);
    }
    ReleaseLocks();
    return true;
}

void AllJoynObj::PingReplyTransportHandler(Message& reply, void* context)
{
    PingAlarmContext* ctx = static_cast<PingAlarmContext*>(context);
    if (ajn::MESSAGE_ERROR == reply->GetType()) {
        const char* errorName = reply->GetErrorName();
        if (errorName && (0 == strcmp(errorName, "org.alljoyn.Bus.Timeout"))) {
            /*
             * There may be multiple ping callers with different timeouts being
             * serviced by a single DBus Ping, so don't send a response here,
             * let the caller timeout on their own schedule.
             */
        } else {
            /* Likely error name is "org.freedesktop.DBus.Error.ServiceUnknown */
            SendIPNSResponse(ctx->name, ALLJOYN_PING_REPLY_UNREACHABLE);
        }
    } else {
        SendIPNSResponse(ctx->name, ALLJOYN_PING_REPLY_SUCCESS);
    }
    delete ctx;
}

void AllJoynObj::PingResponse(TransportMask transport, const qcc::IPEndpoint& ns4, const qcc::String& name, uint32_t replyCode)
{
    MDNSPacket response;
    response->SetDestination(ns4);

    // Similar to advertise record with only one name
    MDNSPingReplyRData* pingReplyRData = new MDNSPingReplyRData();
    pingReplyRData->SetWellKnownName(name);
    String replyCodeText;
    if (replyCode == ALLJOYN_PING_REPLY_SUCCESS) {
        replyCodeText = "ALLJOYN_PING_REPLY_SUCCESS";
    } else if (replyCode == ALLJOYN_PING_REPLY_UNREACHABLE) {
        replyCodeText = "ALLJOYN_PING_REPLY_UNREACHABLE";
    } else {
        replyCodeText = "ALLJOYN_PING_REPLY_UNKNOWN_NAME";
    }

    pingReplyRData->SetReplyCode(replyCodeText);

    MDNSResourceRecord pingReplyRecord("ping-reply." + guid.ToString() + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, pingReplyRData);
    response->AddAdditionalRecord(pingReplyRecord);
    delete pingReplyRData;

    QStatus status = IpNameService::Instance().Response(transport, 120, response);
    if (ER_OK != status) {
        QCC_LogError(status, ("Response failed"));
    }
}

}
