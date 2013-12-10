/**
 * @file
 * This file defines the class ProxyBusObject.
 * The ProxyBusObject represents a single object registered registered on the bus.
 * ProxyBusObjects are used to make method calls on these remotely located DBus objects.
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
using System.Runtime.InteropServices;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * Each %ProxyBusObject instance represents a single DBus/AllJoyn object registered
		 * somewhere on the bus. ProxyBusObjects are used to make method calls on these
		 * remotely located DBus objects.
		 */
		public class ProxyBusObject : IDisposable
		{
			/**
			 * Create an empty proxy object that refers to an object at given remote service name. Note
			 * that the created proxy object does not contain information about the interfaces that the
			 * actual remote object implements with the exception that org.freedesktop.DBus.Peer
			 * interface is special-cased (per the DBus spec) and can always be called on any object. Nor
			 * does it contain information about the child objects that the actual remote object might
			 * contain.
			 *
			 * To fill in this object with the interfaces and child object names that the actual remote
			 * object describes in its introspection data, call IntrospectRemoteObject() or
			 * IntrospectRemoteObjectAsync().
			 *
			 * @param bus        The bus.
			 * @param service    The remote service name (well-known or unique).
			 * @param path       The absolute (non-relative) object path for the remote object.
			 * @param sessionId  The session id the be used for communicating with remote object.
			 */
			public ProxyBusObject(BusAttachment bus, string service, string path, uint sessionId)
			{
				_proxyBusObject = alljoyn_proxybusobject_create(bus.UnmanagedPtr, service, path, sessionId);
			}

			/**
			 * Create an empty proxy object that refers to an object at given remote service name. Note
			 * that the created proxy object does not contain information about the interfaces that the
			 * actual remote object implements with the exception that org.freedesktop.DBus.Peer
			 * interface is special-cased (per the DBus spec) and can always be called on any object. Nor
			 * does it contain information about the child objects that the actual remote object might
			 * contain.
			 *
			 * To fill in this object with the interfaces and child object names that the actual remote
			 * object describes in its introspection data, call IntrospectRemoteObject() or
			 * IntrospectRemoteObjectAsync().
			 *
			 * @param bus        The bus.
			 * @param service    The remote service name (well-known or unique).
			 * @param path       The absolute (non-relative) object path for the remote object.
			 * @param sessionId  The session id the be used for communicating with remote object.
			 * @param secure     The security mode for the remote object.
			 */
			public ProxyBusObject(BusAttachment bus, string service, string path, uint sessionId, bool secure)
			{
				_proxyBusObject = alljoyn_proxybusobject_create_secure(bus.UnmanagedPtr, service, path, sessionId);
			}

			internal ProxyBusObject(IntPtr busObject)
			{
				_proxyBusObject = busObject;
				_isDisposed = true;
			}

			/**
			 * Add an interface to this ProxyBusObject.
			 *
			 * Occasionally, AllJoyn library user may wish to call a method on
			 * a %ProxyBusObject that was not reported during introspection of the remote object.
			 * When this happens, the InterfaceDescription will have to be registered with the
			 * Bus manually and the interface will have to be added to the %ProxyBusObject using
			 * this method.
			 *
			 * @remark
			 * The interface added via this call must have been previously registered with the
			 * Bus. (i.e. it must have come from a call to Bus.GetInterface()).
			 *
			 * @param iface    The interface to add to this object. Must come from Bus.GetInterface().
			 * @return
			 *      - QStatus.OK if successful.
			 *      - An error status otherwise
			 */
			public QStatus AddInterface(InterfaceDescription iface)
			{
				return alljoyn_proxybusobject_addinterface(_proxyBusObject, iface.UnmanagedPtr);
			}

			/**
			 * Query the remote object on the bus to determine the interfaces and
			 * children that exist. Use this information to populate this proxy's
			 * interfaces and children.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - An error status otherwise
			 */
			public QStatus IntrospectRemoteObject()
			{
				return alljoyn_proxybusobject_introspectremoteobject(_proxyBusObject);
			}
			/**
			 * Make a synchronous method call from this object
			 *
			 * The MsgArgs passed in for args must be fully initilized and the MsgArgs.Length must match
			 * the number of arguments required for the Method being invoked.
			 *
			 * If the Method being invoked has no input arguments use MsgArgs.Zero for the args parameter.
			 *
			 * @param ifaceName		Name of the interface being used.
			 * @param methodName       Method being invoked.
			 * @param args         The arguments for the method call
			 * @param replyMsg     The reply message received for the method call
			 * @param timeout      Timeout specified in milliseconds to wait for a reply
			 * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
			 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
			 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
			 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
			 *
			 *
			 * @return
			 *      - QStatus.OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
			 *      - QStatus.BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
			 */
			[Obsolete("Usage of MethodCallSynch that takes MsgArgs been depricated. Please use MsgArg inplace of MsgArgs or MethodCall")]
			public QStatus MethodCallSynch(string ifaceName, string methodName, MsgArgs args, Message replyMsg,
				uint timeout, byte flags)
			{
				return alljoyn_proxybusobject_methodcall(_proxyBusObject, ifaceName, methodName, args.UnmanagedPtr,
					(UIntPtr)args.Length, replyMsg.UnmanagedPtr, timeout, flags);
			}

			/**
			 * Make a synchronous method call from this object
			 *
			 * The MsgArg passed in for args must be fully initilized and the MsgArg.Length must match
			 * the number of arguments required for the Method being invoked.
			 *
			 * If the Method being invoked has no input arguments use MsgArg.Zero for the args parameter.
			 *
			 * @param ifaceName    Name of the interface being used.
			 * @param methodName   Method being invoked.
			 * @param args         The arguments for the method call
			 * @param replyMsg     The reply message received for the method call
			 * @param timeout      Timeout specified in milliseconds to wait for a reply
			 * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
			 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
			 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
			 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
			 *
			 *
			 * @return
			 *      - QStatus.OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
			 *      - QStatus.BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
			 */
			[Obsolete("Usage of MethodCallSynch has been replaced with MethodCall.")]
			public QStatus MethodCallSynch(string ifaceName, string methodName, MsgArg args, Message replyMsg,
				uint timeout, byte flags)
			{
				return alljoyn_proxybusobject_methodcall(_proxyBusObject, ifaceName, methodName, args.UnmanagedPtr,
					(UIntPtr)args.Length, replyMsg.UnmanagedPtr, timeout, flags);
			}

			/**
			 * Make a synchronous method call from this object
			 *
			 * The MsgArg passed in for args must be fully initilized and the MsgArg.Length must match
			 * the number of arguments required for the Method being invoked.
			 *
			 * If the Method being invoked has no input arguments use MsgArg.Zero for the args parameter.
			 *
			 * @param ifaceName    Name of the interface being used.
			 * @param methodName   Method being invoked.
			 * @param args         The arguments for the method call.
			 * @param replyMsg     The reply message received for the method call
			 * @param timeout      Timeout specified in milliseconds to wait for a reply
			 * @param flags        Logical OR of the message flags for this method call. The following flags apply to method calls:
			 *                     - If ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is encrypted.
			 *                     - If ALLJOYN_FLAG_COMPRESSED is set the header is compressed for destinations that can handle header compression.
			 *                     - If ALLJOYN_FLAG_AUTO_START is set the bus will attempt to start a service if it is not running.
			 *
			 *
			 * @return
			 *      - QStatus.OK if the method call succeeded and the reply message type is MESSAGE_METHOD_RET
			 *      - QStatus.BUS_REPLY_IS_ERROR_MESSAGE if the reply message type is MESSAGE_ERROR
			 */
			public QStatus MethodCall(string ifaceName, string methodName, MsgArg args, Message replyMsg,
				uint timeout, byte flags)
			{
				return alljoyn_proxybusobject_methodcall(_proxyBusObject, ifaceName, methodName, args.UnmanagedPtr,
					(UIntPtr)args.Length, replyMsg.UnmanagedPtr, timeout, flags);
			}

			/**
			 * Returns an interface description. Returns NULL if the object does not implement
			 * the requested interface.
			 *
			 * @param iface  The name of interface to get.
			 *
			 * @return
			 *      - The requested interface description.
			 *      - NULL if requested interface is not implemented or not found
			 */
			public InterfaceDescription GetInterface(string iface)
			{
				IntPtr infacePtr = alljoyn_proxybusobject_getinterface(_proxyBusObject, iface);
				if (infacePtr == IntPtr.Zero)
				{
					return null;
				}
				else
				{
					return new InterfaceDescription(infacePtr);
				}
			}

			/**
			 * Tests if this object implements the requested interface.
			 *
			 * @param iface  Name of the interface to check for
			 *
			 * @return  true if the object implements the requested interface
			 */
			public bool ImplementsInterface(string iface)
			{
				return alljoyn_proxybusobject_implementsinterface(_proxyBusObject, iface);
			}

			/**
			 * Initialize this proxy object from an XML string. Calling this method does several things:
			 *
			 *  -# Create and register any new InterfaceDescription(s) that are mentioned in the XML.
			 *     (Interfaces that are already registered with the bus are left "as-is".)
			 *  -# Add all the interfaces mentioned in the introspection data to this ProxyBusObject.
			 *  -# Recursively create any child ProxyBusObject(s) and create/add their associated @n
			 *     interfaces as mentioned in the XML. Then add the descendant object(s) to the appropriate
			 *     descendant of this ProxyBusObject (in the children collection). If the named child object
			 *     already exists as a child of the appropriate ProxyBusObject, then it is updated
			 *     to include any new interfaces or children mentioned in the XML.
			 *
			 * Note that when this method fails during parsing, the return code will be set accordingly.
			 * However, any interfaces which were successfully parsed prior to the failure
			 * may be registered with the bus. Similarly, any objects that were successfully created
			 * before the failure will exist in this object's set of children.
			 *
			 * @param xml         An XML string in DBus introspection format.
			 *
			 * @return
			 *      - QStatus.OK if parsing is completely successful.
			 *      - An error status otherwise.
			 */
			public QStatus ParseXml(string xml)
			{
				return alljoyn_proxybusobject_parsexml(_proxyBusObject, xml, null);
			}

			/**
			 * Initialize this proxy object from an XML string. Calling this method does several things:
			 *
			 *  -# Create and register any new InterfaceDescription(s) that are mentioned in the XML.
			 *     (Interfaces that are already registered with the bus are left "as-is".)
			 *  -# Add all the interfaces mentioned in the introspection data to this ProxyBusObject.
			 *  -# Recursively create any child ProxyBusObject(s) and create/add their associated @n
			 *     interfaces as mentioned in the XML. Then add the descendant object(s) to the appropriate
			 *     descendant of this ProxyBusObject (in the children collection). If the named child object
			 *     already exists as a child of the appropriate ProxyBusObject, then it is updated
			 *     to include any new interfaces or children mentioned in the XML.
			 *
			 * Note that when this method fails during parsing, the return code will be set accordingly.
			 * However, any interfaces which were successfully parsed prior to the failure
			 * may be registered with the bus. Similarly, any objects that were successfully created
			 * before the failure will exist in this object's set of children.
			 *
			 * @param xml         An XML string in DBus introspection format.
			 * @param identifier  An optional identifying string to include in error logging messages.
			 *
			 * @return
			 *      - QStatus.OK if parsing is completely successful.
			 *      - An error status otherwise.
			 */
			public QStatus ParseXml(string xml, string identifier)
			{
				return alljoyn_proxybusobject_parsexml(_proxyBusObject, xml, identifier);
			}

			/**
			 * Explicitly secure the connection to the remote peer for this proxy object. Peer-to-peer
			 * connections can only be secured if EnablePeerSecurity() was previously called on the bus
			 * attachment for this proxy object. If the peer-to-peer connection is already secure this
			 * function does nothing. Note that peer-to-peer connections are automatically secured when a
			 * method call requiring encryption is sent.
			 *
			 * This call causes messages to be send on the bus, therefore it cannot be called within AllJoyn
			 * callbacks (method/signal/reply handlers or ObjectRegistered callbacks, etc.)
			 *
			 * @return
			 *          - QStatus.OK if the connection was secured or an error status indicating that the
			 *            connection could not be secured.
			 *          - QStatus.BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment.EnablePeerSecurity() has not been called.
			 *          - QStatus.AUTH_FAIL if the attempt(s) to authenticate the peer failed.
			 *          - Other error status codes indicating a failure.
			 */
			public QStatus SecureConnection()
			{
				return SecureConnection(false);
			}

			/**
			 * Explicitly secure the connection to the remote peer for this proxy object. Peer-to-peer
			 * connections can only be secured if EnablePeerSecurity() was previously called on the bus
			 * attachment for this proxy object. If the peer-to-peer connection is already secure this
			 * function does nothing. Note that peer-to-peer connections are automatically secured when a
			 * method call requiring encryption is sent.
			 *
			 * This call causes messages to be send on the bus, therefore it cannot be called within AllJoyn
			 * callbacks (method/signal/reply handlers or ObjectRegistered callbacks, etc.)
			 *
			 * @param forceAuth  If true, forces an re-authentication even if the peer connection is already
			 *                   authenticated.
			 *
			 * @return
			 *          - QStatus.OK if the connection was secured or an error status indicating that the
			 *            connection could not be secured.
			 *          - QStatus.BUS_NO_AUTHENTICATION_MECHANISM if BusAttachment.EnablePeerSecurity() has not been called.
			 *          - QStatus.AUTH_FAIL if the attempt(s) to authenticate the peer failed.
			 *          - Other error status codes indicating a failure.
			 */
			public QStatus SecureConnection(bool forceAuth)
			{
				return alljoyn_proxybusobject_secureconnection(_proxyBusObject, forceAuth);
			}
			#region Properties

			/**
			 * Return the absolute object path for the remote object.
			 *
			 * @return Object path
			 */
			public string Path
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_proxybusobject_getpath(_proxyBusObject));
				}
			}

			/**
			 * Return the remote service name for this object.
			 *
			 * @return Service name (typically a well-known service name but may be a unique name)
			 */
			public string ServiceName
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_proxybusobject_getservicename(_proxyBusObject));
				}
			}

			/**
			 * Return the session Id for this object.
			 *
			 * @return Session Id
			 */
			public uint SessionId
			{
				get
				{
					return alljoyn_proxybusobject_getsessionid(_proxyBusObject);
				}
			}

			/**
			 * Indicates if this is a valid (usable) proxy bus object.
			 *
			 * @return true if a valid proxy bus object, false otherwise.
			 */
			public bool IsValid
			{
				get
				{
					return alljoyn_proxybusobject_isvalid(_proxyBusObject);
				}
			}

			/**
			 * Indicates if the remote object for this proxy bus object is secure.
			 *
			 * @return  true if the object is secure
			 */
			public bool IsSecure
			{
				get
				{
					return alljoyn_proxybusobject_issecure(_proxyBusObject);
				}
			}
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_proxybusobject_create(IntPtr proxyObj,
				[MarshalAs(UnmanagedType.LPStr)] string service,
				[MarshalAs(UnmanagedType.LPStr)] string path,
				uint sessionId);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_proxybusobject_create_secure(IntPtr proxyObj,
				[MarshalAs(UnmanagedType.LPStr)] string service,
				[MarshalAs(UnmanagedType.LPStr)] string path,
				uint sessionId);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_proxybusobject_destroy(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_proxybusobject_addinterface(IntPtr proxyObj, IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_proxybusobject_addinterface_by_name(IntPtr proxyObj, [MarshalAs(UnmanagedType.LPStr)] string ifaceName);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_proxybusobject_introspectremoteobject(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_proxybusobject_methodcall(IntPtr proxyObj,
				[MarshalAs(UnmanagedType.LPStr)] string ifaceName,
				[MarshalAs(UnmanagedType.LPStr)] string methodName,
				IntPtr args,
				UIntPtr numArgs,
				IntPtr replyMsg,
				uint timeout,
				byte flags);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_proxybusobject_getinterface(IntPtr proxyObj, [MarshalAs(UnmanagedType.LPStr)] string iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_proxybusobject_getpath(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_proxybusobject_getservicename(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern uint alljoyn_proxybusobject_getsessionid(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private static extern bool alljoyn_proxybusobject_implementsinterface(IntPtr proxyObj, [MarshalAs(UnmanagedType.LPStr)] string iface);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private static extern bool alljoyn_proxybusobject_isvalid(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private static extern bool alljoyn_proxybusobject_issecure(IntPtr proxyObj);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_proxybusobject_parsexml(IntPtr proxyObj,
				[MarshalAs(UnmanagedType.LPStr)] string xml,
				[MarshalAs(UnmanagedType.LPStr)] string identifier);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_proxybusobject_secureconnection(IntPtr proxyObj, [MarshalAs(UnmanagedType.I1)] bool forceAuth);

			#endregion

			#region IDisposable
			~ProxyBusObject()
			{
				Dispose(false);
			}

			/**
			 * Dispose the ProxyBusObject
			 * @param disposing  describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if (!_isDisposed)
				{
					alljoyn_proxybusobject_destroy(_proxyBusObject);
					_proxyBusObject = IntPtr.Zero;
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the ProxyBusObject
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region Data
			IntPtr _proxyBusObject;
			bool _isDisposed = false;
			#endregion
		}
	}
}
