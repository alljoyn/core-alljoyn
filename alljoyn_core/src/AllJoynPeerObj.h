/**
 * @file
 * The AllJoynPeer object implements interfaces for AllJoyn functionality including header compression and
 * security.
 */

/******************************************************************************
 * Copyright (c) 2010-2012, 2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_ALLJOYNPEEROBJ_H
#define _ALLJOYN_ALLJOYNPEEROBJ_H

#include <qcc/platform.h>

#include <map>
#include <deque>

#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/Timer.h>
#include <qcc/KeyBlob.h>

#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>

#include "BusEndpoint.h"
#include "RemoteEndpoint.h"
#include "PeerState.h"
#include "AuthMechanism.h"
#include "KeyExchanger.h"

namespace ajn {


/* Forward declaration */
class SASLEngine;
class BusAttachment;

/**
 * The AllJoynPeer object @c /org/alljoyn/Bus/Peer implements interfaces that provide AllJoyn
 * functionality.
 *
 * %AllJoynPeerObj inherits from BusObject
 */
class AllJoynPeerObj : public BusObject, public BusListener, public qcc::AlarmListener {
  public:

    /**
     * Constructor
     *
     * @param bus          Bus to associate with /org/alljoyn/Bus/Peer message handler.
     */
    AllJoynPeerObj(BusAttachment& bus);

    /**
     * Initialize and register this AllJoynPeerObj instance.
     *
     * @param bus          Bus to associate with /org/alljoyn/Bus/Peer message handler.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Init(BusAttachment& bus);

    /**
     * Called when object is successfully registered.
     */
    void ObjectRegistered(void);

    /**
     * This function is called when a message with a compressed header has been received but the
     * compression token is unknown. A method call is made to the remote peer to obtain the
     * expansion rule for the compression token.
     *
     * @param msg     The message to be expanded.
     * @param sender  The remote endpoint that the compressed message was received on.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus RequestHeaderExpansion(Message& msg, RemoteEndpoint& sender);

    /**
     * This function is called when an encrypted message requires authentication.
     *
     * @param msg     The message to be encrypted.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus RequestAuthentication(Message& msg);

    /**
     * Setup for peer-to-peer authentication. The authentication mechanisms listed can only be used
     * if they are already registered with bus. The authentication mechanisms names are separated by
     * space characters.
     *
     * @param authMechanisms   The names of the authentication mechanisms to set
     * @param listener         Required for authentication mechanisms that require interation with the user
     *                         or application. Can be NULL if not required.
     */
    void SetupPeerAuthentication(const qcc::String& authMechanisms, AuthListener* listener);

    /**
     * Check if authentication has been enabled.
     *
     * @return  Returns true is there are authentication mechanisms registered.
     */
    bool AuthenticationEnabled() { return !peerAuthMechanisms.empty(); }

    /**
     * Force re-authentication for the specified peer.
     */
    void ForceAuthentication(const qcc::String& busName);

    /**
     * Authenticate the connection to a remote peer. Authentication establishes a session key with a remote peer.
     *
     * @param busName   The bus name of the remote peer we are securing.
     * @param wait      If true the function will block if there is an authentication already in
     *                  progress with the peer on a separate thread. If false, the function will
     *                  return an ER_WOULD_BLOCK status instead of waiting.
     *
     * @return
     *      - ER_OK if successful
     *      - ER_WOULD_BLOCK if there is already an authentication in progress with the remote peer
     *        and the wait parameter was false.
     *      - An error status otherwise
     */
    QStatus AuthenticatePeer(AllJoynMessageType msgType,  const qcc::String& busName, bool wait = true);

    /**
     * Authenticate the connection to a remote peer asynchronously. Authentication establishes a session key with
     * a remote peer.
     *
     * Notification of success or failure will be via the AuthListener.
     *
     * @param busName   The bus name of the remote peer we are securing.
     *
     * @return
     *      - ER_OK if the authentication is successfully begun
     *      - An error status otherwise
     */
    QStatus AuthenticatePeerAsync(const qcc::String& busName);

