/**
 * @file
 *
 * Program to test bus object properties.
 */

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

#include <assert.h>
#include <signal.h>
#include <stdio.h>

#include <qcc/Debug.h>
#include <qcc/Event.h>
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



static const char* propTesterInterfaceXML =
    "<node name=\"/org/alljoyn/Testing/PropertyTester\">"
    "  <interface name=\"org.alljoyn.Testing.PropertyTester\">"

    "    <property name=\"int32\" type=\"i\" access=\"readwrite\"/>"
    "    <property name=\"uint32\" type=\"u\" access=\"read\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "    <property name=\"string\" type=\"s\" access=\"write\"/>"
    "    <property name=\"sessionId\" type=\"u\" access=\"read\"/>"

    "  </interface>"
    "</node>";

static const char* propTester2InterfaceXML =
    "<node name=\"/org/alljoyn/Testing/PropertyTester\">"
    "  <interface name=\"org.alljoyn.Testing.PropertyTester2\">"


    "    <property name=\"int1\" type=\"u\" access=\"read\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "    <property name=\"int2\" type=\"u\" access=\"read\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"invalidates\"/>"
    "    </property>"
    "    <property name=\"int3\" type=\"u\" access=\"read\">"
    "    </property>"
    "    <property name=\"string1\" type=\"s\" access=\"read\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "    <property name=\"string2\" type=\"s\" access=\"read\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"invalidates\"/>"
    "    </property>"
    "    <property name=\"string3\" type=\"s\" access=\"read\">"
    "    </property>"

    "  </interface>"
    "</node>";

static const char* propTester2Names[] = { "int1", "int2", "int3", "string1", "string2", "string3" };
static const int propTester2Count = 6;
static bool multiProp = true;
static bool singleProp = true;


class PropTesterObject : public BusObject, private Thread {
  public:
    PropTesterObject(BusAttachment& bus, const char* path, SessionId id, bool autoChange);
    ~PropTesterObject();

    void Set(int32_t v);
    void Set(uint32_t v);
    void Set(const char* v);

    void ObjectRegistered(void) { if (autoChange) { Start(); } }

  private:

    const bool autoChange;
    int32_t int32Prop;    // RW property
    uint32_t uint32Prop;  // RO property
    String stringProp;    // RW property
    SessionId id;         // SessionId and RO property
    bool stop;
    Mutex lock;

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    QStatus Set(const char* ifcName, const char* propName, MsgArg& val);

    // Thread methods
    ThreadReturn STDCALL Run(void* arg);
};

PropTesterObject::PropTesterObject(BusAttachment& bus, const char* path, SessionId id, bool autoChange) :
    BusObject(path),
    autoChange(autoChange),
    int32Prop(0),
    uint32Prop(0),
    stringProp(path),
    id(id),
    stop(false)
{
    const InterfaceDescription* ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester");
    if (!ifc) {
        bus.CreateInterfacesFromXml(propTesterInterfaceXML);
        ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester");
    }
    assert(ifc);

    AddInterface(*ifc);
}

PropTesterObject::~PropTesterObject()
{
    lock.Lock();
    stop = true;
    lock.Unlock();

    if (autoChange) {
        Stop();
        Join();
    }
}

void PropTesterObject::Set(int32_t v)
{
    int32Prop = v;
    MsgArg val("i", v);
    EmitPropChanged("org.alljoyn.Testing.PropertyTester", "int32", val, id);
}

void PropTesterObject::Set(uint32_t v)
{
    uint32Prop = v;
    MsgArg val("u", v);
    EmitPropChanged("org.alljoyn.Testing.PropertyTester", "uint32", val, id);
}

void PropTesterObject::Set(const char* v)
{
    stringProp = v;
    MsgArg val("s", v);
    EmitPropChanged("org.alljoyn.Testing.PropertyTester", "string", val, id);
}

QStatus PropTesterObject::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (strcmp(ifcName, "org.alljoyn.Testing.PropertyTester") == 0) {
        lock.Lock();
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

        } else if (strcmp(propName, "session") == 0) {
            val.Set("u", id);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%u) at %s\n", propName, id, GetPath());
        }
        lock.Unlock();
    }
    return status;
}

