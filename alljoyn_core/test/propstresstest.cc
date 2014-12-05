/*
 * Copyright (c) 2014 AllSeen Alliance. All rights reserved.
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
 */

/**
 * Use a test program that registers/unregisters multiple Listeners and
 * continuously receives PropertiesChanged signals (i.e., the callback registered via
 * RegisterPropertiesChangedListener gets called repeatedly).
 * The test program is expected to run for a few hours and exit.
 * Run the debug build of test program for a couple of hours.
 * The test program should exit gracefully without any crashes or deadlocks.
 *
 * Start the client side as follows:
 * ./propstresstest -c [-n <name>] [-s <timeout>] [-o <nbrofobjects>]
 * Start the server side as follows:
 * ./propstresstest [-n <name>] [-s <timeout>] [-o <nbrofobjects>]
 *
 * <name> (optional) is the well known bus name. If not applied, a default will be used
 * <timeout> (optional) is the amount of time the executable will run (default=3600s)
 * <nbrofobjects> (optional) the number of objects used in the test (default=100)
 *
 * Running with valgrind (memcheck):
 * valgrind --tool=memcheck --leak-check=full ./propstresstest -c [-n <name>] [-s <timeout>] [-o <nbrofobjects>]
 * valgrind --tool=memcheck --leak-check=full ./propstresstest [-n <name>] [-s <timeout>] [-o <nbrofobjects>]
 *
 * Running with valgrind (memcheck):
 * valgrind --tool=massif ./propstresstest -c [-n <name>] [-s <timeout>] [-o <nbrofobjects>]
 * valgrind --tool=massif ./propstresstest [-n <name>] [-s <timeout>] [-o <nbrofobjects>]
 *
 */

#include <assert.h>
#include <signal.h>
#include <stdio.h>

#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/time.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>


using namespace ajn;
using namespace std;
using namespace qcc;


static volatile sig_atomic_t quit;
static const SessionPort PORT = 123;
static const SessionOpts SESSION_OPTS(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

static const char* propStressTestInterfaceXML =
    "<node name=\"/org/alljoyn/Testing/PropertyStressTest\">"
    "  <interface name=\"org.alljoyn.Testing.PropertyStressTest\">"
    "    <property name=\"int32\" type=\"i\" access=\"readwrite\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "    <property name=\"uint32\" type=\"u\" access=\"readwrite\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "    <property name=\"string\" type=\"s\" access=\"readwrite\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "  </interface>"
    "</node>";

static const char* obj_path = "/org/alljoyn/Testing/PropertyStressTest/";
static const char* interfaceName = "org.alljoyn.Testing.PropertyStressTest";
static const char* props[] = { "int32", "uint32", "string" };


class PropTesterObject : public BusObject {
  public:
    PropTesterObject(BusAttachment& bus, const char* path, SessionId id);
    ~PropTesterObject();

    using BusObject::Set;
    QStatus Set(int32_t int32Prop, uint32_t uint32Prop, const char* stringProp);

  private:
    int32_t int32Prop;    // RW property
    uint32_t uint32Prop;  // RW property
    String stringProp;    // RW property
    SessionId id;         // SessionId

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
};

PropTesterObject::PropTesterObject(BusAttachment& bus, const char* path, SessionId id) :
    BusObject(path),
    int32Prop(0),
    uint32Prop(0),
    stringProp(path),
    id(id)
{
    const InterfaceDescription* ifc = bus.GetInterface(interfaceName);
    if (!ifc) {
        bus.CreateInterfacesFromXml(propStressTestInterfaceXML);
        ifc = bus.GetInterface(interfaceName);
    }
    assert(ifc);

    AddInterface(*ifc);
}

PropTesterObject::~PropTesterObject()
{
}

QStatus PropTesterObject::Set(int32_t int32Prop, uint32_t uint32Prop, const char* stringProp)
{
    this->int32Prop = int32Prop;
    this->uint32Prop = uint32Prop;
    this->stringProp = stringProp;
    QCC_SyncPrintf("Emits properties changed for: \"%s\"\n", this->GetPath());
    return EmitPropChanged(interfaceName, props, ArraySize(props), id, 0);
}

QStatus PropTesterObject::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    QCC_SyncPrintf("Get properties of interface: \"%s\"\n", ifcName);
    if (strcmp(ifcName, interfaceName) == 0) {
        if (strcmp(propName, "int32") == 0) {
            val.Set("i", int32Prop);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%d) at %s\n", propName, int32Prop, GetPath());

        } else if (strcmp(propName, "uint32") == 0) {
            val.Set("u", uint32Prop);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%u) at %s\n", propName, uint32Prop, GetPath());

        } else if (strcmp(propName, "string") == 0) {
            val.Set("s", stringProp.c_str());
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%s) at %s\n", propName, stringProp.c_str(), GetPath());

        }
    }
    return status;
}


