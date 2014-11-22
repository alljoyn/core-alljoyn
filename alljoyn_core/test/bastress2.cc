/**
 * @file
 * Bundled daemon bus attachment stress test
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Environ.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/AboutObj.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"
#define THREAD_COUNT 5

using namespace std;
using namespace qcc;
using namespace ajn;

enum OperationMode {
    Default = 0,
    Client = 1,
    Service = 2
};

class ClientBusListener;
class ClientAboutListener;
class ServiceBusListener;
class ThreadClass;

static bool s_noDestruct = false;
static bool s_useMultipointSessions = true;
static OperationMode s_operationMode;
static volatile sig_atomic_t g_interrupt = false;
static TransportMask s_transports = TRANSPORT_ANY;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Bus.test.bastress";
static const char* DEFAULT_SERVICE_NAME = "org.alljoyn.Bus.test.bastress";
static const char* SERVICE_PATH = "/sample";
static const SessionPort SERVICE_PORT = 25;

static String s_wellKnownName = DEFAULT_SERVICE_NAME;

static String g_testAboutApplicationName = "bastress2";
static bool g_useAboutFeatureDiscovery = false;

class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(path)
    {
        /** Add the test interface to this object */
        const InterfaceDescription* exampleIntf = bus.GetInterface(INTERFACE_NAME);
        assert(exampleIntf);
        if (g_useAboutFeatureDiscovery) {
            AddInterface(*exampleIntf, ANNOUNCED);
        } else {
            AddInterface(*exampleIntf);
        }

        /** Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { exampleIntf->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&BasicSampleObject::Cat) }
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        if (ER_OK != status) {
            QCC_SyncPrintf("Failed to register method handlers for BasicSampleObject");
        }
    }

    void ObjectRegistered()
    {
        BusObject::ObjectRegistered();
        QCC_SyncPrintf("ObjectRegistered has been called\n");
    }


    void Cat(const InterfaceDescription::Member* member, Message& msg)
    {
        /* Concatenate the two input strings and reply with the result. */
        qcc::String inStr1 = msg->GetArg(0)->v_string.str;
        qcc::String inStr2 = msg->GetArg(1)->v_string.str;
        qcc::String outStr = inStr1 + inStr2;

        MsgArg outArg("s", outStr.c_str());
        QStatus status = MethodReply(msg, &outArg, 1);
        if (ER_OK != status) {
            QCC_SyncPrintf("Ping: Error sending reply\n");
        }
    }
};

class ThreadClass : public Thread {

  public:
    ThreadClass(char* name);

    friend class ClientBusListener;
    friend class ClientAboutListener;


  protected:
    bool joinComplete;
    ClientBusListener* clientBusListener;
    ServiceBusListener* serviceBusListener;
    ClientAboutListener* clientAboutListener;
    BusAttachment* bus;
    BusObject* busObject;
    SessionId sessionId;
    qcc::String discoveredServiceName;

    void DefaultRun();

    void ClientRun();

    void ServiceRun();

    qcc::ThreadReturn STDCALL Run(void* arg);

  private:
    String name;
};


class ClientBusListener : public BusListener, public SessionListener {
  public:
    ClientBusListener(ThreadClass* owner) : owner(owner), wasNameFoundAlready(false) { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, prefix=%s)\n", name, namePrefix);

        if (0 == strcmp(namePrefix, s_wellKnownName.c_str())) {

            mutex.Lock();
            bool shouldReturn = wasNameFoundAlready;
            if ((s_transports & transport) == transport) { wasNameFoundAlready = true; }
            mutex.Unlock();

            if (shouldReturn) {
                QCC_SyncPrintf("Will not form a session with(name=%s, prefix=%s) because we already joined a session.\n", name, namePrefix);
                return;
            }

            /* Only proceed further if we want to connect over the transport over which the FoundAdvertisedName
             * was received */
            if ((s_transports & transport) == 0) {
                QCC_SyncPrintf("We are not interested in connecting over a transport with mask 0x%x.\n", transport);
                return;
            }

            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            owner->bus->EnableConcurrentCallbacks();

            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, s_useMultipointSessions, SessionOpts::PROXIMITY_ANY, s_transports);
            QStatus status = owner->bus->JoinSession(name, SERVICE_PORT, owner->clientBusListener, owner->sessionId, opts);
            if (ER_OK != status) {
                QCC_SyncPrintf("JoinSession to %s failed (status=%s)\n", name, QCC_StatusText(status));

            } else {
                QCC_SyncPrintf("JoinSession to %s SUCCEEDED (Session id=%d)\n", name, owner->sessionId);

                if (!owner->joinComplete) {
                    owner->joinComplete = true;
                    owner->discoveredServiceName = name;
                }

            }
        }
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, s_wellKnownName.c_str()))) {
            QCC_SyncPrintf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n",
                           busName,
                           previousOwner ? previousOwner : "<none>",
                           newOwner ? newOwner : "<none>");
        }
    }

  protected:
    ThreadClass* owner;
    Mutex mutex;
    bool wasNameFoundAlready;
};

