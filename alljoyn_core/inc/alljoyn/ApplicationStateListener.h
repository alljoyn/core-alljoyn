/**
 * @file
 * Contains the ApplicationStateListener class
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
#ifndef _ALLJOYN_APPLICATIONSTATELISTENER_H
#define _ALLJOYN_APPLICATIONSTATELISTENER_H

#include <alljoyn/PermissionConfigurator.h>

namespace ajn {

/**
 * Listener used to handle the security State signal.
 */
class ApplicationStateListener {
  public:
    /**
     * ApplicationStateListener constructor
     */
    ApplicationStateListener() { }

    /**
     * ApplicationStateListener destructor
     */
    virtual ~ApplicationStateListener() { }

    /**
     * Handler for the org.allseen.Bus.Application's State sessionless signal.
     *
     * @param[in] busName          unique name of the remote BusAttachment that
     *                             sent the State signal
     * @param[in] publicKeyInfo the application public key
     * @param[in] state the application state
     */
    virtual void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state) = 0;
};
}
#endif //_ALLJOYN_APPLICATIONSTATELISTENER_H