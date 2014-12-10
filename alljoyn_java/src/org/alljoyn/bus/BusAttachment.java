/*
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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
 */

package org.alljoyn.bus;

import java.lang.ref.WeakReference;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.alljoyn.bus.AuthListener.AuthRequest;
import org.alljoyn.bus.AuthListener.CertificateRequest;
import org.alljoyn.bus.AuthListener.Credentials;
import org.alljoyn.bus.AuthListener.ExpirationRequest;
import org.alljoyn.bus.AuthListener.LogonEntryRequest;
import org.alljoyn.bus.AuthListener.PasswordRequest;
import org.alljoyn.bus.AuthListener.PrivateKeyRequest;
import org.alljoyn.bus.AuthListener.UserNameRequest;
import org.alljoyn.bus.AuthListener.VerifyRequest;
import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;

/**
 * A connection to a message bus.
 * Using BusAttachment, an application may register objects on the bus for other
 * bus connections to access. BusAttachment is also used to access remote
 * objects that have been registered by other processes and/or remote devices.
 */
public class BusAttachment {

    /**
     * @deprecated
     * Emit PropertiesChanged to signal the bus that this property has been updated
     *
     * @param busObject The BusObject that is the source of this signal
     * @param ifcName   The name of the interface
     * @param propName  The name of the property being changed
     * @param val       The new value of the property
     * @param sessionId Id of the session we broadcast to (0 for all)
     */
    @Deprecated
    public native void emitChangedSignal(BusObject busObject, String ifcName, String propName, Object val, int sessionId);


    /**
     * Request a well-known name.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.RequestName method call to the local router
     * and interprets the response.
     *
     * @param name    Well-known name being requested.
     * @param flags   Bitmask name flag (see DBusStd.h)
     *                  <ul>
     *                  <li>ALLJOYN_NAME_FLAG_ALLOW_REPLACEMENT</li>
     *                  <li>ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING</li>
     *                  <li>ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE</li>
     *                  </ul>
     *
     * @return
     * <ul>
     * <li>OK if request completed successfully and primary ownership was granted.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status requestName(String name, int flags);

    /**
     * Value for requestName flags bit corresponding to allowing another bus
     * attachment to take ownership of this name.
     */
    public static final int ALLJOYN_NAME_FLAG_ALLOW_REPLACEMENT = 0x01;

    /**
     * Value for requestName flags bit corresponding to a request to take
     * ownership of the name in question if it is already taken.
     */
    public static final int ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING = 0x02;

    /**
     * Value for requestName flags bit corresponding to a request to
     * fail if the name in question cannot be immediately obtained.
     */
    public static final int ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE = 0x04;

    /**
     * Release a previously requeted well-known name.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.ReleaseName method call to the local router
     * and interprets the response.
     *
     * @param name  Well-known name being released.
     *
     * @return
     * <ul>
     * <li>OK if the name was released.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status releaseName(String name);

    /**
     * Add a DBus match rule.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch method call to the local router.
     *
     * @param rule  Match rule to be added (see the DBus specification for the
     *              format of this string).
     *
     * @return
     * <ul>
     * <li>OK if the match rule was added.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure</li>
     * </ul>
     */
    public native Status addMatch(String rule);

    /**
     * Remove a DBus match rule.
     * This method is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch method call to the local router.
     *
     * @param rule  Match rule to be removed (see the DBus specification for the
     *              format of this string).
     *
     * @return
     * <ul>
     * <li>OK if the match rule was removed.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure</li>
     * </ul>
     */
    public native Status removeMatch(String rule);

    /**
     * Advertise the existence of a well-known name to other (possibly disconnected) AllJoyn routers.
     *
     * This method is a shortcut/helper that issues an org.allJoyn.Bus.AdvertisedName method call to the local router
     * and interprets the response.
     *
     * @param name        The well-known name to advertise. (Must be owned by the caller via RequestName).
     * @param transports  Set of transports to use for sending advertisment.
     *
     * @return
     * <ul>
     * <li>OK if the name was advertised.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure </li>
     * </ul>
     */
    public native Status advertiseName(String name, short transports);

    /**
     * Stop advertising the existence of a well-known name to other AllJoyn routers.
     *
     * This method is a shortcut/helper that issues an
     * org.allJoyn.Bus.CancelAdvertiseName method call to the local
     * router and interprets the response.
     *
     * @param name        A well-known name that was previously advertised via AdvertiseName.
     * @param transports  Set of transports whose name advertisment will be cancelled.
     *
     * @return
     * <ul>
     * <li>OK if the name advertisements were stopped.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>BUS_MATCH_RULE_NOT_FOUND if interfaces added using the WhoImplements method were not found.</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status cancelAdvertiseName(String name, short transports);

    /**
     * Register interest in a well-known name prefix for the purpose of discovery over transports included in TRANSPORT_ANY.
     * This method is a shortcut/helper that issues an org.allJoyn.Bus.FindAdvertisedName method call to the local router
     * and interprets the response.
     *
     * @param namePrefix  Well-known name prefix that application is interested in receiving BusListener::FoundAdvertisedName
     *                    notifications about.
     *
     * @return
     * <ul>
     * <li>OK if discovery was started.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status findAdvertisedName(String namePrefix);

    /**
     * Register interest in a well-known name prefix for the purpose of discovery over specified transports.
     * This method is a shortcut/helper that issues an org.allJoyn.Bus.FindAdvertisedName method call to the local router
     * and interprets the response.
     *
     * @param namePrefix  Well-known name prefix that application is interested in receiving BusListener::FoundAdvertisedName
     *                    notifications about.
     * @param transports  Set of transports over which discovery will be enabled.
     *
     * @return
     * <ul>
     * <li>OK if discovery was started.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status findAdvertisedNameByTransport(String namePrefix, short transports);

    /**
     * Cancel interest in a well-known name prefix that was previously
     * registered with FindAdvertisedName. This cancels well-known name discovery
     * over transports included in TRANSPORT_ANY.  This method is a shortcut/helper
     * that issues an org.allJoyn.Bus.CancelFindAdvertisedName method
     * call to the local router and interprets the response.
     *
     * @param namePrefix  Well-known name prefix that application is no longer interested in receiving
     *                    BusListener::FoundAdvertisedName notifications about.
     *
     * @return
     * <ul>
     * <li>OK if discovery was cancelled.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a general failure condition.</li>
     * </ul>
     */
    public native Status cancelFindAdvertisedName(String namePrefix);

    /**
     * Cancel interest in a well-known name prefix that was previously
     * registered with FindAdvertisedName. This cancels well-known name discovery
     * over the specified transports.  This method is a shortcut/helper
     * that issues an org.allJoyn.Bus.CancelFindAdvertisedName method
     * call to the local router and interprets the response.
     *
     * @param namePrefix  Well-known name prefix that application is no longer interested in receiving
     *                    BusListener::FoundAdvertisedName notifications about.
     * @param transports  Set of transports over which discovery will be cancelled.
     *
     * @return
     * <ul>
     * <li>OK if discovery was cancelled.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a general failure condition.</li>
     * </ul>
     */
    public native Status cancelFindAdvertisedNameByTransport(String namePrefix, short transports);

