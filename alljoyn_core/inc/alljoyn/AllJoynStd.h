#ifndef _ALLJOYN_ALLJOYNSTD_H
#define _ALLJOYN_ALLJOYNSTD_H
/**
 * @file
 * This file provides definitions for AllJoyn interfaces
 */

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

#include <qcc/platform.h>

#include <alljoyn/InterfaceDescription.h>

/**
 * @def QCC_MODULE
 * Internal usage
 */
#define QCC_MODULE  "ALLJOYN"

/** Router-to-router protocol version number */
#define ALLJOYN_PROTOCOL_VERSION  12

namespace ajn {


namespace org {
namespace alljoyn {

/** Interface definitions for org.alljoyn.About */
namespace About {

extern const char* ObjectPath;        /**< Object path */
extern const char* InterfaceName;     /**< Interface name */
extern const char* WellKnownName;     /**< Well-known bus name */

}

/** Interface definitions for org.alljoyn.Icon */
namespace Icon {

extern const char* ObjectPath;        /**< Object path */
extern const char* InterfaceName;     /**< Interface name */
extern const char* WellKnownName;     /**< Well known bus name */

}

/** Interface definitions for org.alljoyn.Bus */
namespace Bus {

extern const char* ErrorName;                     /**< Standard AllJoyn error name */
extern const char* ObjectPath;                    /**< Object path */
extern const char* InterfaceName;                 /**< Interface name */
extern const char* WellKnownName;                 /**< Well known bus name */
extern const char* Secure;                        /**< Secure interface annotation */

/** Interface definitions for org.alljoyn.Bus.Peer.* */
namespace Peer {
extern const char* ObjectPath;                         /**< Object path */
namespace HeaderCompression {
extern const char* InterfaceName;                      /**<Interface name */
}
namespace Authentication {
extern const char* InterfaceName;                      /**<Interface name */
}
namespace Session {
extern const char* InterfaceName;                      /**<Interface name */
}
}
}

/** Interface definitions for org.alljoyn.Daemon */
namespace Daemon {

extern const char* ErrorName;                     /**< Standard AllJoyn error name */
extern const char* ObjectPath;                    /**< Object path */
extern const char* InterfaceName;                 /**< Interface name */
extern const char* WellKnownName;                 /**< Well known bus name */

namespace Debug {
extern const char* ObjectPath;                    /**< Object path */
extern const char* InterfaceName;                 /**< Interface name */
}
}

QStatus CreateInterfaces(BusAttachment& bus);          /**< Create the org.alljoyn.* interfaces and sub-interfaces */
}

namespace allseen {

/** Interface definitions for org.allseen.Introspectable */
namespace Introspectable {

extern const char* IntrospectDocType;                 /**< Type of extended (with descriptions) introspection document */
extern const char* InterfaceName;                 /**< Interface name */

}
}

}

/**
 * @anchor BindSessionPortReplyAnchor
 * @name org.alljoyn.Bus.BindSessionPort
 *  Interface: org.alljoyn.Bus
 *  Method: UINT32 disposition SessionPort outPort BindSessionPort(SessionPort inPort, bool isMultipoint, SessionOpts opts)
 *
 * Create a named session for other bus nodes to join.
 *
 * In params:
 *  inPort       - Session Port number to bind to or SESSION_PORT_ANY to have router allocate an available port number.
 *  isMultipoint - true iff session supports more than two participants.
 *  opts         - Session options
 *
 * Out params:
 *  disposition  - BindSessionPort return value (see below).
 *  outPort      - Bound Session port. (equal to inPort if inPort != SESSION_PORT_ANY)
 */
// @{
/* org.alljoyn.Bus.BindSessionPort */
#define ALLJOYN_BINDSESSIONPORT_REPLY_SUCCESS         1   /**< BindSessionPort reply: Success */
#define ALLJOYN_BINDSESSIONPORT_REPLY_ALREADY_EXISTS  2   /**< BindSessionPort reply: SessionPort already exists */
#define ALLJOYN_BINDSESSIONPORT_REPLY_FAILED          3   /**< BindSessionPort reply: Failed */
#define ALLJOYN_BINDSESSIONPORT_REPLY_INVALID_OPTS    4   /**< BindSessionPort reply: Invalid SessionOpts */
// @}

/**
 * @anchor UnbindSessionPortReplyAnchor
 * @name org.alljoyn.Bus.UnbindSessionPort
 *  Interface: org.alljoyn.Bus
 *  Method: UINT32 disposition UnbindSessionPort(SessionPort port)
 *
 * Cancel a session port binding.
 *
 * In params:
 *  inPort       - Session Port number to unbind.
 *
 * Out params:
 *  disposition  - UnbindSessionPort return value (see below).
 */
// @{
/* org.alljoyn.Bus.UnbindSessionPort */
#define ALLJOYN_UNBINDSESSIONPORT_REPLY_SUCCESS   1   /**< UnbindSessionPort reply: Success */
#define ALLJOYN_UNBINDSESSIONPORT_REPLY_BAD_PORT  2   /**< UnbindSessionPort reply: Unknown session port */
#define ALLJOYN_UNBINDSESSIONPORT_REPLY_FAILED    3   /**< UnbindSessionPort reply: Failed */
// @}

/**
 * @anchor JoinSessionReplyAnchor
 * @name org.alljoyn.Bus.JoinSession
 *  Interface: org.alljoyn.Bus
 *  Method: UINT32 status, UINT32 sessionId, SessionOpts outOpts JoinSession(String sessionHost, SessionPort sessionPort, SessionOptions inOpts)
 *
 * Send a JoinSession request to a remote bus name.
 *
 * In params:
 *  sessionHost   - Bus name of endpoint that is hosting the session.
 *  sessionPort   - Session port number bound by sessionHost.
 *  inOpts - Desired session options.
 *
 * Out params:
 *  status      - JoinSession return value (see below).
 *  sessionId   - Session id.
 *  outOpts     - Actual (final) session options.
 */
// @{
/* org.alljoyn.Bus.JoinSession */
#define ALLJOYN_JOINSESSION_REPLY_SUCCESS              1   /**< JoinSession reply: Success */
#define ALLJOYN_JOINSESSION_REPLY_NO_SESSION           2   /**< JoinSession reply: Session with given name does not exist */
#define ALLJOYN_JOINSESSION_REPLY_UNREACHABLE          3   /**< JoinSession reply: Failed to find suitable transport */
#define ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED       4   /**< JoinSession reply: Connect to advertised address */
#define ALLJOYN_JOINSESSION_REPLY_REJECTED             5   /**< JoinSession reply: The session creator rejected the join req */
#define ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS     6   /**< JoinSession reply: Failed due to session option incompatibilities */
#define ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED       7   /**< JoinSession reply: Caller has already joined this session */
#define ALLJOYN_JOINSESSION_REPLY_FAILED              10   /**< JoinSession reply: Failed for unknown reason */
// @}

/**
 * @anchor LeaveSessionReplyAnchor
 * @name org.alljoyn.Bus.LeaveSession
 *  Interface: org.alljoyn.Bus
 *  Method: void LeaveSession(UINT32 sessionId)
 *
 * Leave a previously joined session.
 *
 * In params:
 *  sessionId    - Id of session to leave.
 */
#define ALLJOYN_LEAVESESSION_REPLY_SUCCESS            1   /**< LeaveSession reply: Success */
#define ALLJOYN_LEAVESESSION_REPLY_NO_SESSION         2   /**< LeaveSession reply: Session with given name does not exist */
#define ALLJOYN_LEAVESESSION_REPLY_FAILED             3   /**< LeaveSession reply: Failed for unspecified reason */

/**
 * @name org.alljoyn.Bus.AdvertiseName
 *  Interface: org.alljoyn.Bus
 *  Method: UINT32 AdvertiseName(String wellKnownName)
 *
 *  Request the local router to advertise the already obtained well-known attachment name to other
 *  AllJoyn instances that might be interested in connecting to the named service.
 *
 *  wellKnownName = Well-known name of the attachment that wishes to be advertised to remote AllJoyn instances.
 *
 *  Returns a status code (see below) indicating success or failure.
 */
// @{
/* org.alljoyn.Bus.AdvertiseName */
#define ALLJOYN_ADVERTISENAME_REPLY_SUCCESS               1   /**< AdvertiseName reply: Success */
#define ALLJOYN_ADVERTISENAME_REPLY_ALREADY_ADVERTISING   2   /**< AdvertiseName reply: This endpoint has already requested advertising this name */
#define ALLJOYN_ADVERTISENAME_REPLY_FAILED                3   /**< AdvertiseName reply: Advertise failed */
#define ALLJOYN_ADVERTISENAME_REPLY_TRANSPORT_NOT_AVAILABLE 4 /**< AdvertiseName reply: The specified transport is unavailable for advertising */
// @}

/**
 * @name org.alljoyn.Bus.CancelAdvertise
 *  Interface: org.alljoyn.Bus
 *  Method: CancelAdvertiseName(String wellKnownName)
 *
 *  wellKnownName = Well-known name of the attachment that should end advertising.
 *
 *  Request the local router to stop advertising the well-known attachment name to other
 *  AllJoyn instances. The well-known name must have previously been advertised via a call
 *  to org.alljoyn.Bus.Advertise().
 *
 *  Returns a status code (see below) indicating success or failure.
 */
// @{
/* org.alljoyn.Bus.CancelAdvertise */
#define ALLJOYN_CANCELADVERTISENAME_REPLY_SUCCESS         1   /**< CancelAdvertiseName reply: Success */
#define ALLJOYN_CANCELADVERTISENAME_REPLY_FAILED          2   /**< CancelAdvertiseName reply: Advertise failed */
// @}

/**
 * @name org.alljoyn.Bus.FindAdvertisedName
 *  Interface: org.alljoyn.Bus
 *  Method: FindAdvertisedName(String wellKnownNamePrefix)
 *
 *  wellKnownNamePrefix = Well-known name prefix of the attachment that client is interested in.
 *
 *  Register interest in a well-known attachment name being advertised by a remote AllJoyn instance.
 *  When the local AllJoyn router receives such an advertisement it will send an org.alljoyn.Bus.FoundAdvertisedName
 *  signal. This attachment can then choose to ignore the advertisement or to connect to the remote Bus by
 *  calling org.alljoyn.Bus.Connect().
 *
 *  Returns a status code (see below) indicating success or failure.
 */
// @{
/* org.alljoyn.Bus.FindAdvertisedName */
#define ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS                1   /**< FindAdvertisedName reply: Success */
#define ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING    2   /**< FindAdvertisedName reply: This enpoint has already requested discover for name */
#define ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED                 3   /**< FindAdvertisedName reply: Failed */
#define ALLJOYN_FINDADVERTISEDNAME_REPLY_TRANSPORT_NOT_AVAILABLE 4  /**< FindAdvertisedName reply: The specified transport is unavailable for discovery */
// @}

/**
 * @name org.alljoyn.Bus.CancelFindAdvertisedName
 *  Interface: org.alljoyn.Bus
 *  Method: CancelFindAdvertisedName(String wellKnownName)
 *
 *  wellKnownName = Well-known name of the attachment that client is no longer interested in.
 *
 *  Cancel interest in a well-known attachment name that was previously included in a call
 *  to org.alljoyn.Bus.FindAdvertisedName().
 *
 *  Returns a status code (see below) indicating success or failure.
 */
// @{
/* org.alljoyn.Bus.CancelDiscover */
#define ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_SUCCESS          1   /**< CancelFindAdvertisedName reply: Success */
#define ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED           2   /**< CancelFindAdvertisedName reply: Failed */
// @}

/**
 * @name org.alljoyn.Bus.GetSessionFd
 *  Interface: org.alljoyn.Bus
 *  Method: Handle GetSessionFd(uint32_t sessionId)
 *
 *  sessionId - Existing sessionId for a streaming (non-message based) session.
 *
 *  Get the socket descriptor for an existing session that was created or joined with
 *  traffic type equal to RAW_UNRELIABLE or RAW_RELIABLE.
 *
 *  Returns the socket descriptor request or an error response
 */

/**
 * @name org.alljoyn.Bus.SetLinkTimeout
 *  Interface: org.alljoyn.Bus
 *  Method: SetLinkTimeout(uint32_t sessionId, uint32_t linkTimeout)
 *
 *  Input params:
 *     sessionId - Id of session whose link timeout will be modified.
 *
 *     linkTimeout - Max number of seconds that a link can be unresponsive before being
 *                   declared lost. 0 indicates that AllJoyn link monitoring will be disabled.
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_SETLINKTMEOUT_* dispositions listed below
 *
 *     replyLinkTimeout - On successful disposition, this value will contain the resulting
 *                        (possibly upward) adjusted linkTimeout value that is acceptable
 *                        to the underlying transport.
 *
 */
// @{
/* org.alljoyn.Bus.SetLinkTimeout */
#define ALLJOYN_SETLINKTIMEOUT_REPLY_SUCCESS          1   /**< SetLinkTimeout reply: Success */
#define ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT  2   /**< SetLinkTimeout reply: Destination endpoint does not support link monitoring */
#define ALLJOYN_SETLINKTIMEOUT_REPLY_NO_SESSION       3   /**< SetLinkTimeout reply: Session with given id does not exist */
#define ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED           4   /**< SetLinkTimeout reply: Failed */
// @}

/**
 * @name org.alljoyn.Bus.AliasUnixUser
 *  Interface: org.alljoyn.Bus
 *  Method: AliasUnixUser(uint32_t aliasUID)
 *
 *  Input params:
 *     aliasUID - The alias user id.
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_ALIASUNIXUSER_* dispositions listed below
 *
 */
// @{
/* org.alljoyn.Bus.AliasUnixUser */
#define ALLJOYN_ALIASUNIXUSER_REPLY_SUCCESS          1   /**< AliasUnixUser reply: Success */
#define ALLJOYN_ALIASUNIXUSER_REPLY_FAILED           2   /**< AliasUnixUser reply: Failed*/
#define ALLJOYN_ALIASUNIXUSER_REPLY_NO_SUPPORT       3   /**< AliasUnixUser reply: Failed*/
// @}

/**
 * @name org.alljoyn.Bus.OnAppSuspend
 *  Interface: org.alljoyn.Bus
 *  Method: OnAppSuspend()
 *
 *  Input params:
 *     None
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_ONAPPSUSPEND_* dispositions listed below
 *
 */
// @{
/* org.alljoyn.Bus.OnAppSuspend */
#define ALLJOYN_ONAPPSUSPEND_REPLY_SUCCESS           1   /**< OnAppSuspend reply: Success */
#define ALLJOYN_ONAPPSUSPEND_REPLY_FAILED            2   /**< OnAppSuspend reply: Failed */
#define ALLJOYN_ONAPPSUSPEND_REPLY_NO_SUPPORT        3   /**< OnAppSuspend reply: Not Supported */
// @}

/**
 * @name org.alljoyn.Bus.OnAppResume
 *  Interface: org.alljoyn.Bus
 *  Method: OnAppResume()
 *
 *  Input params:
 *     None
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_ONAPPRESUME_* dispositions listed below
 *
 */
// @{
/* org.alljoyn.Bus.OnAppResume */
#define ALLJOYN_ONAPPRESUME_REPLY_SUCCESS           1   /**< OnAppResume reply: Success */
#define ALLJOYN_ONAPPRESUME_REPLY_FAILED            2   /**< OnAppResume reply: Failed */
#define ALLJOYN_ONAPPRESUME_REPLY_NO_SUPPORT        3   /**< OnAppResume reply: Not Supported */
// @}

/**
 * Collection of Session Port numbers defined for org.alljoyn endpoint.
 */
// @{
#define ALLJOYN_BTCONTROLLER_SESSION_PORT 0x0001  /**< Session port used by BT topology manager (router-to-router use only) */
// @}

/**
 * @name org.alljoyn.Bus.CancelSessionlessMessage
 *  Interface: org.alljoyn.Bus
 *  Method: CancelSessionlessMessage(uint32_t serialNubmer)
 *
 *  Input params:
 *     serialNumber - Serial number of the message to remove from the store/forward cache.
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_CANCELSESSIONLESS_* dispositions listed below
 *
 */
// @{
/* org.alljoyn.Bus.CancelSessionlessMessage */
#define ALLJOYN_CANCELSESSIONLESS_REPLY_SUCCESS      1   /**< CancelSessionlessMessage reply: Success */
#define ALLJOYN_CANCELSESSIONLESS_REPLY_NO_SUCH_MSG  2   /**< CancelSessionlessMessage reply: Message with given serial num not found */
#define ALLJOYN_CANCELSESSIONLESS_REPLY_NOT_ALLOWED  3   /**< CancelSessionlessMessage reply: Caller is not allowed to cancel msg */
#define ALLJOYN_CANCELSESSIONLESS_REPLY_FAILED       4   /**< CancelSessionlessMessage reply: Failed for unspecified reason */
// @}

/**
 * @name org.alljoyn.Bus.RemoveSessionMember
 *  Interface: org.alljoyn.Bus
 *  Method: RemoveSessionMember(uint32_t sessionId, String memberName)
 *
 *  Input params:
 *     sessionId - Session from which the member needs to be removed.
 *     memberName - Name of member to remove from the session.
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_REMOVESESSIONMEMBER_* dispositions listed below
 *
 */
// @{
/* org.alljoyn.Bus.RemoveSessionMember */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_SUCCESS                    1   /**< RemoveSessionMember reply: Success */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_NO_SESSION                 2   /**< RemoveSessionMember reply: Session with sender and session ID does not exist. */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_NOT_BINDER                 3   /**< RemoveSessionMember reply: Session was found, but sender is not the binder */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_NOT_MULTIPOINT             4   /**< RemoveSessionMember reply: Session was found, but is not multipoint */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_NOT_FOUND                  5   /**< RemoveSessionMember reply: Session was found, but the specified session member was not found */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_INCOMPATIBLE_REMOTE_DAEMON 6   /**< RemoveSessionMember reply: Session was found, but the remote router does not support this feature */
#define ALLJOYN_REMOVESESSIONMEMBER_REPLY_FAILED                     7   /**< RemoveSessionMember reply: Failed for unspecified reason */
// @}

/**
 * @name org.alljoyn.Bus.GetHostInfo
 *  Interface: org.alljoyn.Bus
 *  Method: GetHostInfo(uint32_t sessionId)
 *
 *  Input params:
 *     sessionId - Session for which the host IP needs to be obtained.
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_GETHOSTINFO_* dispositions listed below
 *     ipAddr      - IP Address of the Host
 *
 */
// @{
/* org.alljoyn.Bus.GetHostInfo */
#define ALLJOYN_GETHOSTINFO_REPLY_SUCCESS                    1   /**< GetHostInfo reply: Success */
#define ALLJOYN_GETHOSTINFO_REPLY_NO_SESSION                 2   /**< GetHostInfo reply: Session with the specified session ID does not exist. */
#define ALLJOYN_GETHOSTINFO_REPLY_IS_BINDER                  3   /**< GetHostInfo reply: Session was found, but sender is the binder, so this is not allowed */
#define ALLJOYN_GETHOSTINFO_REPLY_NOT_SUPPORTED_ON_TRANSPORT 4   /**< GetHostInfo reply: Session was found, but this method call is not supported on the transport this session is on */
#define ALLJOYN_GETHOSTINFO_REPLY_FAILED                     5   /**< GetHostInfo reply: Failed for unspecified reason */
// @}

/**
 * @name org.alljoyn.Bus.Ping
 *  Interface: org.alljoyn.Bus
 *  Method: Ping(String busName)
 *
 *  busName = Unique or Well-known name of object you want to ping.
 *
 *  Can the busName provided be connected with
 *
 *  Returns a status code (see below) indicating success or failure.
 */
// @{
/* org.alljoyn.Bus.Ping */
#define ALLJOYN_PING_REPLY_SUCCESS          1   /**< Ping reply: Success */
#define ALLJOYN_PING_REPLY_FAILED           2   /**< Ping reply: Failed */
#define ALLJOYN_PING_REPLY_TIMEOUT          3   /**< Ping reply: Timed out */
#define ALLJOYN_PING_REPLY_UNKNOWN_NAME     4   /**< Ping reply: No route */
#define ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE    5   /**< Ping reply: Ping not supported on remote routing node */
#define ALLJOYN_PING_REPLY_UNREACHABLE      6   /**< Ping reply: Unreachable */
#define ALLJOYN_PING_REPLY_IN_PROGRESS      7   /**< Ping reply: Ping already in progress */
// @}

/** Reason why MPSessionChangedReason is called */
// @{
#define ALLJOYN_MPSESSIONCHANGED_LOCAL_MEMBER_ADDED 0 /** You were added to this session (catch up) */
#define ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_ADDED 1 /** Another member was added to this session */
#define ALLJOYN_MPSESSIONCHANGED_LOCAL_MEMBER_REMOVED 2 /** You were removed to this session (see all remaining members removed) */
#define ALLJOYN_MPSESSIONCHANGED_REMOTE_MEMBER_REMOVED 3 /** Another member was removed from this session */
// @}


/** Indication to which side the SessionLost applies */
// @{
#define ALLJOYN_SESSIONLOST_DISPOSITION_HOST 0 /** Session was lost for the host side of the leaf node */
#define ALLJOYN_SESSIONLOST_DISPOSITION_MEMBER 1 /** Session was lost for the joiner side of the leaf node */
// @}


/**
 * @name org.alljoyn.Bus.SetIdleTimeouts
 *  Interface: org.alljoyn.Bus
 *  Method: SetIdleTimeouts(uint32_t inIdleTO, uint32_t inProbeTO)
 *
 *  Input params:
 *     reqIdleTO -  Requested Idle Timeout for the link. i.e. time after which the Routing node
 *                  must send a DBus ping to Leaf node in case of inactivity.
 *                  Use 0 to leave unchanged.
 *     reqProbeTO - Requested Probe timeout. The time from the Routing node sending the DBus
 *                  ping to the expected response from the leaf node.
 *                  Use 0 to leave unchanged.
 *
 *  Output params:
 *     disposition - One of the ALLJOYN_SETIDLETIMEOUTS_* dispositions listed below
 *     actIdleTO - Actual idle Timeout for the link that was set. i.e. time after which the Routing node
 *                 will send a DBus ping to Leaf node in case of inactivity.
 *     actProbeTO - Actual probe timeout i.e. The time from the Routing node sending the DBus ping
 *                 to the expected response from the leaf node.
 *
 */
// @{
/* org.alljoyn.Bus.SetIdleTimeouts */
#define ALLJOYN_SETIDLETIMEOUTS_REPLY_SUCCESS          1   /**< SetIdleTimeouts reply: Success */
#define ALLJOYN_SETIDLETIMEOUTS_REPLY_NOT_ALLOWED      2   /**< SetIdleTimeouts reply: Not allowed for bus-to-bus and Null endpoints */
#define ALLJOYN_SETIDLETIMEOUTS_REPLY_FAILED           3   /**< SetIdleTimeouts reply: Failed */
// @}

}

#undef QCC_MODULE

#endif
