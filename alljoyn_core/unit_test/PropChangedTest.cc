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

#include <map>
#include <sstream>
#include <utility>
#include <vector>
#include <memory>

#include <gtest/gtest.h>

#include <qcc/Condition.h>
#include <qcc/Debug.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/Event.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>

#include "ajTestCommon.h"
#include "TestSecurityManager.h"
#include "TestSecureApplication.h"

using namespace ajn;
using namespace qcc;
using namespace std;

/* client/service synchronization time-out */
#define TIMEOUT 5000

/* time-out before emitting signal */
#define TIMEOUT_BEFORE_SIGNAL 100

/* time-out to be used for tests that expect to time-out */
#define TIMEOUT_EXPECTED 500

/* time to wait for the property cache to become fully enabled */
#define WAIT_CACHE_ENABLED_MS   500

/* constants */
static SessionPort SERVICE_PORT = 12345;
static const SessionOpts SESSION_OPTS(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
#define INTERFACE_NAME "org.alljoyn.test.PropChangedTest"
#define OBJECT_PATH    "/org/alljoyn/test/PropChangedTest"
static const char* PROP_NOT_SIGNALED = "NotSignaled";

class Semaphore {
  public:
    Semaphore() : value(0) { }

    QStatus Post()
    {
        QStatus status = ER_OK;

        mutex.Lock();
        value++;
        status = cond.Signal();
        mutex.Unlock();
        return status;
    }

    QStatus Wait()
    {
        QStatus status = ER_OK;

        mutex.Lock();
        while (0 == value) {
            status = cond.Wait(mutex);
        }
        if (ER_OK == status) {
            value--;
        }
        mutex.Unlock();
        return status;
    }

    QStatus TimedWait(uint32_t ms)
    {
        QStatus status = ER_OK;
        uint64_t start_time = qcc::GetTimestamp64();

        mutex.Lock();
        while ((0 == value) && (ER_OK == status)) {
            uint64_t elapsed = qcc::GetTimestamp64() - start_time;
            if (elapsed < ms) {
                status = cond.TimedWait(mutex, ms - elapsed);
            } else {
                status = ER_TIMEOUT;
                break;
            }
        }
        if (ER_OK == status) {
            value--;
        }
        mutex.Unlock();
        return status;
    }

  private:
    unsigned int value;
    qcc::Condition cond;
    qcc::Mutex mutex;

    // prevent copy by construction or assignment
    Semaphore(const Semaphore& other);
    Semaphore& operator=(const Semaphore& other);
};

/**
 * ProxyBusObject interface creation method.
 */
typedef enum {
    PCM_INTROSPECT,
    PCM_XML,
    PCM_PROGRAMMATIC
} ProxyCreationMethod;

/**
 * Class for defining ranges.
 */
class Range {
  public:
    int first; // inclusive
    int last;  // inclusive

    Range(int first,
          int last) :
        first(first), last(last)
    { }

    int Size() const
    {
        return last - first + 1;
    }

    bool IsIn(int num) const
    {
        return ((num >= first) && (num <= last));
    }
};

static Range Pnone(1, 0); // match none (size of 0)
static Range Pall(1, 0); // match all (size of 0)
static Range P1(1, 1);
static Range P2(2, 2);
static Range P1to2(1, 2);
static Range P1to3(1, 3);
static Range P1to4(1, 4);
static Range P2to3(2, 3);
static Range P2to4(2, 4);
static Range P3to4(3, 4);

/**
 * Class for parameterizing the interface description used in the tests.
 */
class InterfaceParameters {
  public:
    Range rangeProp;     // number of properties in interface and published
    String emitsChanged; // value for EmitsChanged annotation
    String name;         // interface name
    bool emitsFalse;     // add/emit

    InterfaceParameters(const Range& rangeProp,
                        const String& emitsChanged = "true",
                        bool emitsFalse = false,
                        String name = INTERFACE_NAME) :
        rangeProp(rangeProp),
        emitsChanged(emitsChanged),
        name(name),
        emitsFalse(emitsFalse)
    { }
};

/**
 * Class for parameterizing the property changed listener and emitter used in
 * the tests.
 */
class TestParameters {
  public:
    bool newEmit;          // use new (true) or old (false) emitter method
    Range rangePropEmit;   // number of properties to emit (newEmit only)
    ProxyCreationMethod creationMethod; // how to create proxy
    vector<Range> rangePropListen; // number of properties to listen for on each interface
    vector<Range> rangePropListenExp; // number of properties expected to be received for each interface
    vector<InterfaceParameters> intfParams;

    TestParameters(bool newEmit,
                   const InterfaceParameters& ip) :
        newEmit(newEmit),
        rangePropEmit(ip.rangeProp),
        creationMethod(PCM_INTROSPECT)
    {
        this->rangePropListen.push_back(ip.rangeProp);
        this->rangePropListenExp.push_back(ip.rangeProp); // same as listen
        this->intfParams.push_back(ip);
    }

    TestParameters(bool newEmit,
                   const Range& rangePropEmit,
                   ProxyCreationMethod creationMethod = PCM_INTROSPECT) :
        newEmit(newEmit),
        rangePropEmit(rangePropEmit),
        creationMethod(creationMethod)
    { }

    TestParameters(bool newEmit,
                   const Range& rangePropListen,
                   const Range& rangePropEmit,
                   ProxyCreationMethod creationMethod) :
        newEmit(newEmit),
        rangePropEmit(rangePropEmit),
        creationMethod(creationMethod)
    {
        this->rangePropListen.push_back(rangePropListen);
        this->rangePropListenExp.push_back(rangePropListen); // same as listen
    }

    TestParameters(bool newEmit,
                   const Range& rangePropListen,
                   const Range& rangePropEmit,
                   ProxyCreationMethod creationMethod,
                   const InterfaceParameters& ip) :
        newEmit(newEmit),
        rangePropEmit(rangePropEmit),
        creationMethod(creationMethod)
    {
        this->rangePropListen.push_back(rangePropListen);
        this->rangePropListenExp.push_back(rangePropListen); // same as listen
        this->intfParams.push_back(ip);
    }

    TestParameters& AddInterfaceParameters(const InterfaceParameters& ip)
    {
        this->intfParams.push_back(ip);
        return *this;
    }

    TestParameters& AddListener(const Range& listenerRangePropListen)
    {
        this->rangePropListen.push_back(listenerRangePropListen);
        this->rangePropListenExp.push_back(listenerRangePropListen); // same as listen
        return *this;
    }

    TestParameters& AddListener(const Range& listenerRangePropListen,
                                const Range& listenerRangePropListenExp)
    {
        this->rangePropListen.push_back(listenerRangePropListen);
        this->rangePropListenExp.push_back(listenerRangePropListenExp);
        return *this;
    }
};

static vector<const char*> BuildPropertyNameVector(Range range)
{
    EXPECT_LE(range.first, range.last);
    EXPECT_LT(range.last, 10);
    vector<const char*> propertyNames;
    for (int i = range.first; i <= range.last; i++) {
        char* name = strdup("P0");
        name[1] = '0' + i;
        propertyNames.push_back(name);
    }
    return propertyNames;
}

static void AddProperty(InterfaceDescription& intf,
                        const char* name,
                        const char* annotation)
{
    QStatus status;

    status = intf.AddProperty(name, "i", ajn::PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status);
    status = intf.AddPropertyAnnotation(name,
                                        ajn::org::freedesktop::DBus::AnnotateEmitsChanged, annotation);
    EXPECT_EQ(ER_OK, status);
    //QCC_SyncPrintf("AddProperty(%s, emits=%s)\n", name, annotation);
}

static const InterfaceDescription* SetupInterface(BusAttachment& bus,
                                                  const InterfaceParameters& ip)
{
    const InterfaceDescription* intf;
    QStatus status;

    EXPECT_LE(ip.rangeProp.first, ip.rangeProp.last);
    EXPECT_LT(ip.rangeProp.last, 10);
    // only create once
    intf = bus.GetInterface(ip.name.c_str());
    if (NULL == intf) {
        InterfaceDescription* tmp;
        /* create/activate alljoyn_interface */
        //QCC_SyncPrintf("SetupInterface(%s)\n", ip.name.c_str());
        status = bus.CreateInterface(ip.name.c_str(), tmp, false);
        EXPECT_EQ(ER_OK, status);
        EXPECT_TRUE(tmp != NULL);
        if (tmp != NULL) {
            for (int i = ip.rangeProp.first; i <= ip.rangeProp.last; i++) {
                String name = String("P").append('0' + i);
                AddProperty(*tmp, name.c_str(), ip.emitsChanged.c_str());
            }
            if (ip.emitsFalse) {
                // always add a property that does not get signaled
                AddProperty(*tmp, PROP_NOT_SIGNALED, "false");
            }
            tmp->Activate();
        }
        intf = tmp;
    }
    return intf;
}

/**
 * BusObject used for testing.
 */
class PropChangedTestBusObject :
    public BusObject {
  public:
    QStatus status;
    BusAttachment& bus;
    const vector<InterfaceParameters> intfParams;
    map<String, int> propvalOffsets;
    map<String, int> getsPerPropName;

    PropChangedTestBusObject(BusAttachment& bus,
                             const vector<InterfaceParameters> ip,
                             const char* path = OBJECT_PATH) :
        BusObject(path), status(ER_OK), bus(bus),
        intfParams(ip)
    {
        //QCC_SyncPrintf("PropChangedTestBusObject::constructor for %s\n", path);
        for (size_t i = 0; i < ip.size(); i++) {
            const InterfaceDescription* intf = SetupInterface(bus, ip[i]);
            status = AddInterface(*intf);
            propvalOffsets[ip[i].name] = 0;
        }
        EXPECT_EQ(ER_OK, status);
        status = bus.RegisterBusObject(*this);
        EXPECT_EQ(ER_OK, status);
    }

    virtual ~PropChangedTestBusObject()
    {
    }

    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        size_t i;

        for (i = 0; i < intfParams.size(); i++) {
            if (intfParams[i].name == ifcName) {
                break; // found valid interface
            }
        }
        EXPECT_LT(i, intfParams.size()) << "  Invalid interface name: " << ifcName;
        EXPECT_EQ('P', propName[0]); // the not-signaled property should not match
        int num = atoi(&propName[1]);
        EXPECT_NE(0, num); // property names range from P1 to P9 (max)
        EXPECT_TRUE(intfParams[i].rangeProp.IsIn(num));
        val.typeId = ALLJOYN_INT32;
        val.v_int32 = propvalOffsets[ifcName] + num;
        //QCC_SyncPrintf("PropChangedTestBusObject::Get(%s, %s) -> %d\n", ifcName, propName, val.v_int32);

        getsPerPropName[propName] += 1;

        return ER_OK;
    }

    void ChangePropertyValues(const InterfaceParameters& ip, int offset)
    {
        propvalOffsets[ip.name] = offset;
    }

    void ChangePropertyValues(const TestParameters& tp, int offset)
    {
        for (size_t intf = 0; intf < tp.intfParams.size(); intf++) {
            ChangePropertyValues(tp.intfParams[intf], offset);
        }
    }

    void EmitSignal(const TestParameters& tp,
                    const InterfaceParameters& ip,
                    SessionId id = SESSION_ID_ALL_HOSTED)
    {
        if (tp.newEmit) {
            // use new emit
            vector<const char*> propertyNames = BuildPropertyNameVector(tp.rangePropEmit);
            // add the not-signaled property if needed
            if (ip.emitsFalse) {
                propertyNames.push_back(strdup(PROP_NOT_SIGNALED));
            }
            //ostringstream s;
            //copy(propertyNames.begin(), propertyNames.end(), ostream_iterator<const char*>(s, ","));
            //QCC_SyncPrintf("Emit for %s:%s in session %ud\n", ip.name.c_str(), s.str().c_str(), id);
            // signal
            status = EmitPropChanged(ip.name.c_str(), &propertyNames[0], propertyNames.size(), id);
            EXPECT_EQ(ER_OK, status);
            // clean up
            for (size_t i = 0; i < propertyNames.size(); i++) {
                free((void*)propertyNames[i]);
            }
        } else {
            // use old emit (only one property possible per signal)
            char name[] = "P0";
            for (int i = tp.rangePropEmit.first; i <= tp.rangePropEmit.last; i++) {
                name[1] = '0' + i;
                //QCC_SyncPrintf("Old emit for %s:%s on session %ud\n", ip.name.c_str(), name, id);
                MsgArg val("i", i);
                EmitPropChanged(ip.name.c_str(), name, val, id);
            }
        }
    }

    void EmitSignals(const TestParameters& tp)
    {
        for (size_t intf = 0; intf < tp.intfParams.size(); intf++) {
            EmitSignal(tp, tp.intfParams[intf]);
        }
    }
};

