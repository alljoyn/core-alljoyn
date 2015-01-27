/**
 * @file
 * Bundled daemon bus attachment stress test
 * TODO: Check for status of every alljoyn public API.
 * TODO: Add all possible callbacks possiblke, disconnectcb, session -member left, session member joined, session lost, etc
 */

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
#include <signal.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Environ.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include <alljoyn/AboutObj.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Init.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>
#include <alljoyn/Status.h>

#define QCC_MODULE "BASTRESS2"

using namespace std;
using namespace qcc;
using namespace ajn;

enum OperationMode {
    Client = 0,
    Service = 1
};

class ClientBusListener;
class ClientAboutListener;
class ServiceBusListener;
class ThreadClass;

static bool s_useMultipointSessions = true;
static OperationMode s_operationMode;
static volatile sig_atomic_t g_interrupt = false;
static TransportMask s_transports = TRANSPORT_ANY;

static void CDECL_CALL SigIntHandler(int sig)
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
static uint32_t g_lastIntrospectedTimestamp = 0;

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
            QCC_LogError(status, ("Failed to register method handlers for BasicSampleObject"));
        }
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
            QCC_LogError(status, ("Ping: Error sending reply\n"));
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

        if (0 == strcmp(namePrefix, s_wellKnownName.c_str())) {

            // If  a number is odd, then join a session. else dont.
            if ((qcc::Rand32() % 2) == 0) { return; }

            mutex.Lock();
            bool shouldReturn = wasNameFoundAlready;
            if ((s_transports & transport) == transport) { wasNameFoundAlready = true; }
            mutex.Unlock();

            if (shouldReturn) {
                return;
            }

            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            owner->bus->EnableConcurrentCallbacks();
            cout << "[" << owner->name << "] FoundAdvertisedName(name=" << name << ", transport=" << transport << ")" << endl;

            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, s_useMultipointSessions, SessionOpts::PROXIMITY_ANY, s_transports);
            QStatus status = owner->bus->JoinSession(name, SERVICE_PORT, owner->clientBusListener, owner->sessionId, opts);
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSession to %s failed.", name));

            } else {
                cout << "JoinSession to " << name << " SUCCEEDED (Session id=" << owner->sessionId << ")" << endl;
                if (!owner->joinComplete) {
                    owner->joinComplete = true;
                    owner->discoveredServiceName = name;
                }

            }
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

        MyAboutData ad;
        ad.CreatefromMsgArg(aboutDataArg);

        char* appName;
        ad.GetAppName(&appName);

        if (appName != NULL && strcmp(g_testAboutApplicationName.c_str(), appName) == 0) {

            // If  a number is odd, then join a session. else dont.
            if ((qcc::Rand32() % 2) == 0) { return; }

            TransportMask transport;
            ad.GetTransportOpts(&transport);

            mutex.Lock();
            bool shouldReturn = wasNameFoundAlready;
            if ((s_transports & transport) == transport) { wasNameFoundAlready = true; }
            mutex.Unlock();

            if (shouldReturn) {
                return;
            }

            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            owner->bus->EnableConcurrentCallbacks();
            cout << "[" << owner->name << "] AnnounceSignal received(name=" << busName << ", transport=" << transport << ")" << endl;

            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, s_useMultipointSessions, SessionOpts::PROXIMITY_ANY, s_transports);
            QStatus status = owner->bus->JoinSession(busName, port, owner->clientBusListener, owner->sessionId, opts);
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSession to %s failed", busName));
            } else {
                cout << "JoinSession to " << busName << " SUCCEEDED (Session id=" << owner->sessionId << ")" << endl;

                if (!owner->joinComplete) {
                    owner->joinComplete = true;
                    owner->discoveredServiceName = busName;
                }
            }
        }
    }
  protected:
    ThreadClass* owner;
    Mutex mutex;
    bool wasNameFoundAlready;
};