QStatus PropTesterObject::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (strcmp(ifcName, "org.alljoyn.Testing.PropertyTester") == 0) {
        lock.Lock();
        if (strcmp(propName, "int32") == 0) {
            val.Get("i", &int32Prop);
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%d) at %s\n", propName, int32Prop, GetPath());

        } else if (strcmp(propName, "uint32") == 0) {
            val.Get("u", &uint32Prop);
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%u) at %s\n", propName, uint32Prop, GetPath());

        } else if (strcmp(propName, "string") == 0) {
            const char* s;
            val.Get("s", &s);
            stringProp = s;
            EmitPropChanged(ifcName, propName, val, id);
            status = ER_OK;
            QCC_SyncPrintf("Set property %s (%s) at %s\n", propName, stringProp.c_str(), GetPath());

        } else if (strcmp(propName, "session") == 0) {
            status = ER_OK;  // ignore
            QCC_SyncPrintf("Set property %s (%u) at %s (IGNORED) \n", propName, id, GetPath());
        }
        lock.Unlock();
    }
    return status;
}


ThreadReturn STDCALL PropTesterObject::Run(void* arg)
{
    Event dummy;
    lock.Lock();
    while (!IsStopping()) {
        lock.Unlock();
        Event::Wait(dummy, 2000);
        lock.Lock();
        ++uint32Prop;
        MsgArg val("u", uint32Prop);
        QCC_SyncPrintf("Updating uint32: %u\n", uint32Prop);
        EmitPropChanged("org.alljoyn.Testing.PropertyTester", "uint32", val, id);
    }
    lock.Unlock();

    return 0;
}


class PropTesterObject2 : public BusObject, private Thread {
  public:
    PropTesterObject2(BusAttachment& bus, const char* path, SessionId id, bool autoChange);
    ~PropTesterObject2();

    void ObjectRegistered(void) { if (autoChange) { Start(); } }

  private:

    const bool autoChange;
    uint32_t intProp;
    String stringProp;

    SessionId id;         // SessionId and RO property
    bool stop;
    Mutex lock;

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

    // Thread methods
    ThreadReturn STDCALL Run(void* arg);
};

PropTesterObject2::PropTesterObject2(BusAttachment& bus, const char* path, SessionId id, bool autoChange) :
    BusObject(path),
    autoChange(autoChange),
    intProp(0),
    stringProp("String: "),
    id(id),
    stop(false)
{
    const InterfaceDescription* ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester2");
    if (!ifc) {
        bus.CreateInterfacesFromXml(propTester2InterfaceXML);
        ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester2");
    }
    assert(ifc);

    AddInterface(*ifc);
}

PropTesterObject2::~PropTesterObject2()
{
    lock.Lock();
    stop = true;
    lock.Unlock();

    if (autoChange) {
        Stop();
        Join();
    }
}

QStatus PropTesterObject2::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (strcmp(ifcName, "org.alljoyn.Testing.PropertyTester2") == 0) {
        lock.Lock();
        if (strcmp(propName, "int1") == 0 || strcmp(propName, "int2") == 0 || strcmp(propName, "int3") == 0) {
            val.Set("i", intProp);
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%d) at %s\n", propName, intProp, GetPath());

        } else if (strcmp(propName, "string1") == 0 || strcmp(propName, "string2") == 0 || strcmp(propName, "string3") == 0) {
            val.Set("s", stringProp.c_str());
            status = ER_OK;
            QCC_SyncPrintf("Get property %s (%s) at %s\n", propName, stringProp.c_str(), GetPath());

        } else {
            std::cerr << "Trying to get unknown property on interface " << ifcName << ": " << propName << std::endl;
        }
        lock.Unlock();
    }

    return status;
}