class SampleStore {
  public:
    Mutex mutex;
    Semaphore signalSema;
    vector<ProxyBusObject> proxySamples;
    map<String, vector<MsgArg> > changedSamples;
    map<String, vector<MsgArg> > invalidatedSamples;

    void AddSample(const ProxyBusObject& proxy,
                   const char* ifaceName,
                   const MsgArg& changed,
                   const MsgArg& invalidated)
    {
        mutex.Lock();
        proxySamples.push_back(proxy);
        changedSamples[ifaceName].push_back(changed);
        invalidatedSamples[ifaceName].push_back(invalidated);
        signalSema.Post();
        mutex.Unlock();
    }

    void Clear()
    {
        mutex.Lock();
        proxySamples.clear();
        changedSamples.clear();
        invalidatedSamples.clear();
        mutex.Unlock();
    }

    QStatus TimedWait(uint32_t timeout)
    {
        return signalSema.TimedWait(timeout);
    }
};

/**
 * PropertiesChangedListener used for testing.  It will add the received
 * samples to the PropChangedTestProxyBusObject for later validation.
 */
class PropChangedTestListener :
    public ProxyBusObject::PropertiesChangedListener {
  public:
    SampleStore& store;
    multimap<String, ProxyBusObject> registeredInterfaces;

    PropChangedTestListener(SampleStore& store) :
        store(store)
    { }

    virtual ~PropChangedTestListener() { }

    virtual void PropertiesChanged(ProxyBusObject& obj,
                                   const char* ifaceName,
                                   const MsgArg& changed,
                                   const MsgArg& invalidated,
                                   void* context)
    {
        QCC_UNUSED(context);
        ASSERT_EQ(ALLJOYN_ARRAY, changed.typeId);
        ASSERT_EQ(ALLJOYN_ARRAY, invalidated.typeId);
        //QCC_SyncPrintf("PropChangedTestListener::PropertiesChanged called (changed:%d,invalidated:%d) for %s\n", changed.v_array.GetNumElements(), invalidated.v_array.GetNumElements(), (context ? (const char*)context : "?"));
        store.AddSample(obj, ifaceName, changed, invalidated);
    }
  private:
    /* Private assignment operator - does nothing */
    PropChangedTestListener& operator=(const PropChangedTestListener&);
};

static void BuildXMLProperty(String& xml, const String& name, const String& annotation)
{
    xml.append("<property name=\"");
    xml.append(name);
    xml.append("\" type=\"i\" access=\"read\">\n");
    xml.append("<annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"");
    xml.append(annotation);
    xml.append("\"/>\n");
    xml.append("</property>\n");
}

static String BuildXML(const TestParameters& tp)
{
    String xml;
    xml.append("<node name=\"" OBJECT_PATH "\">\n");
    for (size_t i = 0; i < tp.intfParams.size(); i++) {
        const InterfaceParameters& ip = tp.intfParams[i];
        xml.append("<interface name=\"");
        xml.append(ip.name.c_str());
        xml.append("\">\n");
        for (int num = ip.rangeProp.first; num <= ip.rangeProp.last; num++) {
            String name("P");
            name.append('0' + num);
            BuildXMLProperty(xml, name, ip.emitsChanged.c_str());
        }
        if (ip.emitsFalse) {
            BuildXMLProperty(xml, PROP_NOT_SIGNALED, "false");
        }
        xml.append("</interface>\n");
    }
    xml.append("</node>\n");
    return xml;
}

static QStatus AddProxyInterface(BusAttachment& bus,
                                 ProxyBusObject& proxy,
                                 const TestParameters& tp)
{
    QStatus status = ER_FAIL;

    switch (tp.creationMethod) {
    case PCM_INTROSPECT:
        //QCC_SyncPrintf("AddProxyInterface PCM_INTROSPECT\n");
        status = proxy.IntrospectRemoteObject();
        break;

    case PCM_PROGRAMMATIC:
        //QCC_SyncPrintf("AddProxyInterface PCM_PROGRAMMATIC\n");
        for (size_t i = 0; i < tp.intfParams.size(); i++) {
            const InterfaceDescription* intf = SetupInterface(bus, tp.intfParams[i]);
            status = proxy.AddInterface(*intf);
        }
        break;

    case PCM_XML:
        String xml = BuildXML(tp);
        //QCC_SyncPrintf("AddProxyInterface PCM_XML:\n%s\n", xml.c_str());
        status = proxy.ParseXml(xml.c_str());
        break;
    }
    return status;
}

/**
 * Client side BusAttachment that is able to set up a session to a service bus.
 */
class ClientBusAttachment :
    public BusAttachment,
    public BusListener,
    public BusAttachment::JoinSessionAsyncCB {
  public:
    QStatus status;
    SessionId id;
    Semaphore sessionSema;

    ClientBusAttachment(const char* name) :
        BusAttachment(name, false), status(ER_OK), id(0)
    { }

    void Setup(const char* serviceName)
    {
        status = Start();
        ASSERT_EQ(ER_OK, status);
        status = Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status);
        RegisterBusListener(*this);
        status = FindAdvertisedName(serviceName);
        ASSERT_EQ(ER_OK, status);
    }

    void WaitForSession()
    {
        status = sessionSema.TimedWait(TIMEOUT);
        EXPECT_EQ(ER_OK, status);
    }

    void FoundAdvertisedName(const char* name,
                             TransportMask transport,
                             const char* namePrefix)
    {
        QCC_UNUSED(transport);
        QCC_UNUSED(namePrefix);
        JoinSessionAsync(name, SERVICE_PORT, NULL, SESSION_OPTS, this, NULL);
    }

    void JoinSessionCB(QStatus joinStatus,
                       SessionId sessionId,
                       const SessionOpts& opts,
                       void* context)
    {
        QCC_UNUSED(opts);
        QCC_UNUSED(context);

        EXPECT_EQ(ER_OK, joinStatus);
        id = sessionId;
        sessionSema.Post();
    }
};

/**
 * ProxyBusObject used for testing.
 */
