/*
 * Copyright (c) 2011-2014 AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/Session.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Util.h>
#include <qcc/Log.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <cassert>
#include <cstdio>

#include <set>
#include <map>
#include <vector>
#include <limits>

using namespace ajn;
using namespace std;
using namespace qcc;

/* constants */
static const char* TEST_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.test.sessions";
static const char* TEST_SERVICE_OBJECT_PATH = "/sessions";

/* forward declaration */
class SessionTestObject;
class MyBusListener;

/* Local Types */
struct DiscoverInfo {
    String peerName;
    TransportMask transport;
    DiscoverInfo(const char* peerName, TransportMask transport) : peerName(peerName), transport(transport) { }

    bool operator<(const DiscoverInfo& other) const
    {
        return (peerName < other.peerName) || ((peerName == other.peerName) && (transport < other.transport));
    }

    bool operator==(const DiscoverInfo& other) const
    {
        return (peerName == other.peerName) && (transport == other.transport);
    }
};

struct SessionPortInfo {
    SessionPort port;
    String sessionHost;
    SessionOpts opts;
    SessionPortInfo() : port(0) { }
    SessionPortInfo(SessionPort port, const String& sessionHost, const SessionOpts& opts) : port(port), sessionHost(sessionHost), opts(opts) { }
};

struct SessionInfo {
    SessionId id;
    SessionPortInfo portInfo;
    vector<String> peerNames;
    SessionInfo() : id(0) { }
    SessionInfo(SessionId id, const SessionPortInfo& portInfo) : id(0), portInfo(portInfo) { }
};


/* static data */
static ajn::BusAttachment* s_bus = NULL;
static MyBusListener* s_busListener = NULL;

static set<String> s_requestedNames;
static set<pair<String, TransportMask> > s_advertisements;
static set<DiscoverInfo> s_discoverSet;
static map<SessionPort, SessionPortInfo> s_sessionPortMap;
static map<SessionId, SessionInfo> s_sessionMap;
static Mutex s_lock;
static bool s_chatEcho = true;

static String s_name;
static bool s_found = false;

/*
 * get a line of input from the the file pointer (most likely stdin).
 * This will capture the the num-1 characters or till a newline character is
 * entered.
 *
 * @param[out] str a pointer to a character array that will hold the user input
 * @param[in]  num the size of the character array 'str'
 * @param[in]  fp  the file pointer the sting will be read from. (most likely stdin)
 *
 * @return returns the same string as 'str' if there has been a read error a null
 *                 pointer will be returned and 'str' will remain unchanged.
 */
char* get_line(char*str, size_t num, FILE*fp)
{
    char*p = fgets(str, num, fp);

    // fgets will capture the '\n' character if the string entered is shorter than
    // num. Remove the '\n' from the end of the line and replace it with nul '\0'.
    if (p != NULL) {
        size_t last = strlen(str) - 1;
        if (str[last] == '\n') {
            str[last] = '\0';
        }
    }
    return p;
}

/* Bus object */
class SessionTestObject : public BusObject {
  public:

    SessionTestObject(BusAttachment& bus, const char* path) : BusObject(path), chatSignalMember(NULL), ttl(0)
    {
        QStatus status;

        /* Add the session test interface to this object */
        const InterfaceDescription* testIntf = bus.GetInterface(TEST_SERVICE_INTERFACE_NAME);
        assert(testIntf);
        AddInterface(*testIntf);

        /* Store the Chat signal member away so it can be quickly looked up when signals are sent */
        chatSignalMember = testIntf->GetMember("Chat");
        assert(chatSignalMember);

        /* Register signal handler */
        status =  bus.RegisterSignalHandler(this,
                                            static_cast<MessageReceiver::SignalHandler>(&SessionTestObject::ChatSignalHandler),
                                            chatSignalMember,
                                            NULL);

        if (ER_OK != status) {
            printf("Failed to register signal handler for SessionTestObject::Chat (%s)\n", QCC_StatusText(status));
        }
    }

    /** Send a Chat signal */
    void SendChatSignal(SessionId id, const char* chat, uint8_t flags)
    {
        MsgArg chatArg("s", chat);
        Message msg(*bus);
        QStatus status = Signal(NULL, id, *chatSignalMember, &chatArg, 1, ttl, flags, &msg);
        if (status == ER_OK) {
            printf("Sent chat signal with serial = %d\n", msg->GetCallSerial());
        } else {
            printf("Failed to send chat signal (%s)\n", QCC_StatusText(status));
        }
    }

    /** Cancel a sessionless signal */
    void CancelSessionless(uint32_t serialNum)
    {
        QStatus status = CancelSessionlessMessage(serialNum);
        if (status != ER_OK) {
            printf("BusObject::CancelSessionlessMessage(0x%x) failed with %s\n", serialNum, QCC_StatusText(status));
        }
    }

    /** Receive a signal from another Chat client */
    void ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
    {
        if (s_chatEcho) {
            printf("RX chat from %s[%u]: %s\n", msg->GetSender(), msg->GetSessionId(), msg->GetArg(0)->v_string.str);
        }
    }

    /** Set ttl for all outgoing chat messages */
    void SetTtl(uint32_t ttl) {
        this->ttl = ttl;
    }

  private:
    const InterfaceDescription::Member* chatSignalMember;
    uint32_t ttl;
};

class MyBusListener : public BusListener, public SessionPortListener, public SessionListener {
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("FoundAdvertisedName name=%s namePrefix=%s\n", name, namePrefix);
        s_lock.Lock(MUTEX_CONTEXT);
        s_discoverSet.insert(DiscoverInfo(name, transport));
        s_lock.Unlock(MUTEX_CONTEXT);