ThreadReturn STDCALL PropTesterObject2::Run(void* arg)
{
    Event dummy;
    QStatus status;
    lock.Lock();
    while (!IsStopping()) {
        ++intProp;
        stringProp.append("X");
        QCC_SyncPrintf("PropTesterObject2::Run : (%d) %d -- %s\n", id, intProp, stringProp.c_str());
        status = EmitPropChanged("org.alljoyn.Testing.PropertyTester2", propTester2Names, propTester2Count, id, ALLJOYN_FLAG_GLOBAL_BROADCAST);
        assert(status == ER_OK);
        (void)status; // avoid warning in case of non-debug build (assert not called)
        lock.Unlock();
        Event::Wait(dummy, 2000);
        lock.Lock();
    }
    lock.Unlock();

    return 0;
}




class _PropTesterProxyObject :
    public ProxyBusObject,
    private ProxyBusObject::PropertiesChangedListener {
  public:
    _PropTesterProxyObject(BusAttachment& bus, const String& service, const String& path, SessionId sessionId);
    ~_PropTesterProxyObject();

    QStatus Set(int32_t v);
    QStatus Set(uint32_t v);
    QStatus Set(const char* v);

    QStatus Get(int32_t& v);
    QStatus Get(uint32_t& v);
    QStatus Get(const char*& v);

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
    const InterfaceDescription* ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester");
    if (!ifc) {
        bus.CreateInterfacesFromXml(propTesterInterfaceXML);
        ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester");
    }
    assert(ifc);

    AddInterface(*ifc);
    const char* watchProps[] = {
        "int32",
        "uint32",
        "string"
    };

    RegisterPropertiesChangedListener("org.alljoyn.Testing.PropertyTester", watchProps, ArraySize(watchProps), *this, NULL);
}

_PropTesterProxyObject::~_PropTesterProxyObject()
{
    UnregisterPropertiesChangedListener("org.alljoyn.Testing.PropertyTester", *this);
}


QStatus _PropTesterProxyObject::Set(int32_t v)
{
    return SetProperty("org.alljoyn.Testing.PropertyTester", "int32", v);
}

QStatus _PropTesterProxyObject::Set(uint32_t v)
{
    return SetProperty("org.alljoyn.Testing.PropertyTester", "uint32", v);
}

QStatus _PropTesterProxyObject::Set(const char* v)
{
    return SetProperty("org.alljoyn.Testing.PropertyTester", "string", v);
}

QStatus _PropTesterProxyObject::Get(int32_t& v)
{
    MsgArg val;
    QStatus status = GetProperty("org.alljoyn.Testing.PropertyTester", "int32", val);
    if (status == ER_OK) {
        status = val.Get("i", &v);
    }
    return status;
}

QStatus _PropTesterProxyObject::Get(uint32_t& v)
{
    MsgArg val;
    QStatus status = GetProperty("org.alljoyn.Testing.PropertyTester", "uint32", val);
    if (status == ER_OK) {
        status = val.Get("u", &v);
    }
    return status;
}

QStatus _PropTesterProxyObject::Get(const char*& v)
{
    MsgArg val;
    QStatus status = GetProperty("org.alljoyn.Testing.PropertyTester", "string", val);
    if (status == ER_OK) {
        status = val.Get("s", &v);
    }
    return status;
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


class _PropTesterProxyObject2 :
    public ProxyBusObject,
    private ProxyBusObject::Listener,
    private ProxyBusObject::PropertiesChangedListener {
  public:
    _PropTesterProxyObject2(BusAttachment& bus, const String& service, const String& path, SessionId sessionId);
    ~_PropTesterProxyObject2();

  private:

    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context);
    void PropCB(QStatus status, ProxyBusObject* obj, const MsgArg& value, void* context);

    struct PropCtx {
        String name;
        const MsgArg* value;
    };

};

typedef ManagedObj<_PropTesterProxyObject2> PropTesterProxyObject2;



_PropTesterProxyObject2::_PropTesterProxyObject2(BusAttachment& bus, const String& service, const String& path, SessionId sessionId) :
    ProxyBusObject(bus, service.c_str(), path.c_str(), sessionId)
{
    const InterfaceDescription* ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester2");
    if (!ifc) {
        bus.CreateInterfacesFromXml(propTester2InterfaceXML);
        ifc = bus.GetInterface("org.alljoyn.Testing.PropertyTester2");
    }
    assert(ifc);

    AddInterface(*ifc);

    QStatus status = RegisterPropertiesChangedListener("org.alljoyn.Testing.PropertyTester2",
                                                       NULL, 0, *this, NULL);
    assert(status == ER_OK);
    (void)status;
}

