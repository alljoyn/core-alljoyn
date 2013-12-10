/**
 * @file
 * This file provides definitions for standard DBus interfaces
 *
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
		 * This file provides definitions for standard DBus interfaces
		 */
		public class DBus
		{
			[Flags]
			/**
			 * DBus RequestName input params.
			 * org.freedesktop.DBus.RequestName input params (see DBus spec)
			 */
			public enum NameFlags : uint
			{
				None = 0x00, /**< Do not change the default RequestName behavor */
				AllowReplacement = 0x01, /**< RequestName input flag: Allow others to take ownership of this name */
				ReplaceExisting = 0x02, /**< RequestName input flag: Attempt to take ownership of name if already taken */
				DoNotQueue = 0x04 /**< RequestName input flag: Fail if name cannot be immediately obtained */
			}

			/**
			 * DBus RequestName return values
			 * org.freedesktop.DBus.RequestName return values (see DBus spec)
			 */
			public enum RequestNameReply : uint
			{
				PrimaryOwner = 1, /**< The caller is now the primary owner of the name, replacing any previous owner. */
				InQueue = 2, /**< The name already had an owner and the name request has been added to a queue */
				Exists = 3, /**< The name already has an owner and the name request was not added to a queue */
				AlreadyOwner = 4 /**< The application trying to request ownership of a name is already the owener of it */
			}
		}
	}
}
