/**
 * @file
 * This file defines the AuthListener class that provides the interface between
 * authentication mechanisms and applications.
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
		public abstract class AuthListener : IDisposable
		{
			/**
				 * Constructor for AuthListener class 
				 */
			public AuthListener()
			{
				_requestCredentials = new InternalRequestCredentials(this._RequestCredentials);
				_verifyCredentials = new InternalVerifyCredentials(this._VerifyCredentials);
				_securityViolation = new InternalSecurityViolation(this._SecurityViolation);
				_authenticationComplete = new InternalAuthenticationComplete(this._AuthenticationComplete);

				AuthListenerCallbacks callbacks;
				callbacks.requestCredentials = Marshal.GetFunctionPointerForDelegate(_requestCredentials);
				callbacks.verifyCredentials = Marshal.GetFunctionPointerForDelegate(_verifyCredentials);
				callbacks.securityViolation = Marshal.GetFunctionPointerForDelegate(_securityViolation);
				callbacks.authenticationComplete = Marshal.GetFunctionPointerForDelegate(_authenticationComplete);

				main = GCHandle.Alloc(callbacks, GCHandleType.Pinned);
				_authListener = alljoyn_authlistener_create(main.AddrOfPinnedObject(), IntPtr.Zero);
			}

			#region Virtual Methods
			/**
			 * Authentication mechanism requests user credentials. If the user name is not an empty string
			 * the request is for credentials for that specific user. A count allows the listener to decide
			 * whether to allow or reject multiple authentication attempts to the same peer.
			 *
			 * @param authMechanism  The name of the authentication mechanism issuing the request.
			 * @param peerName       The name of the remote peer being authenticated.  On the initiating
			 *                       side this will be a well-known-name for the remote peer. On the
			 *                       accepting side this will be the unique bus name for the remote peer.
			 * @param authCount      Count (starting at 1) of the number of authentication request attempts made.
			 * @param userName       The user name for the credentials being requested.
			 * @param credMask       A bit mask identifying the credentials being requested. The application
			 *                       may return none, some or all of the requested credentials.
			 * @param[out] credentials    The credentials returned.
			 *
			 * @return  The caller should return true if the request is being accepted or false if the
			 *          requests is being rejected. If the request is rejected the authentication is
			 *          complete.
			 */
			protected abstract bool RequestCredentials(string authMechanism, string peerName, ushort authCount,
				string userName, Credentials.CredentialFlags credMask, Credentials credentials);

			/**
			 * Authentication mechanism requests verification of credentials from a remote peer.
			 *
			 * @param authMechanism  The name of the authentication mechanism issuing the request.
			 * @param peerName       The name of the remote peer being authenticated.  On the initiating
			 *                       side this will be a well-known-name for the remote peer. On the
			 *                       accepting side this will be the unique bus name for the remote peer.
			 * @param credentials    The credentials to be verified.
			 *
			 * @return  The listener should return true if the credentials are acceptable or false if the
			 *          credentials are being rejected.
			 */
			protected virtual bool VerifyCredentials(string authMechanism, string peerName, Credentials credentials)
			{
				return false;
			}

			/**
			 * Optional method that if implemented allows an application to monitor security violations. This
			 * function is called when an attempt to decrypt an encrypted messages failed or when an unencrypted
			 * message was received on an interface that requires encryption. The message contains only
			 * header information.
			 *
			 * @param status  A status code indicating the type of security violation.
			 * @param msg     The message that cause the security violation.
			 */
			protected virtual void SecurityViolation(QStatus status, Message msg)
			{
			
			}

			/**
			 * Reports successful or unsuccessful completion of authentication.
			 *
			 * @param authMechanism  The name of the authentication mechanism that was used or an empty
			 *                       string if the authentication failed.
			 * @param peerName       The name of the remote peer being authenticated.  On the initiating
			 *                       side this will be a well-known-name for the remote peer. On the
			 *                       accepting side this will be the unique bus name for the remote peer.
			 * @param success        true if the authentication was successful, otherwise false.
			 */
			protected abstract void AuthenticationComplete(string authMechanism, string peerName, bool success);
			#endregion

			#region Callbacks
			private int _RequestCredentials(IntPtr context, IntPtr authMechanism, IntPtr peerName, ushort authCount,
				IntPtr userName, ushort credMask, IntPtr credentials)
			{
			
				return (RequestCredentials(Marshal.PtrToStringAnsi(authMechanism), Marshal.PtrToStringAnsi(peerName),
					authCount, Marshal.PtrToStringAnsi(userName), (Credentials.CredentialFlags)credMask, new Credentials(credentials)) ? 1 : 0);
			}

			private int _VerifyCredentials(IntPtr context, IntPtr authMechanism, IntPtr peerName,
				IntPtr credentials)
			{
			
				return (VerifyCredentials(Marshal.PtrToStringAnsi(authMechanism), Marshal.PtrToStringAnsi(peerName),
					new Credentials(credentials)) ? 1 : 0);
			}

			private void _SecurityViolation(IntPtr context, int status, IntPtr msg)
			{
			
				SecurityViolation(status, new Message(msg));
			}

			private void _AuthenticationComplete(IntPtr context, IntPtr authMechanism, IntPtr peerName, int success)
			{
			
				AuthenticationComplete(Marshal.PtrToStringAnsi(authMechanism), Marshal.PtrToStringAnsi(peerName),
					success == 1 ? true : false);
			}
			#endregion

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate int InternalRequestCredentials(IntPtr context, IntPtr authMechanism, IntPtr peerName, ushort authCount,
				IntPtr userName, ushort credMask, IntPtr credentials);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate int InternalVerifyCredentials(IntPtr context, IntPtr authMechanism, IntPtr peerName,
				IntPtr credentials);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalSecurityViolation(IntPtr context, int status, IntPtr msg);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate void InternalAuthenticationComplete(IntPtr context, IntPtr authMechanism, IntPtr peerName, int success);
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_authlistener_create(
				IntPtr callbacks,
				IntPtr context);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_authlistener_destroy(IntPtr listener);
			#endregion

			#region IDisposable
			~AuthListener()
			{

				Dispose(false);
			}

			/**
			 * Dispose the AuthListener
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
			
				if(!_isDisposed)
				{
					alljoyn_authlistener_destroy(_authListener);
					_authListener = IntPtr.Zero;
					main.Free();
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the AuthListener
			 */
			public void Dispose()
			{

				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region Structs
			private struct AuthListenerCallbacks
			{
				public IntPtr requestCredentials;
				public IntPtr verifyCredentials;
				public IntPtr securityViolation;
				public IntPtr authenticationComplete;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _authListener;
				}
			}
			#endregion

			#region Data
			IntPtr _authListener;
			bool _isDisposed = false;

            GCHandle main;

			InternalRequestCredentials _requestCredentials;
			InternalVerifyCredentials _verifyCredentials;
			InternalSecurityViolation _securityViolation;
			InternalAuthenticationComplete _authenticationComplete;
			#endregion
		}
	}
}