class _PropTesterProxyObject :
    public ProxyBusObject,
    private ProxyBusObject::PropertiesChangedListener {
  public:
    _PropTesterProxyObject(BusAttachment& bus, const String& service, const String& path, SessionId sessionId);
    ~_PropTesterProxyObject();

    void Register();
    void Unregister();

  private:
    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context);
};

typedef ManagedObj<_PropTesterProxyObject> PropTesterProxyObject;

_PropTesterProxyObject::_PropTesterProxyObject(BusAttachment& bus, const String& service, const String& path, SessionId sessionId) :
    ProxyBusObject(bus, service.c_str(), path.c_str(), sessionId)
{
    const InterfaceDescription* ifc = bus.GetInterface(interfaceName);
    if (!ifc) {
        bus.CreateInterfacesFromXml(propStressTestInterfaceXML);
        ifc = bus.GetInterface(interfaceName);
    }
    assert(ifc);

    AddInterface(*ifc);
    Register();
}

_PropTesterProxyObject::~_PropTesterProxyObject()
{
    Unregister();
}

void _PropTesterProxyObject::Register()
{
    RegisterPropertiesChangedListener(interfaceName, props, ArraySize(props), *this, NULL);
}

void _PropTesterProxyObject::Unregister()
{
    UnregisterPropertiesChangedListener(interfaceName, *this);
}

void _PropTesterProxyObject::PropertiesChanged(ProxyBusObject& obj,
                                               const char* ifaceName,
                                               const MsgArg& changed,
                                               const MsgArg& invalidated,
                                               void* context)
{
    MsgArg* entries;
    const char** propNames;
    size_t numEntries;
    size_t i;

    QCC_SyncPrintf("PropertiesChanged (bus name:    %s\n"
                   "                   object path: %s\n"
                   "                   interface:   %s)\n",
                   obj.GetServiceName().c_str(), obj.GetPath().c_str(), ifaceName);

    changed.Get("a{sv}", &numEntries, &entries);
    for (i = 0; i < numEntries; ++i) {
        const char* propName;
        MsgArg* propValue;
        entries[i].Get("{sv}", &propName, &propValue);
        String valStr = propValue->ToString();
        QCC_SyncPrintf("    Property Changed: %u/%u %s = %s \n",
                       (unsigned int)i + 1, (unsigned int)numEntries,
                       propName, valStr.c_str());
    }

    invalidated.Get("as", &numEntries, &propNames);
    for (i = 0; i < numEntries; ++i) {
        QCC_SyncPrintf("    Property Invalidated event: %u/%u %s\n",
                       (unsigned int)i + 1, (unsigned int)numEntries,
                       propNames[i]);
    }
}


class App {
  public:
    virtual ~App() { }
    virtual void Execute(uint64_t timeToRun) = 0;
};


class Service : public App, private SessionPortListener, private SessionListener {
  public:
    Service(BusAttachment& bus, int nbrOfObjects);
    ~Service();

    void Execute(uint64_t timeToRun);

  private:
    BusAttachment& bus;
    int nbrOfObjects;
    multimap<SessionId, PropTesterObject*> objects;
    SessionPort port;

    void Add(SessionId id, uint32_t number);

    // SessionPortListener methods
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);

    // SessionListener methods
    void SessionLost(SessionId sessionId);
};

Service::Service(BusAttachment& bus, int nbrOfObjects) :
    bus(bus),
    nbrOfObjects(nbrOfObjects),
    port(PORT)
{
    QStatus status = bus.BindSessionPort(port, SESSION_OPTS, *this);
    if (status != ER_OK) {
        QCC_SyncPrintf("Failed to bind session port \"%u\": %s\n", port, QCC_StatusText(status));
        exit(1);
    }
}

Service::~Service()
{
    bus.UnbindSessionPort(port);
    while (!objects.empty()) {
        delete objects.begin()->second;
        objects.erase(objects.begin());
    }
}

void Service::Add(SessionId id, uint32_t number)
{
    String path = obj_path;
    path += U32ToString(number);
    PropTesterObject* obj = new PropTesterObject(bus, path.c_str(), id);
    pair<SessionId, PropTesterObject*> item(id, obj);
    objects.insert(item);
    bus.RegisterBusObject(*obj);
    QCC_SyncPrintf("Added to bus: \"%s\"\n", path.c_str());
}