class ServiceBusListener : public BusListener, public SessionPortListener {

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        cout << "Accepting join session request from " << joiner << " (opts.transports=" << opts.transports << ")" << endl;
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

inline void ThreadClass::ClientRun() {
    QStatus status = ER_OK;

    joinComplete = false;

    /* Register a bus listener in order to get discovery indications */
    clientBusListener = new ClientBusListener(this);
    bus->RegisterBusListener(*clientBusListener);

    clientAboutListener = new ClientAboutListener(this);
    bus->RegisterAboutListener(*clientAboutListener);
    const char* interfaces[] = { INTERFACE_NAME };

    if (g_useAboutFeatureDiscovery) {
        /* Begin discovery using About Who-implements */
        status = bus->WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
        if (status != ER_OK) {
            QCC_LogError(status, ("WhoImplements failed"));
        }
    } else {
        /* Begin name service based discovery on the well-known name of the service to be called */
        status = bus->FindAdvertisedNameByTransport(s_wellKnownName.c_str(), s_transports);
        if (status != ER_OK) {
            QCC_LogError(status, ("FindAdvertisedName failed"));
        }
    }

    //Sleep for random amount of time, so that you discover some names.
    bool limitReached = false;
    if (status == ER_OK) {
        /* Wait for join session to complete */
        int count = 0;
        int limit = 10 + (qcc::Rand32() % 50);
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
    }

    qcc::String serviceName = discoveredServiceName;
    //Make the method call.
    if ((status == ER_OK) && ((joinComplete && limitReached == false))) {

        ProxyBusObject remoteObj(*bus, serviceName.c_str(), SERVICE_PATH, sessionId);
        status = remoteObj.IntrospectRemoteObject();
        g_lastIntrospectedTimestamp = GetTimestamp();
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to introspect remote bus object.\n"));
        } else {
            cout << "Successfully introspected remote bus object." << endl;
        }

        Message reply(*bus);
        MsgArg inputs[2];
        inputs[0].Set("s", "Hello ");
        inputs[1].Set("s", "World!");
        status = remoteObj.MethodCall(INTERFACE_NAME, "cat", inputs, 2, reply, 5000);
        if (ER_OK == status) {
            cout << serviceName.c_str() << ".cat (path=" << SERVICE_PATH << ") returned \"" << reply->GetArg(0)->v_string.str << "\" " << endl;
        } else {
            QCC_LogError(status, ("MethodCall on %s.%s failed\n", serviceName.c_str(), "cat"));
        }

        status = bus->LeaveSession(sessionId);
        if (ER_OK != status) {
            QCC_LogError(status, ("LeaveSession failed."));
        }
    }

    if (g_useAboutFeatureDiscovery) {
        status = bus->CancelWhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
        if (status != ER_OK) {
            QCC_LogError(status, ("CancelWhoImplements failed "));
        }
    } else {
        status = bus->CancelFindAdvertisedName(s_wellKnownName.c_str());
        if (status != ER_OK) {
            QCC_LogError(status, ("CancelFindAdvertisedName failed "));
        }
    }

    if (clientBusListener) {
        bus->UnregisterBusListener(*clientBusListener);
    }

    if (clientAboutListener) {
        bus->UnregisterAboutListener(*clientAboutListener);
    }

    // If  a number is even,. do an explicit clean up of bus attachment, else just delete the bus attachment at the end.
    if ((qcc::Rand32() % 2) == 0) {
        //Stop bus attachment, cleanup and delete bus attachment
        bus->Disconnect();
        bus->Stop();
        bus->Join();
    }

    delete bus;

    if (clientBusListener) {
        delete clientBusListener;
    }

    if (clientAboutListener) {
        delete clientAboutListener;
    }

}

