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

using namespace std;
using namespace ajn;
using namespace qcc;

QStatus TestSecureApplication::Init(TestSecurityManager& tsm)
{
    QStatus status = bus.Start();
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed " << __LINE__ << endl;
        return status;
    }
    status = bus.Connect();
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed " << __LINE__ << endl;
        return status;
    }

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_NULL", &authListener, NULL, true);
    if (ER_OK != status) {
        cerr << "SecureApplication::Init failed " << __LINE__ << endl;
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
        cerr << "SecureApplication::Init failed " << __LINE__ << endl;
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
        cerr << "SecureApplication::Init failed " << __LINE__ << endl;
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
    QStatus status = bus.JoinSession(sessionHost.bus.GetUniqueName().c_str(), port, this, sessionId, opts);
    if (ER_OK == status) {
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
    return tsm.UpdatePolicy(bus, policy);
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
    cout << "Sending signal(" << value << ") from '" << bus.GetUniqueName() << "' to 'SESSION_ID_ALL_HOSTED' on session " << endl;
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
    cout << "Sending signal(" << value << ") from '" << bus.GetUniqueName() << "' to '" << destination.bus.GetUniqueName().c_str() << "' on session " << sid << endl;
    return testObj->Signal(destination.bus.GetUniqueName().c_str(), sid, *bus.GetInterface(TEST_INTERFACE)->GetMember(TEST_SIGNAL_NAME), &outArg, 1, 0, 0,  NULL);
}
