#ifndef _ALLJOYN_OBSINTERNAL_H
#define _ALLJOYN_OBSINTERNAL_H
/**
 * @file
 *
 * This file defines internal state for a BusAttachment
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#ifndef __cplusplus
#error Only include CoreObserver.h in C++ code.
#endif

#include <set>
#include <map>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/ManagedObj.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/InterfaceDescription.h>

namespace ajn {

class CoreObserver {
  public:
    typedef std::set<qcc::String> InterfaceSet;
    InterfaceSet mandatory;

    CoreObserver(InterfaceSet mandatory) : mandatory(mandatory) { }

    virtual ~CoreObserver() { }

    /* interface towards ObserverManager */
    virtual void ObjectDiscovered(const ObjectId& oid, const InterfaceSet& interfaces, SessionId sessionid) = 0;
    virtual void ObjectLost(const ObjectId& oid) = 0;
    /**
     * Enable all disabled listeners for this observer.
     *
     * Called from the ObserverManager work queue to make sure the
     * initial callbacks of triggerOnExisting listeners are called
     * from the local endpoint dispatcher threads.
     */
    virtual void EnablePendingListeners() = 0;

};

}


#endif