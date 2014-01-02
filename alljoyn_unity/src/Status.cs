
/******************************************************************************
 * @file
 * This file contains an enumerated list values that QStatus can return
 *
 * Note: This file is generated during the make process.
 *
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
 ******************************************************************************/

using System;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * Enumerated list of values QStatus can return
		 */ 
		public class QStatus
		{
			private QStatus(int x)
			{
				value = x;
			}

			/** 
			 * Static constructor
			 * @param x status to set for QStatus object
			 *
			 * @return a new QStatus object
			 */
			public static implicit operator QStatus(int x)
			{
				return new QStatus(x);
			}

			/** 
			 * Gets the int value of the QStatus object  
			 *
			 * @param x QStatus object to check status
			 * @return the int value of the QStatus object  
			 */
			public static implicit operator int(QStatus x)
			{
				return x.value;
			}

			/** 
			 * Shortcut to determine if a QStatus is an OK status
			 *
			 * @param x QStatus object to check status
			 * @return true if the status == OK
			 */
			public static bool operator true(QStatus x)
			{
				return (x == OK);
			}

			/** 
			 * Shortcut to determine if a QStatus is not an OK status
			 *
			 * @param x QStatus object to check status
			 * @return true if the status != OK
			 */
			public static bool operator false(QStatus x)
			{
				return (x != OK);
			}

			/** 
			 * Compares the status value of two QStatus objects
			 *
			 * @param x QStatus object to compare with
			 * @param y QStatus object to compare against
			 * @return true if the status values are equal
			 */
			public static bool operator ==(QStatus x, QStatus y)
			{
				return x.value == y.value;
			}

			/** 
			 * Compares two QStatus objects
			 *
			 * @param o object to compare against this QStatus
			 * @return true if two QStatus objects are equals
			 */
			public override bool Equals(object o) 
			{
				try
				{
					return (this == (QStatus)o);
				}
				catch
				{
					return false;
				}
			}

			/** 
			 * Gets the numeric error code
			 *
			 * @return the numeric error code
			 */
			public override int GetHashCode()
			{
				return value;
			}

			/** 
			 * Gets a string representing the QStatus value
			 *
			 * @return a string representing the QStatus value
			 */
			public override string ToString()
			{
			
				return Marshal.PtrToStringAnsi(QCC_StatusText(value));
			}

			/** 
			 * Gets the string representation of the QStatus value
			 *
			 * @param x QStatus object to get value from 
			 * @return the string representation of the QStatus value
			 */
			public static implicit operator string(QStatus x)
			{
				return x.value.ToString();
			}

			/** 
			 * Checks if two QStatus objects are not equal
			 *
			 * @param x QStatus object to compare with
			 * @param y QStatus object to compare against
			 * @return true if two QStatus objects are not equal
			 */
			public static bool operator !=(QStatus x, QStatus y)
			{
				return x.value != y.value;
			}

			/** 
			 * checks if the QStatus object does not equal OK
			 * 
			 * @param x QStatus object to compare against
			 * @return true if the QStatus object does not equal OK
			 */
			public static bool operator !(QStatus x)
			{
				return (x != OK);
			}

			internal int value;

			/// Success.
			public static readonly QStatus OK = new QStatus(0x0);
			/// Generic failure.
			public static readonly QStatus FAIL = new QStatus(0x1);
			/// Conversion between UTF bases failed.
			public static readonly QStatus UTF_CONVERSION_FAILED = new QStatus(0x2);
			/// Not enough space in buffer for operation.
			public static readonly QStatus BUFFER_TOO_SMALL = new QStatus(0x3);
			/// Underlying OS has indicated an error.
			public static readonly QStatus OS_ERROR = new QStatus(0x4);
			/// Failed to allocate memory.
			public static readonly QStatus OUT_OF_MEMORY = new QStatus(0x5);
			/// Bind to IP address failed.
			public static readonly QStatus SOCKET_BIND_ERROR = new QStatus(0x6);
			/// Initialization failed.
			public static readonly QStatus INIT_FAILED = new QStatus(0x7);
			/// An I/O attempt on non-blocking resource would block
			public static readonly QStatus WOULDBLOCK = new QStatus(0x8);
			/// Feature not implemented
			public static readonly QStatus NOT_IMPLEMENTED = new QStatus(0x9);
			/// Operation timed out
			public static readonly QStatus TIMEOUT = new QStatus(0xa);
			/// Other end closed the socket
			public static readonly QStatus SOCK_OTHER_END_CLOSED = new QStatus(0xb);
			/// Function call argument 1 is invalid
			public static readonly QStatus BAD_ARG_1 = new QStatus(0xc);
			/// Function call argument 2 is invalid
			public static readonly QStatus BAD_ARG_2 = new QStatus(0xd);
			/// Function call argument 3 is invalid
			public static readonly QStatus BAD_ARG_3 = new QStatus(0xe);
			/// Function call argument 4 is invalid
			public static readonly QStatus BAD_ARG_4 = new QStatus(0xf);
			/// Function call argument 5 is invalid
			public static readonly QStatus BAD_ARG_5 = new QStatus(0x10);
			/// Function call argument 6 is invalid
			public static readonly QStatus BAD_ARG_6 = new QStatus(0x11);
			/// Function call argument 7 is invalid
			public static readonly QStatus BAD_ARG_7 = new QStatus(0x12);
			/// Function call argument 8 is invalid
			public static readonly QStatus BAD_ARG_8 = new QStatus(0x13);
			/// Address is NULL or invalid
			public static readonly QStatus INVALID_ADDRESS = new QStatus(0x14);
			/// Generic invalid data error
			public static readonly QStatus INVALID_DATA = new QStatus(0x15);
			/// Generic read error
			public static readonly QStatus READ_ERROR = new QStatus(0x16);
			/// Generic write error
			public static readonly QStatus WRITE_ERROR = new QStatus(0x17);
			/// Generic open failure
			public static readonly QStatus OPEN_FAILED = new QStatus(0x18);
			/// Generic parse failure
			public static readonly QStatus PARSE_ERROR = new QStatus(0x19);
			/// Generic EOD/EOF error
			public static readonly QStatus END_OF_DATA = new QStatus(0x1A);
			/// Connection was refused because no one is listening
			public static readonly QStatus CONN_REFUSED = new QStatus(0x1B);
			/// Incorrect number of arguments given to function call
			public static readonly QStatus BAD_ARG_COUNT = new QStatus(0x1C);
			/// Generic warning
			public static readonly QStatus WARNING = new QStatus(0x1D);
			/// Error code block for the Common subsystem.
			public static readonly QStatus COMMON_ERRORS = new QStatus(0x1000);
			/// Operation interrupted by ERThread stop signal.
			public static readonly QStatus STOPPING_THREAD = new QStatus(0x1001);
			/// Operation interrupted by ERThread alert signal.
			public static readonly QStatus ALERTED_THREAD = new QStatus(0x1002);
			/// Cannot parse malformed XML
			public static readonly QStatus XML_MALFORMED = new QStatus(0x1003);
			/// Authentication failed
			public static readonly QStatus AUTH_FAIL = new QStatus(0x1004);
			/// Authentication was rejected by user
			public static readonly QStatus AUTH_USER_REJECT = new QStatus(0x1005);
			/// Attempt to reference non-existent timer alarm
			public static readonly QStatus NO_SUCH_ALARM = new QStatus(0x1006);
			/// A timer thread is missing scheduled alarm times
			public static readonly QStatus TIMER_FALLBEHIND = new QStatus(0x1007);
			/// Error code block for SSL subsystem
			public static readonly QStatus SSL_ERRORS = new QStatus(0x1008);
			/// SSL initialization failed.
			public static readonly QStatus SSL_INIT = new QStatus(0x1009);
			/// Failed to connect to remote host using SSL
			public static readonly QStatus SSL_CONNECT = new QStatus(0x100a);
			/// Failed to verify identity of SSL destination
			public static readonly QStatus SSL_VERIFY = new QStatus(0x100b);
			/// Operation not supported on external thread wrapper
			public static readonly QStatus EXTERNAL_THREAD = new QStatus(0x100c);
			/// Non-specific error in the crypto subsystem
			public static readonly QStatus CRYPTO_ERROR = new QStatus(0x100d);
			/// Not enough room for key
			public static readonly QStatus CRYPTO_TRUNCATED = new QStatus(0x100e);
			/// No key to return
			public static readonly QStatus CRYPTO_KEY_UNAVAILABLE = new QStatus(0x100f);
			/// Cannot lookup hostname
			public static readonly QStatus BAD_HOSTNAME = new QStatus(0x1010);
			/// Key cannot be used
			public static readonly QStatus CRYPTO_KEY_UNUSABLE = new QStatus(0x1011);
			/// Key blob is empty
			public static readonly QStatus EMPTY_KEY_BLOB = new QStatus(0x1012);
			/// Key blob is corrupted
			public static readonly QStatus CORRUPT_KEYBLOB = new QStatus(0x1013);
			/// Encoded key is not valid
			public static readonly QStatus INVALID_KEY_ENCODING = new QStatus(0x1014);
			/// Operation not allowed thread is dead
			public static readonly QStatus DEAD_THREAD = new QStatus(0x1015);
			/// Cannot start a thread that is already running
			public static readonly QStatus THREAD_RUNNING = new QStatus(0x1016);
			/// Cannot start a thread that is already stopping
			public static readonly QStatus THREAD_STOPPING = new QStatus(0x1017);
			/// Encoded string did not have the expected format or contents
			public static readonly QStatus BAD_STRING_ENCODING = new QStatus(0x1018);
			/// Crypto algorithm parameters do not provide sufficient security
			public static readonly QStatus CRYPTO_INSUFFICIENT_SECURITY = new QStatus(0x1019);
			/// Crypto algorithm parameter value is illegal
			public static readonly QStatus CRYPTO_ILLEGAL_PARAMETERS = new QStatus(0x101a);
			/// Cryptographic hash function must be initialized
			public static readonly QStatus CRYPTO_HASH_UNINITIALIZED = new QStatus(0x101b);
			/// Thread cannot be blocked by a WAIT or SLEEP call
			public static readonly QStatus THREAD_NO_WAIT = new QStatus(0x101c);
			/// Cannot add an alarm to a timer that is exiting
			public static readonly QStatus TIMER_EXITING = new QStatus(0x101d);
			/// String is not a hex encoded GUID string
			public static readonly QStatus INVALID_GUID = new QStatus(0x101e);
			/// A thread pool has reached its specified concurrency
			public static readonly QStatus THREADPOOL_EXHAUSTED = new QStatus(0x101f);
			/// Cannot execute a closure on a stopping thread pool
			public static readonly QStatus THREADPOOL_STOPPING = new QStatus(0x1020);
			/// Attempt to reference non-existent stream entry
			public static readonly QStatus INVALID_STREAM = new QStatus(0x1021);
			/// Attempt to reference non-existent stream entry
			public static readonly QStatus TIMER_FULL = new QStatus(0x1022);
			/// Cannot execute a read or write command on an IODispatch thread because it is stopping.
			public static readonly QStatus IODISPATCH_STOPPING = new QStatus(0x1023);
			/// Length of SLAP packet is invalid.
			public static readonly QStatus SLAP_INVALID_PACKET_LEN = new QStatus(0x1024);
			/// SLAP packet header checksum error.
			public static readonly QStatus SLAP_HDR_CHECKSUM_ERROR = new QStatus(0x1025);
			/// Invalid SLAP packet type.
			public static readonly QStatus SLAP_INVALID_PACKET_TYPE = new QStatus(0x1026);
			/// Calculated length does not match the received length.
			public static readonly QStatus SLAP_LEN_MISMATCH = new QStatus(0x1027);
			/// Packet type does not match reliability bit.
			public static readonly QStatus SLAP_PACKET_TYPE_MISMATCH = new QStatus(0x1028);
			/// SLAP packet CRC error.
			public static readonly QStatus SLAP_CRC_ERROR = new QStatus(0x1029);
			/// Generic SLAP error.
			public static readonly QStatus SLAP_ERROR = new QStatus(0x102A);
			/// Other end closed the SLAP connection
			public static readonly QStatus SLAP_OTHER_END_CLOSED = new QStatus(0x102B);
			/// No error code to report
			public static readonly QStatus NONE = new QStatus(0xffff);
			/// Error code block for ALLJOYN wire protocol
			public static readonly QStatus BUS_ERRORS = new QStatus(0x9000);
			/// Error attempting to read
			public static readonly QStatus BUS_READ_ERROR = new QStatus(0x9001);
			/// Error attempting to write
			public static readonly QStatus BUS_WRITE_ERROR = new QStatus(0x9002);
			/// Read an invalid value type
			public static readonly QStatus BUS_BAD_VALUE_TYPE = new QStatus(0x9003);
			/// Read an invalid header field
			public static readonly QStatus BUS_BAD_HEADER_FIELD = new QStatus(0x9004);
			/// Signature was badly formed
			public static readonly QStatus BUS_BAD_SIGNATURE = new QStatus(0x9005);
			/// Object path contained an illegal character
			public static readonly QStatus BUS_BAD_OBJ_PATH = new QStatus(0x9006);
			/// A member name contained an illegal character
			public static readonly QStatus BUS_BAD_MEMBER_NAME = new QStatus(0x9007);
			/// An interface name contained an illegal character
			public static readonly QStatus BUS_BAD_INTERFACE_NAME = new QStatus(0x9008);
			/// An error name contained an illegal character
			public static readonly QStatus BUS_BAD_ERROR_NAME = new QStatus(0x9009);
			/// A bus name contained an illegal character
			public static readonly QStatus BUS_BAD_BUS_NAME = new QStatus(0x900a);
			/// A name exceeded the permitted length
			public static readonly QStatus BUS_NAME_TOO_LONG = new QStatus(0x900b);
			/// Length of an array was not a multiple of the array element size
			public static readonly QStatus BUS_BAD_LENGTH = new QStatus(0x900c);
			/// Parsed value in a message was invalid (for example: boolean > 1) 
			public static readonly QStatus BUS_BAD_VALUE = new QStatus(0x900d);
			/// Unknown header flags
			public static readonly QStatus BUS_BAD_HDR_FLAGS = new QStatus(0x900e);
			/// Body length was to long or too short
			public static readonly QStatus BUS_BAD_BODY_LEN = new QStatus(0x900f);
			/// Header length was to long or too short
			public static readonly QStatus BUS_BAD_HEADER_LEN = new QStatus(0x9010);
			/// Serial number in a method response was unknown
			public static readonly QStatus BUS_UNKNOWN_SERIAL = new QStatus(0x9011);
			/// Path in a method call or signal was unknown
			public static readonly QStatus BUS_UNKNOWN_PATH = new QStatus(0x9012);
			/// Interface in a method call or signal was unknown
			public static readonly QStatus BUS_UNKNOWN_INTERFACE = new QStatus(0x9013);
			/// Failed to establish a connection
			public static readonly QStatus BUS_ESTABLISH_FAILED = new QStatus(0x9014);
			/// Signature in message was not what was expected
			public static readonly QStatus BUS_UNEXPECTED_SIGNATURE = new QStatus(0x9015);
			/// Interface header field is missing
			public static readonly QStatus BUS_INTERFACE_MISSING = new QStatus(0x9016);
			/// Object path header field is missing
			public static readonly QStatus BUS_PATH_MISSING = new QStatus(0x9017);
			/// Member header field is missing
			public static readonly QStatus BUS_MEMBER_MISSING = new QStatus(0x9018);
			/// Reply-Serial header field is missing
			public static readonly QStatus BUS_REPLY_SERIAL_MISSING = new QStatus(0x9019);
			/// Error Name header field is missing
			public static readonly QStatus BUS_ERROR_NAME_MISSING = new QStatus(0x901a);
			/// Interface does not have the requested member
			public static readonly QStatus BUS_INTERFACE_NO_SUCH_MEMBER = new QStatus(0x901b);
			/// Object does not exist
			public static readonly QStatus BUS_NO_SUCH_OBJECT = new QStatus(0x901c);
			/// Object does not have the requested member (on any interface)
			public static readonly QStatus BUS_OBJECT_NO_SUCH_MEMBER = new QStatus(0x901d);
			/// Object does not have the requested interface
			public static readonly QStatus BUS_OBJECT_NO_SUCH_INTERFACE = new QStatus(0x901e);
			/// Requested interface does not exist
			public static readonly QStatus BUS_NO_SUCH_INTERFACE = new QStatus(0x901f);
			/// Member exists but does not have the requested signature
			public static readonly QStatus BUS_MEMBER_NO_SUCH_SIGNATURE = new QStatus(0x9020);
			/// A string or signature was not NUL terminated
			public static readonly QStatus BUS_NOT_NUL_TERMINATED = new QStatus(0x9021);
			/// No such property for a GET or SET operation 
			public static readonly QStatus BUS_NO_SUCH_PROPERTY = new QStatus(0x9022);
			/// Attempt to set a property value with the wrong signature
			public static readonly QStatus BUS_SET_WRONG_SIGNATURE = new QStatus(0x9023);
			/// Attempt to get a property whose value has not been set
			public static readonly QStatus BUS_PROPERTY_VALUE_NOT_SET = new QStatus(0x9024);
			/// Attempt to set or get a property failed due to access rights
			public static readonly QStatus BUS_PROPERTY_ACCESS_DENIED = new QStatus(0x9025);
			/// No physical message transports were specified
			public static readonly QStatus BUS_NO_TRANSPORTS = new QStatus(0x9026);
			/// Missing or badly formatted transports args specified
			public static readonly QStatus BUS_BAD_TRANSPORT_ARGS = new QStatus(0x9027);
			/// Message cannot be routed to destination
			public static readonly QStatus BUS_NO_ROUTE = new QStatus(0x9028);
			/// An endpoint with given name cannot be found
			public static readonly QStatus BUS_NO_ENDPOINT = new QStatus(0x9029);
			/// Bad parameter in send message call
			public static readonly QStatus BUS_BAD_SEND_PARAMETER = new QStatus(0x902a);
			/// Serial number in method call reply message did not match any method calls
			public static readonly QStatus BUS_UNMATCHED_REPLY_SERIAL = new QStatus(0x902b);
			/// Sender identifier is invalid
			public static readonly QStatus BUS_BAD_SENDER_ID = new QStatus(0x902c);
			/// Attempt to send on a transport that has not been started
			public static readonly QStatus BUS_TRANSPORT_NOT_STARTED = new QStatus(0x902d);
			/// Attempt to deliver an empty message
			public static readonly QStatus BUS_EMPTY_MESSAGE = new QStatus(0x902e);
			/// A bus name operation was not permitted because sender does not own name
			public static readonly QStatus BUS_NOT_OWNER = new QStatus(0x902f);
			/// Application rejected a request to set a property
			public static readonly QStatus BUS_SET_PROPERTY_REJECTED = new QStatus(0x9030);
			/// Connection failed
			public static readonly QStatus BUS_CONNECT_FAILED = new QStatus(0x9031);
			/// Response from a method call was an ERROR message
			public static readonly QStatus BUS_REPLY_IS_ERROR_MESSAGE = new QStatus(0x9032);
			/// Not in an authentication conversation
			public static readonly QStatus BUS_NOT_AUTHENTICATING = new QStatus(0x9033);
			/// A listener is required to implement the requested function
			public static readonly QStatus BUS_NO_LISTENER = new QStatus(0x9034);
			/// The Bluetooth transport reported an error
			public static readonly QStatus BUS_BT_TRANSPORT_ERROR = new QStatus(0x9035);
			/// The operation attempted is not allowed
			public static readonly QStatus BUS_NOT_ALLOWED = new QStatus(0x9036);
			/// Write failed because write queue is full
			public static readonly QStatus BUS_WRITE_QUEUE_FULL = new QStatus(0x9037);
			/// Operation not permitted on endpoint in process of closing
			public static readonly QStatus BUS_ENDPOINT_CLOSING = new QStatus(0x9038);
			/// Received two conflicting definitions for the same interface
			public static readonly QStatus BUS_INTERFACE_MISMATCH = new QStatus(0x9039);
			/// Attempt to add a member to an interface that already exists
			public static readonly QStatus BUS_MEMBER_ALREADY_EXISTS = new QStatus(0x903a);
			/// Attempt to add a property to an interface that already exists
			public static readonly QStatus BUS_PROPERTY_ALREADY_EXISTS = new QStatus(0x903b);
			/// Attempt to add an interface to an object that already exists
			public static readonly QStatus BUS_IFACE_ALREADY_EXISTS = new QStatus(0x903c);
			/// Received an error response to a method call
			public static readonly QStatus BUS_ERROR_RESPONSE = new QStatus(0x903d);
			/// XML data is improperly formatted
			public static readonly QStatus BUS_BAD_XML = new QStatus(0x903e);
			/// The path of a child object is incorrect given it's parent's path
			public static readonly QStatus BUS_BAD_CHILD_PATH = new QStatus(0x903f);
			/// Attempt to add a RemoteObject child that already exists
			public static readonly QStatus BUS_OBJ_ALREADY_EXISTS = new QStatus(0x9040);
			/// Object with given path does not exist
			public static readonly QStatus BUS_OBJ_NOT_FOUND = new QStatus(0x9041);
			/// Expansion information for a compressed message is not available
			public static readonly QStatus BUS_CANNOT_EXPAND_MESSAGE = new QStatus(0x9042);
			/// Attempt to expand a message that is not compressed
			public static readonly QStatus BUS_NOT_COMPRESSED = new QStatus(0x9043);
			/// Attempt to connect to a bus which is already connected
			public static readonly QStatus BUS_ALREADY_CONNECTED = new QStatus(0x9044);
			/// Attempt to use a bus attachment that is not connected to a bus daemon
			public static readonly QStatus BUS_NOT_CONNECTED = new QStatus(0x9045);
			/// Attempt to listen on a bus address which is already being listened on
			public static readonly QStatus BUS_ALREADY_LISTENING = new QStatus(0x9046);
			/// The request key is not available
			public static readonly QStatus BUS_KEY_UNAVAILABLE = new QStatus(0x9047);
			/// Insufficient memory to copy data
			public static readonly QStatus BUS_TRUNCATED = new QStatus(0x9048);
			/// Accessing the key store before it is loaded
			public static readonly QStatus BUS_KEY_STORE_NOT_LOADED = new QStatus(0x9049);
			/// There is no authentication mechanism
			public static readonly QStatus BUS_NO_AUTHENTICATION_MECHANISM = new QStatus(0x904a);
			/// Bus has already been started
			public static readonly QStatus BUS_BUS_ALREADY_STARTED = new QStatus(0x904b);
			/// Bus has not yet been started
			public static readonly QStatus BUS_BUS_NOT_STARTED = new QStatus(0x904c);
			/// The operation requested cannot be performed using this key blob
			public static readonly QStatus BUS_KEYBLOB_OP_INVALID = new QStatus(0x904d);
			/// Invalid header checksum in an encrypted message
			public static readonly QStatus BUS_INVALID_HEADER_CHECKSUM = new QStatus(0x904e);
			/// Security policy requires the message to be encrypted
			public static readonly QStatus BUS_MESSAGE_NOT_ENCRYPTED = new QStatus(0x904f);
			/// Serial number in message header is invalid
			public static readonly QStatus BUS_INVALID_HEADER_SERIAL = new QStatus(0x9050);
			/// Message time-to-live has expired
			public static readonly QStatus BUS_TIME_TO_LIVE_EXPIRED = new QStatus(0x9051);
			/// Something is wrong with a header expansion
			public static readonly QStatus BUS_HDR_EXPANSION_INVALID = new QStatus(0x9052);
			/// Compressed headers require a compression token
			public static readonly QStatus BUS_MISSING_COMPRESSION_TOKEN = new QStatus(0x9053);
			/// There is no GUID for this peer
			public static readonly QStatus BUS_NO_PEER_GUID = new QStatus(0x9054);
			/// Message decryption failed
			public static readonly QStatus BUS_MESSAGE_DECRYPTION_FAILED = new QStatus(0x9055);
			/// A fatal security failure
			public static readonly QStatus BUS_SECURITY_FATAL = new QStatus(0x9056);
			/// An encryption key has expired
			public static readonly QStatus BUS_KEY_EXPIRED = new QStatus(0x9057);
			/// Key store is corrupt
			public static readonly QStatus BUS_CORRUPT_KEYSTORE = new QStatus(0x9058);
			/// A reply only allowed in response to a method call
			public static readonly QStatus BUS_NO_CALL_FOR_REPLY = new QStatus(0x9059);
			/// Signature must be a single complete type
			public static readonly QStatus BUS_NOT_A_COMPLETE_TYPE = new QStatus(0x905a);
			/// Message does not meet policy restrictions
			public static readonly QStatus BUS_POLICY_VIOLATION = new QStatus(0x905b);
			/// Service name is unknown
			public static readonly QStatus BUS_NO_SUCH_SERVICE = new QStatus(0x905c);
			/// Transport cannot be used due to underlying mechanism disabled by OS
			public static readonly QStatus BUS_TRANSPORT_NOT_AVAILABLE = new QStatus(0x905d);
			/// Authentication mechanism is not valid
			public static readonly QStatus BUS_INVALID_AUTH_MECHANISM = new QStatus(0x905e);
			/// Key store has wrong version number
			public static readonly QStatus BUS_KEYSTORE_VERSION_MISMATCH = new QStatus(0x905f);
			/// A synchronous method call from within handler is not permitted.
			public static readonly QStatus BUS_BLOCKING_CALL_NOT_ALLOWED = new QStatus(0x9060);
			/// MsgArg(s) do not match signature.
			public static readonly QStatus BUS_SIGNATURE_MISMATCH = new QStatus(0x9061);
			/// The bus is stopping.
			public static readonly QStatus BUS_STOPPING = new QStatus(0x9062);
			/// The method call was aborted.
			public static readonly QStatus BUS_METHOD_CALL_ABORTED = new QStatus(0x9063);
			/// An interface cannot be added to an object that is already registered.
			public static readonly QStatus BUS_CANNOT_ADD_INTERFACE = new QStatus(0x9064);
			/// A method handler cannot be added to an object that is already registered.
			public static readonly QStatus BUS_CANNOT_ADD_HANDLER = new QStatus(0x9065);
			/// Key store has not been loaded
			public static readonly QStatus BUS_KEYSTORE_NOT_LOADED = new QStatus(0x9066);
			/// Handle is not in the handle table
			public static readonly QStatus BUS_NO_SUCH_HANDLE = new QStatus(0x906b);
			/// Passing of handles is not enabled for this connection
			public static readonly QStatus BUS_HANDLES_NOT_ENABLED = new QStatus(0x906c);
			/// Message had more handles than expected
			public static readonly QStatus BUS_HANDLES_MISMATCH = new QStatus(0x906d);
			/// Maximum Bluetooth connections already in use
			public static readonly QStatus BT_MAX_CONNECTIONS_USED = new QStatus(0x906e);
			/// Session id is not valid
			public static readonly QStatus BUS_NO_SESSION = new QStatus(0x906f);
			/// Dictionary element was not found
			public static readonly QStatus BUS_ELEMENT_NOT_FOUND = new QStatus(0x9070);
			/// MsgArg was not an array of dictionary elements
			public static readonly QStatus BUS_NOT_A_DICTIONARY = new QStatus(0x9071);
			/// Wait failed
			public static readonly QStatus BUS_WAIT_FAILED = new QStatus(0x9072);
			/// Session options are bad or incompatible
			public static readonly QStatus BUS_BAD_SESSION_OPTS = new QStatus(0x9074);
			/// Incoming connection rejected
			public static readonly QStatus BUS_CONNECTION_REJECTED = new QStatus(0x9075);
			/// RequestName reply: Name was successfully obtained
			public static readonly QStatus DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER = new QStatus(0x9076);
			/// RequestName reply: Name is already owned, request for name has been queued
			public static readonly QStatus DBUS_REQUEST_NAME_REPLY_IN_QUEUE = new QStatus(0x9077);
			/// RequestName reply: Name is already owned and DO_NOT_QUEUE was specified in request
			public static readonly QStatus DBUS_REQUEST_NAME_REPLY_EXISTS = new QStatus(0x9078);
			/// RequestName reply: Name is already owned by this endpoint
			public static readonly QStatus DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER = new QStatus(0x9079);
			/// ReleaseName reply: Name was released
			public static readonly QStatus DBUS_RELEASE_NAME_REPLY_RELEASED = new QStatus(0x907a);
			///  ReleaseName reply: Name does not exist
			public static readonly QStatus DBUS_RELEASE_NAME_REPLY_NON_EXISTENT = new QStatus(0x907b);
			/// ReleaseName reply: Request to release name that is not owned by this endpoint
			public static readonly QStatus DBUS_RELEASE_NAME_REPLY_NOT_OWNER = new QStatus(0x907c);
			/// StartServiceByName reply: Service is already running
			public static readonly QStatus DBUS_START_REPLY_ALREADY_RUNNING = new QStatus(0x907e);
			/// BindSessionPort reply: SessionPort already exists
			public static readonly QStatus ALLJOYN_BINDSESSIONPORT_REPLY_ALREADY_EXISTS = new QStatus(0x9080);
			/// BindSessionPort reply: Failed
			public static readonly QStatus ALLJOYN_BINDSESSIONPORT_REPLY_FAILED = new QStatus(0x9081);
			/// JoinSession reply: Session with given name does not exist
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_NO_SESSION = new QStatus(0x9083);
			/// JoinSession reply: Failed to find suitable transport
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_UNREACHABLE = new QStatus(0x9084);
			/// JoinSession reply: Connect to advertised address
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED = new QStatus(0x9085);
			/// JoinSession reply: The session creator rejected the join req
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_REJECTED = new QStatus(0x9086);
			/// JoinSession reply: Failed due to session option incompatibilities
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS = new QStatus(0x9087);
			/// JoinSession reply: Failed for unknown reason
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_FAILED = new QStatus(0x9088);
			/// LeaveSession reply: Session with given name does not exist
			public static readonly QStatus ALLJOYN_LEAVESESSION_REPLY_NO_SESSION = new QStatus(0x908a);
			/// LeaveSession reply: Failed for unspecified reason
			public static readonly QStatus ALLJOYN_LEAVESESSION_REPLY_FAILED = new QStatus(0x908b);
			/// AdvertiseName reply: This endpoint is already advertising this name
			public static readonly QStatus ALLJOYN_ADVERTISENAME_REPLY_ALREADY_ADVERTISING = new QStatus(0x908d);
			/// AdvertiseName reply: Advertise failed
			public static readonly QStatus ALLJOYN_ADVERTISENAME_REPLY_FAILED = new QStatus(0x908e);
			/// CancelAdvertiseName reply: Advertise failed
			public static readonly QStatus ALLJOYN_CANCELADVERTISENAME_REPLY_FAILED = new QStatus(0x9090);
			/// FindAdvertisedName reply: This endpoint is already discovering this name
			public static readonly QStatus ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING = new QStatus(0x9092);
			/// FindAdvertisedName reply: Failed
			public static readonly QStatus ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED = new QStatus(0x9093);
			/// CancelFindAdvertisedName reply: Failed
			public static readonly QStatus ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED = new QStatus(0x9095);
			/// An unexpected disposition was returned and has been treated as an error
			public static readonly QStatus BUS_UNEXPECTED_DISPOSITION = new QStatus(0x9096);
			/// An InterfaceDescription cannot be modified once activated
			public static readonly QStatus BUS_INTERFACE_ACTIVATED = new QStatus(0x9097);
			/// UnbindSessionPort reply: SessionPort does not exist
			public static readonly QStatus ALLJOYN_UNBINDSESSIONPORT_REPLY_BAD_PORT = new QStatus(0x9098);
			/// UnbindSessionPort reply: Failed
			public static readonly QStatus ALLJOYN_UNBINDSESSIONPORT_REPLY_FAILED = new QStatus(0x9099);
			/// BindSessionPort reply: SessionOpts are invalid
			public static readonly QStatus ALLJOYN_BINDSESSIONPORT_REPLY_INVALID_OPTS = new QStatus(0x909a);
			/// JoinSession reply: Caller has already joined the session
			public static readonly QStatus ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED = new QStatus(0x909b);
			/// Received BusHello from self
			public static readonly QStatus BUS_SELF_CONNECT = new QStatus(0x909c);
			/// Security is not enabled for this bus attachment
			public static readonly QStatus BUS_SECURITY_NOT_ENABLED = new QStatus(0x909d);
			/// A listener has already been set
			public static readonly QStatus BUS_LISTENER_ALREADY_SET = new QStatus(0x909e);
			/// Incompatible peer authentication version numbers
			public static readonly QStatus BUS_PEER_AUTH_VERSION_MISMATCH = new QStatus(0x909f);
			/// Local daemon does not support SetLinkTimeout
			public static readonly QStatus ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED = new QStatus(0x90a0);
			/// SetLinkTimeout not supported by destination
			public static readonly QStatus ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT = new QStatus(0x90a1);
			/// SetLinkTimeout failed
			public static readonly QStatus ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED = new QStatus(0x90a2);
			/// No permission to use Wifi/Bluetooth
			public static readonly QStatus ALLJOYN_ACCESS_PERMISSION_WARNING = new QStatus(0x90a3);
			/// No permission to access peer service
			public static readonly QStatus ALLJOYN_ACCESS_PERMISSION_ERROR = new QStatus(0x90a4);
			/// Cannot send a signal to a destination that is not authenticated
			public static readonly QStatus BUS_DESTINATION_NOT_AUTHENTICATED = new QStatus(0x90a5);
			/// Endpoint was redirected to another address
			public static readonly QStatus BUS_ENDPOINT_REDIRECTED = new QStatus(0x90a6);
			/// Authentication of remote peer is pending
			public static readonly QStatus BUS_AUTHENTICATION_PENDING = new QStatus(0x90a7);
			/// Operation was not authorized
			public static readonly QStatus BUS_NOT_AUTHORIZED = new QStatus(0x90a8);
			/// Received packet for unknown channel
			public static readonly QStatus PACKET_BUS_NO_SUCH_CHANNEL = new QStatus(0x90a9);
			/// Received packet with incorrect header information
			public static readonly QStatus PACKET_BAD_FORMAT = new QStatus(0x90aa);
			/// Timed out waiting for connect response
			public static readonly QStatus PACKET_CONNECT_TIMEOUT = new QStatus(0x90ab);
			/// Failed to create new comm channel
			public static readonly QStatus PACKET_CHANNEL_FAIL = new QStatus(0x90ac);
			/// Message too large for use with packet based transport
			public static readonly QStatus PACKET_TOO_LARGE = new QStatus(0x90ad);
			/// Invalid PacketEngine control packet received
			public static readonly QStatus PACKET_BAD_PARAMETER = new QStatus(0x90ae);
			/// Packet has invalid CRC
			public static readonly QStatus PACKET_BAD_CRC = new QStatus(0x90af);
			/// STUN attribute size does not match size parsed
			public static readonly QStatus STUN_ATTR_SIZE_MISMATCH = new QStatus(0x90b0);
			/// STUN server has denied request, issued Challenge
			public static readonly QStatus STUN_AUTH_CHALLENGE = new QStatus(0x90b1);
			/// Underlying socket not open for operation
			public static readonly QStatus STUN_SOCKET_NOT_OPEN = new QStatus(0x90b2);
			/// Underlying socket alread open
			public static readonly QStatus STUN_SOCKET_OPEN = new QStatus(0x90b3);
			/// Failed to send STUN message
			public static readonly QStatus STUN_FAILED_TO_SEND_MSG = new QStatus(0x90b4);
			/// Application specified invalid TCP framing
			public static readonly QStatus STUN_FRAMING_ERROR = new QStatus(0x90b5);
			/// Invalid STUN error code
			public static readonly QStatus STUN_INVALID_ERROR_CODE = new QStatus(0x90b6);
			/// Fingerprint CRC does not match
			public static readonly QStatus STUN_INVALID_FINGERPRINT = new QStatus(0x90b7);
			/// Invalid address family value in STUN 'address' attribute
			public static readonly QStatus STUN_INVALID_ADDR_FAMILY = new QStatus(0x90b8);
			/// SHA1-HMAC message integrity value does not match. When passed to upper layer, indicates unauthorized response, message must be ignored
			public static readonly QStatus STUN_INVALID_MESSAGE_INTEGRITY = new QStatus(0x90b9);
			/// Invalid STUN message type
			public static readonly QStatus STUN_INVALID_MSG_TYPE = new QStatus(0x90ba);
			/// Invalid STUN message attribute type
			public static readonly QStatus STUN_INVALID_ATTR_TYPE = new QStatus(0x90bb);
			/// STUN response message included a USERNAME attribute
			public static readonly QStatus STUN_RESPONSE_WITH_USERNAME = new QStatus(0x90bc);
			/// Received bad STUN request, upper layer must send error code 400
			public static readonly QStatus STUN_ERR400_BAD_REQUEST = new QStatus(0x90bd);
			/// Received bad STUN indication, upper layer must ignore message
			public static readonly QStatus STUN_BAD_INDICATION = new QStatus(0x90be);
			/// Received STUN request with invalid USERNAME or invalid MESSAGE-INTEGRITY, upper layer must send error code 401
			public static readonly QStatus STUN_ERR401_UNAUTHORIZED_REQUEST = new QStatus(0x90bf);
			/// Too many attributes in STUN message or unknown attributes list
			public static readonly QStatus STUN_TOO_MANY_ATTRIBUTES = new QStatus(0x90c0);
			/// STUN message attribute must only be added once
			public static readonly QStatus STUN_DUPLICATE_ATTRIBUTE = new QStatus(0x90c1);
			/// Receive STUN indication with invalid USERNAME or invalid MESSAGE-INTEGRITY, upper layer must ignore message
			public static readonly QStatus STUN_UNAUTHORIZED_INDICATION = new QStatus(0x90c2);
			/// Unable to allocate heap from ICE
			public static readonly QStatus ICE_ALLOCATING_MEMORY = new QStatus(0x90c3);
			/// ICE Checks have not completed
			public static readonly QStatus ICE_CHECKS_INCOMPLETE = new QStatus(0x90c4);
			/// TURN server rejected ALLOCATE request
			public static readonly QStatus ICE_ALLOCATE_REJECTED_NO_RESOURCES = new QStatus(0x90c5);
			/// TURN server rejected with 486
			public static readonly QStatus ICE_ALLOCATION_QUOTA_REACHED = new QStatus(0x90c6);
			/// TURN server has expired the allocation
			public static readonly QStatus ICE_ALLOCATION_MISMATCH = new QStatus(0x90c7);
			/// Generic ICE error
			public static readonly QStatus ICE_STUN_ERROR = new QStatus(0x90c8);
			/// ICE Agent is not in proper state to perform request
			public static readonly QStatus ICE_INVALID_STATE = new QStatus(0x90c9);
			/// ICE Component type is not recognized
			public static readonly QStatus ICE_UNKNOWN_COMPONENT_ID = new QStatus(0x90ca);
			/// Rendezvous Server has deactivated the current user. Register with the Rendezvous Server to continue.
			public static readonly QStatus RENDEZVOUS_SERVER_DEACTIVATED_USER = new QStatus(0x90cb);
			/// Rendezvous Server does not recognize the current user. Register with the Rendezvous Server to continue.
			public static readonly QStatus RENDEZVOUS_SERVER_UNKNOWN_USER = new QStatus(0x90cc);
			/// Unable to connect to the Rendezvous Server
			public static readonly QStatus UNABLE_TO_CONNECT_TO_RENDEZVOUS_SERVER = new QStatus(0x90cd);
			/// Not connected to the Rendezvous Server
			public static readonly QStatus NOT_CONNECTED_TO_RENDEZVOUS_SERVER = new QStatus(0x90ce);
			/// Unable to send message to the Rendezvous Server
			public static readonly QStatus UNABLE_TO_SEND_MESSAGE_TO_RENDEZVOUS_SERVER = new QStatus(0x90cf);
			/// Invalid Rendezvous Server interface message
			public static readonly QStatus INVALID_RENDEZVOUS_SERVER_INTERFACE_MESSAGE = new QStatus(0x90d0);
			/// Invalid message response received over the Persistent connection with the Rendezvous Server
			public static readonly QStatus INVALID_PERSISTENT_CONNECTION_MESSAGE_RESPONSE = new QStatus(0x90d1);
			/// Invalid message response received over the On Demand connection with the Rendezvous Server
			public static readonly QStatus INVALID_ON_DEMAND_CONNECTION_MESSAGE_RESPONSE = new QStatus(0x90d2);
			/// Invalid HTTP method type used for Rendezvous Server interface message
			public static readonly QStatus INVALID_HTTP_METHOD_USED_FOR_RENDEZVOUS_SERVER_INTERFACE_MESSAGE = new QStatus(0x90d3);
			/// Received a HTTP 500 status code from the Rendezvous Server. This indicates an internal error in the Server
			public static readonly QStatus RENDEZVOUS_SERVER_ERR500_INTERNAL_ERROR = new QStatus(0x90d4);
			/// Received a HTTP 503 status code from the Rendezvous Server. This indicates unavailability of the Server error state
			public static readonly QStatus RENDEZVOUS_SERVER_ERR503_STATUS_UNAVAILABLE = new QStatus(0x90d5);
			/// Received a HTTP 401 status code from the Rendezvous Server. This indicates that the client is unauthorized to send a request to the Server. The Client login procedure must be initiated.
			public static readonly QStatus RENDEZVOUS_SERVER_ERR401_UNAUTHORIZED_REQUEST = new QStatus(0x90d6);
			/// Received a HTTP status code indicating unrecoverable error from the Rendezvous Server. The connection with the Server should be re-established.
			public static readonly QStatus RENDEZVOUS_SERVER_UNRECOVERABLE_ERROR = new QStatus(0x90d7);
			/// Rendezvous Server root ceritificate uninitialized.
			public static readonly QStatus RENDEZVOUS_SERVER_ROOT_CERTIFICATE_UNINITIALIZED = new QStatus(0x90d8);
			/// No such annotation for a GET or SET operation 
			public static readonly QStatus BUS_NO_SUCH_ANNOTATION = new QStatus(0x90d9);
			/// Attempt to add an annotation to an interface or property that already exists
			public static readonly QStatus BUS_ANNOTATION_ALREADY_EXISTS = new QStatus(0x90da);
			/// Socket close in progress
			public static readonly QStatus SOCK_CLOSING = new QStatus(0x90db);
			/// A referenced device cannot be located
			public static readonly QStatus NO_SUCH_DEVICE = new QStatus(0x90dc);
			/// An error occurred in a Wi-Fi Direct helper method call
			public static readonly QStatus P2P = new QStatus(0x90dd);
			/// A timeout occurred in a Wi-Fi Direct helper method call
			public static readonly QStatus P2P_TIMEOUT = new QStatus(0x90de);
			/// A required Wi-Fi Direct network connection does not exist
			public static readonly QStatus P2P_NOT_CONNECTED = new QStatus(0x90df);
			/// Exactly one mask bit was not set in the provided TransportMask
			public static readonly QStatus BAD_TRANSPORT_MASK = new QStatus(0x90e0);
			/// Fail to establish P2P proximity connection
			public static readonly QStatus PROXIMITY_CONNECTION_ESTABLISH_FAIL = new QStatus(0x90e1);
			/// Cannot find proximity P2P peers
			public static readonly QStatus PROXIMITY_NO_PEERS_FOUND = new QStatus(0x90e2);
			/// Operation not permitted on unregistered bus object
			public static readonly QStatus BUS_OBJECT_NOT_REGISTERED = new QStatus(0x90e3);
			/// Wi-Fi Direct is disabled on the device
			public static readonly QStatus P2P_DISABLED = new QStatus(0x90e4);
			/// Wi-Fi Direct resources are in busy state
			public static readonly QStatus P2P_BUSY = new QStatus(0x90e5);
			/// The daemon version is too old to be used by this client
			public static readonly QStatus BUS_INCOMPATIBLE_DAEMON = new QStatus(0x90e6);
			/// Attempt to execute a Wi-Fi Direct GO-related operation while STA
			public static readonly QStatus P2P_NO_GO = new QStatus(0x90e7);
			/// Attempt to execute a Wi-Fi Direct STA-related operation while GO
			public static readonly QStatus P2P_NO_STA = new QStatus(0x90e8);
			/// Attempt to execute a forbidden Wi-Fi Direct operation
			public static readonly QStatus P2P_FORBIDDEN = new QStatus(0x90e9);
			/// OnAppSuspend reply: Failed
			public static readonly QStatus ALLJOYN_ONAPPSUSPEND_REPLY_FAILED = new QStatus(0x90ea);
			/// OnAppSuspend reply: Unsupported operation
			public static readonly QStatus ALLJOYN_ONAPPSUSPEND_REPLY_UNSUPPORTED = new QStatus(0x90eb);
			/// OnAppResume reply: Failed
			public static readonly QStatus ALLJOYN_ONAPPRESUME_REPLY_FAILED = new QStatus(0x90ec);
			/// OnAppResume reply: Unsupported operation
			public static readonly QStatus ALLJOYN_ONAPPRESUME_REPLY_UNSUPPORTED = new QStatus(0x90ed);
			/// Message not found
			public static readonly QStatus BUS_NO_SUCH_MESSAGE = new QStatus(0x90ee);
			/// RemoveSessionMember reply: Specified session Id with this endpoint was not found
			public static readonly QStatus ALLJOYN_REMOVESESSIONMEMBER_REPLY_NO_SESSION = new QStatus(0x90ef);
			/// RemoveSessionMember reply: Endpoint is not the binder of session
			public static readonly QStatus ALLJOYN_REMOVESESSIONMEMBER_NOT_BINDER = new QStatus(0x90f0);
			/// RemoveSessionMember reply: Session is not multipoint
			public static readonly QStatus ALLJOYN_REMOVESESSIONMEMBER_NOT_MULTIPOINT = new QStatus(0x90f1);
			/// RemoveSessionMember reply: Specified session member was not found
			public static readonly QStatus ALLJOYN_REMOVESESSIONMEMBER_NOT_FOUND = new QStatus(0x90f2);
			/// RemoveSessionMember reply: The remote daemon does not support this feature
			public static readonly QStatus ALLJOYN_REMOVESESSIONMEMBER_INCOMPATIBLE_REMOTE_DAEMON = new QStatus(0x90f3);
			/// RemoveSessionMember reply: Failed for unspecified reason
			public static readonly QStatus ALLJOYN_REMOVESESSIONMEMBER_REPLY_FAILED = new QStatus(0x90f4);
			/// The session member was removed by the binder
			public static readonly QStatus BUS_REMOVED_BY_BINDER = new QStatus(0x90f5);
		}
		#region DLL Imports
		[DllImport(DLL_IMPORT_TARGET)]
		private extern static IntPtr QCC_StatusText(int status);

		#endregion
	}
}
