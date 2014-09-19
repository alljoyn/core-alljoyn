/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <Common.h>
#include <ApplicationInfo.h>
#include <SecurityManager.h>
#include <StorageConfig.h>
#include <SecurityManagerFactory.h>
#include <ApplicationListener.h>
#include <ApplicationMonitor.h>
#include <alljoyn/BusAttachment.h>

using namespace std;
using namespace ajn::securitymgr;

// Event listener for the monitor
class EventListener :
    public ajn::securitymgr::ApplicationListener {
    virtual void OnApplicationStateChange(const ajn::securitymgr::ApplicationInfo* old,
                                          const ajn::securitymgr::ApplicationInfo* updated)
    {
        cout << "  Application updated:" << endl;
        cout << "  ====================" << endl;
        cout << "  Application name :" << updated->appName << endl;
        cout << "  Hostname            :" << updated->deviceName << endl;
        cout << "  Busname            :" << updated->busName << endl;
        cout << "  - claim state     :" << ToString(old->claimState) << " --> " <<
        ToString(updated->claimState) <<
        endl;
        cout << "  - running state     :" << ajn::securitymgr::ToString(old->runningState) << " --> " <<
        ajn::securitymgr::ToString(updated->runningState) <<
        endl << "> " << flush;
    }
};

static void list_claimable_applications(ajn::securitymgr::SecurityManager* secMgr)
{
    vector<ajn::securitymgr::ApplicationInfo> claimableApps;

    const vector<ajn::securitymgr::ApplicationInfo> listOfAllApps = secMgr->GetApplications();
    vector<ajn::securitymgr::ApplicationInfo>::const_iterator it = listOfAllApps.begin();

    for (; it != listOfAllApps.end(); ++it) {
        const ajn::securitymgr::ApplicationInfo& info = *it;
        if ((ajn::securitymgr::ApplicationClaimState::CLAIMABLE == info.claimState)) {
            claimableApps.push_back(info);
        }
    }

    if (0 == claimableApps.size()) {
        cout << "There are currently no claimable applications published" << endl;
        return;
    } else {
        cout << "There are currently " << claimableApps.size() << " unclaimed applications published" << endl;
    }

    it = claimableApps.begin();
    for (int i = 0; it < claimableApps.end(); ++it) {
        const ajn::securitymgr::ApplicationInfo& info = *it;
        cout << i << ". bus name: " << info.busName << " (" << info.appName << "@" << info.deviceName <<
        ") running: " << ajn::securitymgr::ToString(info.runningState)
             << ", claimed: " << ajn::securitymgr::ToString(info.claimState) <<
        endl;
    }
}

static void list_applications(ajn::securitymgr::SecurityManager* secMgr)
{
    const vector<ajn::securitymgr::ApplicationInfo> listOfAllApps = secMgr->GetApplications();

    if (0 == listOfAllApps.size()) {
        cout << "There are currently no claimable applications published" << endl;
        return;
    } else {
        cout << "There are currently " << listOfAllApps.size() << " applications known." << endl;
    }

    vector<ajn::securitymgr::ApplicationInfo>::const_iterator it = listOfAllApps.begin();
    for (int i = 0; it != listOfAllApps.end(); ++it, i++) {
        const ajn::securitymgr::ApplicationInfo& info = *it;

        cout << i << ". bus name: " << info.busName << " (" << info.appName << "@" << info.deviceName <<
        ") running: " << ajn::securitymgr::ToString(info.runningState)
             << ", claimed: " << ajn::securitymgr::ToString(info.claimState) << ", key = " << info.publicKey <<
        ", root of trust count = " << info.rootOfTrustList.size() << endl;
    }
}

static void list_claimed_applications(ajn::securitymgr::SecurityManager* secMgr)
{
    vector<ajn::securitymgr::ApplicationInfo> claimedApps = secMgr->GetApplications(ApplicationClaimState::CLAIMED);
    if (!claimedApps.empty()) {
        cout << "  Following claimed applications have been found:" << endl;
        cout << "  ===============================================" << endl;

        vector<ajn::securitymgr::ApplicationInfo>::const_iterator it = claimedApps.begin();
        for (int i = 0; it < claimedApps.end(); ++it, i++) {
            const ajn::securitymgr::ApplicationInfo& info = *it;
            cout << i << ". bus name: " << info.busName << " (" << info.appName << "@" << info.deviceName <<
            ") running: " << ajn::securitymgr::ToString(info.runningState)
                 << ", claimed: " << ajn::securitymgr::ToString(info.claimState) <<
            endl;
        }
    } else {
        cout << "There are currently no claimed applications" << endl;
    }
}

