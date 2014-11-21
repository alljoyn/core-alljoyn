/**
 * @file
 *
 * This file implements the org.alljoyn.Bus.Peer.* interfaces
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include <qcc/StringSink.h>
#include <qcc/StringSource.h>

#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>

#include "SessionInternal.h"
#include "KeyStore.h"
#include "BusEndpoint.h"
#include "PeerState.h"
#include "CompressionRules.h"
#include "AllJoynPeerObj.h"
#include "SASLEngine.h"
#include "AllJoynCrypto.h"
#include "BusInternal.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

/*
 * Version number of the key generation algorithm.
 */
static const uint32_t MIN_KEYGEN_VERSION = 0x00;
static const uint32_t MAX_KEYGEN_VERSION = 0x01;

/*
 * The base authentication version number
 */
static const uint32_t MIN_AUTH_VERSION = 0x0001;
static const uint32_t MAX_AUTH_VERSION = 0x0002;

static const uint32_t PREFERRED_AUTH_VERSION = (MAX_AUTH_VERSION << 16) | MIN_KEYGEN_VERSION;

static bool IsCompatibleVersion(uint32_t version)
{
    uint16_t authV = version >> 16;
    uint8_t keyV = version & 0xFF;

    if ((authV < MIN_AUTH_VERSION) || (authV > MAX_AUTH_VERSION)) {
        return false;
    }
    //The llvm clang compiler will complain about the code `keyV < MIN_KEYGEN_VERSION`
    // will always return false.  Which is true as long as MIN_KEYGEN_VERSION is
    // zero however we don't want to remove this check because the MIN_KEYGEN_VERSION
    // may be changed in the future. If it is changed we want this check to still
    // be in the code.
#if defined __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
    if ((keyV < MIN_KEYGEN_VERSION) || (keyV > MAX_KEYGEN_VERSION)) {
        return false;
    }
#if defined __clang__
#pragma clang diagnostic pop
#endif
    return (version & 0xFF00) == 0;
}

static uint32_t GetLowerVersion(uint32_t v1, uint32_t v2)
{
    uint16_t authV1 = v1 >> 16;
    uint8_t keyV1 = v1 & 0xFF;
    uint16_t authV2 = v2 >> 16;
    uint8_t keyV2 = v2 & 0xFF;

    if (authV1 < authV2) {
        return v1;
    }
    if (authV1 > authV2) {
        return v2;
    }
    if (keyV1 <= keyV2) {
        return v1;
    }
    return v2;
}

static bool UseKeyExchanger(uint32_t peerAuthVersion, const uint32_t*authMaskList, size_t authCount)
{
    uint16_t authV = peerAuthVersion >> 16;
    if (authV < 2) {
        return false;
    }
    for (size_t cnt = 0; cnt < authCount; cnt++) {
        uint32_t suite = authMaskList[cnt];
        if ((suite & AUTH_KEYX_ECDHE) == AUTH_KEYX_ECDHE) {
            return true;
        }
    }
    return false;
}

static void SetRights(PeerState& peerState, bool mutual, bool challenger)
{
    if (mutual) {
        QCC_DbgHLPrintf(("SetRights mutual"));
        peerState->SetAuthorization(MESSAGE_METHOD_CALL, _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX);
        peerState->SetAuthorization(MESSAGE_METHOD_RET,  _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX);
        peerState->SetAuthorization(MESSAGE_ERROR,       _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX);
        peerState->SetAuthorization(MESSAGE_SIGNAL,      _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX);
    } else {
        if (challenger) {
            QCC_DbgHLPrintf(("SetRights challenger"));
            /*
             * We are the challenger in the auth conversation. The authentication was one-side so we
             * will accept encrypted calls from the remote peer but will not send them.
             */
            peerState->SetAuthorization(MESSAGE_METHOD_CALL, _PeerState::ALLOW_SECURE_RX);
            peerState->SetAuthorization(MESSAGE_METHOD_RET,  _PeerState::ALLOW_SECURE_TX);
            peerState->SetAuthorization(MESSAGE_ERROR,       _PeerState::ALLOW_SECURE_TX);
            peerState->SetAuthorization(MESSAGE_SIGNAL,      _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX);
        } else {
            QCC_DbgHLPrintf(("SetRights responder"));
            /*
             * We initiated the authentication and responded to challenges from the remote peer. The
             * authentication was not mutual so we are not going to allow encrypted method calls
             * from the remote peer.
             */
            peerState->SetAuthorization(MESSAGE_METHOD_CALL, _PeerState::ALLOW_SECURE_TX);
            peerState->SetAuthorization(MESSAGE_METHOD_RET,  _PeerState::ALLOW_SECURE_RX);
            peerState->SetAuthorization(MESSAGE_ERROR,       _PeerState::ALLOW_SECURE_RX);
            peerState->SetAuthorization(MESSAGE_SIGNAL,      _PeerState::ALLOW_SECURE_TX | _PeerState::ALLOW_SECURE_RX);
        }
    }
}

AllJoynPeerObj::AllJoynPeerObj(BusAttachment& bus) :
    BusObject(org::alljoyn::Bus::Peer::ObjectPath, false),
    AlarmListener(),
    dispatcher("PeerObjDispatcher", true, 3), supportedAuthSuitesCount(0), supportedAuthSuites(NULL)
{
    /* Add org.alljoyn.Bus.Peer.HeaderCompression interface */
    {
        const InterfaceDescription* ifc = bus.GetInterface(org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName);
        if (ifc) {
            AddInterface(*ifc);
            AddMethodHandler(ifc->GetMember("GetExpansion"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::GetExpansion));
        }
    }
    /* Add org.alljoyn.Bus.Peer.Authentication interface */
    {
        const InterfaceDescription* ifc = bus.GetInterface(org::alljoyn::Bus::Peer::Authentication::InterfaceName);
        if (ifc) {
            AddInterface(*ifc);
            AddMethodHandler(ifc->GetMember("AuthChallenge"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::AuthChallenge));
            AddMethodHandler(ifc->GetMember("ExchangeGuids"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::ExchangeGuids));
            AddMethodHandler(ifc->GetMember("ExchangeSuites"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::ExchangeSuites));
            AddMethodHandler(ifc->GetMember("KeyExchange"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::KeyExchange));
            AddMethodHandler(ifc->GetMember("KeyAuthentication"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::KeyAuthentication));
            AddMethodHandler(ifc->GetMember("GenSessionKey"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::GenSessionKey));
            AddMethodHandler(ifc->GetMember("ExchangeGroupKeys"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::ExchangeGroupKeys));
        }
    }
    /* Add org.alljoyn.Bus.Peer.Session interface */
    {
        const InterfaceDescription* ifc = bus.GetInterface(org::alljoyn::Bus::Peer::Session::InterfaceName);
        if (ifc) {
            AddInterface(*ifc);
            AddMethodHandler(ifc->GetMember("AcceptSession"), static_cast<MessageReceiver::MethodHandler>(&AllJoynPeerObj::AcceptSession));
            bus.RegisterSignalHandler(
                this,
                static_cast<MessageReceiver::SignalHandler>(&AllJoynPeerObj::SessionJoined),
                ifc->GetMember("SessionJoined"),
                NULL);
        }
    }

}

QStatus AllJoynPeerObj::Start()
{
    assert(bus);
    bus->RegisterBusListener(*this);
    dispatcher.Start();
    return ER_OK;
}

QStatus AllJoynPeerObj::Stop()
{
    assert(bus);
    dispatcher.Stop();
    bus->UnregisterBusListener(*this);
    return ER_OK;
}

QStatus AllJoynPeerObj::Join()
{
    lock.Lock(MUTEX_CONTEXT);
    std::map<qcc::String, SASLEngine*>::iterator iter = conversations.begin();
    while (iter != conversations.end()) {
        delete iter->second;
        ++iter;
    }
    conversations.clear();
    std::map<qcc::String, KeyExchanger*>::iterator keyExIter = keyExConversations.begin();
    while (keyExIter != keyExConversations.end()) {
        delete keyExIter->second;
        ++keyExIter;
    }
    keyExConversations.clear();
    lock.Unlock(MUTEX_CONTEXT);

    dispatcher.Join();
    return ER_OK;
}

AllJoynPeerObj::~AllJoynPeerObj()
{
    if ((supportedAuthSuitesCount > 0) && supportedAuthSuites) {
        delete [] supportedAuthSuites;
    }
}

QStatus AllJoynPeerObj::Init(BusAttachment& bus)
{
    QStatus status = bus.RegisterBusObject(*this);
    return status;
}