    /**
     * Make a SessionPort available for external BusAttachments to join.
     *
     * Each BusAttachment binds its own set of SessionPorts. Session joiners use
     * the bound session port along with the name of the attachement to create a
     * persistent logical connection (called a Session) with the original
     * BusAttachment.
     *
     * A SessionPort and bus name form a unique identifier that BusAttachments
     * use when joining a session.  SessionPort values can be pre-arranged
     * between AllJoyn services and their clients (well-known SessionPorts).
     *
     * Once a session is joined using one of the service's well-known
     * SessionPorts, the service may bind additional SessionPorts (dyanamically)
     * and share these SessionPorts with the joiner over the original
     * session. The joiner can then create additional sessions with the service
     * by calling JoinSession with these dynamic SessionPort ids.
     *
     * @param sessionPort SessionPort value to bind or SESSION_PORT_ANY to allow
     *                    this method to choose an available port. On successful
     *                    return, this value contains the chosen SessionPort.
     *
     * @param opts        Session options that joiners must agree to in order to
     *                    successfully join the session.
     *
     * @param listener    SessionPortListener that will be notified via callback
     *                    when a join attempt is made on the bound session port.
     *
     * @return
     * <ul>
     * <li>OK if the new session port was bound.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status bindSessionPort(Mutable.ShortValue sessionPort,
            SessionOpts opts,
            SessionPortListener listener);

    /**
     * Set a Translator for all BusObjects and InterfaceDescriptions. This Translator is used for
     * descriptions appearing in introspection. Note that any Translators set on a specific
     * InterfaceDescription or BusObject will be used for those specific elements - this Translator
     * is used only for BusObjects and InterfaceDescriptions that do not have Translators set for them.
     *
     * @param translator       The Translator instance
     */
    public native void setDescriptionTranslator(Translator translator);

    /**
     * When passed to BindSessionPort as the requested port, the system will
     * assign an ephemeral session port
     */
    public static final short SESSION_PORT_ANY = 0;

    /**
     * When passed to ProxyBusObject, the system will use any available connection.
     */
    public static final int SESSION_ID_ANY = 0;

    /**
     * When specified during SignalEmitter creation, emits on all session hosted
     * by this BusAttachment.
     */
    public static final int SESSION_ID_ALL_HOSTED = -1;

    /**
     * Cancel an existing port binding.
     *
     * @param   sessionPort    Existing session port to be un-bound.
     *
     * @return
     * <ul>
     * <li>OK if the session port was unbound.</li>
     * <li>BUS_NOT_CONNECTED if connection has not been made with the local router.</li>
     * <li>other error status codes indicating a failure</li>
     * </ul>
     */
    public native Status unbindSessionPort(short sessionPort);

    /**
     * Join a session.
     *
     * This method is a shortcut/helper that issues an
     * org.allJoyn.Bus.JoinSession method call to the local router and
     * interprets the response.
     *
     * @param sessionHost   Bus name of attachment that is hosting the session to be joined.
     * @param sessionPort   SessionPort of sessionHost to be joined.
     * @param sessionId     Set to the unique identifier for session.
     * @param opts          Set to the actual session options of the joined session.
     * @param listener      Listener to be called when session related asynchronous
     *                      events occur.
     *
     * @return
     * <ul>
     * <li>OK if the session was joined.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status joinSession(String sessionHost,
            short sessionPort,
            Mutable.IntegerValue sessionId,
            SessionOpts opts,
            SessionListener listener);

    /**
     * The JNI loader can't resolve the overloaded joinSession if both the sync and async versions
     * are native.  This is the workaround.
     */
    private native Status joinSessionAsync(String sessionHost,
            short sessionPort,
            SessionOpts opts,
            SessionListener listener,
            OnJoinSessionListener onJoinSession,
            Object context);

    /**
     * Asynchronous version of {@link #joinSession(String, short, Mutable.IntegerValue, SessionOpts,
     * SessionListener)}.
     *
     * @param sessionHost   Bus name of attachment that is hosting the session to be joined.
     * @param sessionPort   SessionPort of sessionHost to be joined.
     * @param opts          The requested session options of the session to be joined.
     * @param listener      Listener to be called when session related asynchronous
     *                      events occur.
     * @param onJoinSession Listener to be called when joinSession completes.
     * @param context       User-defined context object.  Passed through to {@link
     *                      OnJoinSessionListener#onJoinSession(Status, int, SessionOpts, Object)}.
     *
     * @return
     * <ul>
     * <li>OK if method call to local router response was was successful.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public Status joinSession(String sessionHost,
            short sessionPort,
            SessionOpts opts,
            SessionListener listener,
            OnJoinSessionListener onJoinSession,
            Object context) {
        return joinSessionAsync(sessionHost, sessionPort, opts, listener, onJoinSession, context);
    }

    /**
     * Leave an existing session.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call to the local router
     * and interprets the response.
     * This method cannot be called on self-joined session.
     *
     * @param sessionId     Session id.
     *
     * @return
     * <ul>
     * <li>OK if router response was left.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>ER_BUS_NO_SESSION if session did not exist.</li>
     * <li>other error status codes indicating failures.</li>
     * </ul>
     */
    public native Status leaveSession(int sessionId);

    /**
     * Leave an existing session as host. This function will fail if you were not the host. This method is a
     * shortcut/helper that issues an org.alljoyn.Bus.LeaveHostedSession method call to the local router and interprets
     * the response.
     *
     * @param  sessionId     Session id.
     *
     * @return
     * <ul>
     * <li>ER_OK if router response was received and the leave operation was successfully completed.</li>
     * <li>ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.</li>
     * <li>ER_BUS_NO_SESSION if session did not exist or if not host of the session.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status leaveHostedSession(int sessionId);

    /**
     * Leave an existing session as joiner. This function will fail if you were not the joiner.
     * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveJoinedSession method call to the local router
     * and interprets the response.
     *
     * @param sessionId Session id.
     *
     * @return
     * <ul>
     * <li>ER_OK if router response was received and the leave operation was successfully completed.</li>
     * <li>ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus.</li>
     * <li>ER_BUS_NO_SESSION if session did not exist or if not joiner of the session.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public native Status leaveJoinedSession(int sessionId);

    /**
     * Remove a session member from an existing multipoint session.
     *
     * This method is a shortcut/helper that issues an
     * org.alljoyn.Bus.RemoveSessionMember method call to the local router
     * and interprets the response.
     *
     * @param sessionId             Session id.
     * @param sessionMemberName     Name of member to remove from session.
     *
     * @return
     * <ul>
     * <li>OK if router response was left.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating failures.</li>
     * </ul>
     */
    public native Status removeSessionMember(int sessionId, String sessionMemberName);

    /**
     * Set the SessionListener for an existing session on both host and joiner side.
     *
     * Calling this method will override (replace) the listener set by a previoius call to
     * setSessionListener, SetHostedSessionListener, SetJoinedSessionListener or a listener specified in joinSession.
     *
     * @param sessionId    The session id of an existing session.
     * @param listener     The SessionListener to associate with the session. May be null to clear previous listener.
     * @return  ER_OK if successful.
     * @return  ER_BUS_NO_SESSION if session did not exist
     */
    public native Status setSessionListener(int sessionId, SessionListener listener);

    /**
     * Set the SessionListener for an existing sessionId on the joiner side.
     *
     * Calling this method will override the listener set by a previous call to SetSessionListener, SetJoinedSessionListener
     * or any listener specified in JoinSession.
     *
     * @param sessionId    The session id of an existing session.
     * @param listener     The SessionListener to associate with the session. May be NULL to clear previous listener.
     * @return  ER_OK if successful.
     * @return  ER_BUS_NO_SESSION if session did not exist or if not joiner side of the session
     */
    public native Status setJoinedSessionListener(int sessionId, SessionListener listener);

    /**
     * Set the SessionListener for an existing sessionId on the host side.
     *
     * Calling this method will override the listener set by a previous call to SetSessionListener or SetHostedSessionListener.
     *
     * @param sessionId    The session id of an existing session.
     * @param listener     The SessionListener to associate with the session. May be NULL to clear previous listener.
     * @return  ER_OK if successful.
     * @return  ER_BUS_NO_SESSION if session did not exist or if not host side of the session
     */
    public native Status setHostedSessionListener(int sessionId, SessionListener listener);

