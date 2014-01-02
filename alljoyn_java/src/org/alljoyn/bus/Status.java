/* This file is auto-generated.  Do not modify. */
/*
 * Copyright (c) 2009-2013, AllSeen Alliance. All rights reserved.
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

/**
 * Standard function return codes for this package.
 */
public enum Status {

    /** <b><tt>0x0</tt></b> Success.. */
    OK(0x0),
    /** <b><tt>0x1</tt></b> Generic failure.. */
    FAIL(0x1),
    /** <b><tt>0x2</tt></b> Conversion between UTF bases failed.. */
    UTF_CONVERSION_FAILED(0x2),
    /** <b><tt>0x3</tt></b> Not enough space in buffer for operation.. */
    BUFFER_TOO_SMALL(0x3),
    /** <b><tt>0x4</tt></b> Underlying OS has indicated an error.. */
    OS_ERROR(0x4),
    /** <b><tt>0x5</tt></b> Failed to allocate memory.. */
    OUT_OF_MEMORY(0x5),
    /** <b><tt>0x6</tt></b> Bind to IP address failed.. */
    SOCKET_BIND_ERROR(0x6),
    /** <b><tt>0x7</tt></b> Initialization failed.. */
    INIT_FAILED(0x7),
    /** <b><tt>0x8</tt></b> An I/O attempt on non-blocking resource would block. */
    WOULDBLOCK(0x8),
    /** <b><tt>0x9</tt></b> Feature not implemented. */
    NOT_IMPLEMENTED(0x9),
    /** <b><tt>0xa</tt></b> Operation timed out. */
    TIMEOUT(0xa),
    /** <b><tt>0xb</tt></b> Other end closed the socket. */
    SOCK_OTHER_END_CLOSED(0xb),
    /** <b><tt>0xc</tt></b> Function call argument 1 is invalid. */
    BAD_ARG_1(0xc),
    /** <b><tt>0xd</tt></b> Function call argument 2 is invalid. */
    BAD_ARG_2(0xd),
    /** <b><tt>0xe</tt></b> Function call argument 3 is invalid. */
    BAD_ARG_3(0xe),
    /** <b><tt>0xf</tt></b> Function call argument 4 is invalid. */
    BAD_ARG_4(0xf),
    /** <b><tt>0x10</tt></b> Function call argument 5 is invalid. */
    BAD_ARG_5(0x10),
    /** <b><tt>0x11</tt></b> Function call argument 6 is invalid. */
    BAD_ARG_6(0x11),
    /** <b><tt>0x12</tt></b> Function call argument 7 is invalid. */
    BAD_ARG_7(0x12),
    /** <b><tt>0x13</tt></b> Function call argument 8 is invalid. */
    BAD_ARG_8(0x13),
    /** <b><tt>0x14</tt></b> Address is NULL or invalid. */
    INVALID_ADDRESS(0x14),
    /** <b><tt>0x15</tt></b> Generic invalid data error. */
    INVALID_DATA(0x15),
    /** <b><tt>0x16</tt></b> Generic read error. */
    READ_ERROR(0x16),
    /** <b><tt>0x17</tt></b> Generic write error. */
    WRITE_ERROR(0x17),
    /** <b><tt>0x18</tt></b> Generic open failure. */
    OPEN_FAILED(0x18),
    /** <b><tt>0x19</tt></b> Generic parse failure. */
    PARSE_ERROR(0x19),
    /** <b><tt>0x1A</tt></b> Generic EOD/EOF error. */
    END_OF_DATA(0x1A),
    /** <b><tt>0x1B</tt></b> Connection was refused because no one is listening. */
    CONN_REFUSED(0x1B),
    /** <b><tt>0x1C</tt></b> Incorrect number of arguments given to function call. */
    BAD_ARG_COUNT(0x1C),
    /** <b><tt>0x1D</tt></b> Generic warning. */
    WARNING(0x1D),
    /** <b><tt>0x1000</tt></b> Error code block for the Common subsystem.. */
    COMMON_ERRORS(0x1000),
    /** <b><tt>0x1001</tt></b> Operation interrupted by ERThread stop signal.. */
    STOPPING_THREAD(0x1001),
    /** <b><tt>0x1002</tt></b> Operation interrupted by ERThread alert signal.. */
    ALERTED_THREAD(0x1002),
    /** <b><tt>0x1003</tt></b> Cannot parse malformed XML. */
    XML_MALFORMED(0x1003),
    /** <b><tt>0x1004</tt></b> Authentication failed. */
    AUTH_FAIL(0x1004),
    /** <b><tt>0x1005</tt></b> Authentication was rejected by user. */
    AUTH_USER_REJECT(0x1005),
    /** <b><tt>0x1006</tt></b> Attempt to reference non-existent timer alarm. */
    NO_SUCH_ALARM(0x1006),
    /** <b><tt>0x1007</tt></b> A timer thread is missing scheduled alarm times. */
    TIMER_FALLBEHIND(0x1007),
    /** <b><tt>0x1008</tt></b> Error code block for SSL subsystem. */
    SSL_ERRORS(0x1008),
    /** <b><tt>0x1009</tt></b> SSL initialization failed.. */
    SSL_INIT(0x1009),
    /** <b><tt>0x100a</tt></b> Failed to connect to remote host using SSL. */
    SSL_CONNECT(0x100a),
    /** <b><tt>0x100b</tt></b> Failed to verify identity of SSL destination. */
    SSL_VERIFY(0x100b),
    /** <b><tt>0x100c</tt></b> Operation not supported on external thread wrapper. */
    EXTERNAL_THREAD(0x100c),
    /** <b><tt>0x100d</tt></b> Non-specific error in the crypto subsystem. */
    CRYPTO_ERROR(0x100d),
    /** <b><tt>0x100e</tt></b> Not enough room for key. */
    CRYPTO_TRUNCATED(0x100e),
    /** <b><tt>0x100f</tt></b> No key to return. */
    CRYPTO_KEY_UNAVAILABLE(0x100f),
    /** <b><tt>0x1010</tt></b> Cannot lookup hostname. */
    BAD_HOSTNAME(0x1010),
    /** <b><tt>0x1011</tt></b> Key cannot be used. */
    CRYPTO_KEY_UNUSABLE(0x1011),
    /** <b><tt>0x1012</tt></b> Key blob is empty. */
    EMPTY_KEY_BLOB(0x1012),
    /** <b><tt>0x1013</tt></b> Key blob is corrupted. */
    CORRUPT_KEYBLOB(0x1013),
    /** <b><tt>0x1014</tt></b> Encoded key is not valid. */
    INVALID_KEY_ENCODING(0x1014),
    /** <b><tt>0x1015</tt></b> Operation not allowed thread is dead. */
    DEAD_THREAD(0x1015),
    /** <b><tt>0x1016</tt></b> Cannot start a thread that is already running. */
    THREAD_RUNNING(0x1016),
    /** <b><tt>0x1017</tt></b> Cannot start a thread that is already stopping. */
    THREAD_STOPPING(0x1017),
    /** <b><tt>0x1018</tt></b> Encoded string did not have the expected format or contents. */
    BAD_STRING_ENCODING(0x1018),
    /** <b><tt>0x1019</tt></b> Crypto algorithm parameters do not provide sufficient security. */
    CRYPTO_INSUFFICIENT_SECURITY(0x1019),
    /** <b><tt>0x101a</tt></b> Crypto algorithm parameter value is illegal. */
    CRYPTO_ILLEGAL_PARAMETERS(0x101a),
    /** <b><tt>0x101b</tt></b> Cryptographic hash function must be initialized. */
    CRYPTO_HASH_UNINITIALIZED(0x101b),
    /** <b><tt>0x101c</tt></b> Thread cannot be blocked by a WAIT or SLEEP call. */
    THREAD_NO_WAIT(0x101c),
    /** <b><tt>0x101d</tt></b> Cannot add an alarm to a timer that is exiting. */
    TIMER_EXITING(0x101d),
    /** <b><tt>0x101e</tt></b> String is not a hex encoded GUID string. */
    INVALID_GUID(0x101e),
    /** <b><tt>0x101f</tt></b> A thread pool has reached its specified concurrency. */
    THREADPOOL_EXHAUSTED(0x101f),
    /** <b><tt>0x1020</tt></b> Cannot execute a closure on a stopping thread pool. */
    THREADPOOL_STOPPING(0x1020),
    /** <b><tt>0x1021</tt></b> Attempt to reference non-existent stream entry. */
    INVALID_STREAM(0x1021),
    /** <b><tt>0x1022</tt></b> Attempt to reference non-existent stream entry. */
    TIMER_FULL(0x1022),
    /** <b><tt>0x1023</tt></b> Cannot execute a read or write command on an IODispatch thread because it is stopping.. */
    IODISPATCH_STOPPING(0x1023),
    /** <b><tt>0x1024</tt></b> Length of SLAP packet is invalid.. */
    SLAP_INVALID_PACKET_LEN(0x1024),
    /** <b><tt>0x1025</tt></b> SLAP packet header checksum error.. */
    SLAP_HDR_CHECKSUM_ERROR(0x1025),
    /** <b><tt>0x1026</tt></b> Invalid SLAP packet type.. */
    SLAP_INVALID_PACKET_TYPE(0x1026),
    /** <b><tt>0x1027</tt></b> Calculated length does not match the received length.. */
    SLAP_LEN_MISMATCH(0x1027),
    /** <b><tt>0x1028</tt></b> Packet type does not match reliability bit.. */
    SLAP_PACKET_TYPE_MISMATCH(0x1028),
    /** <b><tt>0x1029</tt></b> SLAP packet CRC error.. */
    SLAP_CRC_ERROR(0x1029),
    /** <b><tt>0x102A</tt></b> Generic SLAP error.. */
    SLAP_ERROR(0x102A),
    /** <b><tt>0x102B</tt></b> Other end closed the SLAP connection. */
    SLAP_OTHER_END_CLOSED(0x102B),
    /** <b><tt>0xffff</tt></b> No error code to report. */
    NONE(0xffff),
    /** <b><tt>0x9000</tt></b> Error code block for ALLJOYN wire protocol. */
    BUS_ERRORS(0x9000),
    /** <b><tt>0x9001</tt></b> Error attempting to read. */
    BUS_READ_ERROR(0x9001),
    /** <b><tt>0x9002</tt></b> Error attempting to write. */
    BUS_WRITE_ERROR(0x9002),
    /** <b><tt>0x9003</tt></b> Read an invalid value type. */
    BUS_BAD_VALUE_TYPE(0x9003),
    /** <b><tt>0x9004</tt></b> Read an invalid header field. */
    BUS_BAD_HEADER_FIELD(0x9004),
    /** <b><tt>0x9005</tt></b> Signature was badly formed. */
    BUS_BAD_SIGNATURE(0x9005),
    /** <b><tt>0x9006</tt></b> Object path contained an illegal character. */
    BUS_BAD_OBJ_PATH(0x9006),
    /** <b><tt>0x9007</tt></b> A member name contained an illegal character. */
    BUS_BAD_MEMBER_NAME(0x9007),
    /** <b><tt>0x9008</tt></b> An interface name contained an illegal character. */
    BUS_BAD_INTERFACE_NAME(0x9008),
    /** <b><tt>0x9009</tt></b> An error name contained an illegal character. */
    BUS_BAD_ERROR_NAME(0x9009),
    /** <b><tt>0x900a</tt></b> A bus name contained an illegal character. */
    BUS_BAD_BUS_NAME(0x900a),
    /** <b><tt>0x900b</tt></b> A name exceeded the permitted length. */
    BUS_NAME_TOO_LONG(0x900b),
    /** <b><tt>0x900c</tt></b> Length of an array was not a multiple of the array element size. */
    BUS_BAD_LENGTH(0x900c),
    /** <b><tt>0x900d</tt></b> Parsed value in a message was invalid (for example: boolean > 1) . */
    BUS_BAD_VALUE(0x900d),
    /** <b><tt>0x900e</tt></b> Unknown header flags. */
    BUS_BAD_HDR_FLAGS(0x900e),
    /** <b><tt>0x900f</tt></b> Body length was to long or too short. */
    BUS_BAD_BODY_LEN(0x900f),
    /** <b><tt>0x9010</tt></b> Header length was to long or too short. */
    BUS_BAD_HEADER_LEN(0x9010),
    /** <b><tt>0x9011</tt></b> Serial number in a method response was unknown. */
    BUS_UNKNOWN_SERIAL(0x9011),
    /** <b><tt>0x9012</tt></b> Path in a method call or signal was unknown. */
    BUS_UNKNOWN_PATH(0x9012),
    /** <b><tt>0x9013</tt></b> Interface in a method call or signal was unknown. */
    BUS_UNKNOWN_INTERFACE(0x9013),
    /** <b><tt>0x9014</tt></b> Failed to establish a connection. */
    BUS_ESTABLISH_FAILED(0x9014),
    /** <b><tt>0x9015</tt></b> Signature in message was not what was expected. */
    BUS_UNEXPECTED_SIGNATURE(0x9015),
    /** <b><tt>0x9016</tt></b> Interface header field is missing. */
    BUS_INTERFACE_MISSING(0x9016),
    /** <b><tt>0x9017</tt></b> Object path header field is missing. */
    BUS_PATH_MISSING(0x9017),
    /** <b><tt>0x9018</tt></b> Member header field is missing. */
    BUS_MEMBER_MISSING(0x9018),
    /** <b><tt>0x9019</tt></b> Reply-Serial header field is missing. */
    BUS_REPLY_SERIAL_MISSING(0x9019),
    /** <b><tt>0x901a</tt></b> Error Name header field is missing. */
    BUS_ERROR_NAME_MISSING(0x901a),
    /** <b><tt>0x901b</tt></b> Interface does not have the requested member. */
    BUS_INTERFACE_NO_SUCH_MEMBER(0x901b),
    /** <b><tt>0x901c</tt></b> Object does not exist. */
    BUS_NO_SUCH_OBJECT(0x901c),
    /** <b><tt>0x901d</tt></b> Object does not have the requested member (on any interface). */
    BUS_OBJECT_NO_SUCH_MEMBER(0x901d),
    /** <b><tt>0x901e</tt></b> Object does not have the requested interface. */
    BUS_OBJECT_NO_SUCH_INTERFACE(0x901e),
    /** <b><tt>0x901f</tt></b> Requested interface does not exist. */
    BUS_NO_SUCH_INTERFACE(0x901f),
    /** <b><tt>0x9020</tt></b> Member exists but does not have the requested signature. */
    BUS_MEMBER_NO_SUCH_SIGNATURE(0x9020),
    /** <b><tt>0x9021</tt></b> A string or signature was not NUL terminated. */
    BUS_NOT_NUL_TERMINATED(0x9021),
    /** <b><tt>0x9022</tt></b> No such property for a GET or SET operation . */
    BUS_NO_SUCH_PROPERTY(0x9022),
    /** <b><tt>0x9023</tt></b> Attempt to set a property value with the wrong signature. */
    BUS_SET_WRONG_SIGNATURE(0x9023),
    /** <b><tt>0x9024</tt></b> Attempt to get a property whose value has not been set. */
    BUS_PROPERTY_VALUE_NOT_SET(0x9024),
    /** <b><tt>0x9025</tt></b> Attempt to set or get a property failed due to access rights. */
    BUS_PROPERTY_ACCESS_DENIED(0x9025),
    /** <b><tt>0x9026</tt></b> No physical message transports were specified. */
    BUS_NO_TRANSPORTS(0x9026),
    /** <b><tt>0x9027</tt></b> Missing or badly formatted transports args specified. */
    BUS_BAD_TRANSPORT_ARGS(0x9027),
    /** <b><tt>0x9028</tt></b> Message cannot be routed to destination. */
    BUS_NO_ROUTE(0x9028),
    /** <b><tt>0x9029</tt></b> An endpoint with given name cannot be found. */
    BUS_NO_ENDPOINT(0x9029),
    /** <b><tt>0x902a</tt></b> Bad parameter in send message call. */
    BUS_BAD_SEND_PARAMETER(0x902a),
    /** <b><tt>0x902b</tt></b> Serial number in method call reply message did not match any method calls. */
    BUS_UNMATCHED_REPLY_SERIAL(0x902b),
    /** <b><tt>0x902c</tt></b> Sender identifier is invalid. */
    BUS_BAD_SENDER_ID(0x902c),
    /** <b><tt>0x902d</tt></b> Attempt to send on a transport that has not been started. */
    BUS_TRANSPORT_NOT_STARTED(0x902d),
    /** <b><tt>0x902e</tt></b> Attempt to deliver an empty message. */
    BUS_EMPTY_MESSAGE(0x902e),
    /** <b><tt>0x902f</tt></b> A bus name operation was not permitted because sender does not own name. */
    BUS_NOT_OWNER(0x902f),
    /** <b><tt>0x9030</tt></b> Application rejected a request to set a property. */
    BUS_SET_PROPERTY_REJECTED(0x9030),
    /** <b><tt>0x9031</tt></b> Connection failed. */
    BUS_CONNECT_FAILED(0x9031),
    /** <b><tt>0x9032</tt></b> Response from a method call was an ERROR message. */
    BUS_REPLY_IS_ERROR_MESSAGE(0x9032),
    /** <b><tt>0x9033</tt></b> Not in an authentication conversation. */
    BUS_NOT_AUTHENTICATING(0x9033),
    /** <b><tt>0x9034</tt></b> A listener is required to implement the requested function. */
    BUS_NO_LISTENER(0x9034),
    /** <b><tt>0x9035</tt></b> The Bluetooth transport reported an error. */
    BUS_BT_TRANSPORT_ERROR(0x9035),
    /** <b><tt>0x9036</tt></b> The operation attempted is not allowed. */
    BUS_NOT_ALLOWED(0x9036),
    /** <b><tt>0x9037</tt></b> Write failed because write queue is full. */
    BUS_WRITE_QUEUE_FULL(0x9037),
    /** <b><tt>0x9038</tt></b> Operation not permitted on endpoint in process of closing. */
    BUS_ENDPOINT_CLOSING(0x9038),
    /** <b><tt>0x9039</tt></b> Received two conflicting definitions for the same interface. */
    BUS_INTERFACE_MISMATCH(0x9039),
    /** <b><tt>0x903a</tt></b> Attempt to add a member to an interface that already exists. */
    BUS_MEMBER_ALREADY_EXISTS(0x903a),
    /** <b><tt>0x903b</tt></b> Attempt to add a property to an interface that already exists. */
    BUS_PROPERTY_ALREADY_EXISTS(0x903b),
    /** <b><tt>0x903c</tt></b> Attempt to add an interface to an object that already exists. */
    BUS_IFACE_ALREADY_EXISTS(0x903c),
    /** <b><tt>0x903d</tt></b> Received an error response to a method call. */
    BUS_ERROR_RESPONSE(0x903d),
    /** <b><tt>0x903e</tt></b> XML data is improperly formatted. */
    BUS_BAD_XML(0x903e),
    /** <b><tt>0x903f</tt></b> The path of a child object is incorrect given it's parent's path. */
    BUS_BAD_CHILD_PATH(0x903f),
    /** <b><tt>0x9040</tt></b> Attempt to add a RemoteObject child that already exists. */
    BUS_OBJ_ALREADY_EXISTS(0x9040),
    /** <b><tt>0x9041</tt></b> Object with given path does not exist. */
    BUS_OBJ_NOT_FOUND(0x9041),
    /** <b><tt>0x9042</tt></b> Expansion information for a compressed message is not available. */
    BUS_CANNOT_EXPAND_MESSAGE(0x9042),
    /** <b><tt>0x9043</tt></b> Attempt to expand a message that is not compressed. */
    BUS_NOT_COMPRESSED(0x9043),
    /** <b><tt>0x9044</tt></b> Attempt to connect to a bus which is already connected. */
    BUS_ALREADY_CONNECTED(0x9044),
    /** <b><tt>0x9045</tt></b> Attempt to use a bus attachment that is not connected to a bus daemon. */
    BUS_NOT_CONNECTED(0x9045),
    /** <b><tt>0x9046</tt></b> Attempt to listen on a bus address which is already being listened on. */
    BUS_ALREADY_LISTENING(0x9046),
    /** <b><tt>0x9047</tt></b> The request key is not available. */
    BUS_KEY_UNAVAILABLE(0x9047),
    /** <b><tt>0x9048</tt></b> Insufficient memory to copy data. */
    BUS_TRUNCATED(0x9048),
    /** <b><tt>0x9049</tt></b> Accessing the key store before it is loaded. */
    BUS_KEY_STORE_NOT_LOADED(0x9049),
    /** <b><tt>0x904a</tt></b> There is no authentication mechanism. */
    BUS_NO_AUTHENTICATION_MECHANISM(0x904a),
    /** <b><tt>0x904b</tt></b> Bus has already been started. */
    BUS_BUS_ALREADY_STARTED(0x904b),
    /** <b><tt>0x904c</tt></b> Bus has not yet been started. */
    BUS_BUS_NOT_STARTED(0x904c),
    /** <b><tt>0x904d</tt></b> The operation requested cannot be performed using this key blob. */
    BUS_KEYBLOB_OP_INVALID(0x904d),
    /** <b><tt>0x904e</tt></b> Invalid header checksum in an encrypted message. */
    BUS_INVALID_HEADER_CHECKSUM(0x904e),
    /** <b><tt>0x904f</tt></b> Security policy requires the message to be encrypted. */
    BUS_MESSAGE_NOT_ENCRYPTED(0x904f),
    /** <b><tt>0x9050</tt></b> Serial number in message header is invalid. */
    BUS_INVALID_HEADER_SERIAL(0x9050),
    /** <b><tt>0x9051</tt></b> Message time-to-live has expired. */
    BUS_TIME_TO_LIVE_EXPIRED(0x9051),
    /** <b><tt>0x9052</tt></b> Something is wrong with a header expansion. */
    BUS_HDR_EXPANSION_INVALID(0x9052),
    /** <b><tt>0x9053</tt></b> Compressed headers require a compression token. */
    BUS_MISSING_COMPRESSION_TOKEN(0x9053),
    /** <b><tt>0x9054</tt></b> There is no GUID for this peer. */
    BUS_NO_PEER_GUID(0x9054),
    /** <b><tt>0x9055</tt></b> Message decryption failed. */
    BUS_MESSAGE_DECRYPTION_FAILED(0x9055),
    /** <b><tt>0x9056</tt></b> A fatal security failure. */
    BUS_SECURITY_FATAL(0x9056),
    /** <b><tt>0x9057</tt></b> An encryption key has expired. */
    BUS_KEY_EXPIRED(0x9057),
    /** <b><tt>0x9058</tt></b> Key store is corrupt. */
    BUS_CORRUPT_KEYSTORE(0x9058),
    /** <b><tt>0x9059</tt></b> A reply only allowed in response to a method call. */
    BUS_NO_CALL_FOR_REPLY(0x9059),
    /** <b><tt>0x905a</tt></b> Signature must be a single complete type. */
    BUS_NOT_A_COMPLETE_TYPE(0x905a),
    /** <b><tt>0x905b</tt></b> Message does not meet policy restrictions. */
    BUS_POLICY_VIOLATION(0x905b),
    /** <b><tt>0x905c</tt></b> Service name is unknown. */
    BUS_NO_SUCH_SERVICE(0x905c),
    /** <b><tt>0x905d</tt></b> Transport cannot be used due to underlying mechanism disabled by OS. */
    BUS_TRANSPORT_NOT_AVAILABLE(0x905d),
    /** <b><tt>0x905e</tt></b> Authentication mechanism is not valid. */
    BUS_INVALID_AUTH_MECHANISM(0x905e),
    /** <b><tt>0x905f</tt></b> Key store has wrong version number. */
    BUS_KEYSTORE_VERSION_MISMATCH(0x905f),
    /** <b><tt>0x9060</tt></b> A synchronous method call from within handler is not permitted.. */
    BUS_BLOCKING_CALL_NOT_ALLOWED(0x9060),
    /** <b><tt>0x9061</tt></b> MsgArg(s) do not match signature.. */
    BUS_SIGNATURE_MISMATCH(0x9061),
    /** <b><tt>0x9062</tt></b> The bus is stopping.. */
    BUS_STOPPING(0x9062),
    /** <b><tt>0x9063</tt></b> The method call was aborted.. */
    BUS_METHOD_CALL_ABORTED(0x9063),
    /** <b><tt>0x9064</tt></b> An interface cannot be added to an object that is already registered.. */
    BUS_CANNOT_ADD_INTERFACE(0x9064),
    /** <b><tt>0x9065</tt></b> A method handler cannot be added to an object that is already registered.. */
    BUS_CANNOT_ADD_HANDLER(0x9065),
    /** <b><tt>0x9066</tt></b> Key store has not been loaded. */
    BUS_KEYSTORE_NOT_LOADED(0x9066),
    /** <b><tt>0x906b</tt></b> Handle is not in the handle table. */
    BUS_NO_SUCH_HANDLE(0x906b),
    /** <b><tt>0x906c</tt></b> Passing of handles is not enabled for this connection. */
    BUS_HANDLES_NOT_ENABLED(0x906c),
    /** <b><tt>0x906d</tt></b> Message had more handles than expected. */
    BUS_HANDLES_MISMATCH(0x906d),
    /** <b><tt>0x906e</tt></b> Maximum Bluetooth connections already in use. */
    BT_MAX_CONNECTIONS_USED(0x906e),
    /** <b><tt>0x906f</tt></b> Session id is not valid. */
    BUS_NO_SESSION(0x906f),
    /** <b><tt>0x9070</tt></b> Dictionary element was not found. */
    BUS_ELEMENT_NOT_FOUND(0x9070),
    /** <b><tt>0x9071</tt></b> MsgArg was not an array of dictionary elements. */
    BUS_NOT_A_DICTIONARY(0x9071),
    /** <b><tt>0x9072</tt></b> Wait failed. */
    BUS_WAIT_FAILED(0x9072),
    /** <b><tt>0x9074</tt></b> Session options are bad or incompatible. */
    BUS_BAD_SESSION_OPTS(0x9074),
    /** <b><tt>0x9075</tt></b> Incoming connection rejected. */
    BUS_CONNECTION_REJECTED(0x9075),
    /** <b><tt>0x9076</tt></b> RequestName reply: Name was successfully obtained. */
    DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER(0x9076),
    /** <b><tt>0x9077</tt></b> RequestName reply: Name is already owned, request for name has been queued. */
    DBUS_REQUEST_NAME_REPLY_IN_QUEUE(0x9077),
    /** <b><tt>0x9078</tt></b> RequestName reply: Name is already owned and DO_NOT_QUEUE was specified in request. */
    DBUS_REQUEST_NAME_REPLY_EXISTS(0x9078),
    /** <b><tt>0x9079</tt></b> RequestName reply: Name is already owned by this endpoint. */
    DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER(0x9079),
    /** <b><tt>0x907a</tt></b> ReleaseName reply: Name was released. */
    DBUS_RELEASE_NAME_REPLY_RELEASED(0x907a),
    /** <b><tt>0x907b</tt></b>  ReleaseName reply: Name does not exist. */
    DBUS_RELEASE_NAME_REPLY_NON_EXISTENT(0x907b),
    /** <b><tt>0x907c</tt></b> ReleaseName reply: Request to release name that is not owned by this endpoint. */
    DBUS_RELEASE_NAME_REPLY_NOT_OWNER(0x907c),
    /** <b><tt>0x907e</tt></b> StartServiceByName reply: Service is already running. */
    DBUS_START_REPLY_ALREADY_RUNNING(0x907e),
    /** <b><tt>0x9080</tt></b> BindSessionPort reply: SessionPort already exists. */
    ALLJOYN_BINDSESSIONPORT_REPLY_ALREADY_EXISTS(0x9080),
    /** <b><tt>0x9081</tt></b> BindSessionPort reply: Failed. */
    ALLJOYN_BINDSESSIONPORT_REPLY_FAILED(0x9081),
    /** <b><tt>0x9083</tt></b> JoinSession reply: Session with given name does not exist. */
    ALLJOYN_JOINSESSION_REPLY_NO_SESSION(0x9083),
    /** <b><tt>0x9084</tt></b> JoinSession reply: Failed to find suitable transport. */
    ALLJOYN_JOINSESSION_REPLY_UNREACHABLE(0x9084),
    /** <b><tt>0x9085</tt></b> JoinSession reply: Connect to advertised address. */
    ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED(0x9085),
    /** <b><tt>0x9086</tt></b> JoinSession reply: The session creator rejected the join req. */
    ALLJOYN_JOINSESSION_REPLY_REJECTED(0x9086),
    /** <b><tt>0x9087</tt></b> JoinSession reply: Failed due to session option incompatibilities. */
    ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS(0x9087),
    /** <b><tt>0x9088</tt></b> JoinSession reply: Failed for unknown reason. */
    ALLJOYN_JOINSESSION_REPLY_FAILED(0x9088),
    /** <b><tt>0x908a</tt></b> LeaveSession reply: Session with given name does not exist. */
    ALLJOYN_LEAVESESSION_REPLY_NO_SESSION(0x908a),
    /** <b><tt>0x908b</tt></b> LeaveSession reply: Failed for unspecified reason. */
    ALLJOYN_LEAVESESSION_REPLY_FAILED(0x908b),
    /** <b><tt>0x908d</tt></b> AdvertiseName reply: This endpoint is already advertising this name. */
    ALLJOYN_ADVERTISENAME_REPLY_ALREADY_ADVERTISING(0x908d),
    /** <b><tt>0x908e</tt></b> AdvertiseName reply: Advertise failed. */
    ALLJOYN_ADVERTISENAME_REPLY_FAILED(0x908e),
    /** <b><tt>0x9090</tt></b> CancelAdvertiseName reply: Advertise failed. */
    ALLJOYN_CANCELADVERTISENAME_REPLY_FAILED(0x9090),
    /** <b><tt>0x9092</tt></b> FindAdvertisedName reply: This endpoint is already discovering this name. */
    ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING(0x9092),
    /** <b><tt>0x9093</tt></b> FindAdvertisedName reply: Failed. */
    ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED(0x9093),
    /** <b><tt>0x9095</tt></b> CancelFindAdvertisedName reply: Failed. */
    ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED(0x9095),
    /** <b><tt>0x9096</tt></b> An unexpected disposition was returned and has been treated as an error. */
    BUS_UNEXPECTED_DISPOSITION(0x9096),
    /** <b><tt>0x9097</tt></b> An InterfaceDescription cannot be modified once activated. */
    BUS_INTERFACE_ACTIVATED(0x9097),
    /** <b><tt>0x9098</tt></b> UnbindSessionPort reply: SessionPort does not exist. */
    ALLJOYN_UNBINDSESSIONPORT_REPLY_BAD_PORT(0x9098),
    /** <b><tt>0x9099</tt></b> UnbindSessionPort reply: Failed. */
    ALLJOYN_UNBINDSESSIONPORT_REPLY_FAILED(0x9099),
    /** <b><tt>0x909a</tt></b> BindSessionPort reply: SessionOpts are invalid. */
    ALLJOYN_BINDSESSIONPORT_REPLY_INVALID_OPTS(0x909a),
    /** <b><tt>0x909b</tt></b> JoinSession reply: Caller has already joined the session. */
    ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED(0x909b),
    /** <b><tt>0x909c</tt></b> Received BusHello from self. */
    BUS_SELF_CONNECT(0x909c),
    /** <b><tt>0x909d</tt></b> Security is not enabled for this bus attachment. */
    BUS_SECURITY_NOT_ENABLED(0x909d),
    /** <b><tt>0x909e</tt></b> A listener has already been set. */
    BUS_LISTENER_ALREADY_SET(0x909e),
    /** <b><tt>0x909f</tt></b> Incompatible peer authentication version numbers. */
    BUS_PEER_AUTH_VERSION_MISMATCH(0x909f),
    /** <b><tt>0x90a0</tt></b> Local daemon does not support SetLinkTimeout. */
    ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED(0x90a0),
    /** <b><tt>0x90a1</tt></b> SetLinkTimeout not supported by destination. */
    ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT(0x90a1),
    /** <b><tt>0x90a2</tt></b> SetLinkTimeout failed. */
    ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED(0x90a2),
    /** <b><tt>0x90a3</tt></b> No permission to use Wifi/Bluetooth. */
    ALLJOYN_ACCESS_PERMISSION_WARNING(0x90a3),
    /** <b><tt>0x90a4</tt></b> No permission to access peer service. */
    ALLJOYN_ACCESS_PERMISSION_ERROR(0x90a4),
    /** <b><tt>0x90a5</tt></b> Cannot send a signal to a destination that is not authenticated. */
    BUS_DESTINATION_NOT_AUTHENTICATED(0x90a5),
    /** <b><tt>0x90a6</tt></b> Endpoint was redirected to another address. */
    BUS_ENDPOINT_REDIRECTED(0x90a6),
    /** <b><tt>0x90a7</tt></b> Authentication of remote peer is pending. */
    BUS_AUTHENTICATION_PENDING(0x90a7),
    /** <b><tt>0x90a8</tt></b> Operation was not authorized. */
    BUS_NOT_AUTHORIZED(0x90a8),
    /** <b><tt>0x90a9</tt></b> Received packet for unknown channel. */
    PACKET_BUS_NO_SUCH_CHANNEL(0x90a9),
    /** <b><tt>0x90aa</tt></b> Received packet with incorrect header information. */
    PACKET_BAD_FORMAT(0x90aa),
    /** <b><tt>0x90ab</tt></b> Timed out waiting for connect response. */
    PACKET_CONNECT_TIMEOUT(0x90ab),
    /** <b><tt>0x90ac</tt></b> Failed to create new comm channel. */
    PACKET_CHANNEL_FAIL(0x90ac),
    /** <b><tt>0x90ad</tt></b> Message too large for use with packet based transport. */
    PACKET_TOO_LARGE(0x90ad),
    /** <b><tt>0x90ae</tt></b> Invalid PacketEngine control packet received. */
    PACKET_BAD_PARAMETER(0x90ae),
    /** <b><tt>0x90af</tt></b> Packet has invalid CRC. */
    PACKET_BAD_CRC(0x90af),
    /** <b><tt>0x90b0</tt></b> STUN attribute size does not match size parsed. */
    STUN_ATTR_SIZE_MISMATCH(0x90b0),
    /** <b><tt>0x90b1</tt></b> STUN server has denied request, issued Challenge. */
    STUN_AUTH_CHALLENGE(0x90b1),
    /** <b><tt>0x90b2</tt></b> Underlying socket not open for operation. */
    STUN_SOCKET_NOT_OPEN(0x90b2),
    /** <b><tt>0x90b3</tt></b> Underlying socket alread open. */
    STUN_SOCKET_OPEN(0x90b3),
    /** <b><tt>0x90b4</tt></b> Failed to send STUN message. */
    STUN_FAILED_TO_SEND_MSG(0x90b4),
    /** <b><tt>0x90b5</tt></b> Application specified invalid TCP framing. */
    STUN_FRAMING_ERROR(0x90b5),
    /** <b><tt>0x90b6</tt></b> Invalid STUN error code. */
    STUN_INVALID_ERROR_CODE(0x90b6),
    /** <b><tt>0x90b7</tt></b> Fingerprint CRC does not match. */
    STUN_INVALID_FINGERPRINT(0x90b7),
    /** <b><tt>0x90b8</tt></b> Invalid address family value in STUN 'address' attribute. */
    STUN_INVALID_ADDR_FAMILY(0x90b8),
    /** <b><tt>0x90b9</tt></b> SHA1-HMAC message integrity value does not match. When passed to upper layer, indicates unauthorized response, message must be ignored. */
    STUN_INVALID_MESSAGE_INTEGRITY(0x90b9),
    /** <b><tt>0x90ba</tt></b> Invalid STUN message type. */
    STUN_INVALID_MSG_TYPE(0x90ba),
    /** <b><tt>0x90bb</tt></b> Invalid STUN message attribute type. */
    STUN_INVALID_ATTR_TYPE(0x90bb),
    /** <b><tt>0x90bc</tt></b> STUN response message included a USERNAME attribute. */
    STUN_RESPONSE_WITH_USERNAME(0x90bc),
    /** <b><tt>0x90bd</tt></b> Received bad STUN request, upper layer must send error code 400. */
    STUN_ERR400_BAD_REQUEST(0x90bd),
    /** <b><tt>0x90be</tt></b> Received bad STUN indication, upper layer must ignore message. */
    STUN_BAD_INDICATION(0x90be),
    /** <b><tt>0x90bf</tt></b> Received STUN request with invalid USERNAME or invalid MESSAGE-INTEGRITY, upper layer must send error code 401. */
    STUN_ERR401_UNAUTHORIZED_REQUEST(0x90bf),
    /** <b><tt>0x90c0</tt></b> Too many attributes in STUN message or unknown attributes list. */
    STUN_TOO_MANY_ATTRIBUTES(0x90c0),
    /** <b><tt>0x90c1</tt></b> STUN message attribute must only be added once. */
    STUN_DUPLICATE_ATTRIBUTE(0x90c1),
    /** <b><tt>0x90c2</tt></b> Receive STUN indication with invalid USERNAME or invalid MESSAGE-INTEGRITY, upper layer must ignore message. */
    STUN_UNAUTHORIZED_INDICATION(0x90c2),
    /** <b><tt>0x90c3</tt></b> Unable to allocate heap from ICE. */
    ICE_ALLOCATING_MEMORY(0x90c3),
    /** <b><tt>0x90c4</tt></b> ICE Checks have not completed. */
    ICE_CHECKS_INCOMPLETE(0x90c4),
    /** <b><tt>0x90c5</tt></b> TURN server rejected ALLOCATE request. */
    ICE_ALLOCATE_REJECTED_NO_RESOURCES(0x90c5),
    /** <b><tt>0x90c6</tt></b> TURN server rejected with 486. */
    ICE_ALLOCATION_QUOTA_REACHED(0x90c6),
    /** <b><tt>0x90c7</tt></b> TURN server has expired the allocation. */
    ICE_ALLOCATION_MISMATCH(0x90c7),
    /** <b><tt>0x90c8</tt></b> Generic ICE error. */
    ICE_STUN_ERROR(0x90c8),
    /** <b><tt>0x90c9</tt></b> ICE Agent is not in proper state to perform request. */
    ICE_INVALID_STATE(0x90c9),
    /** <b><tt>0x90ca</tt></b> ICE Component type is not recognized. */
    ICE_UNKNOWN_COMPONENT_ID(0x90ca),
    /** <b><tt>0x90cb</tt></b> Rendezvous Server has deactivated the current user. Register with the Rendezvous Server to continue.. */
    RENDEZVOUS_SERVER_DEACTIVATED_USER(0x90cb),
    /** <b><tt>0x90cc</tt></b> Rendezvous Server does not recognize the current user. Register with the Rendezvous Server to continue.. */
    RENDEZVOUS_SERVER_UNKNOWN_USER(0x90cc),
    /** <b><tt>0x90cd</tt></b> Unable to connect to the Rendezvous Server. */
    UNABLE_TO_CONNECT_TO_RENDEZVOUS_SERVER(0x90cd),
    /** <b><tt>0x90ce</tt></b> Not connected to the Rendezvous Server. */
    NOT_CONNECTED_TO_RENDEZVOUS_SERVER(0x90ce),
    /** <b><tt>0x90cf</tt></b> Unable to send message to the Rendezvous Server. */
    UNABLE_TO_SEND_MESSAGE_TO_RENDEZVOUS_SERVER(0x90cf),
    /** <b><tt>0x90d0</tt></b> Invalid Rendezvous Server interface message. */
    INVALID_RENDEZVOUS_SERVER_INTERFACE_MESSAGE(0x90d0),
    /** <b><tt>0x90d1</tt></b> Invalid message response received over the Persistent connection with the Rendezvous Server. */
    INVALID_PERSISTENT_CONNECTION_MESSAGE_RESPONSE(0x90d1),
    /** <b><tt>0x90d2</tt></b> Invalid message response received over the On Demand connection with the Rendezvous Server. */
    INVALID_ON_DEMAND_CONNECTION_MESSAGE_RESPONSE(0x90d2),
    /** <b><tt>0x90d3</tt></b> Invalid HTTP method type used for Rendezvous Server interface message. */
    INVALID_HTTP_METHOD_USED_FOR_RENDEZVOUS_SERVER_INTERFACE_MESSAGE(0x90d3),
    /** <b><tt>0x90d4</tt></b> Received a HTTP 500 status code from the Rendezvous Server. This indicates an internal error in the Server. */
    RENDEZVOUS_SERVER_ERR500_INTERNAL_ERROR(0x90d4),
    /** <b><tt>0x90d5</tt></b> Received a HTTP 503 status code from the Rendezvous Server. This indicates unavailability of the Server error state. */
    RENDEZVOUS_SERVER_ERR503_STATUS_UNAVAILABLE(0x90d5),
    /** <b><tt>0x90d6</tt></b> Received a HTTP 401 status code from the Rendezvous Server. This indicates that the client is unauthorized to send a request to the Server. The Client login procedure must be initiated.. */
    RENDEZVOUS_SERVER_ERR401_UNAUTHORIZED_REQUEST(0x90d6),
    /** <b><tt>0x90d7</tt></b> Received a HTTP status code indicating unrecoverable error from the Rendezvous Server. The connection with the Server should be re-established.. */
    RENDEZVOUS_SERVER_UNRECOVERABLE_ERROR(0x90d7),
    /** <b><tt>0x90d8</tt></b> Rendezvous Server root ceritificate uninitialized.. */
    RENDEZVOUS_SERVER_ROOT_CERTIFICATE_UNINITIALIZED(0x90d8),
    /** <b><tt>0x90d9</tt></b> No such annotation for a GET or SET operation . */
    BUS_NO_SUCH_ANNOTATION(0x90d9),
    /** <b><tt>0x90da</tt></b> Attempt to add an annotation to an interface or property that already exists. */
    BUS_ANNOTATION_ALREADY_EXISTS(0x90da),
    /** <b><tt>0x90db</tt></b> Socket close in progress. */
    SOCK_CLOSING(0x90db),
    /** <b><tt>0x90dc</tt></b> A referenced device cannot be located. */
    NO_SUCH_DEVICE(0x90dc),
    /** <b><tt>0x90dd</tt></b> An error occurred in a Wi-Fi Direct helper method call. */
    P2P(0x90dd),
    /** <b><tt>0x90de</tt></b> A timeout occurred in a Wi-Fi Direct helper method call. */
    P2P_TIMEOUT(0x90de),
    /** <b><tt>0x90df</tt></b> A required Wi-Fi Direct network connection does not exist. */
    P2P_NOT_CONNECTED(0x90df),
    /** <b><tt>0x90e0</tt></b> Exactly one mask bit was not set in the provided TransportMask. */
    BAD_TRANSPORT_MASK(0x90e0),
    /** <b><tt>0x90e1</tt></b> Fail to establish P2P proximity connection. */
    PROXIMITY_CONNECTION_ESTABLISH_FAIL(0x90e1),
    /** <b><tt>0x90e2</tt></b> Cannot find proximity P2P peers. */
    PROXIMITY_NO_PEERS_FOUND(0x90e2),
    /** <b><tt>0x90e3</tt></b> Operation not permitted on unregistered bus object. */
    BUS_OBJECT_NOT_REGISTERED(0x90e3),
    /** <b><tt>0x90e4</tt></b> Wi-Fi Direct is disabled on the device. */
    P2P_DISABLED(0x90e4),
    /** <b><tt>0x90e5</tt></b> Wi-Fi Direct resources are in busy state. */
    P2P_BUSY(0x90e5),
    /** <b><tt>0x90e6</tt></b> The daemon version is too old to be used by this client. */
    BUS_INCOMPATIBLE_DAEMON(0x90e6),
    /** <b><tt>0x90e7</tt></b> Attempt to execute a Wi-Fi Direct GO-related operation while STA. */
    P2P_NO_GO(0x90e7),
    /** <b><tt>0x90e8</tt></b> Attempt to execute a Wi-Fi Direct STA-related operation while GO. */
    P2P_NO_STA(0x90e8),
    /** <b><tt>0x90e9</tt></b> Attempt to execute a forbidden Wi-Fi Direct operation. */
    P2P_FORBIDDEN(0x90e9),
    /** <b><tt>0x90ea</tt></b> OnAppSuspend reply: Failed. */
    ALLJOYN_ONAPPSUSPEND_REPLY_FAILED(0x90ea),
    /** <b><tt>0x90eb</tt></b> OnAppSuspend reply: Unsupported operation. */
    ALLJOYN_ONAPPSUSPEND_REPLY_UNSUPPORTED(0x90eb),
    /** <b><tt>0x90ec</tt></b> OnAppResume reply: Failed. */
    ALLJOYN_ONAPPRESUME_REPLY_FAILED(0x90ec),
    /** <b><tt>0x90ed</tt></b> OnAppResume reply: Unsupported operation. */
    ALLJOYN_ONAPPRESUME_REPLY_UNSUPPORTED(0x90ed),
    /** <b><tt>0x90ee</tt></b> Message not found. */
    BUS_NO_SUCH_MESSAGE(0x90ee),
    /** <b><tt>0x90ef</tt></b> RemoveSessionMember reply: Specified session Id with this endpoint was not found. */
    ALLJOYN_REMOVESESSIONMEMBER_REPLY_NO_SESSION(0x90ef),
    /** <b><tt>0x90f0</tt></b> RemoveSessionMember reply: Endpoint is not the binder of session. */
    ALLJOYN_REMOVESESSIONMEMBER_NOT_BINDER(0x90f0),
    /** <b><tt>0x90f1</tt></b> RemoveSessionMember reply: Session is not multipoint. */
    ALLJOYN_REMOVESESSIONMEMBER_NOT_MULTIPOINT(0x90f1),
    /** <b><tt>0x90f2</tt></b> RemoveSessionMember reply: Specified session member was not found. */
    ALLJOYN_REMOVESESSIONMEMBER_NOT_FOUND(0x90f2),
    /** <b><tt>0x90f3</tt></b> RemoveSessionMember reply: The remote daemon does not support this feature. */
    ALLJOYN_REMOVESESSIONMEMBER_INCOMPATIBLE_REMOTE_DAEMON(0x90f3),
    /** <b><tt>0x90f4</tt></b> RemoveSessionMember reply: Failed for unspecified reason. */
    ALLJOYN_REMOVESESSIONMEMBER_REPLY_FAILED(0x90f4),
    /** <b><tt>0x90f5</tt></b> The session member was removed by the binder. */
    BUS_REMOVED_BY_BINDER(0x90f5),
    /** <b><tt>0xa000</tt></b> Annotation is incorrect. */
    BAD_ANNOTATION(0xa000),
    /** <b><tt>0xa001</tt></b> The org.alljoyn.bus.address property is null. */
    INVALID_CONNECT_ARGS(0xa001),
    /** <b><tt>0xa002</tt></b> A find name request for the well-known name prefix is already active. */
    ALREADY_FINDING(0xa002),
    /** <b><tt>0xa003</tt></b> An operation was cancelled. */
    CANCELLED(0xa003),
    /** <b><tt>0xa004</tt></b> An AuthListener is already set on this BusAttachment. */
    ALREADY_REGISTERED(0xa004);

    /** Error Code */
    private int errorCode;

    /** Constructor */
    private Status(int errorCode) {
        this.errorCode = errorCode;
    }   

    /** Static constructor */
    private static Status create(int errorCode) {
        for (Status s : Status.values()) {
            if (s.getErrorCode() == errorCode) {
                return s;
            }
        }
        return NONE;
    }

    /** 
     * Gets the numeric error code. 
     *
     * @return the numeric error code
     */
    public int getErrorCode() { return errorCode; }
}
