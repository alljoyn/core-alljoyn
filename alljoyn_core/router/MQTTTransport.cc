/**
 * @file
 * MQTTTransport is an implementation of TransportBase for daemons
 * to connect to an MQTT broker.
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

#include "MQTTTransport.h"
#include "DaemonRouter.h"
#include "BusController.h"
#define QCC_MODULE "MQTT"
using namespace qcc;
namespace ajn {
/**
 * Name of transport used in transport specs.
 */
const char* MQTTTransport::TransportName = "mqtt";
/*
 * Time in seconds for PING keep-alive
 */
#define MQTT_PING_INTERVAL 60

/*
 * Placeholder for a scope GUID
 */
static String scope = "AllJoyn";

static inline String GetGuid(String name)
{
    return name.substr(0, name.find("."));
}

static inline String GetNum(String name)
{
    return (name.find(".") == String::npos) ? "" : (name.substr(name.find(".") + 1));
}

static inline bool IsThinLeaf(String name)
{
    return (GetNum(name) == ".0") ? true : false;
}

static inline String GetRouter(String name)
{
    return GetGuid(name) + ".1";
}

static inline String SlashTopic(String topic)
{
    return (topic == "") ? "" : ((topic.substr(0, 1) == ":") ? ("/" + GetGuid(topic) + "/" + GetNum(topic)) : ("/" + topic));
}

static inline String SlashTopic(int topic)
{
    return (topic == 0) ? "" : ("/" + U32ToString(topic));
}

String BuildTopic(String name, uint32_t sessionId, String iface, String member)
{
    String topic = scope
                   + SlashTopic(name)
                   + SlashTopic(sessionId)
                   + SlashTopic(iface)
                   + SlashTopic(member);
    return topic;
}

void _MQTTEndpoint::on_connect(int rc)
{
    if (rc == 0) {
        _RemoteEndpoint::IncrementRef();
        /* Only attempt to subscribe on a successful connect. */
        PublishPresence(clientId, true);
        SubscribeForDestination(clientId);

        MQTTEndpoint ep = MQTTEndpoint::wrap(this);
        DaemonRouter& router = reinterpret_cast<DaemonRouter&>(m_transport->m_bus.GetInternal().GetRouter());
        router.GetBusController()->GetSessionlessObj().RegisterMQTTEndpoint(ep);
    }

}

_MQTTEndpoint::_MQTTEndpoint(MQTTTransport* transport,
                             BusAttachment& bus, String uqn) :
    _RemoteEndpoint(bus, true, "mqtt", NULL, "mqtt", false, true),
    mosquittopp(uqn.c_str()),
    m_transport(transport),
    started(false),
    clientId(uqn)
{
    GetFeatures().isBusToBus = true;
    SetEndpointType(ENDPOINT_TYPE_MQTT);
    SetUniqueName(bus.GetInternal().GetRouter().GenerateUniqueName());
    QCC_DbgPrintf(("_MQTTEndpoint::_MQTTEndpoint(uqn=%s)", GetUniqueName().c_str()));
    GetFeatures().allowRemote = m_transport->m_bus.GetInternal().AllowRemoteMessages();
    GetFeatures().protocolVersion = 12;
    String topic = BuildTopic("presence", 0, clientId, "");
    will_set(topic.c_str(), 0, NULL, 0, true);

    connect("127.0.0.1", 1883, MQTT_PING_INTERVAL);

}

QStatus _MQTTEndpoint::Start()
{
    QStatus status =  ER_OK;
    if (!started) {
        MQTTEndpoint me = MQTTEndpoint::wrap(this);

        BusEndpoint bep = BusEndpoint::cast(me);
        status = m_transport->m_bus.GetInternal().GetRouter().RegisterEndpoint(bep);
    }
    return status;
}

_MQTTEndpoint::~_MQTTEndpoint()
{
}

QStatus _MQTTEndpoint::Stop()
{
    PublishPresence(clientId, false);
    disconnect();
    return ER_OK;
}

void _MQTTEndpoint::on_disconnect(int rc)
{
    QCC_UNUSED(rc);
    _RemoteEndpoint::Exited();

    return;
}

