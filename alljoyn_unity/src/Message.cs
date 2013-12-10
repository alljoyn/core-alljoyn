/**
 * @file
 * This file defines a class for parsing and generating message bus messages
 */

/******************************************************************************
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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
using System.Text;
using System.Runtime.InteropServices;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		public const int ALLJOYN_MAX_NAME_LEN = 255;  /*!<  The maximum length of certain bus names */
		public const int ALLJOYN_MAX_ARRAY_LEN = 131072;  /*!<  DBus limits array length to 2^26. AllJoyn limits it to 2^17 */
		public const int ALLJOYN_MAX_PACKET_LEN = (ALLJOYN_MAX_ARRAY_LEN + 4096);  /*!<  DBus limits packet length to 2^27. AllJoyn limits it further to 2^17 + 4096 to allow for 2^17 payload */

		/** @name Endianness indicators */
		// @{
		/** indicates the bus is little endian */
		public const byte ALLJOYN_LITTLE_ENDIAN = 0x6c; // hex value for ascii letter 'l'
		/** indicates the bus is big endian */
		public const byte ALLJOYN_BIG_ENDIAN = 0x42; // hex value for ascii letter 'B'
		// @}


		/** @name Flag types */
		// @{
		/** No reply is expected*/
		public const byte ALLJOYN_FLAG_NO_REPLY_EXPECTED = 0x01;
		/** Auto start the service */
		public const byte ALLJOYN_FLAG_AUTO_START = 0x02;
		/** Allow messages from remote hosts (valid only in Hello message) */
		public const byte ALLJOYN_FLAG_ALLOW_REMOTE_MSG = 0x04;
		/** Sessionless message  */
		public const byte ALLJOYN_FLAG_SESSIONLESS = 0x10;
		/** Global (bus-to-bus) broadcast */
		public const byte ALLJOYN_FLAG_GLOBAL_BROADCAST = 0x20;
		/** Header is compressed */
		public const byte ALLJOYN_FLAG_COMPRESSED = 0x40;
		/** Body is encrypted */
		public const byte ALLJOYN_FLAG_ENCRYPTED = 0x80;
		// @}

		/** ALLJOYN protocol version */
		public const byte ALLJOYN_MAJOR_PROTOCOL_VERSION = 1;

		/**
		 * Message is a reference counted (managed) version of _Message
		 */
		public class Message : IDisposable
		{
			/** Message types */
			public enum Type : int
			{
				Invalid = 0, ///< an invalid message type
				MethodCall = 1, ///< a method call message type
				MethodReturn = 2, ///< a method return message type
				Error = 3, ///< an error message type
				Signal = 4 ///< a signal message type
			}

			/**
			 * Constructor for a message
			 *
			 * @param bus  The bus that this message is sent or received on.
			 */
			public Message(BusAttachment bus)
			{
				_message = alljoyn_message_create(bus.UnmanagedPtr);
			}

			/**
			 * Copy constructor for a message
			 *
			 * @param other  Message to copy from.
			 */
			internal Message(IntPtr message)
			{
				_message = message;
				_isDisposed = true;
			}

			/**
			 * Return a specific argument.
			 *
			 * @param index  The index of the argument to get.
			 *
			 * @return
			 *      - The argument
			 *      - NULL if unmarshal failed or there is not such argument.
			 */
			public MsgArg GetArg(int index)
			{
				IntPtr msgArgs = alljoyn_message_getarg(_message, (UIntPtr)index);
				return (msgArgs != IntPtr.Zero ? new MsgArg(msgArgs) : null);
			}

			/**
			 * Return the arguments for this Message.
			 *
			 * @return An AllJoyn.MsgArg containing all the arguments for this Message
			 */
			public MsgArg GetArgs()
			{
				IntPtr MsgArgPtr;
				UIntPtr numArgs;
				alljoyn_message_getargs(_message, out numArgs, out MsgArgPtr);
				MsgArg args = new MsgArg(MsgArgPtr, (int)numArgs);
				return args;
			}

			/**
			 * Unpack and return the arguments for this message. This method uses the
			 * functionality from AllJoyn.MsgArg.Get(string, out object) see MsgArg.cs
			 * for documentation.
			 *
			 * @param signature  The signature to match against the message arguments.
			 * @param value      object unpacked values the values into
			 * @return
			 *      - QStatus.OK if the signature matched and MsgArg was successfully unpacked.
			 *      - QStatus.BUS_SIGNATURE_MISMATCH if the signature did not match.
			 *      - An error status otherwise
			 */
			public QStatus GetArgs(string signature, out object value)
			{
				return GetArgs().Get(signature, out value);
			}

			/**
			 * Accessor function to get the sender for this message.
			 *
			 * @return
			 *      - The senders well-known name string stored in the AllJoyn header field.
			 *      - An empty string if the message did not specify a sender.
			 */
			[Obsolete("Use the property Sender instead of the function GetSender")]
			public string GetSender()
			{
				IntPtr sender = alljoyn_message_getsender(_message);
				return (sender != IntPtr.Zero ? Marshal.PtrToStringAnsi(sender) : null);
			}

			/**
			 * Return a specific argument.
			 *
			 * @param index  The index of the argument to get.
			 *
			 * @return
			 *      - The argument
			 *      - NULL if unmarshal failed or there is not such argument.
			 */
			public MsgArg this[int i]
			{
				get
				{
					return GetArg(i);
				}
			}

			/**
			 * Return true if message's TTL header indicates that it is expired
			 *
			 * @return Returns true if the message's TTL header indicates that is has expired.
			 */
			public bool IsExpired()
			{
				uint throw_away_value;
				return alljoyn_message_isexpired(_message, out throw_away_value);
			}

			/**
			 * Return true if message's TTL header indicates that it is expired
			 *
			 * @param[out] tillExpireMS  Written with number of milliseconds before message expires
			 *                           If message never expires value is set to the uint.MaxValue.
			 *
			 * @return Returns true if the message's TTL header indicates that is has expired.
			 */
			public bool IsExpired(out uint tillExpireMS)
			{
				return alljoyn_message_isexpired(_message, out tillExpireMS);
			}

			/**
			 * If the message is an error message returns the error name and optionally the error message string
			 *
			 * @param[out] errorMessage  Return the error message string stored
			 *
			 * @return
			 *      - If error detected return error name stored in the AllJoyn header field
			 *      - NULL if error not detected
			 */
			public string GetErrorName(out string errorMessage)
			{
				UIntPtr errorMessageSz;
				alljoyn_message_geterrorname(_message, IntPtr.Zero, out errorMessageSz);
				byte[] sink = new byte[(int)errorMessageSz];

				GCHandle gch = GCHandle.Alloc(sink, GCHandleType.Pinned);
				IntPtr errorName = alljoyn_message_geterrorname(_message, gch.AddrOfPinnedObject(), out errorMessageSz);
				gch.Free();
				// The returned buffer will contain a nul character an so we must remove the last character.
				errorMessage = System.Text.ASCIIEncoding.ASCII.GetString(sink, 0, (int)errorMessageSz - 1);
				return Marshal.PtrToStringAnsi(errorName);
			}

			/**
			 * Returns an XML string representation of the message
			 *
			 * @return an XML string representation of the message
			 *
			 */
			public override string ToString()
			{
				UIntPtr signatureSz = alljoyn_message_tostring(_message, IntPtr.Zero, (UIntPtr)0);
				byte[] sink = new byte[(int)signatureSz];

				GCHandle gch = GCHandle.Alloc(sink, GCHandleType.Pinned);
				alljoyn_message_tostring(_message, gch.AddrOfPinnedObject(), signatureSz);
				gch.Free();
				// The returned buffer will contain a nul character an so we must remove the last character.
				if ((int)signatureSz != 0)
				{
					return System.Text.ASCIIEncoding.ASCII.GetString(sink, 0, (int)signatureSz - 1);
				}
				else
				{
					return "";
				}
			}

			#region Properties
			/**
			 * Determine if message is a broadcast signal.
			 *
			 * @return  Return true if this is a broadcast signal.
			 */
			public bool IsBroadcastSignal
			{
				get
				{
					return alljoyn_message_isbroadcastsignal(_message);
				}
			}

			/**
			 * Messages broadcast to all devices are global broadcast messages.
			 * 
			 * @return  Return true if this is a global broadcast message.
			 */
			public bool IsGlobalBroadcast
			{
				get
				{
					return alljoyn_message_isglobalbroadcast(_message);
				}
			}

			/**
			 * Messages sent without sessions are sessionless.
			 *
			 * @return  Return true if this is a sessionless message.
			 */
			public bool IsSessionless
			{
				get
				{
					return alljoyn_message_issessionless(_message);
				}
			}
			/**
			 * Returns the flags for the message.
			 * @return flags for the message
			 *
			 * @see Flag types
			 */
			public byte Flags
			{
				get
				{
					return alljoyn_message_getflags(_message);
				}
			}

			/**
			 * Determine if the message is marked as unreliable. Unreliable messages have a
			 * non-zero time-to-live and may be silently discarded.
			 *
			 * @return  Returns true if the message is unreliable, that is, has a
			 *          non-zero time-to-live.
			 */
			public bool IsUnreliable
			{
				get
				{
					return alljoyn_message_isunreliable(_message);
				}
			}

			/**
			 * Determine if the message was encrypted.
			 *
			 * @return  Returns true if the message was encrypted.
			 */
			public bool IsEncrypted
			{
				get
				{
					return alljoyn_message_isencrypted(_message);
				}
			}

			/**
			 * Get the name of the authentication mechanism that was used to generate
			 * the encryption key if the message is encrypted.
			 *
			 * @return  the name of an authentication mechanism or an empty string.
			 */
			public string AuthMechanism
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getauthmechanism(_message));
				}
			}

			/**
			 * Return the type of the message
			 *
			 * @return message type
			 */
			public Type MessageType
			{
				get
				{
					return (Type)alljoyn_message_gettype(_message);
				}
			}

			/**
			 * Accessor function to get serial number for the message. Usually only
			 * important for AllJoyn.Message.MethodCall for matching up the reply to
			 * the call.
			 *
			 * @return the serial number of the Message
			 */
			public uint CallSerial
			{
				get
				{
					return alljoyn_message_getcallserial(_message);
				}
			}

			/**
			 * Get the signature for this message
			 *
			 * @return
			 *      - The AllJoyn SIGNATURE string stored in the AllJoyn header field
			 *      - An empty string if unable to find the AllJoyn signature
			 */
			public string Signature
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getsignature(_message));
				}
			}

			/**
			 * Accessor function to get the object path for this message
			 *
			 * @return
			 *      - The AllJoyn object path string stored in the AllJoyn header field
			 *      - An empty string if unable to find the AllJoyn object path
			 */
			public string ObjectPath
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getobjectpath(_message));
				}
			}

			/**
			 * Accessor function to get the interface for this message
			 *
			 * @return
			 *      - The AllJoyn interface string stored in the AllJoyn header field
			 *      - An empty string if unable to find the interface
			 */
			public string Interface
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getinterface(_message));
				}
			}

			/**
			 * Accessor function to get the member (method/signal) name for this message
			 *
			 * @return
			 *      - The AllJoyn member (method/signal) name string stored in the AllJoyn header field
			 *      - An empty string if unable to find the member name
			 */
			public string MemberName
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getmembername(_message));
				}
			}

			/**
			 * Accessor function to get the reply serial number for the message. Only meaningful for
			 * AllJoyn.Message.Type.MethodReturn
			 *
			 * @return
			 *      - The serial number for the message stored in the AllJoyn header field
			 *      - Zero if unable to find the serial number. Note that 0 is an invalid serial number.
			 */
			public uint ReplySerial
			{
				get
				{
					return alljoyn_message_getreplyserial(_message);
				}
			}

			/**
			 * Accessor function to get the sender for this message.
			 *
			 * @return
			 *      - The senders well-known name string stored in the AllJoyn header field.
			 *      - An empty string if the message did not specify a sender.
			 */
			public string Sender
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getsender(_message));
				}
			}

			/**
			 * Get the unique name of the endpoint that the message was received on.
			 *
			 * @return
			 *     - The unique name of the endpoint that the message was received on.
			 */
			public string ReceiveEndPointName
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getreceiveendpointname(_message));
				}
			}

			/**
			 * Accessor function to get the destination for this message
			 *
			 * @return
			 *      - The message destination string stored in the AllJoyn header field.
			 *      - An empty string if unable to find the message destination.
			 */
			public string Destination
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_message_getdestination(_message));
				}
			}

			/**
			 * Accessor function to get the compression token for the message.
			 *
			 * @return
			 *      - Compression token for the message stored in the AllJoyn header field
			 *      - 0 'zero' if there is no compression token.
			 */
			public uint CompressionToken
			{
				get
				{
					return alljoyn_message_getcompressiontoken(_message);
				}
			}

			/**
			 * Accessor function to get the session id for the message.
			 *
			 * @return
			 *      - Session id for the message
			 *      - 0 'zero' if sender did not specify a session
			 */
			public uint SessionId
			{
				get
				{
					return alljoyn_message_getsessionid(_message);
				}
			}

			/**
			 * Returns a string that provides a brief description of the message
			 *
			 * @return A string that provides a brief description of the message
			 */
			public string Description
			{
				get
				{
					UIntPtr descriptionSz = alljoyn_message_description(_message, IntPtr.Zero, (UIntPtr)0);
					byte[] sink = new byte[(int)descriptionSz];

					GCHandle gch = GCHandle.Alloc(sink, GCHandleType.Pinned);
					alljoyn_message_description(_message, gch.AddrOfPinnedObject(), descriptionSz);
					gch.Free();
					// The returned buffer will contain a nul character an so we must remove the last character.
					if ((int)descriptionSz != 0)
					{
						return System.Text.ASCIIEncoding.ASCII.GetString(sink, 0, (int)descriptionSz - 1);
					}
					else
					{
						return "";
					}
				}
			}

			/**
			 * Returns the timestamp (in milliseconds) for this message. If the message header contained a
			 * timestamp this is the estimated timestamp for when the message was sent by the remote device,
			 * otherwise it is the timestamp for when the message was unmarshaled. Note that the timestamp
			 * is always relative to local time.
			 *
			 * @return The timestamp for this message.
			 */
			public uint TimeStamp
			{
				get
				{
					return alljoyn_message_gettimestamp(_message);
				}
			}
			#endregion

			#region IDisposable
			~Message()
			{
				Dispose(false);
			}

			/**
			 * Dispose the Message
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if(!_isDisposed)
				{
					alljoyn_message_destroy(_message);
					_message = IntPtr.Zero;
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the Message
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this); 
			}
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_message_create(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_message_destroy(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_message_getarg(IntPtr msg, UIntPtr argN);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_message_getargs(IntPtr msg, out UIntPtr numArgs, out IntPtr args);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_message_getsender(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_message_isbroadcastsignal(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_message_isglobalbroadcast(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_message_issessionless(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static byte alljoyn_message_getflags(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_message_isexpired(IntPtr msg, out uint tillExpireMS);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_message_isunreliable(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_message_isencrypted(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getauthmechanism(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_message_gettype(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_message_getcallserial(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getsignature(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getobjectpath(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getinterface(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getmembername(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_message_getreplyserial(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getreceiveendpointname(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_getdestination(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_message_getcompressiontoken(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_message_getsessionid(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_message_geterrorname(IntPtr msg, IntPtr erroMessage, out UIntPtr errorMessage_size);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_message_tostring(IntPtr msg, IntPtr str, UIntPtr buf);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_message_description(IntPtr msg, IntPtr str, UIntPtr bug);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_message_gettimestamp(IntPtr msg);
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _message;
				}
			}
			#endregion

			#region Data
			IntPtr _message;
			bool _isDisposed = false;
			#endregion
		}
	}
}
