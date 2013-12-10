/**
 * @file
 *
 * This file defines the base class for message bus objects that
 * are implemented and registered locally.
 *
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
using System.Threading;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * Message Bus Object base class
		 */
		public class BusObject : IDisposable
		{
			/**
			 * %BusObject constructor.
			 *
			 * @param path           Object path for object.
			 */
			public BusObject(string path) : this(path, false) { }
			/**
			 * %BusObject constructor.
			 *
			 * @param path           Object path for object.
			 * @param isPlaceholder  Place-holder objects are created by the bus itself and serve only
			 *                       as parent objects (in the object path sense) to other objects.
			 */
			public BusObject(string path, bool isPlaceholder)
			{
				// Can't let the GC free these delegates so they must be members
				_propertyGet = new InternalPropertyGetEventHandler(this._PropertyGet);
				_propertySet = new InternalPropertySetEventHandler(this._PropertySet);
				_objectRegistered = new InternalObjectRegisteredEventHandler(this._ObjectRegistered);
				_objectUnregistered = new InternalObjectUnregisteredEventHandler(this._ObjectUnregistered);

				// Ref holder for method handler internal delegates
				_methodHandlerDelegateRefHolder = new List<InternalMethodHandler>();

				callbacks.property_get = Marshal.GetFunctionPointerForDelegate(_propertyGet);
				callbacks.property_set = Marshal.GetFunctionPointerForDelegate(_propertySet);
				callbacks.object_registered = Marshal.GetFunctionPointerForDelegate(_objectRegistered);
				callbacks.object_unregistered = Marshal.GetFunctionPointerForDelegate(_objectUnregistered);

				main = GCHandle.Alloc(callbacks, GCHandleType.Pinned);
				_busObject = alljoyn_busobject_create(path, isPlaceholder ? 1 : 0, main.AddrOfPinnedObject(), IntPtr.Zero);
			}

			/**
			 * %BusObject constructor.
			 * 
			 * Use of this BusObject constructor is depricated and will be 
			 * removed Please use other BusObject Constructor BusObject(string, bool)
			 *
			 * @param bus            Bus that this object exists on.
			 * @param path           Object path for object.
			 * @param isPlaceholder  Place-holder objects are created by the bus itself and serve only
			 *                       as parent objects (in the object path sense) to other objects.
			 */
			[Obsolete("Creation of a BusObject no longer requires a BusAttachment.")]
			public BusObject(BusAttachment bus, string path, bool isPlaceholder) : this(path, isPlaceholder){}

			/**
			 * Request the raw pointer of the AllJoyn C BusObject
			 *
			 * @return the raw pointer of the AllJoyn C BusObject
			 */ 
			public IntPtr getAddr()
			{
				return _busObject;
			}

			/**
			 * Add an interface to this object. If the interface has properties this will also add the
			 * standard property access interface. An interface must be added before its method handlers can be
			 * added. Note that the Peer interface (org.freedesktop.DBus.peer) is implicit on all objects and
			 * cannot be explicitly added, and the Properties interface (org.freedesktop,DBus.Properties) is
			 * automatically added when needed and cannot be explicitly added.
			 *
			 * Once an object is registered, it should not add any additional interfaces. Doing so would
			 * confuse remote objects that may have already introspected this object.
			 *
			 * @param iface  The interface to add
			 *
			 * @return
			 *      - QStatus.OK if the interface was successfully added.
			 *      - QStatus.BUS_IFACE_ALREADY_EXISTS if the interface already exists.
			 *      - An error status otherwise
			 */
			public QStatus AddInterface(InterfaceDescription iface)
			{
				return alljoyn_busobject_addinterface(_busObject, iface.UnmanagedPtr);
			}

			/**
			 * Add a method handler to this object. The interface for the method handler must have already
			 * been added by calling AddInterface().
			 *
			 * @param member   Interface member implemented by handler.
			 * @param handler  Method handler.
			 *
			 * @return
			 *      - QStatus.OK if the method handler was added.
			 *      - An error status otherwise
			 */
			public QStatus AddMethodHandler(InterfaceDescription.Member member, MethodHandler handler)
			{
				InternalMethodHandler internalMethodHandler = (IntPtr bus, IntPtr m, IntPtr msg) =>
				{
					MethodHandler h = handler;
					h(new InterfaceDescription.Member(m), new Message(msg));
				};
				_methodHandlerDelegateRefHolder.Add(internalMethodHandler);

				GCHandle membGch = GCHandle.Alloc(member._member, GCHandleType.Pinned);

				MethodEntry entry;
				entry.member = membGch.AddrOfPinnedObject();
				entry.method_handler = Marshal.GetFunctionPointerForDelegate(internalMethodHandler);

				GCHandle gch = GCHandle.Alloc(entry, GCHandleType.Pinned);
				QStatus ret = alljoyn_busobject_addmethodhandlers(_busObject, gch.AddrOfPinnedObject(), (UIntPtr)1);
				gch.Free();
				membGch.Free();

				return ret;
			}

			/**
			 * Reply to a method call.
			 *
			 * @param message      The method call message
			 * @param args     The reply arguments (can be NULL)
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			[Obsolete("Usage of MethodReply that takes MsgArgs been depricated. Please use MsgArg inplace of MsgArgs")]
			protected QStatus MethodReply(Message message, MsgArgs args)
			{
				return alljoyn_busobject_methodreply_args(_busObject, message.UnmanagedPtr, args.UnmanagedPtr,
					(UIntPtr)args.Length);
			}

			/**
			 * Reply to a method call.
			 *
			 * @param message      The method call message
			 * @param args     The reply arguments (can be NULL)
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus MethodReply(Message message, MsgArg args)
			{
				return alljoyn_busobject_methodreply_args(_busObject, message.UnmanagedPtr, args.UnmanagedPtr,
					(UIntPtr)args.Length);
			}

			/**
			 * Reply to a method call with an error message.
			 *
			 * @param message              The method call message
			 * @param error            The name of the error
			 * @param errorMessage     An error message string
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus MethodReply(Message message, string error, string errorMessage)
			{
				return alljoyn_busobject_methodreply_err(_busObject, message.UnmanagedPtr, error,
					errorMessage);
			}

			/**
			 * Reply to a method call with an error message.
			 *
			 * @param message        The method call message
			 * @param status     The status code for the error
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus MethodReply(Message message, QStatus status)
			{
				return alljoyn_busobject_methodreply_status(_busObject, message.UnmanagedPtr, status.value);
			}

			/**
			 * Send a signal.
			 *
			 * @param destination      The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
			 * @param sessionId        A unique SessionId for this AllJoyn session instance
			 * @param signal           Interface member of signal being emitted.
			 * @param args             The arguments for the signal (can be NULL)
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			[Obsolete("Usage of Signal that takes MsgArgs been depricated. Please use MsgArg inplace of MsgArgs")]
			protected QStatus Signal(string destination, uint sessionId, InterfaceDescription.Member signal, MsgArgs args)
			{
				return alljoyn_busobject_signal(_busObject, destination, sessionId, signal._member, args.UnmanagedPtr, (UIntPtr)args.Length, 0, 0, IntPtr.Zero);
			}

			/**
			 * Send a signal.
			 *
			 * @param destination      The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
			 * @param sessionId        A unique SessionId for this AllJoyn session instance
			 * @param signal           Interface member of signal being emitted.
			 * @param args             The arguments for the signal (can be NULL)
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus Signal(string destination, uint sessionId, InterfaceDescription.Member signal, MsgArg args)
			{
				return alljoyn_busobject_signal(_busObject, destination, sessionId, signal._member, args.UnmanagedPtr, (UIntPtr)args.Length, 0, 0, IntPtr.Zero);
			}

			/**
			 * Send a signal.
			 *
			 * @param destination      The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
			 * @param sessionId        A unique SessionId for this AllJoyn session instance
			 * @param signal           Interface member of signal being emitted.
			 * @param args             The arguments for the signal (can be NULL)
			 * @param msg              The sent signal message is returned to the caller
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus Signal(string destination, uint sessionId, InterfaceDescription.Member signal, MsgArg args, Message msg)
			{
				return alljoyn_busobject_signal(_busObject, destination, sessionId, signal._member, args.UnmanagedPtr, (UIntPtr)args.Length, 0, 0, msg.UnmanagedPtr);
			}

			/**
			 * Send a signal.
			 *
			 * @param destination      The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
			 * @param sessionId        A unique SessionId for this AllJoyn session instance
			 * @param signal           Interface member of signal being emitted.
			 * @param args             The arguments for the signal (can be NULL)
			 * @param timeToLife       If non-zero this specifies the useful lifetime for this signal.
			 *                         The units are milliseconds for non-sessionless signals and seconds for 
			 *                         sessionless signals. If delivery of the signal is delayed beyond the 
			 *                         timeToLive due to network congestion or other factors the signal may 
			 *                         be discarded. There is no guarantee that expired signals will not still 
			 *                         be delivered.
			 * @param flags            Logical OR of the message flags for this signals. The following flags apply to signals:
			 *                         - If ALLJOYN_FLAG_SESSIONLESS is set the signal will be sent out to any listener without requireing a connected session.
			 *                         - If ALLJOYN_FLAG_GLOBAL_BROADCAST is set broadcast signal (null destination) will be forwarded across bus-to-bus connections.
			 *                         - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
			 *                         - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			[Obsolete("Usage of Signal that takes MsgArgs been depricated. Please use MsgArg inplace of MsgArgs")]
			protected QStatus Signal(string destination, uint sessionId, InterfaceDescription.Member signal,
				MsgArgs args, ushort timeToLife, byte flags)
			{
				return alljoyn_busobject_signal(_busObject, destination, sessionId, signal._member, args.UnmanagedPtr, (UIntPtr)args.Length, timeToLife, flags, IntPtr.Zero);
			}

			/**
			 * Send a signal.
			 *
			 * @param destination      The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
			 * @param sessionId        A unique SessionId for this AllJoyn session instance
			 * @param signal           Interface member of signal being emitted.
			 * @param args             The arguments for the signal (can be NULL)
			 * @param timeToLife       If non-zero this specifies in milliseconds the useful lifetime for this
			 *                         signal. If delivery of the signal is delayed beyond the timeToLive due to
			 *                         network congestion or other factors the signal may be discarded. There is
			 *                         no guarantee that expired signals will not still be delivered.
			 * @param flags            Logical OR of the message flags for this signals. The following flags apply to signals:
			 *                         - If ALLJOYN_FLAG_SESSIONLESS is set the signal will be sent out to any listener without requireing a connected session
			 *                         - If ALLJOYN_FLAG_GLOBAL_BROADCAST is set broadcast signal (null destination) will be
			 *                           forwarded across bus-to-bus connections.
			 *                         - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can
			 *                           handle header compression.
			 *                         - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any
			 *                           is encrypted.
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus Signal(string destination, uint sessionId, InterfaceDescription.Member signal,
				MsgArg args, ushort timeToLife, byte flags)
			{
				return alljoyn_busobject_signal(_busObject, destination, sessionId, signal._member, args.UnmanagedPtr, (UIntPtr)args.Length, timeToLife, flags, IntPtr.Zero);
			}

			/**
			 * Send a signal.
			 *
			 * @param destination      The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
			 * @param sessionId        A unique SessionId for this AllJoyn session instance
			 * @param signal           Interface member of signal being emitted.
			 * @param args             The arguments for the signal (can be NULL)
			 * @param timeToLife       If non-zero this specifies in milliseconds the useful lifetime for this
			 *                         signal. If delivery of the signal is delayed beyond the timeToLive due to
			 *                         network congestion or other factors the signal may be discarded. There is
			 *                         no guarantee that expired signals will not still be delivered.
			 * @param flags            Logical OR of the message flags for this signals. The following flags apply to signals:
			 *                         - If ALLJOYN_FLAG_SESSIONLESS is set the signal will be sent out to any listener without
			 *                           requireing a connected session
			 *                         - If ALLJOYN_FLAG_GLOBAL_BROADCAST is set broadcast signal (null destination) will be
			 *                           forwarded across bus-to-bus connections.
			 *                         - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can
			 *                           handle header compression.
			 *                         - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any
			 *                           is encrypted.
			 * @param msg              The sent signal message is returned to the caller.
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			protected QStatus Signal(string destination, uint sessionId, InterfaceDescription.Member signal,
				MsgArg args, ushort timeToLife, byte flags, Message msg)
			{
				return alljoyn_busobject_signal(_busObject, destination, sessionId, signal._member, args.UnmanagedPtr, (UIntPtr)args.Length, timeToLife, flags, msg.UnmanagedPtr);
			}

			/**
			 * Remove sessionless message sent from this object from local daemon's
			 * store/forward cache.
			 *
			 * @param serialNumber    Serial number of previously sent sessionless signal.
			 * @return  QStatus.OK if successful.
			 */
			public QStatus CancelSessionlessMessage(uint serialNumber)
			{
				return alljoyn_busobject_cancelsessionlessmessage_serial(_busObject, serialNumber);
			}

			/**
			 * Remove sessionless message sent from this object from local daemon's
			 * store/forward cache.
			 *
			 * @param msg    Message to be removed.
			 * @return   QStatus.OK if successful.
			 */
			public QStatus CancelSessionlessMessage(Message msg)
			{
				return alljoyn_busobject_cancelsessionlessmessage(_busObject, msg.UnmanagedPtr);
			}
			#region Properties
			/**
			 * Return the path for the object
			 *
			 * @return Object path
			 */
			public string Path
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_busobject_getpath(_busObject));
				}
			}

			/**
			 * Get the name of this object.
			 * The name is the last component of the path.
			 *
			 * @return Last component of object path.
			 */
			public string Name
			{
				get
				{
					UIntPtr nameSz = alljoyn_busobject_getname(_busObject, IntPtr.Zero, (UIntPtr)0);
					byte[] sink = new byte[(int)nameSz];

					GCHandle gch = GCHandle.Alloc(sink, GCHandleType.Pinned);
					alljoyn_busobject_getname(_busObject, gch.AddrOfPinnedObject(), nameSz);
					gch.Free();
					// The returned buffer will contain a nul character an so we must remove the last character.
					if ((int)nameSz != 0)
					{
						return System.Text.ASCIIEncoding.ASCII.GetString(sink, 0, (Int32)nameSz - 1);
					}
					else
					{
						return "";
					}
				}
			}

			/**
			 * Indicates if this object is secure.
			 *
			 * @param bus    The busobject we want to check for security
			 *
			 * @return Return QCC_TRUE if authentication is required to emit signals or
			 *                call methods on this object.
			 */
			public bool IsSecure
			{
				get
				{
					return alljoyn_busobject_issecure(_busObject);
				}
			}
			#endregion

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			/**
			 * MethodHandlers are %MessageReceiver methods which are called by AllJoyn library
			 * to forward AllJoyn method_calls to AllJoyn library users.
			 *
			 * @param member    Method interface member entry.
			 * @param message   The received method call message.
			 */
			public delegate void MethodHandler(InterfaceDescription.Member member, Message message);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalMethodHandler(IntPtr bus, IntPtr member, IntPtr message);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalPropertyGetEventHandler(IntPtr context, IntPtr ifcName, IntPtr propName, IntPtr val);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalPropertySetEventHandler(IntPtr context, IntPtr ifcName, IntPtr propName, IntPtr val);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalObjectRegisteredEventHandler(IntPtr context);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalObjectUnregisteredEventHandler(IntPtr context);
			#endregion

			#region Virtual Methods
			/**
			 * Handle a bus request to read a property from this object.
			 * BusObjects that implement properties should override this method.
			 * The default version simply returns QStatus.BUS_NO_SUCH_PROPERTY
			 *
			 * @param ifcName   Identifies the interface that the property is defined on
			 * @param propName  Identifies the the property to get
			 * @param[out] val  Returns the property value. The type of this value is the actual value
			 *                  type.
			 */
			protected virtual void OnPropertyGet(string ifcName, string propName, MsgArg val)
			{
			
			}

			/**
			 * Handle a bus attempt to write a property value to this object.
			 * BusObjects that implement properties should override this method.
			 * This default version just replies with QStatus.BUS_NO_SUCH_PROPERTY
			 *
			 * @param ifcName    Identifies the interface that the property is defined on
			 * @param propName  Identifies the the property to set
			 * @param val        The property value to set. The type of this value is the actual value
			 *                   type.
			 */
			protected virtual void OnPropertySet(string ifcName, string propName, MsgArg val)
			{
			
			}

			/**
			 * Called by the message bus when the object has been successfully registered. The object can
			 * perform any initialization such as adding match rules at this time.
			 */
			protected virtual void OnObjectRegistered()
			{
			
			}

			/**
			 * Called by the message bus when the object has been successfully unregistered
			 * @remark
			 * This base class implementation @b must be called explicitly by any overriding derived class.
			 */
			protected virtual void OnObjectUnregistered()
			{
			
			}
			#endregion

			#region Callbacks
			private void _PropertyGet(IntPtr context, IntPtr ifcName, IntPtr propName, IntPtr val)
			{
			
				OnPropertyGet(Marshal.PtrToStringAnsi(ifcName),
					Marshal.PtrToStringAnsi(propName), new MsgArg(val));
			}

			private void _PropertySet(IntPtr context, IntPtr ifcName, IntPtr propName, IntPtr val)
			{
			
				OnPropertySet(Marshal.PtrToStringAnsi(ifcName),
					Marshal.PtrToStringAnsi(propName), new MsgArg(val));
			}

			private void _ObjectRegistered(IntPtr context)
			{
			
				OnObjectRegistered();
			}

			private void _ObjectUnregistered(IntPtr context)
			{
			
				OnObjectUnregistered();
			}
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busobject_create(
				[MarshalAs(UnmanagedType.LPStr)] string path,
				int isPlaceholder,
				IntPtr callbacks_in,
				IntPtr context_in);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busobject_destroy(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busobject_getpath(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_busobject_getname(IntPtr bus, IntPtr buffer, UIntPtr bufferSz);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_addinterface(IntPtr bus, IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_addmethodhandlers(IntPtr bus,
				IntPtr entries, UIntPtr numEntries);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_methodreply_args(IntPtr bus,
				IntPtr msg, IntPtr msgArgs, UIntPtr numArgs);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_methodreply_err(IntPtr bus, IntPtr msg,
				[MarshalAs(UnmanagedType.LPStr)] string error,
				[MarshalAs(UnmanagedType.LPStr)] string errorMsg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_methodreply_status(IntPtr bus, IntPtr msg, int status);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_signal(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string destination,
				uint sessionId,
				InterfaceDescription._Member signal,
				IntPtr msgArgs, UIntPtr numArgs,
				ushort timeToLive, byte flags, IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_cancelsessionlessmessage_serial(IntPtr bus, uint serialNumber);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busobject_cancelsessionlessmessage(IntPtr bus, IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_busobject_issecure(IntPtr bus);
			#endregion

			#region IDisposable
			~BusObject()
			{

				Dispose(false);
			}

			/**
			 * Dispose the BusObject
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
			
				if(!_isDisposed)
				{
					Thread destroyThread = new Thread((object o) => {
						alljoyn_busobject_destroy(_busObject); 
					});
					destroyThread.Start();
					while(destroyThread.IsAlive)
					{
						AllJoyn.TriggerCallbacks();
						Thread.Sleep(0);
					}
					_busObject = IntPtr.Zero;
					main.Free();
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the BusObject
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region Structs
			[StructLayout(LayoutKind.Sequential)]
			private struct BusObjectCallbacks
			{
				public IntPtr property_get;
				public IntPtr property_set;
				public IntPtr object_registered;
				public IntPtr object_unregistered;
			}

			[StructLayout(LayoutKind.Sequential)]
			private struct MethodEntry
			{
				public IntPtr member;
				public IntPtr method_handler;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _busObject;
				}
			}
			#endregion

			#region Data
			IntPtr _busObject;
			bool _isDisposed = false;

			GCHandle main;
			InternalPropertyGetEventHandler _propertyGet;
			InternalPropertySetEventHandler _propertySet;
			InternalObjectRegisteredEventHandler _objectRegistered;
			InternalObjectUnregisteredEventHandler _objectUnregistered;

			BusObjectCallbacks callbacks;

			List<InternalMethodHandler> _methodHandlerDelegateRefHolder;
			#endregion
		}
	}
}
