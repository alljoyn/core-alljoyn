/**
 * @file ICESession.cc
 * ICESession is responsible for ...
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#include <algorithm>
#include <string.h>
#include <qcc/SocketTypes.h>
#include <qcc/Config.h>
#include <qcc/Socket.h>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <qcc/IPAddress.h>
#include <Stun.h>
#include <ICEManager.h>
#include <Component.h>
#include <ICESession.h>
#include <ICECandidate.h>
#include <StunRetry.h>
#include <StunAttribute.h>
#include <StunCredential.h>
#include <ICEStream.h>
#include <alljoyn/version.h>
#include "RendezvousServerInterface.h"
#include "ICECandidate.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICESESSION"

using namespace std;

namespace ajn {

ICESession::~ICESession(void)
{
    // ToDo... need to release any TURN allocations by using Refresh=0?

    // Notify pacing thread to terminate
    terminating = true;

    // Ensure that it terminates
    if (pacingThread) {
        pacingThread->Stop();
        pacingThread->Join();
    }

    Lock();

    // Release resources
    delete pacingThread;
    pacingThread = NULL;

    // Empty queue of messages to send
    while (!stunQueue.empty()) {
        StunWork* stunWork = stunQueue.front();
        delete stunWork->msg;
        delete stunWork;
        stunQueue.pop_front();
    }

    // Notify listeners to terminate
    EmptyICEStreamList();

    // Stop all candidate listener threads by deallocating candidates
    candidates.clear();

    if (shortTermHmacKey) {
        delete[] shortTermHmacKey;
    }

    if (remoteShortTermHmacKey) {
        delete[] remoteShortTermHmacKey;
    }

    if (hmacKey) {
        delete[] hmacKey;
    }

    Unlock();
}

void ICESession::StopPacingThreadAndClearStunQueue(void)
{
    QCC_DbgPrintf(("ICESession::StopPacingThreadAndClearStunQueue()"));

    // Notify pacing thread to terminate
    terminating = true;

}

QStatus ICESession::GetIPAddressFromConnectionData(String& connectionData,
                                                   IPAddress& IPAddressParm)
{
    QStatus status = ER_OK;

    // first determine address type by moving past 'network type'
    size_t found = connectionData.find(' ');
    if (found == connectionData.npos) {
        status = ER_FAIL;
    } else {
        String data = connectionData.substr(found + 1);

        // skip to address
        found = data.find_last_of(' ');
        if (found != data.npos) {
            String address = data.substr(found + 1);
            IPAddressParm = IPAddress(address);
        } else {
            status = ER_FAIL;
        }
    }
    return status;
}

void Tokenize(const String& str,
              vector<String>& tokens,
              const char* delimiters)
{
    // Skip delimiters at beginning.
    size_t lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    size_t pos = str.find_first_of(delimiters, lastPos);

    while (String::npos != pos || String::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

void ICESession::ComposeAndEnqueueStunRequest(Stun* stun, Retransmit* gatherRetransmit)
{
    bool allocateTURN = stun->GetComponent()->GetICEStream()->GetSession()->GetAddRelayedCandidates();

    // We cannot allocate anything on the Relay Server if we do not have the Relay Server Information
    if (!TurnServerAvailable) {
        allocateTURN = false;
    }

    StunMessage* msg;
    StunTransactionID tid;

    if (!gatherRetransmit->GetTransactionID(tid) ||
        (gatherRetransmit->GetState() == Retransmit::ReceivedAuthenticateResponse)) {
        // New transaction
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              (allocateTURN ? STUN_MSG_ALLOCATE_METHOD : STUN_MSG_BINDING_METHOD),
                              stun->GetComponent()->GetHmacKey(), stun->GetComponent()->GetHmacKeyLength());
        msg->GetTransactionID(tid);
        gatherRetransmit->SetTransactionID(tid);
    } else {
        // Retry attempt.
        if (gatherRetransmit->GetErrorCode() == ER_ICE_ALLOCATE_REJECTED_NO_RESOURCES) {
            // If previous attempt to TURN server failed due to lack of resources, omit TURN
            allocateTURN = false;
        }

        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              (allocateTURN ? STUN_MSG_ALLOCATE_METHOD : STUN_MSG_BINDING_METHOD),
                              stun->GetComponent()->GetHmacKey(), stun->GetComponent()->GetHmacKeyLength(),
                              tid);
    }

    QCC_DbgPrintf(("Enqueue Request: class %s, Transaction %s",
                   msg->MessageClassToString(msg->GetTypeClass()).c_str(),
                   tid.ToString().c_str()));

    msg->AddAttribute(new StunAttributeSoftware(String("AllJoyn ") + String(GetVersion())));

    msg->AddAttribute(new StunAttributeUsername(usernameForShortTermCredential));

    msg->AddAttribute(new StunAttributeRequestedTransport(REQUESTED_TRANSPORT_TYPE_UDP));

    if (allocateTURN) {
        uint32_t requestedLifetime = TURN_PERMISSION_REFRESH_PERIOD_SECS;
        msg->AddAttribute(new StunAttributeLifetime(requestedLifetime));
    }

    msg->AddAttribute(new StunAttributeMessageIntegrity(*msg));

    msg->AddAttribute(new StunAttributeFingerprint(*msg));

    IPEndpoint Server = StunServer;

    if (allocateTURN) {
        Server = TurnServer;
    }

    // enqueue our request
    stunQueue.push_back(new StunWork(stun, msg, Server));
}

void ICESession::EnqueueTurnRefresh(StunActivity* stunActivity)
{
    Retransmit& retransmit = stunActivity->retransmit;
    Stun* stun = stunActivity->stun;
    StunMessage* msg;
    StunTransactionID tid;

    if (!retransmit.GetTransactionID(tid) ||
        (retransmit.GetState() == Retransmit::ReceivedAuthenticateResponse)) {
        // First attempt (or required to use different transaction)
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              STUN_MSG_REFRESH_METHOD,
                              stun->GetComponent()->GetHmacKey(),
                              stun->GetComponent()->GetHmacKeyLength());
        msg->GetTransactionID(tid);
        retransmit.SetTransactionID(tid);
    } else {
        // Retry attempt.
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              STUN_MSG_REFRESH_METHOD,
                              stun->GetComponent()->GetHmacKey(),
                              stun->GetComponent()->GetHmacKeyLength(),
                              tid);
    }

    msg->AddAttribute(new StunAttributeSoftware(String("AllJoyn ") + String(GetVersion())));

    if (retransmit.GetState() == Retransmit::ReceivedAuthenticateResponse) {
        msg->AddAttribute(new StunAttributeUsername(usernameForShortTermCredential));

        uint32_t requestedLifetime = TURN_PERMISSION_REFRESH_PERIOD_SECS;
        msg->AddAttribute(new StunAttributeLifetime(requestedLifetime));
        msg->AddAttribute(new StunAttributeRequestedTransport(REQUESTED_TRANSPORT_TYPE_UDP));
        msg->AddAttribute(new StunAttributeMessageIntegrity(*msg));
    }
    msg->AddAttribute(new StunAttributeFingerprint(*msg));

    // Enqueue our request
    stunQueue.push_back(new StunWork(stun, msg, TurnServer));
}

void ICESession::ComposeAndEnqueueNATKeepalive(Stun* stun, IPEndpoint& destination)
{
    StunMessage* msg = new StunMessage(STUN_MSG_INDICATION_CLASS, STUN_MSG_BINDING_METHOD,
                                       stun->GetComponent()->GetHmacKey(), stun->GetComponent()->GetHmacKeyLength());

    // Per ICE spec (Section 10) should not contain any attributes.

    // Enqueue our request
    stunQueue.push_back(new StunWork(stun, msg, destination));
}



// Sort candidates prior to determining uniqueness. The ordering logic
// below is arbitrary.
bool compareCandidatesForFoundation(FoundationAttrs* first, FoundationAttrs* second)
{
    bool greater = false;

    if (first->candidateType > second->candidateType) {
        greater = true;
    } else if (first->candidateType == second->candidateType) {
        if (second->baseAddr.ToString() < first->baseAddr.ToString()) {
            greater = true;
        } else if (first->baseAddr.ToString() == second->baseAddr.ToString()) {
            if (second->serverAddr.ToString() < first->serverAddr.ToString()) {
                greater = true;
            } else {
                greater = first->transportProtocol > second->transportProtocol;
            }
        }
    }
    return greater;
}


void ICESession::DeterminePeerReflexiveFoundation(IPAddress addr,
                                                  SocketType transportProtocol,
                                                  String& foundation)
{
    foundation = "";

    // See if candidate match exists.
    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {

        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {

            Component::const_iterator candidateIt;
            for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                if ((*candidateIt)->GetType() == _ICECandidate::PeerReflexive_Candidate &&
                    (*candidateIt)->GetBase().addr == addr &&
                    (*candidateIt)->GetTransportProtocol() == transportProtocol) {
                    foundation = (*candidateIt)->GetFoundation();
                    goto matchTest;
                }
            }
        }
    }

matchTest:
    // If no match exists, return a new foundation
    if (foundation == "") {
        foundation = U32ToString((uint32_t)++ foundationID, 10);
    }
}

// Section 4.1.1.3 draft-ietf-mmusic-ice-19
void ICESession::AssignFoundations(void)
{
    list<FoundationAttrs*> candidateList;

    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {

        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {

            Component::iterator candidateIt;
            for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                candidateList.push_back((*candidateIt)->GetFoundationAttrs());
            }
        }
    }
    // sort the list
    candidateList.sort(compareCandidatesForFoundation);

    // walk the list, assigning unique foundations
    FoundationAttrs prev;

    while (!candidateList.empty()) {
        FoundationAttrs*foundationAttrs = candidateList.front();
        if (*foundationAttrs != prev) {
            foundationID++;
        }
        foundationAttrs->IceCandidate->SetFoundation(U32ToString((uint32_t) foundationID, 10));
        prev = *foundationAttrs;

        delete foundationAttrs;
        candidateList.pop_front();
    }
}


void ICESession::AssignPriorities(void)
{
    list<FoundationAttrs*> candidateList;

    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {
        AssignPrioritiesPerICEStream(*streamIt);
    }
}

uint32_t ICESession::AssignPriority(uint16_t componentID,
                                    const ICECandidate& IceCandidate,
                                    _ICECandidate::ICECandidateType candidateType)
{
    QCC_DbgPrintf(("ICESession::AssignPriority(): OnDemandAddress(%s) PersistentAddress(%s)", OnDemandAddress.ToString().c_str(), PersistentAddress.ToString().c_str()));
    uint16_t typePreference = 0;
    uint16_t localPreference = 0;

    // Set 'type' preference per draft-ietf-mmusic-ice-19 Section 4.1.2.2
    switch (candidateType) {
    case _ICECandidate::Host_Candidate:
        if (networkInterface.IsMultiHomed() &&
            networkInterface.IsVPN(IceCandidate->GetBase().addr)) {
            typePreference = 0;
        } else {
            typePreference = 126;
        }
        break;

    case _ICECandidate::ServerReflexive_Candidate:
        typePreference = 100;
        break;

    case _ICECandidate::Relayed_Candidate:
        typePreference = 0;
        break;

    case _ICECandidate::PeerReflexive_Candidate:
        typePreference = 110;
        break;

    default:
        break;
    }

    // Set 'local' preference per draft-ietf-mmusic-ice-19 Section 4.1.2.2
    if (!networkInterface.IsMultiHomed()) {
        localPreference = 65535;
    } else {
        if (_ICECandidate::Host_Candidate == candidateType &&
            networkInterface.IsVPN(IceCandidate->GetBase().addr)) {
            localPreference = 0;
        } else {
            if (IceCandidate->GetBase().addr.IsIPv6()) {
                localPreference = 40000;
            } else {
                localPreference = 20000;
            }
        }

        // Bump up the priority of the interfaces that have been used for connection with the Rendezvous
        // Server
        if ((IceCandidate->GetBase().addr == OnDemandAddress) ||
            (IceCandidate->GetBase().addr == PersistentAddress)) {
            QCC_DbgPrintf(("ICESession::AssignPriority(): Bumping up the priority of candidate with IP Addr %s as it is "
                           "used in the Rendezvous Connection", IceCandidate->GetBase().addr.ToString().c_str()));
            localPreference += 25535;
        }
    }

    uint32_t priority = ((typePreference << 24) & 0x7e000000) +
                        ((localPreference << 8) & 0xffff00) +
                        256 - componentID;

    return priority;
}

void ICESession::AssignPrioritiesPerICEStream(const ICEStream* stream)
{
    ICEStream::const_iterator componentIt;
    for (componentIt = stream->Begin(); componentIt != stream->End(); ++componentIt) {

        Component::iterator candidateIt;
        for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
            (*candidateIt)->SetPriority(AssignPriority((*componentIt)->GetID(), *candidateIt, (*candidateIt)->GetType()));
        }
    }
}

void ICESession::StunTurnPacingWork(void)
{
    // Note: we enter this method holding the object lock!!!
    // Therefore ensure that we are holding it when we exit.

    uint32_t pacingIntervalMsecs = 500;

    Thread* thisThread = Thread::GetThread();
    while (!terminating && !thisThread->IsStopping()) {
        // If any requests are to be sent, enqueue them. Check for timeouts.
        FindPendingWork();

        // Now send request at top of the queue
        if (stunQueue.size() > 0) {
            StunWork* stunWork = stunQueue.front();

            QStatus status = stunWork->stun->SendStunMessage(*(stunWork->msg),
                                                             stunWork->destination.addr,
                                                             stunWork->destination.port,
                                                             false); // not sending to peer
            if (ER_OK != status) {
                QCC_LogError(status, ("StunTurnPacingWork"));
                terminating = true;
            }

            delete stunWork->msg;
            delete stunWork;
            stunQueue.pop_front();
        }

        Unlock();
        qcc::Sleep(pacingIntervalMsecs);
        Lock();
    }
}



String ICESession::GetTransport(const String& transport) const
{
    String convertedTransport = "UDP";

    if (transport.find("RTP") == transport.npos &&
        transport.find("RTCP") == transport.npos) {
        convertedTransport = "fixme";
    }
    return convertedTransport;
}


QStatus ICESession::UpdateLocalICECandidates(void)
{
    QStatus status = ER_OK;

    // Add random short-term credential material
    uint8_t ufragBuf[ICE_CREDENTIAL_UFRAG_CHAR_LENGTH];
    uint8_t pwdBuf[ICE_CREDENTIAL_PWD_CHAR_LENGTH];

    Crypto_GetRandomBytes(ufragBuf, ICE_CREDENTIAL_UFRAG_CHAR_LENGTH);
    Crypto_GetRandomBytes(pwdBuf, ICE_CREDENTIAL_PWD_CHAR_LENGTH);

    // Compute the short-term credential for inbound checks.
    StunCredential stunCredential(String(reinterpret_cast<char*>(pwdBuf), sizeof(pwdBuf)));

    // Size buffer first
    //stunCredential.GetKey(NULL, hmacKeyLen);
    //uint8_t* key = new uint8_t[hmacKeyLen];

    // Now get the real key ...
    //stunCredential.GetKey(key, hmacKeyLen);

    ufrag = BytesToHexString(ufragBuf, sizeof(ufragBuf));
    pwd = BytesToHexString(pwdBuf, sizeof(pwdBuf));

    shortTermHmacKeyLength = pwd.size();

    if (shortTermHmacKey) {
        delete[] shortTermHmacKey;
        shortTermHmacKey = NULL;
    }

    shortTermHmacKey = new uint8_t[shortTermHmacKeyLength];

    if (!shortTermHmacKey) {
        status = ER_ICE_ALLOCATING_MEMORY;
        QCC_LogError(status, ("Allocating memory for shortTermHmacKey"));
        return status;
    }

    ::memcpy(shortTermHmacKey, pwd.data(), pwd.size());

    // Add candidates
    stream_const_iterator streamIt;
    ICECandidates tempCandidate;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {

        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
            Component::const_iterator candidateIt;
            for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                IPEndpoint endPoint = (*candidateIt)->GetEndpoint();
                String endPointPort = U32ToString((uint32_t) endPoint.port, 10);
                IPEndpoint base = (*candidateIt)->GetBase();
                String basePort = U32ToString((uint32_t)base.port, 10);

                tempCandidate.type = GetICECandidateTypeValue((*candidateIt)->GetTypeString());
                tempCandidate.foundation = (*candidateIt)->GetFoundation();
                tempCandidate.componentID = (*componentIt)->GetID();
                tempCandidate.transport = GetICETransportTypeValue(GetTransport((*componentIt)->GetTransport()));
                tempCandidate.priority = (*candidateIt)->GetPriority();
                tempCandidate.address = endPoint.addr;
                tempCandidate.port = endPoint.port;

                if ((*candidateIt)->GetType() ==  _ICECandidate::ServerReflexive_Candidate ||
                    (*candidateIt)->GetType() ==  _ICECandidate::PeerReflexive_Candidate) {

                    tempCandidate.raddress = base.addr;
                    tempCandidate.rport = base.port;
                }

                if ((*candidateIt)->GetType() ==  _ICECandidate::Relayed_Candidate) {
                    IPEndpoint mappedAddress = (*candidateIt)->GetMappedAddress();
                    String mappedAddressPort = U32ToString(mappedAddress.port, 10);

                    tempCandidate.raddress = mappedAddress.addr;
                    tempCandidate.rport = mappedAddress.port;
                }

                candidates.push_back(tempCandidate);
            }
        }
    }

    return status;
}

void ICESession::ComposeCandidateList(list<ICECandidate>& composedList) const
{
    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {
        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
            Component::const_iterator candidateIt;
            for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                composedList.push_back(*candidateIt);
            }
        }
    }
}


void ICESession::SortAndPruneCandidatePairs(void)
{
    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {
        (*streamIt)->SortAndPruneCandidatePairs();
    }
}

QStatus ICESession::StartInitialCheckList(void)
{
    // Set the first media stream to active
    stream_const_iterator streamIt = streamList.begin();

    return (streamIt != streamList.end()) ? (*streamIt)->ActivateCheckList() : ER_FAIL;
}

void ICESession::StartSubsequentCheckList(ICEStream*stream)
{
    stream->ActivateCheckList();
}




void ICESession::ComposeICEStreamCandidateList(list<ICECandidate>& composedList) const
{
    stream_const_iterator streamIt;
    size_t stream = 0;

    // This will always return the local candidates for steam 0
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++stream, ++streamIt) {
        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
            Component::const_iterator candidateIt;
            for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                composedList.push_back(*candidateIt);
            }
        }
        break;
    }
}


bool compareCandidatesToEliminate(const ICECandidate& first, const ICECandidate& second)
{
    IPEndpoint firstEndPoint = first->GetEndpoint();
    IPEndpoint secondEndPoint = second->GetEndpoint();
    IPEndpoint firstBase = first->GetBase();
    IPEndpoint secondBase = second->GetBase();

    bool greater = false;

    if (secondEndPoint.addr.ToString() < firstEndPoint.addr.ToString()) {
        greater = true;
    } else if (firstEndPoint.addr.ToString() == secondEndPoint.addr.ToString()) {
        if (firstEndPoint.port > secondEndPoint.port) {
            greater = true;
        } else if (firstEndPoint.port == secondEndPoint.port) {
            if (secondBase.addr.ToString() < firstBase.addr.ToString()) {
                greater = true;
            } else if (firstBase.addr.ToString() == secondBase.addr.ToString()) {
                if (firstBase.port > secondBase.port) {
                    greater = true;
                } else if (firstBase.port == secondBase.port) {
                    greater = first->GetPriority() > second->GetPriority();
                }
            }
        }
    }

    return greater;
}

void ICESession::EliminateRedundantCandidates(void)
{
    list<ICECandidate> composedList;
    list<ICECandidate>::iterator iter;

    ComposeCandidateList(composedList);
    composedList.sort(compareCandidatesToEliminate);
    ICECandidate prev;

    for (iter = composedList.begin(); iter != composedList.end(); ++iter) {
        if (prev->GetType() == _ICECandidate::Invalid_Candidate) {
            prev = *iter;
            continue;
        }

        if ((prev->GetEndpoint() == (*iter)->GetEndpoint()) && (prev->GetBase() == (*iter)->GetBase())) {
            Component*component = (*iter)->GetComponent();
            // This is guaranteed to be the lower priority candidate
            component->RemoveCandidate(*iter);
        } else {
            prev = *iter;
        }
    }
}

void ICESession::ChooseDefaultCandidates(void)
{
    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {
        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
            Component::const_iterator candidateIt;
            for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                (*componentIt)->AssignDefaultCandidate(*candidateIt);
            }
        }
    }
}

//todo deallocate non-selected local candidates on checks completing.
bool ICESession::RelayedCandidateActivityIsStale(StunActivity* stunActivity)
{
    bool isStale = false;

    Retransmit::RetransmitState retryState = stunActivity->retransmit.GetState();

    switch (retryState) {
    case Retransmit::ReceivedAuthenticateResponse:
        // Server has responded to previous request with a challenge for credentials,
        // so arrange for this to appear first in sorted list.
        stunActivity->retransmit.PrematurelyAge();

        // This will also refresh NAT bindings..
        isStale = true;
        break;

    case Retransmit::AwaitingResponse:
        if (stunActivity->retransmit.RetryTimedOut()) {
            if (stunActivity->retransmit.RetryAvailable()) {
                isStale = true;
            } else {
                // All retry attempts failed.
                stunActivity->retransmit.SetState(Retransmit::NoResponseToAllRetries);
            }
        }
        break;

    case Retransmit::ReceivedSuccessResponse:   // previous attempt refreshed successfully
    case Retransmit::AwaitingTransmitSlot:      // very first refresh attempt
    case Retransmit::NoResponseToAllRetries:    // optimistic, huh.
        {
            // Time for next keepalive?

            // How long ago did we refresh?
            uint32_t ageMsecs = stunActivity->retransmit.GetAwaitingTransmitTimeMsecs();
            // How long were we given to live?
            uint32_t refreshAgeSecs =
                (stunActivity == stunActivity->candidate->GetPermissionStunActivity()) ?
                TURN_PERMISSION_REFRESH_PERIOD_SECS :
                stunActivity->candidate->GetAllocationLifetimeSeconds();

            if (ageMsecs + (TURN_REFRESH_WARNING_PERIOD_SECS * 1000) > refreshAgeSecs * 1000) {
                isStale = true;
            }
            break;
        }

    default:
        break;
    }

    return isStale;
}


bool compareStunActivitiesByTime(StunActivity* first, StunActivity* second)
{
    // longer wait is higher priority
    return (first->retransmit.GetAwaitingTransmitTimeMsecs() >
            second->retransmit.GetAwaitingTransmitTimeMsecs());
}





void ICESession::GetAllReadyStunActivities(list<StunActivity*>& foundList)
{
    // assume we are done, and have no more messages to transmit/retransmit
    bool allCandidatesGathered = true;
#ifdef AGGRESSIVE_FAIL_GATHERING
    bool anyCandidatesFailedRetries = false;
#endif
    bool errorFound = false;

    stream_const_iterator streamIt;
    for (streamIt = streamList.begin(); streamIt != streamList.end(); ++streamIt) {
        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
            const list<StunActivity*>*stunActivityList = (*componentIt)->GetStunActivityList();

            list<StunActivity*>::const_iterator stunActivityIt;
            for (stunActivityIt = stunActivityList->begin(); stunActivityIt != stunActivityList->end(); ++stunActivityIt) {
                switch ((*stunActivityIt)->candidate->GetType()) {
                case _ICECandidate::Relayed_Candidate:
                    {
                        // Once gathered, any relayed candidate must be periodically refreshed.
                        if (RelayedCandidateActivityIsStale(*stunActivityIt)) {
                            foundList.push_back(*stunActivityIt);
                        }
                        break;
                    }

                case _ICECandidate::ServerReflexive_Candidate:
                case _ICECandidate::PeerReflexive_Candidate:
                    {
                        // During and after gathering phase, the NAT bindings for each reflexive
                        // candidate are 'kept-alive' by sending a periodic Binding Indication.

                        // See if it's time to send another indication (just to keep NAT bindings alive.)
                        // We do not timeout on a response.
                        // ToDo: the retransmit timestamp should be updated each time the application sends
                        // via the Stun object, probably in ice_SpliceSend (or the equivalent...)
                        if ((*stunActivityIt)->retransmit.GetAwaitingTransmitTimeMsecs() >
                            STUN_KEEP_ALIVE_INTERVAL_IN_MILLISECS) {
                            foundList.push_back(*stunActivityIt);
                        }
                        break;
                    }

                case _ICECandidate::Host_Candidate:
                    {
                        Retransmit& retransmit = (*stunActivityIt)->retransmit;

                        switch (retransmit.GetState()) {
                        case Retransmit::AwaitingTransmitSlot:
                        case Retransmit::ReceivedAuthenticateResponse:
                            foundList.push_back(*stunActivityIt);
                            allCandidatesGathered = false;
                            break;

                        case Retransmit::AwaitingResponse:
                            // see if we have timed out
                            if (retransmit.RetryTimedOut()) {
                                //verify that we have not exceeded retries
                                if (retransmit.AnyRetriesNotSent()) {
                                    retransmit.SetState(Retransmit::AwaitingTransmitSlot);
                                    foundList.push_back(*stunActivityIt);
                                    allCandidatesGathered = false;
                                } else {
                                    // We are done with attempting to reach the server on this candidate.
                                    retransmit.SetState(Retransmit::NoResponseToAllRetries);
#ifdef AGGRESSIVE_FAIL_GATHERING
                                    anyCandidatesFailedRetries = true;
                                    SetErrorCode(ER_ICE_SERVER_NO_RESPONSE);
#endif
                                }
                            } else {
                                // We haven't timed out yet. Give this guy a chance.
                                allCandidatesGathered = false;
                            }
                            break;

                        case Retransmit::ReceivedErrorResponse:
                            errorFound = true;
                            SetErrorCode(retransmit.GetErrorCode());
                            break;

                        case Retransmit::NoResponseToAllRetries:
#ifdef AGGRESSIVE_FAIL_GATHERING
                            anyCandidatesFailedRetries = true;
                            SetErrorCode(ER_ICE_SERVER_NO_RESPONSE);
#endif
                            break;

                        case Retransmit::ReceivedSuccessResponse:
                            // All done gathering for this local interface.
                            break;

                        case Retransmit::Error:
                        default:
                            errorFound = true;
                            SetErrorCode(ER_ICE_STUN_ERROR);
                            break;
                        }
                        break;
                    }

                case _ICECandidate::Invalid_Candidate:
                default:
                    break;
                }
            }
        }
    }

#ifdef AGGRESSIVE_FAIL_GATHERING
    // Consider any failure to contact the STUN/TURN server
    // (including all retries) by a host candidate to be fatal.
    if (errorFound || anyCandidatesFailedRetries) {
#else
    if (errorFound) {
#endif
        // Only notify app one time.
        if (ICEGatheringCandidates == GetState()) {
            SetState(ICEProcessingFailed);
            sessionListener->ICESessionChanged(this);
        } else {
            SetState(ICEProcessingFailed);
        }
    } else if (allCandidatesGathered) {
        if (ICEGatheringCandidates == GetState()) {
            SetState(ICECandidatesGathered);

            // Gathering phase is successful.
            AssignFoundations();
            AssignPriorities();
            EliminateRedundantCandidates();
            ChooseDefaultCandidates();
            QStatus status = UpdateLocalICECandidates();
            if (ER_OK != status) {
                SetErrorCode(status);
                SetState(ICEProcessingFailed);
            }

            sessionListener->ICESessionChanged(this);
        }
    }
}


void ICESession::FindPendingWork(void)
{
    list<StunActivity*> stunReadyList;

    // Create list of all StunActivity objects waiting to transmit/retransmit
    // Also time out any ones that are overdue, and notify app if appropriate.
    GetAllReadyStunActivities(stunReadyList);

    if (stunReadyList.size() > 0) {
        // Some are ready now, sort the list by time waiting to transmit/retransmit
        stunReadyList.sort(compareStunActivitiesByTime);

        // Take the oldest.
        StunActivity* nextStunActivity = stunReadyList.front();
        stunReadyList.pop_front();

        switch (nextStunActivity->candidate->GetType()) {
        case _ICECandidate::Host_Candidate:
            // Queue it for transmit/retransmit.
            // (We have already verified that we will not exceed retries.)
            ComposeAndEnqueueStunRequest(nextStunActivity->stun, &nextStunActivity->retransmit);

            // Update its time stamp and set state to awaiting response
            nextStunActivity->retransmit.IncrementAttempts();
            break;

        case _ICECandidate::ServerReflexive_Candidate:
        case _ICECandidate::PeerReflexive_Candidate:
            {
                // Queue it for transmit only. Because this is an
                // Indication there is no retransmit on timeout.
                IPEndpoint destination = nextStunActivity->candidate->GetType() == _ICECandidate::ServerReflexive_Candidate ?
                                         StunServer :
                                         StunServer;                                     //ToDo use peer-reflexive address
                ComposeAndEnqueueNATKeepalive(nextStunActivity->stun, destination);

                // Update its time stamp.
                nextStunActivity->retransmit.IncrementAttempts();
                break;
            }

        case _ICECandidate::Relayed_Candidate:
            {
                // Queue it for transmit/retransmit.
                // (We have already verified that we will not exceed retries.)
                if (nextStunActivity->candidate->GetPermissionStunActivity() ==
                    nextStunActivity) {
                    EnqueueTurnCreatePermissions(nextStunActivity->candidate);
                } else {
                    EnqueueTurnRefresh(nextStunActivity);
                }

                // Update its time stamp and set state to awaiting response
                nextStunActivity->retransmit.IncrementAttempts();
                break;
            }

        case _ICECandidate::Invalid_Candidate:
        default:
            break;
        }
    } else {
        // nothing is ready at the moment to transmit.
    }
}


QStatus ICESession::StartStunTurnPacingThread(void)
{
    QStatus status = ER_OK;

    SetState(ICEGatheringCandidates);

    pacingThread = new Thread("GatheringKeepalivePacingThreadStub", GatheringKeepalivePacingThreadStub);

    // Start the thread which will send STUN/TURN requests (and retries), at appropriate
    // pace. Once candidates are gathered, it will perform periodic keepalives.

    status = pacingThread->Start(this);
    if (ER_OK != status) {
        SetState(ICEProcessingFailed);
    } else {
        // Try to mitigate the chance of memory corruption, should caller immediately delete this object
        // upon return. (See comment at definition of 'GatheringKeepalivePacingThreadStub'
        qcc::Sleep(1);
    }

    return status;
}

QStatus ICESession::GatherHostCandidates(bool enableIpv6)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("ICESession::GatherHostCandidates(): enableIpv6 = %d", enableIpv6));

    uint16_t streamIndex = 0;
    SocketType socketType = QCC_SOCK_DGRAM;
    AddressFamily af = QCC_AF_INET;

    Component* component = NULL;
    Component* implicitComponent = NULL;

    uint16_t port = 0;

    // Create a stream object
    ICEStream* stream = new ICEStream(streamIndex, this, STUNInfo, hmacKey, hmacKeyLen);
    streamList.push_back(stream);

    // add candidates per default connections
    status = stream->AddComponent(af, socketType, component, implicitComponent);
    if (ER_OK != status) {
        QCC_LogError(status, ("stream->AddComponent"));
        return status;
    }

    if (!component) {
        status = ER_FAIL;
        QCC_LogError(status, ("component is NULL"));
        return status;
    }

    // now see if we want to look for all local network interfaces
    if (addHostCandidates) {
        // Update the interface list
        status = networkInterface.UpdateNetworkInterfaces();
        if (status != ER_OK) {
            QCC_LogError(status, ("%s: networkInterface.UpdateNetworkInterfaces() failed", __FUNCTION__));
            return status;
        }

        // Ensure that live interfaces are available before proceeding further
        if (!networkInterface.IsAnyNetworkInterfaceUp()) {
            status = ER_FAIL;
            QCC_LogError(status, ("%s: None of the interfaces are up", __FUNCTION__));
            return status;
        }

        std::vector<qcc::IfConfigEntry>::iterator networkInterfaceIter;

        for (networkInterfaceIter = networkInterface.liveInterfaces.begin(); networkInterfaceIter != networkInterface.liveInterfaces.end(); ++networkInterfaceIter) {
            // Ignore IPv6 interfaces if IPv6 support is disabled
            if ((!enableIpv6) && (networkInterfaceIter->m_family == QCC_AF_INET6)) {
                continue;
            }

            if (NULL != component) {
                // (This typically ignores the specified port and OS binds to ephemeral port.)
                status = component->CreateHostCandidate(socketType, qcc::IPAddress(networkInterfaceIter->m_addr), port, networkInterfaceIter->m_mtu);

                if (status != ER_OK) {
                    QCC_LogError(status, ("component->CreateHostCandidate"));
                    stream->RemoveComponent(component);
                    delete component;
                    component = NULL;
                }
            }

            if (NULL != implicitComponent) {
                // (This typically ignores the specified port and OS binds to ephemeral port.)
                status = implicitComponent->CreateHostCandidate(socketType, qcc::IPAddress(networkInterfaceIter->m_addr), port + 1, networkInterfaceIter->m_mtu);

                if (status != ER_OK) {
                    QCC_LogError(status, ("implicitComponent->CreateHostCandidate"));
                    delete implicitComponent;
                    implicitComponent = NULL;
                }
            }
        }
    }

    return status;
}

QStatus ICESession::Init(void)
{
    QStatus status = ER_OK;

    // Gather candidates for host
    status = GatherHostCandidates(EnableIPv6);
    if (ER_OK != status) {
        QCC_LogError(status, ("GatherHostCandidates()"));
        goto exit;
    }

    // Gather server-reflexive (and relayed if requested) candidates,) by
    // creating a new thread.  We will be notified asynchronously upon completion.
    // This new thread observes proper pacing of STUN/TURN requests, and,
    // once candidates are gathered, performs keepalives until the session is ended.
    status = StartStunTurnPacingThread();
    if (ER_OK != status) {
        QCC_LogError(status, ("StartStunTurnPacingThread()"));
    }

exit:
    if (ER_OK != status) {
        EmptyICEStreamList();
    }

    return status;
}


QStatus ICESession::GetLocalICECandidates(list<ICECandidates>& iceCandidates, String& session_ufrag, String& session_pwd)
{
    QStatus status = ER_FAIL;

    if (ICEGatheringCandidates != GetState()) {
        iceCandidates = candidates;
        session_ufrag = ufrag;
        session_pwd = pwd;
        status = ER_OK;
    }

    return status;
}

void ICESession::EmptyICEStreamList(void)
{
    while (!streamList.empty()) {
        ICEStream* stream = streamList.back();
        delete stream;
        streamList.pop_back();
    }
}




QStatus ICESession::StartChecks(list<ICECandidates>& peerCandidates, bool useAggressiveNomination, String ice_frag, String ice_pwd)
{
    QCC_DbgTrace(("ICESession::StartChecks(...)"));
    QStatus status = ER_ICE_INVALID_STATE;

    lock.Lock();

    if (sessionState == ICECandidatesGathered) {

        isControllingAgent = true;
        this->useAggressiveNomination = useAggressiveNomination;

        status = StartChecks(peerCandidates, ice_frag, ice_pwd);
    }

    lock.Unlock();

    return status;
}


QStatus ICESession::StartChecks(list<ICECandidates>& peerCandidates, String ice_frag, String ice_pwd)
{
    QStatus status = ER_ICE_INVALID_STATE;

    lock.Lock();

    if (sessionState == ICECandidatesGathered) {

        // Nice-to-have: Verify peer is ICE-aware (For each remote media stream, the default
        // destination for each component of that stream appears in a candidate attribute.)

        status = FormCheckLists(peerCandidates, ice_frag, ice_pwd);
    } else {
        QCC_LogError(status, ("StartChecks called with bad ICESessionState=%d", sessionState));
    }

    lock.Unlock();

    return status;
}

// Optional: Section 9.
// Upon completion of checks, see if subsequent offer/answer needs to be made.
// If so, update candidates and notify.
//      Same ufrag, pwd, implementation level
//      Include only the candidate attribute matching the default destination
//      controlling agent sets a=remote-candidates attribute



QStatus ICESession::FormCheckLists(list<ICECandidates>& peerCandidates, String ice_frag, String ice_pwd)
{
    QCC_DbgTrace(("ICESession::FormCheckLists(%p, ice_frag=%s, ice_pwd=%s", this, ice_frag.c_str(), ice_pwd.c_str()));

    QStatus status = ER_OK;

    localInitiatedCheckUsername = ice_frag + ":" + ufrag;
    remoteInitiatedCheckUsername = ufrag + ":" + ice_frag;

    // Compute the short-term credential for checks.
    size_t hmacKeyLen = 0;
    StunCredential stunCredential(ice_pwd);

    // Size buffer first
    stunCredential.GetKey(NULL, hmacKeyLen);

    if (remoteShortTermHmacKey) {
        delete[] remoteShortTermHmacKey;
        remoteShortTermHmacKey = NULL;
    }

    remoteShortTermHmacKey = new uint8_t[hmacKeyLen];

    if (!remoteShortTermHmacKey) {
        status = ER_ICE_ALLOCATING_MEMORY;
        QCC_LogError(status, ("Allocating memory for remoteShortTermHmacKey"));
        return status;
    }

    remoteShortTermHmacKeyLength = ice_pwd.size();

    // Now get the real key ...
    stunCredential.GetKey(remoteShortTermHmacKey, hmacKeyLen);

    list<ICECandidate> localICEStreamCandidates;
    list<ICECandidate>::iterator localCandidatesIter;

    // Get our local candidates
    ComposeICEStreamCandidateList(localICEStreamCandidates);

    while (!peerCandidates.empty()) {
        SocketType remoteTransportProtocol = QCC_SOCK_DGRAM;
        uint32_t remotePriority = peerCandidates.front().priority;
        IPAddress remoteIPAddress(peerCandidates.front().address);
        IPEndpoint remoteEndPoint;
        remoteEndPoint.addr = remoteIPAddress;
        remoteEndPoint.port = peerCandidates.front().port;
        _ICECandidate::ICECandidateType remoteCandidateType = (_ICECandidate::ICECandidateType)peerCandidates.front().type;
        String remoteFoundation = peerCandidates.front().foundation;
        ComponentID remoteComponentID = peerCandidates.front().componentID;
        ICECandidate remoteCandidate;

        for (localCandidatesIter = localICEStreamCandidates.begin();
             localCandidatesIter != localICEStreamCandidates.end();
             localCandidatesIter++) {
            ICECandidate& localCandidate = *localCandidatesIter;
            if (localCandidate->GetComponent()->GetID() == remoteComponentID &&
                (localCandidate->GetEndpoint().addr.IsIPv4() == remoteIPAddress.IsIPv4() ||
                 localCandidate->GetEndpoint().addr.IsIPv6() == remoteIPAddress.IsIPv6())) {

                // Now that we know the local component, create the remote candidate object
                if (remoteCandidate->GetType() == _ICECandidate::Invalid_Candidate) {
                    Component* component = localCandidate->GetComponent();
                    remoteCandidate = ICECandidate(remoteCandidateType,
                                                   remoteEndPoint,
                                                   component,
                                                   remoteTransportProtocol,
                                                   remotePriority,
                                                   remoteFoundation);

                    if (remoteCandidateType == _ICECandidate::Relayed_Candidate) {
                        IPEndpoint remoteMappedEndpoint;
                        remoteMappedEndpoint.addr = peerCandidates.front().raddress;
                        remoteMappedEndpoint.port = peerCandidates.front().rport;
                        remoteCandidate->SetMappedAddress(remoteMappedEndpoint);
                    } else if (remoteCandidateType == _ICECandidate::ServerReflexive_Candidate) {
                        IPEndpoint remoteMappedEndpoint;
                        remoteMappedEndpoint.addr = peerCandidates.front().address;
                        remoteMappedEndpoint.port = peerCandidates.front().port;
                        remoteCandidate->SetMappedAddress(remoteMappedEndpoint);
                    }

                    streamList[0]->AddRemoteCandidate(remoteCandidate);
                }

                // Pair remote candidate with local
                // Identify default candidate pair
                bool isDefaultPair = localCandidate == localCandidate->GetComponent()->GetDefaultCandidate();

                // Compute pair priority
                uint64_t pairPriority = ComputePairPriority(isControllingAgent, localCandidate->GetPriority(), remotePriority);
                ICECandidatePair* pair = new ICECandidatePair(localCandidate, remoteCandidate, isDefaultPair, pairPriority);

                uint64_t controlTieBreaker = 0;
                Crypto_GetRandomBytes(reinterpret_cast<uint8_t*>(&controlTieBreaker), sizeof(controlTieBreaker));

                uint32_t bindRequestPriority = AssignPriority(localCandidate->GetComponent()->GetID(), localCandidate, _ICECandidate::PeerReflexive_Candidate);
                status = pair->InitChecker(controlTieBreaker, useAggressiveNomination, bindRequestPriority);
                if (ER_OK != status) {
                    delete pair;
                    goto exit;
                }

                localCandidate->GetComponent()->GetICEStream()->AddCandidatePair(pair);
            }
        }

        peerCandidates.pop_front();
    }

    // Sort each stream's pairs in decreasing order of priority
    SortAndPruneCandidatePairs();

    // Set permissions for the peer streams in the TURN server
    SetTurnPermissions();

    // Set the active check list for the first media stream
    status = StartInitialCheckList();

    if (ER_OK == status) {
        // Now that we have received the peer candidates, we can process received checks more fully.
        checksStarted = true;
    }

exit:
    return status;
}


void ICESession::EnqueueTurnCreatePermissions(ICECandidate& candidate)
{
    // For the TURN allocation associated with this local candidate,
    // compose one CreatePermission request for all remote candidates
    Stun* stun = candidate->GetStunActivity()->stun;
    Retransmit& retransmit = candidate->GetPermissionStunActivity()->retransmit;
    StunTransactionID tid;
    StunMessage* msg;

    if (!retransmit.GetTransactionID(tid)) {
        QCC_DbgPrintf(("!retransmit.GetTransactionID(tid) = %d (retransmit.GetState() == Retransmit::ReceivedAuthenticateResponse) = %d", !retransmit.GetTransactionID(tid), (retransmit.GetState() == Retransmit::ReceivedAuthenticateResponse)));
        // First attempt (or required to use different transaction)
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              STUN_MSG_CREATE_PERMISSION_METHOD,
                              stun->GetComponent()->GetHmacKey(),
                              stun->GetComponent()->GetHmacKeyLength());
        msg->GetTransactionID(tid);
        retransmit.SetTransactionID(tid);
    } else {
        // Retry attempt. Use same tid.
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              STUN_MSG_CREATE_PERMISSION_METHOD,
                              stun->GetComponent()->GetHmacKey(),
                              stun->GetComponent()->GetHmacKeyLength(),
                              tid);
    }

    // Walk remote candidate list
    ICEStream* stream = candidate->GetComponent()->GetICEStream();
    ICEStream::constRemoteListIterator iter;

    for (iter = stream->RemoteListBegin(); iter != stream->RemoteListEnd(); ++iter) {
        IPEndpoint peerEndpoint = (*iter)->GetEndpoint();
        msg->AddAttribute(new StunAttributeXorPeerAddress(*msg, peerEndpoint.addr, peerEndpoint.port));

        QCC_DbgPrintf(("Permission requested for addr = %s port = %d",
                       peerEndpoint.addr.ToString().c_str(), peerEndpoint.port));
    }

    msg->AddAttribute(new StunAttributeUsername(usernameForShortTermCredential));

    msg->AddAttribute(new StunAttributeMessageIntegrity(*msg));

    QCC_DbgPrintf(("Enqueueing CreatePermissions for addr = %s port = %d",
                   candidate->GetEndpoint().addr.ToString().c_str(), candidate->GetEndpoint().port));

    msg->AddAttribute(new StunAttributeFingerprint(*msg));

    // enqueue our request
    stunQueue.push_back(new StunWork(stun, msg, TurnServer));
}

void ICESession::SetTurnPermissions(void)
{
    stream_iterator streamIt;
    for (streamIt = Begin(); streamIt != End(); ++streamIt) {
        /* We should enque the TURN create permission only if we have something to check. i.e. the
         * checklist in the stream is non-empty */
        if (!((*streamIt)->CheckListEmpty())) {
            ICEStream::const_iterator componentIt;
            for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
                Component::iterator candidateIt;
                for (candidateIt = (*componentIt)->Begin(); candidateIt != (*componentIt)->End(); ++candidateIt) {
                    if ((*candidateIt)->GetType() == _ICECandidate::Relayed_Candidate) {
                        EnqueueTurnCreatePermissions(*candidateIt);
                    }
                }
            }
        }
    }
}