class MyAboutData : public AboutData {
  public:
    static const char* TRANSPORT_OPTS;

    MyAboutData() : AboutData() {
        // TRANSPORT_OPTS field is required, is announced, not localized
        SetNewFieldDetails(TRANSPORT_OPTS, REQUIRED | ANNOUNCED, "q");
    }

    MyAboutData(const char* defaultLanguage) : AboutData(defaultLanguage) {
        SetNewFieldDetails(TRANSPORT_OPTS, REQUIRED | ANNOUNCED, "q");
    }

    QStatus SetTransportOpts(TransportMask transportOpts)
    {
        QStatus status = ER_OK;
        MsgArg arg;
        status = arg.Set(GetFieldSignature(TRANSPORT_OPTS), transportOpts);
        if (status != ER_OK) {
            return status;
        }
        status = SetField(TRANSPORT_OPTS, arg);
        return status;
    }

    QStatus GetTransportOpts(TransportMask* transportOpts)
    {
        QStatus status;
        MsgArg* arg;
        status = GetField(TRANSPORT_OPTS, arg);
        if (status != ER_OK) {
            return status;
        }
        status = arg->Get(GetFieldSignature(TRANSPORT_OPTS), transportOpts);
        return status;
    }
};

const char* MyAboutData::TRANSPORT_OPTS = "TransportOpts";

static MyAboutData g_aboutData("en");

class ClientAboutListener : public AboutListener {
  public:
    ClientAboutListener(ThreadClass* owner) : owner(owner), wasNameFoundAlready(false) { }

    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        QCC_SyncPrintf("Announce Signal Received busName=%s\n", busName);

        MyAboutData ad;
        ad.CreatefromMsgArg(aboutDataArg);

        char* appName;
        ad.GetAppName(&appName);

        if (appName != NULL && strcmp(g_testAboutApplicationName.c_str(), appName) == 0) {
            TransportMask transport;
            ad.GetTransportOpts(&transport);

            mutex.Lock();
            bool shouldReturn = wasNameFoundAlready;
            if ((s_transports & transport) == transport) { wasNameFoundAlready = true; }
            mutex.Unlock();

            if (shouldReturn) {
                QCC_SyncPrintf("Will not form a session with(name=%s) because we already joined a session.\n", busName);
                return;
            }

            /* Only proceed further if we want to connect over the transport over which the FoundAdvertisedName
             * was received */
            if ((s_transports & transport) == 0) {
                QCC_SyncPrintf("We are not interested in connecting over a transport with mask 0x%x.\n", transport);
                return;
            }

            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            owner->bus->EnableConcurrentCallbacks();

            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, s_useMultipointSessions, SessionOpts::PROXIMITY_ANY, s_transports);
            QStatus status = owner->bus->JoinSession(busName, port, owner->clientBusListener, owner->sessionId, opts);
            if (ER_OK != status) {
                QCC_SyncPrintf("JoinSession to %s failed (status=%s)\n", busName, QCC_StatusText(status));

            } else {
                QCC_SyncPrintf("JoinSession to %s SUCCEEDED (Session id=%d)\n", busName, owner->sessionId);

                if (!owner->joinComplete) {
                    owner->joinComplete = true;
                    owner->discoveredServiceName = busName;
                }

            }
        }
    }
  private:
    ThreadClass* owner;
    Mutex mutex;
    bool wasNameFoundAlready;
};