inline void ThreadClass::ServiceRun() {
    QStatus status = ER_OK;

    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = NULL;
    status = bus->CreateInterface(INTERFACE_NAME, testIntf);
    if (status == ER_OK) {
        testIntf->AddMethod("cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        testIntf->Activate();
    } else {
        QCC_LogError(status, ("Failed to create interface '%s'", INTERFACE_NAME));
    }

    /* Register a bus listener */
    serviceBusListener = new ServiceBusListener();
    bus->RegisterBusListener(*serviceBusListener);

    /* Register local objects */
    busObject = new BasicSampleObject(*bus, SERVICE_PATH);
    status = bus->RegisterBusObject(*busObject);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register the service bus object."));
    }

    /* Create session */
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, s_useMultipointSessions, SessionOpts::PROXIMITY_ANY, s_transports);
    SessionPort sp = SERVICE_PORT;
    status = bus->BindSessionPort(sp, opts, *serviceBusListener);
    if (ER_OK != status) {
        QCC_LogError(status, ("BindSessionPort failed"));
    }

    char buf[512];
    sprintf(buf, "%s.i%05d", s_wellKnownName.c_str(), qcc::Rand32() & 0xffff);
    qcc::String serviceName(buf);

    AboutObj* aboutObj = new AboutObj(*bus);

    if (g_useAboutFeatureDiscovery) {
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

        /* Request name */
        uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
        status = bus->RequestName(serviceName.c_str(), flags);
        if (ER_OK != status) {
            QCC_LogError(status, ("RequestName(%s) failed", s_wellKnownName.c_str()));
        }

        /* Advertise name */
        status = bus->AdvertiseName(serviceName.c_str(), opts.transports);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to advertise name %s", serviceName.c_str()));
        }
    }

    if (ER_OK == status) {
        int count = 0;
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
        status = aboutObj->Unannounce();
        if (status != ER_OK) {
            QCC_LogError(status, ("Error calling Unannounce."));
        }
    } else {
        status = bus->CancelAdvertiseName(serviceName.c_str(), opts.transports);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to cancel advertise name %s", serviceName.c_str()));
        }
    }

    if (busObject) {
        bus->UnregisterBusObject(*busObject);
    }

    if (serviceBusListener) {
        bus->UnregisterBusListener(*serviceBusListener);
    }

    // If  a number is even,. do an explicit clean up of bus attachment, else just delete the bus attachment at the end.
    if ((qcc::Rand32() % 2) == 0) {
        //Stop bus attachment, cleanup and delete bus attachment
        bus->Disconnect();
        bus->Stop();
        bus->Join();
    }

    if (busObject) {
        delete busObject;
    }

    if (serviceBusListener) {
        delete serviceBusListener;
    }

    if (aboutObj) {
        delete aboutObj;
    }

    delete bus;
}

inline qcc::ThreadReturn STDCALL ThreadClass::Run(void* arg) {

    bus = new BusAttachment(name.c_str(), true);
    QStatus status =  bus->Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment start failed"));
        return this;
    }

    qcc::String* connectArgs = static_cast<qcc::String*>(arg);
    // 'arg' is string value of env variable "BUS_ADDRESS"
    if (connectArgs->empty()) {
        status = bus->Connect();
    } else {
        status = bus->Connect(connectArgs->c_str());
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment connect failed"));
        return this;
    }

    // determine which operation mode we are running in
    if (s_operationMode == Client) {
        ClientRun();
    } else if (s_operationMode == Service) {
        ServiceRun();
    }

    return this;
}

static void usage(void)
{
    cout << "Options:" << endl;
    cout << "-h:			Print this help message again"<< endl;
    cout << "-threads #:                Number of threads, default is 5" << endl;
    cout << "-run-time #:               Run time of the program, Default is 10 minutes" << endl;
    cout << "-s:                        Stop the threads before joining them" << endl;
    cout << "-oc:                       Client mode of operation" << endl;
    cout << "-os:                       Service mode of operation, this is default" << endl;
    cout << "-p:                        point-to-point sessions, default is multipoint" << endl;
    cout << "-tcp:                      TCP transport for discovery and sessions" << endl;
    cout << "-udp:                      UDP transport for discovery and sessions" << endl;
    cout << "-local:			LOCAL transport for discovery and sessions, both instances of bastress2 needs to be connected to standalone routing node."<< endl;
    cout << "-n <well-known name>:      Well-known name to be requested and advertised" << endl;
    cout << "-about <interface name>:   Use the about feature for discovery" << endl;
    cout << endl;

    cout <<  "Example of bastress2 with regular discovery" << endl;
    cout <<  "-------------------------------------------" << endl;
    cout << "bastress2  -os -s -n hello.a  -run-time  600000  -tcp" << endl;
    cout << "bastress2  -oc -s -n hello.a  -run-time  600000  -tcp" << endl;
    cout << endl;

    cout <<  "Example of bastress2 with About discovery" << endl;
    cout <<  "------------------------------------------" << endl;
    cout << "bastress2  -os -s -about hello.a  -run-time  600000  -udp" << endl;
    cout << "bastress2  -oc -s -about hello.a  -run-time  600000  -udp" << endl;
    cout << endl;

}