// Section 8.1.2 draft-ietf-mmusic-ice-19
void ICESession::UpdateICEStreamStates(void)
{
    bool allCheckListsAreCompleted = true;
    bool atLeastOneIsCompleted = false;
    bool allCheckListsAreFailed = true;
    bool atLeastOneIsRunning = false;

    stream_iterator streamIt;
    QCC_DbgTrace(("ICESession::UpdateICEStreamStates"));

    for (streamIt = Begin(); streamIt != End(); ++streamIt) {
        ICEStream::ICEStreamCheckListState checkListState = (*streamIt)->GetCheckListState();
        bool nominatedPairPerComponent = true;
        ICEStream::const_iterator componentIt;
        for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {

            bool hasNominatedPair = false;
            Component::const_iterator_validList validPairIt;
            uint64_t lowestPairPriority = -1; // 0xffffffffffffffff;
            for (validPairIt = (*componentIt)->BeginValidList(); validPairIt != (*componentIt)->EndValidList(); ++validPairIt) {
                if ((*validPairIt)->IsNominated() && (checkListState == ICEStream::CheckStateRunning)) {
                    (*streamIt)->RemoveWaitFrozenPairsForComponent((*componentIt));
                    lowestPairPriority = min(lowestPairPriority, (*validPairIt)->GetPriority());
                    hasNominatedPair = true;
                }
            }
            for (validPairIt = (*componentIt)->BeginValidList(); validPairIt != (*componentIt)->EndValidList(); ++validPairIt) {
                if ((*validPairIt)->IsNominated() && checkListState == ICEStream::CheckStateRunning) {
                    (*streamIt)->CeaseRetransmissions((*componentIt), lowestPairPriority);
                }
            }

            if (!hasNominatedPair) {
                nominatedPairPerComponent = false;
            }
        }

        if (nominatedPairPerComponent && checkListState == ICEStream::CheckStateRunning) {
            (*streamIt)->SetCheckListState(ICEStream::CheckStateCompleted);

            // Because we are not integrated with SIP (Section 8.3.1), we stop now.
            (*streamIt)->SetTerminate();

            // Section 9:  If haven't already, notify app that stream is ready
        }

        // Refresh our local variable
        checkListState = (*streamIt)->GetCheckListState();

        if (checkListState == ICEStream::CheckStateCompleted) {
            atLeastOneIsCompleted = true;
        } else {
            allCheckListsAreCompleted = false;
        }

        if (checkListState != ICEStream::CheckStateFailed) {
            allCheckListsAreFailed = false;
        }

        if (checkListState == ICEStream::CheckStateRunning) {
            atLeastOneIsRunning = true;
        }
    }

    if (allCheckListsAreCompleted) {
        QStatus status = ER_OK;

        // Notify checking threads to terminate
        for (streamIt = Begin(); streamIt != End(); ++streamIt) {
            (*streamIt)->SetTerminate();

            // Prepare selected list
            selectedCandidatePairList.clear();

            ICEStream::const_iterator componentIt;
            for (componentIt = (*streamIt)->Begin(); componentIt != (*streamIt)->End(); ++componentIt) {
                ICECandidatePair* selectedPair = NULL;
                status = (*componentIt)->GetSelectedCandidatePair(selectedPair);
                if (ER_OK == status) {
                    selectedCandidatePairList.push_back(selectedPair);

                    // Prepare for media on the local stun object. Configure
                    // the stun object (previously potentially shared by candidates).
                    bool usingTurn = (selectedPair->local->GetType() == _ICECandidate::Relayed_Candidate);
                    selectedPair->local->GetStunActivity()->stun->Connect(
                        selectedPair->remote->GetEndpoint().addr,
                        selectedPair->remote->GetEndpoint().port,
                        usingTurn);
                } else {
                    QCC_LogError(status, ("GetSelectedCandidatePair failed."));
                    break;
                }
            }
        }

        sessionState = (ER_OK == status) ? ICEChecksSucceeded : ICEProcessingFailed;

        // Notify application (only once)
        if (ICEProcessingFailed == sessionState) {
            NotifyListenerIfNeeded();
        } else {
            // Success.

            NotifyListenerIfNeeded();
        }

    } else if (allCheckListsAreFailed) {
        // Notify checking threads to terminate
        for (streamIt = Begin(); streamIt != End(); ++streamIt) {
            (*streamIt)->SetTerminate();
        }

        sessionState = ICEProcessingFailed;

        // Notify listener
        NotifyListenerIfNeeded();
    } else {   //look for failed lists
        bool atLeastOneFailed = false;
        for (streamIt = Begin(); streamIt != End(); ++streamIt) {
            if ((*streamIt)->GetCheckListState() == ICEStream::CheckStateFailed) {
                (*streamIt)->SetTerminate();
                atLeastOneFailed = true;
                if (atLeastOneIsCompleted) {
                    // Section 9: If/when we implement the updated offer/answer exchange
                    // we should notify listener of _this_ stream failure,
                    // remove stream from session in an updated offer,
                    // and continue checks for remaining streams.
                } else if (!atLeastOneIsRunning) {
                    // No streams are completed, none are running. Time to quit.

                    sessionState = ICEProcessingFailed;

                    // Notify listener
                    NotifyListenerIfNeeded();

                    // Per the Section 9 comment, cancel checks for all other streams.
                }
            }
        }
        // In lieu of Section 9 implementation, we consider one failed stream to
        // be a failure for all.
        if (atLeastOneFailed) {
            for (streamIt = Begin(); streamIt != End(); ++streamIt) {
                (*streamIt)->SetTerminate();
            }
        }
    }
}


