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

#include <string>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/SecurityApplicationProxy.h>

#include "TestSecureApplication.h"

#include <gtest/gtest.h>
#include "ajTestCommon.h"

#define QCC_MODULE "SECURITY_TEST"

using namespace std;
using namespace ajn;
using namespace qcc;

/*
 * Wait to be lenient toward slow tests running on VMs, built in
 * Debug mode, with App Verifier enabled, using an out-of-proc router node, while
 * the VM host machine is busy compiling from multiple threads, etc.
 */
#define SLOW_TEST_RESPONSE_TIMEOUT (10000 * s_globalTimerMultiplier)

QStatus TestSecureApplication::Init(TestSecurityManager& tsm)
{
    QStatus status = bus.Start();
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed, line = " << __LINE__ << ", status = " << status << endl;
        return status;
    }
    status = bus.Connect();
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed, line = " << __LINE__ << ", status = " << status << endl;
        return status;
    }

    QCC_DbgHLPrintf(("%s: app name: '%s' bus name = '%s'", __FUNCTION__, appName.c_str(), bus.GetUniqueName().c_str()));

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_NULL", &authListener, NULL, true, &pcl);
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed, line = " << __LINE__ << ", status = " << status << endl;
        return status;
    }

    PermissionPolicy::Acl mnf;
    PermissionPolicy::Rule::Member member;
    member.SetMemberName("*");
    member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    member.SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY
                         | PermissionPolicy::Rule::Member::ACTION_OBSERVE
                         | PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    PermissionPolicy::Rule rules;

    rules.SetInterfaceName(TEST_INTERFACE);

    rules.SetMembers(1, &member);
    mnf.SetRules(1, &rules);

    status = tsm.Claim(bus, mnf);
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed, line = " << __LINE__ << ", status = " << status << endl;
        return status;
    }

    InterfaceDescription* testIntf = nullptr;
    status = bus.CreateInterface(TEST_INTERFACE, testIntf, AJ_IFC_SECURITY_REQUIRED);
    if (ER_OK == status) {
        testIntf->AddMethod(TEST_METHOD_NAME, "b", "b", "success,echosuccess");
        testIntf->AddSignal(TEST_SIGNAL_NAME, "b", "state", 0);
        testIntf->AddProperty(TEST_PROP_NAME, "b", PROP_ACCESS_RW);
        testIntf->AddPropertyAnnotation(TEST_PROP_NAME, org::freedesktop::DBus::AnnotateEmitsChanged, "true");
        testIntf->AddProperty(TEST_PROP_NAME2, "b", PROP_ACCESS_RW);
        testIntf->AddPropertyAnnotation(TEST_PROP_NAME2, org::freedesktop::DBus::AnnotateEmitsChanged, "true");
        testIntf->Activate();
    } else {
        cerr << "SecureApplication::Init failed, line = " << __LINE__ << ", status = " << status << endl;
        return status;
    }
    return status;
}

QStatus TestSecureApplication::HostSession(SessionPort port, bool multipoint)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, multipoint, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = bus.BindSessionPort(port, opts, *this);

    if (ER_OK != status) {
        return status;
    }

    testObj = new TestObject(bus);
    status = testObj->Init();

    return status;
}

QStatus TestSecureApplication::JoinSession(TestSecureApplication& sessionHost, SessionId& sessionId, SessionPort port, bool multipoint)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, multipoint, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    /* sessionHost sends the JoinSession reply message BEFORE it updates its sessions map! Wait until sessionHost finished these updates */
    qcc::Event sessionHostReady;
    sessionHost.SetJoinSessionEvent(&sessionHostReady);
    QStatus status = bus.JoinSession(sessionHost.bus.GetUniqueName().c_str(), port, this, sessionId, opts);
    if (ER_OK == status) {
        sessionHost.WaitJoinSessionEvent(SLOW_TEST_RESPONSE_TIMEOUT);
        sessionHost.RemoveJoinSessionEvent();

        sessionLock.Lock();
        joinedSessions.push_back(sessionId);
        sessionLock.Unlock();
    }
    return status;
}