class PropChangedTestProxyBusObject :
    public ProxyBusObject,
    public SampleStore {
  public:
    QStatus status;
    BusAttachment& bus;
    vector<PropChangedTestListener*> listeners;
    TestParameters tp;

    PropChangedTestProxyBusObject(ClientBusAttachment& clientBus,
                                  String serviceName,
                                  const TestParameters& tp,
                                  const char* path = OBJECT_PATH) :
        ProxyBusObject(clientBus, serviceName.c_str(), path, clientBus.id),
        status(ER_OK),
        bus(clientBus),
        tp(tp)
    {
        //QCC_SyncPrintf("PropChangedTestProxyBusObject::constructor for %s\n", path);
        status = AddProxyInterface(bus, *this, tp);
        EXPECT_EQ(ER_OK, status);
        // loop listeners
        for (size_t num = 0; num < tp.rangePropListen.size(); num++) {
            // loop interfaces
            for (size_t intf = 0; intf < tp.intfParams.size(); intf++) {
                PropChangedTestListener* listener = new PropChangedTestListener(*this);
                String name = tp.intfParams[intf].name;
                //QCC_SyncPrintf("Register listener %d for %s\n", num, name.c_str());
                RegisterListener(listener, name, tp.rangePropListen[num]);
                listeners.push_back(listener);
            }
        }
    }

    PropChangedTestProxyBusObject(const PropChangedTestProxyBusObject& other) :
        ProxyBusObject(other),
        status(ER_OK),
        bus(other.bus),
        tp(other.tp)
    {
        if (&other == this) {
            return;
        }

        // loop listeners
        for (size_t num = 0; num < tp.rangePropListen.size(); num++) {
            // loop interfaces
            for (size_t intf = 0; intf < tp.intfParams.size(); intf++) {
                PropChangedTestListener* listener = new PropChangedTestListener(*this);
                String name = tp.intfParams[intf].name;
                //QCC_SyncPrintf("copy: Register listener %d for %s\n", num, name.c_str());
                RegisterListener(listener, name, tp.rangePropListen[num]);
                listeners.push_back(listener);
            }
        }
    }

    ~PropChangedTestProxyBusObject()
    {
        for (size_t i = 0; i < listeners.size(); i++) {
            // Need to unregister listeners before deleting them.
            while (!listeners[i]->registeredInterfaces.empty()) {
                multimap<String, ProxyBusObject>::const_iterator it = listeners[i]->registeredInterfaces.begin();
                String name = it->first;
                ProxyBusObject pbo = it->second;
                pbo.UnregisterPropertiesChangedListener(name.c_str(), *listeners[i]);
                listeners[i]->registeredInterfaces.erase(it);
            }

            delete listeners[i];
        }
    }

    void RegisterListener(ProxyBusObject::PropertiesChangedListener* listener,
                          String ifaceName,
                          const Range& props,
                          const char* who = NULL)
    {
        if (props.Size() > 0) {
            vector<const char*> propertyNames = BuildPropertyNameVector(props);
#if !defined(NDEBUG)
            ostringstream s;
            copy(propertyNames.begin(), propertyNames.end(), ostream_iterator<const char*>(s, ","));
            //QCC_SyncPrintf("Listener for properties %s\n", s.str().c_str());
#endif
            status = RegisterPropertiesChangedListener(ifaceName.c_str(),
                                                       &propertyNames[0], propertyNames.size(),
                                                       *listener, (void*)who);
            for (int i = 0; i < props.Size(); i++) {
                free((void*)propertyNames[i]);
            }
        } else {
            //QCC_SyncPrintf("Listener for all properties\n");
            status = RegisterPropertiesChangedListener(ifaceName.c_str(),
                                                       NULL, 0, *listener, (void*)who);
        }
        EXPECT_EQ(ER_OK, status);
        if (status == ER_OK) {
            ((PropChangedTestListener*)listener)->registeredInterfaces.insert(pair<String, ProxyBusObject>(ifaceName, *this));
        }
    }

    /* By default we do not expect a time-out when waiting for the signals.  If
     * the \a expectTimeout argument is provided an different from 0 we will
     * wait for that amount of milliseconds and will NOT expect any signals. */
    void WaitForSignals(const TestParameters& testParameters,
                        uint32_t expectTimeout = 0)
    {
        uint32_t timeout = expectTimeout ? expectTimeout : TIMEOUT;
        QStatus expStatus = expectTimeout ? ER_TIMEOUT : ER_OK;

        /* wait for signals for all interfaces on all listeners */
        size_t num = testParameters.intfParams.size() * testParameters.rangePropListen.size();
        for (size_t i = 0; i < num; i++) {
            //QCC_SyncPrintf("WaitForSignals %dms on listener %d\n", timeout, i);
            status = TimedWait(timeout); // wait for property changed signal
            EXPECT_EQ(expStatus, status);
        }
    }

    void ValidateSignals(const TestParameters& testParameters,
                         const InterfaceParameters& ip)
    {
        int emitChanged = 0, emitInvalidated = 0;
        const MsgArg* elems;
        int n, i;

        // ensure correct number of samples
        ASSERT_EQ(testParameters.rangePropListenExp.size(), changedSamples[ip.name].size());
        ASSERT_EQ(testParameters.rangePropListenExp.size(), invalidatedSamples[ip.name].size());

        if (ip.emitsChanged == "true") {
            emitChanged = testParameters.rangePropEmit.Size();
        } else if (ip.emitsChanged == "invalidates") {
            emitInvalidated = testParameters.rangePropEmit.Size();
        }
        //QCC_SyncPrintf("ValidateSignal expects %d samples with each %d changed, %d invalidated\n", testParameters.rangePropListenExp.size(), emitChanged, emitInvalidated);

        // loop over all samples
        for (size_t sample = 0; sample < testParameters.rangePropListenExp.size(); sample++) {
            MsgArg& changed = changedSamples[ip.name][sample];
            MsgArg& invalidated = invalidatedSamples[ip.name][sample];

            ASSERT_EQ(ALLJOYN_ARRAY, changed.typeId);
            n = changed.v_array.GetNumElements();
            int numListen = testParameters.rangePropListenExp[sample].Size();
            if (0 == numListen) {
                // listen to all
                numListen = testParameters.rangePropEmit.Size();
            }
            ASSERT_EQ(min(emitChanged, numListen), n);
            elems = changed.v_array.GetElements();
            for (i = 0; i < n; i++) {
                int num = testParameters.rangePropListenExp[sample].first + i;

                ASSERT_EQ(ALLJOYN_DICT_ENTRY, elems[i].typeId);
                // validate property name
                ASSERT_EQ(ALLJOYN_STRING, elems[i].v_dictEntry.key->typeId);
                String name = String("P").append('0' + num);
                EXPECT_STREQ(name.c_str(), elems[i].v_dictEntry.key->v_string.str);
                // validate property value
                ASSERT_EQ(ALLJOYN_VARIANT, elems[i].v_dictEntry.val->typeId);
                ASSERT_EQ(ALLJOYN_INT32, elems[i].v_dictEntry.val->v_variant.val->typeId);
                EXPECT_EQ(num, elems[i].v_dictEntry.val->v_variant.val->v_int32);
            }

            ASSERT_EQ(ALLJOYN_ARRAY, invalidated.typeId);
            n = invalidated.v_array.GetNumElements();
            ASSERT_EQ(emitInvalidated, n);
            elems = invalidated.v_array.GetElements();
            for (i = 0; i < n; i++) {
                int num = testParameters.rangePropListenExp[sample].first + i;

                // validate property name
                ASSERT_EQ(ALLJOYN_STRING, elems[i].typeId);
                String name = String("P").append('0' + num);
                EXPECT_STREQ(name.c_str(), elems[i].v_string.str);
            }
        }
    }

    void ValidateSignals(const TestParameters& testParameters)
    {
        for (size_t i = 0; i < testParameters.intfParams.size(); i++) {
            //QCC_SyncPrintf("ValidateSignal for %s\n", testParameters.intfParams[i].name.c_str());
            ValidateSignals(testParameters, testParameters.intfParams[i]);
        }
    }

    void ValidateSignals(const InterfaceParameters& ip)
    {
        TestParameters testParameters(true, ip);
        ValidateSignals(testParameters);
    }
};

/**
 * Base class for testing property emission.  It will set up a client and
 * service BusAttachment.
 */
class PropChangedTestTwoBusSetup :
    private SessionPortListener {
  public:
    QStatus status;
    BusAttachment serviceBus;
    ClientBusAttachment clientBus;
    String serviceName;
    String peerName;
    PropChangedTestBusObject* obj;
    PropChangedTestProxyBusObject* proxy;

    PropChangedTestTwoBusSetup() :
        status(ER_FAIL),
        serviceBus("PropChangedTestService", false),
        clientBus("PropChangedTestClient"),
        serviceName(genUniqueName(serviceBus)),
        obj(NULL),
        proxy(NULL)
    {
    }

    ~PropChangedTestTwoBusSetup()
    {
        if (NULL != proxy) {
            delete proxy;
        }
        if (NULL != obj) {
            delete obj;
        }
    }

    void SetUp()
    {
        // service
        status = serviceBus.Start();
        ASSERT_EQ(ER_OK, status);
        status = serviceBus.Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status);
        status = serviceBus.BindSessionPort(SERVICE_PORT, SESSION_OPTS, *this);

        status = serviceBus.RequestName(serviceName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        ASSERT_EQ(ER_OK, status);
        status = serviceBus.AdvertiseName(serviceName.c_str(), TRANSPORT_ANY);
        ASSERT_EQ(ER_OK, status);

        // client
        clientBus.Setup(serviceName.c_str());
    }

    void TearDown()
    {
        // client
        clientBus.CancelFindAdvertisedName(serviceName.c_str());
        clientBus.Disconnect();
        clientBus.Stop();
        clientBus.Join();
        // service
        serviceBus.CancelAdvertiseName(serviceName.c_str(), TRANSPORT_ANY);
        serviceBus.ReleaseName(serviceName.c_str());
        serviceBus.Disconnect();
        serviceBus.Stop();
        serviceBus.Join();
    }

    void SetupPropChanged(const TestParameters& tpService,
                          const TestParameters& tpClient)
    {
        clientBus.WaitForSession();

        obj = new PropChangedTestBusObject(serviceBus, tpService.intfParams);
        proxy = new PropChangedTestProxyBusObject(clientBus, serviceName, tpClient);
        qcc::Sleep(TIMEOUT_BEFORE_SIGNAL); // otherwise we might miss the signal
    }

    /**
     * Main test logic.
     */
    void TestPropChanged(const TestParameters& tpService,
                         const TestParameters& tpClient,
                         uint32_t expectTimeout = 0)
    {
        SetupPropChanged(tpService, tpClient);
        // test
        obj->EmitSignals(tpService);
        proxy->WaitForSignals(tpClient, expectTimeout);
        if (!expectTimeout) {
            // validate if not timed out
            proxy->ValidateSignals(tpClient);
        }
    }

    /**
     * Main test logic.
     */
    void TestPropChanged(const TestParameters& testParameters)
    {
        TestPropChanged(testParameters, testParameters);
    }

    bool AcceptSessionJoiner(SessionPort sessionPort,
                             const char* joiner,
                             const SessionOpts& opts)
    {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

class PropChangedTest :
    public PropChangedTestTwoBusSetup,
    public testing::TestWithParam<TestParameters> {
  public:
    PropChangedTest() : PropChangedTestTwoBusSetup() { }
    virtual ~PropChangedTest() { }
    virtual void SetUp() { PropChangedTestTwoBusSetup::SetUp(); }
    virtual void TearDown() { PropChangedTestTwoBusSetup::TearDown(); }
};

/*
 * Base parameterized test.
 *
 * Parameterization:
 *  - new (true) or old (false) emit method
 *  - number of listeners
 *  - number of properties to listen for
 *  - number of properties to emit
 *  - proxy bus object creation method
 *  - interface parameters
 *     - number of properties
 *     - [EmitsChanged annotation value]
 *     - [also test EmitsChanged="false" property or not?]
 *  - [extra interface(s)]
 *  - [extra listener(s)]
 */
TEST_P(PropChangedTest, Default)
{
    TestParameters tp = GetParam();

    TestPropChanged(tp);
}

/*
 * Functional tests for the newly added EmitPropChanged function for multiple
 * properties (independent of RegisterPropertiesChangedListener). For BusObject
 * containing interfaces created with three different annotations of
 * PropertiesChanged (true, invalidated, false).
 *
 * Note: Property with annotation "false" is part of all tests and validation
 *       is done that it is not sent over.
 */
INSTANTIATE_TEST_CASE_P(EmitPropChanged, PropChangedTest,
                        testing::Values(
                            /* Create a BusObject containing an interface with
                             * single property, P1. Invoke newly added
                             * EmitPropChanged function for multiple properties
                             * to indicate a change to P1. Verify that the
                             * signal sent across contains the P1 and its
                             * value.
                             * Note: Property with annotation "true". */
                            TestParameters(true, InterfaceParameters(P1, "true", true)),
                            /* Create a BusObject containing an interface with
                             * multiple properties, P1, P2, P3, and P4. Invoke
                             * newly added EmitPropChanged function for
                             * multiple properties to indicate a change to P1,
                             * P2, P3 and P4. Verify that the signal sent
                             * across contains the P1, P2, P3 and P4.
                             * Note: Properties with annotation "true".  */
                            TestParameters(true, InterfaceParameters(P1to4, "true", true)),
                            /* See above but with annotation "invalidates". */
                            TestParameters(true, InterfaceParameters(P1, "invalidates", true)),
                            /* See above but with annotation "invalidates". */
                            TestParameters(true, InterfaceParameters(P1to4, "invalidates", true))));

/*
 * Functional tests for the newly added RegisterPropertiesChangedListener.
 * For ProxyBusObject created in three different ways (via Introspection,
 * via raw xml, programmatically).
 */
INSTANTIATE_TEST_CASE_P(PropertiesChangedListener, PropChangedTest,
                        testing::Values(
                            /* Register a single listener for a property P
                             * of interface I. EmitPropChanged existing
                             * signal for the single Property P1. Verify
                             * that listener is called with P1. */
                            TestParameters(false, P1, P1, PCM_INTROSPECT,   InterfaceParameters(P1)),
                            TestParameters(false, P1, P1, PCM_PROGRAMMATIC, InterfaceParameters(P1)),
                            TestParameters(false, P1, P1, PCM_XML,          InterfaceParameters(P1)),
                            /* Register a single listener for a property P
                             * of interface I.  EmitPropChanged the newly
                             * added signal for the multiple properties
                             * with Property P1. Verify that listener is
                             * called with P1. */
                            TestParameters(true, P1, P1, PCM_INTROSPECT,   InterfaceParameters(P1)),
                            TestParameters(true, P1, P1, PCM_PROGRAMMATIC, InterfaceParameters(P1)),
                            TestParameters(true, P1, P1, PCM_XML,          InterfaceParameters(P1)),
                            /* Register two listeners for the same property
                             * P of interface I. Emit PropChanged signal
                             * for the property P of interface I. Verify
                             * that both listeners get called with P. */
                            TestParameters(true, P1, P1, PCM_INTROSPECT,   InterfaceParameters(P1)).AddListener(P1),
                            TestParameters(true, P1, P1, PCM_PROGRAMMATIC, InterfaceParameters(P1)).AddListener(P1),
                            TestParameters(true, P1, P1, PCM_XML,          InterfaceParameters(P1)).AddListener(P1),
                            /* Register a single listener for a property P
                             * of interface I. EmitPropChanged for the
                             * property P of interface I marked as true
                             * PropertiesChanged annotation, changed to value
                             * v. Verify that the listener is called with P
                             * with v.
                             * Note: same as 2nd block of tests above */
                            TestParameters(true, P1, P1, PCM_INTROSPECT,   InterfaceParameters(P1)),
                            TestParameters(true, P1, P1, PCM_PROGRAMMATIC, InterfaceParameters(P1)),
                            TestParameters(true, P1, P1, PCM_XML,          InterfaceParameters(P1)),
                            /* Register a single listener for a property P
                             * of interface I. EmitPropChanged for the
                             * property P of interface I marked as
                             * invalidates PropertiesChanged annotation,
                             * changed to value v. Verify that the listener
                             * is called with P. */
                            TestParameters(true, P1, P1, PCM_INTROSPECT,   InterfaceParameters(P1, "invalidates")),
                            TestParameters(true, P1, P1, PCM_PROGRAMMATIC, InterfaceParameters(P1, "invalidates")),
                            TestParameters(true, P1, P1, PCM_XML,          InterfaceParameters(P1, "invalidates")),
                            /* Register a single listener for specific
                             * properties P1, P2 and P3 of interface I.
                             * EmitPropChanged for the single Property P1.
                             * Verify that listener is called with P1. */
                            TestParameters(true, P1to3, P1, PCM_INTROSPECT,   InterfaceParameters(P1to3)),
                            TestParameters(true, P1to3, P1, PCM_PROGRAMMATIC, InterfaceParameters(P1to3)),
                            TestParameters(true, P1to3, P1, PCM_XML,          InterfaceParameters(P1to3)),
                            /* Register a single listener for specific
                             * properties P1, P2 and P3 of interface I.
                             * EmitPropChanged for properties P1, P2 and P3.
                             * Verify that listener is called with P1, P2 and
                             * P3. */
                            TestParameters(true, P1to3, P1to3, PCM_INTROSPECT,   InterfaceParameters(P1to3)),
                            TestParameters(true, P1to3, P1to3, PCM_PROGRAMMATIC, InterfaceParameters(P1to3)),
                            TestParameters(true, P1to3, P1to3, PCM_XML,          InterfaceParameters(P1to3)),
                            /* Register a single listener for all properties
                             * of interface I using NULL as argument.
                             * EmitPropChanged for all properties of I. Verify
                             * that listener is called with all the properties. */
                            TestParameters(true, Pall, P1to3, PCM_INTROSPECT,   InterfaceParameters(P1to3)),
                            TestParameters(true, Pall, P1to3, PCM_PROGRAMMATIC, InterfaceParameters(P1to3)),
                            TestParameters(true, Pall, P1to3, PCM_XML,          InterfaceParameters(P1to3)),
                            /* Register two listeners L1 and L2 for all properties of I1 and I2
                             * respectively. EmitPropChanged for all properties of I1 and I2 (two separate
                             * signals). Verify that both listeners get called with all the properties of
                             * appropriate interfaces. */
                            TestParameters(true, P1, P1, PCM_INTROSPECT).AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1")).AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2")),
                            TestParameters(true, P1, P1, PCM_PROGRAMMATIC).AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1")).AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2")),
                            TestParameters(true, P1, P1, PCM_XML).AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1")).AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2")),
                            /* Register two listeners L1 and L2 for two mutually exclusive halves of
                             * properties in I. EmitPropChanged for all properties of I. Verify that both
                             * listeners get called with appropriate properties. */
                            TestParameters(true, P1to2, P1to4, PCM_INTROSPECT, InterfaceParameters(P1to4)).AddListener(P3to4),
                            TestParameters(true, P1to2, P1to4, PCM_PROGRAMMATIC, InterfaceParameters(P1to4)).AddListener(P3to4),
                            TestParameters(true, P1to2, P1to4, PCM_XML, InterfaceParameters(P1to4)).AddListener(P3to4),
                            /* Register listener L1 for properties P1 and P2. Register listener L2 with
                             * properties P2 and P3. EmitPropChanged for P2. Verify that both listeners
                             * get called with P2. */
                            TestParameters(true, P2, PCM_INTROSPECT).AddInterfaceParameters(InterfaceParameters(P1to3)).AddListener(P1to2, P2).AddListener(P2to3, P2),
                            TestParameters(true, P2, PCM_PROGRAMMATIC).AddInterfaceParameters(InterfaceParameters(P1to3)).AddListener(P1to2, P2).AddListener(P2to3, P2),
                            TestParameters(true, P2, PCM_XML).AddInterfaceParameters(InterfaceParameters(P1to3)).AddListener(P1to2, P2).AddListener(P2to3, P2)
                            ));

