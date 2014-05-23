/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.about.sample;

import org.alljoyn.services.common.utils.GenericLogger;

/*
 * empty logger added to override the generic logger
 * This is empty because we don't want to see all the logs from the About
 * that would normally be logged by default.
 */
public class SampleLogger implements GenericLogger {

    @Override
    public void debug(String TAG, String msg) { }

    @Override
    public void info(String TAG, String msg) { }

    @Override
    public void warn(String TAG, String msg) { }

    @Override
    public void error(String TAG, String msg) { }

    @Override
    public void fatal(String TAG, String msg) { }
}