void AllJoynPeerObj::ObjectRegistered(void)
{
    /* Must call base class */
    BusObject::ObjectRegistered();

}

void AllJoynPeerObj::GetExpansion(const InterfaceDescription::Member* member, Message& msg)
{
    uint32_t token = msg->GetArg(0)->v_uint32;
    MsgArg replyArg;
    QStatus status = msg->GetExpansion(token, replyArg);
    if (status == ER_OK) {
        status = MethodReply(msg, &replyArg, 1);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to send GetExpansion reply"));
        }
    } else {
        MethodReply(msg, status);
    }
}

QStatus AllJoynPeerObj::RequestHeaderExpansion(Message& msg, RemoteEndpoint& sender)
{
    bool expansionPending = false;
    uint32_t token = msg->GetCompressionToken();

    assert(bus);
    //assert(sender == bus.GetInternal().GetRouter().FindEndpoint(msg->GetRcvEndpointName()));

    lock.Lock(MUTEX_CONTEXT);
    /*
     * First check if there are any other messages waiting for the same expansion rule.
     */
    for (std::deque<Message>::iterator iter = msgsPendingExpansion.begin(); iter != msgsPendingExpansion.end(); ++iter) {
        if ((*iter)->GetCompressionToken() == token) {
            expansionPending = true;
            break;
        }
    }
    msgsPendingExpansion.push_back(msg);
    lock.Unlock(MUTEX_CONTEXT);
    /*
     * If there is already an expansion request for this message we don't need another one.
     */
    if (expansionPending) {
        return ER_OK;
    } else {
        return DispatchRequest(msg, EXPAND_HEADER, sender->GetRemoteName());
    }
}

QStatus AllJoynPeerObj::RequestAuthentication(Message& msg)
{
    return DispatchRequest(msg, AUTHENTICATE_PEER);
}

