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
#ifndef _ALLJOYN_JSIGNALHANDLER_H
#define _ALLJOYN_JSIGNALHANDLER_H

#include <jni.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>

class JSignalHandler : public ajn::MessageReceiver {
  public:
    JSignalHandler(jobject jobj, jobject jmethod);
    virtual ~JSignalHandler();
    bool IsSameObject(jobject jobj, jobject jmethod);
    virtual QStatus Register(ajn::BusAttachment& bus, const char* ifaceName, const char* signalName, const char* ancillary);
    virtual void Unregister(ajn::BusAttachment& bus) = 0;
    void SignalHandler(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);
  protected:
    jweak jsignalHandler;
    jobject jmethod;
    const ajn::InterfaceDescription::Member* member;
    qcc::String ancillary_data; /* can be both source or matchRule; */

  private:
    JSignalHandler(const JSignalHandler& other);
    JSignalHandler& operator =(const JSignalHandler& other);

};

#endif