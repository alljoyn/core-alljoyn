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

using namespace std;

namespace secmgr_tests {
TestApplication::TestApplication() :
    busAttachment(nullptr)
{
}

QStatus TestApplication::Start()
{
    QStatus status = ER_OK;

    if (!busAttachment) {
        busAttachment = new BusAttachment("test", true);

        status = busAttachment->Start();
        if (ER_OK != status) {
            return status;
        }
        status = busAttachment->Connect();
        if (ER_OK != status) {
            return status;
        }

        status = busAttachment->EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                                                   &authListener);
        if (ER_OK != status) {
            return status;
        }

        status = SetManifest();
        if (ER_OK != status) {
            return status;
        }
    }

    return status;
}

QStatus TestApplication::Stop()
{
    QStatus status = ER_OK;

    if (busAttachment) {
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

        delete busAttachment;
        busAttachment = nullptr;
    }

    return status;
}

void TestApplication::GetManifest(PermissionPolicy::Rule** retRules,
                                  size_t& count)
{
    count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[count];
    rules[0].SetInterfaceName("org.allseenalliance.control.TV");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
}

QStatus TestApplication::SetManifest()
{
    QStatus status = ER_OK;

    PermissionPolicy::Rule* rules;
    size_t count;
    GetManifest(&rules, count);

    status = busAttachment->GetPermissionConfigurator().
             SetPermissionManifest(rules, count);

    delete[] rules;
    rules = nullptr;

    return status;
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