    /**
     * Reports a security failure. This would normally be due to stale or expired keys.
     *
     * @param msg     The message that had the security violation.
     * @param status  A status code indicating the type of security violation.
     *
     * @return   - ER_OK if the security failure was handled.
     *           - ER_BUS_SECURITY_FATAL if the security failure is not recoverable.
     */
    void HandleSecurityViolation(Message& msg, QStatus status);


    /**
     * Start AllJoynPeerObj.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Start();

    /**
     * Stop AllJoynPeerObj.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Stop();

    /**
     * Although AllJoynPeerObj is not a thread it contains threads that may need to be joined.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise
     */
    QStatus Join();

    /**
     * Dispatcher callback.
     * (For internal use only)
     */
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    /**
     * Factory method to insantiate a KeyExchanger class.
     * @param initiator initiator or responder
     * @param requestingAuthList the list of requesting auth masks
     * @param requestingAuthCount the length of the auth mask list
     * @return an instance of the KeyExchanger; NULL if none of the masks in the list is satisfied.
     */

    KeyExchanger* GetKeyExchangerInstance(bool initiator, const uint32_t* requestingAuthList, size_t requestingAuthCount);

    /**
     * All a KeyExchanger to send a reply message.
     * @param msg the reference message
     * @param args the message arguments
     * @param len the number of message arguments
     * @return status ER_OK for success; error otherwise.
     */
    QStatus HandleMethodReply(Message& msg, const MsgArg* args = NULL, size_t numArgs = 0);

    /**
     * Destructor
     */
    ~AllJoynPeerObj();

  private:

    /**
     * Types of request that can be queued.
     */
    typedef enum {
        AUTHENTICATE_PEER,
        AUTH_CHALLENGE,
        EXPAND_HEADER,
        SECURE_CONNECTION,
        KEY_EXCHANGE,
        KEY_AUTHENTICATION
    } RequestType;

    /* Dispatcher context */
    struct Request {
        Message msg;
        RequestType reqType;
        const qcc::String data;
        Request(const Message& msg, RequestType type, const qcc::String& data) : msg(msg), reqType(type), data(data) { }
    };

    /**
     * Header decompression method'
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void GetExpansion(const InterfaceDescription::Member* member, Message& msg);

    /**
     * ExchangeGuids method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void ExchangeGuids(const InterfaceDescription::Member* member, Message& msg);

    /**
     * GenSessionKey method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void GenSessionKey(const InterfaceDescription::Member* member, Message& msg);

    /**
     * ExchangeGroupKeys method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void ExchangeGroupKeys(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Authentication challenge method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void AuthChallenge(const InterfaceDescription::Member* member, Message& msg);

    /**
     * KeyExchange method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void KeyExchange(const InterfaceDescription::Member* member, Message& msg);

    /**
     * KeyAuthentication method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void KeyAuthentication(const InterfaceDescription::Member* member, Message& msg);

    /**
     * ExchangeSuites method call handler
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void ExchangeSuites(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Process a message to advance an authentication conversation.
     *
     * @param msg  The auth challenge message
     */
    void AuthAdvance(Message& msg);

    /**
     * Process a message to exchange the key authentication suites
     *
     * @param msg  The auth challenge message
     */
    void DoExchangeSuites(Message& msg);

    /**
     * Process a message to start the key exchange negotiation
     *
     * @param msg  The auth challenge message
     */
    void DoKeyExchange(Message& msg);

    /**
     * Process a message to perform the key exchange authentication/verification
     *
     * @param msg  The auth challenge message
     */
    void DoKeyAuthentication(Message& msg);

    /**
     * Process a message to advance an authentication conversation.
     *
     * @param msg  The auth challenge message
     */
    void ExpandHeader(Message& msg, const qcc::String& sendingEndpoint);

    /**
     * Session key generation algorithm.
     *
     * @param peerState  The peer state object where the session key will be store.
     * @param seed       A seed string negotiated by the peers.
     * @param verifier   A verifier string that used to verify the session key.
     */
    QStatus KeyGen(PeerState& peerState, qcc::String seed, qcc::String& verifier, qcc::KeyBlob::Role role);