void ICESession::NotifyListenerIfNeeded(void)
{
    if (!listenerNotifiedOnSuccessOrFailure) {
        // Notify listener if haven't already
        sessionListener->ICESessionChanged(this);

        listenerNotifiedOnSuccessOrFailure = true;
    }
}



void ICESession::SwapControllingAgent(void)
{
    isControllingAgent = !isControllingAgent;

    //Todo recompute priorities 5.7.2
}


uint64_t ICESession::ComputePairPriority(bool isControllingAgent, uint32_t localPriority, uint32_t remotePriority)
{
    uint64_t priority_G = (isControllingAgent ? localPriority : remotePriority);
    uint64_t priority_D = (isControllingAgent ? remotePriority : localPriority);

    return (min(priority_G, priority_D) << 32) + 2 * max(priority_G, priority_D) +
           (priority_G > priority_D ? 1 : 0);
}

/**
 * Cancel ICE Connection checks. (Called by user app, ergo locking.)
 *
 * Cancel any in-progress connection checks for a session.
 * Note: this will NOT stop any candidate gathering, should
 * it be called during that phase.
 *
 * @return   ER_OK if checks were running, ER_FAIL otherwise.
 */
QStatus ICESession::CancelChecks()
{
    QStatus status = ER_FAIL;

    lock.Lock();
    if (ICEChecksRunning == sessionState) {
        // Notify listening threads to terminate
        stream_iterator streamIter;
        for (streamIter = Begin(); streamIter != End(); ++streamIter) {
            (*streamIter)->CancelChecks();
        }

        // revert to state before checking, in case we want to
        // restart checks.
        sessionState = ICECandidatesGathered;
        status = ER_OK;
    }
    lock.Unlock();

    return status;
}