    /**
     * Get the file descriptor for a raw (non-message based) session.
     *
     * @param sessionId  Id of an existing streamming session.
     * @param sockFd     Socket file descriptor for session.
     *
     * @return
     * <ul>
     * <li>Status.OK if the socket FD was returned.</li>
     * <li>ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * <li>other error status codes indicating a failure.</li>
     * </ul>
     *
     */
    public native Status getSessionFd(int sessionId, Mutable.IntegerValue sockFd);

    /**
     * Set the link timeout for a session.
     *
     * Link timeout is the maximum number of seconds that an unresponsive
     * router-to-router connection will be monitored before delcaring the
     * session lost (via SessionLost callback). Link timeout defaults to 0 which
     * indicates that AllJoyn link monitoring is disabled.
     *
     * Each transport type defines a lower bound on link timeout to avoid
     * defeating transport specific power management algorithms.
     *
     * @param sessionId   Id of session whose link timeout will be modified.
     * @param linkTimeout [IN/OUT] Max number of seconds that a link can be
     *                    unresponsive before being delcared lost. 0 indicates
     *                    that AllJoyn link monitoring will be disabled. On
     *                    return, this value will be the resulting (possibly
     *                    upward) adjusted linkTimeout value that acceptible
     *                    to the underlying transport.
     * @return
     * <ul>
     * <li>Status.OK if the linkTimeout was successfully modified</li>
     * <li>ER_BUS_NOT_CONNECTED if a connection has not been made with a local bus</li>
     * </ul>
     */
    public native Status setLinkTimeout(int sessionId, Mutable.IntegerValue linkTimeout);

    /**
     * Get the peer GUID for this peer or an authenticated remote peer. Peer
     * GUIDs are used by the authentication mechanisms to uniquely and identify
     * a remote application instance. The peer GUID for a remote peer is only
     * available if the remote peer has been authenticated.
     *
     * @param name  Name of a remote peer or NULL to get the local (our) peer
     *              GUID.
     * @param guid  Mutable value that contains a reference to the returned
     *              GUID string (think C++ [out] parameter.
     *
     * @return
     * <ul>
     * <li>OK if the requested GUID was obtained</li>
     * <li>other error status codes indicating a failure</li>
     * </ul>
     */
    public native Status getPeerGUID(String name, Mutable.StringValue guid);

    /**
     * Determine if a name is present and responding by pinging it. The name can
     * be the unique or well-known name.
     *
     * @param name     The unique or well-known name to ping
     * @param timeout  Timeout specified in milliseconds to wait for reply
     *
     * @return
     * <ul>
     * <li>OK the name is present and responding</li>
     * <li>ALLJOYN_PING_REPLY_UNREACHABLE the name is no longer present</li>
     * </ul>
     * The following return values indicate that the router cannot determine if the
     * remote name is present and responding:
     * <ul>
     * <li>ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out</li>
     * <li>ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session</li>
     * <li>ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE the remote routing node does not implement Ping</li>
     * </ul>
     * The following return values indicate an error with the ping call itself:
     * <ul>
     * <li>ALLJOYN_PING_FAILED Ping failed</li>
     * <li>BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error</li>
     * <li>BUS_NOT_CONNECTED the BusAttachment is not connected to the bus</li>
     * <li>BUS_BAD_BUS_NAME the name parameter is not a valid bus name</li>
     * <li>An error status otherwise</li>
     * </ul>
     */
    public native Status ping(String name, int timeout);

    /**
     * The JNI loader can't resolve the overloaded ping if both the sync and async versions
     * are native.  This is the workaround.
     */
    private native Status pingAsync(String name,
            int timeout,
            OnPingListener onPing,
            Object context);

    /**
     * Asynchronous version of {@link #ping(String, int)}.
     *
     * @param name     The unique or well-known name to ping
     * @param timeout  Timeout specified in milliseconds to wait for reply
     * @param onPing   Listener to be called when ping completes.
     * @param context  User-defined context object.  Passed through to {@link
     *                 OnPingListener#onPing(Status, Object)}.
     *
     * @return
     * <ul>
     * <li>OK iff method call to local router response was was successful.</li>
     * <li>BUS_NOT_CONNECTED if a connection has not been made with a local bus.</li>
     * <li>BUS_BAD_BUS_NAME if the name passed in is an invalid bus name.</li>
     * <li>Other error status codes indicating a failure.</li>
     * </ul>
     */
    public Status ping(String name,
            int timeout,
            OnPingListener onPing,
            Object context) {
        return pingAsync(name, timeout, onPing, context);
    }

    /**
     * This sets the debug level of the local AllJoyn router if that router
     * was built in debug mode.
     *
     * The debug level can be set for individual subsystems or for "ALL"
     * subsystems.  Common subsystems are "ALLJOYN" for core AllJoyn code,
     * "ALLJOYN_OBJ" for the sessions management code and "ALLJOYN_NS" for the
     * TCP name services.  Debug levels for specific subsystems override the
     * setting for "ALL" subsystems.  For example if "ALL" is set to 7, but
     * "ALLJOYN_OBJ" is set to 1, then detailed debug output will be generated
     * for all subsystems expcept for "ALLJOYN_OBJ" which will only generate
     * high level debug output.  "ALL" defaults to 0 which is off, or no debug
     * output.
     *
     * The debug output levels are actually a bit field that controls what
     * output is generated.  Those bit fields are described below:
     *<ul>
     *     <li>0x1: High level debug prints (these debug prints are not common)</li>
     *     <li>0x2: Normal debug prints (these debug prints are common)</li>
     *     <li>0x4: Function call tracing (these debug prints are used
     *            sporadically)</li>
     *     <li>0x8: Data dump (really only used in the "SOCKET" module - can
     *            generate a <strong>lot</strong> of output)</li>
     * </ul>
     *
     * Typically, when enabling debug for a subsystem, the level would be set
     * to 7 which enables High level debug, normal debug, and function call
     * tracing.  Setting the level 0, forces debug output to be off for the
     * specified subsystem.
     *
     * @param module  the name of the module to generate debug output from.
     * @param level   the debug level to set for the module.
     *
     * @return
     * <ul>
     * <li>OK if debug request was successfully sent to the AllJoyn router</li>
     * <li>BUS_NO_SUCH_OBJECT if router was not built in debug mode.</li>
     * </ul>
     */
    public native Status setDaemonDebug(String module, int level);

    /**
     * Set AllJoyn logging levels.
     *
     * @param logEnv    A semicolon separated list of KEY=VALUE entries used
     *                  to set the log levels for internal AllJoyn modules.
     *                  (i.e. ALLJOYN=7;ALL=1)
     */
    public native void setLogLevels(String logEnv);

    /**
     * Set AllJoyn debug levels.
     *
     * @param module    name of the module to generate debug output
     * @param level     debug level to set for the module
     */
    public native void setDebugLevel(String module, int level);

    /**
     * Indicate whether AllJoyn logging goes to OS logger or stdout
     *
     * @param  useOSLog   true iff OS specific logging should be used rather than print for AllJoyn debug messages.
     */
    public native void useOSLogging(boolean useOSLog);

    private Set<AboutListener> registeredAboutListeners;

    /**
     * Register an object that will receive About Interface event notifications.
     *
     * @param listener  Object instance that will receive bus event notifications.
     */
    public void registerAboutListener(AboutListener listener)
    {
        if (registeredAboutListeners.isEmpty()) {
            registerSignalHandlers(this);
        }
        registeredAboutListeners.add(listener);
    }

    /**
     * unregister an object that was previously registered with registerAboutListener.
     *
     * @param listener  Object instance to un-register as a listener.
     */
    public void unregisterAboutListener(AboutListener listener)
    {
        registeredAboutListeners.remove(listener);
        if (registeredAboutListeners.isEmpty()) {
            unregisterSignalHandlers(this);
        }
    }

