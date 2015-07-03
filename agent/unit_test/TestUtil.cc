/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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
#include "TestUtil.h"

#include <stdlib.h>

#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/Environ.h>

#include <alljoyn/securitymgr/Util.h>

using namespace ajn;
using namespace ajn::securitymgr;
using namespace std;

#define STORAGE_DEFAULT_PATH "/tmp/secmgr.db"
#define STORAGE_DEFAULT_PATH_KEY "STORAGE_PATH"

namespace secmgrcoretest_unit_testutil {
TestApplicationListener::TestApplicationListener(Condition& _sem,
                                                 Mutex& _lock) :
    sem(_sem), lock(_lock)
{
}

void TestApplicationListener::OnApplicationStateChange(const OnlineApplication* old,
                                                       const OnlineApplication* updated)
{
    const OnlineApplication* info = updated ? updated : old;
    cout << "TAL>> Old Application info = " << (old == nullptr ? "null" : old->ToString().c_str()) << endl;
    cout << "TAL>> New Application info = " << (updated == nullptr ? "null" : updated->ToString().c_str()) << endl;
    lock.Lock();
    events.push_back(*info);
    sem.Broadcast();
    lock.Unlock();
}

BasicTest::BasicTest() :
    tal(nullptr), stub(nullptr),
    secMgr(nullptr), ba(nullptr), storage(nullptr), ca(nullptr), pg(nullptr),
    proxyObjectManager(nullptr)
{
}

void BasicTest::SetUp()
{
    string storage_path;

    storage_path = Environ::GetAppEnviron()->Find(STORAGE_DEFAULT_PATH_KEY, STORAGE_DEFAULT_PATH).c_str();
    Environ::GetAppEnviron()->Add(STORAGE_DEFAULT_PATH_KEY, STORAGE_DEFAULT_PATH);

    remove(storage_path.c_str());
    // clean up any lingering stub keystore
    string fname = GetHomeDir().c_str();
    fname.append("/.alljoyn_keystore/stub.ks");
    remove(fname.c_str());

    SecurityAgentFactory& secFac = SecurityAgentFactory::GetInstance();
    StorageFactory& storageFac = StorageFactory::GetInstance();

    ba = new BusAttachment("test", true);
    ASSERT_TRUE(ba != nullptr);
    ASSERT_EQ(ER_OK, ba->Start());
    ASSERT_EQ(ER_OK, ba->Connect());

    ba->RegisterAboutListener(testAboutListener);

    /* Passing nullptr into WhoImplements will listen for all About announcements */

    if (ER_OK != ba->WhoImplements(nullptr)) {
        printf("WhoImplements nullptr failed.\n");
    }

    ASSERT_EQ(ER_OK, storageFac.GetStorage("test", storage));
    ASSERT_EQ(ER_OK, storage->GetCaStorage(ca));
    ASSERT_EQ(ER_OK, secFac.GetSecurityAgent(GetAgentCAStorage(), secMgr, ba));

    secMgr->SetManifestListener(&aa);

    tal = new TestApplicationListener(sem, lock);
    secMgr->RegisterApplicationListener(tal);

    GroupInfo adminGroup;
    storage->GetAdminGroup(adminGroup);
    pg = new PolicyGenerator(adminGroup);

    proxyObjectManager = new ProxyObjectManager(ba);
}

void BasicTest::UpdateLastAppInfo()
{
    lock.Lock();
    if (tal->events.size()) {
        vector<OnlineApplication>::iterator it = tal->events.begin();
        lastAppInfo = *it;
        tal->events.erase(it);
    }
    lock.Unlock();
}

bool BasicTest::WaitForState(PermissionConfigurator::ApplicationState newState,
                             const bool hasBusName, const int updatesPending)
{
    lock.Lock();
    printf("\nWaitForState: waiting for event(s) ...\n");
    //Prior to entering this function, the test should have taken an action which leads to one or more events.
    //These events are handled in a separate thread.
    do {
        if (tal && tal->events.size()) { //if no event is in the queue, we will return immediately
            UpdateLastAppInfo(); //update latest value.
            printf("WaitForState: Checking event ... ");
            if (lastAppInfo.applicationState == newState && ((hasBusName == !lastAppInfo.busName.empty())) &&
                ((updatesPending == -1 ? 1 : ((bool)updatesPending == lastAppInfo.updatesPending)))) {
                printf("ok\n");
                lock.Unlock();
                return true;
            }
            printf("not ok, waiting/checking for next event\n");
        } else {
            QStatus status = sem.TimedWait(lock, 10000);
            if (ER_OK != status) {
                printf("timeout- failing test - %i\n", status);
                break;
            }
            assert(tal->events.size()); // assume TimedWait returns != ER_OK in case of timeout
        }
    } while (true);
    printf("WaitForState failed.\n");
    printf("\tClaimableState: expected = %s, got %s\n", PermissionConfigurator::ToString(
               newState), PermissionConfigurator::ToString(lastAppInfo.applicationState));
    printf("\tHas BusName: expected = %s, got %s\n", (hasBusName ? "YES" : "NO"),
           (lastAppInfo.busName.empty() ? "NO" : "YES"));
    printf("\t Busname lastAppInfo.busName (%s)\n", lastAppInfo.busName.c_str());
    if (updatesPending != -1) {
        printf("\tUpdatesPending : expected = %s, got %s\n", (updatesPending ? "True" : "False"),
               (lastAppInfo.updatesPending ? "True" : "False"));
    }

    lock.Unlock();
    return false;
}

bool BasicTest::CheckRemotePolicy(PermissionPolicy& expected)
{
    bool result = true;

    printf("Checking remote policy ... ");
    PermissionPolicy remote;
    QStatus status = proxyObjectManager->GetPolicy(lastAppInfo, remote);
    if (ER_OK != status) {
        printf("failed to GetPolicy\n");
        return false;
    }

    uint32_t expectedVersion = expected.GetVersion();
    uint32_t remoteVersion = remote.GetVersion();
    if (expectedVersion != remoteVersion) {
        printf("mismatching version: expected %i, got %i\n", expectedVersion, remoteVersion);
        return false;
    }

    size_t expectedAclsSize = expected.GetAclsSize();
    size_t remoteAclsSize = remote.GetAclsSize();
    if (expectedAclsSize != remoteAclsSize) {
        printf("mismatching aclsSize: expected %lu, got %lu\n",
               (unsigned long)expectedAclsSize,
               (unsigned long)remoteAclsSize);
        return false;
    }

    if (expected != remote) {
        printf("mismatching remote policy: expected %s, got %s\n",
               expected.ToString().c_str(),
               remote.ToString().c_str());
        return false;
    }

    PermissionPolicy stored;
    ca->GetPolicy(lastAppInfo, stored);
    if (expected != stored) {
        printf("mismatching stored policy: expected %s, got %s\n",
               expected.ToString().c_str(),
               stored.ToString().c_str());
        return false;
    }

    if (result) {
        printf("ok\n");
    }

    return result;
}

bool BasicTest::CheckRemoteIdentity(IdentityInfo& expected,
                                    Manifest& expectedManifest)
{
    printf("Checking remote identity ... ");
    QStatus status = ER_OK;

    IdentityCertificate remote;
    status = proxyObjectManager->GetIdentity(lastAppInfo, &remote);
    if (ER_OK != status) {
        printf("failed to GetIdentity\n");
        return false;
    }

    string expectedAlias = expected.guid.ToString().c_str();
    string remoteAlias = remote.GetAlias().c_str();
    if (expectedAlias != remoteAlias) {
        printf("mismatching alias: expected %s, got %s\n", expectedAlias.c_str(), remoteAlias.c_str());
        return false;
    }

    IdentityCertificateChain storedIdCerts;
    Manifest storedManifest;
    status = ca->GetIdentityCertificatesAndManifest(lastAppInfo, storedIdCerts, storedManifest);
    if (ER_OK != status) {
        printf("failed to GetIdentityCertificateAndManifest\n");
        return false;
    }

    String storedDer;
    status = storedIdCerts[0].EncodeCertificateDER(storedDer);
    if (ER_OK != status) {
        printf("failed to encode stored certificate\n");
        return false;
    }

    String remoteDer;
    status = remote.EncodeCertificateDER(remoteDer);
    if (ER_OK != status) {
        printf("failed to encode remote certificate\n");
        return false;
    }

    if (storedDer != remoteDer) {
        printf("mismatching encoded certificates\n");
        return false;
    }

    Manifest remoteManifest;
    status = proxyObjectManager->GetManifest(lastAppInfo, remoteManifest);
    if (ER_OK != status) {
        printf("failed to GetManifest\n");
        return false;
    }

    if (expectedManifest != remoteManifest) {
        printf("mismatching remote manifest: expected %s, got %s\n",
               expectedManifest.ToString().c_str(),
               remoteManifest.ToString().c_str());
        return false;
    }

    if (expectedManifest != storedManifest) {
        printf("mismatching stored manifest: expected %s, got %s\n",
               expectedManifest.ToString().c_str(),
               storedManifest.ToString().c_str());
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::CheckRemoteMemberships(vector<GroupInfo> expected)
{
    printf("Checking remote memberships ... ");
    vector<MembershipSummary> remote;
    QStatus status = proxyObjectManager->GetMembershipSummaries(lastAppInfo, remote);
    if (ER_OK != status) {
        printf("failed to GetMembershipSummaries\n");
        return false;
    }

    size_t expectedSize = expected.size();
    size_t remoteSize = remote.size();
    if (expectedSize != remoteSize) {
        printf("mismatching size: expected %lu, got %lu\n",
               (unsigned long)expectedSize,
               (unsigned long)remoteSize);
        return false;
    }

    vector<MembershipCertificate> stored;
    status = ca->GetMembershipCertificates(lastAppInfo, stored);
    if (ER_OK != status) {
        printf("failed to GetMembershipCertificates\n");
        return status;
    }

    vector<GroupInfo>::size_type e;
    vector<MembershipCertificate>::size_type s;
    vector<MembershipSummary>::size_type r;
    // for each expected membership
    for (e = 0; e != expected.size(); e++) {
        // find serial number from storage
        string serial;
        for (s = 0; s != stored.size(); s++) {
            if (stored[s].GetGuild() == expected[e].guid) {
                serial = string((const char*)stored[s].GetSerial(),
                                stored[s].GetSerialLen());
                break;
            }
        }
        if (serial.empty()) {
            printf("could not determine serial number for %s\n",
                   expected[e].name.c_str());
            return false;
        }

        // find serial number in remotes
        for (r = 0; r != remote.size(); r++) {
            if (remote[r].serial == serial) {
                break;
            }
        }
        if (r == remote.size()) {
            printf("could not find remote certificate for %s\n",
                   expected[e].name.c_str());
            return false;
        }

        // remove from remotes
        remote.erase(remote.begin() + r);
    }

    if (remote.size() != 0) {
        printf("found unexpected remote certificate\n");
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::CheckUpdatesPending(bool expected)
{
    printf("Checking updates pending in security agent ... ");
    OnlineApplication check;
    check.keyInfo = lastAppInfo.keyInfo;
    QStatus status = secMgr->GetApplication(check);
    if (ER_OK != status) {
        printf("failed to GetApplication\n");
        return false;
    }

    bool actual = check.updatesPending;
    if (expected != actual) {
        printf("unexpected updatesPending: expected %s , got %s\n",
               expected ? "true" : "false",
               actual ? "true" : "false");
        return false;
    }

    printf("ok\n");
    return true;
}

bool BasicTest::WaitForUpdatesCompleted()
{
    printf("Waiting for updates completed ... ");
    bool result = true;

    result = WaitForState(PermissionConfigurator::CLAIMED, true, true);
    if (!result) {
        return result;
    }

    result = WaitForState(PermissionConfigurator::CLAIMED, true, false);
    if (!result) {
        return result;
    }

    result = CheckUpdatesPending(false);

    return result;
}

void BasicTest::TearDown()
{
    delete proxyObjectManager;
    proxyObjectManager = nullptr;

    if (tal) {
        secMgr->UnregisterApplicationListener(tal);
        delete tal;
        tal = nullptr;
    }

    delete pg;
    pg = nullptr;

    secMgr = nullptr;

    ba->UnregisterAboutListener(testAboutListener);

    ba->Disconnect();
    ba->Stop();
    ba->Join();

    delete ba;
    ba = nullptr;
    delete stub;
    stub = nullptr;
    storage->Reset();
}
}
