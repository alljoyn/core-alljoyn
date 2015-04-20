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
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <thread>
#include <map>
#include <thread>
#include <cstdio>
#include <sstream>
#include <sys/wait.h>
#include <assert.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Debug.h>
#include <qcc/time.h>
#include <qcc/Environ.h>
#include <alljoyn/Init.h>
#include <alljoyn/Status.h>
#include <qcc/platform.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/AboutObj.h>

using namespace ajn;
using namespace std;
using namespace qcc;

#define QCC_MODULE "ALLJOYN_OBSERVER_TEST"

#define METHOD  "Identify"
#define METHOD_OBSUPDATER  "Updater"

#define PATH_PREFIX "/test/"

#define DEFAULT_WAIT_MS 3000

static qcc::String getConnectArg(const char* envvar = "BUS_ADDRESS") {
    qcc::Environ* env = Environ::GetAppEnviron();
#if defined(QCC_OS_GROUP_WINDOWS)
    #if (_WIN32_WINNT > 0x0603)
    return env->Find(envvar, "npipe:");
    #else
    return env->Find(envvar, "tcp:addr=127.0.0.1,port=9956");
    #endif
#else
    return env->Find(envvar, "unix:abstract=alljoyn");
#endif
}

class MultiPeerTestObject : public BusObject {
  public:
    MultiPeerTestObject(BusAttachment& bus, qcc::String path, String _interfaces) :
        BusObject(path.c_str()), bus(bus), path(path), interface(_interfaces) {
        busname = bus.GetUniqueName();
        const InterfaceDescription* intf = bus.GetInterface(interface.c_str());
        assert(intf != NULL);
        AddInterface(*intf, ANNOUNCED);

        QStatus status;

        status = AddMethodHandler(intf->GetMember(METHOD), static_cast<MessageReceiver::MethodHandler>(&MultiPeerTestObject::HandleIdentify));
        assert(status == ER_OK);
        status = AddMethodHandler(intf->GetMember(METHOD_OBSUPDATER),
                                  static_cast<MessageReceiver::MethodHandler>(&MultiPeerTestObject::UpdateObservedSoFarBy));
        assert(status == ER_OK);
        QCC_UNUSED(status);
    }

    virtual ~MultiPeerTestObject() { }

    void HandleIdentify(const InterfaceDescription::Member* member, Message& message) {
        MsgArg args[2] = { MsgArg("s", busname.c_str()), MsgArg("s", path.c_str()) };
        QStatus status = MethodReply(message, args, 2);
        assert(ER_OK == status);
        QCC_UNUSED(status);
    }

    void UpdateObservedSoFarBy(const InterfaceDescription::Member* member, Message& message) {
        observedSoFarByLock.Lock(MUTEX_CONTEXT);
        const MsgArg* op = message->GetArg(0);
        const MsgArg* observerPID = message->GetArg(1);
        assert(observerPID != NULL);
        assert(op != NULL);
        pid_t pid;
        observerPID->Get("u", &pid);
        char* option;
        op->Get("s", &option);

        if (strcmp(option, "insert") == 0) {
            assert(observedSoFarBy.find(pid) == observedSoFarBy.end());     // Double observation sanity
            observedSoFarBy.insert(pid);
            QCC_DbgPrintf(("Object %s is observed (also) by %d", path.c_str(), (int)pid));
        } else {
            QCC_LogError(ER_FAIL, ("Invalid option !"));
        }
        observedSoFarByLock.Unlock(MUTEX_CONTEXT);
    }

    BusAttachment& bus;
    qcc::String busname;
    qcc::String path;
    String interface;
    set<pid_t> observedSoFarBy;
    Mutex observedSoFarByLock;
};

class Participant : public SessionPortListener, public SessionListener {
  public:

    BusAttachment bus;
    String uniqueBusName;

    SessionOpts opts;
    SessionPort port;

    AboutData aboutData;
    AboutObj aboutObj;

    pid_t parentPID;
    qcc::String intfName;