        if (strcmp(name, s_name.c_str()) == 0) {
            s_found = true;
        }
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n", busName, previousOwner ? previousOwner : "<none>",
               newOwner ? newOwner : "<none>");
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        printf("LostAdvertisedName name=%s, namePrefix=%s\n", name, namePrefix);
        s_lock.Lock(MUTEX_CONTEXT);
        s_discoverSet.erase(DiscoverInfo(name, transport));
        s_lock.Unlock(MUTEX_CONTEXT);
    }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        bool ret = false;
        s_lock.Lock(MUTEX_CONTEXT);
        map<SessionPort, SessionPortInfo>::iterator it = s_sessionPortMap.find(sessionPort);
        if (it != s_sessionPortMap.end()) {
            printf("Accepting join request on %u from %s (multipoint=%d)\n", sessionPort, joiner, opts.isMultipoint);
            ret = true;
        } else {
            printf("Rejecting join attempt to unregistered port %u from %s\n", sessionPort, joiner);
        }
        s_lock.Unlock(MUTEX_CONTEXT);
        return ret;
    }

    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        s_lock.Lock(MUTEX_CONTEXT);
        map<SessionPort, SessionPortInfo>::iterator it = s_sessionPortMap.find(sessionPort);
        if (it != s_sessionPortMap.end()) {
            s_bus->SetHostedSessionListener(id, this);
            map<SessionId, SessionInfo>::iterator sit = s_sessionMap.find(id);
            if (sit == s_sessionMap.end()) {
                SessionInfo sessionInfo(id, it->second);
                s_sessionMap[id] = sessionInfo;
            }
            s_sessionMap[id].peerNames.push_back(joiner);
            s_lock.Unlock(MUTEX_CONTEXT);
            printf("SessionJoined with %s (id=%u)\n", joiner, id);
        } else {
            s_lock.Unlock(MUTEX_CONTEXT);
            printf("Leaving unexpected session %u with %s\n", id, joiner);
            s_bus->LeaveSession(id);
        }
    }

    void SessionLost(SessionId id, SessionLostReason reason)
    {
        s_lock.Lock(MUTEX_CONTEXT);
        map<SessionId, SessionInfo>::iterator it = s_sessionMap.find(id);
        if (it != s_sessionMap.end()) {
            s_sessionMap.erase(it);
            s_lock.Unlock(MUTEX_CONTEXT);
            printf("Session %u is lost. Reason=%u\n", id, reason);
        } else {
            s_lock.Unlock(MUTEX_CONTEXT);
            printf("SessionLost for unknown sessionId %u. Reason=%u\n", id, reason);
        }
    }

    void SessionMemberAdded(SessionId id, const char* uniqueName)
    {
        printf("%s was added to session %u\n", uniqueName, id);
    }

    void SessionMemberRemoved(SessionId id, const char* uniqueName)
    {
        printf("%s was removed from session %u\n", uniqueName, id);
    }
};

class AutoChatThread : public Thread, public ThreadListener {
  public:
    static AutoChatThread* Launch(SessionTestObject& busObj, SessionId id, uint32_t count, uint32_t freqMs, uint32_t minSize, uint32_t maxSize)
    {
        AutoChatThread* t = new AutoChatThread(busObj, id, count, freqMs, minSize, maxSize);
        t->Start();
        return t;
    }

    AutoChatThread(SessionTestObject& busObj, SessionId id, uint32_t count, uint32_t delay, uint32_t minSize, uint32_t maxSize)
        : busObj(busObj), id(id), count(count), delay(delay), minSize(minSize), maxSize(maxSize) { }

    void ThreadExit(Thread* thread)
    {
        delete thread;
    }

  protected:
    ThreadReturn STDCALL Run(void* args)
    {
        char* buf = new char[maxSize + 1];
        for (size_t i = 0; i <= maxSize; ++i) {
            buf[i] = 'a' + (i % 26);
        }

        while (IsRunning() && count--) {
            size_t len = minSize + static_cast<size_t>((maxSize - minSize) * (((float)Rand16()) / (std::numeric_limits<uint16_t>::max())));
            buf[len] = '\0';
            busObj.SendChatSignal(id, buf, 0);
            buf[len] = 'a' + (len % 26);
            qcc::Sleep(delay);
        }
        delete[] buf;
        return 0;
    }

  private:
    SessionTestObject& busObj;
    SessionId id;
    uint32_t count;
    uint32_t delay;
    uint32_t minSize;
    uint32_t maxSize;
};

static void usage()
{
    printf("Usage: sessions [command-file]\n");
    exit(1);
}

static String NextTok(String& inStr)
{
    String ret;
    size_t off = inStr.find_first_of(' ');
    if (off == String::npos) {
        ret = inStr;
        inStr.clear();
    } else {
        ret = inStr.substr(0, off);
        inStr = Trim(inStr.substr(off));
    }
    return Trim(ret);
}

static SessionId NextTokAsSessionId(String& inStr)
{
    uint32_t ret = 0;
    String tok = NextTok(inStr);
    if (tok[0] == '#') {
        uint32_t i = StringToU32(tok.substr(1), 0, 0);
        s_lock.Lock(MUTEX_CONTEXT);
        map<SessionId, SessionInfo>::const_iterator sit = s_sessionMap.begin();
        if (i < s_sessionMap.size()) {
            while (i--) {
                sit++;
            }
            ret = sit->first;
        }
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        ret = StringToU32(tok, 0, 0);
    }
    return static_cast<SessionId>(ret);
}


