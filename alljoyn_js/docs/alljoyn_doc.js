/*
 * Copyright (c) 2011 - 2014, AllSeen Alliance. All rights reserved.
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
/*
 * NOTE: This documentation is only a reflection of the AllJoyn JavaScript API.
 * AllJoyn for JavaScript is developed using NPAPI and has very little actual
 * JavaScript code.  This documentation is so developers can develop programs
 * using the JavaScript API.
 */
/**
 * @fileOverview
 *
 * AllJoyn&trade; JavaScript API Reference Manual
 * @version 14.12.00
 * @description
 * Type coercion between DBus and JavaScript types is done according to the following table.
 * <table>
 * <tr><th><b>IDL type</b></th><th><b>DBus type</b></th><th><b>Signature</b></th><th><b>JavaScript
 * type</b></th></tr>
 * <tr><td>octet</td><td>BYTE</td><td>'y'</td><td>Number</td></tr>
 * <tr><td>boolean</td><td>BOOLEAN</td><td>'b'</td><td>Boolean</td></tr>
 * <tr><td>short</td><td>INT16</td><td>'n'</td><td>Number</td></tr>
 * <tr><td>unsigned short</td><td>UINT16</td><td>'q'</td><td>Number</td></tr>
 * <tr><td>long</td><td>INT32</td><td>'i'</td><td>Number</td></tr>
 * <tr><td>unsigned long</td><td>UINT32</td><td>'u'</td><td>Number</td></tr>
 * <tr><td>long long</td><td>INT64</td><td>'x'</td><td>The DBus value is coerced into a decimal
 * string.  Use <em>parseInt</em> to convert the argument to a number if loss of precision is not a
 * concern.</td></tr>
 * <tr><td>unsigned long long</td><td>UINT64</td><td>'t'</td><td>The DBus value is coerced into a
 * decimal string.  Use <em>parseInt</em> to convert the argument to a number if loss of precision
 * is not a concern.</td></tr>
 * <tr><td>double</td><td>DOUBLE</td><td>'d'</td><td>Number</td></tr>
 * <tr><td>DOMString</td><td>STRING</td><td>'s'</td><td>String</td></tr>
 * <tr><td>DOMString</td><td>OBJECT_PATH</td><td>'o'</td><td>String</td></tr>
 * <tr><td>DOMString</td><td>SIGNATURE</td><td>'g'</td><td>String</td></tr>
 * <tr><td>sequence&lt;T&gt;</td><td>ARRAY</td><td>'a'</td><td>When the DBus value is an array of
 * DICT_ENTRY, the argument will be an Object where each property is the DICT_ENTRY key and each
 * value is the DICT_ENTRY value.  Otherwise the argument will be an Array.</td></tr>
 * <tr><td>sequence&lt;any&gt;</td><td>STRUCT</td><td>'r', '(', ')'</td><td>Array</td></tr>
 * <tr><td>any</td><td>VARIANT</td><td>'v'</td><td>DBus VARIANT types remove the VARIANT wrapper.
 * For example, a VARIANT of the string "value" will be <em>"value"</em>.  Nested VARIANTs will
 * remove only one VARIANT wrapper.  For example, a VARIANT of a VARIANT of a string will be the
 * Object <em>{ "s": "value" }</em>.</td></tr>
 * </table>
 *
 * Debug output of the plugin can be configured dynamically by setting the debug attribute of the
 * <em>object</em> element containing the plugin.  The syntax of the attribute value is the same as
 * BusAttachment.setDaemonDebug, e.g. <em>debug="ALLJOYN_JS=15"</em>.  The destination for debug
 * output depends on the platform: Windows is OutputDebugString, and for all
 * others it is stdout.
 */

/** {Number} SessionId uniquely identifies an AllJoyn session instance (range 0 to 4,294,967,295). */
var SessionId;

/**
 * {Number} SessionPort identifies a per-BusAttachment receiver for incoming JoinSession
 * requests. (range 0 to 65,535)
 *
 * SessionPort values are bound to a BusAttachment when the attachment calls
 * BindSessionPort.
 *
 * NOTE: Valid SessionPort values range from 1 to 0xFFFF.
 */
var SessionPort;

/** {Number} Bitmask of all transport types (mask values 0 to 0xFF)*/
var TransportMask;

/** {Number} Status code value (range 0 to 65,535)*/
var Status;

/**
 * @class
 * @name BusError
 * @description
 * AllJoyn-specific error codes.
 *
 * Note that NPAPI does not support raising custom exceptions, and the browsers behave
 * differently.  To workaround this, the plugin provides the name, message, and code
 * fields of the most recently raised exception on the exception interface object.
 */
var BusError = Class.create(
/**
 * @lends BusError
 */
{
    /**
     *
     * The error name.
     *
     * The default value is "BusError".
     * @type {String}
     */
    name: "BusError",
    /**
     * The error message.
     *
     * The default value is the empty string.
     * @type {String}
     */
    message: "",
    /**
     * The error code.
     * @type {Status}
     */
    code: Status
});