void _MQTTEndpoint::on_message(const struct mosquitto_message*message)
{
    String topicStr = message->topic;
    Message msg(m_transport->m_bus);

    bool isPresence;
    QCC_DbgTrace(("on message %s", topicStr.c_str()));
    mosqpp::topic_matches_sub("+/presence/+/+", message->topic, &isPresence);
    if (isPresence) {
        char** topics;
        int numTopics;
        mosqpp::sub_topic_tokenise(message->topic, &topics, &numTopics);
        String name = String(topics[2]) + "." + String(topics[3]);
        MsgArg args[3];
        args[0].Set("s", name.c_str());
        args[1].Set("s", (message->payloadlen == 0) ? name.c_str() : "");
        args[2].Set("s", (message->payloadlen != 0) ? name.c_str() : "");
        msg->SignalMsg("sss",
                       org::alljoyn::Daemon::WellKnownName,
                       0,
                       org::alljoyn::Daemon::ObjectPath,
                       org::alljoyn::Daemon::InterfaceName,
                       "NameChanged",
                       args,
                       ArraySize(args),
                       0,
                       0);
    } else if (message->payloadlen != 0) {
        size_t pos = topicStr.find("/");
        String endpointName = topicStr.substr(0, pos - 1);
        uint8_t* payload = static_cast<uint8_t*>(message->payload);
        msg->LoadBytes(payload, message->payloadlen);
        msg->Unmarshal(endpointName, false, false, true, 0);

        if (GetGuid(msg->GetSender()) != GetGuid(clientId)) {
            QCC_DbgTrace(("_MQTTEndpoint::on_message %s, dest %s msg->GetMemberName() %s topicStr %s", msg->Description().c_str(), msg->GetDestination(), msg->GetMemberName(), topicStr.c_str()));
            if ((strcmp(msg->GetMemberName(), "JoinSession") == 0) || (strcmp(msg->GetMemberName(), "MPSessionChangedWithReason") == 0)) {
                msg->ReMarshal(msg->GetSender(), clientId.c_str());
            }
        } else {
            return;
        }
        Start();
    } else {
        /*TODO: Might be a cancelled SLS if payloadlen is 0 */
        return;

    }
    msg->rcvEndpointName = GetUniqueName().c_str();
    QCC_DbgTrace(("_MQTTEndpoint::on_message %s, dest %s. Calling Router::PushMessage", msg->Description().c_str(), msg->GetDestination()));


    RemoteEndpoint rep = RemoteEndpoint::wrap(this);
    BusEndpoint bep  = BusEndpoint::cast(rep);
    m_transport->m_bus.GetInternal().GetRouter().PushMessage(msg, bep);
}

int _MQTTEndpoint::Publish(int*mid, const char*topic, int payloadlen, const void*payload, int qos, bool retain)
{
    QCC_DbgPrintf(("_MQTTEndpoint::Publish topic=%s payloadLen= %d", topic, payloadlen));
    return publish(mid, topic, payloadlen, payload, qos, retain);
}
int _MQTTEndpoint::Subscribe(int*mid, const char*sub, int qos)
{
    QCC_DbgPrintf(("_MQTTEndpoint::Subscribe topic=%s ", sub));
    return subscribe(mid, sub, qos);
}

QStatus _MQTTEndpoint::PushMessage(Message& msg)
{
    if (strcmp(msg->GetMemberName(), "NameChanged") != 0) {
        QCC_DbgTrace(("_MQTTEndpoint::PushMessage %s,%s to %s, sender %s", msg->Description().c_str(), msg->GetMemberName(), msg->GetDestination(), msg->GetSender()));
        if (msg->IsSessionless()) {
            String topic = BuildTopic(msg->GetSender(), 0, msg->GetInterface(), msg->GetMemberName());
            const uint8_t*   writePtr = reinterpret_cast<const uint8_t*>(msg->GetBuffer());
            size_t countWrite = msg->GetBufferSize();
            Publish(NULL, topic.c_str(), countWrite, writePtr, 0, true);
        } else if (msg->GetSessionId() != 0) {
            /* session cast */
            SessionId id = msg->GetSessionId();
            String topic = BuildTopic(sessionHostMap[id], id, "", "");
            const uint8_t*   writePtr = reinterpret_cast<const uint8_t*>(msg->GetBuffer());
            size_t countWrite = msg->GetBufferSize();
            Publish(NULL, topic.c_str(), countWrite, writePtr, 0, true);
        } else {
            PublishToDestination(msg->GetDestination(), msg);
        }
    } else {

        //Publish Presence
        msg->UnmarshalArgs("sss");
        size_t numArgs;
        const MsgArg* args;
        msg->GetArgs(numArgs, args);

        const qcc::String alias = args[0].v_string.str;
        const qcc::String oldOwner = args[1].v_string.str;
        const qcc::String newOwner = args[2].v_string.str;
        if (alias.substr(0, 1) == ":") {
            if (newOwner == "") {
                PublishPresence(alias, false);
            } else {
                PublishPresence(alias, true);
            }
        }

    }
    return ER_OK;
}