static void claim_application(ajn::securitymgr::SecurityManager* secMgr,
                              const string& arg)
{
    if (arg.empty()) {
        cout << "Empty bus name not allowed" << endl;
        return;
    }

    qcc::String busName(arg.c_str());
    const ajn::securitymgr::ApplicationInfo* appInfo = secMgr->GetApplication(busName);

    if (NULL == appInfo) {
        cout << "Invalid bus name or Application for this bus name went offline or was claimed already" << endl;
        return;
    } else {
        secMgr->ClaimApplication(*appInfo);
    }

    delete appInfo;
    appInfo = NULL;
}

static void help()
{
    cout << endl;
    cout << "  Supported commands:" << endl;
    cout << "  ===================" << endl;
    cout << "    q         quit" << endl;
    cout << "    f         list all claimable applications" << endl;
    cout << "    c <index> claim an application" << endl;
    cout << "    l         list all claimed applications" << endl;
    cout << "    a         list all active applications" << endl;
    cout << "    h         show this help message" << endl << endl;
}

static bool parse(ajn::securitymgr::SecurityManager* secMgr,
                  const string& input)
{
    char cmd;
    size_t argpos;
    string arg = "";

    if (input.length() == 0) {
        return true;
    }

    cmd = input[0];
    argpos = input.find_first_not_of(" \t", 1);
    if (argpos != input.npos) {
        arg = input.substr(argpos);
    }

    switch (cmd) {
    case 'q':
        return false;

    case 'f':
        list_claimable_applications(secMgr);
        break;

    case 'l':
        list_claimed_applications(secMgr);
        break;

    case 'c':
        claim_application(secMgr, arg);
        break;

    case 'a':
        list_applications(secMgr);
        break;

    case 'h':
    default:
        help();
        break;
    }

    return true;
}

int main(int argc, char** argv)
{
//    system("reset");
    cout << "################################################################################" << endl;
    cout << "##                  __                      _ _                               ##" << endl;
    cout << "##                 / _\\ ___  ___ _   _ _ __(_) |_ _   _                       ##" << endl;
    cout << "##                 \\ \\ / _ \\/ __| | | | '__| | __| | | |                      ##" << endl;
    cout << "##                 _\\ \\  __/ (__| |_| | |  | | |_| |_| |                      ##" << endl;
    cout << "##                 \\__/\\___|\\___|\\__,_|_|  |_|\\__|\\__, |                      ##" << endl;
    cout << "##                                                |___/                       ##" << endl;
    cout << "##                                                                            ##" << endl;
    cout << "##                   /\\/\\   __ _ _ __   __ _  __ _  ___ _ __                  ##" << endl;
    cout << "##                  /    \\ / _` | '_ \\ / _` |/ _` |/ _ \\ '__|                 ##" << endl;
    cout << "##                 / /\\/\\ \\ (_| | | | | (_| | (_| |  __/ |                    ##" << endl;
    cout << "##                 \\/    \\/\\__,_|_| |_|\\__,_|\\__, |\\___|_|                    ##" << endl;
    cout << "##                                           |___/                            ##" << endl;
    cout << "##                                                                            ##" << endl;
    cout << "################################################################################" << endl;
    cout << "##                    Type h for displaying the help menu                     ##" << endl;
    cout << endl;

    ajn::securitymgr::SecurityManagerFactory& secFac = ajn::securitymgr::SecurityManagerFactory::GetInstance();
    ajn::securitymgr::StorageConfig storageCfg; //Will rely on default storage path
    ajn::securitymgr::SecurityManager* secMgr = secFac.GetSecurityManager("hello", "world", storageCfg, NULL);

    if (NULL == secMgr) {
        cerr << "> Error: Security Factory returned an invalid SecurityManager object !!" << endl;
        cerr << "> Exiting" << endl << endl;
        return EXIT_FAILURE;
    }

    // Activate live monitoring
    EventListener listener;
    secMgr->RegisterApplicationListener(&listener);

    bool done = false;
    while (!done) {
        string input;
        cout << "> ";
        getline(cin, input);
        done = !parse(secMgr, input);
    }

    // Cleanup
    secMgr->UnregisterApplicationListener(&listener);

    return EXIT_SUCCESS;
}