/*
 *  Functional test for partially created ProxyBusObject run the following test
 * (a scenario where for example the BusObject could have 20 different
 * interfaces while the ProxyBusObject only has 1 out of 20):
 *
 * Create a ProxyBusObject with only one of interfaces I1 as compared the full
 * list of interfaces in BusObject. Register listeners L1 for all properties of
 * I1. EmitPropChanged signal for properties in I1. Verify that L1 does get
 * invoked.
 */
TEST_F(PropChangedTest, PartialProxy)
{
    // one interface for client
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    // an extra interface for service
    TestParameters tpService = tpClient;
    tpService.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2"));

    TestPropChanged(tpService, tpClient);
}

/*
 * Functional test for the same Listener being registered for different
 * ProxyBusObjects (listener is tied to an interface and a set of properties).
 *
 * Create three different proxy bus objects PB1, PB2 and PB3. PB1 and PB2 are
 * proxies for the same bus object BobA over different session ids (S1 and S2).
 * PB3 is a proxy for a different bus object BobB. Both the BusObjects BobA
 * and BobB implement interface I. Register the same Listener L for all three
 * proxy bus objects for all properties of I.
 * - Emit PropChanged signal from BobA over S1. Verify that L gets invoked
 *   with PB1.
 * - Emit PropChanged signal from BobA over S2. Verify that L gets invoked
 *   with PB2.
 * - EmitPropChanged signal from BobB. Verify that L gets invoked with PB3.
 *
 * Negative tests also included:
 * - Using the same listener L for all.
 *   - Emit PropChanged signal from BobA over S1. Verify that L does NOT get
 *     invoked with PB2 and PB3.
 *   - Emit PropChanged signal from BobA over S2. Verify that L does NOT get
 *     invoked with PB1 and PB3.
 *   - EmitPropChanged signal from BobB. Verify that L does NOT get invoked
 *     with PB1 and PB2.
 * - Using different listeners L and L2 on S1 and S2 respectively.
 *   - Emit a PropChanged signal for P1 only over S1. Verify that listener
 *     of PB2 does NOT get called.
 *   - and vice versa
 */
TEST_F(PropChangedTest, MultiSession)
{
    TestParameters tp(true, P1);
    tp.AddInterfaceParameters(InterfaceParameters(P1));

    clientBus.WaitForSession(); // S1
    // second session setup
    ClientBusAttachment clientBus2("PropChangedTestClient2");
    clientBus2.Setup(serviceName.c_str());
    clientBus2.WaitForSession(); // S2

    // set up bus objects
    PropChangedTestBusObject bobA(serviceBus, tp.intfParams, OBJECT_PATH "/BobA");
    PropChangedTestBusObject bobB(serviceBus, tp.intfParams, OBJECT_PATH "/BobB");

    // set up proxy bus objects
    PropChangedTestProxyBusObject pb1(clientBus, serviceName, tp, OBJECT_PATH "/BobA");
    PropChangedTestProxyBusObject pb2(clientBus2, serviceName, tp, OBJECT_PATH "/BobA");
    PropChangedTestProxyBusObject pb3(clientBus, serviceName, tp, OBJECT_PATH "/BobB");

    // set up listeners
    SampleStore store;
    PropChangedTestListener l(store);
    pb1.RegisterListener(&l, tp.intfParams[0].name, P1, "PB1");
    pb2.RegisterListener(&l, tp.intfParams[0].name, P1, "PB2");
    pb3.RegisterListener(&l, tp.intfParams[0].name, P1, "PB3");

    PropChangedTestListener l2(store);
    pb2.RegisterListener(&l2, tp.intfParams[0].name, P1, "PB2_L2");

    // test for pb1 (only l)
    store.Clear();
    EXPECT_EQ((size_t)0, store.proxySamples.size());
    bobA.EmitSignal(tp, tp.intfParams[0], clientBus.id);
    qcc::Sleep(500);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status);
    EXPECT_EQ((size_t)1, store.proxySamples.size());
    EXPECT_TRUE(pb1.iden(store.proxySamples[0]));

    // test for pb2 (both l and l2)
    store.Clear();
    EXPECT_EQ((size_t)0, store.proxySamples.size());
    bobA.EmitSignal(tp, tp.intfParams[0], clientBus2.id);
    qcc::Sleep(500);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status);
    EXPECT_EQ((size_t)2, store.proxySamples.size());
    EXPECT_TRUE(pb2.iden(store.proxySamples[0]));
    EXPECT_TRUE(pb2.iden(store.proxySamples[1]));

    // test for pb3 (only l)
    store.Clear();
    EXPECT_EQ((size_t)0, store.proxySamples.size());
    bobB.EmitSignal(tp, tp.intfParams[0], clientBus.id);
    qcc::Sleep(500);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status);
    EXPECT_EQ((size_t)1, store.proxySamples.size());
    EXPECT_TRUE(pb3.iden(store.proxySamples[0]));

    // clean up
    pb2.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l2);
    pb3.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l);
    pb2.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l);
    pb1.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l);
    l.registeredInterfaces.erase(tp.intfParams[0].name);
    l2.registeredInterfaces.erase(tp.intfParams[0].name);
}

/*
 * The following are the tests that check the return codes of EmitPropChanged.
 */
TEST_F(PropChangedTest, NegativeEmitPropChanged)
{
    TestParameters tp(true, InterfaceParameters(P1));
    const char* okProps[] = { "P1" };
    const char* nokProps[] = { "P2" };
    const char* mixProps[] = { "P1", "P2" };

    clientBus.WaitForSession();

    PropChangedTestBusObject testBusObject(serviceBus, tp.intfParams);
    /* Invoke the newly added EmitPropChanged with NULL as the interface name.
     * ER_OK should not be returned. */
    EXPECT_NE(ER_OK, testBusObject.EmitPropChanged(NULL, okProps, 1, 0));
    /* Invoke the newly added EmitPropChanged with an invalid interface name.
     * ER_OK should not be returned. */
    EXPECT_NE(ER_OK, testBusObject.EmitPropChanged("invalid.interface", okProps, 1, 0));
    /* Invoke the newly added EmitPropChanged with an invalid property name.
     * ER_OK should not be returned. */
    EXPECT_NE(ER_OK, testBusObject.EmitPropChanged(tp.intfParams[0].name.c_str(), nokProps, 2, 0));
    /* Invoke the newly added EmitPropChanged with a mixture of valid and
     * invalid properties. ER_OK should not be returned */
    EXPECT_NE(ER_OK, testBusObject.EmitPropChanged(tp.intfParams[0].name.c_str(), mixProps, 2, 0));
}

