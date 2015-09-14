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

#include "TestUtil.h"
#include <qcc/Thread.h>

using namespace ajn::securitymgr;

/** @file ManifestTests.cc */

namespace secmgr_tests {
class ManifestTests :
    public SecurityAgentTest {
  public:
    IdentityInfo idInfo;

    ManifestTests()
    {
        idInfo.guid = GUID128();
        idInfo.name = "testName";
    }

    void GetManifest(Manifest& mf)
    {
        PermissionPolicy::Rule rules[1];

        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member prms[3];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("Channel");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        rules[0].SetMembers(3, prms);

        mf.SetFromRules(rules, 1);
    }

    void GetExtendedManifest(Manifest& mf)
    {
        PermissionPolicy::Rule rules[2];

        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member prms[3];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("Channel");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE |
                              PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(3, prms);

        rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
        PermissionPolicy::Rule::Member mprms[1];
        mprms[0].SetMemberName("*");
        mprms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, mprms);

        mf.SetFromRules(rules, 2);
    }
};

/**
 * @test Update the manifest of an application and check whether a ManifestUpdate event is
 *       triggered if the manifest contains additional rules.
 *       -# Set the manifest of the application to manifest1.
 *       -# Claim the application and check whether the manifest during claiming matches the
 *          remote manifest.
 *       -# Set the manifest of the application to manifest2 which extends manifest1.
 *       -# Check whether a ManifestUpdate event is triggered.
 *       -# Update the identity certificate based on the newly requested manifest.
 *       -# Check that no additional ManifestUpdate events are triggered.
 *       -# Set the manifest of the application to manifest1.
 *       -# Make sure no additional ManifestUpdate events are triggered.
 **/
TEST_F(ManifestTests, UpdateManifest) {
    Manifest manifest;
    GetManifest(manifest);
    TestApplication testApp;
    testApp.SetManifest(manifest);
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_OK, storage->StoreIdentity(idInfo));
    secMgr->Claim(lastAppInfo, idInfo);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED));
    ASSERT_TRUE(CheckIdentity(idInfo, manifest));

    Manifest extendedManifest;
    GetExtendedManifest(extendedManifest);
    testApp.UpdateManifest(extendedManifest);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::NEED_UPDATE));

    ManifestUpdate update;
    ASSERT_TRUE(WaitForManifestUpdate(update));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(update.app, idInfo, update.newManifest));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::NEED_UPDATE, SYNC_PENDING));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED));
    ASSERT_TRUE(CheckIdentity(idInfo, extendedManifest));

    testApp.UpdateManifest(manifest);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::NEED_UPDATE));

    RemoveSecAgent(); // wait for all updates to complete
}
} // namespace
