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
#ifndef _ALLJOYN_PENDINGASYNCPING_H
#define _ALLJOYN_PENDINGASYNCPING_H

#include <jni.h>

/**
 * A C++ class to hold the Java object references required for an asynchronous
 * ping operation while AllJoyn mulls over what it can do about the operation.
 *
 * An instance of this class is given to the C++ PingAsync method as the context
 * object.  Note well that the context object passed around in the C++ side of
 * things is **not** the same as the Java context object passed into pingAsync.
 *
 * Another thing to keep in mind is that since the Java objects have been taken
 * into the JNI fold, they are referenced by JNI global references to the
 * objects provided by Java which may be different than the references seen by
 * the Java code.  Compare using JNI IsSameObject() to see if they are really
 * referencing the same object..
 */
class PendingAsyncPing {
  public:
    PendingAsyncPing(jobject jonPingListener, jobject jcontext) {
        this->jonPingListener = jonPingListener;
        this->jcontext = jcontext;
    }
    jobject jonPingListener;
    jobject jcontext;
  private:
    /**
     * private copy constructor this object can not be copied or assigned
     */
    PendingAsyncPing(const PendingAsyncPing& other);
    /**
     * private assignment operator this object can not be copied or assigned
     */
    PendingAsyncPing& operator =(const PendingAsyncPing& other);
};

#endif