    Participant(qcc::String _intfName) : bus("Participant", true),
        opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        port(42), aboutData("en"), aboutObj(bus), intfName(_intfName)
    {
        StartBus();

        /* create interface */
        InterfaceDescription* intf = NULL;
        QStatus status = bus.CreateInterface(intfName.c_str(), intf);
        assert(ER_OK == status);
        assert(intf != NULL);
        status = intf->AddMethod(METHOD, "", "ss", "busname,path");
        assert(ER_OK == status);
        status = intf->AddMethod(METHOD_OBSUPDATER, "su", "", "op,pid");
        assert(ER_OK == status);
        intf->Activate();

        QCC_UNUSED(status);

        /* set up totally uninteresting about data */
        //AppId is a 128bit uuid
        uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                            0x1E, 0x82, 0x11, 0xE4,
                            0x86, 0x51, 0xD1, 0x56,
                            0x1D, 0x5D, 0x46, 0xB0 };
        aboutData.SetAppId(appId, 16);
        aboutData.SetDeviceName("My Device Name");
        //DeviceId is a string encoded 128bit UUID
        aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
        aboutData.SetAppName("Application");
        aboutData.SetManufacturer("Manufacturer");
        aboutData.SetModelNumber("123456");
        aboutData.SetDescription("A poetic description of this application");
        aboutData.SetDateOfManufacture("2014-03-24");
        aboutData.SetSoftwareVersion("0.1.2");
        aboutData.SetHardwareVersion("0.0.1");
        aboutData.SetSupportUrl("http://www.example.org");

        Announce();
    }

    void StartBus() {
        QStatus status = bus.Start();
        assert(ER_OK == status);
        qcc::String busAddress;
        busAddress = getConnectArg().c_str();
        QCC_DbgPrintf(("Fetched bus address is : %s", busAddress.c_str()));
        status = bus.Connect(busAddress.c_str());
        assert(ER_OK == status);
        status = bus.BindSessionPort(port, opts, *this);
        assert(ER_OK == status);
        uniqueBusName = bus.GetUniqueName();
        QCC_UNUSED(status);
    }

    void Announce() {
        QStatus status = aboutObj.Announce(port, aboutData);
        assert(ER_OK == status);
        QCC_UNUSED(status);
    }

    virtual ~Participant() {
        StopBus();
    }

    void StopBus() {
        QStatus status = bus.Disconnect();
        assert(ER_OK == status);
        status = bus.Stop();
        assert(ER_OK == status);
        status = bus.Join();
        assert(ER_OK == status);
        QCC_UNUSED(status);
    }

    MultiPeerTestObject* CreateAndRegisterObject(qcc::String name, qcc::String interface) {
        qcc::String path = qcc::String(PATH_PREFIX) + name;
        MultiPeerTestObject* obj = NULL;
        obj = new MultiPeerTestObject(bus, path, interface);
        QStatus status = bus.RegisterBusObject(*obj);
        assert(ER_OK == status);
        QCC_UNUSED(status);
        return obj;
    }

    void UnregisterObject(MultiPeerTestObject* obj) {
        bus.UnregisterBusObject(*obj);
    }

    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        return true;
    }

  private:
    //Private copy constructor to prevent copying the class and double freeing of memory
    Participant(const Participant& rhs);
    //Private assignment operator to prevent copying the class and double freeing of memory
    Participant& operator=(const Participant& rhs);
};

class ObserverListener : public Observer::Listener {
  public:
    BusAttachment& bus;

    typedef vector<ProxyBusObject> ProxyVector;
    ProxyVector proxies;

    int counter;
    Event event;
    bool strict;
    qcc::String intfName;

    ObserverListener(BusAttachment& bus, qcc::String _intfName) : bus(bus), counter(0), strict(true), intfName(_intfName) { }

    void ExpectInvocations(int newCounter) {
        /* first, check whether the counter was really 0 from last invocation */
        assert(0 == counter);

        event.ResetEvent();
        counter = newCounter;
    }

    ProxyVector::iterator FindProxy(ProxyBusObject proxy) {
        ProxyVector::iterator it;
        for (it = proxies.begin(); it != proxies.end(); ++it) {
            if (it->iden(proxy)) {
                break;
            }
        }
        return it;
    }