    /**
     * Signal handler used to process announce signals from the bus. It will
     * forward the signal to the registered AboutListener
     *
     * @param version - version of the announce signal received
     * @param port - Session Port used by the remote device
     * @param objectDescriptions - list of object paths any interfaces found at
     *                             that object path
     * @param aboutData - A dictionary containing information about the remote
     *                    device.
     */
    @BusSignalHandler(iface = "org.alljoyn.About", signal = "Announce")
    public void announce(short version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData)
    {
        for (AboutListener al : BusAttachment.this.registeredAboutListeners) {
            al.announced(BusAttachment.this.getMessageContext().sender, version, port, objectDescriptions, aboutData);
        }
    }

    /**
     * Change the announce flag for an already added interface. Changes in the
     * announce flag are not visible to other devices till Announce is called.
     *
     * @param busObject   The BusObject that the interface is registered with
     * @param intf        The interface to change the announce flag on.
     * @param isAnnounced If "true" the interface will be part of the next
     *                    Announce signal if "false" the interface will not be
     *                    part of the next Announce signal.
     *
     * @return
     *  <ul>
     *  <li>OK if successful</li>
     *  <li>BUS_OBJECT_NO_SUCH_INTERFACE if the interface is not part of the BusObject.</li>
     *  </ul>
     */
    public Status setAnnounceFlag(BusObject busObject, Class<?> intf, boolean isAnnounced) {
        return setAnnounceFlag(busObject, InterfaceDescription.getName(intf), isAnnounced);
    }

    /**
     * Change the announce flag for an already added interface. Changes in the
     * announce flag are not visible to other devices till Announce is called.
     *
     * @param busObject   The BusObject that the interface is registered with
     * @param ifcName     The name of the interface
     * @param isAnnounced if "true" the interface will be part of the next
     *                    Announce signal if "false" the interface will not be
     *                    part of the next Announce signal.
     *
     * @return
     *  <ul>
     *  <li>OK if successful</li>
     *  <li>BUS_OBJECT_NO_SUCH_INTERFACE if the interface is not part of the BusObject.</li>
     *  </ul>
     */
    public native Status setAnnounceFlag(BusObject busObject, String ifcName, boolean isAnnounced);

    /**
     * TODO cleanup the documentation make sure it is accurate remove doxygen
     * style code blocks.
     *
     * List the interfaces your application is interested in.  If a remote device
     * is announcing that interface then the all Registered AboutListeners will
     * be called.
     *
     * For example, if you need both "com.example.Audio" <em>and</em>
     * "com.example.Video" interfaces then do the following.
     * registerAboutListener once:
     * <pre>{@code
     * String interfaces[] = {"com.example.Audio", "com.example.Video"};
     * registerAboutListener(aboutListener);
     * whoImplements(interfaces);
     * }</pre>
     *
     * If the handler should be called if "com.example.Audio" <em>or</em>
     * "com.example.Video" interfaces are implemented then call
     * RegisterAboutListener multiple times:
     * <pre>{@code
     * registerAboutListener(aboutListener);
     * String audioInterface[] = {"com.example.Audio"};
     * whoImplements(interfaces);
     * whoImplements(audioInterface);
     * String videoInterface[] = {"com.example.Video"};
     * whoImplements(videoInterface);
     * }</pre>
     *
     * The interface name may be a prefix followed by a *.  Using
     * this, the example where we are interested in "com.example.Audio" <em>or</em>
     * "com.example.Video" interfaces could be written as:
     * <pre>{@code
     * String exampleInterface[] = {"com.example.*"};
     * registerAboutListener(aboutListener);
     * whoImplements(exampleInterface);
     * }</pre>
     *
     * The AboutListener will receive any announcement that implements an interface
     * beginning with the "com.example." name.
     *
     * If the same AboutListener is used for for multiple interfaces then it is
     * the listeners responsibility to parse through the reported interfaces to
     * figure out what should be done in response to the Announce signal.
     *
     * Note: specifying null for the interfaces parameter could have significant
     * impact on network performance and should be avoided unless its known that
     * all announcements are needed.
     *
     * @param interfaces a list of interfaces that the Announce
     *               signal reports as implemented. NULL to receive all Announce
     *               signals regardless of interfaces
     * @return Status.OK on success. An error status otherwise.
     */
    public native Status whoImplements(String[] interfaces);

    /**
     * Stop showing interest in the listed interfaces. Stop receiving announce
     * signals from the devices with the listed interfaces.
     *
     * Note if WhoImplements has been called multiple times the announce signal
     * will still be received for any interfaces that still remain.
     *
     * @param interfaces a list of interfaces list must match a list previously
     *                   passed to the whoImplements method.
     *
     * @return Status.OK on success. An error status otherwise.
     */
    public native Status cancelWhoImplements(String[] interfaces);

    /**
     * Register an object that will receive bus event notifications.
     *
     * @param listener  Object instance that will receive bus event notifications.
     */
    public native void registerBusListener(BusListener listener);

    /**
     * unregister an object that was previously registered with registerBusListener.
     *
     * @param listener  Object instance to un-register as a listener.
     */
    public native void unregisterBusListener(BusListener listener);

    /** The native connection handle. */
    private long handle;

    /** The connect spec. */
    private String address;

    /**
     * {@code true} if this attachment is allowed to receive messages from
     * remote devices.
     */
    private boolean allowRemoteMessages;

    private KeyStoreListener keyStoreListener;

    private class AuthListenerInternal {

        private static final int PASSWORD       = 0x0001;
        private static final int USER_NAME      = 0x0002;
        private static final int CERT_CHAIN     = 0x0004;
        private static final int PRIVATE_KEY    = 0x0008;
        private static final int LOGON_ENTRY    = 0x0010;
        private static final int EXPIRATION     = 0x0020;
        private static final int NEW_PASSWORD   = 0x1001;
        private static final int ONE_TIME_PWD   = 0x2001;

        private AuthListener authListener = null;
        private SecurityViolationListener violationListener;

        public void setAuthListener(AuthListener authListener) {
            this.authListener = authListener;
        }

        public boolean authListenerSet() {
            return authListener != null;
        }

        public void setSecurityViolationListener(SecurityViolationListener violationListener) {
            this.violationListener = violationListener;
        }

        @SuppressWarnings("unused")
        public Credentials requestCredentials(String authMechanism, String authPeer, int authCount,
                String userName, int credMask) throws BusException {
            if (authListener == null) {
                throw new BusException("No registered application AuthListener");
            }

            Credentials credentials = new Credentials();
            List<AuthRequest> requests = new ArrayList<AuthRequest>();
            if ((credMask & PASSWORD) == PASSWORD) {
                boolean isNew = (credMask & NEW_PASSWORD) == NEW_PASSWORD;
                boolean isOneTime = (credMask & ONE_TIME_PWD) == ONE_TIME_PWD;
                requests.add(new PasswordRequest(credentials, isNew, isOneTime));
            }
            if ((credMask & USER_NAME) == USER_NAME) {
                requests.add(new UserNameRequest(credentials));
            }
            if ((credMask & CERT_CHAIN) == CERT_CHAIN) {
                requests.add(new CertificateRequest(credentials));
            }
            if ((credMask & PRIVATE_KEY) == PRIVATE_KEY) {
                requests.add(new PrivateKeyRequest(credentials));
            }
            if ((credMask & LOGON_ENTRY) == LOGON_ENTRY) {
                requests.add(new LogonEntryRequest(credentials));
            }
            /*
             * Always add this as it doesn't show up in credMask, but can be set by the application.
             */
            requests.add(new ExpirationRequest(credentials));

            if (authListener.requested(authMechanism, authPeer, authCount, userName,
                                       requests.toArray(new AuthRequest[0]))) {
                return credentials;
            }
            return null;
        }

        @SuppressWarnings("unused")
        public boolean verifyCredentials(String authMechanism, String peerName, String userName,
                String cert) throws BusException {
            if (authListener == null) {
                throw new BusException("No registered application AuthListener");
            }
            /*
             * authCount is set to 0 here since it can't be cached from
             * requestCredentials, and it's assumed that the application will
             * not immediately reject a request with an authCount of 0.
             */
            return authListener.requested(authMechanism, peerName, 0, userName == null ? "" : userName,
                    new AuthRequest[] { new VerifyRequest(cert) });
        }

