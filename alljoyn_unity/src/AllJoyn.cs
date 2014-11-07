/**
 * @file AllJoyn.cs provides namespace for AllJoyn and methods to request
 * the library for version information.
 */

//-----------------------------------------------------------------------
// <copyright file="AllJoyn.cs" company="AllSeen Alliance.">
// Copyright (c) 2012,2014 AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------
using System;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;

namespace AllJoynUnity
{
	/**
	 * %AllJoyn namespace and methods to request the library for version information, also starts AllJoyn processing for callbacks.
	 */
	public partial class AllJoyn
	{
		// DLL name for externs
#if UNITY_IPHONE 
		// On iOS plugins are statically linked so __Internal must be used as the library name
		private const string DLL_IMPORT_TARGET = "__Internal";
#else
		private const string DLL_IMPORT_TARGET = "alljoyn_unity_native";
#endif

		private const string UNITY_VERSION = "0.0.3";

		private static readonly int DEFERED_CALLBACK_WAIT_TIMER = 50;

		private static Boolean autoProcessCallback = true;
		private static Thread callbackPumpThread = null;
		private static Boolean isProcessing = false;

		/**
		 * Request version of the C# binding
		 *
		 * @return string representing the version of the csharp language binding
		 */
		public static string GetExtensionVersion()
		{
			return UNITY_VERSION;
		}

		/**
		 *  Get the version string from AllJoyn.
		 *
		 * @return string representing the version from AllJoyn
		 */
		public static string GetVersion()
		{
			return Marshal.PtrToStringAnsi(alljoyn_getversion());
		}

		/**
		 *  Get the build info string from AllJoyn
		 *
		 * @return string representing the buid info from AllJoyn
		 */
		public static string GetBuildInfo()
		{
			return Marshal.PtrToStringAnsi(alljoyn_getbuildinfo());
		}

		/**
		 * Gives the version of AllJoyn Library as a single number
		 * 
		 * @return a number representing the version of AllJoyn
		 */
		public static uint GetNumericVersion()
		{
			return alljoyn_getnumericversion();
		}

		/**
		 * Turn off auto processing of callback data from a seperate thread. 
		 * Used if developer manually calls Trigger callbacks on their schedule.
		 * @param autoProcess	Turn on/off the ability for AllJoyn to process the callbacks in a new thread
		 */
		public static void SetAutoAllJoynCallbackProcessing(Boolean autoProcess)
		{
			autoProcessCallback = autoProcess;
			if (autoProcessCallback)
			{
				StartAllJoynCallbackProcessing();
			}
		}

		/**
		 * Starts a thread to process AllJoyn callback data.
		 */
		public static void StartAllJoynCallbackProcessing()
		{
			if (!autoProcessCallback)
			{
				return;
			}
			if (callbackPumpThread == null)
			{
				alljoyn_unity_set_deferred_callback_mainthread_only(1); //FOR ANDROID THIS NEEDS TO BE SET TO 1 INSTEAD OF 0
				callbackPumpThread = new Thread((object o) =>
				{
					while (isProcessing)
					{
						alljoyn_unity_deferred_callbacks_process();
						Thread.Sleep(DEFERED_CALLBACK_WAIT_TIMER);
					}
				});
			}
			if (!callbackPumpThread.IsAlive)
			{
				isProcessing = true;
				callbackPumpThread.Start();
			}
			alljoyn_unity_deferred_callbacks_process();
		}

		/**
		 * Stops the thread processing AllJoyn callback data.
		 */
		public static void StopAllJoynProcessing()
		{
			if (callbackPumpThread != null && callbackPumpThread.IsAlive)
			{
				isProcessing = false;
				Thread.Sleep(DEFERED_CALLBACK_WAIT_TIMER);
				callbackPumpThread.Join();
			}
			callbackPumpThread = null;
		}

		/**
		 * Call to trigger callbacks on main thread. Allows to manually process the callbacks.
		 * @return the number of callbacks processed
		 */
		public static int TriggerCallbacks()
		{
			return alljoyn_unity_deferred_callbacks_process();
		}

		/**
		 * Enable/disable main-thread-only callbacks.
		 * NOTE: For Android this should be called with a value of true
		 * @param mainThreadOnly set to true to limit callbacks to the main thread
		 */
		public static void SetMainThreadOnlyCallbacks(bool mainThreadOnly)
		{
			alljoyn_unity_set_deferred_callback_mainthread_only(mainThreadOnly ? 1 : 0);
		}

		[Flags]
		/** Bitmask of all transport types */
		public enum TransportMask : ushort
		{
			None = 0x0000, /**< no transports */
			Any = 0xFF7F, /**< ANY transport (but Wi-Fi Direct) */
			Local = 0x0001, /**< Local (same device) transport */
			WLAN = 0x0004, /**< Wireless local-area network transport */
			WWAN = 0x0008, /**< Wireless wide-area network transport */
			LAN = 0x0010, /**< Wired local-area network transport */
			TCP = 0x0004, /**< TCP/IP transport */
			UDP = 0x0100, /**< UDP/IP transport */
			IP =  0x0104  /**< IP transport (system chooses best IP transport) */
		}

		#region DLL Imports
		[DllImport(DLL_IMPORT_TARGET, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
		private extern static IntPtr alljoyn_getversion();

		[DllImport(DLL_IMPORT_TARGET)]
		private extern static IntPtr alljoyn_getbuildinfo();

		[DllImport(DLL_IMPORT_TARGET)]
		private extern static uint alljoyn_getnumericversion();

		[DllImport(DLL_IMPORT_TARGET)]
		private extern static int alljoyn_unity_deferred_callbacks_process();

		[DllImport(DLL_IMPORT_TARGET)]
		private extern static void alljoyn_unity_set_deferred_callback_mainthread_only(int mainthread_only);

		#endregion
	}
}