void Service::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    bus.SetSessionListener(id, this);
    for (int i = 0; i < nbrOfObjects; i++) {
        Add(id, i);
    }
}

void Service::SessionLost(SessionId sessionId)
{
    multimap<SessionId, PropTesterObject*>::iterator it;
    it = objects.find(sessionId);
    while (it != objects.end()) {
        bus.UnregisterBusObject(*(it->second));
        delete it->second;
        objects.erase(it);
        it = objects.find(sessionId);
    }
}

void Service::Execute(uint64_t timeToRun)
{
    uint64_t startTime = qcc::GetTimestamp64();
    uint64_t stopTime = qcc::GetTimestamp64();
    while ((timeToRun > (stopTime - startTime) / 1000) && !quit) {
        multimap<SessionId, PropTesterObject*>::iterator it;
        int32_t int32 = 0;
        uint32_t uint32 = 0;
        String string = "Test";
        for (it = objects.begin(); it != objects.end(); it++) {
            int32++;
            uint32++;
            string += "t";
            it->second->Set(int32, uint32, string.c_str());
        }
        qcc::Sleep(100);
        stopTime = qcc::GetTimestamp64();
    }
}


class Client : public App, private BusListener, private BusAttachment::JoinSessionAsyncCB {
  public:
    Client(BusAttachment& bus, int nbrOfObjects);
    ~Client();

    void Execute(uint64_t TimeToRun);

  private:
    BusAttachment& bus;
    int nbrOfObjects;
    multimap<SessionId, PropTesterProxyObject> objects;
    set<String> foundNames;
    Mutex lock;

    void Add(const String& name, SessionId id, uint32_t number);

    // BusListener methods
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);

    //BusAttachment::JoinsessionAsyncCB methods
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context);
};

Client::Client(BusAttachment& bus, int nbrOfObjects) :
    bus(bus), nbrOfObjects(nbrOfObjects)
{
    bus.RegisterBusListener(*this);
}

Client::~Client()
{
    while (!objects.empty()) {
        //delete objects.begin()->second;
        objects.erase(objects.begin());
    }
    bus.UnregisterBusListener(*this);
}


void Client::Add(const String& name, SessionId id, uint32_t number)
{
    String path = obj_path;
    path += U32ToString(number);
    PropTesterProxyObject obj(bus, name, path, id);
    pair<SessionId, PropTesterProxyObject> item(id, obj);
    objects.insert(item);
}

void Client::FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    QCC_SyncPrintf("FoundAdvertisedName: \"%s\"\n", name);
    String nameStr = name;
    lock.Lock();
    set<String>::iterator it = foundNames.find(nameStr);
    if (it == foundNames.end()) {
        QCC_SyncPrintf("Joining session with %s\n", name);
        bus.EnableConcurrentCallbacks();
        bus.JoinSessionAsync(name, PORT, NULL, SESSION_OPTS, this, new String(nameStr));
        foundNames.insert(nameStr);
    }
    lock.Unlock();
}

void Client::LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    QCC_SyncPrintf("LostAdvertisedName: \"%s\"\n", name);
    String nameStr = name;
    lock.Lock();
    set<String>::iterator it = foundNames.find(nameStr);
    if (it != foundNames.end()) {
        foundNames.erase(it);
    }
    lock.Unlock();
}

void Client::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context)
{
    String* nameStr = reinterpret_cast<String*>(context);
    QCC_SyncPrintf("JoinSessionCB: name = %s   status = %s\n", nameStr->c_str(), QCC_StatusText(status));
    if (status == ER_OK) {
        lock.Lock();
        for (int i = 0; i < nbrOfObjects; i++) {
            QCC_SyncPrintf("Adding ProxyBusObject for name = %s\n", nameStr->c_str());
            Add(*nameStr, sessionId, i);
            QCC_SyncPrintf("Added ProxyBusObject for name = %s\n", nameStr->c_str());
        }
        lock.Unlock();
    }
    delete nameStr;
}

