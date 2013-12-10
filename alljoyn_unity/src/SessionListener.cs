/**
 * @file
 * SessionListener is an abstract base class (interface) implemented by users of the
 * AllJoyn API in order to receive sessions related event information.
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
		public class SessionListener : IDisposable
		{
			/** Reason for the session being lost */
			public enum SessionLostReason{
				ALLJOYN_SESSIONLOST_INVALID                      = 0x00, /**< Invalid */
				ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION      = 0x01, /**< Remote end called LeaveSession */
				ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY   = 0x02, /**< Remote end closed abruptly */
				ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER            = 0x03, /**< Session binder removed this endpoint by calling RemoveSessionMember */
				ALLJOYN_SESSIONLOST_LINK_TIMEOUT                 = 0x04, /**< Link was timed-out */
				ALLJOYN_SESSIONLOST_REASON_OTHER                 = 0x05 /**< Unspecified reason for session loss */
			};
		    /**
		     * Constructor for a SessionListener.
		     */
			public SessionListener()
			{
			
				_sessionLost = new InternalSessionLost(this._SessionLost);
				_sessionMemberAdded = new InternalSessionMemberAdded(this._SessionMemberAdded);
				_sessionMemberRemoved = new InternalSessionMemberRemoved(this._SessionMemberRemoved);

				callbacks.sessionLost = Marshal.GetFunctionPointerForDelegate(_sessionLost);
				callbacks.sessionMemberAdded = Marshal.GetFunctionPointerForDelegate(_sessionMemberAdded);
				callbacks.sessionMemberRemoved = Marshal.GetFunctionPointerForDelegate(_sessionMemberRemoved);

				main = GCHandle.Alloc(callbacks, GCHandleType.Pinned);
				_sessionListener = alljoyn_sessionlistener_create(main.AddrOfPinnedObject(), IntPtr.Zero);
			}

			/**
			 * Request the raw pointer of the AllJoyn C SessionListener
			 *
			 * @return the raw pointer of the AllJoyn C SessionListener
			 */ 
			public IntPtr getAddr()
			{
				return _sessionListener;
			}

			#region Virtual Methods
			/**
			 * Called by the bus when an existing session becomes disconnected.(Deprecated)
			 *
			 * @see SessionLost(uint sessionId, SessionLostReason reason)
			 *
			 * @param sessionId     Id of session that was lost.
			 */
			[System.Obsolete("SessionLost callback that only returns sessionId has been depricated in favor of the SessionLost that returns the SessionLostReason.")]
			protected virtual void SessionLost(uint sessionId)
			{
			
			}

			/**
			 * Called by the bus when an existing session becomes disconnected.
			 *
			 * @param sessionId     Id of session that was lost.
			 * @param reason        The reason for the session being lost
			 */
			protected virtual void SessionLost(uint sessionId, SessionLostReason reason)
			{

			}

			/**
			 * Called by the bus when a member of a multipoint session is added.
			 *
			 * @param sessionId     Id of session whose member(s) changed.
			 * @param uniqueName    Unique name of member who was added.
			 */
			protected virtual void SessionMemberAdded(uint sessionId, string uniqueName)
			{
			
			}

			/**
			 * Called by the bus when a member of a multipoint session is removed.
			 *
			 * @param sessionId     Id of session whose member(s) changed.
			 * @param uniqueName    Unique name of member who was removed.
			 */
			protected virtual void SessionMemberRemoved(uint sessionId, string uniqueName)
			{
			
			}
			#endregion

			#region Callbacks
			private void _SessionLost(IntPtr context, uint sessionId, SessionLostReason reason)
			{
				uint _sessionId = sessionId;
				SessionLostReason _reason = reason;
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
				{
				/* SessionLost must be called here for the obsolite callback function to
				 * continue to work
				 */
#pragma warning disable 618
					SessionLost(_sessionId);
#pragma warning restore 618
					SessionLost(_sessionId, _reason);
				});
				callIt.Start();
			}

			private void _SessionMemberAdded(IntPtr context, uint sessionId, IntPtr uniqueName)
			{
				uint _sessionId = sessionId;
				String _uniqueName = Marshal.PtrToStringAnsi(uniqueName);
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						SessionMemberAdded(_sessionId, _uniqueName);
					});
				callIt.Start();
			}

			private void _SessionMemberRemoved(IntPtr context, uint sessionId, IntPtr uniqueName)
			{
				uint _sessionId = sessionId;
				String _uniqueName = Marshal.PtrToStringAnsi(uniqueName);
				System.Threading.Thread callIt = new System.Threading.Thread((object o) =>
					{
						SessionMemberRemoved(_sessionId, _uniqueName);
					});
				 callIt.Start();
			}
			#endregion

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalSessionLost(IntPtr context, uint sessionId, SessionLostReason reason);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalSessionMemberAdded(IntPtr context, uint sessionId, IntPtr uniqueName);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalSessionMemberRemoved(IntPtr context, uint sessionId, IntPtr uniqueName);
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_sessionlistener_create(
				IntPtr callbacks,
				IntPtr context);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_sessionlistener_destroy(IntPtr listener);
			#endregion

			#region IDisposable
			~SessionListener()
			{
				Dispose(false);
			}

			/**
			 * Dispose the SessionListener
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if(!_isDisposed)
				{
					alljoyn_sessionlistener_destroy(_sessionListener);
					_sessionListener = IntPtr.Zero;
					main.Free();
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the SessionListener
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region Structs
			private struct SessionListenerCallbacks
			{
				public IntPtr sessionLost;
				public IntPtr sessionMemberAdded;
				public IntPtr sessionMemberRemoved;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _sessionListener;
				}
			}
			#endregion

			#region Data
			IntPtr _sessionListener;
			bool _isDisposed = false;

			GCHandle main;
			InternalSessionLost _sessionLost;
			InternalSessionMemberAdded _sessionMemberAdded;
			InternalSessionMemberRemoved _sessionMemberRemoved;
			SessionListenerCallbacks callbacks;
			#endregion
		}
	}
}
