/**
 * @file
 * This file defines the Credentials class that describes the
 * authentication credentials.
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
		 * Generic class for describing different authentication credentials.
		 */
		public class Credentials : IDisposable
		{
			[Flags]
			/**
			 * CredentialFlags indication Bitmasks.
			 * Bitmasks used to indicated what type of credentials are being used.
			 */
			public enum CredentialFlags : ushort
			{
				Password = 0x0001, /**< Bit 0 indicates credentials include a password, pincode, or passphrase */
				UserName = 0x0002, /**< Bit 1 indicates credentials include a user name */
				CertChain = 0x0004, /**< Bit 2 indicates credentials include a chain of PEM-encoded X509 certificates */
				PrivateKey = 0x0008, /**< Bit 3 indicates credentials include a PEM-encoded private key */
				LogonEntry = 0x0010, /**< Bit 4 indicates credentials include a logon entry that can be used to logon a remote user */
				Expiration = 0x0020, /**< Bit 5 indicates credentials include an expiration time */
				NewPassword = 0x1001, /**< Indicates the credential request is for a newly created password */
				OneTimePassword = 0x2001 /**< Indicates the credential request is for a one time use password */
			}

			/**
			 * Constructor for a Crendtials object.
			 */
			public Credentials()
			{
				_credentials = alljoyn_credentials_create();
			}

			internal Credentials(IntPtr credentials)
			{
				_credentials = credentials;
				_isDisposed = true;
			}

			/**
			 * Tests if one or more credentials are set.
			 *
			 * @param credMask  A logical or of the credential bit values.
			 * @return true if the credentials are set.
			 */
			public bool IsSet(CredentialFlags credMask)
			{
				return (alljoyn_credentials_isset(_credentials, (ushort)credMask) == 1 ? true : false);
			}

			/**
			 * Clear the credentials.
			 */
			public void Clear()
			{
				alljoyn_credentials_clear(_credentials);
			}

			#region Properties
			/**
			 * Gets or Sets a requested password, pincode, or passphrase.
			 */
			public string Password
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_credentials_getpassword(_credentials));
				}
				set
				{
					alljoyn_credentials_setpassword(_credentials, (string)value);
				}
			}

			/**
			 * Gets or Sets a requested user name.
			 */
			public string UserName
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_credentials_getusername(_credentials));
				}
				set
				{
					alljoyn_credentials_setusername(_credentials, (string)value);
				}
			}

			/**
			 * Gets or Sets a requested public key certificate chain. The certificates must be PEM encoded.
			 */
			public string CertChain
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_credentials_getcertchain(_credentials));
				}
				set
				{
					alljoyn_credentials_setcertchain(_credentials, (string)value);
				}
			}

			/**
			 * Gets or Sets a requested private key. The private key must be PEM encoded and may be encrypted. If
			 * the private key is encrypted the passphrase required to decrypt it must also be supplied.
			 */
			public string PrivateKey
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_credentials_getprivateKey(_credentials));
				}
				set
				{
					alljoyn_credentials_setprivatekey(_credentials, (string)value);
				}
			}
			/**
			 * Gets or Sets a LogonEntry.
			 */
			public string LogonEntry
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_credentials_getlogonentry(_credentials));
				}
				set
				{
					alljoyn_credentials_setlogonentry(_credentials, (string)value);
				}
			}

			/**
			 * Gets or Sets an expiration time in seconds relative to the current time for the credentials. This value is optional and
			 * can be set on any response to a credentials request. After the specified expiration time has elapsed any secret
			 * keys based on the provided credentials are invalidated and a new authentication exchange will be required. If an
			 * expiration is not set the default expiration time for the requested authentication mechanism is used.
			 */
			public uint Expiration
			{
				get
				{
					return alljoyn_credentials_getexpiration(_credentials);
				}
				set
				{
					alljoyn_credentials_setexpiration(_credentials, (uint)value);
				}
			}
			#endregion

			#region IDisposable
			~Credentials()
			{
				Dispose(false);
			}

			/**
			 * Dispose the Credentials
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if(!_isDisposed)
				{
					alljoyn_credentials_destroy(_credentials);
					_credentials = IntPtr.Zero;
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the Credentials
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_credentials_create();

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_destroy(IntPtr msg);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_credentials_isset(IntPtr cred, ushort creds);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_setpassword(IntPtr cred, [MarshalAs(UnmanagedType.LPStr)] string pwd);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_setusername(IntPtr cred, [MarshalAs(UnmanagedType.LPStr)] string userName);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_setcertchain(IntPtr cred, [MarshalAs(UnmanagedType.LPStr)] string certChain);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_setprivatekey(IntPtr cred, [MarshalAs(UnmanagedType.LPStr)] string pk);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_setlogonentry(IntPtr cred, [MarshalAs(UnmanagedType.LPStr)] string logonEntry);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_setexpiration(IntPtr cred, uint expiration);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_credentials_getpassword(IntPtr cred);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_credentials_getusername(IntPtr cred);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_credentials_getcertchain(IntPtr cred);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_credentials_getprivateKey(IntPtr cred);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_credentials_getlogonentry(IntPtr cred);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern uint alljoyn_credentials_getexpiration(IntPtr cred);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_credentials_clear(IntPtr cred);
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _credentials;
				}
			}
			#endregion

			#region Data
			IntPtr _credentials;
			bool _isDisposed = false;
			#endregion
		}
	}
}