bool AllJoynPeerObj::RemoveCompressedMessage(Message& msg, uint32_t token)
{
    lock.Lock(MUTEX_CONTEXT);
    for (std::deque<Message>::iterator iter = msgsPendingExpansion.begin(); iter != msgsPendingExpansion.end(); ++iter) {
        if ((*iter)->GetCompressionToken() == token) {
            msg = *iter;
            msgsPendingExpansion.erase(iter);
            lock.Unlock(MUTEX_CONTEXT);
            return true;
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
    return false;
}

/**
 * We keep the timeout for the expansion request small to bound the number of unexpanded messages
 * that we have to queue while we wait for the response. This neutralizes a DOS attack where a remote device
 * that is sending compressed messages never responds to the request for the expansion rule.
 */
#define EXPANSION_TIMEOUT   1000

void AllJoynPeerObj::ExpandHeader(Message& msg, const qcc::String& receivedFrom)
{
    assert(bus);
    QStatus status = ER_OK;
    uint32_t token = msg->GetCompressionToken();

    const HeaderFields* expFields = bus->GetInternal().GetCompressionRules()->GetExpansion(token);
    if (!expFields) {
        Message replyMsg(*bus);
        MsgArg arg("u", token);
        /*
         * The endpoint the message was received on knows the expansion rule for the token we just received.
         */
        ProxyBusObject remotePeerObj(*bus, receivedFrom.c_str(), org::alljoyn::Bus::Peer::ObjectPath, 0);
        const InterfaceDescription* ifc = bus->GetInterface(org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName);
        if (ifc == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        }
        if (status == ER_OK) {
            remotePeerObj.AddInterface(*ifc);
            const InterfaceDescription::Member* getExpansionMember = ifc->GetMember("GetExpansion");
            assert(getExpansionMember);
            status = remotePeerObj.MethodCall(*getExpansionMember, &arg, 1, replyMsg, EXPANSION_TIMEOUT);
        }
        if (status == ER_OK) {
            status = replyMsg->AddExpansionRule(token, replyMsg->GetArg(0));
            if (status == ER_OK) {
                expFields = bus->GetInternal().GetCompressionRules()->GetExpansion(token);
                if (!expFields) {
                    status = ER_BUS_HDR_EXPANSION_INVALID;
                }
            }
        }
    }
    /*
     * Clean up if we can't expand the messages.
     */
    if (status != ER_OK) {
        while (RemoveCompressedMessage(msg, token)) {
            QCC_LogError(status, ("Failed to expand message %s", msg->Description().c_str()));
        }
        return;
    }
    /*
     * Calling RemoveCompressedMessage() in a loop may look innefficient but it is highly unlikely
     * we will be expanding different headers at the same time so we are really just removing the
     * front message from the list.
     */
    while (RemoveCompressedMessage(msg, token)) {
        Router& router = bus->GetInternal().GetRouter();
        BusEndpoint sender = router.FindEndpoint(msg->GetRcvEndpointName());
        if (sender->IsValid()) {
            /*
             * Expand the compressed fields. Don't overwrite headers we received.
             */
            for (size_t id = 0; id < ArraySize(msg->hdrFields.field); id++) {
                if (HeaderFields::Compressible[id] && (msg->hdrFields.field[id].typeId == ALLJOYN_INVALID)) {
                    msg->hdrFields.field[id] = expFields->field[id];
                }
            }
            /*
             * Initialize ttl from the message header.
             */
            if (msg->hdrFields.field[ALLJOYN_HDR_FIELD_TIME_TO_LIVE].typeId != ALLJOYN_INVALID) {
                msg->ttl = msg->hdrFields.field[ALLJOYN_HDR_FIELD_TIME_TO_LIVE].v_uint16;
            } else {
                msg->ttl = 0;
            }
            msg->hdrFields.field[ALLJOYN_HDR_FIELD_COMPRESSION_TOKEN].Clear();
            /*
             * we have succesfully expanded the message so now it can be routed.
             */
            router.PushMessage(msg, sender);
        }
    }
}

/*
 * Get a property
 */
QStatus AllJoynPeerObj::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;

    if (strcmp(ifcName, org::alljoyn::Bus::Peer::Authentication::InterfaceName) == 0) {
        if (strcmp("Mechanisms", propName) == 0) {
            val.typeId = ALLJOYN_STRING;
            val.v_string.str = peerAuthMechanisms.c_str();
            val.v_string.len = peerAuthMechanisms.size();
            status = ER_OK;
        }
    }
    return status;
}

void AllJoynPeerObj::ExchangeGroupKeys(const InterfaceDescription::Member* member, Message& msg)
{
    assert(bus);

    QStatus status;
    PeerStateTable* peerStateTable = bus->GetInternal().GetPeerStateTable();

    /*
     * We expect to know the peer that is making this method call
     */
    if (peerStateTable->IsKnownPeer(msg->GetSender())) {
        PeerState peerState = peerStateTable->GetPeerState(msg->GetSender());
        uint8_t keyGenVersion = peerState->GetAuthVersion() & 0xFF;
        uint16_t authV = peerState->GetAuthVersion() >> 16;
        uint8_t sendKeyBlob = (authV <= 1) && (keyGenVersion == 0);
        QCC_DbgHLPrintf(("ExchangeGroupKeys using key gen version %d", keyGenVersion));
        /*
         * KeyGen version 0 exchanges key blobs, version 1 just exchanges the key
         */
        KeyBlob key;
        if (sendKeyBlob) {
            StringSource src(msg->GetArg(0)->v_scalarArray.v_byte, msg->GetArg(0)->v_scalarArray.numElements);
            status = key.Load(src);
        } else {
            status = key.Set(msg->GetArg(0)->v_scalarArray.v_byte, msg->GetArg(0)->v_scalarArray.numElements, KeyBlob::AES);
        }
        if (status == ER_OK) {
            /*
             * Tag the group key with the auth mechanism used by ExchangeGroupKeys. Group keys
             * are inherently directional - only initiator encrypts with the group key. We set
             * the role to NO_ROLE otherwise senders can't decrypt their own broadcast messages.
             */
            key.SetTag(msg->GetAuthMechanism(), KeyBlob::NO_ROLE);
            peerState->SetKey(key, PEER_GROUP_KEY);
            /*
             * Return the local group key.
             */
            peerStateTable->GetGroupKey(key);
            StringSink snk;
            MsgArg replyArg;
            if (sendKeyBlob) {
                key.Store(snk);
                replyArg.Set("ay", snk.GetString().size(), snk.GetString().data());
            } else {
                replyArg.Set("ay", key.GetSize(), key.GetData());
            }
            MethodReply(msg, &replyArg, 1);
        }
    } else {
        status = ER_BUS_NO_PEER_GUID;
    }
    if (status != ER_OK) {
        MethodReply(msg, status);
    }
}

void AllJoynPeerObj::ExchangeGuids(const InterfaceDescription::Member* member, Message& msg)
{
    assert(bus);
    qcc::GUID128 remotePeerGuid(msg->GetArg(0)->v_string.str);
    uint32_t authVersion = msg->GetArg(1)->v_uint32;
    qcc::String localGuidStr = bus->GetInternal().GetKeyStore().GetGuid();
    if (!localGuidStr.empty()) {
        PeerState peerState = bus->GetInternal().GetPeerStateTable()->GetPeerState(msg->GetSender());
        /*
         * If don't support the proposed version reply with our preferred version
         */
        if (!IsCompatibleVersion(authVersion)) {
            authVersion = PREFERRED_AUTH_VERSION;
        } else {
            authVersion = GetLowerVersion(authVersion, PREFERRED_AUTH_VERSION);
        }
        QCC_DbgHLPrintf(("ExchangeGuids Local %s", localGuidStr.c_str()));
        QCC_DbgHLPrintf(("ExchangeGuids Remote %s", remotePeerGuid.ToString().c_str()));
        QCC_DbgHLPrintf(("ExchangeGuids AuthVersion %d", authVersion));
        /*
         * If we proposed a different version we simply assume it is acceptable. The remote peer
         * will try a different version or give up if it doesn't like our suggestion.
         */
        peerState->SetGuidAndAuthVersion(remotePeerGuid, authVersion);
        /*
         * Associate the remote peer GUID with the sender peer state.
         */
        MsgArg replyArgs[2];
        replyArgs[0].Set("s", localGuidStr.c_str());
        replyArgs[1].Set("u", authVersion);
        MethodReply(msg, replyArgs, ArraySize(replyArgs));
    } else {
        MethodReply(msg, ER_BUS_NO_PEER_GUID);
    }
}

/*
 * These two lengths are used in RFC 5246
 */
#define VERIFIER_LEN  12
#define NONCE_LEN     28

/*
 * Limit session key lifetime to 2 days.
 */
#define SESSION_KEY_EXPIRATION (60 * 60 * 24 * 2)

QStatus AllJoynPeerObj::KeyGen(PeerState& peerState, String seed, qcc::String& verifier, KeyBlob::Role role)
{
    assert(bus);
    QStatus status;
    KeyStore& keyStore = bus->GetInternal().GetKeyStore();
    KeyBlob masterSecret;
    uint8_t keyGenVersion = peerState->GetAuthVersion() & 0xFF;

    status = keyStore.GetKey(peerState->GetGuid(), masterSecret, peerState->authorizations);
    if ((status == ER_OK) && masterSecret.HasExpired()) {
        status = ER_BUS_KEY_EXPIRED;
    }
    if (status == ER_OK) {
        size_t keylen = Crypto_AES::AES128_SIZE + VERIFIER_LEN;
        uint8_t* keymatter = new uint8_t[keylen];

        QCC_DbgHLPrintf(("KeyGen using key gen version %d", keyGenVersion));
        if (keyGenVersion == 0) {
            /*
             * Session key is generated using the procedure described in RFC 5246
             */
            status = Crypto_PseudorandomFunction(masterSecret, "session key", seed, keymatter, keylen);
        } else {
            assert(keyGenVersion == 1);
            /*
             * Session key is generated using AES-CCM key gen procedure
             */
            status = Crypto_PseudorandomFunctionCCM(masterSecret, "session key", seed, keymatter, keylen);
        }
        if (status == ER_OK) {
            KeyBlob sessionKey(keymatter, Crypto_AES::AES128_SIZE, KeyBlob::AES);
            /*
             * Tag the session key with auth mechanism tag from the master secret
             */
            sessionKey.SetTag(masterSecret.GetTag(), role);
            sessionKey.SetExpiration(SESSION_KEY_EXPIRATION);
            /*
             * Store session key in the peer state.
             */
            peerState->SetKey(sessionKey, PEER_SESSION_KEY);
            /*
             * Return verifier string
             */
            verifier = BytesToHexString(keymatter + Crypto_AES::AES128_SIZE, VERIFIER_LEN);
        }
        delete [] keymatter;
    }
    /*
     * Store any changes to the key store.
     */
    keyStore.Store();
    return status;
}

void AllJoynPeerObj::GenSessionKey(const InterfaceDescription::Member* member, Message& msg)
{
    assert(bus);
    QStatus status;
    PeerState peerState = bus->GetInternal().GetPeerStateTable()->GetPeerState(msg->GetSender());
    qcc::GUID128 remotePeerGuid(msg->GetArg(0)->v_string.str);
    qcc::GUID128 localPeerGuid(msg->GetArg(1)->v_string.str);
    /*
     * Check that target GUID is our GUID.
     */
    if (bus->GetInternal().GetKeyStore().GetGuid() != localPeerGuid.ToString()) {
        MethodReply(msg, ER_BUS_NO_PEER_GUID);
    } else {
        qcc::String nonce = RandHexString(NONCE_LEN);
        qcc::String verifier;
        status = KeyGen(peerState, msg->GetArg(2)->v_string.str + nonce, verifier, KeyBlob::RESPONDER);
        if (status == ER_OK) {
            MsgArg replyArgs[2];
            replyArgs[0].Set("s", nonce.c_str());
            replyArgs[1].Set("s", verifier.c_str());
            MethodReply(msg, replyArgs, ArraySize(replyArgs));
        } else {
            MethodReply(msg, status);
        }
    }
}

void AllJoynPeerObj::AuthAdvance(Message& msg)
{
    assert(bus);
    QStatus status = ER_OK;
    ajn::SASLEngine* sasl = NULL;
    ajn::SASLEngine::AuthState authState;
    qcc::String outStr;
    qcc::String sender = msg->GetSender();
    qcc::String mech;

    /*
     * There can be multiple authentication conversations going on simultaneously between the
     * current peer and other remote peers but only one conversation between each pair.
     *
     * Check for existing conversation and allocate a new SASL engine if we need one
     */
    lock.Lock(MUTEX_CONTEXT);
    sasl = conversations[sender];
    conversations.erase(sender);
    lock.Unlock(MUTEX_CONTEXT);

    if (!sasl) {
        sasl = new SASLEngine(*bus, ajn::AuthMechanism::CHALLENGER, peerAuthMechanisms.c_str(), sender.c_str(), peerAuthListener);
        qcc::String localGuidStr = bus->GetInternal().GetKeyStore().GetGuid();
        if (!localGuidStr.empty()) {
            sasl->SetLocalId(localGuidStr);
        } else {
            status = ER_BUS_NO_PEER_GUID;
        }
    }
    /*
     * Move the authentication conversation forward.
     */
    if (status == ER_OK) {
        status = sasl->Advance(msg->GetArg(0)->v_string.str, outStr, authState);
    }
    /*
     * If auth conversation was sucessful store the master secret in the key store.
     */
    if ((status == ER_OK) && (authState == SASLEngine::ALLJOYN_AUTH_SUCCESS)) {
        PeerState peerState = bus->GetInternal().GetPeerStateTable()->GetPeerState(sender);
        SetRights(peerState, sasl->AuthenticationIsMutual(), true /*challenger*/);
        KeyBlob masterSecret;
        KeyStore& keyStore = bus->GetInternal().GetKeyStore();
        status = sasl->GetMasterSecret(masterSecret);
        mech = sasl->GetMechanism();
        if (status == ER_OK) {
            qcc::GUID128 remotePeerGuid(sasl->GetRemoteId());
            /* Tag the master secret with the auth mechanism used to generate it */
            masterSecret.SetTag(mech, KeyBlob::RESPONDER);
            status = keyStore.AddKey(remotePeerGuid, masterSecret, peerState->authorizations);
        }
        /*
         * Report the succesful authentication to allow application to clear UI etc.
         */
        if (status == ER_OK) {
            peerAuthListener.AuthenticationComplete(mech.c_str(), sender.c_str(), true /* success */);
        }
        delete sasl;
        sasl = NULL;
    }

    if (status != ER_OK) {
        /*
         * Report the failed authentication to allow application to clear UI etc.
         */
        peerAuthListener.AuthenticationComplete(mech.c_str(), sender.c_str(), false /* failure */);
        /*
         * Let remote peer know the authentication failed.
         */
        MethodReply(msg, status);
        delete sasl;
    } else {
        /*
         * If we are not done put the SASL engine back
         */
        if (authState != SASLEngine::ALLJOYN_AUTH_SUCCESS) {
            lock.Lock(MUTEX_CONTEXT);
            conversations[sender] = sasl;
            lock.Unlock(MUTEX_CONTEXT);
        }
        MsgArg replyMsg("s", outStr.c_str());
        MethodReply(msg, &replyMsg, 1);
    }
}

void AllJoynPeerObj::DoKeyExchange(Message& msg)
{
    QStatus status = ER_OK;
    qcc::String sender = msg->GetSender();

    uint32_t authMask = msg->GetArg(0)->v_uint32;
    MsgArg* inVariant;
    status = msg->GetArg(1)->Get("v", &inVariant);

    uint32_t effectiveAuthMask = 0;
    lock.Lock(MUTEX_CONTEXT);
    for (int cnt = 0; cnt < supportedAuthSuitesCount; cnt++) {
        if ((authMask & supportedAuthSuites[cnt]) == authMask) {
            effectiveAuthMask = authMask;
            break;
        }
    }

    if (!effectiveAuthMask) {
        lock.Unlock(MUTEX_CONTEXT);
        status = ER_AUTH_FAIL;
        MethodReply(msg, status);
        return;
    }

    uint32_t authMaskList[1];
    authMaskList[0] = effectiveAuthMask;
    KeyExchanger*keyExchanger = GetKeyExchangerInstance(false, authMaskList, 1);
    if (!keyExchanger) {
        lock.Unlock(MUTEX_CONTEXT);
        status = ER_AUTH_FAIL;
        MethodReply(msg, status);
        return;
    }

    /* storing some handle for the given sender  */
    keyExConversations[sender] = keyExchanger;
    lock.Unlock(MUTEX_CONTEXT);

    keyExchanger->RespondToKeyExchange(msg, inVariant, authMask, effectiveAuthMask);
} /* DoKeyExchange */

QStatus AllJoynPeerObj::RecordMasterSecret(const qcc::String& sender, KeyExchanger*keyExchanger, PeerState peerState)
{

    qcc::String guidStr;
    bus->GetPeerGUID(sender.c_str(), guidStr);
    qcc::GUID128 remotePeerGuid(guidStr);
    return keyExchanger->StoreMasterSecret(remotePeerGuid, peerState->authorizations);
}

void AllJoynPeerObj::DoKeyAuthentication(Message& msg)
{
    assert(bus);
    QStatus status = ER_OK;
    qcc::String sender = msg->GetSender();
    PeerStateTable* peerStateTable = bus->GetInternal().GetPeerStateTable();
    PeerState peerState;
    if (peerStateTable->IsKnownPeer(sender)) {
        peerState = peerStateTable->GetPeerState(sender);
    } else {
        return;
    }

    /*
     * There can be multiple authentication conversations going on simultaneously between the
     * current peer and other remote peers but only one conversation between each pair.
     *
     * Check for existing conversation and allocate a new SASL engine if we need one
     */
    lock.Lock(MUTEX_CONTEXT);
    KeyExchanger*keyExchanger = keyExConversations[sender];
    keyExConversations.erase(sender);
    lock.Unlock(MUTEX_CONTEXT);

    if (!keyExchanger) {
        status = ER_AUTH_FAIL;
    }
    if (status == ER_OK) {
        bool authorized = false;
        MsgArg* variant;
        status = msg->GetArg(0)->Get("v", &variant);
        if (status == ER_OK) {
            status = keyExchanger->ValidateRemoteVerifierVariant(sender.c_str(), variant, (uint8_t*) &authorized);

            if ((status == ER_OK) && authorized) {
                SetRights(peerState, true, true /*challenger*/);
                status = RecordMasterSecret(sender, keyExchanger, peerState);
                /*
                 * Report the succesful authentication to allow application to clear UI etc.
                 */
                if (status == ER_OK) {
                    peerAuthListener.AuthenticationComplete(keyExchanger->GetSuiteName(), sender.c_str(), true /* success */);
                    /* compute the local verifier to send back */
                    keyExchanger->ReplyWithVerifier(msg);
                    delete keyExchanger;
                    return;
                }
            }
        }
    }

    /* assume failure */
    status = ER_AUTH_FAIL;
    /*
     * Report the failed authentication to allow application to clear UI etc.
     */
    const char* suiteName;
    if (keyExchanger) {
        suiteName = keyExchanger->GetSuiteName();
        delete keyExchanger;
    } else {
        suiteName = "Unknown";
    }
    peerAuthListener.AuthenticationComplete(suiteName, sender.c_str(), false /* failure */);

    /*
     * Let remote peer know the authentication failed.
     */
    MethodReply(msg, status);
}

void AllJoynPeerObj::AuthChallenge(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    /*
     * Cannot authenticate if we don't have any authentication mechanisms
     */
    if (peerAuthMechanisms.empty()) {
        MethodReply(msg, ER_BUS_NO_AUTHENTICATION_MECHANISM);
        return;
    }
    /*
     * Authentication may involve user interaction or be computationally expensive so cannot be
     * allowed to block the read thread.
     */
    QStatus status = DispatchRequest(msg, AUTH_CHALLENGE);
    if (status != ER_OK) {
        MethodReply(msg, status);
    }
}

void AllJoynPeerObj::ExchangeSuites(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    uint32_t*remoteSuites;
    size_t remoteSuitesLen;

    QStatus status = msg->GetArg(0)->Get("au", &remoteSuitesLen, &remoteSuites);
    if (status != ER_OK) {
        MethodReply(msg, status);
        return;
    }
    size_t effectiveAuthSuitesCount = 0;
    if (supportedAuthSuitesCount == 0) {
        effectiveAuthSuitesCount = 1;
    } else {
        effectiveAuthSuitesCount = supportedAuthSuitesCount;
    }
    uint32_t*effectiveAuthSuites = new uint32_t[supportedAuthSuitesCount];

    if (supportedAuthSuitesCount == 0) {
        effectiveAuthSuites[0] = 0;
    } else {
        for (size_t cnt = 0; cnt < effectiveAuthSuitesCount; cnt++) {
            effectiveAuthSuites[cnt] = 0;
        }
        int netCnt = 0;
        /* the order of precedence is from the server perspective */
        for (size_t cnt = 0; cnt < supportedAuthSuitesCount; cnt++) {
            for (size_t idx = 0; idx < remoteSuitesLen; idx++) {
                if ((supportedAuthSuites[cnt] & remoteSuites[idx]) == supportedAuthSuites[cnt]) {
                    /* add it */
                    effectiveAuthSuites[netCnt++] = supportedAuthSuites[cnt];
                    break;
                }
            }
        }
        effectiveAuthSuitesCount = netCnt;
    }

    MsgArg replyArg;
    replyArg.Set("au", effectiveAuthSuitesCount, effectiveAuthSuites);
    MethodReply(msg, &replyArg, 1);
    delete [] effectiveAuthSuites;
}

void AllJoynPeerObj::KeyExchange(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    /*
     * Cannot authenticate if we don't have any authentication mechanisms
     */
    if (peerAuthMechanisms.empty()) {
        MethodReply(msg, ER_BUS_NO_AUTHENTICATION_MECHANISM);
        return;
    }
    /*
     * Authentication may involve user interaction or be computationally expensive so cannot be
     * allowed to block the read thread.
     */
    QStatus status = DispatchRequest(msg, KEY_EXCHANGE);
    if (status != ER_OK) {
        MethodReply(msg, status);
    }
}

void AllJoynPeerObj::KeyAuthentication(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    /*
     * Cannot authenticate if we don't have any authentication mechanisms
     */
    if (peerAuthMechanisms.empty()) {
        MethodReply(msg, ER_BUS_NO_AUTHENTICATION_MECHANISM);
        return;
    }
    /*
     * Authentication may involve user interaction or be computationally expensive so cannot be
     * allowed to block the read thread.
     */
    QStatus status = DispatchRequest(msg, KEY_AUTHENTICATION);
    if (status != ER_OK) {
        MethodReply(msg, status);
    }
}

void AllJoynPeerObj::ForceAuthentication(const qcc::String& busName)
{
    assert(bus);
    PeerStateTable* peerStateTable = bus->GetInternal().GetPeerStateTable();
    if (peerStateTable->IsKnownPeer(busName)) {
        lock.Lock(MUTEX_CONTEXT);
        PeerState peerState = peerStateTable->GetPeerState(busName);
        peerState->ClearKeys();
        bus->ClearKeys(peerState->GetGuid().ToString());
        lock.Unlock(MUTEX_CONTEXT);
    }
}

/*
 * A long timeout to allow for possible PIN entry
 */
#define AUTH_TIMEOUT      120000
#define DEFAULT_TIMEOUT   10000

QStatus AllJoynPeerObj::AuthenticatePeer(AllJoynMessageType msgType, const qcc::String& busName, bool wait)
{
    assert(bus);
    QStatus status;
    PeerStateTable* peerStateTable = bus->GetInternal().GetPeerStateTable();
    PeerState peerState = peerStateTable->GetPeerState(busName);
    qcc::String mech;
    const InterfaceDescription* ifc = bus->GetInterface(org::alljoyn::Bus::Peer::Authentication::InterfaceName);
    if (ifc == NULL) {
        return ER_BUS_NO_SUCH_INTERFACE;
    }
    /*
     * Cannot authenticate if we don't have an authentication mechanism
     */
    if (peerAuthMechanisms.empty()) {
        return ER_BUS_NO_AUTHENTICATION_MECHANISM;
    }
    /*
     * Return if the peer is already secured.
     */
    if (peerState->IsSecure()) {
        return ER_OK;
    }
    /*
     * Check if this peer is already being authenticated. This check won't catch authentications
     * that use different names for the same peer, but we catch those below when we using the
     * unique name. Worst case we end up making a redundant ExchangeGuids method call.
     */
    if (msgType == MESSAGE_METHOD_CALL) {
        lock.Lock(MUTEX_CONTEXT);
        if (peerState->GetAuthEvent()) {
            if (wait) {
                Event::Wait(*peerState->GetAuthEvent(), lock);
                return peerState->IsSecure() ? ER_OK : ER_AUTH_FAIL;
            } else {
                lock.Unlock(MUTEX_CONTEXT);
                return ER_WOULDBLOCK;
            }
        }
        lock.Unlock(MUTEX_CONTEXT);
    }

    ProxyBusObject remotePeerObj(*bus, busName.c_str(), org::alljoyn::Bus::Peer::ObjectPath, 0);
    remotePeerObj.AddInterface(*ifc);

    /*
     * Exchange GUIDs with the peer, this will get us the GUID of the remote peer and also the
     * unique bus name from which we can determine if we have already have a session key, a
     * master secret or if we have to start an authentication conversation.
     */
    qcc::String localGuidStr = bus->GetInternal().GetKeyStore().GetGuid();
    MsgArg args[2];
    args[0].Set("s", localGuidStr.c_str());
    args[1].Set("u", PREFERRED_AUTH_VERSION);
    Message replyMsg(*bus);
    const InterfaceDescription::Member* exchangeGuidsMember = ifc->GetMember("ExchangeGuids");
    assert(exchangeGuidsMember);
    status = remotePeerObj.MethodCall(*exchangeGuidsMember, args, ArraySize(args), replyMsg, DEFAULT_TIMEOUT);
    if (status != ER_OK) {
        /*
         * ER_BUS_REPLY_IS_ERROR_MESSAGE has a specific meaning in the public API and should not be
         * propogated to the caller from this context.
         */
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            if (replyMsg->GetErrorName() != NULL && strcmp(replyMsg->GetErrorName(), "org.freedesktop.DBus.Error.ServiceUnknown") == 0) {
                status = ER_BUS_NO_SUCH_OBJECT;
            } else {
                status = ER_AUTH_FAIL;
            }
        }
        QCC_LogError(status, ("ExchangeGuids failed"));
        return status;
    }
    const qcc::String sender = replyMsg->GetSender();
    /*
     * Extract the remote guid from the message
     */
    qcc::GUID128 remotePeerGuid(replyMsg->GetArg(0)->v_string.str);
    uint32_t authVersion = replyMsg->GetArg(1)->v_uint32;
    qcc::String remoteGuidStr = remotePeerGuid.ToString();
    /*
     * Check that we can support the version the remote peer proposed.
     */
    if (!IsCompatibleVersion(authVersion)) {
        status = ER_BUS_PEER_AUTH_VERSION_MISMATCH;
        QCC_LogError(status, ("ExchangeGuids incompatible authentication version %u", authVersion));
        return status;
    } else {
        authVersion = GetLowerVersion(authVersion, PREFERRED_AUTH_VERSION);
    }
    QCC_DbgHLPrintf(("ExchangeGuids Local %s", localGuidStr.c_str()));
    QCC_DbgHLPrintf(("ExchangeGuids Remote %s", remoteGuidStr.c_str()));
    QCC_DbgHLPrintf(("ExchangeGuids AuthVersion %d", authVersion));
    /*
     * Now we have the unique bus name in the reply try again to find out if we have a session key
     * for this peer.
     */
    peerState = peerStateTable->GetPeerState(sender, busName);
    peerState->SetGuidAndAuthVersion(remotePeerGuid, authVersion);
    /*
     * We can now return if the peer is authenticated.
     */
    if (peerState->IsSecure()) {
        return ER_OK;
    }
    /*
     * Check again if the peer is being authenticated on another thread. We need to do this because
     * the check above may have used a well-known-namme and now we know the unique name.
     */
    lock.Lock(MUTEX_CONTEXT);
    if (peerState->GetAuthEvent()) {
        if (wait) {
            Event::Wait(*peerState->GetAuthEvent(), lock);
            return peerState->IsSecure() ? ER_OK : ER_AUTH_FAIL;
        } else {
            lock.Unlock(MUTEX_CONTEXT);
            return ER_WOULDBLOCK;
        }
    }
    /*
     * The bus allows a peer to send signals and make method calls to itself. If we are securing the
     * local peer we obviously don't need to authenticate but we must initialize a peer state object
     * with a session key and group key.
     */
    if (bus->GetUniqueName() == sender) {
        assert(remoteGuidStr == localGuidStr);
        QCC_DbgHLPrintf(("Securing local peer to itself"));
        KeyBlob key;
        /* Use the local peer's GROUP key */
        peerStateTable->GetGroupKey(key);
        key.SetTag("SELF", KeyBlob::NO_ROLE);
        peerState->SetKey(key, PEER_GROUP_KEY);
        /* Allocate a random session key - no role because we are both INITIATOR and RESPONDER */
        key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
        key.SetTag("SELF", KeyBlob::NO_ROLE);
        peerState->SetKey(key, PEER_SESSION_KEY);
        /* Record in the peer state that this peer is the local peer */
        peerState->isLocalPeer = true;
        /* Set rights on the local peer - treat as mutual authentication */
        SetRights(peerState, true, false);
        /* We are still holding the lock */
        lock.Unlock(MUTEX_CONTEXT);
        return ER_OK;
    }
    /*
     * Only method calls trigger authentications so if the remote peer is not authenticated or in the
     * process or being authenticated we return an error status which will cause a security
     * violation notification back to the application.
     */
    if (msgType != MESSAGE_METHOD_CALL) {
        /* We are still holding the lock */
        lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_DESTINATION_NOT_AUTHENTICATED;
    }
    /*
     * Other threads authenticating the same peer will block on this event until the authentication completes.
     */
    qcc::Event authEvent;
    peerState->SetAuthEvent(&authEvent);
    lock.Unlock(MUTEX_CONTEXT);

    KeyStore& keyStore = bus->GetInternal().GetKeyStore();
    bool authTried = false;
    bool firstPass = true;
    do {
        /*
         * Try to load the master secret for the remote peer. It is possible that the master secret
         * has expired or been deleted either locally or remotely so if we fail to establish a
         * session key on the first pass we start an authentication conversation to establish a new
         * master secret.
         */
        if (!keyStore.HasKey(remotePeerGuid)) {
            /*
             * If the key store is shared try reloading in case another application has already
             * authenticated this peer.
             */
            if (keyStore.IsShared()) {
                keyStore.Reload();
                if (!keyStore.HasKey(remotePeerGuid)) {
                    status = ER_AUTH_FAIL;
                }
            } else {
                status = ER_AUTH_FAIL;
            }
        }
        if (status == ER_OK) {
            /*
             * Generate a random string - this is the local half of the seed string.
             */
            qcc::String nonce = RandHexString(NONCE_LEN);
            /*
             * Send GenSessionKey message to remote peer.
             */
            MsgArg args[3];
            args[0].Set("s", localGuidStr.c_str());
            args[1].Set("s", remoteGuidStr.c_str());
            args[2].Set("s", nonce.c_str());
            const InterfaceDescription::Member* genSessionKeyMember = ifc->GetMember("GenSessionKey");
            assert(genSessionKeyMember);
            status = remotePeerObj.MethodCall(*genSessionKeyMember, args, ArraySize(args), replyMsg, DEFAULT_TIMEOUT);
            if (status == ER_OK) {
                qcc::String verifier;
                /*
                 * The response completes the seed string so we can generate the session key.
                 */
                status = KeyGen(peerState, nonce + replyMsg->GetArg(0)->v_string.str, verifier, KeyBlob::INITIATOR);
                if ((status == ER_OK) && (verifier != replyMsg->GetArg(1)->v_string.str)) {
                    status = ER_AUTH_FAIL;
                }
            }
        }
        if ((status == ER_OK) || !firstPass) {
            break;
        }
        if (UseKeyExchanger(authVersion, supportedAuthSuites, supportedAuthSuitesCount)) {
            uint32_t*remoteAuthSuites = NULL;
            size_t remoteAuthSuitesCount = 0;
            status = AskForAuthSuites(remotePeerObj, ifc, &remoteAuthSuites, &remoteAuthSuitesCount);
            if (status == ER_OK) {
                status = AuthenticatePeerUsingKeyExchange(remoteAuthSuites, remoteAuthSuitesCount, busName, peerState, localGuidStr, remotePeerObj, ifc, remotePeerGuid, mech);
                if (remoteAuthSuites) {
                    delete [] remoteAuthSuites;
                }
            }
        } else {
            status = AuthenticatePeerUsingSASL(busName, peerState, localGuidStr, remotePeerObj, ifc, remotePeerGuid, mech);
        }
        authTried = true;
        firstPass = false;
    } while (status == ER_OK);
    /*
     * Exchange group keys with the remote peer. This method call is encrypted using the session key
     * that we just established.
     */
    if (status == ER_OK) {
        uint8_t keyGenVersion = authVersion & 0xFF;
        uint16_t authV = authVersion >> 16;
        uint8_t sendKeyBlob = (authV <= 1) && (keyGenVersion == 0);
        Message replyMsg(*bus);
        KeyBlob key;
        peerStateTable->GetGroupKey(key);
        StringSink snk;
        MsgArg arg;
        /*
         * KeyGen version 0 exchanges key blobs, version 1 just exchanges the key
         */
        QCC_DbgHLPrintf(("ExchangeGroupKeys using key gen version %d", keyGenVersion));
        if (sendKeyBlob) {
            key.Store(snk);
            arg.Set("ay", snk.GetString().size(), snk.GetString().data());
        } else {
            arg.Set("ay", key.GetSize(), key.GetData());
        }
        const InterfaceDescription::Member* exchangeGroupKeysMember = ifc->GetMember("ExchangeGroupKeys");
        assert(exchangeGroupKeysMember);
        status = remotePeerObj.MethodCall(*exchangeGroupKeysMember, &arg, 1, replyMsg, DEFAULT_TIMEOUT, ALLJOYN_FLAG_ENCRYPTED);
        if (status == ER_OK) {
            if (sendKeyBlob) {
                StringSource src(replyMsg->GetArg(0)->v_scalarArray.v_byte, replyMsg->GetArg(0)->v_scalarArray.numElements);
                status = key.Load(src);
            } else {
                status = key.Set(replyMsg->GetArg(0)->v_scalarArray.v_byte, replyMsg->GetArg(0)->v_scalarArray.numElements, KeyBlob::AES);
            }
            if (status == ER_OK) {
                /*
                 * Tag the group key with the auth mechanism used by ExchangeGroupKeys. Group keys
                 * are inherently directional - only initiator encrypts with the group key. We set
                 * the role to NO_ROLE otherwise senders can't decrypt their own broadcast messages.
                 */
                key.SetTag(replyMsg->GetAuthMechanism(), KeyBlob::NO_ROLE);
                peerState->SetKey(key, PEER_GROUP_KEY);
            }
        }
    }
    /*
     * If an authentication was tried report the authentication completion to allow application to clear UI etc.
     */
    if (authTried) {
        peerAuthListener.AuthenticationComplete(mech.c_str(), sender.c_str(), status == ER_OK);
    }
    /*
     * ER_BUS_REPLY_IS_ERROR_MESSAGE has a specific meaning in the public API an should not be
     * propogated to the caller from this context.
     */
    if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
        status = ER_AUTH_FAIL;
    }
    /*
     * Release any other threads waiting on the result of this authentication.
     */
    lock.Lock(MUTEX_CONTEXT);
    peerState->SetAuthEvent(NULL);
    while (authEvent.GetNumBlockedThreads() > 0) {
        authEvent.SetEvent();
        qcc::Sleep(10);
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AllJoynPeerObj::AuthenticatePeerUsingSASL(const qcc::String& busName, PeerState peerState, qcc::String& localGuidStr, ProxyBusObject& remotePeerObj, const InterfaceDescription* ifc, qcc::GUID128& remotePeerGuid, qcc::String& mech)
{
    QStatus status;
    ajn::SASLEngine::AuthState authState;

    /*
     * Initiaize the SASL engine as responder (i.e. client) this terminology seems backwards but
     * is the terminology used by the DBus specification.
     */
    SASLEngine sasl(*bus, ajn::AuthMechanism::RESPONDER, peerAuthMechanisms, busName.c_str(), peerAuthListener);
    sasl.SetLocalId(localGuidStr);
    /*
     * This will let us know if we need to make an AuthenticationComplete callback below.
     */
    qcc::String inStr;
    qcc::String outStr;
    status = sasl.Advance(inStr, outStr, authState);
    while (status == ER_OK) {
        Message replyMsg(*bus);
        MsgArg arg("s", outStr.c_str());
        const InterfaceDescription::Member* authChallengeMember = ifc->GetMember("AuthChallenge");
        assert(authChallengeMember);
        status = remotePeerObj.MethodCall(*authChallengeMember, &arg, 1, replyMsg, AUTH_TIMEOUT);
        if (status == ER_OK) {
            /*
             * This will let us know if we need to make an AuthenticationComplete callback below.
             */
            if (authState == SASLEngine::ALLJOYN_AUTH_SUCCESS) {
                SetRights(peerState, sasl.AuthenticationIsMutual(), false /*responder*/);
                break;
            }
            inStr = qcc::String(replyMsg->GetArg(0)->v_string.str);
            status = sasl.Advance(inStr, outStr, authState);
            if (authState == SASLEngine::ALLJOYN_AUTH_SUCCESS) {
                KeyBlob masterSecret;
                mech = sasl.GetMechanism();
                status = sasl.GetMasterSecret(masterSecret);
                if (status == ER_OK) {
                    SetRights(peerState, sasl.AuthenticationIsMutual(), false /*responder*/);
                    /* Tag the master secret with the auth mechanism used to generate it */
                    masterSecret.SetTag(mech, KeyBlob::INITIATOR);
                    status = bus->GetInternal().GetKeyStore().AddKey(remotePeerGuid, masterSecret, peerState->authorizations);
                }
            }
        } else {
            status = ER_AUTH_FAIL;
        }
    }
    return status;
}

QStatus AllJoynPeerObj::AskForAuthSuites(ProxyBusObject& remotePeerObj, const InterfaceDescription* ifc, uint32_t** remoteAuthSuites, size_t* remoteAuthCount)
{
    MsgArg arg;
    arg.Set("au", supportedAuthSuitesCount, supportedAuthSuites);
    Message replyMsg(*bus);
    const InterfaceDescription::Member* exchangeSuites = ifc->GetMember("ExchangeSuites");
    assert(exchangeSuites);
    QStatus status = remotePeerObj.MethodCall(*exchangeSuites, &arg, 1, replyMsg, DEFAULT_TIMEOUT);
    if (status != ER_OK) {
        return status;
    }
    uint32_t*remoteSuites;
    size_t remoteSuitesLen;

    status = replyMsg->GetArg(0)->Get("au", &remoteSuitesLen, &remoteSuites);
    if (status != ER_OK) {
        return status;
    }
    *remoteAuthCount = remoteSuitesLen;
    uint32_t*effectiveAuthSuites = new uint32_t[remoteSuitesLen];
    for (size_t cnt = 0; cnt < remoteSuitesLen; cnt++) {
        effectiveAuthSuites[cnt] = remoteSuites[cnt];
    }
    *remoteAuthSuites = effectiveAuthSuites;
    return ER_OK;
}

QStatus AllJoynPeerObj::AuthenticatePeerUsingKeyExchange(const uint32_t* requestingAuthList, size_t requestingAuthCount, const qcc::String& busName, PeerState peerState, qcc::String& localGuidStr, ProxyBusObject& remotePeerObj, const InterfaceDescription* ifc, qcc::GUID128& remotePeerGuid, qcc::String& mech)
{
    QStatus status;

    QCC_DbgHLPrintf(("AuthenticatePeerUsingKeyExchange"));
    KeyExchanger*keyExchanger = GetKeyExchangerInstance(true, requestingAuthList, requestingAuthCount);  /* initiator */
    if (!keyExchanger) {
        return ER_AUTH_FAIL;
    }
    uint32_t remoteAuthMask = 0;
    uint32_t currentSuite = keyExchanger->GetSuite();
    mech = keyExchanger->GetSuiteName();
    KeyExchangerCB kxCB(remotePeerObj, ifc, AUTH_TIMEOUT);
    status = keyExchanger->ExecKeyExchange(currentSuite, kxCB, &remoteAuthMask);

    if ((status == ER_OK) && (remoteAuthMask == currentSuite)) {
        uint8_t authorized = false;
        status = keyExchanger->KeyAuthentication(kxCB, busName.c_str(), &authorized);
        if (authorized) {
            SetRights(peerState, true, false /*responder*/);
            status = RecordMasterSecret(busName, keyExchanger, peerState);
        } else {
            status = ER_AUTH_FAIL;
        }
    } else if (status == ER_OK) {
        status = ER_AUTH_FAIL; /* remote auth mask is 0 */
    }
    delete keyExchanger;  /* no longer needed */

    if (status == ER_OK) {
        return status;
    }
    if (!remoteAuthMask) {
        return ER_AUTH_FAIL; /* done */
    }
    if (requestingAuthCount == 1) {
        return ER_AUTH_FAIL; /* done.  There is no more to try. */
    }
    size_t smallerCount = requestingAuthCount - 1;
    uint32_t*smallerSuites = new uint32_t[smallerCount];
    size_t idx = 0;
    for (size_t cnt = 0; cnt < requestingAuthCount; cnt++) {
        if ((requestingAuthList[cnt] & currentSuite) != currentSuite) {
            assert(idx < smallerCount);
            if (idx >= smallerCount) {
                delete [] smallerSuites;
                return ER_AUTH_FAIL;
            }
            smallerSuites[idx++] = requestingAuthList[cnt];
        }
    }
    status = AuthenticatePeerUsingKeyExchange(smallerSuites, smallerCount, busName, peerState, localGuidStr, remotePeerObj, ifc, remotePeerGuid, mech);
    delete [] smallerSuites;
    return status;
}

QStatus AllJoynPeerObj::AuthenticatePeerAsync(const qcc::String& busName)
{
    assert(bus);
    Message invalidMsg(*bus);
    return DispatchRequest(invalidMsg, SECURE_CONNECTION, busName);
}

QStatus AllJoynPeerObj::DispatchRequest(Message& msg, RequestType reqType, const qcc::String data)
{
    QStatus status;
    QCC_DbgHLPrintf(("DispatchRequest %s", msg->Description().c_str()));
    lock.Lock(MUTEX_CONTEXT);
    if (dispatcher.IsRunning()) {
        Request* req = new Request(msg, reqType, data);
        qcc::AlarmListener* alljoynPeerListener = this;
        status = dispatcher.AddAlarm(Alarm(alljoynPeerListener, req));
        if (status != ER_OK) {
            delete req;
        }
    } else {
        status = ER_BUS_STOPPING;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

void AllJoynPeerObj::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QStatus status;

    assert(bus);
    QCC_DbgHLPrintf(("AllJoynPeerObj::AlarmTriggered"));
    Request* req = static_cast<Request*>(alarm->GetContext());

    switch (req->reqType) {
    case AUTHENTICATE_PEER:
        /*
         * Push the message onto a queue of messages to be encrypted and forwarded in order when
         * the authentication completes.
         */
        lock.Lock(MUTEX_CONTEXT);
        msgsPendingAuth.push_back(req->msg);
        lock.Unlock(MUTEX_CONTEXT);
        /*
         * Pause timeouts so reply handlers don't expire while waiting for authentication to complete
         */
        if (req->msg->GetType() == MESSAGE_METHOD_CALL) {
            bus->GetInternal().GetLocalEndpoint()->PauseReplyHandlerTimeout(req->msg);
        }
        status = AuthenticatePeer(req->msg->GetType(), req->msg->GetDestination(), false);
        if (status != ER_WOULDBLOCK) {
            PeerStateTable* peerStateTable = bus->GetInternal().GetPeerStateTable();
            /*
             * Check each message that is queued waiting for an authentication to complete
             * to see if this is the authentication the message was waiting for.
             */
            lock.Lock(MUTEX_CONTEXT);
            std::deque<Message>::iterator iter = msgsPendingAuth.begin();
            while (iter != msgsPendingAuth.end()) {
                Message msg = *iter;
                if (peerStateTable->IsAlias(msg->GetDestination(), req->msg->GetDestination())) {
                    LocalEndpoint lep =  bus->GetInternal().GetLocalEndpoint();
                    if (status != ER_OK) {
                        /*
                         * If the failed message was a method call push an error response.
                         */
                        if (msg->GetType() == MESSAGE_METHOD_CALL) {
                            Message reply(*bus);
                            reply->ErrorMsg(status, msg->GetCallSerial());
                            bus->GetInternal().GetLocalEndpoint()->PushMessage(reply);
                        }
                    } else {
                        if (msg->GetType() == MESSAGE_METHOD_CALL) {
                            bus->GetInternal().GetLocalEndpoint()->ResumeReplyHandlerTimeout(msg);
                        }
                        BusEndpoint busEndpoint = BusEndpoint::cast(bus->GetInternal().GetLocalEndpoint());
                        bus->GetInternal().GetRouter().PushMessage(msg, busEndpoint);
                    }
                    iter = msgsPendingAuth.erase(iter);
                } else {
                    iter++;
                }
            }
            lock.Unlock(MUTEX_CONTEXT);
            /*
             * Report a single error for the message the triggered the authentication
             */
            if (status != ER_OK) {
                peerAuthListener.SecurityViolation(status, req->msg);
            }
        }
        break;

    case AUTH_CHALLENGE:
        AuthAdvance(req->msg);
        break;

    case KEY_EXCHANGE:
        DoKeyExchange(req->msg);
        break;

    case KEY_AUTHENTICATION:
        DoKeyAuthentication(req->msg);
        break;

    case EXPAND_HEADER:
        ExpandHeader(req->msg, req->data);
        break;

    case SECURE_CONNECTION:
        status = AuthenticatePeer(MESSAGE_METHOD_CALL, req->data, true);
        if (status != ER_OK) {
            peerAuthListener.SecurityViolation(status, req->msg);
        }
        break;
    }

    delete req;
    QCC_DbgHLPrintf(("AllJoynPeerObj::AlarmTriggered - exiting"));
    return;
}

void AllJoynPeerObj::HandleSecurityViolation(Message& msg, QStatus status)
{
    assert(bus);
    PeerStateTable* peerStateTable = bus->GetInternal().GetPeerStateTable();

    QCC_DbgTrace(("HandleSecurityViolation %s %s", QCC_StatusText(status), msg->Description().c_str()));

    if (status == ER_BUS_MESSAGE_DECRYPTION_FAILED) {
        PeerState peerState = peerStateTable->GetPeerState(msg->GetSender());
        /*
         * If we believe the peer is secure we have a clear security violation
         */
        if (peerState->IsSecure()) {
            /*
             * The keys we have for this peer are no good
             */
            peerState->ClearKeys();
        } else if (msg->IsBroadcastSignal()) {
            /*
             * Encrypted broadcast signals are silently ignored
             */
            QCC_DbgHLPrintf(("Discarding encrypted broadcast signal"));
            status = ER_OK;
        }
    }
    /*
     * Report the security violation
     */
    if (status != ER_OK) {
        QCC_DbgTrace(("Reporting security violation %s for %s", QCC_StatusText(status), msg->Description().c_str()));
        peerAuthListener.SecurityViolation(status, msg);
    }
}


void AllJoynPeerObj::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    assert(bus);

    /*
     * We are only interested in names that no longer have an owner.
     */
    if (newOwner == NULL) {
        QCC_DbgHLPrintf(("Peer %s is gone", busName));
        /*
         * Clean up peer state.
         */
        bus->GetInternal().GetPeerStateTable()->DelPeerState(busName);
        /*
         * We are no longer in an authentication conversation with this peer.
         */
        lock.Lock(MUTEX_CONTEXT);
        delete conversations[busName];
        conversations.erase(busName);
        delete keyExConversations[busName];
        keyExConversations.erase(busName);
        lock.Unlock(MUTEX_CONTEXT);
    }
}

void AllJoynPeerObj::AcceptSession(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    size_t numArgs;
    const MsgArg* args;

    assert(bus);
    msg->GetArgs(numArgs, args);
    SessionPort sessionPort = args[0].v_uint16;
    SessionId sessionId = args[1].v_uint32;
    String joiner = args[2].v_string.str;
    SessionOpts opts;
    status = GetSessionOpts(args[3], opts);

    if (status == ER_OK) {
        MsgArg replyArg;

        /* Call bus listeners */
        bool isAccepted = bus->GetInternal().CallAcceptListeners(sessionPort, joiner.c_str(), opts);

        /* Reply to AcceptSession */
        replyArg.Set("b", isAccepted);
        status = MethodReply(msg, &replyArg, 1);

        if ((status == ER_OK) && isAccepted) {
            BusEndpoint sender = bus->GetInternal().GetRouter().FindEndpoint(msg->GetRcvEndpointName());
            if (sender->GetEndpointType() == ENDPOINT_TYPE_REMOTE) {
                RemoteEndpoint rep = RemoteEndpoint::cast(sender);
                const uint32_t VER_250 = 33882112;
                uint32_t protoVersion = rep->GetRemoteProtocolVersion();
                /**
                 * remote daemon is older than version 2.5.0; it will *NOT* send the SessionJoined signal
                 *
                 * Unfortunately, the original form of this code checked the AllJoyn version number rather than the
                 * protocol version number. Since the AllJoyn version number is only valid at release time,
                 * the check was later updated to also filter on protocol version numbers. Therefore protocol
                 * version number works fine except when protocol version is 3 in which case the AllJoyn version
                 * number must be used.
                 */
                if ((protoVersion < 3) || ((protoVersion == 3) && (rep->GetRemoteAllJoynVersion() < VER_250))) {
                    bus->GetInternal().CallJoinedListeners(sessionPort, sessionId, joiner.c_str());
                }
            }
        }
    } else {
        MethodReply(msg, status);
    }
}



void AllJoynPeerObj::SessionJoined(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
{
    assert(bus);

    // dispatch to the dispatcher thread
    size_t numArgs;
    const MsgArg* args;

    msg->GetArgs(numArgs, args);
    assert(numArgs == 3);
    const SessionPort sessionPort = args[0].v_uint16;
    const SessionId sessionId = args[1].v_uint32;
    const char* joiner = args[2].v_string.str;
    bus->GetInternal().CallJoinedListeners(sessionPort, sessionId, joiner);
}

KeyExchanger* AllJoynPeerObj::GetKeyExchangerInstance(bool initiator, const uint32_t* requestingAuthList, size_t requestingAuthCount)
{
    for (size_t cnt = 0; cnt < requestingAuthCount; cnt++) {
        uint32_t suite = requestingAuthList[cnt];
        if ((suite & AUTH_SUITE_ECDHE_ECDSA) == AUTH_SUITE_ECDHE_ECDSA) {
            return new KeyExchangerECDHE_ECDSA(initiator, this, *bus, peerAuthListener);
        }
        if ((suite & AUTH_SUITE_ECDHE_PSK) == AUTH_SUITE_ECDHE_PSK) {
            return new KeyExchangerECDHE_PSK(initiator, this, *bus, peerAuthListener);
        }
        if ((suite & AUTH_SUITE_ECDHE_NULL) == AUTH_SUITE_ECDHE_NULL) {
            return new KeyExchangerECDHE_NULL(initiator, this, *bus, peerAuthListener);
        }
    }
    return NULL;
}

QStatus AllJoynPeerObj::HandleMethodReply(Message& msg, const MsgArg* args, size_t numArgs)
{
    return MethodReply(msg, args, numArgs);
}

QStatus KeyExchangerCB::SendKeyExchange(MsgArg* args, size_t numArgs, Message* replyMsg)
{
    const InterfaceDescription::Member* keyExchange = ifc->GetMember("KeyExchange");
    assert(keyExchange);
    return remoteObj.MethodCall(*keyExchange, args, numArgs, *replyMsg, timeout);
}

QStatus KeyExchangerCB::SendKeyAuthentication(MsgArg* variant, Message* replyMsg)
{
    MsgArg arg;
    arg.Set("v", variant);
    const InterfaceDescription::Member* keyAuth = ifc->GetMember("KeyAuthentication");
    assert(keyAuth);
    return remoteObj.MethodCall(*keyAuth, &arg, 1, *replyMsg, timeout);
}

/**
 * Setup for peer-to-peer authentication. The authentication mechanisms listed can only be used
 * if they are already registered with bus. The authentication mechanisms names are separated by
 * space characters.
 *
 * @param authMechanisms   The names of the authentication mechanisms to set
 * @param listener         Required for authentication mechanisms that require interation with the user
 *                         or application. Can be NULL if not required.
 */
void AllJoynPeerObj::SetupPeerAuthentication(const qcc::String& authMechanisms, AuthListener* listener)
{
    peerAuthMechanisms = authMechanisms;
    peerAuthListener.Set(listener);
    /* setup the peer auth mask */
    size_t pos;
    qcc::String remainder = authMechanisms;
    qcc::String mech;
    supportedAuthSuitesCount = 0;

    bool done = false;
    int count = 0;
    /* count the entries first */
    while (!done) {
        pos = remainder.find_first_of(" ");
        if (pos == qcc::String::npos) {
            mech = remainder;
            done = true;
        } else {
            mech = remainder.substr(0, pos);
        }
        remainder = remainder.substr(pos + 1);
        if (mech == "ANONYMOUS") {
            count++;
        } else if (mech == "EXTERNAL") {
            count++;
        } else if (mech == "ALLJOYN_PIN_KEYX") {
            count++;
        } else if (mech == "ALLJOYN_SRP_KEYX") {
            count++;
        } else if (mech == "ALLJOYN_SRP_LOGON") {
            count++;
        } else if (mech == "ALLJOYN_RSA_KEYX") {
            count++;
        } else if (mech == "ALLJOYN_ECDHE_NULL") {
            count++;
        } else if (mech == "ALLJOYN_ECDHE_PSK") {
            count++;
        } else if (mech == "ALLJOYN_ECDHE_ECDSA") {
            count++;
        } else if (mech == "GSSAPI") {
            count++;
        }
    }
    supportedAuthSuitesCount = count;
    if (count == 0) {
        return;
    }
    supportedAuthSuites = new uint32_t[count];

    done = false;
    int idx = 0;
    remainder = authMechanisms;
    while (!done) {
        pos = remainder.find_first_of(" ");
        if (pos == qcc::String::npos) {
            mech = remainder;
            done = true;
        } else {
            mech = remainder.substr(0, pos);
        }
        remainder = remainder.substr(pos + 1);
        if (mech == "ANONYMOUS") {
            supportedAuthSuites[idx++] = AUTH_SUITE_ANONYMOUS;
        } else if (mech == "EXTERNAL") {
            supportedAuthSuites[idx++] = AUTH_SUITE_EXTERNAL;
        } else if (mech == "ALLJOYN_PIN_KEYX") {
            supportedAuthSuites[idx++] = AUTH_SUITE_PIN_KEYX;
        } else if (mech == "ALLJOYN_SRP_KEYX") {
            supportedAuthSuites[idx++] = AUTH_SUITE_SRP_KEYX;
        } else if (mech == "ALLJOYN_SRP_LOGON") {
            supportedAuthSuites[idx++] = AUTH_SUITE_SRP_LOGON;
        } else if (mech == "ALLJOYN_RSA_KEYX") {
            supportedAuthSuites[idx++] = AUTH_SUITE_RSA_KEYX;
        } else if (mech == "ALLJOYN_ECDHE_NULL") {
            supportedAuthSuites[idx++] = AUTH_SUITE_ECDHE_NULL;
        } else if (mech == "ALLJOYN_ECDHE_PSK") {
            supportedAuthSuites[idx++] = AUTH_SUITE_ECDHE_PSK;
        } else if (mech == "ALLJOYN_ECDHE_ECDSA") {
            supportedAuthSuites[idx++] = AUTH_SUITE_ECDHE_ECDSA;
        } else if (mech == "GSSAPI") {
            supportedAuthSuites[idx++] = AUTH_SUITE_GSSAPI;
        }
    }
}
}
