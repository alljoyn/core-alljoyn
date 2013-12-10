/**
 * @file
 * BusListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to asynchronously receive bus  related event information.
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
using System.Runtime.InteropServices;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
		 * users of bus related events.
		 */
		public class BusListener : IDisposable
		{
			/**
			 * Construct a BusListener.
			 */
			public BusListener()
			{
				// Can't let the GC free these delegates so they must be members
				_listenerRegistered = new InternalListenerRegisteredDelegate(_ListenerRegistered);
				_listenerUnregistered = new InternalListenerUnregisteredDelegate(_ListenerUnregistered);
				_foundAdvertisedName = new InternalFoundAdvertisedNameDelegate(_FoundAdvertisedName);
				_lostAdvertisedName = new InternalLostAdvertisedNameDelegate(_LostAdvertisedName);
				_nameOwnerChanged = new InternalNameOwnerChangedDelegate(_NameOwnerChanged);
				_busStopping = new InternalBusStoppingDelegate(_BusStopping);
				_busDisconnected = new InternalBusDisconnectedDelegate(_BusDisconnected);

				callbacks.listenerRegistered = Marshal.GetFunctionPointerForDelegate(_listenerRegistered);
				callbacks.listenerUnregistered = Marshal.GetFunctionPointerForDelegate(_listenerUnregistered);
				callbacks.foundAdvertisedName =  Marshal.GetFunctionPointerForDelegate(_foundAdvertisedName);
				callbacks.lostAdvertisedName =  Marshal.GetFunctionPointerForDelegate(_lostAdvertisedName);
				callbacks.nameOwnerChanged = Marshal.GetFunctionPointerForDelegate(_nameOwnerChanged);
				callbacks.busStopping = Marshal.GetFunctionPointerForDelegate(_busStopping);
				callbacks.busDisconnected = Marshal.GetFunctionPointerForDelegate(_busDisconnected);

				main = GCHandle.Alloc(callbacks, GCHandleType.Pinned);
				_busListener = alljoyn_buslistener_create(main.AddrOfPinnedObject(), IntPtr.Zero);
			}

			#region Virtual Methods
			/**
			 * Called by the bus when the listener is registered. This give the listener implementation the
			 * opportunity to save a reference to the bus.
			 *
			 * @param busAttachment  The bus the listener is registered with.
			 */
			protected virtual void ListenerRegistered(BusAttachment busAttachment){}

			/**
			 * Called by the bus when the listener is unregistered.
			 */
			protected virtual void ListenerUnregistered() {}

			/**
			 * Called by the bus when an external bus is discovered that is advertising a well-known name
			 * that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
			 *
			 * @param name         A well known name that the remote bus is advertising.
			 * @param transport    Transport that received the advertisement.
			 * @param namePrefix   The well-known name prefix used in call to FindAdvertisedName that triggered this callback.
			 */
			protected virtual void FoundAdvertisedName(string name, TransportMask transport, string namePrefix) {}

			/**
			 * Called by the bus when an advertisement previously reported through FoundName has become unavailable.
			 *
			 * @param name         A well known name that the remote bus is advertising that is of interest to this attachment.
			 * @param transport    Transport that stopped receiving the given advertised name.
			 * @param namePrefix   The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.
			 */
			protected virtual void LostAdvertisedName(string name, TransportMask transport, string namePrefix) {}

			/**
			 * Called by the bus when the ownership of any well-known name changes.
			 *
			 * @param busName        The well-known name that has changed.
			 * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
			 * @param newOwner       The unique name that now owns the name or NULL if the there is no new owner.
			 */
			protected virtual void NameOwnerChanged(string busName, string previousOwner, string newOwner) {}

			/**
			 * Called when a BusAttachment this listener is registered with is stopping.
			 */
			protected virtual void BusStopping() {}

			/**
			 * Called when a BusAttachment this listener is registered with is has become disconnected from
			 * the bus.
			 */
			protected virtual void BusDisconnected() {}
			#endregion

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalListenerRegisteredDelegate(IntPtr context, IntPtr bus);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalListenerUnregisteredDelegate(IntPtr context);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalFoundAdvertisedNameDelegate(IntPtr context, IntPtr name, System.UInt16 transport, IntPtr namePrefix);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalLostAdvertisedNameDelegate(IntPtr context, IntPtr name, ushort transport, IntPtr namePrefix);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalNameOwnerChangedDelegate(IntPtr context, IntPtr busName, IntPtr previousOwner, IntPtr newOwner);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalBusStoppingDelegate(IntPtr context);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalBusDisconnectedDelegate(IntPtr context);
			#endregion

			#region Callbacks
			private void _ListenerRegistered(IntPtr context, IntPtr bus)
			{
				IntPtr _bus = bus;
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						_registeredBus = BusAttachment.MapBusAttachment(_bus);
						ListenerRegistered(_registeredBus);
					});
				callIt.Start();
			}

			private void _ListenerUnregistered(IntPtr context)
			{
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
				{
					ListenerUnregistered();
					_registeredBus = null;
				});
				callIt.Start();
			}

			private void _FoundAdvertisedName(IntPtr context, IntPtr name, System.UInt16 transport, IntPtr namePrefix)
			{
				String _name = Marshal.PtrToStringAnsi(name);
				TransportMask _transport = (TransportMask)transport;
				String _namePrefix = Marshal.PtrToStringAnsi(namePrefix);
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
						{
							FoundAdvertisedName(_name, _transport, _namePrefix);
						});
				callIt.Start();
			}

			private void _LostAdvertisedName(IntPtr context, IntPtr name, ushort transport, IntPtr namePrefix)
			{
				String _name = Marshal.PtrToStringAnsi(name);
				TransportMask _transport = (TransportMask)transport;
				String _namePrefix = Marshal.PtrToStringAnsi(namePrefix);
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
						{
							LostAdvertisedName(_name, _transport, _namePrefix);
						});
				callIt.Start();
			}

			private void _NameOwnerChanged(IntPtr context, IntPtr busName, IntPtr previousOwner, IntPtr newOwner)
			{
				String _busName = Marshal.PtrToStringAnsi(busName);
				String _previousOwner = Marshal.PtrToStringAnsi(previousOwner);
				String _newOwner = Marshal.PtrToStringAnsi(newOwner);
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						NameOwnerChanged(_busName, _previousOwner, _newOwner);
					});
				callIt.Start();
			}

			private void _BusStopping(IntPtr context)
			{
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						BusStopping();
					});
				callIt.Start();
			}

			private void _BusDisconnected(IntPtr context)
			{
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						BusDisconnected();
					});
				callIt.Start();
			}
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_buslistener_create(
				IntPtr callbacks,
				IntPtr context);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_buslistener_destroy(IntPtr listener);
			#endregion

			#region IDisposable
			~BusListener()
			{

				Dispose(false);
			}

			/**
			 * Dispose the BusListener
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
			
				if(!_isDisposed)
				{
					// Dispose of BusAttachment before listeners
					if(_registeredBus != null)
					{
						_registeredBus.Dispose();
					}
					
					Thread destroyThread = new Thread((object o) => {
						alljoyn_buslistener_destroy(_busListener);
					});
					destroyThread.Start();
					while(destroyThread.IsAlive)
					{
						AllJoyn.TriggerCallbacks();
						Thread.Sleep(0);
					}

					_busListener = IntPtr.Zero;
					main.Free();
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the BusListener
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region Structs
			[StructLayout(LayoutKind.Sequential)]
			private struct BusListenerCallbacks
			{
				public IntPtr listenerRegistered;
				public IntPtr listenerUnregistered;
				public IntPtr foundAdvertisedName;
				public IntPtr lostAdvertisedName;
				public IntPtr nameOwnerChanged;
				public IntPtr busStopping;
				public IntPtr busDisconnected;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _busListener;
				}
			}
			#endregion

			#region Data
			IntPtr _busListener;
			bool _isDisposed = false;
			BusAttachment _registeredBus;

			GCHandle main;
			BusListenerCallbacks callbacks;
			InternalListenerRegisteredDelegate _listenerRegistered;
			InternalListenerUnregisteredDelegate _listenerUnregistered;
			InternalFoundAdvertisedNameDelegate _foundAdvertisedName;
			InternalLostAdvertisedNameDelegate _lostAdvertisedName;
			InternalNameOwnerChangedDelegate _nameOwnerChanged;
			InternalBusStoppingDelegate _busStopping;
			InternalBusDisconnectedDelegate _busDisconnected;
			#endregion
		}
	}
}
