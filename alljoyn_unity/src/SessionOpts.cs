/**
 * @file
 * AllJoyn session options
 */

/******************************************************************************
 * Copyright (c) 2012-2013 AllSeen Alliance. All rights reserved.
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
		 * SessionOpts contains a set of parameters that define a Session's characteristics.
		 */
		public class SessionOpts : IDisposable
		{
			/** Traffic type */
			public enum TrafficType : byte
			{
				Messages = 0x01,   /**< Session carries message traffic */
				RawUnreliable = 0x02,   /**< Session carries an unreliable (lossy) byte stream */
				RawReliable = 0x04   /**< Session carries a reliable byte stream */
			}

			/** Proximity type */
			public enum ProximityType : byte
			{
				Any = 0xFF,
				Physical = 0x01,
				Network = 0x02
			}

			#region Properties
			public TrafficType Traffic /**< holds the Traffic type for this SessionOpt*/
			{
				get
				{
					return (TrafficType)alljoyn_sessionopts_get_traffic(_sessionOpts);
				}
				set
				{
					alljoyn_sessionopts_set_traffic(_sessionOpts, (byte)value);
				}
			}

			/**
			 * Multi-point session capable.
			 * A session is multi-point if it can be joined multiple times to form a single
			 * session with multi (greater than 2) endpoints. When false, each join attempt
			 * creates a new point-to-point session.
			 */
			public bool IsMultipoint
			{
				get
				{
					return (alljoyn_sessionopts_get_multipoint(_sessionOpts) == 1 ? true : false);
				}
				set
				{
					alljoyn_sessionopts_set_multipoint(_sessionOpts, (value) ? (int)1 : (int)0);
				}
			}

			/**
			 * Defines the proximity of the Session as Physical, Network, or Any.
			 */
			public ProximityType Proximity
			{
				get
				{
					return (ProximityType)alljoyn_sessionopts_get_proximity(_sessionOpts);
				}
				set
				{
					alljoyn_sessionopts_set_proximity(_sessionOpts, (byte)value);
				}
			}

			/** Allowed Transports  */
			public TransportMask Transports
			{
				get
				{
					return (TransportMask)alljoyn_sessionopts_get_transports(_sessionOpts);
				}
				set
				{
					alljoyn_sessionopts_set_transports(_sessionOpts, (ushort)value);
				}

			}
			#endregion

			/**
			 * Construct a SessionOpts with specific parameters.
			 *
			 * @param trafficType       Type of traffic.
			 * @param isMultipoint  true iff session supports multipoint (greater than two endpoints).
			 * @param proximity     Proximity constraint bitmask.
			 * @param transports    Allowed transport types bitmask.
			 */
			public SessionOpts(TrafficType trafficType, bool isMultipoint, ProximityType proximity, TransportMask transports)
			{
				_sessionOpts = alljoyn_sessionopts_create((byte)trafficType, isMultipoint ? 1 : 0, (byte)proximity, (ushort)transports);
			}

			internal SessionOpts(IntPtr sessionOpts)
			{
				_sessionOpts = sessionOpts;
				_isDisposed = true;
			}

			/**
			 * Determine whether this SessionOpts is compatible with the SessionOpts offered by other
			 *
			 * @param other  Options to be compared against this one.
			 * @return true iff this SessionOpts can use the option set offered by other.
			 */
			public bool IsCompatible(SessionOpts other)
			{
				return (alljoyn_sessionopts_iscompatible(_sessionOpts, other._sessionOpts) == 1 ? true : false);
			}

			/**
			 * Compare SessionOpts
			 *
			 * @param one the SessionOpts being compared to
			 * @param other the SessionOpts being compared against
			 * @return true if all of the SessionOpts parameters are the same
			 *
			 */
			public static int Compare(SessionOpts one, SessionOpts other)
			{
				return alljoyn_sessionopts_cmp(one._sessionOpts, other._sessionOpts);
			}

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private static extern IntPtr alljoyn_sessionopts_create(byte traffic, int isMultipoint,
				byte proximity, ushort transports);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_sessionopts_destroy(IntPtr opts);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern byte alljoyn_sessionopts_get_traffic(IntPtr opts);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_sessionopts_set_traffic(IntPtr opts, byte traffic);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_sessionopts_get_multipoint(IntPtr opts);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_sessionopts_set_multipoint(IntPtr opts, int isMultipoint);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern byte alljoyn_sessionopts_get_proximity(IntPtr opts);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_sessionopts_set_proximity(IntPtr opts, byte proximity);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern ushort alljoyn_sessionopts_get_transports(IntPtr opts);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern void alljoyn_sessionopts_set_transports(IntPtr opts, ushort transports);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_sessionopts_iscompatible(IntPtr one, IntPtr other);

			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_sessionopts_cmp(IntPtr one, IntPtr other);
			#endregion

			#region IDisposable
			~SessionOpts()
			{
				Dispose(false);
			}

			/**
			 * Dispose the SessionOpts
			 * @param disposing	describes if its activly being disposed
			 */
			protected virtual void Dispose(bool disposing)
			{
				if(!_isDisposed)
				{
					alljoyn_sessionopts_destroy(_sessionOpts);
					_sessionOpts = IntPtr.Zero;
				}
				_isDisposed = true;
			}

			/**
			 * Dispose the SessionOpts
			 */
			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this); 
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _sessionOpts;
				}
			}
			#endregion

			#region Data
			IntPtr _sessionOpts;
			bool _isDisposed = false;
			#endregion
		}
	}
}
