////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNSignalHandlerImpl.h"

class AJNCChatObjectSignalHandlerImpl : public AJNSignalHandlerImpl {
  private:
    const ajn::InterfaceDescription::Member* chatSignalMember;

    /** Receive a signal from another Chat client */
    void ChatSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

  public:
    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNCChatObjectSignalHandlerImpl(id<AJNSignalHandler> aDelegate);

    virtual void RegisterSignalHandler(ajn::BusAttachment& bus);

    virtual void UnregisterSignalHandler(ajn::BusAttachment& bus);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNCChatObjectSignalHandlerImpl();
};