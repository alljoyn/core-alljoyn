/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

package org.alljoyn.services.common;

import org.alljoyn.bus.ErrorReplyBusException;

/**
 * NOTE: The Common Service classes were incorporated from the now deprecated AJCore About Services project,
 *       so that Config Services no longer depends on alljoyn_about.jar.
 *       This specific class continues to be unnecessary. Remove in release 17.04.
 *
 * A BusException that is thrown when a remote client uses an unsupported language to get/set data.
 * @deprecated see org.alljoyn.bus.ErrorReplyBusException
 */
@Deprecated
public class LanguageNotSupportedException extends ErrorReplyBusException {
    private static final long serialVersionUID = -8150015063069797494L;

    /**
     * Constructor
     * @deprecated
     */
    @Deprecated
    public LanguageNotSupportedException() {
        super("org.alljoyn.Error.LanguageNotSupported", "The language specified is not supported");
    }
}
