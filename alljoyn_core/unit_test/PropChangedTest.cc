/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <gtest/gtest.h>

#include <qcc/Condition.h>
#include <qcc/Debug.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>

#include "ajTestCommon.h"

using namespace ajn;
using namespace qcc;
using namespace std;

/* client/service synchronization time-out */
#define TIMEOUT 5000

/* time-out before emitting signal */
#define TIMEOUT_BEFORE_SIGNAL 100

/* time-out te be used for tests that expect to time-out */
#define TIMEOUT_EXPECTED 500

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

    TestParameters& AddListener(const Range& rangePropListen)
    {
        this->rangePropListen.push_back(rangePropListen);
        this->rangePropListenExp.push_back(rangePropListen); // same as listen
        return *this;
    }

    TestParameters& AddListener(const Range& rangePropListen,
                                const Range& rangePropListenExp)
    {
        this->rangePropListen.push_back(rangePropListen);
        this->rangePropListenExp.push_back(rangePropListenExp);
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = intf.AddPropertyAnnotation(name,
                                        ajn::org::freedesktop::DBus::AnnotateEmitsChanged, annotation);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
        }
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = bus.RegisterBusObject(*this);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
        val.v_int32 = num;
        //QCC_SyncPrintf("PropChangedTestBusObject::Get(%s, %s) -> %d\n", ifcName, propName, num);
        return ER_OK;
    }

    void EmitSignal(const TestParameters& tp,
                    const InterfaceParameters& ip,
                    SessionId id = 0)
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
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    vector<const ProxyBusObject*> proxySamples;
    map<String, vector<MsgArg> > changedSamples;
    map<String, vector<MsgArg> > invalidatedSamples;

    void AddSample(const ProxyBusObject& proxy,
                   const char* ifaceName,
                   const MsgArg& changed,
                   const MsgArg& invalidated)
    {
        mutex.Lock();
        proxySamples.push_back(&proxy);
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
        ASSERT_EQ(ALLJOYN_ARRAY, changed.typeId);
        ASSERT_EQ(ALLJOYN_ARRAY, invalidated.typeId);
        //QCC_SyncPrintf("PropChangedTestListener::PropertiesChanged called (changed:%d,invalidated:%d) for %s\n", changed.v_array.GetNumElements(), invalidated.v_array.GetNumElements(), (context ? (const char*)context : "?"));
        store.AddSample(obj, ifaceName, changed, invalidated);
    }
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
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        RegisterBusListener(*this);
        status = FindAdvertisedName(serviceName);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void WaitForSession()
    {
        status = sessionSema.TimedWait(TIMEOUT);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void FoundAdvertisedName(const char* name,
                             TransportMask transport,
                             const char* namePrefix)
    {
        JoinSessionAsync(name, SERVICE_PORT, NULL, SESSION_OPTS, this, NULL);
    }

    void JoinSessionCB(QStatus status,
                       SessionId sessionId,
                       const SessionOpts& opts,
                       void* context)
    {
        EXPECT_EQ(ER_OK, status);
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

    PropChangedTestProxyBusObject(ClientBusAttachment& clientBus,
                                  String serviceName,
                                  const TestParameters& tp,
                                  const char* path = OBJECT_PATH) :
        ProxyBusObject(clientBus, serviceName.c_str(), path, clientBus.id),
        status(ER_OK),
        bus(clientBus)
    {
        //QCC_SyncPrintf("PropChangedTestProxyBusObject::constructor for %s\n", path);
        status = AddProxyInterface(bus, *this, tp);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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

    ~PropChangedTestProxyBusObject()
    {
        for (size_t i = 0; i < listeners.size(); i++) {
            delete listeners[i];
        }
    }

    void RegisterListener(PropChangedTestListener* listener,
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
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    /* By default we do not expect a time-out when waiting for the signals.  If
     * the \a expectTimeout argument is provided an different from 0 we will
     * wait for that amount of milliseconds and will NOT expect any signals. */
    void WaitForSignals(const TestParameters& tp,
                        uint32_t expectTimeout = 0)
    {
        uint32_t timeout = expectTimeout ? expectTimeout : TIMEOUT;
        QStatus expStatus = expectTimeout ? ER_TIMEOUT : ER_OK;

        /* wait for signals for all interfaces on all listeners */
        size_t num = tp.intfParams.size() * tp.rangePropListen.size();
        for (size_t i = 0; i < num; i++) {
            //QCC_SyncPrintf("WaitForSignals %dms on listener %d\n", timeout, i);
            status = TimedWait(timeout); // wait for property changed signal
            EXPECT_EQ(expStatus, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    void ValidateSignals(const TestParameters& tp,
                         const InterfaceParameters& ip)
    {
        int emitChanged = 0, emitInvalidated = 0;
        const MsgArg* elems;
        int n, i;

        // ensure correct number of samples
        ASSERT_EQ(tp.rangePropListenExp.size(), changedSamples[ip.name].size());
        ASSERT_EQ(tp.rangePropListenExp.size(), invalidatedSamples[ip.name].size());

        if (ip.emitsChanged == "true") {
            emitChanged = tp.rangePropEmit.Size();
        } else if (ip.emitsChanged == "invalidates") {
            emitInvalidated = tp.rangePropEmit.Size();
        }
        //QCC_SyncPrintf("ValidateSignal expects %d samples with each %d changed, %d invalidated\n", tp.rangePropListenExp.size(), emitChanged, emitInvalidated);

        // loop over all samples
        for (size_t sample = 0; sample < tp.rangePropListenExp.size(); sample++) {
            MsgArg& changed = changedSamples[ip.name][sample];
            MsgArg& invalidated = invalidatedSamples[ip.name][sample];

            ASSERT_EQ(ALLJOYN_ARRAY, changed.typeId);
            n = changed.v_array.GetNumElements();
            int numListen = tp.rangePropListenExp[sample].Size();
            if (0 == numListen) {
                // listen to all
                numListen = tp.rangePropEmit.Size();
            }
            ASSERT_EQ(min(emitChanged, numListen), n);
            elems = changed.v_array.GetElements();
            for (i = 0; i < n; i++) {
                int num = tp.rangePropListenExp[sample].first + i;

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
                int num = tp.rangePropListenExp[sample].first + i;

                // validate property name
                ASSERT_EQ(ALLJOYN_STRING, elems[i].typeId);
                String name = String("P").append('0' + num);
                EXPECT_STREQ(name.c_str(), elems[i].v_string.str);
            }
        }
    }

    void ValidateSignals(const TestParameters& tp)
    {
        for (size_t i = 0; i < tp.intfParams.size(); i++) {
            //QCC_SyncPrintf("ValidateSignal for %s\n", tp.intfParams[i].name.c_str());
            ValidateSignals(tp, tp.intfParams[i]);
        }
    }

    void ValidateSignals(const InterfaceParameters& ip)
    {
        TestParameters tp(true, ip);
        ValidateSignals(tp);
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
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.BindSessionPort(SERVICE_PORT, SESSION_OPTS, *this);

        status = serviceBus.RequestName(serviceName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.AdvertiseName(serviceName.c_str(), TRANSPORT_ANY);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    void TestPropChanged(const TestParameters& tp)
    {
        TestPropChanged(tp, tp);
    }

    bool AcceptSessionJoiner(SessionPort sessionPort,
                             const char* joiner,
                             const SessionOpts& opts)
    {
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
    // EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((size_t)1, store.proxySamples.size());
    EXPECT_EQ(dynamic_cast<ProxyBusObject*>(&pb1), store.proxySamples[0]);

    // test for pb2 (both l and l2)
    store.Clear();
    EXPECT_EQ((size_t)0, store.proxySamples.size());
    bobA.EmitSignal(tp, tp.intfParams[0], clientBus2.id);
    qcc::Sleep(500);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((size_t)2, store.proxySamples.size());
    EXPECT_EQ(dynamic_cast<ProxyBusObject*>(&pb2), store.proxySamples[0]);
    EXPECT_EQ(dynamic_cast<ProxyBusObject*>(&pb2), store.proxySamples[1]);

    // test for pb3 (only l)
    store.Clear();
    EXPECT_EQ((size_t)0, store.proxySamples.size());
    bobB.EmitSignal(tp, tp.intfParams[0], clientBus.id);
    qcc::Sleep(500);
    // status = store.TimedWait(TIMEOUT); // wait for property changed signal
    // EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((size_t)1, store.proxySamples.size());
    EXPECT_EQ(dynamic_cast<ProxyBusObject*>(&pb3), store.proxySamples[0]);

    // clean up
    pb2.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l2);
    pb3.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l);
    pb2.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l);
    pb1.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), l);
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

    PropChangedTestBusObject obj(serviceBus, tp.intfParams);
    /* Invoke the newly added EmitPropChanged with NULL as the interface name.
     * ER_OK should not be returned. */
    EXPECT_NE(ER_OK, obj.EmitPropChanged(NULL, okProps, 1, 0));
    /* Invoke the newly added EmitPropChanged with an invalid interface name.
     * ER_OK should not be returned. */
    EXPECT_NE(ER_OK, obj.EmitPropChanged("invalid.interface", okProps, 1, 0));
    /* Invoke the newly added EmitPropChanged with an invalid property name.
     * ER_OK should not be returned. */
    EXPECT_NE(ER_OK, obj.EmitPropChanged(tp.intfParams[0].name.c_str(), nokProps, 2, 0));
    /* Invoke the newly added EmitPropChanged with a mixture of valid and
     * invalid properties. ER_OK should not be returned */
    EXPECT_NE(ER_OK, obj.EmitPropChanged(tp.intfParams[0].name.c_str(), mixProps, 2, 0));
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
    PropChangedTestBusObject obj(serviceBus, tp.intfParams);
    PropChangedTestProxyBusObject proxy(clientBus, serviceName, tp);
    // extra listener for testing
    PropChangedTestListener listener(proxy);

    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with NULL as the interface parameter. The return code should be
     * ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxy.RegisterPropertiesChangedListener(NULL, okProps, 1, listener, NULL);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with an invalid string as an interface parameter.  The return code
     * should be ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxy.RegisterPropertiesChangedListener("invalid.interface", okProps, 1, listener, NULL);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with a non-existent property. The return code should be
     * ER_BUS_NO_SUCH_PROPERTY. */
    status = proxy.RegisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), nokProps, 1, listener, NULL);
    EXPECT_EQ(ER_BUS_NO_SUCH_PROPERTY, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create a ProxyBusObject and invoke RegisterPropertiesChangedListener
     * with an array of properties that contains a mix of valid properties
     * and invalid / non-existent properties. The return code should be
     * ER_BUS_NO_SUCH_PROPERTY. */
    status = proxy.RegisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), mixProps, 2, listener, NULL);
    EXPECT_EQ(ER_BUS_NO_SUCH_PROPERTY, status) << "  Actual Status: " << QCC_StatusText(status);
}

/*
 * The following are the tests that check the return codes of
 * UnregisterPropertiesChangedListener.
 */
TEST_F(PropChangedTest, NegativeUnregisterPropertiesChangedListener)
{
    TestParameters tp(true, InterfaceParameters(P1));

    clientBus.WaitForSession();
    PropChangedTestBusObject obj(serviceBus, tp.intfParams);
    PropChangedTestProxyBusObject proxy(clientBus, serviceName, tp);

    /* Create a ProxyBusObject and register a listener. Invoke
     * UnregisterPropertiesChangedListener with NULL as interface parameter.
     * The return code should be ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxy.UnregisterPropertiesChangedListener(NULL, *proxy.listeners[0]);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create a ProxyBusObject and register a listener. Invoke
     * UnregisterPropertiesChangedListener with a non-existent random string
     * as interface parameter. The return code should be
     * ER_BUS_OBJECT_NO_SUCH_INTERFACE. */
    status = proxy.UnregisterPropertiesChangedListener("invalid.interface", *proxy.listeners[0]);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);
}

/*
 * Create a ProxyBusObject and try to unregister a never registered
 * listener. The return code should be ER_OK.
 */
TEST_F(PropChangedTest, NegativeUnregisterInvalidPropertiesChangedListener)
{
    TestParameters tp(true, InterfaceParameters(P1));

    clientBus.WaitForSession();
    PropChangedTestBusObject obj(serviceBus, tp.intfParams);
    PropChangedTestProxyBusObject proxy(clientBus, serviceName, tp);

    SampleStore store;
    PropChangedTestListener invalid(store);
    status = proxy.UnregisterPropertiesChangedListener(tp.intfParams[0].name.c_str(), invalid);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    // fire signal again and expect no callback to be called
    obj->EmitSignals(tp);
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    // fire signal for I2 and expect time-out
    obj->EmitSignal(tp, tp.intfParams[1]);
    EXPECT_EQ(ER_TIMEOUT, proxy->signalSema.TimedWait(TIMEOUT_EXPECTED));
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