/**
 * @class
 * @name AuthListener
 * @description
 * Interface to allow authentication mechanisms to interact with the user or
 * application.
 */
var AuthListener = Class.create(
/**
 * @lends AuthListener
 */
{
    /**
     * The authentication mechanism requests user credentials.
     *
     * If the user name is not an empty string the request is for credentials for that
     * specific user. A count allows the listener to decide whether to allow or reject
     * multiple authentication attempts to the same peer.
     *
     * @param {String} authMechanism the name of the authentication mechanism issuing the request
     * @param {String} peerName the name of the remote peer being authenticated.  On the
     *                 initiating side this will be a well-known-name for the remote
     *                 peer. On the accepting side this will be the unique bus name for
     *                 the remote peer.
     * @param {Number} authCount count (starting at 1) of the number of authentication request
     *                  attempts made (number range 0 to 65,535)
     * @param {String} userName the user name for the credentials being requested
     * @param {Number} credMask a bit mask identifying the credentials being requested. The
     *                 application may return none, some or all of the requested
     *                 credentials. (bit mask range 0 to 0xFF)
     * @param {Credentials} credentials the credentials returned
     *
     * @return {boolean} the caller should return true if the request is being accepted or false
     *         if the requests is being rejected. If the request is rejected the
     *         authentication is complete.
     */
    onRequest: function(authMechanism, peerName, authCount, userName, credMask, credentials) { },

    /**
     * The authentication mechanism requests verification of credentials from a
     * remote peer.
     *
     * This operation is mandatory for the ALLJOYN_RSA_KEYX auth mechanism, optional for others.
     *
     * @param {String} authMechanism the name of the authentication mechanism issuing the request
     * @param {String} peerName the name of the remote peer being authenticated.  On the
     *                 initiating side this will be a well-known-name for the remote
     *                 peer. On the accepting side this will be the unique bus name for
     *                 the remote peer.
     * @param {Credentials} credentials the credentials to be verified
     *
     * @return {boolean} the listener should return true if the credentials are acceptable or
     *         false if the credentials are being rejected.
     */
    onVerify: function(authMechanism, peerName, credentials) { },

    /**
     * Optional operation that if implemented allows an application to monitor
     * security violations.
     *
     * This operation is called when an attempt to decrypt an encrypted messages failed
     * or when an unencrypted message was received on an interface that requires
     * encryption. The message contains only header information.
     *
     * @param {Status} status a status code indicating the type of security violation
     * @param {Message} context the message that cause the security violation
     */
    onSecurityViolation: function(status, context) { },

    /**
     * Reports successful or unsuccessful completion of authentication.
     *
     * @param {String} authMechanism the name of the authentication mechanism that was used or an
     *                      empty string if the authentication failed
     * @param {String} peerName the name of the remote peer being authenticated.  On the
     *                 initiating side this will be a well-known-name for the remote
     *                 peer. On the accepting side this will be the unique bus name for
     *                 the remote peer.
     * @param {boolean} success true if the authentication was successful, otherwise false
     */
    onComplete: function(authMechanism, peerName, success) { }
});

/**
 * @namespace
 * @name org
 */
var org = org || {};
/** @namespace */
org.alljoyn = {};
/** @namespace */
org.alljoyn.bus = {};

/**
 * BusAttachment is the top-level object responsible for connecting to and optionally
 * managing a message bus.
 * @constructor
 */
