/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/securitymgr/ApplicationListener.h>
#include <alljoyn/securitymgr/ApplicationState.h>

#include <iostream>

using namespace std;

namespace ajn {
namespace securitymgr {
void ApplicationListener::PrintStateChangeEvent(const ApplicationInfo* old,
                                                const ApplicationInfo* updated)
{
    const ApplicationInfo* info = updated ? updated : old;
    cout << "  Application updated:" << endl;
    cout << "  ====================" << endl;
    cout << "  Application name : " << info->appName << endl;
    cout << "  User-defined name : " << info->userDefinedName << endl;
    cout << "  Hostname         : " << info->deviceName << endl;
    cout << "  Busname          : " << info->busName << endl;
    cout << "  - claim state    : " <<
    ToString(old ? old->claimState : ajn::PermissionConfigurator::STATE_UNKNOWN) << " --> "
         << ToString(updated ? updated->claimState : ajn::PermissionConfigurator::STATE_UNKNOWN)
         << endl;
    cout << "  - running state  : "
         << ToString(old ? old->runningState : STATE_NOT_RUNNING) << " --> "
         << ToString(updated ? updated->runningState : STATE_NOT_RUNNING) << endl
         << "> " << flush;
}
}
}