    virtual void ObjectDiscovered(ProxyBusObject& proxy) {
        if (strict) {
            assert(FindProxy(proxy) == proxies.end());
        }
        proxies.push_back(proxy);

        Message reply(bus);
        QStatus status;

        bus.EnableConcurrentCallbacks();
        status = proxy.MethodCall(intfName.c_str(), METHOD, NULL, 0, reply);
        assert(ER_OK == status);
        if (ER_OK == status) {
            String ubn(reply->GetArg(0)->v_string.str), path(
                reply->GetArg(1)->v_string.str);
            if (strict) {
                assert(proxy.GetUniqueName() == ubn);
            }
            assert(proxy.GetPath() == path);
        }

        MsgArg updateArgs[2];
        updateArgs[0].Set("s", "insert");
        updateArgs[1].Set("u", (uint32_t)getpid());
        status = proxy.MethodCall(intfName.c_str(), METHOD_OBSUPDATER, updateArgs, 2); // Tell the object that it's being observed by me
        assert(ER_OK == status);
        QCC_UNUSED(status);

        if (--counter == 0) {
            event.SetEvent();
        }

    }

    virtual void ObjectLost(ProxyBusObject& proxy) {
        ProxyVector::iterator it = FindProxy(proxy);
        assert(it != proxies.end());
        proxies.erase(it);
        if (--counter == 0) {
            event.SetEvent();
        }
    }
};

static bool WaitForAll(vector<Event*>& events, uint32_t wait_ms = DEFAULT_WAIT_MS)
{
    uint32_t final = qcc::GetTimestamp() + wait_ms;
    vector<Event*> remaining = events;
    while (remaining.size() > 0) {
        vector<Event*> triggered;
        uint32_t now = qcc::GetTimestamp();

        if (now >= final) {
            return false;
        }
        QStatus status = Event::Wait(remaining, triggered, final - now);
        if (status != ER_OK && status != ER_TIMEOUT) {
            return false;
        }
        vector<Event*>::iterator trigit, remit;
        for (trigit = triggered.begin(); trigit != triggered.end(); ++trigit) {
            for (remit = remaining.begin(); remit != remaining.end(); ++remit) {
                if (*remit == *trigit) {
                    remaining.erase(remit);
                    break;
                }
            }
        }
    }
    return true;
}

static int BeObserver(int observees, pid_t parentPID) {

    QCC_DbgPrintf(("Called be_observer with %u observee(s)", observees));
    bool retval = false;

    char intfName [50];
    snprintf(intfName, sizeof(intfName), "org.test.observer.a.parentPID%u", (uint32_t)parentPID);

    Participant consumer(intfName);
    ObserverListener listener(consumer.bus, intfName);
    const char* mandatoryInterfaces[] = { intfName };
    Observer obs(consumer.bus, mandatoryInterfaces, 1);
    listener.strict = true;
    obs.RegisterListener(listener);

    vector<Event*> events;
    events.push_back(&(listener.event));
    listener.ExpectInvocations(observees * 2); // Objects are published and removed (hence x 2)

    retval = WaitForAll(events, DEFAULT_WAIT_MS * (observees));

    QCC_DbgPrintf(("be_observer pid %u finished with %s", getpid(), (retval ? "OK" : "NOK")));

    obs.UnregisterAllListeners();
    return retval;
}

static int BeProvider(int objects, int observers, pid_t parentPID)
{
    QCC_DbgPrintf(("Called BeProvider with %d object(s) and potentially %d observer(s)", objects, observers));

    bool retval = true;

    char intfName [50];
    snprintf(intfName, sizeof(intfName), "org.test.observer.a.parentPID%u", (uint32_t)parentPID);

    Participant provider(intfName);
    vector<MultiPeerTestObject*> myObjects;

    for (int i = 0; i < objects; i++) {
        char objName[20];
        snprintf(objName, sizeof(objName), "object%u", i);
        MultiPeerTestObject* obj = provider.CreateAndRegisterObject(objName, intfName);
        assert(obj != NULL);
        myObjects.push_back(obj);
        QCC_DbgPrintf(("Object %s created with Intf : %s", objName, intfName));

    }

    provider.Announce();
    QCC_DbgPrintf(("Published provider's (pid - %u) about info after registering all objects", getpid()));

    bool stop;

    uint32_t final = qcc::GetTimestamp() + (DEFAULT_WAIT_MS * observers);

    do {
        stop = true;
        uint32_t now = qcc::GetTimestamp();
        for (size_t i = 0; i < myObjects.size(); ++i) {
            if (myObjects[i] != NULL) {
                stop = stop && ((*myObjects[i]).observedSoFarBy.size() == (uint32_t)(observers));
            }
        }
        if (now >= final) { // Timeout
            retval = false;
            stop = true;
            QCC_LogError(ER_FAIL, ("Provider (pid - %u) has timed out waiting for all its objects to be discovered", getpid()));
        }
    } while (!stop);

    if (!retval) {
        return retval;
    }

    for (size_t i = 0; i < myObjects.size(); ++i) {
        provider.UnregisterObject(myObjects[i]);
    }

    provider.Announce();
    QCC_DbgPrintf(("Published provider's (pid - %u) about info after unregistering all objects", getpid()));

    retval = (ER_OK == qcc::Sleep(DEFAULT_WAIT_MS * (observers)));

    QCC_DbgPrintf(("BeProvider pid %u finished with %s", getpid(), (retval ? "OK" : "NOK")));

    return retval;
}