void _MQTTEndpoint::CancelMessage(qcc::String sender, qcc::String iface, qcc::String member)
{
    String topic = BuildTopic(sender, 0, iface, member);
    Publish(NULL, topic.c_str(), 0, NULL, 0, false);
}

void _MQTTEndpoint::SubscribeToSessionless(qcc::String iface, qcc::String member)
{
    String topic = BuildTopic("+/+", 0, iface, member);
    Subscribe(NULL, topic.c_str());
}

void _MQTTEndpoint::SubscribeToSession(qcc::String sessionHost, SessionId id)
{
    String topic = BuildTopic(sessionHost, id, "", "");
    Subscribe(NULL, topic.c_str());
    sessionHostMap[id] = sessionHost;
}

void _MQTTEndpoint::SubscribeForDestination(String name)
{
    String topic = BuildTopic("", 0, name, "");
    Subscribe(NULL, topic.c_str());
}

void _MQTTEndpoint::PublishToDestination(String name, Message& msg)
{
    String topic = BuildTopic("", 0, name, "");
    Message clone = Message(msg, true);
    const uint8_t*   writePtr = reinterpret_cast<const uint8_t*>(clone->GetBuffer());
    size_t countWrite = msg->GetBufferSize();
    Publish(NULL, topic.c_str(), countWrite, writePtr, 0, false);
}

void _MQTTEndpoint::SubscribeToPresence(String name)
{
    String topic = BuildTopic("presence", 0, name, "");
    Subscribe(NULL, topic.c_str());
    if (!IsThinLeaf(name)) {
        /* Subscribe for leaf presence */
        topic = BuildTopic("presence", 0, GetRouter(name), "");
        Subscribe(NULL, topic.c_str());

    }
}

void _MQTTEndpoint::PublishPresence(String name, bool isPresent)
{
    String topic = BuildTopic("presence", 0, name, "");
    char present[5] = "true";
    QCC_UNUSED(present);
    Publish(NULL, topic.c_str(), isPresent ? 5 : 0, isPresent ? present : NULL, 0, true);
}

QStatus MQTTTransport::Start()
{
    MQTTTransport* ptr = this;
    m_Ep = MQTTEndpoint(ptr, m_bus, m_bus.GetUniqueName());
    return Thread::Start();
}

QStatus MQTTTransport::StartListen(const char* listenSpec)
{
    QCC_UNUSED(listenSpec);
    return ER_OK;
}

void* MQTTTransport::Run(void* arg)
{
    QCC_UNUSED(arg);

    while (!IsStopping()) {
        if (m_Ep->IsValid()) {
            m_Ep->loop();
        }
    }
    return NULL;
}

MQTTTransport::~MQTTTransport()
{
    QCC_DbgTrace(("MQTTTransport::~MQTTTransport()"));
    Stop();
    Join();
}

QStatus MQTTTransport::Stop()
{
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("MQTTTransport::Stop(): Failed to Stop() server thread"));
    }

    m_Ep->DecrementRef();
    QStatus status1 = m_Ep->Stop();
    if (status1 != ER_OK) {
        QCC_LogError(status, ("MQTTTransport::Stop(): Failed to Stop() endpoint"));
    }

    status = (status != ER_OK) ? status : status1;
    return status;
}

QStatus MQTTTransport::Join()
{
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("MQTTTransport::Join(): Failed to Join() server thread"));
    }

    QStatus status1 = m_Ep->Join();
    if (status1 != ER_OK) {
        QCC_LogError(status, ("MQTTTransport::Join(): Failed to Join() endpoint"));
    }
    status = (status != ER_OK) ? status : status1;
    return status;
}

QStatus MQTTTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
    QCC_DbgPrintf(("MQTTConnect %s", connectSpec));
    String connectSpecStr = connectSpec;
    String name = connectSpecStr.substr(connectSpecStr.find('=') + 1);
    QCC_UNUSED(opts);
    m_Ep->SubscribeToPresence(name);
    m_Ep->Start();
    newep = BusEndpoint::cast(m_Ep);
    return ER_OK;
}
}
