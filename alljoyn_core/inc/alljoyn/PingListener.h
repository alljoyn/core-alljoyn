/**
 * @file
 *
 * PingListener
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

#ifndef _ALLJOYN_PINGLISTENER_H_
#define _ALLJOYN_PINGLISTENER_H_

#ifndef __cplusplus
#error Only include PingListener.h in C++ code.
#endif

#include <qcc/String.h>
#include <qcc/Debug.h>

namespace ajn {

/**
 * PingListener base class
 */
class PingListener {
  public:
    /**
     * Destructor
     */
    virtual ~PingListener() { }

    /**
     * Destination Lost: called when the destination becomes unreachable.
     * Called once.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    virtual void DestinationLost(const qcc::String& group, const qcc::String& destination) {
        QCC_UNUSED(group);
        QCC_UNUSED(destination);
    }

    /**
     * Destination Found: called when the destination becomes reachable.
     * Called once.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    virtual void DestinationFound(const qcc::String& group, const qcc::String& destination) {
        QCC_UNUSED(group);
        QCC_UNUSED(destination);
    }
};
}
#endif /* PINGLISTENER_H_ */