_PropTesterProxyObject2::~_PropTesterProxyObject2()
{
    for (int i = 0; i < propTester2Count; ++i) {
        UnregisterPropertiesChangedListener("org.alljoyn.Testing.PropertyTester2", *this);
    }
}

void _PropTesterProxyObject2::PropertiesChanged(ProxyBusObject& obj,
                                                const char* ifaceName,
                                                const MsgArg& changed,
                                                const MsgArg& invalidated,
                                                void* context)
{
    MsgArg* entries;
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

        PropCtx* ctx = new PropCtx;
        ctx->name = String(propName);
        ctx->value = new MsgArg(*propValue);
        GetPropertyAsync(ifaceName, propName, this,
                         static_cast<GetPropertyCB>(&_PropTesterProxyObject2::PropCB),
                         (void*) ctx);
    }

    invalidated.Get("as", &numEntries, &entries);
    for (i = 0; i < numEntries; ++i) {
        const char* propName;
        entries[i].Get("s", &propName);
        QCC_SyncPrintf("    Property Invalidated event: %u/%u %s\n",
                       (unsigned int)i + 1, (unsigned int)numEntries,
                       propName);

        PropCtx* ctx = new PropCtx;
        ctx->name = String(propName);
        ctx->value = NULL;
        GetPropertyAsync(ifaceName, propName, this,
                         static_cast<GetPropertyCB>(&_PropTesterProxyObject2::PropCB),
                         (void*) ctx);
    }
}


void _PropTesterProxyObject2::PropCB(QStatus status, ProxyBusObject* obj, const MsgArg& value, void* context)
{
    PropCtx* ctx = (PropCtx*) context;
    if (ctx->name.compare("int1") == 0) {
        int32_t i;
        ctx->value->Get("i", &i);
        int32_t i2;
        value.Get("i", &i2);
        QCC_SyncPrintf("Property Get Callback: %s (%d = %d)\n", ctx->name.c_str(), i, i2);
        assert(i == i2);
    } else if (ctx->name.compare("int2") == 0) {
        int32_t i;
        value.Get("i", &i);
        QCC_SyncPrintf("Property Get Callback: %s (%d)\n", ctx->name.c_str(), i);
        assert(ctx->value == NULL);
    } else if (ctx->name.compare("string1") == 0) {
        const char* s;
        ctx->value->Get("s", &s);
        const char* s2;
        value.Get("s", &s2);
        QCC_SyncPrintf("Property Get Callback: %s (%s = %s)\n", ctx->name.c_str(), s, s2);
        assert(strcmp(s, s2) == 0);
    } else if (ctx->name.compare("string2") == 0) {
        const char* s;
        value.Get("s", &s);
        QCC_SyncPrintf("Property Get Callback: %s (%s)\n", ctx->name.c_str(), s);
        assert(ctx->value == NULL);
    } else {
        QCC_SyncPrintf("Unknown property\n");
    }
    delete ctx->value;
    delete ctx;
}


class App {
  public:
    virtual ~App() { }
};


class Service : public App, private SessionPortListener, private SessionListener {
  public:
    Service(BusAttachment& bus);
    ~Service();

  private:
    BusAttachment& bus;
    multimap<SessionId, BusObject*> objects;
    SessionPort port;

    void Add(SessionId id, bool autoUpdate);

    // SessionPortListener methods
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);

    // SessionListener methdods
    void SessionLost(SessionId sessionId);
};

