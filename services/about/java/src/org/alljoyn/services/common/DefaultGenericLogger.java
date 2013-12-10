/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.services.common;

import org.alljoyn.services.common.utils.GenericLogger;

/**
 * An Android implementation of the {@link GenericLogger} class  
 */
public class DefaultGenericLogger implements GenericLogger {

	@Override
	public void warn(String TAG, String msg)
	{
		System.out.println(new StringBuilder(TAG).append("Warning: ").append(msg).toString());
	}
	
	@Override
	public void info(String TAG, String msg)
	{
		System.out.println(new StringBuilder(TAG).append("Info: ").append(msg).toString());
	}
	
	@Override
	public void fatal(String TAG, String msg)
	{
		System.out.println(new StringBuilder(TAG).append("WTF: ").append(msg).toString());
	}
	
	@Override
	public void error(String TAG, String msg)
	{
		System.out.println(new StringBuilder(TAG).append("Error: ").append(msg).toString());
	}
	
	@Override
	public void debug(String TAG, String msg)
	{
		System.out.println(new StringBuilder(TAG).append("Debug: ").append(msg).toString());
	}
}