        @SuppressWarnings("unused")
        public void securityViolation(Status status) {
            if (violationListener != null) {
                violationListener.violated(status);
            }
        }

        @SuppressWarnings("unused")
        public void authenticationComplete(String authMechanism, String peerName,  boolean success) {
            if (authListener != null) {
                authListener.completed(authMechanism, peerName, success);
            }
        }
    }

    private AuthListenerInternal busAuthListener;

    /** End-to-end authentication mechanisms. */
    private String authMechanisms;

    /** Default key store file name. */
    private String keyStoreFileName;

    /** Specify if the default key store is shared */
    private boolean isShared;

    /** Specify if the attachment is connected */
    private boolean isConnected;

    private Method foundAdvertisedName;

    private ExecutorService executor;

    private Method lostAdvertisedName;

    private DBusProxyObj dbus;
    private ProxyBusObject dbusbo;

    /** Policy for handling messages received from remote devices. */
    public enum RemoteMessage {

        /** This attachment will not receive messages from remote devices. */
        Ignore,

        /** This attachment will receive messages from remote devices. */
        Receive
    }

    /**
     * Constructs a BusAttachment.
     *
     * @param applicationName the name of the application
     * @param policy if this attachment is allowed to receive messages from
     *               remote devices
     * @param concurrency The maximum number of concurrent method and signal
     *                    handlers locally executing.
     */
    public BusAttachment(String applicationName, RemoteMessage policy, int concurrency) {
        isShared = false;
        isConnected = false;
        this.allowRemoteMessages = (policy == RemoteMessage.Receive);
        busAuthListener = new AuthListenerInternal();
        try {
            foundAdvertisedName = getClass().getDeclaredMethod(
                    "foundAdvertisedName", String.class, Short.class, String.class);
            foundAdvertisedName.setAccessible(true);
            lostAdvertisedName = getClass().getDeclaredMethod(
                    "lostAdvertisedName", String.class, Short.class, String.class);
            lostAdvertisedName.setAccessible(true);
        } catch (NoSuchMethodException ex) {
            /* This will not happen */
        }
        create(applicationName, allowRemoteMessages, concurrency);

        /*
         * Create a separate dbus bus object (dbusbo) and interface so we get at
         * it and can quickly release its resources when we're done with it.
         * The corresponding interface (dbus) is what we give the clients.
         */
        dbusbo = new ProxyBusObject(this, "org.freedesktop.DBus", "/org/freedesktop/DBus", SESSION_ID_ANY,
                                    new Class<?>[] { DBusProxyObj.class });
        dbus = dbusbo.getInterface(DBusProxyObj.class);
        executor = Executors.newSingleThreadExecutor();
        registeredAboutListeners = new HashSet<AboutListener>();
    }

    /**
     * Constructs a BusAttachment.
     *
     * @param applicationName the name of the application
     * @param policy if this attachment is allowed to receive messages
     *               from remote devices
     */
    public BusAttachment(String applicationName, RemoteMessage policy) {
        this(applicationName, policy, DEFAULT_CONCURRENCY);
    }

    /**
     * Construct a BusAttachment that will only communicate on the local device.
     *
     * @param applicationName the name of the application
     */
    public BusAttachment(String applicationName) {
        this(applicationName, RemoteMessage.Ignore, DEFAULT_CONCURRENCY);
    }

    /* Set of all the connected BusAttachments. Maintain a weakreference so we dont delay garbage collection */
    private static HashSet<WeakReference<BusAttachment>> busAttachmentSet = new HashSet<WeakReference<BusAttachment>>();

    /* Shutdown hook used to release any connected BusAttachments when the VM exits */
    private static class ShutdownHookThread extends Thread {
        public void run() {
            synchronized (busAttachmentSet) {
                /* Iterate through the busAttachmentSet and release any BusAttachments that havent been released yet.*/
                Iterator<WeakReference<BusAttachment>> iterator = busAttachmentSet.iterator();
                while (iterator.hasNext()) {
                    BusAttachment busAttachment = iterator.next().get();
                    if (busAttachment != null) {
                        busAttachment.releaseWithoutRemove();
                    }
                    iterator.remove();
                }
            }
        }
    }

    /** Allocate native resources. */
    private native void create(String applicationName, boolean allowRemoteMessages, int concurrency);

    /** Release native resources. */
    private synchronized native void destroy();

    /** Start and connect to the bus. */
    private native Status connect(String connectArgs, KeyStoreListener keyStoreListener,
            String authMechanisms, AuthListenerInternal busAuthListener,
            String keyStoreFileName, boolean isShared);

    /** Stop and disconnect from the bus. */
    private native void disconnect(String connectArgs);

    private native Status enablePeerSecurity(String authMechanisms,
            AuthListenerInternal busAuthListener, String keyStoreFileName, Boolean isShared);

    private native Status registerBusObject(String objPath, BusObject busObj,
            InterfaceDescription[] busInterfaces, boolean secure,
            String languageTag, String description, Translator dt);

    private native boolean isSecureBusObject(BusObject busObj);

    private native Status registerNativeSignalHandlerWithSrcPath(String ifaceName, String signalName,
            Object obj, Method handlerMethod, String source);

    private native Status registerNativeSignalHandlerWithRule(String ifaceName, String signalName, Object obj,
        Method handlerMethod, String rule);

    /**
     * Release resources immediately.
     *
     * Normally, when all references are removed to a given object, the Java
     * garbage collector notices the fact that the object is no longer required
     * and will destroy it.  This can happen at the garbage collector's leisure
     * an so any resources held by the object will not be released until "some
     * time later" after the object is no longer needed.
     *
     * Often, in test programs, we cycle through many BusAttachments in a very
     * short time, and if we rely on the garbage collector to clean up, we can
     * fairly quickly run out of scarce underlying resources -- especially file
     * descriptors.
     *
     * We provide an explicity release() method to allow test programs to release
     * the underlying resources immediately.  The garbage collector will still
     * call finalize, but the resources held by the underlying C++ objects will
     * go away immediately.
     *
     * It is a programming error to call another method on the BusAttachment
     * after the release() method has been called.
     */
    public void release() {
        synchronized (busAttachmentSet) {
            releaseWithoutRemove();
            /* Remove this bus attachment from the busAttachmentSet */
            Iterator<WeakReference<BusAttachment>> iterator = busAttachmentSet.iterator();
            while (iterator.hasNext()) {
                BusAttachment busAttachment = iterator.next().get();
                if (busAttachment != null && busAttachment.equals(this)) {
                    iterator.remove();
                }
            }
        }
    }

    private void releaseWithoutRemove() {
        if (isConnected == true) {
            disconnect();
        }
        if (dbusbo != null) {
            dbusbo.release();
            dbusbo = null;
        }
        dbus = null;
        destroy();
    }

    /**
     * Let the Java garbage collector release resources.
     */
    @Override
    protected void finalize() throws Throwable {
        if (isConnected == true) {
            disconnect();
        }
        try {
            dbusbo = null;
            dbus = null;
            destroy();
        } finally {
            super.finalize();
        }
    }

    /**
     * Convert to UTF-8 for native code.  This is intended for sensitive string
     * data (i.e. passwords).  The native code must take care of scrubbing the
     * buffer when done.
     *
     * This method can be called from a listener object and must therefore be
     * MT-Safe.
     *
     * @param charArray the sensitive string
     * @return the UTF-8 encoded version of the string
     */
    static byte[] encode(char[] charArray) {
        try {
            Charset charset = Charset.forName("UTF-8");
            CharsetEncoder encoder = charset.newEncoder();
            ByteBuffer bb = encoder.encode(CharBuffer.wrap(charArray));
            byte[] ba = new byte[bb.limit()];
            bb.get(ba);
            return ba;
        } catch (CharacterCodingException ex) {
            BusException.log(ex);
            return null;
        }
    }

