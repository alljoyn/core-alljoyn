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
#ifndef _ALLJOYN_JPERMISSIONCONFIGURATIONLISTENER_H
#define _ALLJOYN_JPERMISSIONCONFIGURATIONLISTENER_H

#include <jni.h>
#include <alljoyn/PermissionConfigurationListener.h>

class JBusAttachment;

class JPermissionConfigurationListener : public ajn::PermissionConfigurationListener {
  public:

    JPermissionConfigurationListener(jobject jlistener);
    ~JPermissionConfigurationListener();
    QStatus FactoryReset();
    void PolicyChanged();
    void StartManagement();
    void EndManagement();

  private:

    JBusAttachment* busPtr;
    jobject jpcListener;
    jmethodID MID_factoryReset;
    jmethodID MID_policyChanged;
    jmethodID MID_startManagement;
    jmethodID MID_endManagement;
};

#endif