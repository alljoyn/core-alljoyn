/**
 * @file
 * Implementation of class for launching a bundled router
 */

/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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

#include <stdio.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/Log.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/FileStream.h>

#include <alljoyn/BusAttachment.h>

#include <alljoyn/Status.h>

#include "Bus.h"
#include "BusController.h"
#include "ConfigDB.h"
#include "Transport.h"
#include "TCPTransport.h"
#include "UDPTransport.h"

#include "NullTransport.h"
#include "PasswordManager.h"

#if defined(QCC_OS_ANDROID)
//#include "android/WFDTransport.h"
#endif

#define QCC_MODULE "ALLJOYN_ROUTER"

using namespace qcc;
using namespace std;
using namespace ajn;

static const char bundledConfig[] =
    "<busconfig>"
    "  <type>alljoyn_bundled</type>"
    "  <listen>unix:abstract=alljoyn</listen>"
#if defined(QCC_OS_DARWIN)
    "  <listen>launchd:env=DBUS_LAUNCHD_SESSION_BUS_SOCKET</listen>"
#endif
    "  <listen>tcp:iface=*,port=0</listen>"
    "  <listen>udp:iface=*,port=0</listen>"
    "  <limit name=\"auth_timeout\">20000</limit>"
    "  <limit name=\"max_incomplete_connections\">4</limit>"
    "  <limit name=\"max_completed_connections\">16</limit>"
    "  <limit name=\"max_untrusted_clients\">8</limit>"
    "  <flag name=\"restrict_untrusted_clients\">false</flag>"
    "</busconfig>";

class ClientAuthListener : public AuthListener {
  public:

    ClientAuthListener() : AuthListener(), maxAuth(2) { }

  private:

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {

        if (authCount > maxAuth) {
            return false;
        }

        printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);

        if (strcmp(authMechanism, PasswordManager::GetAuthMechanism().c_str()) == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword(PasswordManager::GetPassword());
            }
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_DbgPrintf(("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed"));
    }

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
    Mutex lock;
    std::set<NullTransport*> transports;
    ConfigDB* config;
#ifndef NDEBUG
    qcc::String configFile;
#endif
};

bool ExistFile(const char* fileName) {
    FILE* file = NULL;
    if (fileName && (file = fopen(fileName, "r"))) {
        fclose(file);
        return true;
    }
    return false;
}

/*
 * Create the singleton bundled router instance.
 *
 * Sidebar on starting a bundled router
 * ====================================
 *
 * How this works is via a fairly non-obvious mechanism, so we describe the
 * process here.  If it is desired to use the bundled router, the user (for
 * example bbclient or bbservice) includes this compilation unit.  Since the
 * following defines a C++ static initializer, an instance of the BundledRouter
 * object will be created before any call into a function in this file.  In
 * Linux, for example, this happens as a result of _init() being called before
 * the main() function of the program using the bundled router.  _init() loops
 * through the list of compilation units in link order and will eventually call
 * out to BundledRouter.cc:__static_initialization_and_destruction_0().  This is
 * the initializer function for this file which will then calls the constructor
 * for the BundledRouter object.  The constructor calls into a static method
 * (RegisterRouterLauncher) of the NullTransport to register itself as the
 * router to be launched.  This sets the stage for the use of the bundled
 * router.
 *
 * When the program using the bundled router tries to connect to a bus
 * attachment it calls BusAttachment::Connect().  This tries to connect to an
 * existing router first and if that connect does not succeed, it tries to
 * connect over the NullTransport to the bundled router.
 *
 * The NullTransport::Connect() method looks to see if it (the null transport)
 * is running, and if it is not it looks to see if it has a routerLauncher.
 * Recall that the constructor for the BundledRouter object registered itself as
 * a router launcher, so the null transport will find the launcher since it
 * included the object file corresponding to this source.  The null transport
 * then does a routerLauncher->Start() which calls back into the bundled router
 * object BundledRouter::Start() method below, providing the router with the
 * NullTransport pointer.  The Start() method brings up the bundled router and
 * links the routing node to the bus attachment using the provided null transport.
 *
 * So to summarize, one uses the bundled router simply by linking to the object
 * file corresponding to this source file.  This automagically creates a bundled
 * router static object and registers it with the null transport.  When trying
 * to connect to a router using a bus attachment in the usual way, if there is
 * no currently running native router process, the bus attachment will
 * automagically try to connect to a registered bundled router using the null
 * transport.  This will start the bundled router and then connect to it.
 *
 * The client uses the bundled router transparently -- it only has to link to it.
 *
 * Stopping the bundled router happens in the destructor for the C++ static
 * global object, again transparently to the client.
 *
 * It's pretty magical.
 */
static BundledRouter bundledRouter;