    /**
     * Used to defer listener methods called on a signal handler to
     * another thread.  This allows the listener method to make
     * blocking calls back into the library.
     */
    void execute(Runnable runnable) {
        executor.execute(runnable);
    }
    private static boolean shutdownHookRegistered = false;
    /**
     * Starts the message bus and connects to the local router.
     * This method blocks until the connection attempt succeeds or fails.
     * <p>
     * {@link BusObjectListener#registered()} is called by the bus when the bus
     * is connected.
     *
     * @return OK if successful
     */
    public Status connect() {
        /*
         * os.name is one of the standard system properties will be used to
         * decide the value of org.alljoyn.bus.address.
         */
        if ( System.getProperty("os.name").startsWith("Windows")) {
            address = System.getProperty("org.alljoyn.bus.address", "tcp:addr=127.0.0.1,port=9956");
        } else {
            address = System.getProperty("org.alljoyn.bus.address", "unix:abstract=alljoyn");
        }
        if (address != null) {
            Status status = connect(address, keyStoreListener, authMechanisms, busAuthListener, keyStoreFileName, isShared);
            if (status == Status.OK) {
                /* Add this BusAttachment to the busAttachmentSet */
                synchronized (busAttachmentSet) {
                    /* Register a shutdown hook if it is not already registered. */
                    if (shutdownHookRegistered == false) {
                        Runtime.getRuntime().addShutdownHook(new ShutdownHookThread());
                        shutdownHookRegistered = true;
                    }
                    busAttachmentSet.add(new WeakReference<BusAttachment>(this));
                }
                isConnected = true;
            }
            return status;
        } else {
            return Status.INVALID_CONNECT_ARGS;
        }
    }

    /**
     * Indicate whether bus is currently connected.
     *
     * @return true if the bus is connected
     */
    public native boolean isConnected();

    /**
     * Disconnects from the local router and stops the message bus.
     */
    public void disconnect() {
        if (address != null) {
            //            unregisterSignalHandler(this, foundAdvertisedName);
            //            unregisterSignalHandler(this, lostAdvertisedName);
            disconnect(address);
            isConnected = false;
        }
    }

    /**
     * Registers a bus object.
     * Once registered, the bus object may communicate to and from other
     * objects via its implemented bus interfaces.
     * <p>
     * The same object may not be registered on multiple bus connections.
     *
     * @param busObj the BusObject to register
     * @param objPath the object path of the BusObject
     * @return OK if successful
     * @see org.alljoyn.bus.annotation.BusInterface
     */
    public Status registerBusObject(BusObject busObj, String objPath) {
        return registerBusObject(busObj, objPath, false, null, null, null);
    }

    /**
     * Registers a bus object.
     * Once registered, the bus object may communicate to and from other
     * objects via its implemented bus interfaces.
     * <p>
     * The same object may not be registered on multiple bus connections.
     *
     * @param busObj the BusObject to register
     * @param objPath the object path of the BusObject
     * @param secure true if authentication is required to access this object
     * @return OK if successful
     * @see org.alljoyn.bus.annotation.BusInterface
     */
    public Status registerBusObject(BusObject busObj, String objPath, boolean secure) {
        return registerBusObject(busObj, objPath, secure, null, null, null);
    }

    /**
     * Registers a bus object.
     * Once registered, the bus object may communicate to and from other
     * objects via its implemented bus interfaces.
     * <p>
     * The same object may not be registered on multiple bus connections.
     *
     * @param busObj the BusObject to register
     * @param objPath the object path of the BusObject
     * @param secure true if authentication is required to access this object
     * @param languageTag a language tag describing the language of the description of this BusObject
     * @param description a textual description of this BusObject
     * @return OK if successful
     * @see org.alljoyn.bus.annotation.BusInterface
     */
    public Status registerBusObject(BusObject busObj, String objPath, boolean secure, String languageTag, String description) {
        return registerBusObject(busObj, objPath, secure, languageTag, description, null);
    }
    /**
     * Registers a bus object.
     * Once registered, the bus object may communicate to and from other
     * objects via its implemented bus interfaces.
     * <p>
     * The same object may not be registered on multiple bus connections.
     *
     * @param busObj the BusObject to register
     * @param objPath the object path of the BusObject
     * @param secure true if authentication is required to access this object
     * @param languageTag a language tag describing the language of the description of this BusObject
     * @param description a textual description of this BusObject
     * @param dt a Translator instance to translate descriptions of this object
     * @return OK if successful
     * @see org.alljoyn.bus.annotation.BusInterface
     */
    public Status registerBusObject(BusObject busObj, String objPath, boolean secure,
            String languageTag, String description, Translator dt) {
        try {
            List<InterfaceDescription> descs = new ArrayList<InterfaceDescription>();
            Status status = InterfaceDescription.create(this, busObj.getClass().getInterfaces(),
                                                        descs);
            if (status != Status.OK) {
                return status;
            }
            return registerBusObject(objPath, busObj, descs.toArray(new InterfaceDescription[0]), secure,
                                     languageTag, description, dt);
        } catch (AnnotationBusException ex) {
            BusException.log(ex);
            return Status.BAD_ANNOTATION;
        }
    }

    /**
     * Indicates if the BusObject is secure.
     *
     * @param busObj BusObject to check if it is secure
     * @return Return true if authentication is required to emit signals or call
     *         methods on this object.
     */
    public boolean isBusObjectSecure(BusObject busObj) {
        return isSecureBusObject(busObj);
    }
    /**
     * Unregisters a bus object.
     *
     * @param obj the BusObject to unregister
     */
    public native void unregisterBusObject(BusObject obj);

    /**
     * Creates a proxy bus object for a remote bus object.
     * Methods on the remote object can be invoked through the proxy object.
     * <p>
     * There is no guarantee that the remote object referred to by the proxy
     * acutally exists.  If the remote object does not exist, proxy method
     * calls will fail.
     * <p>
     * Java proxy classes do not allow methods from two different interfaces to
     * have the same name and calling parameters. If two AllJoyn methods from two
     * different interfaces are implemented by the same remote object, one (or
     * both) of the method names must be modified. You may then use an
     * annotation for the renamed method to cause AllJoyn to use the originally
     * expected method name in any "wire" operations.
     *
     * @param busName the remote endpoint name (well-known or unique)
     * @param objPath the absolute (non-relative) object path for the object
     * @param sessionId the session corresponding to the connection to the the object
     * @param busInterfaces an array of BusInterfaces that this proxy should respond to
     * @return a ProxyBusObject for an object that implements all interfaces listed in busInterfaces
     * @see org.alljoyn.bus.annotation.BusMethod
     */
    public ProxyBusObject getProxyBusObject(String busName,
            String objPath,
            int sessionId,
            Class<?>[] busInterfaces) {
        return new ProxyBusObject(this, busName, objPath, sessionId, busInterfaces);
    }

    /**
     * Creates a proxy bus object for a remote bus object.
     * Methods on the remote object can be invoked through the proxy object.
     * <p>
     * There is no guarantee that the remote object referred to by the proxy
     * acutally exists.  If the remote object does not exist, proxy method
     * calls will fail.
     * <p>
     * Java proxy classes do not allow methods from two different interfaces to
     * have the same name and calling parameters. If two AllJoyn methods from two
     * different interfaces are implemented by the same remote object, one (or
     * both) of the method names must be modified. You may then use an
     * annotation for the renamed method to cause AllJoyn to use the originally
     * expected method name in any "wire" operations.
     *
     * @param busName        the remote endpoint name (well-known or unique)
     * @param objPath        the absolute (non-relative) object path for the object
     * @param sessionId      the session corresponding to the connection to the the object
     * @param busInterfaces  an array of BusInterfaces that this proxy should respond to
     * @param secure         the security mode for the remote object
     *
     * @return a ProxyBusObject for an object that implements all interfaces listed in busInterfaces
     *
     * @see org.alljoyn.bus.annotation.BusMethod
     */
    public ProxyBusObject getProxyBusObject(String busName,
            String objPath,
            int sessionId,
            Class<?>[] busInterfaces,
            boolean secure) {
        return new ProxyBusObject(this, busName, objPath, sessionId, busInterfaces, secure);
    }