class ServiceBusListener : public BusListener, public SessionPortListener {

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, s_wellKnownName.c_str()))) {
            QCC_SyncPrintf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n",
                           busName,
                           previousOwner ? previousOwner : "<none>",
                           newOwner ? newOwner : "<none>");
        }
    }
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            QCC_SyncPrintf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }
        QCC_SyncPrintf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
                       joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }
};

inline ThreadClass::ThreadClass(char* name) : Thread(name),
    joinComplete(false),
    clientBusListener(NULL),
    serviceBusListener(NULL),
    clientAboutListener(NULL),
    bus(NULL),
    busObject(NULL),
    sessionId(0),
    name(name)
{
}

inline void ThreadClass::DefaultRun() {
    char buf[256];
    // Don't use qcc::Rand16() because it can result in the same exact sequence
    // for multiple threads.
    sprintf(buf, "Thread.i%05d", qcc::Rand32() & 0xffff);
    QStatus status = bus->RequestName(name.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (status != ER_OK) {
        QCC_LogError(status, ("RequestName(%s) failed.", name.c_str()));
    }
    /* Begin Advertising the well-known name */
    status = bus->AdvertiseName(name.c_str(), s_transports);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not advertise (%s)", name.c_str()));
    }

    BusObject bo("/org/cool");
    bus->RegisterBusObject(bo);
    bus->UnregisterBusObject(bo);
    if (!s_noDestruct) {
        /* Cancel Advertising the well-known name */
        status = bus->CancelAdvertiseName(name.c_str(), s_transports);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not cancel advertising (%s)", name.c_str()));
        }

        delete bus;
        bus = NULL;
    }
}

inline void ThreadClass::ClientRun() {
    QStatus status = ER_OK;

    joinComplete = false;

    /* Register a bus listener in order to get discovery indications */
    clientBusListener = new ClientBusListener(this);
    bus->RegisterBusListener(*clientBusListener);
    QCC_SyncPrintf("ClientBusListener Registered.\n");



    if (g_useAboutFeatureDiscovery) {
        clientAboutListener = new ClientAboutListener(this);
        bus->RegisterAboutListener(*clientAboutListener);
        QCC_SyncPrintf("ClientAboutListener Registered.\n");
        const char* interfaces[] = { INTERFACE_NAME };
        status = bus->WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    } else {
        /* Begin discovery on the well-known name of the service to be called */
        status = bus->FindAdvertisedName(s_wellKnownName.c_str());
        if (status != ER_OK) {
            QCC_SyncPrintf("org.alljoyn.Bus.FindAdvertisedName failed (%s))\n", QCC_StatusText(status));
        }
    }

    /* Wait for join session to complete */
    int count = 0;
    // Don't use qcc::Rand16() because it can result in the same exact sequence
    // for multiple threads.
    int limit = 10 + (qcc::Rand32() % 50);
    bool limitReached = false;
    while (!joinComplete && !limitReached) {
        qcc::Sleep(100);
        if (count > limit) {
            limitReached = true;
        }
        count++;
        if (g_interrupt) {
            break;
        }
    }

    if (joinComplete && limitReached == false) {

        qcc::String serviceName = discoveredServiceName;

        ProxyBusObject remoteObj(*bus, serviceName.c_str(), SERVICE_PATH, sessionId);
        status = remoteObj.IntrospectRemoteObject();
        if (status != ER_OK) {
            QCC_SyncPrintf("Failed to introspect remote bus object.\n");
        } else {
            QCC_SyncPrintf("Successfully introspected remote bus object.\n");
        }

        Message reply(*bus);
        MsgArg inputs[2];
        inputs[0].Set("s", "Hello ");
        inputs[1].Set("s", "World!");
        status = remoteObj.MethodCall(INTERFACE_NAME, "cat", inputs, 2, reply, 5000);
        if (ER_OK == status) {
            QCC_SyncPrintf("%s.%s ( path=%s) returned \"%s\"\n", serviceName.c_str(), "cat",
                           SERVICE_PATH, reply->GetArg(0)->v_string.str);
        } else {
            QCC_SyncPrintf("MethodCall on %s.%s failed\n", serviceName.c_str(), "cat");
        }
    }


    bus->LeaveSession(sessionId);

    if (g_useAboutFeatureDiscovery) {
        clientAboutListener = new ClientAboutListener(this);
        bus->RegisterAboutListener(*clientAboutListener);
        QCC_SyncPrintf("ClientAboutListener Registered.\n");
        const char* interfaces[] = { INTERFACE_NAME };
        status = bus->CancelWhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    } else {
        /* Cancel discovery on the well-known name of the service */
        status = bus->CancelFindAdvertisedName(s_wellKnownName.c_str());
        if (status != ER_OK) {
            QCC_SyncPrintf("org.alljoyn.Bus.CancelFindAdvertisedName failed (%s))\n", QCC_StatusText(status));
        }
    }

    if (clientBusListener) {
        bus->UnregisterBusListener(*clientBusListener);
    }

    if (clientAboutListener) {
        bus->UnregisterAboutListener(*clientAboutListener);
    }

    if (!s_noDestruct) {
        /* Delete BusAttachment before deleting clientBusListener */
        delete bus;
        bus = NULL;
    }

    if (clientBusListener) {
        delete clientBusListener;
        clientBusListener = NULL;
    }

    if (clientAboutListener) {
        delete clientAboutListener;
        clientAboutListener = NULL;
    }

    QCC_SyncPrintf("client exiting with status %d (%s)\n", status, QCC_StatusText(status));

}