ICESession::ICESessionState ICESession::GetState(void)
{
    ICESessionState state;

    state = sessionState;

    return state;
}

void ICESession::SetState(ICESessionState state)
{
    sessionState = state;
}

QStatus ICESession::GetErrorCode(void)
{
    QStatus code;

    code = errorCode;

    return code;
}

void ICESession::SetErrorCode(QStatus code)
{
    errorCode = code;
}



void ICESession::GetSelectedCandidatePairList(vector<ICECandidatePair*>& selectedCandidatePairList)
{
    selectedCandidatePairList = this->selectedCandidatePairList;


    // walk list and display
    vector<ICECandidatePair*>::iterator iter;
    for (iter = selectedCandidatePairList.begin(); iter != selectedCandidatePairList.end(); ++iter) {
        QCC_DbgPrintf(("SelectedPair: local %s:%d (%s) remote %s:%d (%s)",
                       (*iter)->local->GetEndpoint().addr.ToString().c_str(),
                       (*iter)->local->GetEndpoint().port,
                       (*iter)->local->GetTypeString().c_str(),
                       (*iter)->remote->GetEndpoint().addr.ToString().c_str(),
                       (*iter)->remote->GetEndpoint().port,
                       (*iter)->remote->GetTypeString().c_str()));
    }

}

