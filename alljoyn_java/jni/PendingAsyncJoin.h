/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 *
 ******************************************************************************/
#ifndef _ALLJOYN_PENDINGASYNCJOIN_H
#define _ALLJOYN_PENDINGASYNCJOIN_H

#include <jni.h>

/**
 * A C++ class to hold the Java object references required for an asynchronous
 * join operation while AllJoyn mulls over what it can do about the operation.
 *
 * An instance of this class is given to the C++ JoinSessionAsync method as the
 * context object.  Note well that the context object passed around in the C++
 * side of things is *not* the same as the Java context object passed into
 * joinSessionAsync.
 *
 * Another thing to keep in mind is that since the Java objects have been taken
 * into the JNI fold, they are referenced by JNI global references to the
 * objects provided by Java which may be different than the references seen by
 * the Java code.  Compare using JNI IsSameObject() to see if they are really
 * referencing the same object..
 */
class PendingAsyncJoin {
  public:
    PendingAsyncJoin(jobject jsessionListener, jobject jonJoinSessionListener, jobject jcontext) {
        this->jsessionListener = jsessionListener;
        this->jonJoinSessionListener = jonJoinSessionListener;
        this->jcontext = jcontext;
    }
    jobject jsessionListener;
    jobject jonJoinSessionListener;
    jobject jcontext;
  private:
    PendingAsyncJoin(const PendingAsyncJoin& other);
    PendingAsyncJoin& operator =(const PendingAsyncJoin& other);
};

#endif