TestSecureApplication::~TestSecureApplication()
{
    sessionLock.Lock();
    while (joinedSessions.size() > 0) {
        SessionId sid = joinedSessions[0];
        sessionLock.Unlock();
        bus.LeaveJoinedSession(sid);
        sessionLock.Lock();
        SessionLost(sid, SessionListener::ALLJOYN_SESSIONLOST_REASON_OTHER);
    }
    while (hostedSessions.size() > 0) {
        SessionId sid = hostedSessions[0];
        sessionLock.Unlock();
        bus.LeaveHostedSession(sid);
        sessionLock.Lock();
        if (hostedSessions.size() > 0) {
            hostedSessions.erase(hostedSessions.begin());
        }
    }
    sessionLock.Unlock();
    if (testObj) {
        delete testObj;
        testObj = NULL;
    }
    bus.EnablePeerSecurity("", NULL);
    bus.Disconnect();
    bus.Stop();
    bus.Join();
    bus.ClearKeyStore();
}

void TestSecureApplication::SessionLost(SessionId sessionId,
                                        SessionLostReason reason)
{
    QCC_UNUSED(reason);
    sessionLock.Lock();
    auto it = joinedSessions.begin();
    for (; it != joinedSessions.end(); it++) {
        if (*it == sessionId) {
            joinedSessions.erase(it);
            break;
        }
    }

    sessionLock.Unlock();
}

bool TestSecureApplication::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(joiner);
    QCC_UNUSED(opts);
    return true;
}

void TestSecureApplication::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(joiner);
    sessionLock.Lock();
    hostedSessions.push_back(id);
    sessionLock.Unlock();

    QCC_DbgHLPrintf(("%s: app name = %s, bus name = %s, joiner = %s", __FUNCTION__, appName.c_str(), bus.GetUniqueName().c_str(), joiner));

    if (joinSessionEvent != nullptr) {
        QCC_DbgHLPrintf(("%s: Setting event @ %p", __FUNCTION__, joinSessionEvent));
        EXPECT_EQ(ER_OK, joinSessionEvent->SetEvent());
    }
}

ProxyBusObject* TestSecureApplication::GetProxyObject(TestSecureApplication& host, SessionId sessionid, const char* objPath)
{
    const InterfaceDescription* remoteIntf = bus.GetInterface(TEST_INTERFACE);
    if (nullptr == remoteIntf) {
        return NULL;
    }

    ProxyBusObject* proxy = new ProxyBusObject(bus, host.bus.GetUniqueName().c_str(),
                                               objPath, sessionid);

    proxy->AddInterface(*remoteIntf);
    return proxy;
}

QStatus TestSecureApplication::SetAnyTrustedUserPolicy(TestSecurityManager& tsm, uint8_t actionMask, const char* interfaceName)
{
    PermissionPolicy policy;
    PermissionPolicy::Acl acl;
    PermissionPolicy::Rule::Member member;
    member.SetMemberName("*");
    member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    member.SetActionMask(actionMask);
    PermissionPolicy::Rule rules;

    rules.SetInterfaceName(interfaceName);

    rules.SetMembers(1, &member);
    acl.SetRules(1, &rules);
    PermissionPolicy::Peer peer;
    peer.SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acl.SetPeers(1, &peer);
    policy.SetAcls(1, &acl);

    /* Wait for EndManagement after receiving a new policy, to make sure all settings are consistent before continuing */
    qcc::Event endManagement;
    SetEndManagementEvent(&endManagement);
    QStatus status = tsm.UpdatePolicy(bus, policy);
    if (status == ER_OK) {
        WaitEndManagementEvent(SLOW_TEST_RESPONSE_TIMEOUT);
    }
    RemoveEndManagementEvent();
    return status;
}

QStatus TestSecureApplication::SetPolicy(TestSecurityManager& tsm, PermissionPolicy& newPolicy)
{
    return tsm.UpdatePolicy(bus, newPolicy);
}

