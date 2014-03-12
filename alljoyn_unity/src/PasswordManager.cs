/**
 * @file
 * This file defines the PasswordManager class that provides the interface to
 * set credentials used for the authentication of thin clients.
 */

/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
using System;
using System.Runtime.InteropServices;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * Class to allow the user or application to set credentials used for the authentication
		 * of thin clients.
		 * Before invoking Connect() to BusAttachment, the application should call SetCredentials
		 * if it expects to be able to communicate to/from thin clients.
		 * The bundled daemon will start advertising the name as soon as it is started and MUST have
		 * the credentials set to be able to authenticate any thin clients that may try to use the
		 * bundled daemon to communicate with the app.
		 */
		public static class PasswordManager
		{
			/**
			 * Set credentials used for the authentication of thin clients.
			 *
			 * @param authMechanism  Mechanism to use for authentication.
			 * @param password       Password to use for authentication.
			 *
			 * @return   Returns QStatus.OK if the credentials was successfully set.
			 */
			public static QStatus SetCredentials(string authMechanism, string password)
			{
				return alljoyn_passwordmanager_setcredentials(authMechanism, password);
			}

			#region DLL Imports
			[DllImport(DLL_IMPORT_TARGET)]
			private static extern int alljoyn_passwordmanager_setcredentials([MarshalAs(UnmanagedType.LPStr)] string authMechanism,
																			[MarshalAs(UnmanagedType.LPStr)] string password);
			#endregion
		}
	}
}