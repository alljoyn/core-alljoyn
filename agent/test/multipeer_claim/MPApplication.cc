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

#include "MPApplication.h"

#include <qcc/Thread.h>

using namespace std;

namespace secmgr_tests {
MPApplication::MPApplication(pid_t pid) :
    busAttachment(nullptr), authListener()
{
    appName = string("MP-") + to_string(pid);
}

QStatus MPApplication::Start()
{
    QStatus status = ER_OK;

    if (!busAttachment) {
        busAttachment = new BusAttachment(appName.c_str(), true);

        status = busAttachment->Start();
        if (ER_OK != status) {
            return status;
        }
        status = busAttachment->Connect();
        if (ER_OK != status) {
            return status;
        }

        cout << "MPApplication[" << getpid() << "]::Start> appName = '" << appName << "', busname = '" <<
            busAttachment->GetUniqueName() << "'" << endl;

        status = busAttachment->EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA",
                                                   &authListener);
        if (ER_OK != status) {
            return status;
        }
    }

    PermissionPolicy::Rule manifestRules[1];
    manifestRules[0].SetInterfaceName("org.allseenalliance.SecmgrTest.MP");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    manifestRules[0].SetMembers(1, prms);

    status = busAttachment->GetPermissionConfigurator().SetPermissionManifest(manifestRules, 1);
    if (ER_OK != status) {
        return status;
    }

    return status;
}

QStatus MPApplication::Stop()
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

        busAttachment->ClearKeyStore();

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

QStatus MPApplication::WaitUntilFinished()
{
    PermissionConfigurator::ApplicationState currentState = PermissionConfigurator::CLAIMABLE;
    do {
        qcc::Sleep(1000);
        busAttachment->GetPermissionConfigurator().GetApplicationState(currentState);
    } while (PermissionConfigurator::CLAIMED != currentState);
    cout << "MPApplication[" << getpid() << "] is claimed. WaitUntilFinished checks for reset" << endl;
    qcc::Sleep(2500);
    busAttachment->GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::NEED_UPDATE);
    do {
        qcc::Sleep(1000);
        busAttachment->GetPermissionConfigurator().GetApplicationState(currentState);
    } while (PermissionConfigurator::CLAIMABLE != currentState);
    cout << "MPApplication[" << getpid() << "] is reset. WaitUntilFinished Done" << endl;

    /*
     * The test is completed for this application. However we keep the application running so that
     * the security manager can come by and fetch its security signal.
     */
    do {
        qcc::Sleep(1000);
    } while (PermissionConfigurator::CLAIMABLE == currentState);
    return ER_OK;
}

MPApplication::~MPApplication()
{
    Stop();
}
} // namespace