QStatus TestSecureApplication::UpdateManifest(TestSecurityManager& tsm, uint8_t actionMask, const char* interfaceName)
{
    PermissionPolicy::Acl acl;
    PermissionPolicy::Rule::Member member;
    member.SetMemberName("*");
    member.SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    member.SetActionMask(actionMask);
    PermissionPolicy::Rule rules;

    rules.SetInterfaceName(interfaceName);

    rules.SetMembers(1, &member);
    acl.SetRules(1, &rules);
    PermissionPolicy::Peer peer;
    peer.SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acl.SetPeers(1, &peer);
    return tsm.UpdateIdentity(bus, acl);
}

QStatus TestSecureApplication::UpdateManifest(TestSecurityManager& tsm, const PermissionPolicy::Acl& manifest)
{
    return tsm.UpdateIdentity(bus, manifest);
}

QStatus TestSecureApplication::UpdateTestProperty(bool newState)
{
    if (testObj == NULL) {
        return ER_BUS_NO_SUCH_OBJECT;
    }
    MsgArg val;
    testObj->state = newState;
    val.Set("b", newState);
    testObj->EmitPropChanged(TEST_INTERFACE, TEST_PROP_NAME, val, SESSION_ID_ALL_HOSTED, 0);
    return ER_OK;
}

QStatus TestSecureApplication::SendSignal(bool value)
{
    if (testObj == NULL) {
        return ER_BUS_NO_SUCH_OBJECT;
    }

    MsgArg outArg;
    outArg.Set("b", value);

    /*
     * Keep displaying these messages until SecSignalTest proves to be reliable.
     * When/if SecSignalTest hangs, the source code line of the SecSignalTest
     * hang can be deducted by counting the previous "Sending signal..." messages.
     * Also, the unique name displayed here can be helpful for investigating the hang.
     */
    cout << "Sending signal value = " << value << " from '" << bus.GetUniqueName() << "' to SESSION_ID_ALL_HOSTED" << endl;
    return testObj->Signal(NULL, SESSION_ID_ALL_HOSTED, *bus.GetInterface(TEST_INTERFACE)->GetMember(TEST_SIGNAL_NAME), &outArg, 1, 0, 0,  NULL);
}

QStatus TestSecureApplication::SendSignal(bool value, TestSecureApplication& destination)
{
    if (testObj == NULL) {
        return ER_BUS_NO_SUCH_OBJECT;
    }

    SessionId sid = 0;
    sessionLock.Lock();
    if (hostedSessions.size() != 1) {
        sessionLock.Unlock();
        return ER_BUS_BAD_SENDER_ID;
    }
    sid = hostedSessions[0];
    sessionLock.Unlock();

    MsgArg outArg;
    outArg.Set("b", value);

    /*
     * Keep displaying these messages until SecSignalTest proves to be reliable.
     * When/if SecSignalTest hangs, the source code line of the SecSignalTest
     * hang can be deducted by counting the previous "Sending signal..." messages.
     * Also, the unique name displayed here can be helpful for investigating the hang.
     */
    cout << "Sending signal value = " << value << " from '" << bus.GetUniqueName() << "' to '" << destination.bus.GetUniqueName().c_str() << "' on session " << sid << endl;
    return testObj->Signal(destination.bus.GetUniqueName().c_str(), sid, *bus.GetInterface(TEST_INTERFACE)->GetMember(TEST_SIGNAL_NAME), &outArg, 1, 0, 0,  NULL);
}

void TestSecureApplication::DeleteAllAuthenticationEvents()
{
    authEvents.clear();
}

void TestSecureApplication::AddAuthenticationEvent(const qcc::String& peerName, Event* authEvent)
{
    EXPECT_EQ(ER_OK, authEvent->ResetEvent());
    authEvents.insert(std::pair<qcc::String, qcc::Event*>(peerName, authEvent));
}