uint16_t ICESession::GetActiveCheckListCount(void)
{
    uint16_t count = 0;

    stream_iterator streamIter;
    for (streamIter = Begin(); streamIter != End(); ++streamIter) {
        if ((*streamIter)->CheckListIsActive()) {
            ++count;
        }
    }

    return count;
}


uint8_t* ICESession::GetRemoteInitiatedCheckHmacKey(void) const
{
    QCC_DbgPrintf(("ICESession::GetRemoteInitiatedCheckHmacKey(): %s", (char*)shortTermHmacKey));
    return shortTermHmacKey; // based on local password
}

size_t ICESession::GetRemoteInitiatedCheckHmacKeyLength(void) const
{
    QCC_DbgPrintf(("ICESession::GetRemoteInitiatedCheckHmacKeyLength(): %d", shortTermHmacKeyLength));
    return shortTermHmacKeyLength;     // based on local password
}


uint8_t* ICESession::GetLocalInitiatedCheckHmacKey(void) const
{
    QCC_DbgPrintf(("ICESession::GetLocalInitiatedCheckHmacKey(): %s", (char*)remoteShortTermHmacKey));
    return remoteShortTermHmacKey; // based on remote password
}

size_t ICESession::GetLocalInitiatedCheckHmacKeyLength(void) const
{
    QCC_DbgPrintf(("ICESession::GetLocalInitiatedCheckHmacKeyLength(): %d", remoteShortTermHmacKeyLength));
    return remoteShortTermHmacKeyLength;     // based on local password
}

} //namespace ajn