static void DoRequestName(const String& name)
{
    QStatus status = s_bus->RequestName(name.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_requestedNames.insert(name);
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("RequestName(%s) failed with %s\n", name.c_str(), QCC_StatusText(status));
    }
}

static void DoReleaseName(const String& name)
{
    QStatus status = s_bus->ReleaseName(name.c_str());
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_requestedNames.erase(name);
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("ReleaseName(%s) failed with %s\n", name.c_str(), QCC_StatusText(status));
    }
}

static void DoBind(SessionPort port, const SessionOpts& opts)
{
    if (port == 0) {
        printf("Invalid session port (%u) specified to BindSessionPort\n", port);
        return;
    } else if ((opts.traffic < SessionOpts::TRAFFIC_MESSAGES) || (opts.traffic > SessionOpts::TRAFFIC_RAW_UNRELIABLE)) {
        printf("Invalid SesionOpts.traffic (0x%x) specified to BindSessionPort\n", (unsigned int) opts.traffic);
        return;
    } else if (opts.proximity > SessionOpts::PROXIMITY_ANY) {
        printf("Invalid SessionOpts.proximity (0x%x) specified to BindSessionPort\n", (unsigned int) opts.proximity);
        return;
    } else if (opts.transports == 0) {
        printf("Invalid SessionOpts.transports (0x%x) specified to BindSessionPort\n", (unsigned int) opts.transports);
    }
    QStatus status = s_bus->BindSessionPort(port, opts, *s_busListener);
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_sessionPortMap.insert(pair<SessionPort, SessionPortInfo>(port, SessionPortInfo(port, s_bus->GetUniqueName(), opts)));
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("BusAttachment::BindSessionPort(%u, <>, <>) failed with %s\n", port, QCC_StatusText(status));
    }
}

static void DoUnbind(SessionPort port)
{
    if (port == 0) {
        printf("Invalid session port (%u) specified to BindSessionPort\n", port);
        return;
    }
    QStatus status = s_bus->UnbindSessionPort(port);
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_sessionPortMap.erase(port);
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("BusAttachment::UnbindSessionPort(%u) failed with %s\n", port, QCC_StatusText(status));
    }
}

static void DoAdvertise(String name, TransportMask transports)
{
    QStatus status = s_bus->AdvertiseName(name.c_str(), transports);
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_advertisements.insert(pair<String, TransportMask>(name, transports));
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("BusAttachment::AdvertiseName(%s, 0x%x) failed with %s\n", name.c_str(), transports, QCC_StatusText(status));
    }
}

static void DoCancelAdvertise(String name, TransportMask transports)
{
    if (transports == 0) {
        printf("Invalid transports (0x%x) specified to canceladvertise\n", transports);
        return;
    }
    QStatus status = s_bus->CancelAdvertiseName(name.c_str(), transports);
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_advertisements.erase(pair<String, TransportMask>(name, transports));
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("BusAttachment::AdvertiseName(%s, 0x%x) failed with %s\n", name.c_str(), transports, QCC_StatusText(status));
    }
}

static void DoWait(String name)
{
    while (s_found == false) {
        qcc::Sleep(250);
        printf(".");
    }
    printf("\n");
}

static void DoFind(String name)
{
    s_name = name;
    s_found = false;
    QStatus status = s_bus->FindAdvertisedName(name.c_str());
    if (status != ER_OK) {
        printf("BusAttachment::FindAdvertisedName(%s) failed with %s\n", name.c_str(), QCC_StatusText(status));
    }
}

static void DoCancelFind(String name)
{
    QStatus status = s_bus->CancelFindAdvertisedName(name.c_str());
    if (status != ER_OK) {
        printf("BusAttachment::CancelFindAdvertisedName(%s) failed with %s\n", name.c_str(), QCC_StatusText(status));
    }
}


static void DoList()
{
    printf("---------Locally Owned Names-------------------\n");
    printf("  %s\n", s_bus->GetUniqueName().c_str());
    set<String>::const_iterator nit = s_requestedNames.begin();
    while (nit != s_requestedNames.end()) {
        printf("  %s\n", nit++->c_str());
    }

    printf("---------Outgoing Advertisments----------------\n");
    s_lock.Lock(MUTEX_CONTEXT);
    set<pair<String, TransportMask> >::const_iterator ait = s_advertisements.begin();
    while (ait != s_advertisements.end()) {
        printf("  Name: %s: transport=0x%x\n", ait->first.c_str(), ait->second);
        ait++;
    }
    printf("---------Discovered Names----------------------\n");
    set<DiscoverInfo>::const_iterator dit = s_discoverSet.begin();
    while (dit != s_discoverSet.end()) {
        printf("   Peer: %s, transport=0x%x\n", dit->peerName.c_str(), dit->transport);
        ++dit;
    }
    printf("---------Bound Session Ports-------------------\n");
    map<SessionPort, SessionPortInfo>::const_iterator spit = s_sessionPortMap.begin();
    while (spit != s_sessionPortMap.end()) {
        printf("   Port: %u, isMultipoint=%s, traffic=%u, proximity=%u, transports=0x%x\n",
               spit->first,
               (spit->second.opts.isMultipoint ? "true" : "false"),
               spit->second.opts.traffic,
               spit->second.opts.proximity,
               spit->second.opts.transports);
        ++spit;
    }
    printf("---------Active sessions-----------------------\n");
    map<SessionId, SessionInfo>::const_iterator sit = s_sessionMap.begin();
    int i = 0;
    while (sit != s_sessionMap.end()) {
        printf("   #%d: SessionId: %u, Creator: %s, Port:%u, isMultipoint=%s, traffic=%u, proximity=%u, transports=0x%x\n",
               i++,
               sit->first,
               sit->second.portInfo.sessionHost.c_str(),
               sit->second.portInfo.port,
               sit->second.portInfo.opts.isMultipoint ? "true" : "false",
               sit->second.portInfo.opts.traffic,
               sit->second.portInfo.opts.proximity,
               sit->second.portInfo.opts.transports);
        if (!sit->second.peerNames.empty()) {
            printf("    Peers: ");
            for (size_t j = 0; j < sit->second.peerNames.size(); ++j) {
                printf("%s%s", sit->second.peerNames[j].c_str(), (j == sit->second.peerNames.size() - 1) ? "" : ",");
            }
            printf("\n");
        }
        ++sit;
    }
    s_lock.Unlock(MUTEX_CONTEXT);
}

