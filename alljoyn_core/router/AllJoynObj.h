/**
 * @file
 * BusObject responsible for implementing the AllJoyn methods (org.alljoyn.Bus)
 * for messages directed to the bus.
 */

/******************************************************************************
 * Copyright (c) 2010-2012, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_ALLJOYNOBJ_H
#define _ALLJOYN_ALLJOYNOBJ_H

#include <qcc/platform.h>
#include <vector>
#include <map>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/StringMapKey.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/SocketTypes.h>
#include <qcc/Timer.h>
#include <qcc/GUID.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>

#include "Bus.h"
#include "BusUtil.h"
#include "NameTable.h"
#include "RemoteEndpoint.h"
#include "Transport.h"
#include "VirtualEndpoint.h"
#include "PermissionMgr.h"
#include "ns/IpNameService.h"

namespace ajn {

/** Forward Declaration */
class BusController;

/**
 * BusObject responsible for implementing the standard AllJoyn methods at org.alljoyn.Bus
 * for messages directed to the bus.
 */
class AllJoynObj : public BusObject, public NameListener, public TransportListener, public qcc::AlarmListener,
    public IpNameServiceListener {
    friend class _RemoteEndpoint;
    struct PingAlarmContext;
    struct SessionMapEntry;
    class OutgoingPingInfo {
      public:
        qcc::Alarm alarm;
        Message message;
        OutgoingPingInfo(qcc::Alarm alarm, Message message) : alarm(alarm), message(message) { };
      private:
        OutgoingPingInfo();
    };
    class IncomingPingInfo {
      public:
        TransportMask transport;
        qcc::IPEndpoint ns4;

        IncomingPingInfo(TransportMask transport, qcc::IPEndpoint ns4) :
            transport(transport), ns4(ns4)
        { };
      private:
        IncomingPingInfo();
    };
  public:
    typedef enum {
        LEAVE_HOSTED_SESSION,
        LEAVE_JOINED_SESSION,
        LEAVE_SESSION,
    } LeaveSessionType;
    /**
     * Constructor
     *
     * @param bus            Bus to associate with org.freedesktop.DBus message handler.
     * @param router         The DaemonRouter associated with the bus.
     * @param busController  Controller that created this object.
     */
    AllJoynObj(Bus& bus, BusController* busController);

    /**
     * Destructor
     */
    ~AllJoynObj();

    /**
     * Initialize and register this DBusObj instance.
     *
     * @return ER_OK if successful.
     */
    QStatus Init();

    /**
     * Stop AlljoynObj.
     *
     * @return ER_OK if successful.
     */
    QStatus Stop();

    /**
     * Join AlljoynObj.
     *
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Called when object is successfully registered.
     */
    void ObjectRegistered(void);

    /**
     * Respond to a bus request to bind a SessionPort.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionPort  sessionPort    SessionPort identifier.
     *   opts         SessionOpts    SessionOpts that must be agreeable to any joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   An ALLJOYN_BINDSESSIONPORT_* reply code (see AllJoynStd.h).
     *   sessionPort  uint16   SessionPort (same as input sessionPort unless SESSION_PORT_ANY was specified)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void BindSessionPort(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to unbind a SessionPort.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionPort  sessionPort    SessionPort to be unbound.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   An ALLJOYN_UNBINDSESSIONPORT_* reply code (see AllJoynStd.h).
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void UnbindSessionPort(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to join an existing session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   creatorName  string        Name of session creator.
     *   sessionPort  SessionPort   SessionPort targeted for join request.
     *   opts         SessionOpts   Session options requested by the joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32        A ALLJOYN_JOINSESSION_* reply code (see AllJoynStd.h).
     *   sessionId    uint32        Session identifier.
     *   opts         SessionOpts   Final (negociated) session options.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void JoinSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to leave a previously joined or created session.
     * Note this function will produce an error if called on a self-joined session by
     * a self-joined member (host or joiner)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void LeaveSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to leave a created session.
     *
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void LeaveHostedSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to leave a joined session.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void LeaveJoinedSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Checks whether the LeaveSession/LeaveHostedSession/LeaveJoinedSession
     * request is valid
     *
     * @param smEntry   Relevant sessionMapEntry
     * @param sender    Sender of the request
     * @param id        sessionid
     * @param lst       type of leavesession request
     * @param senderWasSelfJoined indicates whether sender was involved in self-joined session
     *
     * @return Returns an ALLJOYN_LEAVESESSION_REPLY_*
     *
     */
    uint32_t CheckLeaveSession(const SessionMapEntry* smEntry, const char* sender, SessionId id, LeaveSessionType lst, bool& senderWasSelfJoined) const;

    /**
     * Common logic for all LeaveSession requests
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionId    uint32   Session identifier.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_LEAVESESSION_* reply code (see AllJoynStd.h).
     *
     * @param member  Member.
     * @param msg     The incoming message.
     * @param lst     type of leavesession request
     */
    void LeaveSessionCommon(const InterfaceDescription::Member* member, Message& msg, LeaveSessionType lst);

    /**
     * Respond to a bus request to advertise the existence of a remote AllJoyn instance.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   advertisedName  string   A locally obtained well-known name that should be advertised to external AllJoyn instances.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_ADVERTISE_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void AdvertiseName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to cancel a previous advertisement.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   advertisedName  string   A previously advertised well-known name that no longer needs to be advertised.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_ADVERTISE_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void CancelAdvertiseName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to look for advertisements from remote AllJoyn instances.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   namePrefix    string   A well-known name prefix that the caller wants to be notified of (via signal)
     *                          when a remote Bus instance is found that advertises a name that matches the prefix.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_FINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void FindAdvertisedName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to look for advertisements from remote AllJoyn instances over a set of specified transports.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   namePrefix    string   A well-known name prefix that the caller wants to be notified of (via signal)
     *                          when a remote Bus instance is found that advertises a name that matches the prefix.
     *   transports    uint16   Transport bit mask
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_FINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void FindAdvertisedNameByTransport(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to look for advertisements from remote AllJoyn instances over a set of specified transports.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   matching      string   Key, value match criteria that the caller wants to be notified of (via signal)
     *                          when a remote Bus instance is found with an advertisement that matches the criteria.
     *   transports    uint16   Transport bit mask
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_FINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void FindAdvertisementByTransport(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to cancel a previous (successful) FindName request.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   namePrefix    string   The well-known name prefix that was used in a successful call to FindName.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_CANCELFINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void CancelFindAdvertisedName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to cancel a previous (successful) FindName request over a set of specified transports.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   namePrefix    string   The well-known name prefix that was used in a successful call to FindName.
     *   transports    uint16   Transport bit mask
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_CANCELFINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void CancelFindAdvertisedNameByTransport(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to cancel a previous (successful) FindAdvertisement request over a set of specified transports.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   matching      string   The key, value match criteria that was used in a successful call to FindAdvertisement.
     *   transports    uint16   Transport bit mask
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_CANCELFINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void CancelFindAdvertisementByTransport(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to get a (streaming) file descritor for an existing session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionId   uint32    A session id that identifies an existing streaming session.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   sessionFd   handle    The socket file descriptor for session.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void GetSessionFd(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to set the link timeout for a given session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionId      uint32    A session id that identifies an existing streaming session.
     *   reqLinkTimeout uint32    Requested max number of seconds that an unresponsive comm link
     *                            will be monitored before delcaring the link dead via SessionLost.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode     uint32    ALLJOYN_SETLINKTIMEOUT_REPLY_* value.
     *   actLinkTimeout uint32    Actual link timeout value.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void SetLinkTimeout(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to set the idle timeouts for a leaf node.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   reqIdleTO    uint32  Requested Idle Timeout for the link. i.e. time after which the Routing node must
     *                        must send a DBus ping to Leaf node in case of inactivity.
     *                        Use 0 to leave unchanged.
     *   reqProbeTO   uint32  Requested Probe timeout. The time from the Routing node sending the DBus
     *                        ping to the expected response.
     *                        Use 0 to leave unchanged.
     *
     *  Output params:
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   disposition  uint32  ALLJOYN_SETIDLETIMEOUTS_* value
     *   actIdleTO    uint32  Actual Idle Timeout. i.e. time after which the Routing node will
     *                        send a DBus ping to Leaf node in case of inactivity.
     *   actProbeTO   uint32  Actual Probe Timeout. The time from the Routing node sending the DBus ping
     *                        to the expected response from the leaf node.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void SetIdleTimeouts(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Add an alias to a Unix User ID
     * The input Message (METHOD_CALL) is expected to contain the following parameter
     *   aliasUID      uint32    The alias ID
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode    uint32    ALLJOYN_ALIASUNIXUSER_REPLY_* value.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void AliasUnixUser(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Add an advertised name alias for remote daemon guid.
     *
     * This method is used by SessionlessObj to tell AllJoynObj that JoinSession requests
     * for unique names containing the given GUID / transport mask should be aliased with the
     * given well-known name for the purpose of estabilishing a transport connection.
     *
     * @param guid       Short GUID string of remote daemon that advertised the name.
     * @param mask       Transport used by remote daemon to advertise advName.
     * @param advName    Well-known name advertised by remote daemon.
     */
    void AddAdvNameAlias(const qcc::String& guid, const TransportMask mask, const qcc::String& advName);

    /**
     * Handle event that the application/process is suspending on OS like WinRT.
     * On Windows RT, an application is suspended when it becomes invisible after about
     * 10 seconds and resumed when users bring it back to the foreground if not terminated.
     * Upon the suspending event the daemon bundled with the Windows Store applcation has to release
     * exclusive resources (eg., socket file descriptor and port); the bundled daemon re-aquires the
     * resource when the application resumes.
     *
     * The input Message (METHOD_CALL) is expected to contain no parameters.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_ONAPPSUSPEND_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     *
     */
    void OnAppSuspend(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Handle event that the application/process is suspending on OS like WinRT.
     * On Windows RT, an application is suspended when it becomes invisible after about
     * 10 seconds and resumed when users bring it back to the foreground if not terminated.
     * Upon the suspending event the daemon bundled with the Windows Store applcation has to release
     * exclusive resources (eg., socket file descriptor and port); the bundled daemon re-aquires the
     * resource when the application resumes.
     *
     * The input Message (METHOD_CALL) is expected to contain no parameters.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_ONAPPRESUME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     *
     */
    void OnAppResume(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Method handler for org.alljoyn.Bus.CancelSessionlessMessage
     *
     * @param member    Interface member.
     * @param msg       The incoming method call message.
     *
     */
    void CancelSessionlessMessage(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Method handler for org.alljoyn.Bus.RemoveSessionMember
     *
     * @param member    Interface member.
     * @param msg       The incoming method call message.
     *
     */
    void RemoveSessionMember(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Method handler for org.alljoyn.Bus.GetHostInfo
     *
     * @param member    Interface member.
     * @param msg       The incoming method call message.
     *
     */
    void GetHostInfo(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Method handler for org.alljoyn.Bus.ReloadConfig
     *
     * @param member    Interface member.
     * @param msg       The incoming method call message.
     *
     */
    void ReloadConfig(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Method handler for org.alljoyn.Bus.Ping
     *
     * @param member    Interface member.
     * @param msg       The incoming method call message.
     *
     */
    void Ping(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Add a new Bus-to-bus endpoint.
     *
     * @param endpoint  Bus-to-bus endpoint to add.
     * @param ER_OK if successful
     */
    QStatus AddBusToBusEndpoint(RemoteEndpoint& endpoint);

    /**
     * Remove an existing Bus-to-bus endpoint.
     *
     * @param endpoint  Bus-to-bus endpoint to add.
     */
    void RemoveBusToBusEndpoint(RemoteEndpoint& endpoint);

    /**
     * Respond to a remote daemon request to attach a session through this daemon.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionPort   SessionPort  The session port.
     *   joiner        string       The unique name of the session joiner.
     *   creator       string       The name of the session creator.
     *   optsIn        SesionOpts   The session options requested by the joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode    uint32       A ALLJOYN_JOINSESSION_* reply code (see AllJoynStd.h).
     *   sessionId     uint32       The session id (valid if resultCode indicates success).
     *   opts          SessionOpts  The actual (final) session options.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void AttachSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a remote daemon request to get session info from this daemon.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   creator       string       Name of attachment that bound the session port.
     *   sessionPort   SessionPort  The sessionPort whose info is being requested.
     *   opts          SesionOpts   The session options requested by the joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   busAddr       string       The bus address to use when attempting to create
     *                              a connection for the purpose of joining the given session.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void GetSessionInfo(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Process incoming ExchangeNames signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void ExchangeNamesSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    /**
     * Process incoming NameChanged signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void NameChangedSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    /**
     * Process incoming SessionDetach signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void DetachSessionSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                          const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);

    /**
     * Receive notification of a new bus instance via TransportListener.
     * Internal use only.
     *
     * @param   busAddr   Address of discovered bus.
     * @param   guid      GUID of daemon that sent the advertisement.
     * @param   transport Transport that received the advertisement.
     * @param   names     Vector of bus names advertised by the discovered bus.
     * @param   ttl       Number of seconds before this advertisement expires
     *                    (0 means expire immediately, numeric_limits<uint32_t>::max() means never expire)
     */
    void FoundNames(const qcc::String& busAddr, const qcc::String& guid, TransportMask transport, const std::vector<qcc::String>* names, uint32_t ttl);

    /**
     * Called when a transport gets a surprise disconnect from a remote bus.
     *
     * @param busAddr       The address of the bus formatted as a string.
     */
    void BusConnectionLost(const qcc::String& busAddr);

    /**
     * Get reference to the daemon router object
     */
    DaemonRouter& GetDaemonRouter() { return router; }

  private:
    Bus& bus;                             /**< The bus */
    DaemonRouter& router;                 /**< The router */
    qcc::Mutex stateLock;                 /**< Lock that protects AlljoynObj state */

    const InterfaceDescription* daemonIface;               /**< org.alljoyn.Daemon interface */

    const InterfaceDescription::Member* foundNameSignal;   /**< org.alljoyn.Bus.FoundName signal */
    const InterfaceDescription::Member* lostAdvNameSignal; /**< org.alljoyn.Bus.LostAdvertisdName signal */
    const InterfaceDescription::Member* sessionLostSignal; /**< org.alljoyn.Bus.SessionLost signal */
    const InterfaceDescription::Member* sessionLostWithReasonSignal; /**< org.alljoyn.Bus.SessionLostWithReason signal */
    const InterfaceDescription::Member* sessionLostWithReasonAndDispositionSignal; /**< org.alljoyn.Bus.SessionLostWithReasonAndDisposition signal */
    const InterfaceDescription::Member* mpSessionChangedSignal;  /**< org.alljoyn.Bus.MPSessionChanged signal */
    const InterfaceDescription::Member* mpSessionChangedWithReason;  /**< org.alljoyn.Bus.MPSessionChangedWithReason signal */
    const InterfaceDescription::Member* mpSessionJoinedSignal;  /**< org.alljoyn.Bus.JoinSession signal */

    /** Map of open connectSpecs to local endpoint name(s) that require the connection. */
    std::multimap<qcc::String, qcc::String> connectMap;

    /** Map of active advertised names to requesting local endpoint's permitted transport mask(s) and name(s) */
    std::multimap<qcc::String, std::pair<TransportMask, qcc::String> > advertiseMap;

    /** Map of active discovery names to requesting local endpoint's permitted transport mask(s) and name(s) */
    struct DiscoverMapEntry {
        TransportMask transportMask;
        qcc::String sender;
        MatchMap matching;
        bool initComplete;
        DiscoverMapEntry(TransportMask transportMask, const qcc::String& sender, const MatchMap& matching, bool initComplete = false) :
            transportMask(transportMask),
            sender(sender),
            matching(matching),
            initComplete(initComplete) { }
    };
    typedef std::multimap<qcc::String, DiscoverMapEntry> DiscoverMapType;
    DiscoverMapType discoverMap;

    /** Map of discovered bus names (protected by discoverMapLock) */
    struct NameMapEntry {
        qcc::String busAddr;
        qcc::String guid;
        TransportMask transport;
        uint64_t timestamp;
        uint64_t ttl;
        qcc::Alarm alarm;
        static void* truthiness;

        NameMapEntry(const qcc::String& busAddr, const qcc::String& guid, TransportMask transport, uint64_t ttl, qcc::AlarmListener* listener) :
            busAddr(busAddr),
            guid(guid),
            transport(transport),
            timestamp(qcc::GetTimestamp64()),
            ttl(ttl),
            alarm(ttl, listener, truthiness) { }
    };
    typedef std::multimap<qcc::String, NameMapEntry> NameMapType;
    NameMapType nameMap;

    /* Session map */
    struct SessionMapEntry {
        qcc::String endpointName;
        SessionId id;
        qcc::String sessionHost;
        SessionPort sessionPort;
        SessionOpts opts;
        qcc::SocketFd fd;
        RemoteEndpoint streamingEp;
        std::vector<qcc::String> memberNames;
        bool isInitializing;
        bool isRawReady;
        bool IsSelfJoin() const {
            return find(memberNames.begin(), memberNames.end(), sessionHost) != memberNames.end();
        }

        qcc::String ToString() const {
            char idbuf[16];
            qcc::String str;
            str.append("endpoint: ");
            str.append(endpointName);
            str.append(", id: ");
            snprintf(idbuf, sizeof(idbuf), "%u", id);
            str.append(idbuf);
            str.append(", host: ");
            str.append(sessionHost);
            str.append(", members: ");
            for (std::vector<qcc::String>::const_iterator it = memberNames.begin(); it != memberNames.end(); ++it) {
                str.append(*it);
                str.append(",");
            }
            str.append(" selfjoin: ");
            str.append(IsSelfJoin() ? "yes" : "no");
            return str;
        }

        SessionMapEntry() :
            id(0),
            sessionPort(0),
            opts(),
            fd(qcc::INVALID_SOCKET_FD),
            isInitializing(false),
            isRawReady(false) { }
    };

    typedef std::multimap<std::pair<qcc::String, SessionId>, SessionMapEntry> SessionMapType;

    SessionMapType sessionMap;  /**< Map (endpointName,sessionId) to session info */

    /**
     * Helper function to convert a QStatus into a SessionLost reason
     */
    SessionListener::SessionLostReason ConvertReasonToSessionLostReason(QStatus reason) const;

    /*
     * Helper function to get session map interator
     */
    SessionMapEntry* SessionMapFind(const qcc::String& name, SessionId session);

    /*
     * Helper function to get session map range interator
     */
    SessionMapType::iterator SessionMapLowerBound(const qcc::String& name, SessionId session);

    /*
     * Helper function to get session map range interator
     */
    SessionMapType::iterator SessionMapUpperBound(const qcc::String& name, SessionId session);

    /**
     * Helper function to insert a sesssion map
     */
    void SessionMapInsert(SessionMapEntry& sme);

    /**
     * Helper function to erase a sesssion map
     */
    void SessionMapErase(SessionMapEntry& sme);

    const qcc::GUID128& guid;                                  /**< Global GUID of this daemon */

    const InterfaceDescription::Member* exchangeNamesSignal;   /**< org.alljoyn.Daemon.ExchangeNames signal member */
    const InterfaceDescription::Member* detachSessionSignal;   /**< org.alljoyn.Daemon.DetachSession signal member */

    std::map<qcc::String, VirtualEndpoint> virtualEndpoints;   /**< Map of endpoints that reside behind a connected AllJoyn daemon */

    std::map<qcc::StringMapKey, RemoteEndpoint> b2bEndpoints;  /**< Map of bus-to-bus endpoints that are connected to external daemons */

    struct AdvAliasEntry {
        qcc::String name;
        TransportMask transport;
        AdvAliasEntry(qcc::String name, TransportMask transport) : name(name), transport(transport) { }
        bool operator<(const AdvAliasEntry& other) const {
            return (name < other.name) || ((name == other.name) && (transport < other.transport));
        }
        bool operator==(const AdvAliasEntry& other) const {
            return name == other.name && transport == other.transport;
        }
    };
    std::map<qcc::String, std::set<AdvAliasEntry> > advAliasMap;  /**< Map remote daemon guid/transport to advertised name alias */

    struct SentSetEntry {
        qcc::String name;
        TransportMask transport;
        SentSetEntry(qcc::String name, TransportMask transport) : name(name), transport(transport) { }
        bool operator<(const SentSetEntry& other) const {
            // Order in descending order of transport so that UDP transport is sent first.
            return (name < other.name) || ((name == other.name) && (transport > other.transport));
        }
        bool operator==(const SentSetEntry& other) const {
            return name == other.name && transport == other.transport;
        }
    };

    struct JoinSessionEntry {
        qcc::String name;
        TransportMask transport;
        qcc::String busAddr;
        JoinSessionEntry(qcc::String name, TransportMask transport, qcc::String busAddr) : name(name), transport(transport), busAddr(busAddr) { }
        bool operator<(const JoinSessionEntry& other) const {
            // Order in descending order of transport so that UDP transport is sent first.
            return (name < other.name) || ((name == other.name) && (transport > other.transport))
                   || ((name == other.name) && (transport == other.transport)  && (busAddr < other.busAddr));
        }
        bool operator==(const JoinSessionEntry& other) const {
            return name == other.name && transport == other.transport && busAddr == other.busAddr;
        }
    };
    qcc::Timer timer;           /**< Timer object for reaping expired names */

    /**
     * Name reaper timeout alarm handler.
     *
     * @param alarm  The alarm object for the timeout that expired.
     */
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    /** JoinSessionThread handles a JoinSession request from a local client on a separate thread */
    class JoinSessionThread : public qcc::Thread, public qcc::ThreadListener {
      public:
        JoinSessionThread(AllJoynObj& ajObj, const Message& msg, bool isJoin) :
            qcc::Thread(qcc::String("JoinS-") + qcc::U32ToString(qcc::IncrementAndFetch(&jstCount))),
            ajObj(ajObj),
            msg(msg),
            isJoin(isJoin) { }

        void ThreadExit(Thread* thread);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        static int jstCount;
        qcc::ThreadReturn STDCALL RunJoin();
        qcc::ThreadReturn STDCALL RunAttach();

        AllJoynObj& ajObj;
        Message msg;
        bool isJoin;
    };

    std::vector<JoinSessionThread*> joinSessionThreads;  /**< List of outstanding join session requests */
    qcc::Mutex joinSessionThreadsLock;                   /**< Lock that protects joinSessionThreads */
    bool isStopping;                                     /**< True while waiting for threads to exit */
    BusController* busController;                        /**< BusController that created this BusObject */

    /**
     * Acquire AllJoynObj locks.
     */
    void AcquireLocks();

    /**
     * Release AllJoynObj locks.
     */
    void ReleaseLocks();

    /**
     * Utility function used to send a single FoundName signal.
     *
     * @param dest        Unique name of destination.
     * @param name        Well-known name that was found.
     * @param transport   The transport that received the advertisment.
     * @param namePrefix  Well-known name prefix used in call to FindName() that triggered this notification.
     * @return ER_OK if succssful.
     */
    QStatus SendFoundAdvertisedName(const qcc::String& dest,
                                    const qcc::String& name,
                                    TransportMask transport,
                                    const qcc::String& namePrefix);

    /**
     * Utility function used to send LostAdvertisedName signals to each "interested" local endpoint.
     *
     * @param name        Well-known name whose advertisment was lost.
     * @param transport   Transport whose advertisment for name has gone away.
     * @return ER_OK if succssful.
     */
    QStatus SendLostAdvertisedName(const qcc::String& name, TransportMask transport);

    /**
     * Utility method used to invoke SessionAttach remote method.
     *
     * @param sessionPort      SessionPort used in join request.
     * @param src              Unique name of session joiner.
     * @param sessionHost      Unique name of sessionHost.
     * @param dest             Unique name of session creator.
     * @param b2bEp            Directly connected (next hop) B2B endpoint.
     * @param remoteControllerName  Unique name of bus controller at next hop.
     * @param outgoingSessionId     SessionId to use for outgoing AttachSession message. Should
     *                              be 0 for newly created (non-multipoint) sessions.
     * @param busAddr          Destination bus address from advertisement or GetSessionInfo.
     * @param optsIn           Session options requested by joiner.
     * @param replyCode        [OUT] SessionAttach response code
     * @param sessionId        [OUT] session id if reply code indicates success.
     * @param optsOut          [OUT] Actual (final) session options.
     * @param members          [OUT] Array or session members (strings) formatted as MsgArg.
     */
    QStatus SendAttachSession(SessionPort sessionPort,
                              const char* src,
                              const char* sessionHost,
                              const char* dest,
                              RemoteEndpoint& b2bEp,
                              const char* remoteControllerName,
                              SessionId outgoingSessionId,
                              const char* busAddr,
                              const SessionOpts& optsIn,
                              uint32_t& replyCode,
                              SessionId& sessionId,
                              SessionOpts& optsOut,
                              MsgArg& members);

    /**
     * Utility method used to invoke AcceptSession on device local endpoint.
     *
     * @param sessionPort      SessionPort that received the join request.
     * @param sessionId        Id for new session (if accepted).
     * @param creatorName      Session creator unique name.
     * @param joinerName       Session joiner unique name.
     * @param opts             Session options requsted by joiner
     * @param isAccepted       [OUT] true iff creator accepts session. (valid if return is ER_OK).
     */
    QStatus SendAcceptSession(SessionPort sessionPort,
                              SessionId sessionId,
                              const char* creatorName,
                              const char* joinerName,
                              const SessionOpts& opts,
                              bool& isAccepted);

    /**
     * Utility method used to send SessionJoined.
     *
     * @param sessionPort      SessionPort that received the join request.
     * @param sessionId        Id for new session (if accepted).
     * @param creatorName      Session creator unique name.
     * @param joinerName       Session joiner unique name.
     */
    QStatus SendSessionJoined(SessionPort sessionPort,
                              SessionId sessionId,
                              const char* joinerName,
                              const char* creatorName);

    /**
     * Utility method used to send SessionLost signal to locally attached endpoint.
     *
     * @param       entry    SessionMapEntry that was lost.
     * @param       reason   Reason for the SessionLost.
     */
    void SendSessionLost(const SessionMapEntry& entry, QStatus reason, unsigned int disposition = 0);

    /**
     * Utility method used to send MPSessionChanged signal to locally attached endpoint.
     *
     * @param   sessionId   The sessionId.
     * @param   name        Unique name of session member that changed.
     * @param   isAdd       true if member added.
     * @param   dest        Local destination for MPSessionChanged.
     * @param   reason      Specifies the reason why the session changed
     */
    void SendMPSessionChanged(SessionId sessionId, const char* name, bool isAdd, const char* dest, unsigned int reason);

    /**
     * Utility method used to invoke GetSessionInfo remote method.
     *
     * @param       creatorName    Bus name of session creator.
     * @param       sessionPort    Session port value.
     * @param       opts           Requested session options.
     * @param[out]  busAddrs       Returned busAddrs for session (if return value is ER_OK)
     * @return  ER_OK if successful.
     */
    QStatus SendGetSessionInfo(const char* creatorName,
                               SessionPort sessionPort,
                               const SessionOpts& opts,
                               std::vector<qcc::String>& busAddrs);

    /**
     * Add a virtual endpoint with a given unique name.
     *
     * @param uniqueName          The uniqueName of the virtual endpoint.
     * @param b2bEpName           Unique name of bus-to-bus endpoint that "owns" the virtual endpoint.
     * @param changesMade         [OUT] Written to true of virtual endpoint was created (as opposed to already existing).
     */
    void AddVirtualEndpoint(const qcc::String& uniqueName, const qcc::String& b2bEpName, bool* changesMade = NULL);

    /**
     * Remove a virtual endpoint.
     *
     * @param uniqueName   The unique name of the virtualEndpoint to be removed.
     */
    void RemoveVirtualEndpoint(const qcc::String& uniqueName);

    /**
     * Find a virtual endpoint by its name.
     *
     * @param uniqueName    The name of the endpoint to find.
     * @return The requested virtual endpoint or NULL if not found.
     */
    VirtualEndpoint FindVirtualEndpoint(const qcc::String& uniqueName);

    /**
     * Internal bus-to-bus remote endpoint listener.
     * Called when any virtual endpoint's remote endpoint exits.
     *
     * @param ep   RemoteEndpoint that is exiting.
     */
    void EndpointExit(RemoteEndpoint& ep);

    /**
     * Send signal that informs remote bus of names available on local daemon.
     * This signal is used only in bus-to-bus connections.
     *
     * @param endpoint    Remote endpoint to exchange names with.
     * @return  ER_OK if successful.
     */
    QStatus ExchangeNames(RemoteEndpoint& endpoint);

    /**
     * Process a request to cancel advertising a name from a given (locally-connected) endpoint.
     *
     * @param uniqueName         Name of endpoint requesting end of advertising
     * @param advertiseName      Well-known name whose advertising is to be canceled.
     * @param transports         Set of transports that should cancel the advertisment.
     * @return ER_OK if successful.
     */
    QStatus ProcCancelAdvertise(const qcc::String& uniqueName, const qcc::String& advertiseName, TransportMask transports);

    /**
     * Process a request to cancel discovery of a name prefix from a given (locally-connected) endpoint.
     *
     * @param endpointName         Name of endpoint requesting end of discovery
     * @param matching             Key, value match criteria to be removed from discovery list
     * @param transports           Set of transports that should cancel the discovery.
     * @return ER_OK if successful.
     */
    QStatus ProcCancelFindAdvertisement(const qcc::String& endpointName, const qcc::String& matching, TransportMask transports);

    /**
     * Process a request to discover a matching advertisement by a set of transports
     *
     * @param status               The result of parsing one of FindAdvertisedName, FindAdvertisedNameByTransport, or
     *                             FindAdvertisementByTransport.
     * @param msg                  The incoming message
     * @param matching             Key, value match criteria that the caller wants to be notified of (via signal)
     *                             when a remote Bus instance is found with an advertisement that matches the criteria.
     * @param transports           Transport bit mask
     */
    void ProcFindAdvertisement(QStatus status, Message& msg, const qcc::String& matching, TransportMask transports);

    /**
     * Handle a request to cancel the discovery a name prefix by a set of transports
     *
     * @param status               The result of parsing one of CancelFindAdvertisedName, CancelFindAdvertisedNameByTransport, or
     *                             CancelFindAdvertisementByTransport.
     * @param msg                  The incoming message
     * @param matching             Key, value match criteria to be removed from discovery list
     * @param transports           Transport bit mask
     */
    void HandleCancelFindAdvertisement(QStatus status, Message& msg, const qcc::String& matching, TransportMask transports);

    /**
     * Validate and normalize a transport specification string.  Given a
     * transport specification, convert it into a form which is guaranteed to
     * have a one-to-one relationship with a transport.  (This is just a
     * convenient inline wrapper for internal use).
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   [OUT] Normalized transport connect spec.
     * @param argMap    [OUT] Normalized parameter map.
     * @return ER_OK if successful.
     */
    QStatus NormalizeTransportSpec(const char* inSpec,
                                   qcc::String& outSpec,
                                   std::map<qcc::String, qcc::String>& argMap)
    {
        return bus.GetInternal().GetTransportList().NormalizeTransportSpec(inSpec, outSpec, argMap);
    }

    /**
     * Helper method used to shutdown a remote endpoint while preserving it's open file descriptor.
     * This is used for converting a RemoteEndpoint into a raw streaming socket.
     *
     * @param b2bEp    Bus to bus endpoint being shutdown.
     * @param sockFd   [OUT] b2bEp's socket descriptor.
     * @return   ER_OK if successful.
     */
    QStatus ShutdownEndpoint(RemoteEndpoint& b2bEp, qcc::SocketFd& sockFd);

    /**
     * Utility function used to clean up the session map when a session participant.
     * leaves a session.
     *
     * @param endpoint          Endpoint (virtual or remote) that has left(or is being removed from) the session.
     * @param id                Session id.
     * @param sendSessionLost   Whether to send a SessionLost to this endpoint.
     *                          Set to true if this endpoint is being forcefully removed from the session by the binder.
     * @return   returns true if epName is still being used
     */
    bool RemoveSessionRefs(const char* epName, SessionId id, bool sendSessionLost = false, LeaveSessionType lst = LEAVE_SESSION);

    /**
     * Utility function used to clean up the session map when a virtual endpoint with a
     * given b2b endpoint leaves a session.
     *
     * This utility is used when the given B2B ep has closed for some reason and we
     * need to clean any virtual endpoints that might have been using that b2b ep
     * from the sessionMap
     *
     * @param vepName       Virtual that should be cleaned from sessionMap if it routes
     *                      through b2bEp for a given session.
     * @param b2bEpName     B2B endpoint that vep must route through in order to be cleaned.
     */
    void RemoveSessionRefs(const qcc::String& vepName, const qcc::String& b2bEpName);

    /**
     * Remove entry from advertise alias map.
     *
     * @param name    Name to remove from advAliasMap.
     * @param mask    Set of transports whose advertisement for name will be removed from alias map.
     */
    void CleanAdvAliasMap(const qcc::String& name, TransportMask mask);

    /**
     * Check if this guid has advertised names or we are in a session with this guid
     * @param guid    Long Guid string which needs to be checked
     * @return true if there are names in namemap from this guid or there is at least one active session to it
     */
    bool IsGuidLongStringKnown(qcc::String& guid);

    /**
     * Check if this guid has advertised names or we are in a session with this guid
     * @param guid    Short Guid string which needs to be checked
     * @return true if there are names in namemap from this guid or there is at least one active session to it
     */
    bool IsGuidShortStringKnown(qcc::String& guid);


    /* TODO document */
    void PingReplyMethodHandler(Message& reply, void* context);
    void PingReplyMethodHandlerUsingCode(Message& msg, uint32_t replyCode);
    void PingReplyTransportHandler(Message& reply, void* context);

    bool QueryHandler(TransportMask transport, MDNSPacket query, uint16_t recvPort,
                      const qcc::IPEndpoint& ns4);
    bool ResponseHandler(TransportMask transport, MDNSPacket response, uint16_t recvPort);
    void PingResponse(TransportMask transport, const qcc::IPEndpoint& ns4, const qcc::String& name, uint32_t replyCode);

    std::multimap<std::pair<qcc::String, qcc::String>, OutgoingPingInfo> outgoingPingMap;
    std::multimap<qcc::String, IncomingPingInfo> incomingPingMap;
    std::set<std::pair<qcc::String, qcc::String> > dbusPingsInProgress; //contains the caller of ping and destination of ping
    TransportMask GetCompleteTransportMaskFilter();
    void SendIPNSResponse(qcc::String name, uint32_t replyCode);
    bool IsSelfJoinSupported(BusEndpoint& joinerEp) const;
};

}

#endif