inline void ThreadClass::ServiceRun() {
    QStatus status = ER_OK;

    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = NULL;
    status = bus->CreateInterface(INTERFACE_NAME, testIntf);
    if (status == ER_OK) {
        QCC_SyncPrintf("Interface Created.\n");
        testIntf->AddMethod("cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        testIntf->Activate();
    } else {
        QCC_SyncPrintf("Failed to create interface '%s'\n", INTERFACE_NAME);
    }

    /* Register a bus listener */
    serviceBusListener = new ServiceBusListener();
    bus->RegisterBusListener(*serviceBusListener);

    /* Register local objects */
    busObject = new BasicSampleObject(*bus, SERVICE_PATH);
    status = bus->RegisterBusObject(*busObject);
    if (status != ER_OK) {
        QCC_SyncPrintf("Failed to register the service bus object.");
    }

    /* Create session */
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, s_useMultipointSessions, SessionOpts::PROXIMITY_ANY, s_transports);
    if (ER_OK == status) {
        SessionPort sp = SERVICE_PORT;
        status = bus->BindSessionPort(sp, opts, *serviceBusListener);
        if (ER_OK != status) {
            QCC_SyncPrintf("BindSessionPort failed (%s)\n", QCC_StatusText(status));
        }
    }

    /*
     * Advertise this service on the bus
     * There are three steps to advertising this service on the bus
     * 1) Request a well-known name that will be used by the client to discover
     *    this service
     * 2) Create a session
     * 3) Advertise the well-known name
     */
    char buf[512];
    // Don't use qcc::Rand16() because it can result in the same exact sequence
    // for multiple threads.
    sprintf(buf, "%s.i%05d", s_wellKnownName.c_str(), qcc::Rand32() & 0xffff);
    qcc::String serviceName(buf);

    AboutObj* aboutObj = new AboutObj(*bus);

    if (g_useAboutFeatureDiscovery) {
        QCC_SyncPrintf("------------------------------------------------------------\n");
        QCC_SyncPrintf("Service named %s is starting...\n", bus->GetUniqueName().c_str());
        QCC_SyncPrintf("------------------------------------------------------------\n");
        //AppId is a 128bit uuid
        uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                            0x1E, 0x82, 0x11, 0xE4,
                            0x86, 0x51, 0xD1, 0x56,
                            0x1D, 0x5D, 0x46, 0xB0 };
        g_aboutData.SetAppId(appId, 16);
        g_aboutData.SetDeviceName("DeviceName");
        //DeviceId is a string encoded 128bit UUID
        g_aboutData.SetDeviceId("1273b650-49bc-11e4-916c-0800200c9a66");
        g_aboutData.SetAppName(g_testAboutApplicationName.c_str());
        g_aboutData.SetManufacturer("AllSeen Alliance");
        g_aboutData.SetModelNumber("");
        g_aboutData.SetDescription("bastress2 is a test application used to verify AllJoyn functionality");
        // software version of bbservice is the same as the AllJoyn version
        g_aboutData.SetSoftwareVersion(ajn::GetVersion());
        g_aboutData.SetTransportOpts(s_transports);

        aboutObj->Announce(SERVICE_PORT, g_aboutData);
    } else {
        QCC_SyncPrintf("------------------------------------------------------------\n");
        QCC_SyncPrintf("Service named %s is starting...\n", buf);
        QCC_SyncPrintf("------------------------------------------------------------\n");

        /* Request name */
        if (ER_OK == status) {
            uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
            QStatus status = bus->RequestName(serviceName.c_str(), flags);
            if (ER_OK != status) {
                QCC_SyncPrintf("RequestName(%s) failed (status=%s)\n", s_wellKnownName.c_str(), QCC_StatusText(status));
            }
        }

        /* Advertise name */
        if (ER_OK == status) {
            status = bus->AdvertiseName(serviceName.c_str(), opts.transports);
            if (status != ER_OK) {
                QCC_SyncPrintf("Failed to advertise name %s (%s)\n", serviceName.c_str(), QCC_StatusText(status));
            }
        }
    }

    if (ER_OK == status) {
        int count = 0;
        // Don't use qcc::Rand16() because it can result in the same exact sequence
        // for multiple threads.
        int limit = 10 + (qcc::Rand32() % 50);
        bool limitReached = false;
        while (!limitReached) {
            qcc::Sleep(100);
            if (count > limit) {
                limitReached = true;
            }
            count++;
            if (g_interrupt) {
                break;
            }
        }
    }

    if (g_useAboutFeatureDiscovery) {
        QCC_SyncPrintf("------------------------------------------------------------\n");
        QCC_SyncPrintf("Service named %s is stopping...\n", bus->GetUniqueName().c_str());
        QCC_SyncPrintf("------------------------------------------------------------\n");
        aboutObj->Unannounce();
    } else {
        QCC_SyncPrintf("------------------------------------------------------------\n");
        QCC_SyncPrintf("Service named %s is stopping...\n", buf);
        QCC_SyncPrintf("------------------------------------------------------------\n");

        /* Cancel Advertise name */
        status = bus->CancelAdvertiseName(serviceName.c_str(), opts.transports);
        if (status != ER_OK) {
            QCC_SyncPrintf("Failed to cancel advertise name %s (%s)\n", serviceName.c_str(), QCC_StatusText(status));
        }
    }

    delete aboutObj;
    aboutObj = NULL;

    if (busObject) {
        bus->UnregisterBusObject(*busObject);
        delete busObject;
        busObject = NULL;
    }

    if (serviceBusListener) {
        bus->UnregisterBusListener(*serviceBusListener);
    }

    if (!s_noDestruct) {
        /* Delete BusAttachment before deleting serviceBusListener */
        delete bus;
        bus = NULL;
    }

    if (serviceBusListener) {
        delete serviceBusListener;
        serviceBusListener = NULL;
    }
}