    /**
     * Gets the DBusProxyObj interface of the org.freedesktop.DBus proxy object.
     * The DBusProxyObj interface is provided for backwards compatibility with
     * the DBus protocol.
     *
     * @return the DBusProxyObj interface
     */
    public DBusProxyObj getDBusProxyObj() {
        return dbus;
    }

    /**
     * Get the unique name of this BusAttachment.
     *
     * @return the unique name of this BusAttachment
     */
    public native String getUniqueName();

    /**
     * Get the GUID of this BusAttachment.
     *
     * The returned value may be appended to an advertised well-known name in order to guarantee
     * that the resulting name is globally unique.
     *
     * @return GUID of this BusAttachment as a string.
     */
    public native String getGlobalGUIDString();

    /**
     * Registers a public method to receive a signal from all objects emitting
     * it.
     * Once registered, the method of the object will receive the signal
     * specified from all objects implementing the interface.
     *
     * @param ifaceName the interface name of the signal
     * @param signalName the member name of the signal
     * @param obj the object receiving the signal
     * @param handlerMethod the signal handler method
     * @return OK if the register is succesful
     */
    public Status registerSignalHandler(String ifaceName,
            String signalName,
            Object obj,
            Method handlerMethod) {
        return registerSignalHandler(ifaceName, signalName, obj, handlerMethod, "");
    }

    /**
     * Registers a public method to receive a signal from specific objects emitting it. Once registered, the method of
     * the object will receive the signal specified from objects implementing the interface.
     *
     * @param ifaceName the interface name of the signal
     * @param signalName the member name of the signal
     * @param obj the object receiving the signal
     * @param handlerMethod the signal handler method
     * @param source the object path of the emitter of the signal
     * @return OK if the register is successful
     */
    public Status registerSignalHandler(String ifaceName,
            String signalName,
            Object obj,
            Method handlerMethod,
            String source) {
        Status status = registerNativeSignalHandlerWithSrcPath(ifaceName, signalName, obj, handlerMethod,
                                                    source);
        if (status == Status.BUS_NO_SUCH_INTERFACE) {
            try {
                Class<?> iface = Class.forName(ifaceName);
                InterfaceDescription desc = new InterfaceDescription();
                status = desc.create(this, iface);
                if (status == Status.OK) {
                    ifaceName = InterfaceDescription.getName(iface);
                    try {
                        Method signal = iface.getMethod(signalName, handlerMethod.getParameterTypes());
                        signalName = InterfaceDescription.getName(signal);
                    } catch (NoSuchMethodException ex) {
                        // Ignore, use signalName parameter provided
                    }
                    status = registerNativeSignalHandlerWithSrcPath(ifaceName, signalName, obj, handlerMethod,
                                                         source);
                }
            } catch (ClassNotFoundException ex) {
                BusException.log(ex);
                status = Status.BUS_NO_SUCH_INTERFACE;
            } catch (AnnotationBusException ex) {
                BusException.log(ex);
                status = Status.BAD_ANNOTATION;
            }
        }
        return status;
    }

    /**
     * Register a signal handler.
     *
     * Signals are forwarded to the signalHandler if sender, interface, member and rule qualifiers are ALL met.
     *
     * @param ifaceName the interface name of the signal
     * @param signalName the member name of the signal
     * @param obj the object receiving the signal
     * @param handlerMethod the signal handler method
     * @param matchRule a filter rule.
     * @return OK if the register is successful
     */

    public Status registerSignalHandlerWithRule(String ifaceName, String signalName, Object obj, Method handlerMethod,
        String matchRule)
    {

        Status status = registerNativeSignalHandlerWithRule(ifaceName, signalName, obj, handlerMethod, matchRule);
        if (status == Status.BUS_NO_SUCH_INTERFACE) {
            try {
                Class<?> iface = Class.forName(ifaceName);
                InterfaceDescription desc = new InterfaceDescription();
                status = desc.create(this, iface);
                if (status == Status.OK) {
                    ifaceName = InterfaceDescription.getName(iface);
                    try {
                        Method signal = iface.getMethod(signalName, handlerMethod.getParameterTypes());
                        signalName = InterfaceDescription.getName(signal);
                    }
                    catch (NoSuchMethodException ex) {
                        // Ignore, use signalName parameter provided
                    }
                    status = registerNativeSignalHandlerWithRule(ifaceName, signalName, obj, handlerMethod, matchRule);
                }
            }
            catch (ClassNotFoundException ex) {
                BusException.log(ex);
                status = Status.BUS_NO_SUCH_INTERFACE;
            }
            catch (AnnotationBusException ex) {
                BusException.log(ex);
                status = Status.BAD_ANNOTATION;
            }
        }
        return status;
    }

    /**
     * Registers all public methods that are annotated as signal handlers.
     *
     * @param obj object with methods annotated with as signal handlers
     * @return <ul>
     *         <li>OK if the register is succesful
     *         <li>BUS_NO_SUCH_INTERFACE if the interface and signal
     *         specified in any {@code @BusSignalHandler} annotations
     *         of {@code obj} are unknown to this BusAttachment.  See
     *         {@link org.alljoyn.bus.annotation.BusSignalHandler} for
     *         a discussion of how to annotate signal handlers.
     *         </ul>
     */
    public Status registerSignalHandlers(Object obj) {
        Status status = Status.OK;
        for (Method m : obj.getClass().getMethods()) {
            BusSignalHandler a = m.getAnnotation(BusSignalHandler.class);
            if (a != null) {
                if (a.rule().equals("") == false) {
                    status = registerSignalHandlerWithRule(a.iface(), a.signal(), obj, m, a.rule());
                }
                else {
                    status = registerSignalHandler(a.iface(), a.signal(), obj, m, a.source());
                }
                if (status != Status.OK) {
                    break;
                }
            }
        }
        return status;
    }

    /**
     * Unregisters a signal handler.
     *
     * @param obj the object receiving the signal
     * @param handlerMethod the signal handler method
     */
    public native void unregisterSignalHandler(Object obj, Method handlerMethod);

    /**
     * Unregisters all public methods annotated as signal handlers.
     *
     * @param obj object with previously annotated signal handlers that have
     *            been registered
     * @see org.alljoyn.bus.annotation.BusSignalHandler
     */
    public void unregisterSignalHandlers(Object obj) {
        for (Method m : obj.getClass().getMethods()) {
            BusSignalHandler a = m.getAnnotation(BusSignalHandler.class);
            if (a != null) {
                unregisterSignalHandler(obj, m);
            }
        }
    }

    /**
     * Registers a user-defined key store listener to override the default key store.  This must be
     * called prior to {@link #connect()}.
     *
     * @param listener the key store listener
     */
    public void registerKeyStoreListener(KeyStoreListener listener) {
        keyStoreListener = listener;
    }

    /**
     * Clears all stored keys from the key store. All store keys and authentication information is
     * deleted and cannot be recovered. Any passwords or other credentials will need to be reentered
     * when establishing secure peer connections.
     */
    public native void clearKeyStore();

    /**
     * Clear the keys associated with a specific peer identified by its GUID.
     *
     * @param guid  The guid of a remote authenticated peer.
     *
     * @return
     * <ul>
     * <li>OK if the key was cleared</li>
     * <li>UNKNOWN GUID if theere is no peer with the specified GUID</li>
     * <li>other error status codes indicating a failure</li>
     * </ul>
     */
    public native Status clearKeys(String guid);

