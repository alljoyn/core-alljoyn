/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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