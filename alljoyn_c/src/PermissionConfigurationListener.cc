/**
 * @file
 *
 * This file implements a PermissionConfigurationListener subclass for use by the C API
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

#include <alljoyn/PermissionConfigurationListener.h>
#include <alljoyn_c/PermissionConfigurationListener.h>
#include <stdio.h>
#include <string.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

using namespace qcc;
using namespace std;

namespace ajn {
/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of bus related events.
 */
class PermissionConfigurationListenerCallbackC : public PermissionConfigurationListener {
  public:
    PermissionConfigurationListenerCallbackC(const alljoyn_permissionconfigurationlistener_callbacks* callbacks_in, const void* context_in)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, callbacks_in, sizeof(alljoyn_permissionconfigurationlistener_callbacks));
        context = context_in;
    }

    virtual QStatus FactoryReset()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.factory_reset != NULL) {
            return callbacks.factory_reset(context);
        }
        return ER_OK;
    }

    virtual void PolicyChanged()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.policy_changed != NULL) {
            callbacks.policy_changed(context);
        }
    }

    virtual void StartManagement()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.start_management != NULL) {
            callbacks.start_management(context);
        }
    }

    virtual void EndManagement()
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.end_management != NULL) {
            callbacks.end_management(context);
        }
    }

  private:
    alljoyn_permissionconfigurationlistener_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_permissionconfigurationlistener_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_permissionconfigurationlistener AJ_CALL alljoyn_permissionconfigurationlistener_create(const alljoyn_permissionconfigurationlistener_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_permissionconfigurationlistener) new ajn::PermissionConfigurationListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_permissionconfigurationlistener_destroy(alljoyn_permissionconfigurationlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QCC_ASSERT(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::PermissionConfigurationListenerCallbackC*)listener;
}