    /**
     * Sets the expiration time on keys associated with a specific remote peer as identified by its
     * peer GUID. The peer GUID associated with a bus name can be obtained by calling
     * getPeerGUID(String, Mutable.StringValue).  If the timeout is 0 this is equivalent to calling
     * clearKeys(String).
     *
     * @param guid the GUID of a remote authenticated peer
     * @param timeout the time in seconds relative to the current time to expire the keys
     *
     * @return
     * <ul>
     * <li>OK if the expiration time was succesfully set</li>
     * <li>UNKNOWN_GUID if there is no authenticated peer with the specified GUID</li>
     * </ul>
     */
    public native Status setKeyExpiration(String guid, int timeout);

    /**
     * Gets the expiration time on keys associated with a specific authenticated remote peer as
     * identified by its peer GUID. The peer GUID associated with a bus name can be obtained by
     * calling getPeerGUID(String, Mutable.StringValue).
     *
     * @param guid the GUID of a remote authenticated peer
     * @param timeout the time in seconds relative to the current time when the keys will expire
     *
     * @return
     * <ul>
     * <li>OK if the expiration time was succesfully set</li>
     * <li>UNKNOWN_GUID if there is no authenticated peer with the specified GUID</li>
     * </ul>
     */
    public native Status getKeyExpiration(String guid, Mutable.IntegerValue timeout);

    /**
     * Reloads the key store for this bus attachment. This function would normally only be called in
     * the case where a single key store is shared between multiple bus attachments, possibly by different
     * applications. It is up to the applications to coordinate how and when the shared key store is
     * modified.
     *
     * @return
     * <ul>
     * <li>OK if the key store was succesfully reloaded</li>
     * <li>An error status indicating that the key store reload failed</li>
     * </ul>
     */
    public native Status reloadKeyStore();

    /**
     * Registers a user-defined authentication listener class with a specific default key store.
     *
     * @param authMechanisms the authentication mechanism(s) to use for peer-to-peer authentication. This is a space separated list of any of the following values: ALLJOYN_PIN_KEYX, ALLJOYN_SRP_LOGON, ALLJOYN_RSA_KEYX, ALLJOYN_SRP_KEYX, ALLJOYN_ECDHE_NULL, ALLJOYN_ECDHE_PSK, ALLJOYN_ECDHE_ECDSA, GSSAPI.
     * @param listener the authentication listener
     * @param keyStoreFileName the name of the default key store.  Under Android, the recommended
     *                         value of this parameter is {@code
     *                         Context.getFileStreamPath("alljoyn_keystore").getAbsolutePath()}.  Note
     *                         that the default key store implementation may be overrided with
     *                         {@link #registerKeyStoreListener(KeyStoreListener)}.
     * @param isShared Set to true if the default keystore will be shared between multiple programs.
     *                 all programs must have read/write permissions to the keyStoreFileName file.
     * @return OK if successful
     */
    public Status registerAuthListener(String authMechanisms, AuthListener listener,
            String keyStoreFileName, boolean isShared) {

        /*
         * It is not possible to register multiple AuthListeners or replace an
         * existing AuthListener.
         */
        if (busAuthListener.authListenerSet()) {
            return Status.ALREADY_REGISTERED;
        }

        this.authMechanisms = authMechanisms;
        busAuthListener.setAuthListener(listener);
        this.keyStoreFileName = keyStoreFileName;
        this.isShared = isShared;
        Status status = enablePeerSecurity(this.authMechanisms, busAuthListener,
                                           this.keyStoreFileName, isShared);
        if (status != Status.OK) {
            busAuthListener.setAuthListener(null);
            this.authMechanisms = null;
        }
        return status;
    }

    /**
     * Registers a user-defined authentication listener class with a specific default key store.
     *
     * @param authMechanisms the authentication mechanism(s) to use for peer-to-peer authentication.  This is a space separated list of any of the following values: ALLJOYN_PIN_KEYX, ALLJOYN_SRP_LOGON, ALLJOYN_RSA_KEYX, ALLJOYN_SRP_KEYX, ALLJOYN_ECDHE_NULL, ALLJOYN_ECDHE_PSK, ALLJOYN_ECDHE_ECDSA, GSSAPI.
     * @param listener the authentication listener
     * @param keyStoreFileName the name of the default key store.  Under Android, the recommended
     *                         value of this parameter is {@code
     *                         Context.getFileStreamPath("alljoyn_keystore").getAbsolutePath()}.  Note
     *                         that the default key store implementation may be overrided with
     *                         {@link #registerKeyStoreListener(KeyStoreListener)}.
     * @return OK if successful
     */
    public Status registerAuthListener(String authMechanisms, AuthListener listener,
            String keyStoreFileName){
        return registerAuthListener(authMechanisms, listener, keyStoreFileName, false);
    }

    /**
     * Registers a user-defined authentication listener class.  Under Android, it is recommended to
     * use {@link #registerAuthListener(String, AuthListener, String)} instead to specify the path
     * of the default key store.
     *
     * @param authMechanisms the authentication mechanism(s) to use for peer-to-peer authentication.  This is a space separated list of any of the following values: ALLJOYN_PIN_KEYX, ALLJOYN_SRP_LOGON, ALLJOYN_RSA_KEYX, ALLJOYN_SRP_KEYX, ALLJOYN_ECDHE_NULL, ALLJOYN_ECDHE_PSK, ALLJOYN_ECDHE_ECDSA, GSSAPI.
     * @param listener the authentication listener
     * @return OK if successful
     */
    public Status registerAuthListener(String authMechanisms, AuthListener listener) {
        return registerAuthListener(authMechanisms, listener, null, false);
    }

    /**
     * Registers a user-defined security violation listener class.
     *
     * @param listener the security violation listener
     */
    public void registerSecurityViolationListener(SecurityViolationListener listener) {
        busAuthListener.setSecurityViolationListener(listener);
    }

    /**
     * Gets the message context of the currently executing method, signal
     * handler, or security violation.
     * The context contains information about the method invoker or signal
     * sender including any authentication that exists with this remote entity.
     * <p>
     * This method can only be called from within the method, signal, or
     * security violation handler itself since the caller's thread information
     * is used to find the appropriate context.
     *
     * @return message context for the currently executing method, signal
     *         handler, security violation, or null if no message can be found
     *         for the calling thread
     */
    public native MessageContext getMessageContext();

    /**
     * Enable callbacks within the context of the currently executing method
     * handler, signal handler or other AllJoyn callback.
     * <p>
     * This method can ONLY be called from within the body of a signal handler,
     * method handler or other AllJoyn callback. It allows AllJoyn to dispatch
     * a single (additional) callback while the current one is still executing.
     * This method is typically used when a method, signal handler or other
     * AllJoyn callback needs to execute for a long period of time or when the
     * callback needs to make any kind of blocking call.
     * <p>
     * This method MUST be called prior to making any non-asynchronous AllJoyn
     * remote procedure calls from within an AllJoyn callback. This includes
     * calls such as joinSession(), advertiseName(), cancelAdvertisedName(),
     * findAdvertisedName(), cancelFindAdvertisedName(), setLinkTimeout(), etc.
     */
    public native void enableConcurrentCallbacks();

    /**
     * The maximum length of an AllJoyn packet.
     *
     * AllJoyn limits the total size of a packetized Message to 2^17 bytes.
     */
    public static final int ALLJOYN_MAX_PACKET_LEN = 131072;

    /**
     * The maximum length of an array sent in an AllJoyn Message.
     *
     * AllJoyn limits arrays to a maximum sze of 2^16
     */
    public static final int ALLJOYN_MAX_ARRAY_LEN = 131072;

    /**
     * The maximum number of concurrent method and signal handlers locally
     * executing by default.
     */
    private static final int DEFAULT_CONCURRENCY = 4;
}