static QStatus ShutDown(bool success) {

    cout << "Test completed : " << (success ? "SUCCESS" : "FAILED") << endl;

#ifdef ROUTER
    return AllJoynRouterShutdown();
#endif
    return AllJoynShutdown();
}

int main(int argc, char** argv)
{
    int status;
    int observers;
    int providers;
    int objectsPerProvider;
    int observees;
    pid_t parentPID;
    vector<pid_t> children;

    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif

    if (argc == 5 && (0 == strcmp(argv[1], "p"))) {  // Provider only
        objectsPerProvider = atoi(argv[2]);
        observers = atoi(argv[3]);
        parentPID = atoi(argv[4]);
        return BeProvider(objectsPerProvider, observers, parentPID);

    } else if (argc == 4 && (0 == strcmp(argv[1], "o"))) {   // Observer only
        observees = atoi(argv[2]);
        parentPID = atoi(argv[3]);
        return BeObserver(observees, parentPID);

    } else if (argc == 4) {

        //Initial start
        providers = atoi(argv[1]);
        objectsPerProvider = atoi(argv[2]);
        observers = atoi(argv[3]);
        parentPID = getpid();
        observees = objectsPerProvider * providers;
        children.resize(observers + providers);
        cout << "Set ER_DEBUG_ALLJOYN_OBSERVER_TEST=7 for debug traces" << endl;

    } else {
        QCC_LogError(ER_FAIL, ("Bad initial args !\nUsage: %s Providers ObjectsPerProvider Observers", argv[0]));
        ShutDown(false);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < children.size(); ++i) {
        children[i] = fork();
        if (0 == children[i]) {
            QCC_DbgPrintf(("Forked pid = %u", getpid()));
            char* newArgs[6];
            newArgs[0] = argv[0];
            char buf1[20];
            char buf2[20];
            char buf3[20];
            if (i < (size_t)providers) {
                newArgs[1] = (char*)"p"; //Provider
                snprintf(buf1, 20, "%d", objectsPerProvider);
                newArgs[2] = buf1;
                snprintf(buf2, 20, "%d", observers);
                newArgs[3] = buf2;
                snprintf(buf3, 20, "%d", parentPID);
                newArgs[4] = buf3;
            } else {
                qcc::Sleep(1000);
                newArgs[1] = (char*)"o"; //Observer
                snprintf(buf1, 20, "%d", observees);
                newArgs[2] = buf1;
                snprintf(buf2, 20, "%d", parentPID);
                newArgs[3] = buf2;
                newArgs[4] = NULL;
            }
            newArgs[5] = NULL;
            execv(argv[0], newArgs);
            QCC_LogError(ER_FAIL, ("[MAIN] Exec fails."));
            ShutDown(false);
            return EXIT_FAILURE;
        } else if (-1 == children[i]) {
            perror("fork");
            ShutDown(false);
            return EXIT_FAILURE;
        }
    }

    for (size_t i = 0; i < children.size(); ++i) {
        if (waitpid(children[i], &status, 0) < 0) {
            QCC_LogError(ER_FAIL, ("Could not wait for PID %u", children[i]));
            perror("waitpid");
            status = false;
            break;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) == false) {
            QCC_LogError(ER_FAIL, ("PID %u", children[i]));
            status = false;
            break;
        }
    }

    if (ShutDown(status) != ER_OK) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
