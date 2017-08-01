/*
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
 */
package org.alljoyn.bus.samples;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.OnJoinSessionListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.SessionOpts;

import java.lang.InterruptedException;
import java.util.concurrent.Semaphore;

public class SampleOnJoinSessionListener extends OnJoinSessionListener {

    private int mSessionId = 0;
    private boolean mIsConnected = false;

    public int getSessionId() {
        return mSessionId;
    }

    public boolean isConnected() {
        return mIsConnected;
    }

    @Override
    public void onJoinSession(Status status, int sessionId, SessionOpts opts, Object context) {
        System.out.println(String.format("BusAttachement.joinSession successful sessionId = %d", sessionId));
        mIsConnected = (status == Status.OK || status == Status.ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED);
        mSessionId = sessionId;
    }
}