/*
 * The following are the tests that check the return codes of
 * RegisterPropertiesChangedListener.
 */
TEST_F(PropChangedTest, NegativeRegisterPropertiesChangedListener)
{
    TestParameters tp(true, InterfaceParameters(P1));
    const char* okProps[] = { "P1" };
    const char* nokProps[] = { "P2" };
    const char* mixProps[] = { "P1", "P2" };

    clientBus.WaitForSession();
    PropChangedTestBusObject testBusObject(serviceBus, tp.intfParams);
    PropChangedTestProxyBusObject proxyBusObject(clientBus, serviceName, tp);
    // extra listener for testing
    PropChangedTestListener listener(proxyBusObject);

    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with NULL as the interface parameter. The return code should be
     * ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxyBusObject.RegisterPropertiesChangedListener(NULL, okProps, 1, listener, NULL);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status);
    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with an invalid string as an interface parameter.  The return code
     * should be ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxyBusObject.RegisterPropertiesChangedListener("invalid.interface", okProps, 1, listener, NULL);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status);
    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with a non-existent property. The return code should be
     * ER_BUS_NO_SUCH_PROPERTY. */
    status = proxyBusObject.RegisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), nokProps, 1, listener, NULL);
    EXPECT_EQ(ER_BUS_NO_SUCH_PROPERTY, status);
    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with an array of properties that contains a mix of valid properties
     * and invalid / non-existent properties. The return code should be
     * ER_BUS_NO_SUCH_PROPERTY. */
    status = proxyBusObject.RegisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), mixProps, 2, listener, NULL);
    EXPECT_EQ(ER_BUS_NO_SUCH_PROPERTY, status);
}

/*
 * The following are the tests that check the return codes of
 * UnregisterPropertiesChangedListener.
 */
TEST_F(PropChangedTest, NegativeUnregisterPropertiesChangedListener)
{
    TestParameters tp(true, InterfaceParameters(P1));

    clientBus.WaitForSession();
    PropChangedTestBusObject testBusObject(serviceBus, tp.intfParams);
    PropChangedTestProxyBusObject proxyBusObject(clientBus, serviceName, tp);

    /* Create a ProxyBusObject and register a listener. Invoke
     * UnregisterPropertiesChangedListener with NULL as interface parameter.
     * The return code should be ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxyBusObject.UnregisterPropertiesChangedListener(NULL, *proxyBusObject.listeners[0]);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status);
    /* Create a ProxyBusObject and register a listener. Invoke
     * UnregisterPropertiesChangedListener with a non-existent random string
     * as interface parameter. The return code should be
     * ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxyBusObject.UnregisterPropertiesChangedListener("invalid.interface", *proxyBusObject.listeners[0]);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status);
}

/*
 * Create a ProxyBusObject and try to unregister a never registered
 * listener. The return code should be ER_OK.
 */
TEST_F(PropChangedTest, NegativeUnregisterInvalidPropertiesChangedListener)
{
    TestParameters tp(true, InterfaceParameters(P1));

    clientBus.WaitForSession();
    PropChangedTestBusObject testBusObject(serviceBus, tp.intfParams);
    PropChangedTestProxyBusObject proxyBusObject(clientBus, serviceName, tp);

    SampleStore store;
    PropChangedTestListener invalid(store);
    status = proxyBusObject.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), invalid);
    EXPECT_EQ(ER_OK, status);
}

/*
 * These are the tests to ensure that the registered listener does NOT get
 * called:
 */

/*
 * Create a ProxyBusObject and register a listener to look for
 * PropertiesChanged for property P1. EmitPropChanged signal for P1. Ensure
 * that listener gets called. Unregister the listener. EmitPropChanged signal
 * for P1. Ensure that the listener does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_AfterUnregister)
{
    TestParameters tp(true, InterfaceParameters(P1));

    // test that listener works
    TestPropChanged(tp);
    // now unregister
    status = proxy->UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), *proxy->listeners[0]);
    EXPECT_EQ(ER_OK, status);
    // fire signal again and expect no callback to be called
    obj->EmitSignals(tp);
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
    proxy->listeners[0]->registeredInterfaces.erase(tp.intfParams[0].name);
}

/*
 * Create a ProxyBusObject and register a listener to look for
 * PropertiesChanged for property P1. The BusObject contains two properties P1
 * and P2, where the names of the properties differ by just one character.
 * EmitPropChanged signal for P2. Ensure that listener does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_PropertyNotListeningNew)
{
    TestParameters tp(true, P1, P2, PCM_INTROSPECT, InterfaceParameters(P1to2));

    // test that listener works
    TestPropChanged(tp, tp, TIMEOUT_EXPECTED);
}

/*
 * Create a ProxyBusObject and register a listener to look for
 * PropertiesChanged for all properties of I1 interface. The BusObject contains
 * two interfaces I1 and I2, where the names of the interfaces differ by just
 * one character, while the names of properties are identical.
 * EmitPropChanged signal for all properties of I2. Ensure that listener does
 * NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_InterfaceNotListening)
{
    TestParameters tp(true, P1);
    tp.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    tp.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2"));
    tp.AddListener(P1);

    SetupPropChanged(tp, tp);
    // remove listener for I2
    status = proxy->UnregisterPropertiesChangedListener(INTERFACE_NAME "2", *proxy->listeners[1]);
    EXPECT_EQ(ER_OK, status);
    // fire signal for I2 and expect time-out
    obj->EmitSignal(tp, tp.intfParams[1]);
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
    proxy->listeners[1]->registeredInterfaces.erase(String(INTERFACE_NAME "2"));
}

/*
 * Create a ProxyBusObject and register a listener to look for
 * PropertiesChanged for property P1. EmitPropChanged signal for P1 where P1 is
 * marked as false with PropertyChanged annotation. Ensure that the listener
 * does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_AnnotationFalse)
{
    TestParameters tp(true, InterfaceParameters(P1, "false"));

    // test that listeners work
    TestPropChanged(tp, tp, TIMEOUT_EXPECTED);
}

/*
 * Create a ProxyBusObject and register a listener to look for
 * PropertiesChanged for property P1. Emit a PropChanged signal for property
 * P2. Use EmitPropChanged signal for single property. Ensure that the
 * listener does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_PropertyNotListeningOld)
{
    TestParameters tp(false, P1, P2, PCM_INTROSPECT, InterfaceParameters(P1to2));

    // test that listener works
    TestPropChanged(tp, tp, TIMEOUT_EXPECTED);
}

/*
 * Create a ProxyBusObject and register a listener to look for
 * PropertiesChanged for property P1. Emit a PropChanged signal for properties
 * P2, P3 and P4. Use EmitPropChanged for multiple properties. Ensure that the
 * listener does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_PropertyNotListeningNewMulti)
{
    TestParameters tp(true, P1, P2to4, PCM_INTROSPECT, InterfaceParameters(P1to4));

    // test that listener works
    TestPropChanged(tp, tp, TIMEOUT_EXPECTED);
}

/*
 * Create a ProxyBusObject and register two listeners, L1 and L2 for properties
 * P1 and P2 respectively. Emit a PropChangedSignal for P1. Ensure that L1 gets
 * called. Ensure that L2 does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_PropertyEmitOneProp)
{
    TestParameters tp(true, P1);
    tp.AddInterfaceParameters(InterfaceParameters(P1to2));
    tp.AddListener(P1);
    tp.AddListener(P2);

    SetupPropChanged(tp, tp);
    // emit
    obj->EmitSignals(tp);
    // wait for a single signal
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
    // remove L2 from TestParameters because nothing is expected on it
    tp.rangePropListenExp.pop_back();
    // validate that only signal for P1 was seen
    proxy->ValidateSignals(tp);
}

/*
 * Create a ProxyBusObject and register two listeners, L1 and L2 for the same
 * property P belonging to two different interfaces I1 and I2 respectively.
 * Emit a PropChangedSignal for P of I1. Ensure that L1 gets called and L2 does
 * NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_PropertyEmitOneIntf)
{
    TestParameters tp(true, P1);
    tp.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    tp.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2"));
    tp.AddListener(P1); // creates a listener per interface

    SetupPropChanged(tp, tp);
    // emit
    obj->EmitSignal(tp, tp.intfParams[0]);
    // wait for a single signal
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
    // validate signal for I1 was seen
    proxy->ValidateSignals(tp, tp.intfParams[0]);
    // nothing expected for I2
    tp.rangePropListenExp.clear();
    proxy->ValidateSignals(tp, tp.intfParams[1]);
}

/*
 * Partially created ProxyBusObject scenario, where the ProxyBusObject only has
 * one interface I1 out of the interfaces on BusObject. Register a listener for
 * all properties of I1. Emit a PropChanged signal for properties belonging to
 * another interface I2 (I2 is not even present in ProxyBusObject). Verify that
 * the listener does NOT get called.
 */
TEST_F(PropChangedTest, ListenerNotCalled_PartialProxy)
{
    // one interface for client
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    // an extra interface for service
    TestParameters tpService = tpClient;
    tpService.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "2"));

    SetupPropChanged(tpService, tpClient);
    // emit on I2
    obj->EmitSignal(tpService, tpService.intfParams[1]);
    // expect time-out
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
}

/*
 * Copy proxy bus object, check if PropertiesChanged listeners function as expected
 */
TEST_F(PropChangedTest, Copy)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);

    PropChangedTestProxyBusObject* copy = new PropChangedTestProxyBusObject(*proxy);

    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, copy->signalSema.TimedWait(TIMEOUT));
    copy->Clear();

    delete proxy;
    proxy = NULL;

    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, copy->signalSema.TimedWait(TIMEOUT));

    delete copy;
}


/*
 * Property cache tests.
 * These are added here because they rely on the PropertiesChanged mechanism to maintain the cache.
 */

/*
 * Basic property update scenario, with EmitsChanged="true"
 */
TEST_F(PropChangedTest, PropertyCache_updating)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t intval = 0;

    /* GetProperty tests */

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);
    /* send update signal, see if the cache got updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* GetAllProperties tests */
    /* There is only one property in the interface, and that property is cacheable,
     * so we expect to see the cache behavior kick in for GetAllProperties too. */
    size_t numprops;
    MsgArg* props;
    const char* propname;
    MsgArg* propval;

    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 200);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);
    /* send update signal, see if the cache got updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(201, intval);
}

/*
 * Basic property update scenario, with EmitsChanged="invalidates"
 */
TEST_F(PropChangedTest, PropertyCache_invalidating)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "invalidates", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t intval = 0;

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);
    /* send update signal, see if the cache got updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* GetAllProperties tests */
    /* There is only one property in the interface, and that property is cacheable,
     * so we expect to see the cache behavior kick in for GetAllProperties too. */
    size_t numprops;
    MsgArg* props;
    const char* propname;
    MsgArg* propval;

    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 200);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);
    /* send update signal, see if the cache got invalidated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(201, intval);
}

/*
 * Basic property update scenario, with EmitsChanged="false"
 */