Service::Service(BusAttachment& bus) :
    bus(bus),
    port(PORT)
{
    Add(0, false);
    Add(0, true);
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

void Service::Add(SessionId id, bool autoUpdate)
{

    if (singleProp) {
        String path = "/org/alljoyn/Testing/PropertyTester/";
        path += U32ToString(id);
        if (autoUpdate) {
            path += "/a";
        } else {
            path += "/b";
        }
        PropTesterObject* obj = new PropTesterObject(bus, path.c_str(), id, autoUpdate);
        pair<SessionId, PropTesterObject*> item(id, obj);
        objects.insert(item);
        bus.RegisterBusObject(*obj);
    }
    if (multiProp && autoUpdate) {
        String path = "/org/alljoyn/Testing/PropertyTester/";
        path += U32ToString(id);
        path += "/c";
        PropTesterObject2* obj = new PropTesterObject2(bus, path.c_str(), id, autoUpdate);
        pair<SessionId, PropTesterObject2*> item(id, obj);
        objects.insert(item);
        bus.RegisterBusObject(*obj);
    }

}

void Service::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    bus.SetSessionListener(id, this);
    Add(id, false);
    Add(id, true);
}

void Service::SessionLost(SessionId sessionId)
{
    multimap<SessionId, BusObject*>::iterator it;
    it = objects.find(sessionId);
    while (it != objects.end()) {
        bus.UnregisterBusObject(*(it->second));
        delete it->second;
        objects.erase(it);
        it = objects.find(sessionId);
    }
}


class Client : public App, private BusListener, private BusAttachment::JoinSessionAsyncCB, private Thread {
  public:
    Client(BusAttachment& bus);
    ~Client();

  private:
    BusAttachment& bus;
    map<SessionId, PropTesterProxyObject> aObjects;  // auto updated objects
    map<SessionId, PropTesterProxyObject> bObjects;  // get/set test objects
    map<SessionId, PropTesterProxyObject2> cObjects;  // multi properties test objects
    set<String> foundNames;
    Mutex lock;
    Event newServiceFound;

    void Add(const String& name, SessionId id, bool aObj);
    void TestProps(SessionId id);

    // BusListener methods
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);

    //BusAttachment::JoinsessionAsyncCB methods
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context);

    // Thread methods
    ThreadReturn STDCALL Run(void* arg);
};

Client::Client(BusAttachment& bus) :
    bus(bus)
{
    bus.RegisterBusListener(*this);
    Start();
}

Client::~Client()
{
    while (!aObjects.empty()) {
        aObjects.erase(aObjects.begin());
    }
    while (!bObjects.empty()) {
        bObjects.erase(bObjects.begin());
    }
    while (!cObjects.empty()) {
        cObjects.erase(cObjects.begin());
    }
    Stop();
    Join();
    bus.UnregisterBusListener(*this);
}


void Client::Add(const String& name, SessionId id, bool aObj)
{

    if (singleProp) {
        String path = "/org/alljoyn/Testing/PropertyTester/";
        path += U32ToString(id);
        if (aObj) {
            path += "/a";
        } else {
            path += "/b";
        }
        PropTesterProxyObject obj(bus, name, path, id);
        pair<SessionId, PropTesterProxyObject> item(id, obj);
        if (aObj) {
            aObjects.insert(item);
        } else {
            bObjects.insert(item);
        }
    }
    if (multiProp && aObj) {
        String path = "/org/alljoyn/Testing/PropertyTester/";
        path += U32ToString(id);
        path += "/c";
        PropTesterProxyObject2 obj(bus, name, path, id);
        pair<SessionId, PropTesterProxyObject2> item(id, obj);
        cObjects.insert(item);
    }
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
        Add(*nameStr, 0, false);
        Add(*nameStr, 0, true);
        Add(*nameStr, sessionId, false);
        Add(*nameStr, sessionId, true);
        Alert(static_cast<uint32_t>(sessionId));
    }
    delete nameStr;
}