    /**
     * Get a property from this object
     * @param ifcName the name of the interface
     * @param propName the name of the property requested
     * @param[out] val return the value stored in the property.
     * @return
     *      - ER_OK if property found
     *      - ER_BUS_NO_SUCH_PROPERTY if property not found
     */
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

    /**
     * Called by the bus when the ownership of any well-known name changes.
     *
     * @param busName        The well-known name that has changed.
     * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
     * @param newOwner       The unique name that now owns the name or NULL if the there is no new owner.
     */
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);

    /**
     * AcceptSession method handler called when the local daemon asks permission to accept a JoinSession request.
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void AcceptSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * SessionJoined method handler called when the local daemon has finished setting up the session
     *
     * @param member  The member that was called
     * @param msg     The method call message
     */
    void SessionJoined(const InterfaceDescription::Member* member, const char* srcPath, Message& message);

    /**
     * Add a Request to the peer object's dispatcher
     *
     * @param msg       Message to be dispatched.
     * @param reqType   Type of AllJoynPeerObj request.
     * @param data      Optional reqType specific data.
     */
    QStatus DispatchRequest(Message& msg, AllJoynPeerObj::RequestType reqType, const qcc::String data = "");

    /**
     * Get the next compressed message from the msgsPendingExpansion queue that has the specified
     * compression token. The message is removed from the list.
     *
     * @param msg       Message that was removed.
     * @param token     The compression token
     *
     * @return  Returns true if a message was removed and returned.
     */
    bool RemoveCompressedMessage(Message& msg, uint32_t token);

    /**
     * Record the master secret.
     * @param sender the peer name
     * @param keyExchanger the key exchanger module
     * @param peerState the peer state
     */

    QStatus RecordMasterSecret(const qcc::String& sender, KeyExchanger*keyExchanger, PeerState peerState);

    /**
     * Ask for remote authentication suites.
     * @param remotePeerObj the remote peer object
     * @param ifc the interface object
     * @param localAuthMask the local auth mask
     * @param remoteAuthMask the buffer to store the remote auth mask
     */

    QStatus AskForAuthSuites(ProxyBusObject& remotePeerObj, const InterfaceDescription* ifc, uint32_t**remoteAuthSuites, size_t*remoteAuthCount);

    /**
     * Authenticate Peer using SASL protocol
     */
    QStatus AuthenticatePeerUsingSASL(const qcc::String& busName, PeerState peerState, qcc::String& localGuidStr, ProxyBusObject& remotePeerObj, const InterfaceDescription* ifc, qcc::GUID128& remotePeerGuid, qcc::String& mech);

    /**
     * Authenticate Peer using new Key Exchanger protocol for ECDHE auths
     */
    QStatus AuthenticatePeerUsingKeyExchange(const uint32_t* requestingAuthList, size_t requestingAuthCount, const qcc::String& busName, PeerState peerState, qcc::String& localGuidStr, ProxyBusObject& remotePeerObj, const InterfaceDescription* ifc, qcc::GUID128& remotePeerGuid, qcc::String& mech);


    /**
     * The peer-to-peer authentication mechanisms available to this object
     */
    qcc::String peerAuthMechanisms;

    /**
     * The listener for interacting with the application
     */
    ProtectedAuthListener peerAuthListener;

    /**
     * Peer endpoints currently in an authentication conversation
     */
    std::map<qcc::String, SASLEngine*> conversations;

    /**
     * Peer endpoints currently in an key exchange conversation
     */
    std::map<qcc::String, KeyExchanger*> keyExConversations;

    /** Short term lock to protect the peer object. */
    qcc::Mutex lock;

    /** Dispatcher for handling peer object requests */
    qcc::Timer dispatcher;

    /** Queue of encrypted messages waiting for an authentication to complete */
    std::deque<Message> msgsPendingAuth;

    /** Queue of compressed messages waiting for an expansion rule to be supplied */
    std::deque<Message> msgsPendingExpansion;

    uint16_t supportedAuthSuitesCount;
    uint32_t* supportedAuthSuites;
};


}

#endif
