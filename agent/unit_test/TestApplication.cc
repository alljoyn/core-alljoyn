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

#include <string.h>

#include <alljoyn/AuthListener.h>

#include "TestApplication.h"
#include "AJNCa.h"

using namespace std;

namespace secmgr_tests {
TestApplication::TestApplication(string _appName) :
    busAttachment(nullptr), appName(_appName), psk(), authListener(psk)
{
    manifestRulesCount = 2;
    manifestRules = new PermissionPolicy::Rule[manifestRulesCount];
    manifestRules[0].SetInterfaceName("org.allseenalliance.control.TV");
    PermissionPolicy::Rule::Member prms[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    manifestRules[0].SetMembers(2, prms);

    manifestRules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    PermissionPolicy::Rule::Member mprms[1];
    mprms[0].SetMemberName("*");
    mprms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    manifestRules[1].SetMembers(1, mprms);

    busAttachment = new BusAttachment(appName.c_str(), true);
}

const string TestApplication::GetBusName() const
{
    return busAttachment->GetUniqueName().c_str();
}

QStatus TestApplication::SetClaimByPSK()
{
    QStatus status = busAttachment->GetPermissionConfigurator().SetClaimCapabilities(
        PermissionConfigurator::CAPABLE_ECDHE_PSK);
    if (ER_OK == status) {
        busAttachment->GetPermissionConfigurator().SetClaimCapabilityAdditionalInfo(
            PermissionConfigurator::PSK_GENERATED_BY_APPLICATION);
    }
    return status;
}

QStatus TestApplication::Start()
{
    QStatus status = ER_OK;

    status = busAttachment->Start();
    if (ER_OK != status) {
        return status;
    }
    status = busAttachment->Connect();
    if (ER_OK != status) {
        return status;
    }

    status = busAttachment->EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA",
                                               &authListener);
    if (ER_OK != status) {
        return status;
    }

    status = AnnounceManifest();
    if (ER_OK != status) {
        return status;
    }

    return status;
}

QStatus TestApplication::Stop()
{
    QStatus status = ER_OK;

    status = busAttachment->EnablePeerSecurity("", nullptr, nullptr, true);
    if (ER_OK != status) {
        return status;
    }

    status = busAttachment->Disconnect();
    if (ER_OK != status) {
        return status;
    }

    status = busAttachment->Stop();
    if (ER_OK != status) {
        return status;
    }

    status = busAttachment->Join();
    if (ER_OK != status) {
        return status;
    }

    return status;
}

QStatus TestApplication::SetManifest(const Manifest& manifest)
{
    delete[] manifestRules;
    manifestRules = nullptr;

    return manifest.GetRules(&manifestRules, &manifestRulesCount);
}

QStatus TestApplication::AnnounceManifest()
{
    QStatus status = ER_OK;

    if ((manifestRulesCount > 0) && manifestRules) {
        status = busAttachment->GetPermissionConfigurator().
                 SetPermissionManifest(manifestRules, manifestRulesCount);
    }

    return status;
}

QStatus TestApplication::UpdateManifest(const Manifest& manifest)
{
    QStatus status = ER_FAIL;

    status = SetManifest(manifest);
    if (ER_OK != status) {
        return status;
    }

    status = AnnounceManifest();
    if (ER_OK != status) {
        return status;
    }

    return SetApplicationState(PermissionConfigurator::NEED_UPDATE);
}

QStatus TestApplication::SetApplicationState(const PermissionConfigurator::ApplicationState state)
{
    return busAttachment->GetPermissionConfigurator().SetApplicationState(state);
}

void TestApplication::Reset()
{
    busAttachment->ClearKeyStore();
}

TestApplication::~TestApplication()
{
    Reset();
    Stop();
}
} // namespace