inline qcc::ThreadReturn STDCALL ThreadClass::Run(void* arg) {

    bus = new BusAttachment(name.c_str(), true);
    QStatus status =  bus->Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("ThreadClass::Run failed"));
        return this;
    }

    qcc::String* connectArgs = static_cast<qcc::String*>(arg);
    // 'arg' is string value of env variable "BUS_ADDRESS"
    if (connectArgs->empty()) {
        status = bus->Connect();
    } else {
        status = bus->Connect(connectArgs->c_str());
    }

    // determine which operation mode we are running in
    //
    if (s_operationMode == Default) {
        DefaultRun();
    } else if (s_operationMode == Client) {
        ClientRun();
    } else if (s_operationMode == Service) {
        ServiceRun();
    }

    return this;
}

static void usage(void)
{
    QCC_SyncPrintf("Usage: bastress [-s] [-i <iterations>] [-t <threads>]\n\n");
    QCC_SyncPrintf("Options:\n");
    QCC_SyncPrintf("   -h                    = Print this help message\n");
    QCC_SyncPrintf("   -i                    = Number of iterations, default is 1000\n");
    QCC_SyncPrintf("   -t                    = Number of threads, default is 5\n");
    QCC_SyncPrintf("   -s                    = Stop the threads before joining them\n");
    QCC_SyncPrintf("   -d                    = Don't delete the bus attachments - implies \"-i 1\"r\n");
    QCC_SyncPrintf("   -oc                   = Operate in client mode\n");
    QCC_SyncPrintf("   -os                   = Operate in service mode\n");
    QCC_SyncPrintf("   -p                    = Use point-to-point sessions, default is multipoint\n");
    QCC_SyncPrintf("   -m <mask>             = Transport mask to use for client\n");
    QCC_SyncPrintf("   -u                    = Use UDP Transport for client\n");
    QCC_SyncPrintf("   -n <well-known-name>  = Well-known name to advertise\n");
    QCC_SyncPrintf("   -about [iface name]   = use the about feature for discovery.\n");
    QCC_SyncPrintf("\n");

}