TEST_F(PropChangedTest, PropertyCache_notupdating)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "false", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t intval = 0;

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100);
    /* P1 is not cacheable, so we expect to see the updated value immediately */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);
    /* send update signal, should not arrive */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* GetAllProperties tests */
    size_t numprops;
    MsgArg* props;
    const char* propname;
    MsgArg* propval;

    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 200);
    /* P1 is not cacheable, so we expect to see the updated value immediately */
    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(201, intval);
    /* send update signal, should not arrive */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
    EXPECT_EQ(ER_OK, proxy->GetAllProperties(INTERFACE_NAME "1", value));
    EXPECT_EQ(ER_OK, value.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(201, intval);
}

/*
 * When the PBO is copied, the new PBO keeps a reference to the underlying state
 * of the original PBO and the reference count for that internal state is
 * incremented.  This allows for both copies to have access to the same cached
 * values.
 */
TEST_F(PropChangedTest, PropertyCache_copy)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t intval = 0;

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* OK, the cache is primed. When we copy the proxy, we expect the
     * caching to be enabled in the copy, but the cached values themselves
     * will not be copied along. The rationale for this is that it significantly
     * reduces code complexity in the Core library. You shouldn't copy
     * ProxyBusObjects, anyway.
     */
    ProxyBusObject copy = *proxy;
    obj->ChangePropertyValues(tpService, 100);
    /* object did not send property update signal:
     * - the original proxy should still see the original value (1)
     * - the copy should see the original value too since it shares the same underlying state
     */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);
    EXPECT_EQ(ER_OK, copy.GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* emit the signal and verify that the original proxy has its cache updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* change property value, do not send update signal.
     * both proxy and copy should return the old value.
     */
    obj->ChangePropertyValues(tpService, 200);
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);
    EXPECT_EQ(ER_OK, copy.GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* emit the signal and verify that both proxies have their caches updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    qcc::Sleep(50); //wait a while longer to make sure copy got its update too
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(201, intval);
    EXPECT_EQ(ER_OK, copy.GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(201, intval);

    /* now delete proxy and try again */
    delete proxy;
    proxy = NULL;

    /* is the cache still there? */
    obj->ChangePropertyValues(tpService, 300);
    EXPECT_EQ(ER_OK, copy.GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(201, intval);
    /* do we still respond to updates? */
    obj->EmitSignals(tpService);
    qcc::Sleep(400); //wait a while to make sure copy got its update
    EXPECT_EQ(ER_OK, copy.GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(301, intval);
}

/*
 * Caching feature should still work properly, even if the application does not
 * register a PropertiesChangedListener.
 */
TEST_F(PropChangedTest, PropertyCache_nolistener)
{
    TestParameters tpClient(true, Pnone, PCM_PROGRAMMATIC);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService(true, P1, P1, PCM_PROGRAMMATIC);
    tpService.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t intval = 0;

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &intval));
    EXPECT_EQ(1, intval);
    /* send update signal, see if the cache got updated */
    obj->EmitSignals(tpService);
    /* busy-wait for max 5 seconds */
    int count = 0;
    while (intval != 101 && count++ < 500) {
        qcc::Sleep(10);
        EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
        EXPECT_EQ(ER_OK, value.Get("i", &intval));
    }
    EXPECT_EQ(101, intval);
}

/*
 * test the asynchronous delivery mechanisms
 */
struct GetPropertyListener : public ProxyBusObject::Listener {
    qcc::Event ev;
    MsgArg val;

    GetPropertyListener() : ev(), val() { }
    virtual ~GetPropertyListener() { }

    void GetPropCallback(QStatus status, ProxyBusObject* obj, const MsgArg& value, void* context) {
        QCC_UNUSED(obj);
        QCC_UNUSED(context);

        EXPECT_EQ(ER_OK, status);
        val = value;
        ev.SetEvent();
    }
};

TEST_F(PropChangedTest, PropertyCache_async)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    int32_t intval = 0;
    GetPropertyListener gpl;

    /* GetProperty tests */

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetPropertyAsync(INTERFACE_NAME "1", "P1", &gpl,
                                             static_cast<ProxyBusObject::Listener::GetPropertyCB>(&GetPropertyListener::GetPropCallback),
                                             NULL, TIMEOUT));
    EXPECT_EQ(ER_OK, Event::Wait(gpl.ev, TIMEOUT));
    gpl.ev.ResetEvent();
    EXPECT_EQ(ER_OK, gpl.val.Get("i", &intval));
    EXPECT_EQ(1, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetPropertyAsync(INTERFACE_NAME "1", "P1", &gpl,
                                             static_cast<ProxyBusObject::Listener::GetPropertyCB>(&GetPropertyListener::GetPropCallback),
                                             NULL, TIMEOUT));
    EXPECT_EQ(ER_OK, Event::Wait(gpl.ev, TIMEOUT));
    gpl.ev.ResetEvent();
    EXPECT_EQ(ER_OK, gpl.val.Get("i", &intval));
    EXPECT_EQ(1, intval);
    /* send update signal, see if the cache got updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetPropertyAsync(INTERFACE_NAME "1", "P1", &gpl,
                                             static_cast<ProxyBusObject::Listener::GetPropertyCB>(&GetPropertyListener::GetPropCallback),
                                             NULL, TIMEOUT));
    EXPECT_EQ(ER_OK, Event::Wait(gpl.ev, TIMEOUT));
    gpl.ev.ResetEvent();
    EXPECT_EQ(ER_OK, gpl.val.Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* GetAllProperties tests */
    /* There is only one property in the interface, and that property is cacheable,
     * so we expect to see the cache behavior kick in for GetAllProperties too. */
    size_t numprops;
    MsgArg* props;
    const char* propname;
    MsgArg* propval;

    EXPECT_EQ(ER_OK, proxy->GetAllPropertiesAsync(INTERFACE_NAME "1", &gpl,
                                                  static_cast<ProxyBusObject::Listener::GetAllPropertiesCB>(&GetPropertyListener::GetPropCallback),
                                                  NULL, TIMEOUT));
    EXPECT_EQ(ER_OK, Event::Wait(gpl.ev, TIMEOUT));
    gpl.ev.ResetEvent();
    EXPECT_EQ(ER_OK, gpl.val.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 200);
    /* don't send update signal yet, we expect to see the old value because the
     * client does not know the value has changed */
    EXPECT_EQ(ER_OK, proxy->GetAllPropertiesAsync(INTERFACE_NAME "1", &gpl,
                                                  static_cast<ProxyBusObject::Listener::GetAllPropertiesCB>(&GetPropertyListener::GetPropCallback),
                                                  NULL, TIMEOUT));
    EXPECT_EQ(ER_OK, Event::Wait(gpl.ev, TIMEOUT));
    gpl.ev.ResetEvent();
    EXPECT_EQ(ER_OK, gpl.val.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(101, intval);
    /* send update signal, see if the cache got updated */
    obj->EmitSignals(tpService);
    EXPECT_EQ(ER_OK, proxy->signalSema.TimedWait(TIMEOUT));
    EXPECT_EQ(ER_OK, proxy->GetAllPropertiesAsync(INTERFACE_NAME "1", &gpl,
                                                  static_cast<ProxyBusObject::Listener::GetAllPropertiesCB>(&GetPropertyListener::GetPropCallback),
                                                  NULL, TIMEOUT));
    EXPECT_EQ(ER_OK, Event::Wait(gpl.ev, TIMEOUT));
    gpl.ev.ResetEvent();
    EXPECT_EQ(ER_OK, gpl.val.Get("a{sv}", &numprops, &props));
    EXPECT_EQ(1, (int)numprops);
    EXPECT_EQ(ER_OK, props[0].Get("{sv}", &propname, &propval));
    EXPECT_STREQ("P1", propname);
    EXPECT_EQ(ER_OK, propval->Get("i", &intval));
    EXPECT_EQ(201, intval);
}


class PropCacheUpdatedTestListener :
    public PropChangedTestListener  {

  private:

    String propName;
    int32_t expectInCache; // expected cached value for propName
    SampleStore store;
  public:
    PropCacheUpdatedTestListener(String _propName, int32_t _expectInCache) : PropChangedTestListener(store), propName(_propName), expectInCache(_expectInCache), strict(false)
    {
    }

    virtual ~PropCacheUpdatedTestListener() { }

    virtual void PropertiesChanged(ProxyBusObject& obj,
                                   const char* ifaceName,
                                   const MsgArg& changed,
                                   const MsgArg& invalidated,
                                   void* context)
    {
        QCC_UNUSED(context);
        QCC_UNUSED(invalidated);
        QCC_UNUSED(ifaceName);
        ASSERT_EQ(ALLJOYN_ARRAY, changed.typeId);

        //QCC_SyncPrintf("Prop changed MsgArg : %s", changed.ToString().c_str());

        if (strict) {
            MsgArg value;
            MsgArg* changedMsgArg;
            MsgArg* props;
            size_t numprops;
            const char* receivedPropName;
            int cachedVal;
            int changedVal;

            EXPECT_EQ(ER_OK, obj.GetProperty(INTERFACE_NAME "1", propName.c_str(), value));
            EXPECT_EQ(ER_OK, value.Get("i", &cachedVal));

            EXPECT_EQ(ER_OK, changed.Get("a{sv}", &numprops, &props));
            EXPECT_EQ(1, (int)numprops);
            EXPECT_EQ(ER_OK, props[0].Get("{sv}", &receivedPropName, &changedMsgArg));
            EXPECT_STREQ(propName.c_str(), receivedPropName);
            EXPECT_EQ(ER_OK, changedMsgArg->Get("i", &changedVal));

            EXPECT_EQ(cachedVal, changedVal);
            EXPECT_EQ(cachedVal, expectInCache);

        }
    }

    bool strict;

  private:
    /* Private assignment operator - does nothing */
    PropCacheUpdatedTestListener& operator=(const PropCacheUpdatedTestListener&);
};

