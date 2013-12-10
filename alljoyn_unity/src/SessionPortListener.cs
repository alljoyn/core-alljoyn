/**
 * @file
 * SessionPortListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to receive session port related event information.
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
using System.Runtime.InteropServices;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
		 * users of session related events.
		 */
		public class SessionPortListener : IDisposable
		{
			/**
			 * Constructor for SessionPortListener.
			 */
			public SessionPortListener()
			{
				_acceptSessionJoiner = new InternalAcceptSessionJoiner(this._AcceptSessionJoiner);
				_sessionJoined = new InternalSessionJoined(this._SessionJoined);

				callbacks.acceptSessionJoiner = Marshal.GetFunctionPointerForDelegate(_acceptSessionJoiner);
				callbacks.sessionJoined = Marshal.GetFunctionPointerForDelegate(_sessionJoined);

				main = GCHandle.Alloc(callbacks, GCHandleType.Pinned);
				_sessionPortListener = alljoyn_sessionportlistener_create(main.AddrOfPinnedObject(), IntPtr.Zero);
			}

			#region Virtual Methods
			/**
			 * Accept or reject an incoming JoinSession request. The session does not exist until this
			 * after this function returns.
			 *
			 * This callback is only used by session creators. Therefore it is only called on listeners
			 * passed to BusAttachment.BindSessionPort.
			 *
			 * @param sessionPort    Session port that was joined.
			 * @param joiner         Unique name of potential joiner.
			 * @param opts           Session options requested by the joiner.
			 * @return   Return true if JoinSession request is accepted. false if rejected.
			 */
			protected virtual bool AcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts opts)
			{
				return false;
			}

			/**
			 * Called by the bus when a session has been successfully joined. The session is now fully up.
			 *
			 * This callback is only used by session creators. Therefore it is only called on listeners
			 * passed to BusAttachment.BindSessionPort.
			 *
			 * @param sessionPort    Session port that was joined.
			 * @param sessionId             Id of session.
			 * @param joiner         Unique name of the joiner.
			 */
			protected virtual void SessionJoined(ushort sessionPort, uint sessionId, string joiner)
			{
			
			}
			#endregion

			#region Callbacks
			private int _AcceptSessionJoiner(IntPtr context, ushort sessionPort, IntPtr joiner, IntPtr opts)
			{
				return (AcceptSessionJoiner(sessionPort, Marshal.PtrToStringAnsi(joiner), new SessionOpts(opts)) ? 1 : 0);
			}

			private void _SessionJoined(IntPtr context, ushort sessionPort, uint sessionId, IntPtr joiner)
			{
				ushort _sessionPort = sessionPort;
				uint _sessionId = sessionId;
				String _joiner = Marshal.PtrToStringAnsi(joiner);
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						SessionJoined(_sessionPort, _sessionId, _joiner);
					});
				callIt.Start();
			}
			#endregion

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate int InternalAcceptSessionJoiner(IntPtr context, ushort sessionPort, IntPtr joiner, IntPtr opts);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalSessionJoined(IntPtr context, ushort sessionPort, uint sessionId, IntPtr joiner);
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_sessionportlistener_create(
				IntPtr callbacks,
				IntPtr context);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_sessionportlistener_destroy(IntPtr listener);
			#endregion

			#region IDisposable
			~SessionPortListener()
			{
			
				Dispose(false);
			}

			/**
			 * Dispose the SessionPortListener
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if(!_isDisposed)
				{
					alljoyn_sessionportlistener_destroy(_sessionPortListener);
					_sessionPortListener = IntPtr.Zero;
					main.Free();
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the SessionPortListener
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this); 
			}
			#endregion

			#region Structs
			private struct SessionPortListenerCallbacks
			{
				public IntPtr acceptSessionJoiner;
				public IntPtr sessionJoined;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _sessionPortListener;
				}
			}
			#endregion

			#region Data
			IntPtr _sessionPortListener;
			bool _isDisposed = false;

			GCHandle main;
			InternalAcceptSessionJoiner _acceptSessionJoiner;
			InternalSessionJoined _sessionJoined;
			SessionPortListenerCallbacks callbacks;
			#endregion
		}
	}
}