QStatus TestSecureApplication::WaitAllAuthenticationEvents(uint32_t timeout)
{
    QStatus status = ER_OK;

    for (std::map<qcc::String, qcc::Event*>::iterator it = authEvents.begin(); it != authEvents.end(); it++) {
        QStatus localStatus;
        QCC_DbgHLPrintf(("%s: Waiting for event @ %p", __FUNCTION__, it->second));
        EXPECT_EQ(ER_OK, (localStatus = Event::Wait(*(it->second), timeout)));

        if (localStatus != ER_OK) {
            status = localStatus;
        }
    }

    return status;
}

void TestSecureApplication::AuthenticationCompleteCallback(qcc::String peerName, bool success)
{
    if (success) {
        QCC_DbgHLPrintf(("%s: app name = '%s', bus name = '%s', peer bus name = '%s' - success.",
                         __FUNCTION__, appName.c_str(), bus.GetUniqueName().c_str(), peerName.c_str()));
    } else {
        QCC_LogError(ER_FAIL, ("%s: app name = '%s', bus name = '%s', peer bus name = '%s' - failed",
                               __FUNCTION__, appName.c_str(), bus.GetUniqueName().c_str(), peerName.c_str()));
    }

    if (!authEvents.empty()) {
        std::map<qcc::String, qcc::Event*>::iterator it = authEvents.find(peerName);
        EXPECT_NE(it, authEvents.end());

        if (it != authEvents.end()) {
            QCC_DbgHLPrintf(("%s: Setting event @ %p", __FUNCTION__, it->second));
            EXPECT_EQ(ER_OK, it->second->SetEvent());
        }
    }
}

void TestSecureApplication::RemoveJoinSessionEvent()
{
    joinSessionEvent = nullptr;
}

void TestSecureApplication::SetJoinSessionEvent(qcc::Event* event)
{
    joinSessionEvent = event;
    EXPECT_EQ(ER_OK, joinSessionEvent->ResetEvent());
}

QStatus TestSecureApplication::WaitJoinSessionEvent(uint32_t timeout)
{
    QStatus status;
    QCC_DbgHLPrintf(("%s: Waiting for event @ %p", __FUNCTION__, joinSessionEvent));
    EXPECT_EQ(ER_OK, (status = Event::Wait(*joinSessionEvent, timeout)));
    return status;
}

void TestSecureApplication::RemoveEndManagementEvent()
{
    endManagementEvent = nullptr;
}

void TestSecureApplication::SetEndManagementEvent(qcc::Event* event)
{
    endManagementEvent = event;
    EXPECT_EQ(ER_OK, endManagementEvent->ResetEvent());
}

QStatus TestSecureApplication::WaitEndManagementEvent(uint32_t timeout)
{
    QStatus status = ER_OK;

    QCC_DbgHLPrintf(("%s: Waiting for event @ %p", __FUNCTION__, endManagementEvent));
    EXPECT_EQ(ER_OK, (status = Event::Wait(*endManagementEvent, timeout)));
    return status;
}

void TestSecureApplication::EndManagementCompleteCallback()
{
    QCC_DbgHLPrintf(("%s: app name = '%s', bus name = '%s'", __FUNCTION__, appName.c_str(), bus.GetUniqueName().c_str()));

    if (endManagementEvent != nullptr) {
        QCC_DbgHLPrintf(("%s: Setting event @ %p", __FUNCTION__, endManagementEvent));
        EXPECT_EQ(ER_OK, endManagementEvent->SetEvent());
    }
}

QStatus TestSecureApplication::SecureConnection(TestSecureApplication& peer, bool forceAuth)
{
    QStatus status = ER_OK;
    Event myAuthComplete;
    Event peerAuthComplete;

    /* Wait until authentication finished on both sides */
    AddAuthenticationEvent(peer.GetBusAttachement().GetUniqueName(), &myAuthComplete);
    peer.AddAuthenticationEvent(GetBusAttachement().GetUniqueName(), &peerAuthComplete);

    EXPECT_EQ(ER_OK, (status = bus.SecureConnection(peer.GetBusAttachement().GetUniqueName().c_str(), forceAuth)));

    if (status == ER_OK) {
        WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
        peer.WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
    }

    DeleteAllAuthenticationEvents();
    peer.DeleteAllAuthenticationEvents();
    return status;
}