org.alljoyn.bus.BusAttachment = function() {
    /**
     * Value for requestName flags bit corresponding to allowing another bus
     * attachment to take ownership of this name.
     * @constant
     */
    this.DBUS_NAME_FLAG_ALLOW_REPLACEMENT = 0x01;

    /**
     * Value for requestName flags bit corresponding to a request to take
     * ownership of the name in question if it is already taken.
     * @constant
     */
    this.DBUS_NAME_FLAG_REPLACE_EXISTING = 0x02;

    /**
     * Value for requestName flags bit corresponding to a request to fail if the
     * name in question cannot be immediately obtained.
     * @constant
     */
    this.DBUS_NAME_FLAG_DO_NOT_QUEUE = 0x04;

    /**
     * Invalid SessionPort value used to indicate that bindSessionPort should
     * choose any available port.
     * @constant
     */
    this.SESSION_PORT_ANY = 0;

    /**
     * The GUID of this BusAttachment.
     *
     * The returned value may be appended to an advertised well-known name in order to
     * guarantee that the resulting name is globally unique.
     *
     * Read-only.
     * @type {String}
     */
    this.globalGUIDString;

    /**
     * The unique name of this BusAttachment.
     *
     * Read-only.
     *
     * <em>null</em> if not connected.
     * @type {String}
     */
    this.uniqueName;

    /**
     * Adds a logon entry string for the requested authentication mechanism to the key store.
     *
     * This allows an authenticating server to generate offline authentication
     * credentials for securely logging on a remote peer using a user-name and password
     * credentials pair. This only applies to authentication mechanisms that support a
     * user name + password logon functionality.
     *
     * @param {String} authMechanism the authentication mechanism
     * @param {String} userName the user name to use for generating the logon entry
     * @param {String} password the password to use for generating the logon entry. If the
     *                 password is <em>null</em> the logon entry is deleted from the key
     *                 store.
     * @param {statusCallback} callback function that should be run after the logonEntry has
     *           been added. Status will be OK if the logon entry was generated.
     *           BUS_INVALID_AUTH_MECHANISM if the authentication mechanism
     *           does not support logon functionality.
     *           Other error status codes indicating a failure.
     */
    this.addLogonEntry = function(authMechanism, userName, password, callback) {};

    /**
     * Adds a DBus match rule.
     *
     * This operation is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch method call
     * to the local daemon.
     *
     * @param {String} rule match rule to be added (see DBus specification for format of this string)
     * @param {statusCallback} callback function to run after addMatch has completed. callback will contain
     *        status BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *        Other error status codes indicating a failure.
     */
    this.addMatch = function(rule, callback) {};
    /**
     * Advertises the existence of a well-known name to other (possibly disconnected)
     * AllJoyn daemons.
     *
     * This operation is a shortcut/helper that issues an org.alljoyn.Bus.AdvertisedName method
     * call to the local daemon and interprets the response.
     *
     * @param {String} name the well-known name to advertise. (Must be owned by the caller via RequestName)
     * @param {TransportMask} transports set of transports to use for sending advertisment
     * @param {statusCallback} callback function to run after name is advertised. Callback will
     *           contain status BUS_NOT_CONNECTED if a connection has not been
     *           made with a local bus.
     *           Other error status codes indicating a failure
     */
    this.advertiseName = function(name, transports, callback){};

    /**
     * \brief Makes a SessionPort available for external BusAttachments to join.
     *
     * Each BusAttachment binds its own set of SessionPorts. Session joiners use the bound
     * session port along with the name of the attachement to create a persistent logical
     * connection (called a Session) with the original BusAttachment.
     *
     * SessionPort and bus name form a unique identifier that BusAttachments use when joining
     * a session.
     *
     * SessionPort values can be pre-arranged between AllJoyn services and their clients
     * (well-known SessionPorts).
     *
     * Once a session is joined using one of the service's well-known SessionPorts, the service
     * may bind additional SessionPorts (dyanamically) and share these SessionPorts with the
     * joiner over the original session. The joiner can then create additional sessions with the
     * service by calling JoinSession with these dynamic SessionPort ids.
     *
     * The parameters are supplied as a named-parameter object literal.
     * <dl>
     * <dt><em>port</em></dt>
     * <dd>SessionPort value to bind.  The default value is <em>SESSION_PORT_ANY</em> to allow
     * this operation to choose an available port.  On successful return, this value will
     * be the chosen port.</dd>
     * <dt><em>traffic</em></dt>
     * <dd>TrafficType.  The default value is <em>TRAFFIC_MESSAGES</em>.</dd>
     * <dt><em>isMultipoint</em></dt>
     * <dd>boolean indicating if session is multi-point capable.  The default value is
     * <em>false</em>.</dd>
     * <dt><em>proximity</em></dt>
     * <dd>Proximity.  The default value is <em>PROXIMITY_ANY</em>.</dd>
     * <dt><em>transports</em></dt>
     * <dd>TransportMask of the allowed transports.  The default value is
     * <em>TRANSPORT_ANY</em>.</dd>
     * <dt><em>onAccept</em></dt>
     * <dd>an <a href="#::ajn::AcceptSessionJoinerListener">AcceptSessionJoinerListener</a> to
     * accept or reject an incoming JoinSession request.  The session does not exist until this
     * after this function returns.</dd>
     * <dt><em>onJoined</em></dt>
     * <dd>a <a href="#::ajn::SessionJoinedListener">SessionJoinedListener</a> called by the bus
     * when a session has been successfully joined</dd>
     * <dt><em>onLost</em></dt>
     * <dd>a <a href="#::ajn::SessionLostListener">SessionLostListener</a> called by the bus
     * when an existing session becomes disconnected</dd>
     * <dt><em>onMemberAdded</em></dt>
     * <dd>a <a href="#::ajn::SessionMemberAddedListener">SessionMemberAddedListener</a> called
     * by the bus when a member of a multipoint session is added</dd>
     * <dt><em>onMemberRemoved</em></dt>
     * <dd>a <a href="#::ajn::SessionMemberRemovedListener">SessionMemberRemovedListener</a> called
     * by the bus when a member of a multipoint session is removed</dd>
     * </dl>
     *
     * @param {object} parameters see above
     * @param {sessionPortCallback} callback function run after bindSessionPort has completed.
     *        BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     */
    this.bindSessionPort = function(parameters, callback){};

    /**
     * Stops advertising the existence of a well-known name to other AllJoyn daemons.
     *
     * This operation is a shortcut/helper that issues an org.alljoyn.Bus.CancelAdvertiseName
     * method call to the local daemon and interprets the response.
     *
     * @param {String} name a well-known name that was previously advertised via AdvertiseName
     * @param {TransportMask} transports set of transports whose name advertisment will be cancelled
     * @param {statusCallback} callback function that is run after cancelAdvertiseName
     *        callback returns
     *        BUS_NOT_CONNECTED if a connection has not been made with a local bus
     *        Other error status codes indicating a failure
     */
    this.cancelAdvertiseName = function(name, transports, callback){};

    /**
     * Cancels interest in a well-known name prefix that was previously registered with
     * FindAdvertisedName.
     *
     * This operation is a shortcut/helper that issues an org.alljoyn.Bus.CancelFindAdvertisedName
     * method call to the local daemon and interprets the response.
     *
     * @param {String} namePrefix well-known name prefix that application is no longer interested in
     *                   receiving BusListener onFoundAdvertisedName notifications about
     * @param {statusCallback} callback function run after a cancelFindAdvertisedName has completed
     *           Callback will specify BUS_NOT_CONNECTED if a connection has not been made
     *           with a local bus.
     *           Other error status codes indicating a failure
     */
    this.cancelFindAdvertisedName = function(namePrefix, callback){};

    /**
     * Clears all stored keys from the key store.
     *
     * All store keys and authentication information is deleted and cannot be
     * recovered. Any passwords or other credentials will need to be reentered when
     * establishing secure peer connections.
     *
     * @param {statusCallback} callback function called after clearKeyStore has completed.
     */
    this.clearKeyStore = function(callback) {};

    /**
     * Clear the keys associated with a specific peer identified by its GUID.
     *
     * @param {String} guid the guid of a remote authenticated peer
     * @param {statusCallback} callback function that gets called after clearKeys have completed
     *        callback will return OK if the key was cleared.
     *        UNKNOWN_GUID if there is no peer with the specified GUID.
     *        Other error status codes indicating a failure.
     */
    this.clearKeys = function(guid, callback) {};

    /**
     * Connects to a remote bus address.
     *
     * @param {String|optional} connectSpec an optional transport connection spec string of the form
     *                    "&lt;transport&gt;:&lt;param1&gt;=&lt;value1&gt;,&lt;param2&gt;=&lt;value2&gt;...[;]".
     *                    The default value is platform-specific.
     * @param {statusCallback} callback function that is called once connect has completed.
     */
    this.connect = function(connectSpec, callback) {};

    /**
     * create a new BusAttachment.
     *
     * create must be called before any other BusAttachment methods.  This call
     * is responsible for making sure proper creation a native BusAttachment.
     *
     * @param {boolean} allowRemoteMessages True if this attachment is allowed to receive messages from remote devices.
     * @param {statusCallback} callback callback function called once the native BusAttachment has been created.
     */
    this.create = function(allowRemoteMessages, callback) {};

    /**
     * Add the interface specified by the interfaceDescription to the BusAttachment
     *
     * The interfaceDescription must be a full interface specification.  The interface is
     * created and activated on the BusAttachment.
     *
     * @param {InterfaceDescription} interfaceDescription the specification for the interface being created.
     * @param {statusCallback} callback the function called once the createInterface method has completed.
     */
    this.createInterface = function(interfaceDescription, callback) {};

    /**
     * Initialize one more interface descriptions from an XML string in DBus introspection format.
     * The root tag of the XML can be a \<node\> or a stand alone \<interface\> tag. To initialize more
     * than one interface the interfaces need to be nested in a \<node\> tag.
     *
     * Note that when this method fails during parsing, the return code will be set accordingly.
     * However, any interfaces which were successfully parsed prior to the failure may be registered
     * with the bus.
     *
     * @param {String} xml An XML string in DBus introspection format.
     * @param {statusCallback} callback function that is run after creating the
     *        interfaces from xml.  Callback will specify OK if parsing is
     *        completely successful.  An error status otherwise.
     */
    this.createInterfacesFromXML = function(xml, callback) {};

    /**
     * Disconnects from a remote bus address connection.
     * 
     * @param {statusCallback} callback function will specify BUS_NOT_STARTED if the bus is not started.
     *         BUS_NOT_CONNECTED if the BusAttachment is not connected to the bus.
     *         Other error status codes indicating a failure.
     */
    this.disconnect = function(callback) {};

    /**
     * Explicitly release the reference to the BusAttachment object
     *
     * Explicitly releasing the reference to the BusAttachment object makes it
     * so the JavaScript garbage-collector can free up the reference to the native
     * BusAttachment.
     * destroy should be called when the BusAttachment is no longer needed.
     *
     * @param {statusCallback} callback function that should be run once destroy method call has completed
     */
    this.destroy = function (callback) {};

    /**
     * Enables peer-to-peer security.
     *
     * This operation must be called by applications that want to use authentication and
     * encryption.
     *
     * @param {String} authMechanisms the authentication mechanism(s) to use for peer-to-peer
     *                       authentication
     * @param {AuthListener} [listener] an optional listener that receives password and other
     *                 authentication related requests
     * @param {statusCallback} callback function that is run once the call to enablePeerSecurity has completed
     *           callback specifies status OK if peer security was enabled
     */
    this.enablePeerSecurity = function(authMechanisms, listener, callback) {};

    /**
     * Registers interest in a well-known name prefix for the purpose of discovery.
     *
     * This operation is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method
     * call to the local daemon and interprets the response.
     *
     * @param {String} namePrefix well-known name prefix that application is interested in receiving
     *                  BusListener onFoundAdvertisedName notifications about
     * @param {statusCallback} callback function called after the findAdvertisedName method has completed
     *        callback returns BUS_NOT_CONNECTED if a connection has not been made with a local bus
     *        or Other error status codes indicating a failure
     */
    this.findAdvertisedName = function(namePrefix, callback){};

    /**
     * Retrieve an existing activated InterfaceDescription.
     *
     * @param {String} name interface name
     * @param {interfaceDescriptionCallback} callback containing the specified interfaceDescritption.
     */
    this.getInterface = function(name, callback){};

    /**
     * Returns the existing activated InterfaceDescriptions.
     *
     * @param {interfaceDescriptionsCallback} callback containing the specified unterfaceDescritptions
     */
    this.getInterfaces = function(callback) {};

    /**
     * Get the expiration time on keys associated with a specific authenticated
     * remote peer as identified by its peer GUID.
     *
     * The peer GUID associated with a bus name can be obtained by calling
     * getPeerGUID.
     *
     * Throws BusError an error status.  UNKNOWN_GUID if there is no authenticated peer
     * with the specified GUID.  Other error status codes indicating a failure.
     *
     * @param {String} guid the GUID of a remote authenticated peer
     * @param {timeoutCallback} callback A callback function that will be sent the time in seconds relative to the
     *           current time when the keys will expire
     */
    this.getKeyExpiration = function(guid, callback) {};

    /**
     * Gets the peer GUID for this peer or an authenticated remote peer.
     *
     * Peer GUIDs are used by the authentication mechanisms to uniquely and identify a
     * remote application instance. The peer GUID for a remote peer is only available if
     * the remote peer has been authenticated.
     *
     * @param {String} name name of a remote peer or <em>null</em> to get the local (our) peer
     *             GUID
     * @param {guidCallback} callback function that will contain the guid for the local or remote peer depending on the value of name
     */
    this.getPeerGUID = function(name, callback) {};

    /**
     * Check to see if peer security has been enabled for this bus attachment.
     *
     * @param {securityEnabledCallback} callback function which indicates if peer security is enabled
     */
    this.getPeerSecurityEnabled = function(callback) {};

    /**
     * Gets a proxy to a remote bus object.
     *
     * @param {String} name the name, object path, and optional arguments of the remote object of the
     *             form "org.sample.Foo/foo:sessionId=42" where "org.sample.Foo" is the
     *             well-known or unique name, "/foo" is the object path, and ":sessionId=42" is
     *             the optional session ID the be used for communicating with the remote object
     * @param {proxyBusObjectCallback} callback a function containing a proxy to the remote bus
     *             object at the name and object path
     */
    this.getProxyBusObject = function(name, callback) {};

   /**
    * Returns the current non-absolute real-time clock used internally by AllJoyn.
    *
    * This value can be compared with the timestamps on messages to calculate the
    * time since a timestamped message was sent.
    *
    * @param {timestampCallback} callback function that will contain the current
    *             timestamp in milliseconds.
    */
   this.getTimestamp = function(callback) {};

    /**
     * Joins a session.
     *
     * This operation is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call
     * to the local daemon and interprets the response.
     *
     * The parameters are supplied as a named-parameter object literal.
     * <dl>
     * <dt><em>host</em></dt>
     * <dd>mandatory bus name of the attachment that is hosting the the session to be joined</dd>
     * <dt><em>port</em></dt>
     * <dd>mandatory SessionPort of host to be joined</dd>
     * <dt><em>traffic</em></dt>
     * <dd>TrafficType.  The default value is <em>TRAFFIC_MESSAGES</em>.</dd>
     * <dt><em>isMultipoint</em></dt>
     * <dd>boolean indicating if session is multi-point capable.  The default value is
     * <em>false</em>.</dd>
     * <dt><em>proximity</em></dt>
     * <dd>Proximity.  The default value is <em>PROXIMITY_ANY</em>.</dd>
     * <dt><em>transports</em></dt>
     * <dd>TransportMask of the allowed transports.  The default value is
     * <em>TRANSPORT_ANY</em>.</dd>
     * <dt><em>onLost</em></dt>
     * <dd>a <a href="#::ajn::SessionLostListener">SessionLostListener</a> called by the bus
     * when an existing session becomes disconnected</dd>
     * <dt><em>onMemberAdded</em></dt>
     * <dd>a <a href="#::ajn::SessionMemberAddedListener">SessionMemberAddedListener</a> called
     * by the bus when a member of a multipoint session is added</dd>
     * <dt><em>onMemberRemoved</em></dt>
     * <dd>a <a href="#::ajn::SessionMemberRemovedListener">SessionMemberRemovedListener</a> called
     * by the bus when a member of a multipoint session is removed</dd>
     * </dl>
     *
     * @param {object} parameters see above
     * @param {statusCallback} callback function run after joinSession method call has completed
     */
    this.joinSession = function(parameters, callback) {};

    /**
     * Leaves an existing session.
     *
     * This operation is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call
     * to the local daemon and interprets the response.
     *
     * @param {SessionId} id session id
     * @param {statusCallback} callback funtion to run after leaveSession method call has completed
     *         BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *         Other error status codes indicating a failure.
     */

    this.leaveSession = function(id, callback) {};

    /**
     * Gets the file descriptor for a raw (non-message based) session.
     *
     * @param {SessionId} id ID of an existing streaming session
     * @param {soketFdCallback} callback callback that will contain the socket file descriptor for session
     */
    this.getSessionFd = function(id, callback) {};

    /**
     * Determines whether a given well-known name exists on the bus.
     *
     * This operation is a shortcut/helper that issues an org.freedesktop.DBus.NameHasOwner method
     * call to the daemon and interprets the response.
     *
     * @param {String} name The well known name that the caller is inquiring about
     * @param {hasOwnerCallback} callback callback that indicates whether name exists on the bus
     */
    this.nameHasOwner = function(name, callback) {};

    /**
     * Registers an object that will receive bus event notifications.
     *
     * @param {BusListener} listener the object that will receive bus event notifications
     * @param {statusCallback} callback function that is run after registerBusListener has completed
     */
    this.registerBusListener= function(listener, callback) {};

    /**
     * Registers a locally located DBus object.
     *
     * BusError BUS_BAD_OBJ_PATH if the object path is bad.
     *
     * @param {String} objectPath the absolute object path of the DBus object
     * @param {BusObject} busObject the DBus object implementation
     * @param {statusCallback} callback callback function that should be run once the BusObject has
     *         been registered
     */
    this.registerBusObject = function(objectPath, busObject, callback){};

    /**
     * Registers a signal handler.
     *
     * Signals are forwarded to the signal handler if the sender, interface, member and path
     * qualifiers are all met.
     *
     * If multiple identical signal handlers are registered with the same parameters the
     * duplicate instances are discarded.  They do not need to be removed with the
     * <em>unregisterSignalHandler</em> operation.
     *
     * @param {MessageListener} signalListener the function called when the signal is received
     * @param {String} signalName the interface and member of the signal of the form
     *                   "org.sample.Foo.Echoed" where "org.sample.Foo" is
     *                   the interface and "Echoed" is the member
     * @param {String} [sourcePath] the object path of the emitter of the signal
     * @param {statusCallback} callback status callback that will contain:
     *         BUS_NO_SUCH_INTERFACE if the interface part of signalName does not exist.
     *         BUS_NO_SUCH_MEMBER if the member part of signalName does not exist.
     *         Other error status codes indicating a failure.
     */
    this.registerSignalHandler = function(signalListener, signalName, sourcePath, callback){};

    /**
     * Releases a previously requeted well-known name.
     *
     * This operation is a shortcut/helper that issues an org.freedesktop.DBus.ReleaseName method
     * call to the local daemon and interprets the response.
     *
     * @param {String} name well-known name being released
     * @param {statusCallback} callback function that will contain the following status:
     *           BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *           Other error status codes indicating a failure.
     */
    this.releaseName = function(name, callback){};

    /**
     * Reloads the key store for this bus attachment.
     *
     * This function would normally only be called in the case where a single key store
     * is shared between multiple bus attachments, possibly by different
     * applications. It is up to the applications to coordinate how and when the shared
     * key store is modified.
     *
     * @param {statusCallback} callback statusCallback will contain:
     *           OK if the key store was succesfully reloaded.
     *           An error status indicating that the key store reload failed.
     */
    this.reloadKeyStore = function(callback) {};

    /**
     * Removes a DBus match rule.
     *
     * This operation is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch
     * method call to the local daemon.
     *
     * @param {String} rule match rule to be removed (see DBus specification for format of this
     *             string)
     * @param {statusCallback} callback The statusCallback will contain:
     *           BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *           Other error status codes indicating a failure.
     */
    this.removeMatch = function(rule, callback){};

    /**
     * Requests a well-known name.
     *
     * This operation is a shortcut/helper that issues an org.freedesktop.DBus.RequestName method
     * call to the local daemon and interprets the response.
     *
     * @param {String} requestedName well-known name being requested
     * @param {Number} [flags] optional bitmask of DBUS_NAME_FLAG_* constants.  Defaults to <em>0</em>.
     * @param {statusCallback} callback the statusCallback will contain:
     *            BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *            Other error status codes indicating a failure.
     */
    this.requestName = function(DrequestedName, flags, callback){};

    /**
     * Sets the debug level of the local AllJoyn daemon if that daemon was built in debug
     * mode.
     *
     * The debug level can be set for individual subsystems or for "ALL" subsystems.  Common
     * subsystems are "ALLJOYN" for core AllJoyn code, "ALLJOYN_OBJ" for the sessions management
     * code, and "ALLJOYN_NS" for the TCP name services.  Debug levels for specific subsystems
     * override the setting for "ALL" subsystems.  For example if "ALL" is set to 7, but
     * "ALLJOYN_OBJ" is set to 1, then detailed debug output will be generated for all subsystems
     * except for "ALLJOYN_OBJ" which will only generate high level debug output.  "ALL" defaults
     * to 0 which is off, or no debug output.
     *
     * The debug output levels are actually a bit field that controls what output is generated.
     * Those bit fields are described below:
     * <dl>
     * <dt>0x1</dt><dd>High level debug prints (these debug printfs are not common)</dd>
     * <dt>0x2</dt><dd>Normal debug prints (these debug printfs are common)</dd>
     * <dt>0x4</dt><dd>Function call tracing (these debug printfs are used sporadically)</dd>
     * <dt>0x8</dt><dd>Data dump (really only used in the "SOCKET" module - can generate a
     * <b>lot</b> of output)</dd>
     * </dl>
     *
     * Typically, when enabling debug for a subsystem, the level would be set to 7 which enables
     * High level debug, normal debug, and function call tracing.  Setting the level 0, forces
     * debug output to be off for the specified subsystem.
     *
     * @param {String} _module name of the module to generate debug output
     * @param {Number} level debug level to set for the module
     * @param {statusCallback} callback the statusCallback will contain:
     *           BUS_NO_SUCH_OBJECT if daemon was not built in debug mode.
     */
    this.setDaemonDebug = function(_module, level, callback) {};

    /**
     * Set the expiration time on keys associated with a specific remote peer as
     * identified by its peer GUID.
     *
     * The peer GUID associated with a bus name can be obtained by calling
     * getPeerGUID.  If the timeout is 0 this is equivalent to calling clearKeys.
     *
     * @param {String} guid the GUID of a remote authenticated peer
     * @param {Number} timeout the time in seconds relative to the current time to expire the
     *                keys (range 0 to 4,294,967,295)
     * @param {statusCallback} callback status the statusCallback will contain:
     *           OK if the expiration time was successfully set.
     *           UNKNOWN_GUID if there is no authenticated peer with the specified GUID.
     *           Other error status codes indicating a failure.
     */
    this.setKeyExpiration = function(guid, timeout, callback) {};

    /**
     * Set the link timeout for a session.
     *
     * Link timeout is the maximum number of seconds that an unresponsive
     * daemon-to-daemon connection will be monitored before declaring the session lost
     * (via the SessionLostListener onLost callback). Link timeout defaults to 0 which
     * indicates that AllJoyn link monitoring is disabled.
     *
     * Each transport type defines a lower bound on link timeout to avoid defeating
     * transport specific power management algorithms.
     *
     * @param {SessionId} id ID of session whose link timeout will be modified
     * @param {Number} linkTimeout max number of seconds that a link can be unresponsive before
     *                    being declared lost. 0 indicates that AllJoyn link monitoring
     *                    will be disabled. (range 0 to 4,294,967,295)
     * @param {linkTimeoutCallback} callback callback that indicates the resulting (possibly upward) adjusted
     *           linkTimeout value that acceptable to the underlying transport
     */
    this.setLinkTimeout = function(id, linkTimeout, callback) {};

    /**
     * Sets the session listener for an existing session ID.
     *
     * Calling this operation will override the listener set by a previous call to
     * setSessionListener or any listener specified in JoinSession.
     *
     * The listener is supplied as a named-parameter object literal.
     * <dl>
     * <dt><em>onLost</em></dt>
     * <dd>a <a href="#::ajn::SessionLostListener">SessionLostListener</a> called by the bus
     * when an existing session becomes disconnected</dd>
     * <dt><em>onMemberAdded</em></dt>
     * <dd>a <a href="#::ajn::SessionMemberAddedListener">SessionMemberAddedListener</a> called
     * by the bus when a member of a multipoint session is added</dd>
     * <dt><em>onMemberRemoved</em></dt>
     * <dd>a <a href="#::ajn::SessionMemberRemovedListener">SessionMemberRemovedListener</a> called
     * by the bus when a member of a multipoint session is removed</dd>
     * </dl>
     *
     * @param {SessionId} id the session id of an existing session
     * @param {object} listener the listener to associate with the session; see above.  May be
     *                 <em>null</em> to clear the previous listener.
     * @param {statusCallback} callback contains OK if call to setSessionListener was successfull.
     */
    this.setSessionListener = function(id, listener, cb){};

    /**
     * Cancels an existing port binding.
     *
     * @param {SessionPort} port existing session port to be un-bound
     * @param {sessionCallback} callback The sessionCallback contains:
     *           BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *           Other error status codes indicating a failure.
     */
    this.unbindSessionPort = function(port, callback){};

    /**
     * Unregisters an object that was previously registered with addBusListener.
     *
     * @param {BusListener} listener the object instance to unregister as a listener
     * @param {statusCallback} callback indicates function that will be run once unregisterBusListener
     *           method has completed.
     */
    this.unregisterBusListener = function(listener, callback){};

    /**
     * Unregisters a locally located DBus object.
     *
     * @param {String} objectPath the absolute object path of the DBus object
     * @param {statusCallback} callback callback function that should be run once the BusObject has
     *           been unregistered.
     */
    this.unregisterBusObject = function(objectPath, callback) {};

    /**
     * Unregisters a signal handler.
     *
     * This must be called with the identical parameters supplied to <em>registerSignalHandler</em>.
     *
     * @param {MessageListener} signalListener the function called when the signal is received
     * @param {String} signalName the interface and member of the signal of the form
     *                   "org.sample.Foo.Echoed" where "org.sample.Foo" is
     *                   the interface and "Echoed" is the member
     * @param {String} [sourcePath] the optional object path of the emitter of the signal
     * @param {statusCallback} callback statusCallback with contain:
     *           OK if request was successful
     *           BUS_NOT_CONNECTED if a connection has not been made with a local bus.
     *           other error status codes indicating a failure.
     */
    this.unregisterSignalHandler= function(signalListener, signalName, sourcePath, callback) {};
};

