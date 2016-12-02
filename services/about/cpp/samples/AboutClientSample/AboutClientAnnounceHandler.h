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

#ifndef ABOUTCLIENTANNOUNCEHANDLER_H_
#define ABOUTCLIENTANNOUNCEHANDLER_H_

#include <alljoyn/about/AnnounceHandler.h>

typedef void (*AnnounceHandlerCallback)(qcc::String const& busName, unsigned short port);

class AboutClientAnnounceHandler : public ajn::services::AnnounceHandler {

  public:

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData);

    AboutClientAnnounceHandler(AnnounceHandlerCallback callback = 0);

    ~AboutClientAnnounceHandler();

  private:

    AnnounceHandlerCallback m_Callback;
};

#endif /* ABOUTCLIENTANNOUNCEHANDLER_H_ */