/* only logically usable for maximum 2 proxies */
class PropCacheUpdatedConcurrentCallbackTestListener :
    public PropChangedTestListener  {

  private:
    BusAttachment& clientBus;
    PropChangedTestBusObject* busObj;
    Semaphore* events;
    TestParameters* tpService;
    bool first;
    int32_t expectedNewPropValue;
    int32_t unblockMainAfterNCallbacks;
    int32_t propChangedCallbackCount;
    int32_t unblockFirstAfterNCallbacks;
    Mutex mutex;
    SampleStore store;
  public:
    PropCacheUpdatedConcurrentCallbackTestListener(BusAttachment& _clientBus,
                                                   PropChangedTestBusObject* _busObj, Semaphore* _events,
                                                   TestParameters* _tpService) : PropChangedTestListener(store),
        clientBus(_clientBus), busObj(_busObj), events(_events), tpService(_tpService), first(true), expectedNewPropValue(0), unblockMainAfterNCallbacks(0), propChangedCallbackCount(0), unblockFirstAfterNCallbacks(0) {

    }

    virtual ~PropCacheUpdatedConcurrentCallbackTestListener() { }

    virtual void PropertiesChanged(ProxyBusObject& obj,
                                   const char* ifaceName,
                                   const MsgArg& changed,
                                   const MsgArg& invalidated,
                                   void* context)
    {
        QCC_UNUSED(context);
        QCC_UNUSED(invalidated);
        QCC_UNUSED(ifaceName);
        ASSERT_EQ(ALLJOYN_ARRAY, changed.typeId);
        //QCC_SyncPrintf("Prop changed MsgArg : %s\n", changed.ToString().c_str());

        mutex.Lock();
        propChangedCallbackCount++;
        bool firstValue = this->first;
        this->first = false;
        mutex.Unlock();

        if (firstValue) {

            ASSERT_TRUE(busObj != NULL);
            ASSERT_TRUE(events != NULL);
            ASSERT_TRUE(tpService != NULL);

            clientBus.EnableConcurrentCallbacks();
            CheckCachedVal(obj, changed);
            busObj->ChangePropertyValues(*tpService, expectedNewPropValue - 1); // second parameter is an offset
            busObj->EmitSignals(*tpService);

            for (int i = 0; i < (TIMEOUT - 1000) / 10; ++i) {
                mutex.Lock();
                int32_t count = propChangedCallbackCount;
                mutex.Unlock();
                if (count >= unblockFirstAfterNCallbacks) {
                    break;
                }
                qcc::Sleep(10);
            }

            EXPECT_TRUE(propChangedCallbackCount >= unblockFirstAfterNCallbacks);

        } else {

            clientBus.EnableConcurrentCallbacks();
            CheckCachedVal(obj, changed);

            mutex.Lock();
            int32_t count = propChangedCallbackCount;
            mutex.Unlock();
            if (count >= unblockMainAfterNCallbacks) {
                EXPECT_EQ(ER_OK, events->Post());
            }

        }

    }

    void SetExpectedNewPropValue(int32_t val) {
        expectedNewPropValue = val;
    }
    void SetUnblockMainAfterNCallbacks(int32_t num) {
        unblockMainAfterNCallbacks = num;
        unblockFirstAfterNCallbacks = (num / 2) + 1;
    }

  private:

    void CheckCachedVal(ProxyBusObject& obj, const MsgArg& changed) {
        MsgArg value;
        MsgArg* changedMsgArg;
        MsgArg* props;
        size_t numprops;
        const char* receivedPropName;
        int cachedVal;
        int changedVal;

        EXPECT_EQ(ER_OK, obj.GetProperty(INTERFACE_NAME "1", "P1", value));
        EXPECT_EQ(ER_OK, value.Get("i", &cachedVal));

        EXPECT_EQ(ER_OK, changed.Get("a{sv}", &numprops, &props));
        EXPECT_EQ(1, (int )numprops);
        EXPECT_EQ(ER_OK, props[0].Get("{sv}", &receivedPropName, &changedMsgArg));
        EXPECT_STREQ("P1", receivedPropName);
        EXPECT_EQ(ER_OK, changedMsgArg->Get("i", &changedVal));

        EXPECT_TRUE(cachedVal >= changedVal); // cached value should reflect the present or the future and never the past

    }

    /* Private assignment operator - does nothing */
    PropCacheUpdatedConcurrentCallbackTestListener& operator=(const PropCacheUpdatedConcurrentCallbackTestListener&);
};

/*
 * Test that makes sure the cache is already updated upon PropertyChanged callback
 * and that the round-trip was done only once; one remote get call.
 */

TEST_F(PropChangedTest, PropertyCache_updatedUponPropChangedCallback)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t val = 0;
    int32_t expectedVal = 101;

    // set-up client's listener
    PropCacheUpdatedTestListener* l = new PropCacheUpdatedTestListener("P1", expectedVal);
    l->strict = true;
    proxy->RegisterListener(l, tpClient.intfParams[0].name, P1);

    /* initial value is 1 */
    ASSERT_TRUE(obj->getsPerPropName.end() == obj->getsPerPropName.find("P1"));
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(1, val); // still, the signal of prop value changed was not emitted

    EXPECT_TRUE(obj->getsPerPropName.end() != obj->getsPerPropName.find("P1"));
    EXPECT_EQ(1, obj->getsPerPropName.at("P1"));

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, expectedVal - 1); // second argument is the offset
    // this would internally cause another Get call on the BusObject so that should be taken into account
    obj->EmitSignals(tpService);
    proxy->WaitForSignals(tpClient);

    // cached value was used - comparing to "2" just to take into account the Get called once from EmitSignals internals
    EXPECT_EQ(2, obj->getsPerPropName.at("P1"));

    proxy->UnregisterPropertiesChangedListener(INTERFACE_NAME "1", *l);
    delete l;
}

/*
 * Test that makes sure the cache holds only future (most recent) values in
 * the case where concurrent callbacks is enabled on the client's bus.
 * A single ProxyBusObject is foreseen.
 */

TEST_F(PropChangedTest, PropertyCache_consistentWithConcurrentCallback)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient);
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    MsgArg value;
    int32_t val = 0;

    // set-up client's listener
    Semaphore events;
    PropCacheUpdatedConcurrentCallbackTestListener* l = new PropCacheUpdatedConcurrentCallbackTestListener(clientBus, obj, &events, &tpService);
    int32_t expectedNewPropValue = 201;
    l->SetExpectedNewPropValue(expectedNewPropValue); // will be emitted in the listener
    l->SetUnblockMainAfterNCallbacks(2); // another signal emission also occurs in the listener
    proxy->RegisterListener(l, tpClient.intfParams[0].name, P1);

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(1, val); // still, the signal of prop value changed was not emitted

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100); // second argument is the offset - now we have 101
    obj->EmitSignals(tpService);

    EXPECT_EQ(ER_OK, events.TimedWait(TIMEOUT));

    /* expected value is expectedNewPropValue */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(expectedNewPropValue, val);

    proxy->UnregisterPropertiesChangedListener(INTERFACE_NAME "1", *l);
    delete l;
}

/*
 * Test that makes sure the cache holds only future (most recent) values in
 * the case where concurrent callbacks is enabled on the client's bus.
 * Two ProxyBusObjects are foreseen with the same property change listener.
 *
 * Scenario:
 *  proxy.changed(value = 101) -> change value to 201, enable concurrency, emit signals and block
 *  proxy.changed(value = 201)
 *  otherProxy.changed(value = 201) -> at this moment, first proxy property changed callback will unblock
 *  otherProxy.changed(value = 101) -> unblock main test thread
 *
 *  Cached value for both proxies should be 201
 */

TEST_F(PropChangedTest, PropertyCache_consistentWithConcurrentCallbackMultiProxy)
{
    TestParameters tpClient(true, P1, P1, PCM_INTROSPECT);
    tpClient.AddInterfaceParameters(InterfaceParameters(P1, "true", false, INTERFACE_NAME "1"));
    TestParameters tpService = tpClient;

    SetupPropChanged(tpService, tpClient); // this results in original proxy and obj
    proxy->EnablePropertyCaching();
    /* Wait a little while for the property cache to be enabled.
     * We don't have an easy way to detect when this is the case, so we just rely on a long-enough sleep */
    qcc::Sleep(WAIT_CACHE_ENABLED_MS);

    // create more ProxyBusObjects using the same busattachment; clientBus
    Semaphore events;

    PropChangedTestProxyBusObject* anotherProxy;
    PropCacheUpdatedConcurrentCallbackTestListener* l = new PropCacheUpdatedConcurrentCallbackTestListener(clientBus, obj, &events, &tpService);
    int32_t expectedNewPropValue = 201;
    l->SetExpectedNewPropValue(expectedNewPropValue); // will be emitted in the listener
    l->SetUnblockMainAfterNCallbacks(4);  // another signal emission also occurs in the listener
    proxy->RegisterListener(l, tpClient.intfParams[0].name, P1);
    anotherProxy = new PropChangedTestProxyBusObject(clientBus, serviceName, tpClient);
    anotherProxy->RegisterListener(l, tpClient.intfParams[0].name, P1);

    MsgArg value;
    int32_t val = 0;

    /* initial value is 1 */
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(1, val); // still, the signal of prop value changed was not emitted
    val = 0;
    EXPECT_EQ(ER_OK, anotherProxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(1, val); // still, the signal of prop value changed was not emitted

    /* change the value in the object */
    obj->ChangePropertyValues(tpService, 100); // second argument is the offset - now we have 101
    obj->EmitSignals(tpService);

    EXPECT_EQ(ER_OK, events.TimedWait(TIMEOUT));

    /* expected value is expectedNewPropValue */
    val = 0;
    EXPECT_EQ(ER_OK, proxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(expectedNewPropValue, val);

    val = 0;
    EXPECT_EQ(ER_OK, anotherProxy->GetProperty(INTERFACE_NAME "1", "P1", value));
    EXPECT_EQ(ER_OK, value.Get("i", &val));
    EXPECT_EQ(expectedNewPropValue, val);

    anotherProxy->UnregisterPropertiesChangedListener(INTERFACE_NAME "1", *l);
    proxy->UnregisterPropertiesChangedListener(INTERFACE_NAME "1", *l);
    delete anotherProxy;
    delete l;
}


class SecPropChangedTest :
    public::testing::Test,
    public ProxyBusObject::PropertiesChangedListener {
  public:
    SecPropChangedTest() : prov("provider"), cons("consumer"), proxy(NULL), evented(false) { }
    ~SecPropChangedTest() { }

    TestSecurityManager tsm;
    TestSecureApplication prov;
    TestSecureApplication cons;
    shared_ptr<ProxyBusObject> proxy;
    Mutex lock;
    Condition condition;
    bool evented;

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, tsm.Init());
        ASSERT_EQ(ER_OK, prov.Init(tsm));
        ASSERT_EQ(ER_OK, cons.Init(tsm));

        ASSERT_EQ(ER_OK, prov.HostSession());
        SessionId sid = 0;
        ASSERT_EQ(ER_OK, cons.JoinSession(prov, sid));

        proxy = shared_ptr<ProxyBusObject>(cons.GetProxyObject(prov, sid));
        ASSERT_TRUE(proxy != NULL);
        qcc::Sleep(WAIT_CACHE_ENABLED_MS);
        const char* props[1];
        props[0] = TEST_PROP_NAME;
        proxy->RegisterPropertiesChangedListener(TEST_INTERFACE, props, 1, *this, NULL);
    }

    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context)
    {
        QCC_UNUSED(obj);
        QCC_UNUSED(ifaceName);
        QCC_UNUSED(changed);
        QCC_UNUSED(changed);
        QCC_UNUSED(invalidated);
        QCC_UNUSED(context);
        lock.Lock();
        evented = true;
        condition.Signal();
        lock.Unlock();
    }

    QStatus SendAndWaitForEvent(TestSecureApplication& prov, bool newValue = true)
    {
        lock.Lock();
        evented = false;
        QStatus status = prov.UpdateTestProperty(newValue);
        if (ER_OK != status) {
            cout << "Failed to send event" << __FILE__ << "@" << __LINE__ << endl;
        } else {
            condition.TimedWait(lock, 5000);
            status = evented ? ER_OK : ER_FAIL;
            evented = false;
        }
        lock.Unlock();
        return status;
    }

    QStatus GetProperty()
    {
        MsgArg arg;
        return proxy->GetProperty(TEST_INTERFACE, TEST_PROP_NAME, arg, 5000);
    }

    QStatus GetAllProperties(size_t expected = 2)
    {
        MsgArg props;
        QStatus status = proxy->GetAllProperties(TEST_INTERFACE, props);
        if (ER_OK != status) {
            return status;
        }
        size_t numprops;
        MsgArg* propValues;

        status = props.Get("a{sv}", &numprops, &propValues);
        if (ER_OK != status) {
            EXPECT_EQ(ER_OK, status);
            return status;
        }

        if (expected != numprops) {
            EXPECT_EQ(expected, numprops);
            return ER_FAIL;
        }
        bool foundProp1 = false;
        bool foundProp2 = false;
        for (size_t i = 0; i < numprops; i++) {
            MsgArg* propval;
            const char* propname;
            status = propValues[i].Get("{sv}", &propname, &propval);
            if (ER_OK != status) {
                EXPECT_EQ(ER_OK, status) << "Got invalid property signature  @index " << i;
                return status;
            }
            if (strcmp(propname, TEST_PROP_NAME) == 0) {
                if (foundProp1) {
                    EXPECT_FALSE(foundProp1) << "loop " << i;
                    return ER_FAIL;
                }
                foundProp1 = true;
            } else if (strcmp(propname, TEST_PROP_NAME2) == 0) {
                if (foundProp2) {
                    EXPECT_FALSE(foundProp2) << "loop " << i;
                    return ER_FAIL;
                }
                foundProp2 = true;
            } else {
                EXPECT_TRUE(false) << "Unknown property name '" << propname << "'";
                return ER_BUS_NO_SUCH_PROPERTY;
            }
        }
        return status;
    }

    QStatus CheckProperty(bool expected)
    {
        MsgArg arg;
        QStatus status = proxy->GetProperty(TEST_INTERFACE, TEST_PROP_NAME, arg, 5000);
        EXPECT_EQ(ER_OK, status) << "Failed to GetProperty";
        if (ER_OK != status) {
            return status;
        }
        bool prop = !expected;
        status = arg.Get("b", &prop);
        EXPECT_EQ(ER_OK, status) << "Failed to Get bool value out of MsgArg";
        return prop == expected ? ER_OK : ER_FAIL;
    }

};