//Documentation for the different types of callback functions
/**
 * callback function containing a status code
 *
 * When the previous method call was successful status will be undefined or null
 *
 * @callback statusCallback
 * @param {Status} status AllJoyn status code
 */
var statusCallback = function(status) {};

/**
 * callback function containing the status code and SessionPort number
 *
 * @callback sessionPortCallback
 * @param {Status} status AllJoyn status code
 * @param {Number} port The sessionPort assigned as a result of the bindSessionPort method call. (range 0 to 65,535)
 */
var sessionPortCallback = function(status, port) {};

/**
 * callback function containing the status code and the InterfaceDescription
 *
 * @callback interfaceDescriptionCallback
 * @param {Status} status AllJoyn status code
 * @param {InterfaceDescription} interfaceDescription an AllJoyn InterfaceDescription
 */
var interfaceDescriptionCallback = function(status, interfaceDescription) {};

/**
 * callback function containing the status code and a collection of InterfaceDescritptions
 * 
 * @callback interfaceDescriptionsCallback
 * @param {Status} status AllJoyn status code
 * @param {InterfaceDescription[]} interfaceDescriptions an array of interfaceDescriptions
 * @param {Number} numIfaces the number of interfaces in the interfaceDescriptions param.
 */
var interfaceDescriptionsCallback = function(status, interfaceDescriptions, numIfaces) {};