void Client::TestProps(SessionId id)
{
    lock.Lock();
    map<SessionId, PropTesterProxyObject>::iterator it = bObjects.find(id);
    if (it != bObjects.end()) {
        PropTesterProxyObject obj = it->second;
        lock.Unlock();

        uint32_t rand = Rand32();
        int32_t int32 = -12345678;
        uint32_t uint32 = 12345678;
        const char* string;

        QStatus status = obj->Get(int32);
        QCC_SyncPrintf("Got int32 value: %d   from %s - %s: status = %s: %s\n", int32, obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (status == ER_OK) ? "PASS" : "FAIL");
        status = obj->Get(uint32);
        QCC_SyncPrintf("Got uint32 value: %u   from %s - %s: status = %s: %s\n", uint32, obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (status == ER_OK) ? "PASS" : "FAIL");
        string = NULL;
        status = obj->Get(string);
        QCC_SyncPrintf("Got string value: \"%s\"   from %s - %s: status = %s: %s\n", string, obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (status != ER_OK) ? "PASS" : "FAIL");

        status = obj->Set(static_cast<int32_t>(rand));
        QCC_SyncPrintf("Set int32 value: %d   from %s - %s: status = %s: %s\n", static_cast<int32_t>(rand), obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (status == ER_OK) ? "PASS" : "FAIL");
        status = obj->Set(rand);
        QCC_SyncPrintf("Set uint32 value: %u   from %s - %s: status = %s: %s\n", rand, obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (status != ER_OK) ? "PASS" : "FAIL");
        status = obj->Set(bus.GetUniqueName().c_str());
        QCC_SyncPrintf("Set string value: \"%s\"   from %s - %s: status = %s: %s\n", bus.GetUniqueName().c_str(), obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (status == ER_OK) ? "PASS" : "FAIL");

        status = obj->Get(int32);
        QCC_SyncPrintf("Got int32 value: %d   from %s - %s: status = %s: %s\n", int32, obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (int32 == static_cast<int32_t>(rand)) ? "PASS" : "FAIL");
        status = obj->Get(uint32);
        QCC_SyncPrintf("Got uint32 value: %u   from %s - %s: status = %s: %s\n", uint32, obj->GetServiceName().c_str(), obj->GetPath().c_str(), QCC_StatusText(status), (uint32 != rand) ? "PASS" : "FAIL");


    } else {
        lock.Unlock();
    }
}


ThreadReturn STDCALL Client::Run(void* arg)
{
    Event dummy;
    lock.Lock();
    while (!IsStopping()) {
        lock.Unlock();
        Event::Wait(dummy);
        if (!IsStopping()) {
            GetStopEvent().ResetEvent();
            SessionId newId = static_cast<SessionId>(GetAlertCode());
            TestProps(0);
            TestProps(newId);
        }
        lock.Lock();
    }
    lock.Unlock();

    return 0;
}



void SignalHandler(int sig) {
    if ((sig == SIGINT) ||
        (sig == SIGTERM)) {
        quit = 1;
    }
}


void Usage()
{
    printf("proptester: [ -c ] [ -n <NAME> ] [ -s <SECONDS> ]\n"
           "    -c            Run as client (runs as service by default).\n"
           "    -n <NAME>     Use <NAME> for well known bus name.\n"
           "    -m            Use EmitPropertiesChanged only for multiple properties at once.\n"
           "    -s            Use EmitPropertiesChanged only for single property at once.\n");
}


int main(int argc, char** argv)
{
    String serviceName = "org.alljoyn.Testing.PropertyTester";
    bool client = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            client = true;
        } else if (strcmp(argv[i], "-n") == 0) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                Usage();
                exit(1);
            } else {
                serviceName = argv[i];
            }
        } else if (strcmp(argv[i], "-h") == 0) {
            Usage();
            exit(1);
        } else if (strcmp(argv[i], "-m") == 0) {
            singleProp = false;
        } else if (strcmp(argv[i], "-s") == 0) {
            multiProp = false;
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
    BusAttachment bus("ProperyTester", true);
    Environ* env = Environ::GetAppEnviron();
    String connSpec = env->Find("DBUS_STARTER_ADDRESS");

    if (connSpec.empty()) {
#ifdef _WIN32
        connSpec = env->Find("BUS_ADDRESS", "tcp:addr=127.0.0.1,port=9956");
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
        app = new Client(bus);
        status = bus.FindAdvertisedName(serviceName.c_str());
        if (status != ER_OK) {
            printf("Failed to find name to \"%s\": %s\n", serviceName.c_str(), QCC_StatusText(status));
            ret = 2;
            goto exit;
        }
    } else {
        serviceName += ".A" + bus.GetGlobalGUIDString();

        app = new Service(bus);
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

    while (!quit) {
        qcc::Sleep(100);
    }

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