/** Main entry point */
int main(int argc, char**argv)
{
    QStatus status = ER_OK;
    uint32_t iterations = 1000;
    uint32_t threads = 5;
    bool stop = false;
    s_operationMode = Default;

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-i", argv[i])) {
            ++i;
            if (i == argc) {
                QCC_SyncPrintf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                iterations = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-t", argv[i])) {
            ++i;
            if (i == argc) {
                QCC_SyncPrintf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                threads = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-d", argv[i])) {
            s_noDestruct = true;
        } else if (0 == strcmp("-s", argv[i])) {
            stop = true;
        } else if (0 == strcmp("-oc", argv[i])) {
            s_operationMode = Client;
        } else if (0 == strcmp("-os", argv[i])) {
            s_operationMode = Service;
        } else if (0 == strcmp("-p", argv[i])) {
            s_useMultipointSessions = false;
        } else if (0 == strcmp("-m", argv[i])) {
            ++i;
            if (i == argc) {
                QCC_SyncPrintf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                s_transports = static_cast<TransportMask>(StringToU32(argv[i], 16, TRANSPORT_ANY));
            }
        } else if (0 == strcmp("-u", argv[i])) {
            s_transports = TRANSPORT_UDP;
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                QCC_SyncPrintf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                s_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-about", argv[i])) {
            g_useAboutFeatureDiscovery = true;
            if ((i + 1) < argc && argv[i + 1][0] != '-') {
                ++i;
                g_testAboutApplicationName = argv[i];
            } else {
                g_testAboutApplicationName = "bastress2";
            }
        } else {
            usage();
            exit(1);
        }
    }

    if (s_noDestruct) {
        iterations = 1;
    }

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Get env vars */
    Environ* env = Environ::GetAppEnviron();
    qcc::String connectArgs = env->Find("BUS_ADDRESS");

    ThreadClass** threadList = new ThreadClass *[threads];

    while (!g_interrupt && iterations--) {

        QCC_SyncPrintf("Starting threads... \n");
        for (unsigned int i = 0; i < threads; i++) {
            char buf[256];
            sprintf(buf, "Thread.n%d", i);
            threadList[i] = new ThreadClass((char*)buf);
            threadList[i]->Start(&connectArgs);
            QCC_SyncPrintf("started threadList[%d]... \n", i);
        }

        if (stop) {
            /*
             * Sleep a random time before stopping of bus attachments is tested at different states of up and running
             * Don't use qcc::Rand16() because it can result in the same exact sequence for multiple threads.
             */
            qcc::Sleep(1000 + (qcc::Rand32() % 4000));
            QCC_SyncPrintf("stopping threads... \n");
            for (unsigned int i = 0; i < threads; i++) {
                QCC_SyncPrintf("stopping threadList[%d]... \n", i);
                threadList[i]->Stop();
            }
        }

        QCC_SyncPrintf("deleting threads... \n");
        for (unsigned int i = 0; i < threads; i++) {
            threadList[i]->Join();
            QCC_SyncPrintf("deleting threadList[%d]... \n", i);
            delete threadList[i];
        }

    }

    delete [] threadList;
    return (int) status;
}
