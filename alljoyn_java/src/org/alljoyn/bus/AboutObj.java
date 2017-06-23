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

package org.alljoyn.bus;


public class AboutObj {

    /**
     * Create native resources held by objects of this class.
     *
     * @param bus the BusAttachment this About BusObject will be registered with
     */
    public AboutObj(BusAttachment bus) {
        create(bus, false);
    }

    /**
     * Create native resources held by objects of this class.
     *
     * @param bus the BusAttachment this About BusObject will be registered with
     * @param isAboutObjectAnnounced if 'true' the org.alljoyn.About interface
     *        will be announced as part of the Announce signal
     */
    public AboutObj(BusAttachment bus, boolean isAboutObjectAnnounced) {
        create(bus, isAboutObjectAnnounced);
    }

    /**
     * Destroy native resources held by objects of this class.
     */
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    /**
     * Create any native resources held by objects of this class.  Specifically,
     * we allocate a C++ counterpart of this listener object.
     */
    private native void create(BusAttachment bus, boolean isAnnounced);

	/**
     * Release resources immediately.
     *
     * Normally, when all references to an object are removed, the Java
     * garbage collector will destroy it.  This doesn't happen immediately;
     * any resources held by the object will not be released until "some
     * time later" after the object is no longer referenced.
     *
     * Often, test programs recycle BusAttachments in a very short time. Thus,
     * they would quickly deplete scarce, underlying resources by depending
     * on the garbage collector to clean up.
     *
     * An explicit release() method allows such programs to release
     * the underlying resources immediately.  The garbage collector will still
     * call finalize, but the resources held by the underlying C++ objects will
     * go away immediately after release() is called.
     *
     * It is a programming error to call another method on the AboutObj
     * after the release() method has been called.
     */
    public void release() {
        destroy();
    }

    /**
     * Release any native resources held by objects of this class.
     * Specifically, we may delete a C++ counterpart of this object.
     */
    private native void destroy();

    public native Status announce(short sessionPort, AboutDataListener aboutData);

    /**
     * Cancel the last announce signal sent. If no signals have been sent this
     * method call will return.
     *
     * @return Status.OK on success
     */
    public native Status unannounce();

    /**
     * The opaque pointer to the underlying C++ object which is actually tied
     * to the AllJoyn code.
     */
    private long handle = 0;
}
