/**
 * @file
 * Implementation of class for launching a bundled daemon
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#include "DaemonConfig.h"
#include "Transport.h"
#include "TCPTransport.h"
#include "NullTransport.h"
#include "PasswordManager.h"

#if defined(AJ_ENABLE_ICE)
#include "DaemonICETransport.h"
#endif

#if defined(QCC_OS_WINRT)
#include "ProximityTransport.h"
#endif

#if defined(QCC_OS_ANDROID)
//#include "android/WFDTransport.h"
#endif

#define QCC_MODULE "ALLJOYN_DAEMON"

using namespace qcc;
using namespace std;
using namespace ajn;

static const char bundledConfig[] =
    "<busconfig>"
    "  <type>alljoyn_bundled</type>"
    "  <listen>tcp:r4addr=0.0.0.0,r4port=0</listen>"
#if defined(QCC_OS_ANDROID)
//    "  <listen>wfd:r4addr=0.0.0.0,r4port=9956</listen>"
#endif
    "  <limit auth_timeout=\"5000\"/>"
    "  <limit max_incomplete_connections=\"4\"/>"
    "  <limit max_completed_connections=\"16\"/>"
    "  <limit max_untrusted_clients=\"0\"/>"
    "  <property restrict_untrusted_clients=\"true\"/>"
    "  <ip_name_service>"
    "    <property interfaces=\"*\"/>"
    "    <property disable_directed_broadcast=\"false\"/>"
    "    <property enable_ipv4=\"true\"/>"
    "    <property enable_ipv6=\"true\"/>"
    "  </ip_name_service>"
    "  <tcp>"
//    "    <property router_advertisement_prefix=\"org.alljoyn.BusNode.\"/>"
    "  </tcp>"
#if defined(QCC_OS_WINRT)
//    "  <listen>proximity:addr=0::0,port=0,family=ipv6</listen>"
#endif
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

class BundledDaemon : public DaemonLauncher, public TransportFactoryContainer {

  public:

    BundledDaemon();

    ~BundledDaemon();

    /**
     * Launch the bundled daemon
     */
    QStatus Start(NullTransport* nullTransport);

    /**
     * Terminate the bundled daemon
     */
    QStatus Stop(NullTransport* nullTransport);

    /**
     * Wait for bundled daemon to exit
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
 * Create the singleton bundled daemon instance.
 *
 * Sidebar on starting a bundled daemon
 * ====================================
 *
 * How this works is via a fairly non-obvious mechanism, so we describe the
 * process here.  If it is desired to use the bundled daemon, the user (for
 * example bbclient or bbservice) includes this compilation unit.  Since the
 * following defines a C++ static initializer, an instance of the BundledDaemon
 * object will be created before any call into a function in this file.  In
 * Linux, for example, this happens as a result of _init() being called before
 * the main() function of the program using the bundled daemon.  _init() loops
 * through the list of compilation units in link order and will eventually call
 * out to BundledDaemon.cc:__static_initialization_and_destruction_0().  This is
 * the initializer function for this file which will then calls the constructor
 * for the BundledDaemon object.  The constructor calls into a static method
 * (RegisterDaemonLauncher) of the NullTransport to register itself as the
 * daemon to be launched.  This sets the stage for the use of the bundled
 * daemon.
 *
 * When the program using the bundled daemon tries to connect to a bus
 * attachment it calls BusAttachment::Connect().  This tries to connect to an
 * existing daemon first and if that connect does not succeed, it tries to
 * connect over the NullTransport to the bundled daemon.
 *
 * The NullTransport::Connect() method looks to see if it (the null transport)
 * is running, and if it is not it looks to see if it has a daemonLauncher.
 * Recall that the constructor for the BundledDaemon object registered itself as
 * a daemon launcher, so the null transport will find the launcher since it
 * included the object file corresponding to this source.  The null transport
 * then does a daemonLauncher->Start() which calls back into the bundled daemon
 * object BundledDaemon::Start() method below, providing the daemon with the
 * NullTransport pointer.  The Start() method brings up the bundled daemon and
 * links the daemon to the bus attachment using the provided null transport.
 *
 * So to summarize, one uses the bundled daemon simply by linking to the object
 * file corresponding to this source file.  This automagically creates a bundled
 * daemon static object and registers it with the null transport.  When trying
 * to connect to a daemon using a bus attachment in the usual way, if there is
 * no currently running native daemon process, the bus attachment will
 * automagically try to connect to a registered bundled daemon using the null
 * transport.  This will start the bundled daemon and then connect to it.
 *
 * The client uses the bundled daemon transparently -- it only has to link to it.
 *
 * Stopping the bundled daemon happens in the destructor for the C++ static
 * global object, again transparently to the client.
 *
 * It's pretty magical.
 */
