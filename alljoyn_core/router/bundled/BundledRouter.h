/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

    BundledRouter();

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
