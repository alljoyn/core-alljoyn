/**
 * @file
 * Contains the PermissionConfigurationListener class
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
#ifndef _ALLJOYN_PERMISSIONCONFIGURATIONLISTENER_H
#define _ALLJOYN_PERMISSIONCONFIGURATIONLISTENER_H

#include <alljoyn/Status.h>

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to
 * inform users of security permission related events.
 */
class PermissionConfigurationListener {
  public:
    /**
     * PermissionConfigurationListener constructor.
     */
    PermissionConfigurationListener() { }

    /**
     * PermissionConfigurationListener destructor.
     */
    virtual ~PermissionConfigurationListener() { }

    /**
     * Handler for doing a factory reset of application state.
     *
     * @return  Return ER_OK if the application state reset was successful.
     */
    virtual QStatus FactoryReset()
    {
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Notification that the security manager has updated the security policy
     * for the application.
     */
    virtual void PolicyChanged()
    {
    }

    /**
     * Notification provided before Security Manager is starting to change settings for this application.
     */
    virtual void StartManagement()
    {
    }

    /**
     * Notification provided after Security Manager finished changing settings for this application.
     */
    virtual void EndManagement()
    {
    }
};
}
#endif //_ALLJOYN_PERMISSIONCONFIGURATIONLISTENER_H