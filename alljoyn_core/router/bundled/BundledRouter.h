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
#ifndef _ALLJOYN_BUNDLEDROUTER_H
#define _ALLJOYN_BUNDLEDROUTER_H

#include <qcc/platform.h>

#include "NullTransport.h"
#include "PasswordManager.h"
#include "TransportFactory.h"

namespace ajn {

class Bus;
class BusController;
class ConfigDB;

class ClientAuthListener : public AuthListener {
  public:

    ClientAuthListener();

  private:

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds);

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success);

    uint32_t maxAuth;
};

class BundledRouter : public RouterLauncher, public TransportFactoryContainer {

  public:

    /**
     * Constructor.
     *
     * @param[in] configXml XML string with the configuration to be used by the router.
     *            If the parameter is an empty string, default configuration will be used.
     */
    BundledRouter(AJ_PCSTR configXml);

    ~BundledRouter();

    /**
     * Launch the bundled router
     */
    QStatus Start(NullTransport* nullTransport);

    /**
     * Terminate the bundled router
     */
    QStatus Stop(NullTransport* nullTransport);

    /**
     * Wait for bundled router to exit
     */
    void Join();

  private:

    bool transportsInitialized;
    bool stopping;
    Bus* ajBus;
    BusController* ajBusController;
    ClientAuthListener authListener;
    qcc::Mutex lock;
    std::set<NullTransport*> transports;
    ConfigDB* config;
#ifdef TEST_CONFIG
    qcc::String configFile;
#endif
};

}

#endif