/** Main entry point */
int main(int argc, char**argv)
{
    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        return 1;
    }
#endif
    QStatus status = ER_OK;
    uint32_t sleepTime = 600000;
    uint32_t threads = 5;
    bool stop = false;
    s_operationMode = Service;

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-threads", argv[i])) {
            ++i;
            if (i == argc) {
                cout << "option " << argv[i - 1] << " requires a parameter" << endl;
                usage();
                exit(1);
            } else {
                threads = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-run-time", argv[i])) {
            ++i;
            if (i == argc) {
                cout << "option " << argv[i - 1] << " requires a parameter" << endl;
                usage();
                exit(1);
            } else {
                sleepTime = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-s", argv[i])) {
            stop = true;
        } else if (0 == strcmp("-oc", argv[i])) {
            s_operationMode = Client;
        } else if (0 == strcmp("-os", argv[i])) {
            s_operationMode = Service;
        } else if (0 == strcmp("-p", argv[i])) {
            s_useMultipointSessions = false;
        } else if (0 == strcmp("-tcp", argv[i])) {
            s_transports = TRANSPORT_TCP;
        } else if (0 == strcmp("-udp", argv[i])) {
            s_transports = TRANSPORT_UDP;
        } else if (0 == strcmp("-local", argv[i])) {
            s_transports = TRANSPORT_LOCAL;
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                cout << "option " << argv[i - 1] << " requires a parameter" << endl;
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
            cout << "Unknown option: " << argv[i] << endl;
            exit(1);
        }
    }

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Get env vars */
    Environ* env = Environ::GetAppEnviron();
    qcc::String connectArgs = env->Find("BUS_ADDRESS");

    ThreadClass** threadList = new ThreadClass *[threads];
    uint32_t startTime = GetTimestamp();
    uint32_t endTime = GetTimestamp();

    while (!g_interrupt && ((endTime - startTime) < sleepTime)) {

        cout << "Starting batch of " << threads << " threads..." << endl;
        for (unsigned int i = 0; i < threads; i++) {
            char buf[20];
            sprintf(buf, "Thread.n%d", i);
            threadList[i] = new ThreadClass((char*)buf);
            threadList[i]->Start(&connectArgs);
        }

        if (stop) {
            /*
             * Sleep a random time before stopping of bus attachments is tested at different states of up and running
             */
            qcc::Sleep(1000 + (qcc::Rand32() % 4000));
            cout << "Stopping batch of " << threads << " threads..." << endl;
            for (unsigned int i = 0; i < threads; i++) {
                threadList[i]->Stop();
            }
        }

        for (unsigned int i = 0; i < threads; i++) {
            threadList[i]->Join();
            delete threadList[i];
        }
        endTime = GetTimestamp();
    }

    delete [] threadList;
    cout << "bastress2 exiting after timed completion of " << sleepTime << " ms " << endl;

    uint32_t last_introspection_time = endTime - g_lastIntrospectedTimestamp;

    if (s_operationMode == Client) {
        if (g_lastIntrospectedTimestamp && (last_introspection_time < 60000)) {
            cout << "Last introspection happened at " << last_introspection_time << " ms" << endl;
            cout << "PASSED" << endl;
        } else {
            cout << "Introspection did not happen in the last 1 minute." << endl;
            cout << "FAILED" << endl;
        }
    }

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