class JoinCB : public BusAttachment::JoinSessionAsyncCB {
  public:
    const String name;
    const SessionPort port;

    JoinCB(String& name, SessionPort port) : name(name), port(port)
    { }

    void JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
    {
        if (status == ER_OK) {
            s_lock.Lock(MUTEX_CONTEXT);
            s_sessionMap.insert(pair<SessionId, SessionInfo>(id, SessionInfo(id, SessionPortInfo(port, name, opts))));
            s_lock.Unlock(MUTEX_CONTEXT);
            printf("JoinSessionCB(%s, %u, ...) succeeded with id = %u\n", name.c_str(), port, id);
        } else {
            printf("JoinSessionCB(%s, %u, ...) failed with %s\n", name.c_str(), port, QCC_StatusText(status));
        }

        delete this;
    }
};


static void DoJoinAsync(String name, SessionPort port, const SessionOpts& opts)
{
    JoinCB* callback = new JoinCB(name, port);
    QStatus status = s_bus->JoinSessionAsync(name.c_str(), port, s_busListener, opts, callback);

    if (status != ER_OK) {
        printf("DoJoinAsync(%s, %u) failed with %s (%u)\n", name.c_str(), port, QCC_StatusText(status), status);
    } else {
        printf("DoJoinAsync(%s, %d) OK\n", name.c_str(), port);
    }

}

static void DoJoin(String name, SessionPort port, const SessionOpts& opts)
{
    SessionId id;
    SessionOpts optsOut = opts;
    QStatus status = s_bus->JoinSession(name.c_str(), port, s_busListener, id, optsOut);
    if (status == ER_OK) {
        s_lock.Lock(MUTEX_CONTEXT);
        s_sessionMap.insert(pair<SessionId, SessionInfo>(id, SessionInfo(id, SessionPortInfo(port, name, optsOut))));
        s_lock.Unlock(MUTEX_CONTEXT);
        printf("JoinSession(%s, %u, ...) succeeded with id = %u\n", name.c_str(), port, id);
    } else {
        printf("JoinSession(%s, %u, ...) failed with %s\n", name.c_str(), port, QCC_StatusText(status));
    }
}

static void DoLeave(SessionId id)
{
    /* Validate session id */
    map<SessionId, SessionInfo>::const_iterator it = s_sessionMap.find(id);
    if (it != s_sessionMap.end()) {
        QStatus status = s_bus->LeaveSession(id);
        if (status != ER_OK) {
            printf("SessionLost(%u) failed with %s\n", id, QCC_StatusText(status));
        }
        s_lock.Lock(MUTEX_CONTEXT);
        s_sessionMap.erase(id);
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("Invalid session id %u specified in LeaveSession\n", id);
    }
}

static void DoLeaveHosted(SessionId id)
{
    /* Validate session id */
    map<SessionId, SessionInfo>::const_iterator it = s_sessionMap.find(id);
    if (it != s_sessionMap.end()) {
        QStatus status = s_bus->LeaveHostedSession(id);
        if (status != ER_OK) {
            printf("SessionLost(%u) failed with %s\n", id, QCC_StatusText(status));
        }
        s_lock.Lock(MUTEX_CONTEXT);
        s_sessionMap.erase(id);
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("Invalid session id %u specified in LeaveSession\n", id);
    }
}

static void DoLeaveJoined(SessionId id)
{
    /* Validate session id */
    map<SessionId, SessionInfo>::const_iterator it = s_sessionMap.find(id);
    if (it != s_sessionMap.end()) {
        QStatus status = s_bus->LeaveJoinedSession(id);
        if (status != ER_OK) {
            printf("SessionLost(%u) failed with %s\n", id, QCC_StatusText(status));
        }
        s_lock.Lock(MUTEX_CONTEXT);
        s_sessionMap.erase(id);
        s_lock.Unlock(MUTEX_CONTEXT);
    } else {
        printf("Invalid session id %u specified in LeaveSession\n", id);
    }
}

static void DoRemoveMember(SessionId id, String memberName)
{
    /* Validate session id */
    map<SessionId, SessionInfo>::const_iterator it = s_sessionMap.find(id);
    if (it != s_sessionMap.end()) {
        QStatus status = s_bus->RemoveSessionMember(id, memberName);
        if (status != ER_OK) {
            printf("DoRemoveMember(%u) failed with %s\n", id, QCC_StatusText(status));
        }
    } else {
        printf("Invalid session id %u specified in DoRemoveMember\n", id);
    }

}
static void DoSetLinkTimeout(SessionId id, uint32_t timeout)
{
    QStatus status = s_bus->SetLinkTimeout(id, timeout);
    if (status != ER_OK) {
        printf("SetLinkTimeout(%u, %u) failed with %s\n", id, timeout, QCC_StatusText(status));
    } else {
        printf("Link timeout for session %u is %d\n", id, timeout);
    }
}