static BundledDaemon bundledDaemon;

BundledDaemon::BundledDaemon() : transportsInitialized(false), stopping(false), ajBus(NULL), ajBusController(NULL)
{
    NullTransport::RegisterDaemonLauncher(this);
}

BundledDaemon::~BundledDaemon()
{
    QCC_DbgPrintf(("BundledDaemon::~BundledDaemon"));
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
}

QStatus BundledDaemon::Start(NullTransport* nullTransport)
{
    QStatus status = ER_OK;

    QCC_DbgHLPrintf(("Using BundledDaemon"));

    /*
     * If the bundled daemon is in the process of stopping we need to wait until the operation is
     * complete (BundledDaemon::Join has exited) before we attempt to start up again.
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
#if defined(QCC_OS_ANDROID)
        LoggerSetting::GetLoggerSetting("bundled-daemon", LOG_DEBUG, true, NULL);
#else
        LoggerSetting::GetLoggerSetting("bundled-daemon", LOG_DEBUG, false, stdout);
#endif

        /*
         * Load the configuration
         */
        DaemonConfig* config = NULL;
#ifndef NDEBUG
        qcc::String configFile = qcc::String::Empty;
    #if defined(QCC_OS_ANDROID)
        configFile = "/mnt/sdcard/.alljoyn/config.xml";
    #endif

    #if defined(QCC_OS_LINUX) || defined(QCC_OS_GROUP_WINDOWS) || defined(QCC_OS_GROUP_WINRT)
        configFile = "./config.xml";
    #endif

        if (!configFile.empty() && ExistFile(configFile.c_str())) {
            FileSource fs(configFile);
            if (fs.IsValid()) {
                config = DaemonConfig::Load(fs);
                if (!config) {
                    status = ER_BUS_BAD_XML;
                    QCC_LogError(status, ("Error parsing configuration from %s", configFile.c_str()));
                    goto ErrorExit;
                }
            }
        }
#endif
        if (!config) {
            config = DaemonConfig::Load(bundledConfig);
        }
        if (!config) {
            status = ER_BUS_BAD_XML;
            QCC_LogError(status, ("Error parsing configuration"));
            goto ErrorExit;
        }
        /*
         * Extract the listen specs
         */
        vector<String> listenList = config->GetList("listen");
        String listenSpecs = StringVectorToString(&listenList, ";");
        /*
         * Register the transport factories - this is a one time operation
         */
        if (!transportsInitialized) {
            Add(new TransportFactory<TCPTransport>(TCPTransport::TransportName, false));
#if defined(AJ_ENABLE_ICE)
            Add(new TransportFactory<DaemonICETransport>(DaemonICETransport::TransportName, false));
#endif
#if defined(QCC_OS_WINRT)
//            Add(new TransportFactory<ProximityTransport>(ProximityTransport::TransportName, false));
#endif
#if defined(QCC_OS_ANDROID)
//            QCC_DbgPrintf(("adding WFD transport"));
//            Add(new TransportFactory<WFDTransport>(WFDTransport::TransportName, false));
#endif
            transportsInitialized = true;
        }
        QCC_DbgPrintf(("Starting bundled daemon bus attachment"));
        /*
         * Create and start the daemon
         */
        ajBus = new Bus("bundled-daemon", *this, listenSpecs.c_str());
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
     * Use the null transport to link the daemon and client bus together
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

void BundledDaemon::Join()
{
    QCC_DbgPrintf(("BundledDaemon::Join"));
    lock.Lock(MUTEX_CONTEXT);
    if (transports.empty() && ajBus && ajBusController) {
        QCC_DbgPrintf(("Joining bundled daemon bus attachment"));
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

QStatus BundledDaemon::Stop(NullTransport* nullTransport)
{
    QCC_DbgPrintf(("BundledDaemon::Stop"));
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

