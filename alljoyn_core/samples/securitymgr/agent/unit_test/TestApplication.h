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
#ifndef ALLJOYN_SECMGR_TESTAPPLICATION_H_
#define ALLJOYN_SECMGR_TESTAPPLICATION_H_

#include <string>
#include <memory>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/securitymgr/Manifest.h>

#include <qcc/GUID.h>

using namespace std;
using namespace ajn;
using namespace ajn::securitymgr;
using namespace qcc;

/** @file TestApplication.h */

namespace secmgr_tests {
class TestAppAuthListener :
    public DefaultECDHEAuthListener {
  public:
    TestAppAuthListener(GUID128& psk) :
        DefaultECDHEAuthListener(psk.GetBytes(), GUID128::SIZE)
    {
    }

    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
    {
        QCC_UNUSED(peerName);
        QCC_UNUSED(success);
        lastAuthMechanism = authMechanism;
    }

    string lastAuthMechanism;
};

class TestApplication {
  public:
    /**
     * Creates a new TestApplication.
     */
    TestApplication(string _appName = "secmgrctestapp");

    /**
     * Starts this TestApplication.
     */
    QStatus Start();

    /**
     * Sets the manifest of this TestApplication.
     */
    QStatus SetManifest(const Manifest& manifest);

    /**
     * Sets the manifest on the BusAttachment.
     */
    QStatus AnnounceManifest();

    /**
     * Updates the manifest of this TestApplication.
     */
    QStatus UpdateManifest(const Manifest& manifest);

    /**
     * Sets the application state as permitted by PermissionConfigurator
     */
    QStatus SetApplicationState(const PermissionConfigurator::ApplicationState state);

    /**
     * Resets the keystore of this TestApplication.
     */
    void Reset();

    /*
     * Stops this TestApplication.
     */
    QStatus Stop();

    QStatus SetClaimByPSK();

    const GUID128& GetPsk() const
    {
        return psk;
    }

    const string GetBusName() const;

    bool IsClaimed() const
    {
        PermissionConfigurator::ApplicationState state = PermissionConfigurator::NOT_CLAIMABLE;
        QStatus status = busAttachment->GetPermissionConfigurator().GetApplicationState(state);
        if (ER_OK == status) {
            if (PermissionConfigurator::CLAIMED == state || PermissionConfigurator::NEED_UPDATE == state) {
                return true;
            }
        }
        return false;
    }

    const string& GetLastAuthMechanism()
    {
        return authListener.lastAuthMechanism;
    }

    /*
     * Destructor for TestApplication.
     */
    ~TestApplication();

  private:
    shared_ptr<BusAttachment> busAttachment;
    string appName;
    PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount;
    GUID128 psk;
    TestAppAuthListener authListener;
};
} // namespace
#endif /* ALLJOYN_SECMGR_TESTAPPLICATION_H_ */
