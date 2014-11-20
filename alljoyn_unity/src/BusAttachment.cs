/**
 * @file BusAttachment.cs is the top-level object responsible for connecting to a
 * message bus.
 */

/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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
		 * %BusAttachment is the top-level object responsible for connecting to and optionally managing a message bus.
		 */
		public class BusAttachment : IDisposable
		{
			/**
			 * Construct a BusAttachment.
			 *
			 * @param applicationName      Name of the application.
			 * @param allowRemoteMessages  True if this attachment is allowed to receive messages from remote devices.
			 */
			public BusAttachment(string applicationName, bool allowRemoteMessages)
			{
				SetMainThreadOnlyCallbacks(true);
				_busAttachment = alljoyn_busattachment_create(applicationName, allowRemoteMessages ? 1 : 0);

				_signalHandlerDelegateRefHolder = new Dictionary<SignalHandler, InternalSignalHandler>();
				if (_sBusAttachmentMap == null) _sBusAttachmentMap = new Dictionary<IntPtr, BusAttachment>();
				_sBusAttachmentMap.Add(_busAttachment, this);
			}

			/**
			 * Construct a BusAttachment.
			 *
			 * @param applicationName      Name of the application.
			 * @param allowRemoteMessages  True if this attachment is allowed to receive messages from remote devices.
			 * @param concurrency          The maximum number of concurrent method and signal handlers locally executing.
			 */
			public BusAttachment(string applicationName, bool allowRemoteMessages, uint concurrency)
			{
				SetMainThreadOnlyCallbacks(true);
				_busAttachment = alljoyn_busattachment_create_concurrency(applicationName, allowRemoteMessages ? 1 : 0, concurrency);

				_signalHandlerDelegateRefHolder = new Dictionary<SignalHandler, InternalSignalHandler>();
				if (_sBusAttachmentMap == null) _sBusAttachmentMap = new Dictionary<IntPtr, BusAttachment>();
				_sBusAttachmentMap.Add(_busAttachment, this);
			}

			/**
			 * Request the raw pointer of the AllJoyn C BusAttachment
			 *
			 * @return the raw pointer of the AllJoyn C BusAttachment
			 */
			public IntPtr getAddr()
			{
				return _busAttachment;
			}

			/**
			 * Create an interface description with a given name.
			 *
			 * Typically, interfaces that are implemented by BusObjects are created here.
			 * Interfaces that are implemented by remote objects are added automatically by
			 * the bus if they are not already present via ProxyBusObject.IntrospectRemoteObject().
			 *
			 * Because interfaces are added both explicitly (via this method) and implicitly
			 * (via @c ProxyBusObject.IntrospectRemoteObject), there is the possibility that creating
			 * an interface here will fail because the interface already exists. If this happens, the
			 * QStatus.BUS_IFACE_ALREADY_EXISTS will be returned and NULL will be returned in the iface [OUT]
			 * parameter.
			 *
			 * Interfaces created with this method need to be activated using InterfaceDescription.Activate()
			 * once all of the methods, signals, etc have been added to the interface. The interface will
			 * be unaccessible (via BusAttachment.GetInterfaces() or BusAttachment.GetInterface()) until
			 * it is activated.
			 *
			 * Interfaces created using this Method will have an interface security policy of 'inherit'.
			 *
			 * @param interfaceName   The requested interface name.
			 * @param[out] iface
			 *      - Interface description
			 *      - NULL if cannot be created.
			 *
			 * @return
			 *      - QStatus.OK if creation was successful.
			 *      - QStatus.BUS_IFACE_ALREADY_EXISTS if requested interface already exists
			 * @see ProxyBusObject.IntrospectRemoteObject, InterfaceDescription.Activate, BusAttachment.GetInterface
			 */
			public QStatus CreateInterface(string interfaceName, out InterfaceDescription iface)
			{
				IntPtr interfaceDescription = new IntPtr();
				int qstatus = alljoyn_busattachment_createinterface_secure(_busAttachment, interfaceName,
					ref interfaceDescription,
					InterfaceDescription.SecurityPolicy.Inherit);
				if (qstatus == 0)
				{
					iface = new InterfaceDescription(interfaceDescription);
				}
				else
				{
					iface = null;
				}
				return qstatus;
			}

			/**
			 * Create an interface description with a given name.
			 *
			 * Typically, interfaces that are implemented by BusObjects are created here.
			 * Interfaces that are implemented by remote objects are added automatically by
			 * the bus if they are not already present via ProxyBusObject.IntrospectRemoteObject().
			 *
			 * Because interfaces are added both explicitly (via this method) and implicitly
			 * (via @c ProxyBusObject.IntrospectRemoteObject), there is the possibility that creating
			 * an interface here will fail because the interface already exists. If this happens, the
			 * QStatus.BUS_IFACE_ALREADY_EXISTS will be returned and NULL will be returned in the iface [OUT]
			 * parameter.
			 *
			 * Interfaces created with this method need to be activated using InterfaceDescription.Activate()
			 * once all of the methods, signals, etc have been added to the interface. The interface will
			 * be unaccessible (via BusAttachment.GetInterfaces() or BusAttachment.GetInterface()) until
			 * it is activated.
			 *
			 * @param interfaceName   The requested interface name.
			 * @param secure If true the interface is secure and method calls and signals will be encrypted.
			 * @param[out] iface
			 *      - Interface description
			 *      - NULL if cannot be created.
			 *
			 * @return
			 *      - QStatus.OK if creation was successful.
			 *      - QStatus.BUS_IFACE_ALREADY_EXISTS if requested interface already exists
			 * @see ProxyBusObject.IntrospectRemoteObject, InterfaceDescription.Activate, BusAttachment.GetInterface
			 */
			[Obsolete("Please us the CreateInterface function that takes an Interface SecurityPolicy")]
			public QStatus CreateInterface(string interfaceName, bool secure, out InterfaceDescription iface)
			{
				IntPtr interfaceDescription = new IntPtr();
				int qstatus = 1;
				if (secure == true)
				{
					qstatus = alljoyn_busattachment_createinterface_secure(_busAttachment,
						interfaceName, ref interfaceDescription, InterfaceDescription.SecurityPolicy.Required);
				}
				else
				{
					qstatus = alljoyn_busattachment_createinterface(_busAttachment,
						interfaceName, ref interfaceDescription);
				}
				if (qstatus == 0)
				{
					iface = new InterfaceDescription(interfaceDescription);
				}
				else
				{
					iface = null;
				}
				return qstatus;
			}

			/**
			 * Create an interface description with a given name.
			 *
			 * Typically, interfaces that are implemented by BusObjects are created here.
			 * Interfaces that are implemented by remote objects are added automatically by
			 * the bus if they are not already present via ProxyBusObject.IntrospectRemoteObject().
			 *
			 * Because interfaces are added both explicitly (via this method) and implicitly
			 * (via @c ProxyBusObject.IntrospectRemoteObject), there is the possibility that creating
			 * an interface here will fail because the interface already exists. If this happens, the
			 * QStatus.BUS_IFACE_ALREADY_EXISTS will be returned and NULL will be returned in the iface [OUT]
			 * parameter.
			 *
			 * Interfaces created with this method need to be activated using InterfaceDescription.Activate()
			 * once all of the methods, signals, etc have been added to the interface. The interface will
			 * be unaccessible (via BusAttachment.GetInterfaces() or BusAttachment.GetInterface()) until
			 * it is activated.
			 *
			 * @param interfaceName   The requested interface name.
			 * @param secPolicy
			 *                  security policy of this interface it can be:
			 *                   - InterfaceDescription.SecurityPolicy.Inherit
			 *                   - InterfaceDescription.SecurityPolicy.Required
			 *                   - InterfaceDescription.SecurityPolicy.Off
			 *                  If SecurityPolicy.Required the interface is secure and
			 *                  method calls and signals will be encrypted.
			 *                  If SecurityPolicy.Inherit the interface will gain the
			 *                  security level of the BusObject implementing the interface
			 *                  IF SecurityPolicy.Off authentication is not applicable to
			 *                  the interface.
			 * @param[out] iface
			 *      - Interface description
			 *      - NULL if cannot be created.
			 *
			 * @return
			 *      - QStatus.OK if creation was successful.
			 *      - QStatus.BUS_IFACE_ALREADY_EXISTS if requested interface already exists
			 * @see ProxyBusObject.IntrospectRemoteObject, InterfaceDescription.Activate, BusAttachment.GetInterface
			 */
			public QStatus CreateInterface(string interfaceName, InterfaceDescription.SecurityPolicy secPolicy, out InterfaceDescription iface)
			{
				IntPtr interfaceDescription = new IntPtr();
				int qstatus = alljoyn_busattachment_createinterface_secure(_busAttachment, interfaceName, ref interfaceDescription, secPolicy);
				if (qstatus == 0)
				{
					iface = new InterfaceDescription(interfaceDescription);
				}
				else
				{
					iface = null;
				}
				return qstatus;
			}

			/**
			 * @brief Start the process of spinning up the independent threads used in
			 * the bus attachment, preparing it for action.
			 *
			 * This method only begins the process of starting the bus. Sending and
			 * receiving messages cannot begin until the bus is Connect()ed.
			 *
			 * In most cases, it is not required to understand the threading model of
			 * the bus attachment, with one important exception: The bus attachment may
			 * send callbacks to registered listeners using its own internal threads.
			 * This means that any time a listener of any kind is used in a program, the
			 * implication is that a the overall program is multithreaded, irrespective
			 * of whether or not threads are explicitly used.  This, in turn, means that
			 * any time shared state is accessed in listener methods, that state must be
			 * protected.
			 *
			 * As soon as Start() is called, clients of a bus attachment with listeners
			 * must be prepared to receive callbacks on those listeners in the context
			 * of a thread that will be different from the thread running the main
			 * program or any other thread in the client.
			 *
			 * Although intimate knowledge of the details of the threading model are not
			 * required to use a bus attachment (beyond the caveat above) we do provide
			 * methods on the bus attachment that help users reason about more complex
			 * threading situations.  This will apply to situations where clients of the
			 * bus attachment are multithreaded and need to interact with the
			 * multithreaded bus attachment.  These methods can be especially useful
			 * during shutdown, when the two separate threading systems need to be
			 * gracefully brought down together.
			 *
			 * The BusAttachment methods Start(), Stop() and Join() all work together to
			 * manage the autonomous activities that can happen in a BusAttachment.
			 * These activities are carried out by so-called hardware threads.  POSIX
			 * defines functions used to control hardware threads, which it calls
			 * pthreads.  Many threading packages use similar constructs.
			 *
			 * In a threading package, a start method asks the underlying system to
			 * arrange for the start of thread execution.  Threads are not necessarily
			 * running when the start method returns, but they are being *started*.  Some time later,
			 * a thread of execution appears in a thread run function, at which point the
			 * thread is considered *running*.  At some later time, executing a stop method asks the
			 * underlying system to arrange for a thread to end its execution.  The system
			 * typically sends a message to the thread to ask it to stop doing what it is doing.
			 * The thread is running until it responds to the stop message, at which time the
			 * run method exits and the thread is considered *stopping*.
			 *
			 * Note that neither of Start() nor Stop() are synchronous in the sense that
			 * one has actually accomplished the desired effect upon the return from a
			 * call.  Of particular interest is the fact that after a call to Stop(),
			 * threads will still be *running* for some non-deterministic time.
			 *
			 * In order to wait until all of the threads have actually stopped, a
			 * blocking call is required.  In threading packages this is typically
			 * called join, and our corresponding method is called Join().
			 *
			 * A Start() method call should be thought of as mapping to a threading
			 * package start function.  it causes the activity threads in the
			 * BusAttachment to be spun up and gets the attachment ready to do its main
			 * job.  As soon as Start() is called, the user should be prepared for one
			 * or more of these threads of execution to pop out of the bus attachment
			 * and into a listener callback.
			 *
			 * The Stop() method call should be thought of as mapping to a threading
			 * package stop function.  It asks the BusAttachment to begin shutting down
			 * its various threads of execution, but does not wait for any threads to exit.
			 *
			 * A call to the Join() method should be thought of as mapping to a
			 * threading package join function call.  It blocks and waits until all of
			 * the threads in the BusAttachment have in fact exited their Run functions,
			 * gone through the stopping state and have returned their status.  When
			 * the Join() method returns, one may be assured that no threads are running
			 * in the bus attachment, and therefore there will be no callbacks in
			 * progress and no further callbacks will ever come out of a particular
			 * instance of a bus attachment.
			 *
			 * It is important to understand that since Start(), Stop() and Join() map
			 * to threads concepts and functions, one should not expect them to clean up
			 * any bus attachment state when they are called.  These functions are only
			 * present to help in orderly termination of complex threading systems.
			 *
			 * @see Stop()
			 * @see Join()
			 *
			 * @return
			 *      - QStatus.OK if successful.
			 *      - QStatus.BUS_BUS_ALREADY_STARTED if already started
			 *      - Other error status codes indicating a failure
			 */
			public QStatus Start()
			{
				return alljoyn_busattachment_start(_busAttachment);
			}

			/**
			 * @brief Ask the threading subsystem in the bus attachment to begin the
			 * process of ending the execution of its threads.
			 *
			 * The Stop() method call on a bus attachment should be thought of as
			 * mapping to a threading package stop function.  It asks the BusAttachment
			 * to begin shutting down its various threads of execution, but does not
			 * wait for any threads to exit.
			 *
			 * A call to Stop() is implied as one of the first steps in the destruction
			 * of a bus attachment.
			 *
			 * @warning There is no guarantee that a listener callback may begin executing
			 * after a call to Stop().  To achieve that effect, the Stop() must be followed
			 * by a Join().
			 *
			 * @see Start()
			 * @see Join()
			 *
			 * @return
			 *     - QStatus.OK if successful.
			 *     - An error QStatus if unable to begin the process of stopping the
			 *       message bus threads.
			 */
			public QStatus Stop()
			{
				return alljoyn_busattachment_stop(_busAttachment);
			}

			/**
			 * Wait for all of the threads spawned by the bus attachment to be completely 
			 * exited.
			 *
			 * A call to the Join() method should be thought of as mapping to a
			 * threading package join function call.  It blocks and waits until all of
			 * the threads in the BusAttachment have, in fact, exited their Run functions,
			 * gone through the stopping state and have returned their status.  When
			 * the Join() method returns, one may be assured that no threads are running
			 * in the bus attachment, and therefore there will be no callbacks in
			 * progress and no further callbacks will ever come out of the instance of a
			 * bus attachment on which Join() was called.
			 *
			 * A call to Join() is implied as one of the first steps in the destruction
			 * of a bus attachment.  Thus, when a bus attachment is destroyed, it is
			 * guaranteed that before it completes its destruction process, there will be
			 * no callbacks in process.
			 *
			 * @warning If Join() is called without a previous Stop() it will result in
			 * blocking "forever."
			 *
			 * @see Start()
			 * @see Stop()
			 *
			 * @return
			 *      - QStatus.OK if successful.
			 *      - QStatus.BUS_BUS_ALREADY_STARTED if already started
			 *      - Other error status codes indicating a failure
			 */
			public QStatus Join()
			{
				return alljoyn_busattachment_join(_busAttachment);
			}

			/**
			 * Connect to a remote bus address.
			 *
			 * @return
			 *      - QStatus.OK if successful.
			 *      - An error status otherwise
			 */
			public QStatus Connect()
			{
				StartAllJoynCallbackProcessing();
				return alljoyn_busattachment_connect(_busAttachment, IntPtr.Zero);
			}

			/**
			 * Connect to a remote bus address.
			 *
			 * @param connectSpec  A transport connection spec string of the form:
			 *                     @c "<transport>:<param1>=<value1>,<param2>=<value2>...[;]"
			 * @return
			 *      - QStatus.OK if successful.
			 *      - An error status otherwise
			 */
			public QStatus Connect(string connectSpec)
			{
				StartAllJoynCallbackProcessing();
				return alljoyn_busattachment_connect(_busAttachment, connectSpec);
			}

			/**
			 * Allow the currently executing method/signal handler to enable concurrent
			 * callbacks during the scope of the handler's execution.
			 */
			public void EnableConcurrentCallbacks()
			{
				alljoyn_busattachment_enableconcurrentcallbacks(_busAttachment);
			}

			/**
			 * Register an object that will receive bus event notifications.
			 *
			 * @param listener  Object instance that will receive bus event notifications.
			 */
			public void RegisterBusListener(BusListener listener)
			{
				alljoyn_busattachment_registerbuslistener(_busAttachment, listener.UnmanagedPtr);
			}

			/**
			 * Unregister an object that was previously registered with RegisterBusListener.
			 *
			 * @param listener  Object instance to un-register as a listener.
			 */
			public void UnregisterBusListener(BusListener listener)
			{
				alljoyn_busattachment_unregisterbuslistener(_busAttachment, listener.UnmanagedPtr);
			}

			/**
			 * Register interest in a well-known name prefix for the purpose of discovery over transports included in TRANSPORT_ANY.
			 * This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedName method call to the local daemon
			 * and interprets the response.
			 *
			 * @param[in]  namePrefix    Well-known name prefix that application is interested in receiving
			 *                           BusListener.FoundAdvertisedName notifications about.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and discovery was successfully started.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus FindAdvertisedName(string namePrefix)
			{
				return alljoyn_busattachment_findadvertisedname(_busAttachment, namePrefix);
			}
			/**
			 * Register interest in a well-known name prefix for the purpose of discovery over specified transports.
			 * This method is a shortcut/helper that issues an org.alljoyn.Bus.FindAdvertisedNameByTransport
			 * method call to the local daemon and interprets the response.
			 *
			 * @param[in]  namePrefix    Well-known name prefix that application is interested in receiving
			 *                           BusListener.FoundAdvertisedName notifications about.
			 * @param[in]  transports    Set of transports to use for discovery.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and discovery was successfully started.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus FindAdvertisedNameByTransport(string namePrefix, TransportMask transports)
			{
				return alljoyn_busattachment_findadvertisednamebytransport(_busAttachment, namePrefix, (ushort)transports);
			}
			/**
			 * Cancel interest in a well-known name prefix that was previously
			 * registered with FindAdvertisedName over transports included in TRANSPORT_ANY.
			 * This method is a shortcut/helper
			 * that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
			 * call to the local daemon and interprets the response.
			 *
			 * @param[in]  namePrefix    Well-known name prefix that application is no longer interested in receiving
			 *                           BusListener.FoundAdvertisedName notifications about.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and cancel was successfully completed.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus CancelFindAdvertisedName(string namePrefix)
			{
				return alljoyn_busattachment_cancelfindadvertisedname(_busAttachment, namePrefix);
			}

			/**
			 * Cancel interest in a well-known name prefix that was previously
			 * registered with FindAdvertisedName. This cancels well-known name discovery over
			 * the specified transports. This method is a shortcut/helper
			 * that issues an org.alljoyn.Bus.CancelFindAdvertisedName method
			 * call to the local daemon and interprets the response.
			 *
			 * @param[in]  namePrefix    Well-known name prefix that application is no longer interested in receiving
			 *                           BusListener.FoundAdvertisedName notifications about.
			 * @param[in]  transports    Transports over which to cancel well-known name discovery
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and cancel was successfully completed.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus CancelFindAdvertisedNameByTransport(string namePrefix, TransportMask transports)
			{
				return alljoyn_busattachment_cancelfindadvertisednamebytransport(_busAttachment, namePrefix, (ushort)transports);
			}

			/**
			 * Join a session.
			 * This method is a shortcut/helper that issues an org.alljoyn.Bus.JoinSession method call to
			 * the local daemon and interprets the response.
			 *
			 * @param[in]  sessionHost      Bus name of attachment that is hosting the session to be joined.
			 * @param[in]  sessionPort      SessionPort of sessionHost to be joined.
			 * @param[in]  listener         Optional listener called when session related events occur. May be NULL.
			 * @param[out] sessionId        Unique identifier for session.
			 * @param[in] opts          Session options.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and the session was successfully joined.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus JoinSession(string sessionHost, ushort sessionPort, SessionListener listener,
				out uint sessionId, SessionOpts opts)
			{
				IntPtr optsPtr = opts.UnmanagedPtr;
				uint sessionId_out = 0;
				int qstatus = 0;
				Thread joinSessionThread = new Thread((object o) =>
				{
					qstatus = alljoyn_busattachment_joinsession(_busAttachment, sessionHost, sessionPort,
						(listener == null ? IntPtr.Zero : listener.UnmanagedPtr), ref sessionId_out, optsPtr);
				});
				joinSessionThread.Start();
				while (joinSessionThread.IsAlive)
				{
					AllJoyn.TriggerCallbacks();
					Thread.Sleep(0);
				}
				sessionId = sessionId_out;
				return qstatus;
			}

			/**
			 * Advertise the existence of a well-known name to other (possibly disconnected) AllJoyn daemons.
			 *
			 * This method is a shortcut/helper that issues an org.alljoyn.Bus.AdvertisedName method call
			 * to the local daemon and interprets the response.
			 *
			 * @param[in]  name          the well-known name to advertise. (Must be owned by the caller via RequestName).
			 * @param[in]  transports    Set of transports to use for sending advertisement.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and advertise was successful.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus AdvertiseName(string name, TransportMask transports)
			{
				return alljoyn_busattachment_advertisename(_busAttachment, name, (ushort)transports);
			}

			/**
			 * Stop advertising the existence of a well-known name to other AllJoyn daemons.
			 *
			 * This method is a shortcut/helper that issues an org.alljoyn.Bus.CancelAdvertiseName method call
			 * to the local daemon and interprets the response.
			 *
			 * @param[in]  name          A well-known name that was previously advertised via AdvertiseName.
			 * @param[in]  transports    Set of transports whose name advertisement will be canceled.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and advertisements were successfully stopped.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus CancelAdvertisedName(string name, TransportMask transports)
			{
				return alljoyn_busattachment_canceladvertisename(_busAttachment, name, (ushort)transports);
			}

			/**
			 * Retrieve an existing activated InterfaceDescription.
			 *
			 * @param name       Interface name
			 *
			 * @return
			 *      - A pointer to the registered interface
			 *      - NULL if interface doesn't exist
			 */
			public InterfaceDescription GetInterface(string name)
			{
				IntPtr iface = alljoyn_busattachment_getinterface(_busAttachment, name);
				InterfaceDescription ret = (iface != IntPtr.Zero ? new InterfaceDescription(iface) : null);
				return ret;
			}

			/**
			 * Register a BusObject.
			 *
			 * @param obj      BusObject to register.
			 *
			 * @return
			 *      - QStatus.OK if successful.
			 *      - QStatus.BUS_BAD_OBJ_PATH for a bad object path
			 */
			public QStatus RegisterBusObject(BusObject obj)
			{
				return alljoyn_busattachment_registerbusobject(_busAttachment, obj.UnmanagedPtr);
			}

			/**
			 * Register a BusObject
			 *
			 * @param obj      BusObject to register.
			 * @param secure   If true objects registered will require authentication
			 *                 unless the interfaces security policy is 'Off'
			 *
			 * @return
			 *      - QStatus.OK if successful.
			 *      - QStatus.BUS_BAD_OBJ_PATH for a bad object path
			 */
			public QStatus RegisterBusObject(BusObject obj, bool secure)
			{
				if (secure)
				{
					return alljoyn_busattachment_registerbusobject_secure(_busAttachment, obj.UnmanagedPtr);
				}
				else
				{
					return alljoyn_busattachment_registerbusobject(_busAttachment, obj.UnmanagedPtr);
				}
			}

			/**
			 * Unregister a BusObject
			 *
			 * @param obj  Object to be unregistered.
			 */
			public void UnregisterBusObject(BusObject obj)
			{
				alljoyn_busattachment_unregisterbusobject(_busAttachment, obj.UnmanagedPtr);
			}

			/**
			 * Register a signal handler.
			 *
			 * Signals are forwarded to the signalHandler if sender, interface, member and path
			 * qualifiers are ALL met.
			 *
			 * @param handler  The signal handler method.
			 * @param member         The interface/member of the signal.
			 * @param srcPath        The object path of the emitter of the signal or NULL for all paths.
			 * @return QStatus.OK
			 */
			public QStatus RegisterSignalHandler(SignalHandler handler,
				InterfaceDescription.Member member, string srcPath)
			{
				InternalSignalHandler internalSignalHandler = (IntPtr m, IntPtr s, IntPtr msg) =>
				{
					SignalHandler h = handler;
					h(new InterfaceDescription.Member(m), Marshal.PtrToStringAnsi(s), new Message(msg));
				};
				_signalHandlerDelegateRefHolder.Add(handler, internalSignalHandler);

				QStatus ret = alljoyn_busattachment_registersignalhandler(_busAttachment,
					Marshal.GetFunctionPointerForDelegate(internalSignalHandler),
					member._member, srcPath);

				return ret;
			}

			/**
			 * Unregister a signal handler.
			 *
			 * Remove the signal handler that was registered with the given parameters.
			 *
			 * @param handler        The signal handler method.
			 * @param member         The interface/member of the signal.
			 * @param srcPath        The object path of the emitter of the signal or NULL for all paths.
			 * @return QStatus.OK
			 */
			public QStatus UnregisterSignalHandler(SignalHandler handler,
				InterfaceDescription.Member member, string srcPath)
			{
				QStatus ret = QStatus.OS_ERROR;
				if (_signalHandlerDelegateRefHolder.ContainsKey(handler))
				{
					ret = alljoyn_busattachment_unregistersignalhandler(_busAttachment,
						Marshal.GetFunctionPointerForDelegate(_signalHandlerDelegateRefHolder[handler]),
						member._member, srcPath);
					_signalHandlerDelegateRefHolder.Remove(handler);
				}
				return ret;
			}

			/**
			 * Unregister all signal and reply handlers for the specified message receiver. This function is
			 * intended to be called from within the destructor of a MessageReceiver class instance. It
			 * prevents any pending signals or replies from accessing the MessageReceiver after the message
			 * receiver has been freed.
			 *
			 * @return QStatus.OK if successful.
			 */
			public QStatus UnregisterAllHandlers()
			{
				return alljoyn_busattachment_unregisterallhandlers(_busAttachment);
			}

			/**
			 * Request a well-known name.
			 * This method is a shortcut/helper that issues an org.freedesktop.DBus.RequestName method
			 * call to the local daemon and interprets the response.
			 *
			 * @param[in]  requestedName  Well-known name being requested.
			 * @param[in]  flags          Bitmask of DBUS_NAME_FLAG_* defines (see DBusStd.h)
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and request was successful.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus RequestName(string requestedName, DBus.NameFlags flags)
			{
				return alljoyn_busattachment_requestname(_busAttachment, requestedName, (uint)flags);
			}

			/**
			 * Release a previously requested well-known name.
			 * This method is a shortcut/helper that issues an org.freedesktop.DBus.ReleaseName method
			 * call to the local daemon and interprets the response.
			 *
			 * @param[in]  requestedName          Well-known name being released.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received amd the name was successfully released.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus ReleaseName(string requestedName)
			{
				return alljoyn_busattachment_releasename(_busAttachment, requestedName);
			}

			/**
			 * Make a SessionPort available for external BusAttachments to join.
			 *
			 * Each BusAttachment binds its own set of SessionPorts. Session joiners use the bound session
			 * port along with the name of the attachment to create a persistent logical connection (called
			 * a Session) with the original BusAttachment.
			 *
			 * A SessionPort and bus name form a unique identifier that BusAttachments use when joining a
			 * session.
			 *
			 * SessionPort values can be pre-arranged between AllJoyn services and their clients (well-known
			 * SessionPorts).
			 *
			 * Once a session is joined using one of the service's well-known SessionPorts, the service may
			 * bind additional SessionPorts (dynamically) and share these SessionPorts with the joiner over
			 * the original session. The joiner can then create additional sessions with the service by
			 * calling JoinSession with these dynamic SessionPort ids.
			 *
			 * @param[in,out] sessionPort      SessionPort value to bind or SESSION_PORT_ANY to allow this method
			 *                                 to choose an available port. On successful return, this value
			 *                                 contains the chosen SessionPort.
			 *
			 * @param[in]     opts             Session options that joiners must agree to in order to
			 *                                 successfully join the session.
			 *
			 * @param[in]     listener  Called by the bus when session related events occur.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and the bind operation was successful.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus BindSessionPort(ref ushort sessionPort, SessionOpts opts, SessionPortListener listener)
			{
				QStatus ret = QStatus.OK;
				ushort otherSessionPort = sessionPort;
				Thread bindThread = new Thread((object o) =>
				{
					ret = alljoyn_busattachment_bindsessionport(_busAttachment, ref otherSessionPort,
						opts.UnmanagedPtr, listener.UnmanagedPtr);
				});
				bindThread.Start();
				while (bindThread.IsAlive)
				{
					AllJoyn.TriggerCallbacks();
					Thread.Sleep(0);
				}
				sessionPort = otherSessionPort;
				return ret;
			}

			/**
			 * Cancel an existing port binding.
			 *
			 * @param[in]   sessionPort    Existing session port to be un-bound.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and the bind operation was successful.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus UnbindSessionPort(ushort sessionPort)
			{
				QStatus ret = QStatus.OK;
				Thread bindThread = new Thread((object o) =>
				{
					ret = alljoyn_busattachment_unbindsessionport(_busAttachment, sessionPort);
				});
				bindThread.Start();
				while (bindThread.IsAlive)
				{
					AllJoyn.TriggerCallbacks();
					Thread.Sleep(0);
				}
				return ret;
			}

			/**
			 * Enable peer-to-peer security. This function must be called by applications that want to use
			 * authentication and encryption . The bus must have been started by calling
			 * BusAttachment.Start() before this function is called. If the application is providing its
			 * own key store implementation it must have already called RegisterKeyStoreListener() before
			 * calling this function.
			 *
             * @param authMechanisms   The authentication mechanism(s) to use for peer-to-peer authentication.
             *                         If this parameter is NULL peer-to-peer authentication is disabled.
             *                         This is a space separated list of any of the following values:
             *                          ALLJOYN_PIN_KEYX, ALLJOYN_SRP_LOGON, ALLJOYN_RSA_KEYX, ALLJOYN_SRP_KEYX, ALLJOYN_ECDHE_NULL, ALLJOYN_ECDHE_PSK, ALLJOYN_ECDHE_ECDSA, GSSAPI.
			 *
			 * @param listener         Passes password and other authentication related requests to the application.
			 *
			 * @param keyStoreFileName Optional parameter to specify the filename of the default key store. The
			 *                         default value is the applicationName parameter of BusAttachment().
			 *                         Note that this parameter is only meaningful when using the default
			 *                         key store implementation.
			 *
			 * @param isShared         optional parameter that indicates if the key store is shared between multiple
			 *                         applications. It is generally harmless to set this to true even when the
			 *                         key store is not shared but it adds some unnecessary calls to the key store
			 *                         listener to load and store the key store in this case.
			 *
			 * @return
			 *      - QStatus.OK if peer security was enabled.
			 *      - QStatus.BUS_BUS_NOT_STARTED BusAttachment.Start has not be called
			 */
			public QStatus EnablePeerSecurity(string authMechanisms, AuthListener listener, string keyStoreFileName, bool isShared)
			{
				return alljoyn_busattachment_enablepeersecurity(_busAttachment,
					authMechanisms, (listener == null ? IntPtr.Zero : listener.UnmanagedPtr),
					keyStoreFileName, (isShared ? 1 : 0));
			}

			/**
			 * Initialize one more interface descriptions from an XML string in DBus introspection format.
			 * The root tag of the XML can be a \<node\> or a stand alone \<interface\> tag. To initialize more
			 * than one interface the interfaces need to be nested in a \<node\> tag.
			 *
			 * Note that when this method fails during parsing, the return code will be set accordingly.
			 * However, any interfaces which were successfully parsed prior to the failure may be registered
			 * with the bus.
			 *
			 * @param xml     An XML string in DBus introspection format.
			 *
			 * @return
			 *      - QStatus.OK if parsing is completely successful.
			 *      - An error status otherwise.
			 */
			public QStatus CreateInterfacesFromXml(string xml)
			{
				return alljoyn_busattachment_createinterfacesfromxml(_busAttachment, xml);
			}

			/**
			 * Returns the existing activated InterfaceDescriptions.
			 *
			 * @return InterfaceDescription array that represents all activated interfaces
			 */
			public InterfaceDescription[] GetInterfaces()
			{

				UIntPtr numIfaces = alljoyn_busattachment_getinterfaces(_busAttachment, IntPtr.Zero, (UIntPtr)0);
				IntPtr[] ifaces = new IntPtr[(int)numIfaces];
				GCHandle gch = GCHandle.Alloc(ifaces, GCHandleType.Pinned);
				UIntPtr numIfacesFilled = alljoyn_busattachment_getinterfaces(_busAttachment,
					gch.AddrOfPinnedObject(), numIfaces);
				gch.Free();
				if (numIfaces != numIfacesFilled)
				{
					// Warn?
				}
				InterfaceDescription[] ret = new InterfaceDescription[(int)numIfacesFilled];
				for (int i = 0; i < ret.Length; i++)
				{
					ret[i] = new InterfaceDescription(ifaces[i]);
				}
				return ret;
			}

			/**
			 * Delete an interface description with a given name.
			 *
			 * Deleting an interface is only allowed if that interface has never been activated.
			 *
			 * @param iface  The un-activated interface to be deleted.
			 *
			 * @return
			 *      - QStatus.OK if deletion was successful
			 *      - QStatus.BUS_NO_SUCH_INTERFACE if interface was not found
			 */
			public QStatus DeleteInterface(InterfaceDescription iface)
			{
				return alljoyn_busattachment_deleteinterface(_busAttachment, iface.UnmanagedPtr);
			}

			/**
			 * Disconnect from the remote bus using the internal ConnectSpec.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_BUS_NOT_STARTED if the bus is not started
			 *      - QStatus.BUS_NOT_CONNECTED if the %BusAttachment is not connected to the bus
			 *      - Other error status codes indicating a failure
			 */
			public QStatus Disconnect()
			{
				return Disconnect(this.ConnectSpec);
			}

			/**
			 * Disconnect a remote bus address connection.
			 *
			 * @param connectSpec  The transport connection spec used to connect.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_BUS_NOT_STARTED if the bus is not started
			 *      - QStatus.BUS_NOT_CONNECTED if the %BusAttachment is not connected to the bus
			 *      - Other error status codes indicating a failure
			 */
			public QStatus Disconnect(string connectSpec)
			{
				return alljoyn_busattachment_disconnect(_busAttachment, connectSpec);
			}

			/**
			 * Set a key store listener to listen for key store load and store requests.
			 * This overrides the internal key store listener.
			 *
			 * @param listener  The key store listener to set.
			 *
			 * @return
			 *      - QStatus.OK if the key store listener was set
			 *      - QStatus.BUS_LISTENER_ALREADY_SET if a listener has been set by this function or because
			 *        EnablePeerSecurity has been called.
			 */
			public QStatus RegisterKeyStoreListener(KeyStoreListener listener)
			{
				return alljoyn_busattachment_registerkeystorelistener(_busAttachment, listener.UnmanagedPtr);
			}

			/**
			 * Reloads the key store for this bus attachment. This function would normally only be called in
			 * the case where a single key store is shared between multiple bus attachments, possibly by different
			 * applications. It is up to the applications to coordinate how and when the shared key store is
			 * modified.
			 *
			 * @return
			 *      - QStatus.OK if the key store was successfully reloaded
			 *      - An error status indicating that the key store reload failed.
			 */
			public QStatus ReloadKeyStore()
			{
				return alljoyn_busattachment_reloadkeystore(_busAttachment);
			}

			/**
			 * Clears all stored keys from the key store. All store keys and authentication information is
			 * deleted and cannot be recovered. Any passwords or other credentials will need to be reentered
			 * when establishing secure peer connections.
			 */
			public void ClearKeyStore()
			{
				alljoyn_busattachment_clearkeystore(_busAttachment);
			}

			/**
			 * Clear the keys associated with a specific remote peer as identified by its peer GUID. The
			 * peer GUID associated with a bus name can be obtained by calling GetPeerGUID().
			 *
			 * @param guid  The guid of a remote authenticated peer.
			 *
			 * @return
			 *      - QStatus.OK if the keys were cleared
			 *      - QStatus.UNKNOWN_GUID if there is no peer with the specified GUID
			 *      - Other errors
			 */
			public QStatus ClearKeys(string guid)
			{
				return alljoyn_busattachment_clearkeys(_busAttachment, guid);
			}

			/**
			 * Set the expiration time on keys associated with a specific remote peer as identified by its
			 * peer GUID. The peer GUID associated with a bus name can be obtained by calling GetPeerGUID().
			 * If the timeout is 0 this is equivalent to calling ClearKeys().
			 *
			 * @param guid     The GUID of a remote authenticated peer.
			 * @param timeout  The time in seconds relative to the current time to expire the keys.
			 *
			 * @return
			 *      - QStatus.OK if the expiration time was successfully set.
			 *      - QStatus.UNKNOWN_GUID if there is no authenticated peer with the specified GUID
			 *      - Other errors
			 */
			public QStatus SetKeyExpiration(string guid, uint timeout)
			{
				return alljoyn_busattachment_setkeyexpiration(_busAttachment, guid, timeout);
			}

			/**
			 * Get the expiration time on keys associated with a specific authenticated remote peer as
			 * identified by its peer GUID. The peer GUID associated with a bus name can be obtained by
			 * calling GetPeerGUID().
			 *
			 * @param guid     The GUID of a remote authenticated peer.
			 * @param timeout  The time in seconds relative to the current time when the keys will expire.
			 *
			 * @return
			 *       - QStatus.OK if the expiration time was successfully set.
			 *       - QStatus.UNKNOWN_GUID if there is no authenticated peer with the specified GUID
			 *       - Other errors
			 */
			public QStatus GetKeyExpiration(string guid, out uint timeout)
			{
				uint _timeout = 0;
				QStatus ret = alljoyn_busattachment_getkeyexpiration(_busAttachment, guid, ref _timeout);
				timeout = _timeout;
				return ret;
			}

			/**
			 * Adds a logon entry string for the requested authentication mechanism to the key store. This
			 * allows an authenticating server to generate offline authentication credentials for securely
			 * logging on a remote peer using a user-name and password credentials pair. This only applies
			 * to authentication mechanisms that support a user name + password logon functionality.
			 *
			 * @param authMechanism The authentication mechanism.
			 * @param userName      The user name to use for generating the logon entry.
			 * @param password      The password to use for generating the logon entry. If the password is
			 *                      NULL the logon entry is deleted from the key store.
			 *
			 * @return
			 *      - QStatus.OK if the logon entry was generated.
			 *      - QStatus.BUS_INVALID_AUTH_MECHANISM if the authentication mechanism does not support
			 *                                       logon functionality.
			 *      - QStatus.BAD_ARG_2 indicates a null string was used as the user name.
			 *      - QStatus.BAD_ARG_3 indicates a null string was used as the password.
			 *      - Other error status codes indicating a failure
			 */
			public QStatus AddLogonEntry(string authMechanism, string userName, string password)
			{

				return alljoyn_busattachment_addlogonentry(_busAttachment, authMechanism, userName, password);
			}

			/**
			 * Add a DBus match rule.
			 * This method is a shortcut/helper that issues an org.freedesktop.DBus.AddMatch
			 * method call to the local daemon.
			 *
			 * @param[in]  rule  Match rule to be added (see DBus specification for format of this string).
			 *
			 * @return
			 *      - QStatus.OK if the AddMatch request was successful.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus AddMatch(string rule)
			{
				return alljoyn_busattachment_addmatch(_busAttachment, rule);
			}

			/**
			 * Remove a DBus match rule.
			 * This method is a shortcut/helper that issues an org.freedesktop.DBus.RemoveMatch method call to the local daemon.
			 *
			 * @param[in]  rule  Match rule to be removed (see DBus specification for format of this string).
			 *
			 * @return
			 *      - QStatus.OK if the RemoveMatch request was successful.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus RemoveMatch(string rule)
			{

				return alljoyn_busattachment_removematch(_busAttachment, rule);
			}

			/**
			 * Set the SessionListener for an existing sessionId.
			 * Calling this method will override the listener set by a previous call to SetSessionListener or any
			 * listener specified in JoinSession.
			 *
			 * @param sessionId    The session id of an existing session.
			 * @param listener     The SessionListener to associate with the session. May be NULL to clear previous listener.
			 * @return  QStatus.OK if successful.
			 */
			public QStatus SetSessionListener(SessionListener listener, uint sessionId)
			{
				QStatus ret = QStatus.OK;
				Thread bindThread = new Thread((object o) =>
				{
					ret = alljoyn_busattachment_setsessionlistener(_busAttachment, sessionId, listener == null ? IntPtr.Zero : listener.UnmanagedPtr);
				});
				bindThread.Start();
				while (bindThread.IsAlive)
				{
					AllJoyn.TriggerCallbacks();
					Thread.Sleep(0);
				}
				return ret;
			}

			/**
			 * Leave an existing session.
			 * This method is a shortcut/helper that issues an org.alljoyn.Bus.LeaveSession method call to the local daemon
			 * and interprets the response.
			 *
			 * @param[in]  sessionId     Session id.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and the leave operation was successfully completed.
			 *      - QStatus.BUS_NOT_CONNECTED if a connection has not been made with a local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus LeaveSession(uint sessionId)
			{
				return alljoyn_busattachment_leavesession(_busAttachment, sessionId);
			}

			/**
			 * Remove a member from an existing multipoint session.
			 * This function may be called by the binder of the session to forcefully
			 * remove a member from a session.
			 *
			 * This method is a shortcut/helper that issues an
			 * org.alljoyn.Bus.RemoveSessionMember method call to the local daemon and
			 * interprets the response.
			 *
			 * @param[in]  sessionId     Session id.
			 * @param[in]  memberName    Member to remove.
			 *
			 * @return
			 *      - QStatus.OK iff daemon response was received and the remove member
			 *        operation was successfully completed.
			 *      - QStatus.ER_BUS_NOT_CONNECTED if a connection has not been made with a
			 *        local bus.
			 *      - Other error status codes indicating a failure.
			 */
			public QStatus RemoveSessionMember(uint sessionId, string memberName)
			{
				return alljoyn_busattachment_removesessionmember(_busAttachment, sessionId, memberName);
			}

			/**
			 * Set the link timeout for a session.
			 *
			 * Link timeout is the maximum number of seconds that an unresponsive daemon-to-daemon connection
			 * will be monitored before declaring the session lost (via SessionLost callback). Link timeout
			 * defaults to 0 which indicates that AllJoyn link monitoring is disabled.
			 *
			 * Each transport type defines a lower bound on link timeout to avoid defeating transport
			 * specific power management algorithms.
			 *
			 * @param sessionId     Id of session whose link timeout will be modified.
			 * @param linkTimeout   [IN/OUT] Max number of seconds that a link can be unresponsive before being
			 *                      declared lost. 0 indicates that AllJoyn link monitoring will be disabled. On
			 *                      return, this value will be the resulting (possibly upward) adjusted linkTimeout
			 *                      value that acceptable to the underlying transport.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.ALLJOYN_SETLINKTIMEOUT_REPLY_NOT_SUPPORTED if local daemon does not support SetLinkTimeout
			 *      - QStatus.ALLJOYN_SETLINKTIMEOUT_REPLY_NO_DEST_SUPPORT if SetLinkTimeout not supported by destination
			 *      - QStatus.BUS_NO_SESSION if the Session id is not valid
			 *      - QStatus.ALLJOYN_SETLINKTIMEOUT_REPLY_FAILED if SetLinkTimeout failed
			 *      - QStatus.BUS_NOT_CONNECTED if the BusAttachment is not connected to the daemon
			 */
			public QStatus SetLinkTimeout(uint sessionId, ref uint linkTimeout)
			{
				return alljoyn_busattachment_setlinktimeout(_busAttachment, sessionId, ref linkTimeout);
			}

			/**
			 * Get the peer GUID for this peer of the local peer or an authenticated remote peer. The bus
			 * names of a remote peer can change over time, specifically the unique name is different each
			 * time the peer connects to the bus and a peer may use different well-known-names at different
			 * times. The peer GUID is the only persistent identity for a peer. Peer GUIDs are used by the
			 * authentication mechanisms to uniquely and identify a remote application instance. The peer
			 * GUID for a remote peer is only available if the remote peer has been authenticated.
			 *
			 * @param name  Name of a remote peer or NULL to get the local (this application's) peer GUID.
			 * @param guid  Returns the guid for the local or remote peer depending on the value of name.
			 *
			 * @return
			 *      - QStatus.OK if the requested GUID was obtained.
			 *      - An error status otherwise.
			 */
			public QStatus GetPeerGuid(string name, out string guid)
			{

				UIntPtr guidSz = new UIntPtr();
				QStatus ret = alljoyn_busattachment_getpeerguid(_busAttachment, name,
					IntPtr.Zero, ref guidSz);
				if (!ret)
				{
					guid = "";
				}
				else
				{
					byte[] guidBuffer = new byte[(int)guidSz];
					GCHandle gch = GCHandle.Alloc(guidBuffer, GCHandleType.Pinned);
					ret = alljoyn_busattachment_getpeerguid(_busAttachment, name,
						gch.AddrOfPinnedObject(), ref guidSz);
					gch.Free();
					if (!ret)
					{
						guid = "";
					}
					else
					{	// The returned buffer will contain a nul character an so we must remove the last character.
						guid = System.Text.ASCIIEncoding.ASCII.GetString(guidBuffer, 0, (Int32)guidSz - 1);
					}
				}
				return ret;
			}

			/**
			 * Determine whether a given well-known name exists on the bus.
			 * This method is a shortcut/helper that issues an org.freedesktop.DBus.NameHasOwner method call to the daemon
			 * and interprets the response.
			 *
			 * @param[in]  name       The well known name that the caller is inquiring about.
			 * @param[out] hasOwner   If return is QStatus.OK, indicates whether name exists on the bus.
			 *                        If return is not QStatus.OK, hasOwner parameter is not modified.
			 * @return
			 *      - QStatus.OK if name ownership was able to be determined.
			 *      - An error status otherwise
			 */
			public QStatus NameHasOwner(string name, out bool hasOwner)
			{
				int intHasOwner = 0;
				QStatus ret = alljoyn_busattachment_namehasowner(_busAttachment, name, ref intHasOwner);
				hasOwner = (intHasOwner == 1 ? true : false);
				return ret;
			}

			/**
			 * This sets the debug level of the local AllJoyn daemon if that daemon
			 * was built in debug mode.
			 *
			 * The debug level can be set for individual subsystems or for "ALL"
			 * subsystems.  Common subsystems are "ALLJOYN" for core AllJoyn code,
			 * "ALLJOYN_OBJ" for the sessions management code and "ALLJOYN_NS" for
			 * the TCP name services.  Debug levels for specific subsystems override
			 * the setting for "ALL" subsystems.  For example if "ALL" is set to 7,
			 * but "ALLJOYN_OBJ" is set to 1, then detailed debug output will be
			 * generated for all subsystems except for "ALLJOYN_OBJ" which will only
			 * generate high level debug output.  "ALL" defaults to 0 which is off,
			 * or no debug output.
			 *
			 * The debug output levels are actually a bit field that controls what
			 * output is generated.  Those bit fields are described below:
			 *
			 *     - 0x1: High level debug prints (these debug printfs are not common)
			 *     - 0x2: Normal debug prints (these debug printfs are common)
			 *     - 0x4: Function call tracing (these debug printfs are used
			 *            sporadically)
			 *     - 0x8: Data dump (really only used in the "SOCKET" module - can
			 *            generate a *lot* of output)
			 *
			 * Typically, when enabling debug for a subsystem, the level would be set
			 * to 7 which enables High level debug, normal debug, and function call
			 * tracing.  Setting the level 0, forces debug output to be off for the
			 * specified subsystem.
			 *
			 * @param module    name of the module to generate debug output
			 * @param level     debug level to set for the module
			 *
			 * @return
			 *     - QStatus.OK if debug request was successfully sent to the AllJoyn
			 *       daemon.
			 *     - QStatus.BUS_NO_SUCH_OBJECT if daemon was not built in debug mode.
			 */
			public QStatus SetDaemonDebug(string module, uint level)
			{
				return alljoyn_busattachment_setdaemondebug(_busAttachment, module, level);
			}

			/**
			 * Determine if you are able to find a remote connection based on its BusName.
			 * The BusName can be the Unique or well-known name.
			 * @param name The unique or well-known name to ping
			 * @param timeout Timeout specified in milliseconds to wait for reply
			 * @return
                         *   - QStatus.OK the name is present and responding
                         *   - QStatus.ALLJOYN_PING_REPLY_UNREACHABLE the name is no longer present
                         *   <br>
                         *   The following return values indicate that the router cannot determine if the 
                         *   remote name is present and responding:
                         *   - QStatus.ALLJOYN_PING_REPLY_TIMEOUT Ping call timed out
                         *   - QStatus.ALLJOYN_PING_REPLY_UNKNOWN_NAME name not found currently or not part of any known session
                         *   - QStatus.ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE the remote routing node does not implement Ping
                         *   <br>
                         *   The following return values indicate an error with the ping call itself:
                         *   - QStatus.ALLJOYN_PING_FAILED Ping failed
                         *   - QStatus.BUS_UNEXPECTED_DISPOSITION An unexpected disposition was returned and has been treated as an error
                         *   - QStatus.BUS_NOT_CONNECTED the BusAttachment is not connected to the bus
                         *   - QStatus.BUS_BAD_BUS_NAME the name parameter is not a valid bus name
                         *   - An error status otherwise
			 */
			public QStatus Ping(string name, uint timeout)
			{
				return alljoyn_busattachment_ping(_busAttachment, name, timeout);
			}

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			/**
			 * SignalHandlers are methods which are called by AllJoyn library
			 * to forward AllJoyn received signals to AllJoyn library users.
			 *
			 * @param member    Method or signal interface member entry.
			 * @param srcPath   Object path of signal emitter.
			 * @param message   The received message.
			 */
			public delegate void SignalHandler(InterfaceDescription.Member member, string srcPath, Message message);

			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalSignalHandler(IntPtr member, IntPtr srcPath, IntPtr message);
			#endregion

			#region Properties
			/**
			 * Get the concurrent method and signal handler limit.
			 *
			 * @return The maximum number of concurrent method and signal handlers.
			 */
			public uint Concurrency
			{
				get
				{
					return alljoyn_busattachment_getconcurrency(_busAttachment);
				}
			}

			/**
			 * Get the connect spec used by the alljoyn_busattachment
			 *
			 *  @return The string representing the connect spec used by the alljoyn_busattachment
			 */
			public string ConnectSpec
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_busattachment_getconnectspec(_busAttachment));
				}
			}

			/**
			 * Check is peer security has been enabled for this bus attachment.
			 *
			 * @return   Returns true if peer security has been enabled, false otherwise.
			 */
			public bool IsPeerSecurityEnabled
			{
				get
				{
					return (alljoyn_busattachment_ispeersecurityenabled(_busAttachment) == 1 ? true : false);
				}
			}

			/**
			 * @brief Determine if the bus attachment has been Start()ed.
			 *
			 * @see Start()
			 * @see Stop()
			 * @see Join()
			 *
			 * @return true if the message bus has been Start()ed.
			 */
			public bool IsStarted
			{
				get
				{
					return alljoyn_busattachment_isstarted(_busAttachment);
				}
			}

			/**
			 * @brief Determine if the bus attachment has been Stop()ed.
			 *
			 * @see Start()
			 * @see Stop()
			 * @see Join()
			 *
			 * @return true if the message bus has been Start()ed.
			 */
			public bool IsStopping
			{
				get
				{
					return alljoyn_busattachment_isstopping(_busAttachment);
				}
			}

			/**
			 * Indicate whether bus is currently connected.
			 *
			 * Messages can only be sent or received when the bus is connected.
			 *
			 * @return true if the bus is connected.
			 */
			public bool IsConnected
			{
				get
				{
					return alljoyn_busattachment_isconnected(_busAttachment);
				}
			}

			/**
			 * Get the org.freedesktop.DBus proxy object.
			 *
			 * @return org.freedesktop.DBus proxy object
			 */
			public ProxyBusObject DBusProxyObj
			{
				get
				{
					return new ProxyBusObject(alljoyn_busattachment_getdbusproxyobj(_busAttachment));
				}
			}

			/**
			 * Get the org.alljoyn.Bus proxy object.
			 *
			 * @return org.alljoyn.Bus proxy object
			 */
			public ProxyBusObject AllJoynProxyObj
			{
				get
				{
					return new ProxyBusObject(alljoyn_busattachment_getalljoynproxyobj(_busAttachment));
				}
			}

			/**
			 * Get the org.alljoyn.Debug proxy object.
			 *
			 * @return org.alljoyn.Debug proxy object
			 */
			public ProxyBusObject AllJoynDebugObj
			{
				get
				{
					return new ProxyBusObject(alljoyn_busattachment_getalljoyndebugobj(_busAttachment));
				}
			}

			/**
			 * Get the unique name of this BusAttachment.
			 *
			 * @return The unique name of this BusAttachment.
			 */
			public string UniqueName
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_busattachment_getuniquename(_busAttachment));
				}
			}

			/**
			 * Get the GUID of this BusAttachment.
			 *
			 * The returned value may be appended to an advertised well-known name in order to guarantee
			 * that the resulting name is globally unique.
			 *
			 * @return GUID of this BusAttachment as a string.
			 */
			public string GlobalGUIDString
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_busattachment_getglobalguidstring(_busAttachment));
				}
			}

			/**
			 * Returns the current non-absolute real-time clock used internally by AllJoyn. This value can be
			 * compared with the timestamps on messages to calculate the time since a timestamped message
			 * was sent.
			 *
			 * @return  The current timestamp in milliseconds.
			 */
			public static uint Timestamp
			{
				get
				{
					return alljoyn_busattachment_gettimestamp();
				}
			}
			#endregion

			internal static BusAttachment MapBusAttachment(IntPtr key)
			{

				return _sBusAttachmentMap[key];
			}

			#region IDisposable
			~BusAttachment()
			{
				Dispose(false);
			}

			/**
			 * Dispose the BusAttachment
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if (!_isDisposed)
				{
					AllJoyn.StopAllJoynProcessing();
					Thread destroyThread = new Thread((object o) =>
					{
						if (_signalHandlerDelegateRefHolder.Count > 0)
						{
							UnregisterAllHandlers();
						}
						alljoyn_busattachment_destroy(_busAttachment);
					});
					destroyThread.Start();
					while (destroyThread.IsAlive)
					{
						AllJoyn.TriggerCallbacks();
						Thread.Sleep(0);
					}
					_sBusAttachmentMap.Remove(_busAttachment);
					_busAttachment = IntPtr.Zero;
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the BusAttachment
			 */
			public void Dispose()
			{

				Dispose(true);
				GC.SuppressFinalize(this);
			}

			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_create(
				[MarshalAs(UnmanagedType.LPStr)] string applicationName,
				int allowRemoteMessages);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_create_concurrency(
				[MarshalAs(UnmanagedType.LPStr)] string applicationName,
				int allowRemoteMessages, uint concurrency);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_destroy(IntPtr busAttachment);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_stop(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_join(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_busattachment_getconcurrency(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getconnectspec(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_busattachment_enableconcurrentcallbacks(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_createinterface(
				IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				ref IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_createinterface_secure(
				IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				ref IntPtr iface,
				[In]InterfaceDescription.SecurityPolicy secPolicy);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_start(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_connect(IntPtr bus, IntPtr connectSpec);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_connect(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string connectSpec);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_busattachment_registerbuslistener(IntPtr bus, IntPtr listener);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_busattachment_unregisterbuslistener(IntPtr bus, IntPtr listener);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_findadvertisedname(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string namePrefix);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_findadvertisednamebytransport(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)]string namePrefix, ushort transports);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_cancelfindadvertisedname(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string namePrefix);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_cancelfindadvertisednamebytransport(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string namePrefix, ushort transports);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_advertisename(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name, ushort transports);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_canceladvertisename(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name, ushort transports);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getinterface(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_joinsession(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string sessionHost,
				ushort sessionPort,
				IntPtr listener,
				ref uint sessionId,
				IntPtr opts);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_registerbusobject(IntPtr bus, IntPtr obj);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_registerbusobject_secure(IntPtr bus, IntPtr obj);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_busattachment_unregisterbusobject(IntPtr bus, IntPtr obj);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_requestname(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string requestedName, uint flags);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_releasename(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_bindsessionport(IntPtr bus,
				ref ushort sessionPort,
				IntPtr opts,
				IntPtr listener);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_unbindsessionport(IntPtr bus, ushort sessionPort);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_enablepeersecurity(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string authMechanisms,
				IntPtr listener,
				[MarshalAs(UnmanagedType.LPStr)] string keyStoreFileName,
				int isShared);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_ispeersecurityenabled(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_busattachment_isstarted(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_busattachment_isstopping(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_busattachment_isconnected(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static uint alljoyn_busattachment_gettimestamp();

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_createinterfacesfromxml(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string xml);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_busattachment_getinterfaces(IntPtr bus, IntPtr ifaces, UIntPtr numIfaces);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_deleteinterface(IntPtr bus, IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_disconnect(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string connectSpec);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getdbusproxyobj(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getalljoynproxyobj(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getalljoyndebugobj(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getuniquename(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_busattachment_getglobalguidstring(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_registerkeystorelistener(IntPtr bus, IntPtr listener);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_reloadkeystore(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_busattachment_clearkeystore(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_clearkeys(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string guid);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_setkeyexpiration(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string guid,
				uint timeout);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_getkeyexpiration(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string guid,
				ref uint timeout);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_addlogonentry(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string authMechanism,
				[MarshalAs(UnmanagedType.LPStr)] string userName,
				[MarshalAs(UnmanagedType.LPStr)] string password);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_addmatch(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string rule);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_removematch(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string rule);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_setsessionlistener(IntPtr bus, uint sessionId,
				IntPtr listener);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_leavesession(IntPtr bus, uint sessionId);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_removesessionmember(IntPtr bus, uint sessionid,
				[MarshalAs(UnmanagedType.LPStr)] string memberName);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_setlinktimeout(IntPtr bus, uint sessionid, ref uint linkTimeout);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_namehasowner(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				ref int hasOwner);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_getpeerguid(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				IntPtr guid, ref UIntPtr guidSz);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_setdaemondebug(IntPtr bus,
				[MarshalAs(UnmanagedType.LPStr)] string module,
				uint level);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_registersignalhandler(IntPtr bus,
				IntPtr signalHandler, InterfaceDescription._Member member,
				[MarshalAs(UnmanagedType.LPStr)] string srcPath);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_unregistersignalhandler(IntPtr bus,
				IntPtr signalHandler, InterfaceDescription._Member member,
				[MarshalAs(UnmanagedType.LPStr)] string srcPath);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_unregisterallhandlers(IntPtr bus);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_busattachment_ping(IntPtr bus, [MarshalAs(UnmanagedType.LPStr)] string name, uint timeout);

			#endregion

			#region Structs
			[StructLayout(LayoutKind.Sequential)]
			private struct SignalEntry
			{
				public IntPtr member;
				public IntPtr signal_handler;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _busAttachment;
				}
			}
			#endregion

			#region Data
			IntPtr _busAttachment;
			bool _isDisposed = false;

			Dictionary<SignalHandler, InternalSignalHandler> _signalHandlerDelegateRefHolder;
			static Dictionary<IntPtr, BusAttachment> _sBusAttachmentMap;
			#endregion
		}
	}
}