static void DoAddMatch(const String& rule)
{
    QStatus status = s_bus->AddMatch(rule.c_str());
    if (status != ER_OK) {
        printf("AddMatch(%s) failed with %s\n", rule.c_str(), QCC_StatusText(status));
    }
}

static void DoRemoveMatch(const String& rule)
{
    QStatus status = s_bus->RemoveMatch(rule.c_str());
    if (status != ER_OK) {
        printf("RemoveMatch(%s) failed with %s\n", rule.c_str(), QCC_StatusText(status));
    }
}

struct AsyncTimeoutHandler : public BusAttachment::SetLinkTimeoutAsyncCB {

    const SessionId id;
    const uint32_t timeout;

    AsyncTimeoutHandler(SessionId id, uint32_t timeout) : id(id), timeout(timeout)
    {   }

    void SetLinkTimeoutCB(QStatus status, uint32_t timeout, void* context)
    {
        if (status != ER_OK) {
            printf("SetLinkTimeout(%u, %u) failed with %s\n", id, timeout, QCC_StatusText(status));
        } else {
            printf("Link timeout for session %u is %d\n", id, timeout);
        }

        delete this;
    }
};

static void DoSetLinkTimeoutAsync(SessionId id, uint32_t timeout)
{
    QStatus status = s_bus->SetLinkTimeoutAsync(id, timeout, new AsyncTimeoutHandler(id, timeout));
    if (status != ER_OK) {
        printf("DoSetLinkTimeoutAsync(%u, %u) failed with %s (%u)\n", id, timeout, QCC_StatusText(status), status);
    } else {
        printf("SetLinkTimeoutAsync(%u, %d) OK\n", id, timeout);
    }

}

static void DoPing(String name, uint32_t timeout)
{
    QStatus status = s_bus->Ping(name.c_str(), timeout);
    if (status != ER_OK) {
        printf("DoPing(%s) failed with %s (%u)\n", name.c_str(), QCC_StatusText(status), status);
    } else {
        printf("Ping(%s) OK\n", name.c_str());
    }
}

struct AsyncPingHandler : public BusAttachment::PingAsyncCB {

    const String name;

    AsyncPingHandler(String& name) : name(name)
    {   }

    void PingCB(QStatus status, void* context)
    {
        if (status != ER_OK) {
            printf("PingAsync(%s) failed with %s (%u)\n", name.c_str(), QCC_StatusText(status), status);
        } else {
            printf("PingAsync(%s) OK\n", name.c_str());
        }

        delete this;
    }
};

static void DoPingAsync(String name, uint32_t timeout)
{
    QStatus status = s_bus->PingAsync(name.c_str(), timeout, new AsyncPingHandler(name), NULL);
    if (status != ER_OK) {
        printf("DoPingAsync(%s) failed with %s (%u)\n", name.c_str(), QCC_StatusText(status), status);
    } else {
        printf("PingAsync(%s) OK\n", name.c_str());
    }
}