/**
 * callback function containing the time in seconds relative to the current time when the keys will expire
 *
 * @callback timeoutCallback
 * @param {Status} status AllJoyn status code
 * @param {Number} timeout The time in seconds relative to the current time when the keys will expire
 */
var timeoutCallback = function(status, timeout) {};

/**
 * callback function containing the guid for the local or remote peer
 *
 * @callback guidCallback
 * @param {Status} status AllJoyn status code
 * @param {String} guid the guid string for the local or remote peer
 */
var guidCallback = function(status, guid) {};

/**
 * true if peer security has been enabled, false otherwise.
 *
 * @callback securityEnabledCallback
 * @param {Status} status AllJoyn status code
 * @param {Boolean} enabled true if peer security has been enabled, false otherwise.
 */
var securityEnabledCallback = function(status, enabled) {};

/**
 * callback function containing status code and proxyBusObject
 *
 * @callback proxyBusObjectCallback
 * @param {Status} status AllJoyn status code
 * @param {ProxyBusObject} proxyBusObject the requested ProxyBusObject
 */
var proxyBusObjectCallback = function(status, proxyBusObject) {};

/**
 * callback function containing the current timestamp in milliseconds
 *
 * @callback timestampCallback
 * @param {Status} status AllJoyn status code
 * @param {Number} timestamp The current timestamp in milliseconds
 */