/**
 * Basic test for the property cache with security enabled.
 * Test events being sent when policy is configured properly.
 */
TEST_F(SecPropChangedTest, PropertyCache_SecurityEnabled)
{
    proxy->EnablePropertyCaching();
    // No policy set. So GetProperty should be denied by remote policy.
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());

    // Set Policy on provider & consumer.
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY));
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, CheckProperty(false));
    ASSERT_EQ(ER_OK, SendAndWaitForEvent(prov));
    ASSERT_EQ(ER_OK, CheckProperty(true));
    ASSERT_EQ(1, prov.GetCurrentGetPropertyCount());
}

/**
 * Basic test for the property cache with security enabled. The test validates that no events
 * are received when the provider policy doesn't allow it.
 */
TEST_F(SecPropChangedTest, PropertyCache_NotAllowedByProviderPolicy)
{
    proxy->EnablePropertyCaching();
    // Set Policy on consumer & provider.
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, prov.UpdateTestProperty(true));
    // We should not get an event. Sleep for a while and see if we got one
    qcc::Sleep(500);
    ASSERT_FALSE(evented);
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());
    ASSERT_EQ(0, prov.GetCurrentGetPropertyCount());
}

/**
 * Basic test for the property cache with security enabled. The test validates that events
 * are received when the provider uses a default policy.
 */
TEST_F(SecPropChangedTest, PropertyCache_DefaultProviderPolicy)
{
    proxy->EnablePropertyCaching();
    // Set Policy on consumer.
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());
    ASSERT_EQ(ER_OK, prov.UpdateTestProperty(true));
    // We should not get an event. Sleep for a while and see if we got one
    // The default policy of the provider does not allow to observe properties
    qcc::Sleep(500);
    ASSERT_FALSE(evented);
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());
    ASSERT_EQ(0, prov.GetCurrentGetPropertyCount());
}

/**
 * Basic test for the property cache with security enabled. The test validates that events
 * are not received when the consmuer uses a default policy.
 */
TEST_F(SecPropChangedTest, PropertyCache_DefaultConsumerPolicy)
{
    proxy->EnablePropertyCaching();
    // Set Policy on provider.
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY));
    ASSERT_EQ(ER_OK, prov.UpdateTestProperty(true));
    // We should not get an event. Sleep for a while and see if we got one
    qcc::Sleep(500);
    ASSERT_FALSE(evented);
    // Default policy of consumer should allow the outgoing message
    ASSERT_EQ(ER_OK, CheckProperty(true));
    ASSERT_EQ(1, prov.GetCurrentGetPropertyCount());
}

/**
 * Basic test for the property cache with security enabled. The test validates that events
 * are not received when the consumer policy doesn't allow it.
 */
TEST_F(SecPropChangedTest, PropertyCache_NotAllowedByConsumerPolicy)
{
    proxy->EnablePropertyCaching();
    // Set Policy on provider & consumer.
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY));
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_MODIFY));
    // No matching policy set. So GetProperty should be denied.
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());

    ASSERT_EQ(ER_OK, prov.UpdateTestProperty(true));
    // We should not get an event. Sleep for a while and see if we got one
    qcc::Sleep(500);
    ASSERT_FALSE(evented);
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());
    ASSERT_EQ(0, prov.GetCurrentGetPropertyCount());
}

/**
 * Basic test for the property cache with security enabled. The test validates that events are
 * not received when the policy doesn't specify it.
 */
TEST_F(SecPropChangedTest, PropertyCache_WrongInterfaceName)
{
    proxy->EnablePropertyCaching();
    // Set Policy on consumer & provider.
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE, "wrong.interface"));
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE, "wrong.interface"));
    ASSERT_EQ(ER_OK, prov.UpdateTestProperty(true));
    // We should not get an event. Sleep for a while and see if we got one
    qcc::Sleep(500);
    ASSERT_FALSE(evented);
    ASSERT_EQ(ER_PERMISSION_DENIED, GetProperty());
    ASSERT_EQ(0, prov.GetCurrentGetPropertyCount());
}

static void FillPolicy(const char* propertyName, PermissionPolicy& policy)
{
    PermissionPolicy::Acl acl;
    PermissionPolicy::Rule::Member member;
    member.SetMemberName(propertyName);
    member.SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    PermissionPolicy::Rule rules;
    rules.SetInterfaceName(TEST_INTERFACE);
    rules.SetMembers(1, &member);
    acl.SetRules(1, &rules);
    PermissionPolicy::Peer peer;
    peer.SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acl.SetPeers(1, &peer);
    policy.SetAcls(1, &acl);
}

/**
 * A test validating the behavior of the GetAllProperties call with security enabled.
 */
TEST_F(SecPropChangedTest, GetAllProperties)
{
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE));
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));

    ASSERT_EQ(ER_OK, GetAllProperties());
    ASSERT_EQ(2, prov.GetCurrentGetPropertyCount());

    // Policy allows none of the provided properties
    // Bad Consumer policy
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE, "wrong.interface"));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(2, prov.GetCurrentGetPropertyCount());

    // Bad Provider Policy
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE, "wrong.interface"));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(2, prov.GetCurrentGetPropertyCount());

    // Policy allows some of the provided properties
    PermissionPolicy policy;
    FillPolicy(TEST_PROP_NAME, policy);
    PermissionPolicy policy2;
    FillPolicy(TEST_PROP_NAME2, policy2);

    // Use partial provider policy:
    ASSERT_EQ(ER_OK, prov.SetPolicy(tsm, policy));
    // Auth failure. Provider dropped master secret after policy update, but consumer doesn't know
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, GetAllProperties());
    ASSERT_EQ(ER_OK, GetAllProperties(1));
    ASSERT_EQ(3, prov.GetCurrentGetPropertyCount());

    ASSERT_EQ(ER_OK, prov.SetPolicy(tsm, policy2));
    // Auth failure. Provider dropped master secret after policy update, but consumer doesn't know
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, GetAllProperties());
    ASSERT_EQ(ER_OK, GetAllProperties(1));
    ASSERT_EQ(4, prov.GetCurrentGetPropertyCount());

    // Use partial consumer policy:
    ASSERT_EQ(ER_OK, cons.SetPolicy(tsm, policy));
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY, "wrong.interface"));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(4, prov.GetCurrentGetPropertyCount());

    // Both partial policy
    ASSERT_EQ(ER_OK, prov.SetPolicy(tsm, policy));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(4, prov.GetCurrentGetPropertyCount());

    // Reset policy to valid ones.
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, GetAllProperties(1));
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE));
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, GetAllProperties());
    ASSERT_EQ(ER_OK, GetAllProperties());
    ASSERT_EQ(7, prov.GetCurrentGetPropertyCount());

    // Update Manifest - bad manifest on provider
    ASSERT_EQ(ER_OK, prov.UpdateManifest(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE, "wrong.interface"));
    // Auth failure. Provider dropped master secret after policy update, but consumer doesn't know
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, GetAllProperties());
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(7, prov.GetCurrentGetPropertyCount());

    // Update Manifest - bad manifest on consumer
    ASSERT_EQ(ER_OK, prov.UpdateManifest(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, cons.UpdateManifest(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE, "wrong.interface"));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(7, prov.GetCurrentGetPropertyCount());

    // Partial manifests.
    ASSERT_EQ(ER_OK, cons.UpdateManifest(tsm, *policy.GetAcls()));
    ASSERT_EQ(ER_OK, GetAllProperties(1));
    ASSERT_EQ(8, prov.GetCurrentGetPropertyCount());

    ASSERT_EQ(ER_OK, prov.UpdateManifest(tsm, *policy.GetAcls()));
    ASSERT_EQ(ER_OK, cons.UpdateManifest(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE));
    ASSERT_EQ(ER_PERMISSION_DENIED, GetAllProperties());
    ASSERT_EQ(8, prov.GetCurrentGetPropertyCount());
}

/**
 * A test validating the behavior of the GetAllProperties call with security enabled and no common
 * root of trust is in place.
 */
TEST_F(SecPropChangedTest, GetAllPropertiesNoAuth)
{
    TestSecurityManager tsm2("tsm2");
    ASSERT_EQ(ER_OK, tsm2.Init());
    TestSecureApplication cons2("other.consumer");
    ASSERT_EQ(ER_OK, cons2.Init(tsm2));
    SessionId sid = 0;
    ASSERT_EQ(ER_OK, cons2.JoinSession(prov, sid));
    shared_ptr<ProxyBusObject> proxy2 = shared_ptr<ProxyBusObject>(cons2.GetProxyObject(prov, sid));
    ASSERT_TRUE(proxy2 != NULL);

    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE));
    ASSERT_EQ(ER_OK, cons2.SetAnyTrustedUserPolicy(tsm2, PermissionPolicy::Rule::Member::ACTION_PROVIDE));

    MsgArg props;
    ASSERT_EQ(ER_PERMISSION_DENIED, proxy2->GetAllProperties(TEST_INTERFACE, props));
}