QStatus TestSecureApplication::SecureConnectionAllSessionsCommon(bool async, TestSecureApplication& peer, bool forceAuth)
{
    QStatus status = ER_OK;
    Event myAuthComplete;
    Event peerAuthComplete;

    /*
     * The caller specified that "peer" has one or more active sessions with this
     * application/bus attachment.  Initiate authentication for all sessions, then
     * wait until authentication finished on both sides.
     */
    AddAuthenticationEvent(peer.GetBusAttachement().GetUniqueName(), &myAuthComplete);
    peer.AddAuthenticationEvent(GetBusAttachement().GetUniqueName(), &peerAuthComplete);

    if (async) {
        EXPECT_EQ(ER_OK, (status = bus.SecureConnectionAsync(nullptr, forceAuth)));
    } else {
        EXPECT_EQ(ER_OK, (status = bus.SecureConnection(nullptr, forceAuth)));
    }

    if (status == ER_OK) {
        WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
        peer.WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
    }

    DeleteAllAuthenticationEvents();
    peer.DeleteAllAuthenticationEvents();
    return status;
}

QStatus TestSecureApplication::SecureConnectionAllSessions(TestSecureApplication& peer, bool forceAuth)
{
    return SecureConnectionAllSessionsCommon(false, peer, forceAuth);
}

QStatus TestSecureApplication::SecureConnectionAllSessionsAsync(TestSecureApplication& peer, bool forceAuth)
{
    return SecureConnectionAllSessionsCommon(true, peer, forceAuth);
}

QStatus TestSecureApplication::SecureConnectionAllSessionsCommon(bool async, TestSecureApplication& peer1, TestSecureApplication& peer2, bool forceAuth)
{
    QStatus status = ER_OK;
    Event myAuthComplete1;
    Event myAuthComplete2;
    Event peer1AuthComplete;
    Event peer2AuthComplete;

    /*
     * The caller specified that peer1 and peer2 are currently connected to sessions
     * hosted by this application/bus attachment.  Initiate authentication for all
     * sessions, then wait until authentication finished on all sides of the sessions.
     */
    AddAuthenticationEvent(peer1.GetBusAttachement().GetUniqueName(), &myAuthComplete1);
    AddAuthenticationEvent(peer2.GetBusAttachement().GetUniqueName(), &myAuthComplete2);
    peer1.AddAuthenticationEvent(GetBusAttachement().GetUniqueName(), &peer1AuthComplete);
    peer2.AddAuthenticationEvent(GetBusAttachement().GetUniqueName(), &peer2AuthComplete);

    if (async) {
        EXPECT_EQ(ER_OK, (status = bus.SecureConnectionAsync(nullptr, forceAuth)));
    } else {
        EXPECT_EQ(ER_OK, (status = bus.SecureConnection(nullptr, forceAuth)));
    }

    if (status == ER_OK) {
        WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
        peer1.WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
        peer2.WaitAllAuthenticationEvents(SLOW_TEST_RESPONSE_TIMEOUT);
    }

    DeleteAllAuthenticationEvents();
    peer1.DeleteAllAuthenticationEvents();
    peer2.DeleteAllAuthenticationEvents();
    return status;
}

QStatus TestSecureApplication::SecureConnectionAllSessions(TestSecureApplication& peer1, TestSecureApplication& peer2, bool forceAuth)
{
    return SecureConnectionAllSessionsCommon(false, peer1, peer2, forceAuth);
}

QStatus TestSecureApplication::SecureConnectionAllSessionsAsync(TestSecureApplication& peer1, TestSecureApplication& peer2, bool forceAuth)
{
    return SecureConnectionAllSessionsCommon(true, peer1, peer2, forceAuth);
}

void TestSecureApplication::MyAuthListener::AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
{
    DefaultECDHEAuthListener::AuthenticationComplete(authMechanism, peerName, success);

    if (!success) {
        cerr << __FUNCTION__ << " auth failed" << endl;
    }

    if (m_app != nullptr) {
        m_app->AuthenticationCompleteCallback(peerName, success);
    }
}