var timestampCallback = function(status, timestamp) {};

/**
 * callback function containing the socket file descriptor for a session
 *
 * @callback socketFdCallback
 * @param {Status} status AllJoyn status code
 * @param {SocketFd} socketFd socket fild descriptor for a session
 */
var socketFdCallback = function(status, socketFd) {};

/**
 * callback function indicating if name ownership was able to be determined
 *
 * @callback hasOwnerCallback
 * @param {Status} status AllJoyn status code
 * @param {Boolean} hasOwner true if name exists on the bus. if status is not OK the hasOwner
 *                           parameter is not modified.
 */
var hasOwnerCallback = function(status, hasOwner) {};

/**
 * callback function indicating the number of second that a link can be unresponcive
 *
 * @callback linkTimeoutCallback
 * @param {Status} status AllJoyn status code:
 *                 OK if successful
 *                 ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED if local daemon does not support SetLinkTimeout
 *                 ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT if SetLinkTimeout not supported by destination
 *                 BUS_NO_SESSION if the Session id is not valid
 *                 ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED if SetLinkTimeout failed
 *                 BUS_NOT_CONNECTED if the BusAttachment is not connected to the daemon
 * @param {Number} linkTimeout this value will be the resulting (possibly upward) adjusted
                               linkTimeout value that is acceptable to the underlying transport.
 */
var linkTimeoutCallback = function(status, linkTimeout) {};