int main(int argc, char** argv)
{
    QStatus status = ER_OK;

    /* Parse command line args */
    if (argc > 2) {
        usage();
    }

    /* Create message bus */
    BusAttachment* bus = new BusAttachment("sessions", true);
    s_bus = bus;

    /* Create org.alljoyn.bus.test.sessions interface */
    InterfaceDescription* testIntf = NULL;
    status = bus->CreateInterface(TEST_SERVICE_INTERFACE_NAME, testIntf);
    if (ER_OK == status) {
        testIntf->AddSignal("Chat", "s",  "str", 0);
        testIntf->Activate();
    } else {
        printf("Failed to create interface \"%s\" (%s)\n", TEST_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
    }

    /* Create and register the bus object that will be used to send and receive signals */
    SessionTestObject sessionTestObj(*bus, TEST_SERVICE_OBJECT_PATH);
    bus->RegisterBusObject(sessionTestObj);

    /* Start the msg bus */
    if (ER_OK == status) {
        status = bus->Start();
        if (ER_OK != status) {
            printf("BusAttachment::Start failed (%s)\n", QCC_StatusText(status));
        }
    }

    /* Register a bus listener */
    if (ER_OK == status) {
        s_busListener = new MyBusListener();
        s_bus->RegisterBusListener(*s_busListener);
    }

    /* Get env vars */
    const char* connectSpec = getenv("BUS_ADDRESS");

    /* Connect to the local daemon */
    if (ER_OK == status) {
        if (connectSpec) {
            status = s_bus->Connect(connectSpec);
        } else {
            status = s_bus->Connect();
        }
        if (ER_OK != status) {
            printf("BusAttachment::Connect(%s) failed (%s)\n", s_bus->GetConnectSpec().c_str(), QCC_StatusText(status));
        }
    }

    /*
     * If argc is two, argv[1] is a file name from which we will interpret setup
     * commands until EOF.  If no file, or when the file is parsed, start
     * reading and parsing commands from stdin.
     */
    const int bufSize = 1024;
    char buf[bufSize];

    FILE* fp;

    if (argc == 2) {
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            printf("unable to open \"%s\"\n", argv[1]);
            fp = stdin;
        } else {
            printf("reading commands from \"%s\"\n", argv[1]);
        }
    } else {
        fp = stdin;
    }

    while (ER_OK == status) {
        if (get_line(buf, bufSize, fp) == NULL) {
            if (fp == stdin) {
                break;
            } else {
                fclose(fp);
                fp = stdin;
                printf("ready\n");
                continue;
            }
        }

        String line(buf);
        String cmd = NextTok(line);
        if (cmd == "debug") {
            String module = NextTok(line);
            String level = NextTok(line);
            if (module.empty() || level.empty()) {
                printf("Usage: debug <modulename> <level>\n");
            } else {
                QCC_SetDebugLevel(module.c_str(), StringToU32(level));
            }
        } else if (cmd == "requestname") {
            String name = NextTok(line);
            if (name.empty()) {
                printf("Usage: requestname <name>\n");
            } else {
                DoRequestName(name);
            }
        } else if (cmd == "releasename") {
            String name = NextTok(line);
            if (name.empty()) {
                printf("Usage: releasename <name>\n");
            } else {
                DoReleaseName(name);
            }
        } else if (cmd == "bind") {
            SessionOpts opts;
            SessionPort port = static_cast<SessionPort>(StringToU32(NextTok(line), 0, 0));
            if (port == 0) {
                printf("Usage: bind <port> [isMultipoint (false)] [traffic (TRAFFIC_MESSAGES)] [proximity (PROXIMITY_ANY)] [transports (TRANSPORT_TCP)]\n");
                printf("Example:    bind 1 true TRAFFIC_MESSAGES PROXIMITY_ANY TRANSPORT_UDP\n");
                printf("Equivalent: bind 1 true 1 255 256\n");
                continue;
            }

            String tok = NextTok(line);
            opts.isMultipoint = (tok == "true");

            tok = NextTok(line);
            if (tok == "TRAFFIC_MESSAGES") {
                opts.traffic = SessionOpts::TRAFFIC_MESSAGES;
            } else if (tok == "TRAFFIC_RAW_UNRELIABLE") {
                opts.traffic = SessionOpts::TRAFFIC_RAW_UNRELIABLE;
            } else if (tok == "TRAFFIC_RAW_RELIABLE") {
                opts.traffic = SessionOpts::TRAFFIC_RAW_RELIABLE;
            } else {
                opts.traffic = static_cast<SessionOpts::TrafficType>(StringToU32(tok, 0, 0x1));
            }

            tok = NextTok(line);
            if (tok == "PROXIMITY_ANY") {
                opts.proximity = SessionOpts::PROXIMITY_ANY;
            } else if (tok == "PROXIMITY_PHYSICAL") {
                opts.proximity = SessionOpts::PROXIMITY_PHYSICAL;
            } else if (tok == "PROXIMITY_NETWORK") {
                opts.proximity = SessionOpts::PROXIMITY_NETWORK;
            } else {
                opts.proximity = static_cast<SessionOpts::Proximity>(StringToU32(tok, 0, 0xFF));
            }

            tok = NextTok(line);
            if (tok == "TRANSPORT_TCP") {
                opts.transports = TRANSPORT_TCP;
            } else if (tok == "TRANSPORT_UDP") {
                opts.transports = TRANSPORT_UDP;
            } else {
                opts.transports = static_cast<TransportMask>(StringToU32(tok, 0, TRANSPORT_ANY));
            }

            DoBind(port, opts);
        } else if (cmd == "unbind") {
            SessionPort port = static_cast<SessionPort>(StringToU32(NextTok(line), 0, 0));
            if (port == 0) {
                printf("Usage: unbind <port>\n");
                continue;
            }
            DoUnbind(port);
        } else if (cmd == "advertise") {
            String name = NextTok(line);
            if (name.empty()) {
                printf("Usage:      advertise <name> [transports]\n");
                printf("Example:    advertise com.yadda TRANSPORT_UDP\n");
                printf("Equivalent: advertise com.yadda 256\n");
                continue;
            }

            TransportMask transports;
            String tok = NextTok(line);
            if (tok == "TRANSPORT_TCP") {
                transports = TRANSPORT_TCP;
            } else if (tok == "TRANSPORT_UDP") {
                transports = TRANSPORT_UDP;
            } else {
                transports = static_cast<TransportMask>(StringToU32(tok, 0, TRANSPORT_ANY));
            }

            DoAdvertise(name, transports);
        } else if (cmd == "canceladvertise") {
            String name = NextTok(line);
            if (name.empty()) {
                printf("Usage: canceladvertise <name> [transports]\n");
                continue;
            }
            TransportMask transports = static_cast<TransportMask>(StringToU32(NextTok(line), 0, TRANSPORT_ANY));
            DoCancelAdvertise(name, transports);
        } else if (cmd == "find") {
            String namePrefix = NextTok(line);
            if (namePrefix.empty()) {
                printf("Usage: find <name_prefix>\n");
                continue;
            }
            DoFind(namePrefix);
        } else if (cmd == "cancelfind") {
            String namePrefix = NextTok(line);
            if (namePrefix.empty()) {
                printf("Usage: cancelfind <name_prefix>\n");
                continue;
            }
            DoCancelFind(namePrefix);
        } else if (cmd == "list") {
            DoList();
        } else if (cmd == "join") {
            String name = NextTok(line);
            SessionPort port = static_cast<SessionPort>(StringToU32(NextTok(line), 0, 0));
            if (name.empty() || (port == 0)) {
                printf("Usage:      join <name> <port> [isMultipoint] [traffic] [proximity] [transports]\n");
                printf("Example:    join com.yadda 1 true TRAFFIC_MESSAGES PROXIMITY_ANY TRANSPORT_UDP\n");
                printf("Equivalent: join com.yadda 1 true 1 255 256\n");
                continue;
            }

            SessionOpts opts;
            String tok = NextTok(line);
            opts.isMultipoint = (tok == "true");

            tok = NextTok(line);
            if (tok == "TRAFFIC_MESSAGES") {
                opts.traffic = SessionOpts::TRAFFIC_MESSAGES;
            } else if (tok == "TRAFFIC_RAW_UNRELIABLE") {
                opts.traffic = SessionOpts::TRAFFIC_RAW_UNRELIABLE;
            } else if (tok == "TRAFFIC_RAW_RELIABLE") {
                opts.traffic = SessionOpts::TRAFFIC_RAW_RELIABLE;
            } else {
                opts.traffic = static_cast<SessionOpts::TrafficType>(StringToU32(tok, 0, 0x1));
            }

            tok = NextTok(line);
            if (tok == "PROXIMITY_ANY") {
                opts.proximity = SessionOpts::PROXIMITY_ANY;
            } else if (tok == "PROXIMITY_PHYSICAL") {
                opts.proximity = SessionOpts::PROXIMITY_PHYSICAL;
            } else if (tok == "PROXIMITY_NETWORK") {
                opts.proximity = SessionOpts::PROXIMITY_NETWORK;
            } else {
                opts.proximity = static_cast<SessionOpts::Proximity>(StringToU32(tok, 0, 0xFF));
            }

            tok = NextTok(line);
            if (tok == "TRANSPORT_TCP") {
                opts.transports = TRANSPORT_TCP;
            } else if (tok == "TRANSPORT_UDP") {
                opts.transports = TRANSPORT_UDP;
            } else {
                opts.transports = static_cast<TransportMask>(StringToU32(tok, 0, TRANSPORT_ANY));
            }

            DoJoin(name, port, opts);
        } else if (cmd == "asyncjoin") {
            String name = NextTok(line);
            SessionPort port = static_cast<SessionPort>(StringToU32(NextTok(line), 0, 0));
            if (name.empty() || (port == 0)) {
                printf("Usage: asyncjoin <name> <port> [isMultipoint] [traffic] [proximity] [transports]\n");
                continue;
            }
            SessionOpts opts;
            opts.isMultipoint = (NextTok(line) == "true");
            opts.traffic = static_cast<SessionOpts::TrafficType>(StringToU32(NextTok(line), 0, 0x1));
            opts.proximity = static_cast<SessionOpts::Proximity>(StringToU32(NextTok(line), 0, 0xFF));
            opts.transports = static_cast<TransportMask>(StringToU32(NextTok(line), 0, TRANSPORT_ANY));
            DoJoinAsync(name, port, opts);
        } else if (cmd == "leave") {
            SessionId id = NextTokAsSessionId(line);
            if (id == 0) {
                printf("Usage: leave <sessionId>\n");
                continue;
            }
            DoLeave(id);
        } else if (cmd == "leavehosted") {
            SessionId id = NextTokAsSessionId(line);
            if (id == 0) {
                printf("Usage: leavehosted <sessionId>\n");
                continue;
            }
            DoLeaveHosted(id);
        } else if (cmd == "leavejoiner") {
            SessionId id = NextTokAsSessionId(line);
            if (id == 0) {
                printf("Usage: leavejoiner <sessionId>\n");
                continue;
            }
            DoLeaveJoined(id);
        } else if (cmd == "removemember") {
            SessionId id = NextTokAsSessionId(line);
            String name = NextTok(line);
            if (id == 0 || name.empty()) {
                printf("Usage: removemember <sessionId> <memberName>\n");
                continue;
            }
            DoRemoveMember(id, name);
        } else if (cmd == "timeout") {
            SessionId id = NextTokAsSessionId(line);
            uint32_t timeout = StringToU32(NextTok(line), 0, 0);
            if (id == 0) {
                printf("Usage: timeout <sessionId> <timeout>\n");
                continue;
            }
            DoSetLinkTimeout(id, timeout);
        } else if (cmd == "asynctimeout") {
            SessionId id = NextTokAsSessionId(line);
            uint32_t timeout = StringToU32(NextTok(line), 0, 0);
            if (id == 0) {
                printf("Usage: asynctimeout <sessionId> <timeout>\n");
                continue;
            }
            DoSetLinkTimeoutAsync(id, timeout);
        } else if (cmd == "chat") {
            uint8_t flags = 0;
            SessionId id = NextTokAsSessionId(line);
            String chatMsg = Trim(line);
            if ((id == 0) || chatMsg.empty()) {
                printf("Usage: chat <sessionId> <msg>\n");
                continue;
            }
            sessionTestObj.SendChatSignal(id, chatMsg.c_str(), flags);
        } else if (cmd == "cchat") {
            uint8_t flags = ALLJOYN_FLAG_COMPRESSED;
            SessionId id = NextTokAsSessionId(line);
            String chatMsg = Trim(line);
            if ((id == 0) || chatMsg.empty()) {
                printf("Usage: cchat <sessionId> <msg>\n");
                continue;
            }
            sessionTestObj.SendChatSignal(id, chatMsg.c_str(), flags);
        } else if (cmd == "anychat") {
            uint8_t flags = 0;
            String chatMsg = Trim(line);
            if (chatMsg.empty()) {
                printf("Usage: anychat <msg>\n");
                continue;
            }
            sessionTestObj.SendChatSignal(ajn::SESSION_ID_ALL_HOSTED, chatMsg.c_str(), flags);
        } else if (cmd == "autochat") {
            SessionId id = NextTokAsSessionId(line);
            uint32_t count = StringToU32(NextTok(line), 0, 0);
            uint32_t delay = StringToU32(NextTok(line), 0, 100);
            uint32_t minSize = StringToU32(NextTok(line), 0, 10);
            uint32_t maxSize = StringToU32(NextTok(line), 0, 100);
            if ((id == 0) || (minSize > maxSize)) {
                printf("Usage: autochat <sessionId> [count] [delay] [minSize] [maxSize]\n");
                continue;
            }
            AutoChatThread::Launch(sessionTestObj, id, count, delay, minSize, maxSize);
        } else if (cmd == "chatecho") {
            String arg = NextTok(line);
            if (arg == "on") {
                s_chatEcho = true;
            } else if (arg == "off") {
                s_chatEcho = false;
            } else {
                printf("Usage: chatecho [on|off]\n");
            }
        } else if (cmd == "schat") {
            uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;
            String chatMsg = Trim(line);
            if (chatMsg.empty()) {
                printf("Usage: schat <msg>\n");
                continue;
            }
            sessionTestObj.SendChatSignal(0, chatMsg.c_str(), flags);
        } else if (cmd == "cancelsessionless") {
            uint32_t serial = StringToU32(NextTok(line), 0, 0);
            if (serial == 0) {
                printf("Invalid serial number\n");
                printf("Usage: cancelsessionless <serialNum>\n");
                continue;
            }
            sessionTestObj.CancelSessionless(serial);
        } else if (cmd == "addmatch") {
            String rule = Trim(line);
            if (rule.empty()) {
                printf("Usage: addmatch <rule>\n");
                continue;
            }
            DoAddMatch(rule);
        } else if (cmd == "removematch") {
            String rule = Trim(line);
            if (rule.empty()) {
                printf("Usage: removematch <rule>\n");
                continue;
            }
            DoRemoveMatch(rule);
        } else if (cmd == "sendttl") {
            uint32_t ttl = StringToU32(NextTok(line), 0, numeric_limits<uint32_t>::max());
            if (ttl == numeric_limits<uint32_t>::max()) {
                printf("Usage: sendttl <ttl>\n");
                continue;
            }
            sessionTestObj.SetTtl(ttl);
        } else if (cmd == "wait") {
            String name = NextTok(line);
            DoWait(name);
        } else if (cmd == "ping") {
            String name = NextTok(line);
            uint32_t timeout = StringToU32(NextTok(line), 0, 30000);
            DoPing(name, timeout);
        } else if (cmd == "asyncping") {
            String name = NextTok(line);
            uint32_t timeout = StringToU32(NextTok(line), 0, 30000);
            DoPingAsync(name, timeout);
        } else if (cmd == "exit") {
            break;
        } else if (cmd == "help" || cmd == "?") {
            printf("debug <module_name> <level>                                   - Set debug level for a module\n");
            printf("requestname <name>                                            - Request a well-known name\n");
            printf("releasename <name>                                            - Release a well-known name\n");
            printf("bind <port> [isMultipoint] [traffic] [proximity] [transports] - Bind a session port\n");
            printf("unbind <port>                                                 - Unbind a session port\n");
            printf("advertise <name> [transports]                                 - Advertise a name\n");
            printf("canceladvertise <name> [transports]                           - Cancel an advertisement\n");
            printf("find <name_prefix>                                            - Discover names that begin with prefix\n");
            printf("cancelfind <name_prefix>                                      - Cancel discovering names that begins with prefix\n");
            printf("list                                                          - List port bindings, discovered names and active sessions\n");
            printf("join <name> <port> [isMultipoint] [traffic] [proximity] [transports] - Join a session\n");
            printf("asyncjoin <name> <port> [isMultipoint] [traffic] [proximity] [transports] - Join a session asynchronously\n");
            printf("removemember <sessionId> <memberName>                         - Remove a session member\n");
            printf("leave <sessionId>                                             - Leave a session\n");
            printf("leavehosted <sessionId>                                       - Leave a session as host\n");
            printf("leavejoiner <sessionId>                                       - Leave a session as joiner\n");
            printf("chat <sessionId> <msg>                                        - Send a message over a given session\n");
            printf("cchat <sessionId> <msg>                                       - Send a message over a given session with compression\n");
            printf("schat <msg>                                                   - Send a sessionless message\n");
            printf("anychat <msg>                                                 - Send a message on all hosted sessions\n");
            printf("cancelsessionless <serialNum>                                 - Cancel a sessionless message\n");
            printf("autochat <sessionId> [count] [delay] [minSize] [maxSize]      - Send periodic messages of various sizes\n");
            printf("timeout <sessionId> <linkTimeout>                             - Set link timeout for a session\n");
            printf("asynctimeout <sessionId> <timeout>                            - Set link timeout for a session asynchronously\n");
            printf("chatecho [on|off]                                             - Turn on/off chat messages\n");
            printf("addmatch <rule>                                               - Add a DBUS rule\n");
            printf("removematch <rule>                                            - Remove a DBUS rule\n");
            printf("sendttl <ttl>                                                 - Set ttl (in ms) for all chat messages (0 = infinite)\n");
            printf("wait <name>                                                   - Wait until <name> is found\n");
            printf("ping <name> [timeout]                                         - Ping a name\n");
            printf("asyncping <name> [timeout]                                    - Ping a name asynchronously\n");
            printf("exit                                                          - Exit this program\n");
            printf("\n");
            printf("SessionIds can be specified by value or by #<idx> where <idx> is the session index printed with \"list\" command\n");
        } else {
            printf("Unknown command: %s\n", cmd.c_str());
        }
    }

    /* Cleanup */
    delete bus;
    bus = NULL;
    s_bus = NULL;

    if (NULL != s_busListener) {
        delete s_busListener;
        s_busListener = NULL;
    }

    return (int) status;
}

