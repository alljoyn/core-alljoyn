/**
 * @file
 * The KeyStoreListener class handled requests to load or store the key store.
 */

/******************************************************************************
 * Copyright (c) 2010-2013, AllSeen Alliance. All rights reserved.
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
		 * An application can provide a key store listener to override the default key store Load and Store
		 * behavior. This will override the default key store behavior.
		 */
		public abstract class KeyStoreListener : IDisposable
		{
			/**
			 * Constructor for a KeyStoreListener object.
			 */
			public KeyStoreListener()
			{
			
				_loadRequest = new InternalLoadRequest(this._LoadRequest);
				_storeRequest = new InternalStoreRequest(this._StoreRequest);

				KeyStoreListenerCallbacks callbacks;
				callbacks.loadRequest = Marshal.GetFunctionPointerForDelegate(_loadRequest);
				callbacks.storeRequest = Marshal.GetFunctionPointerForDelegate(_storeRequest);

				GCHandle gch = GCHandle.Alloc(callbacks, GCHandleType.Pinned);
				_keyStoreListener = alljoyn_keystorelistener_create(gch.AddrOfPinnedObject(), IntPtr.Zero);
				gch.Free();
			}

			#region Abstract Methods
			/**
			 * This method is called when a key store needs to be loaded.
			 * @remark The application must call <tt>KeyStore#PutKeys</tt> to put the new key store data into the
			 * internal key store.
			 *
			 * @param store   Reference to the KeyStore to be loaded.
			 *
			 * @return
			 *      - QStatus.OK if the load request was satisfied
			 *      - An error status otherwise
			 *
			 */
			public abstract QStatus LoadRequest(KeyStore store);

			/**
			 * This method is called when a key store needs to be stored.
			 * @remark The application must call <tt>KeyStore#GetKeys</tt> to obtain the key data to be stored.
			 *
			 * @param store   Reference to the KeyStore to be stored.
			 *
			 * @return
			 *      - QStatus.OK if the store request was satisfied
			 *      - An error status otherwise
			 */
			public abstract QStatus StoreRequest(KeyStore store);
			#endregion

			#region Callbacks
			private int _LoadRequest(IntPtr context, IntPtr keyStoreListener, IntPtr keyStore)
			{
			
				return LoadRequest(new KeyStore(keyStore, this.UnmanagedPtr));
			}

			private int _StoreRequest(IntPtr context, IntPtr keyStoreListener, IntPtr keyStore)
			{
			
				return StoreRequest(new KeyStore(keyStore, this.UnmanagedPtr));
			}
			#endregion

			#region Delegates
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate int InternalLoadRequest(IntPtr context, IntPtr keyStoreListener, IntPtr keyStore);
			[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
			private delegate int InternalStoreRequest(IntPtr context, IntPtr keyStoreListener, IntPtr keyStore);
			#endregion

			/**
			 * The %KeyStore class manages the storing and loading of key blobs from
			 * external storage.
			 */
			public class KeyStore
			{
				internal KeyStore(IntPtr keyStore, IntPtr keyStoreListener)
				{
					_keyStore = keyStore;
					_keyStoreListener = keyStoreListener;
				}

				/**
				 * Put keys into the key store from an encrypted byte string.
				 *
				 * @param source    The byte string containing the encrypted key store contents.
				 * @param password  The password required to decrypt the key data
				 *
				 * @return
				 *      - QStatus.OK if successful
				 *      - An error status otherwise
				 *
				 */
				public QStatus PutKeys(string source, string password)
				{
					return alljoyn_keystorelistener_putkeys(_keyStoreListener, _keyStore,
						source, password);
				}

				 /**
				 * Get the current keys from the key store as an encrypted byte string.
				 *
				 * @return current keys 
				 */
				public string GetKeys()
				{
					int sinkSz = 512;
					byte[] sink;
					QStatus status;
					do
					{
						sinkSz *= 2;
						sink = new byte[sinkSz];
						GCHandle gch = GCHandle.Alloc(sink, GCHandleType.Pinned);
						status = alljoyn_keystorelistener_getkeys(_keyStoreListener, _keyStore,
							gch.AddrOfPinnedObject(), (UIntPtr)sinkSz);
						gch.Free();
					} while(status != QStatus.OK);
					// The returned buffer will contain a nul character an so we must remove the last character.
					if (sinkSz != 0)
					{
						return System.Text.ASCIIEncoding.ASCII.GetString(sink, 0, (Int32)sinkSz - 1);
					}
					else
					{
						return "";
					}
				}

				private IntPtr _keyStore;
				private IntPtr _keyStoreListener;
			}

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_keystorelistener_create(
				IntPtr callbacks,
				IntPtr context);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_keystorelistener_destroy(IntPtr listener);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_keystorelistener_putkeys(IntPtr listener, IntPtr keyStore,
				[MarshalAs(UnmanagedType.LPStr)] string source,
				[MarshalAs(UnmanagedType.LPStr)] string password);
			
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_keystorelistener_getkeys(IntPtr listener, IntPtr keyStore,
				IntPtr sink, UIntPtr sink_sz);
			#endregion

			#region IDisposable
			~KeyStoreListener()
			{
				Dispose(false);
			}

			/**
			 * Dispose the KeyStoreListener
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if(!_isDisposed)
				{
					alljoyn_keystorelistener_destroy(_keyStoreListener);
					_keyStoreListener = IntPtr.Zero;
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the KeyStoreListener
			 */
			public void Dispose()
			{

				Dispose(true);
				GC.SuppressFinalize(this);
			}
			#endregion

			#region Structs
			private struct KeyStoreListenerCallbacks
			{
				public IntPtr loadRequest;
				public IntPtr storeRequest;
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _keyStoreListener;
				}
			}
			#endregion

			#region Data
			IntPtr _keyStoreListener;
			bool _isDisposed = false;

			InternalLoadRequest _loadRequest;
			InternalStoreRequest _storeRequest;
			#endregion
		}
	}
}