void Client::Execute(uint64_t timeToRun)
{
    uint64_t startTime = qcc::GetTimestamp64();
    uint64_t stopTime = qcc::GetTimestamp64();
    bool seed = false;
    QCC_SyncPrintf("Start execution\n");
    while ((timeToRun > (stopTime - startTime) / 1000) && !quit) {
        multimap<SessionId, PropTesterProxyObject>::iterator it;
        bool unregister = seed;
        seed = !unregister;
        QCC_SyncPrintf("Seed = %d\n", seed);
        lock.Lock();
        for (it = objects.begin(); it != objects.end(); it++) {
            if (unregister) {
                QCC_SyncPrintf("Unregister\n");
                it->second->Unregister();
            } else {
                QCC_SyncPrintf("Register\n");
                it->second->Register();
            }
            unregister = !unregister;
        }
        lock.Unlock();
        qcc::Sleep(1000);
        stopTime = qcc::GetTimestamp64();
    }
}


void SignalHandler(int sig) {
    if ((sig == SIGINT) ||
        (sig == SIGTERM)) {
        quit = 1;
    }
}

void Usage()
{
    printf("propstresstest: [ -c ] [ -n <NAME> ] [ -s <SECONDS> ]\n"
           "    -c            Run as client (runs as service by default).\n"
           "    -n <NAME>     Use <NAME> for well known bus name.\n"
           "    -s <SEC>      Run for <SEC> seconds.\n"
           "    -o <NBR>      Create <NBR> objects.\n");
}

int main(int argc, char** argv)
{
    String serviceName = "org.alljoyn.Testing.PropertyStressTest";
    bool client = false;
    uint64_t timeToRun = 3600;
    int nbrOfObjects = 100;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            client = true;
        } else if (strcmp(argv[i], "-n") == 0) {
            ++i;
            if ((i == argc) || strchr(argv[i], '-')) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                Usage();
                exit(1);
            } else {
                serviceName = argv[i];
            }
        } else if (strcmp(argv[i], "-s") == 0) {
            ++i;
            if ((i == argc) || strchr(argv[i], '-')) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                Usage();
                exit(1);
            } else {
                timeToRun = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            ++i;
            if ((i == argc) || strchr(argv[i], '-')) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                Usage();
                exit(1);
            } else {
                nbrOfObjects = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], "-h") == 0) {
            Usage();
            exit(1);
        } else {
            Usage();
            exit(1);
        }
    }


    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    quit = 0;

    QStatus status;
    int ret = 0;
    BusAttachment bus("PropertyStressTest", true);
    Environ* env = Environ::GetAppEnviron();
    String connSpec = env->Find("DBUS_STARTER_ADDRESS");

    if (connSpec.empty()) {
#if defined(QCC_OS_GROUP_WINDOWS)
    #if (_WIN32_WINNT > 0x0603)
        connSpec = env->Find("BUS_ADDRESS", "npipe:");
    #else
        connSpec = env->Find("BUS_ADDRESS", "tcp:addr=127.0.0.1,port=9956");
    #endif
#else
        connSpec = env->Find("BUS_ADDRESS", "unix:abstract=alljoyn");
#endif
    }

    status = bus.Start();
    if (status != ER_OK) {
        printf("Failed to start bus attachment: %s\n", QCC_StatusText(status));
        exit(1);
    }

    status = bus.Connect(connSpec.c_str());
    if (status != ER_OK) {
        printf("Failed to connect to \"%s\": %s\n", connSpec.c_str(), QCC_StatusText(status));
        exit(1);
    }

    App* app;

    if (client) {
        app = new Client(bus, nbrOfObjects);
        status = bus.FindAdvertisedName(serviceName.c_str());
        if (status != ER_OK) {
            printf("Failed to find name to \"%s\": %s\n", serviceName.c_str(), QCC_StatusText(status));
            ret = 2;
            goto exit;
        }
    } else {
        serviceName += ".A" + bus.GetGlobalGUIDString();
        app = new Service(bus, nbrOfObjects);
        status = bus.RequestName(serviceName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        if (status != ER_OK) {
            printf("Failed to request name to \"%s\": %s\n", serviceName.c_str(), QCC_StatusText(status));
            ret = 2;
            goto exit;
        }
        status = bus.AdvertiseName(serviceName.c_str(), TRANSPORT_ANY);
        if (status != ER_OK) {
            printf("Failed to request name to \"%s\": %s\n", serviceName.c_str(), QCC_StatusText(status));
            ret = 2;
            goto exit;
        }
    }

    app->Execute(timeToRun);
    printf("QUITTING\n");

exit:
    if (client) {
        bus.CancelFindAdvertisedName(serviceName.c_str());
        bus.Disconnect(connSpec.c_str());
    } else {
        bus.CancelAdvertiseName(serviceName.c_str(), TRANSPORT_ANY);
        bus.ReleaseName(serviceName.c_str());
    }

    delete app;

    bus.Stop();
    bus.Join();

    return ret;
}