BundledRouter::BundledRouter() : transportsInitialized(false), stopping(false), ajBus(NULL), ajBusController(NULL)
{
    NullTransport::RegisterRouterLauncher(this);
    LoggerSetting::GetLoggerSetting("bundled-router");

    /*
     * Setup the config
     */
#ifndef NDEBUG
#if defined(QCC_OS_ANDROID)
    configFile = "/mnt/sdcard/.alljoyn/config.xml";
#else
    configFile = "./config.xml";
#endif
    qcc::String configStr = bundledConfig;
    if (ExistFile(configFile.c_str())) {
        configStr = "";
    } else {
        configFile = "";
    }
    config = new ConfigDB(configStr, configFile);
#else
    config = new ConfigDB(bundledConfig);
#endif
}

BundledRouter::~BundledRouter()
{
    QCC_DbgPrintf(("BundledRouter::~BundledRouter"));
    lock.Lock(MUTEX_CONTEXT);
    while (!transports.empty()) {
        set<NullTransport*>::iterator iter = transports.begin();
        NullTransport* trans = *iter;
        transports.erase(iter);
        lock.Unlock(MUTEX_CONTEXT);
        trans->Disconnect("null:");
        lock.Lock(MUTEX_CONTEXT);
    }
    lock.Unlock(MUTEX_CONTEXT);
    Join();
    delete config;
}

QStatus BundledRouter::Start(NullTransport* nullTransport)
{
    QStatus status = ER_OK;

    QCC_DbgHLPrintf(("Using BundledRouter"));

#ifndef NDEBUG
    if (configFile.size() > 0) {
        QCC_DbgHLPrintf(("Using external config file: %s", configFile.c_str()));
    }
#endif

    /*
     * If the bundled router is in the process of stopping we need to wait until the operation is
     * complete (BundledRouter::Join has exited) before we attempt to start up again.
     */
    lock.Lock(MUTEX_CONTEXT);
    while (stopping) {
        if (!transports.empty()) {
            assert(transports.empty());
        }
        lock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(5);
        lock.Lock(MUTEX_CONTEXT);
    }
    if (transports.empty()) {
        if (!config->LoadConfig()) {
            status = ER_BUS_BAD_XML;
            QCC_LogError(status, ("Error parsing configuration"));
            goto ErrorExit;
        }
        /*
         * Extract the listen specs
         */
        const ConfigDB::ListenList listenList = config->GetListen();
        String listenSpecs;
        for (ConfigDB::_ListenList::const_iterator it = listenList->begin(); it != listenList->end(); ++it) {
            if (!listenSpecs.empty()) {
                listenSpecs.append(";");
            }
            listenSpecs.append(*it);
        }
        /*
         * Register the transport factories - this is a one time operation
         */
        if (!transportsInitialized) {
            Add(new TransportFactory<TCPTransport>(TCPTransport::TransportName, false));
            Add(new TransportFactory<UDPTransport>(UDPTransport::TransportName, false));

#if defined(QCC_OS_ANDROID)
//            QCC_DbgPrintf(("adding WFD transport"));
//            Add(new TransportFactory<WFDTransport>(WFDTransport::TransportName, false));
#endif
            transportsInitialized = true;
        }
        QCC_DbgPrintf(("Starting bundled router bus attachment"));
        /*
         * Create and start the routing node
         */
        ajBus = new Bus("bundled-router", *this, listenSpecs.c_str());
        if (PasswordManager::GetAuthMechanism() != "ANONYMOUS" && PasswordManager::GetPassword() != "") {
            ajBusController = new BusController(*ajBus, &authListener);
        } else {
            ajBusController = new BusController(*ajBus, NULL);
        }

        status = ajBusController->Init(listenSpecs);
        if (ER_OK != status) {
            goto ErrorExit;
        }
    }
    /*
     * Use the null transport to link the routing node and client bus together
     */
    status = nullTransport->LinkBus(ajBus);
    if (status != ER_OK) {
        goto ErrorExit;
    }

    transports.insert(nullTransport);

    lock.Unlock(MUTEX_CONTEXT);

    return ER_OK;

ErrorExit:

    if (transports.empty()) {
        delete ajBusController;
        ajBusController = NULL;
        delete ajBus;
        ajBus = NULL;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

void BundledRouter::Join()
{
    QCC_DbgPrintf(("BundledRouter::Join"));
    lock.Lock(MUTEX_CONTEXT);
    if (transports.empty() && ajBus && ajBusController) {
        QCC_DbgPrintf(("Joining bundled router bus attachment"));
        ajBusController->Join();
        delete ajBusController;
        ajBusController = NULL;
        delete ajBus;
        ajBus = NULL;
        /*
         * Clear the stopping state
         */
        stopping = false;
    }
    lock.Unlock(MUTEX_CONTEXT);
}

QStatus BundledRouter::Stop(NullTransport* nullTransport)
{
    QCC_DbgPrintf(("BundledRouter::Stop"));
    lock.Lock(MUTEX_CONTEXT);
    transports.erase(nullTransport);
    QStatus status = ER_OK;
    if (transports.empty()) {
        /*
         * Set the stopping state to block any calls to Start until
         * after Join() has been called.
         */
        stopping = true;
        if (ajBusController) {
            status = ajBusController->Stop();
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

