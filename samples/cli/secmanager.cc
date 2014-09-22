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
#include <alljoyn/BusAttachment.h>
#include <AuthorizationData.h>
#include <string>

using namespace std;
using namespace ajn::securitymgr;

#define GUILDINFO_DELIMITER "/"
#define GUILD_DESC_MAX 200

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
        cout << "  - claim state     :" << ToString(old->claimState) << " --> "
             << ToString(updated->claimState) << endl;
        cout << "  - running state     :"
             << ajn::securitymgr::ToString(old->runningState) << " --> "
             << ajn::securitymgr::ToString(updated->runningState) << endl
             << "> " << flush;
    }
};

std::ostream& operator<<(std::ostream& strm, const GuildInfo& gi)
{
    return strm << "Guild: (" << gi.guid << " / " << gi.name << " / " << gi.desc
           << ")";
}

static void list_claimable_applications(ajn::securitymgr::SecurityManager* secMgr)
{
    vector<ajn::securitymgr::ApplicationInfo> claimableApps;

    const vector<ajn::securitymgr::ApplicationInfo> listOfAllApps =
        secMgr->GetApplications();
    vector<ajn::securitymgr::ApplicationInfo>::const_iterator it =
        listOfAllApps.begin();

    for (; it != listOfAllApps.end(); ++it) {
        const ajn::securitymgr::ApplicationInfo& info = *it;
        if ((ajn::securitymgr::ApplicationClaimState::CLAIMABLE
             == info.claimState)) {
            claimableApps.push_back(info);
        }
    }

    if (0 == claimableApps.size()) {
        cout << "There are currently no claimable applications published"
        << endl;
        return;
    } else {
        cout << "There are currently " << claimableApps.size()
        << " unclaimed applications published" << endl;
    }

    it = claimableApps.begin();
    for (int i = 0; it < claimableApps.end(); ++it) {
        const ajn::securitymgr::ApplicationInfo& info = *it;
        cout << i << ". bus name: " << info.busName << " (" << info.appName
        << "@" << info.deviceName << ") running: "
        << ajn::securitymgr::ToString(info.runningState)
        << ", claimed: " << ajn::securitymgr::ToString(info.claimState)
        << endl;
    }
}

static void list_applications(ajn::securitymgr::SecurityManager* secMgr)
{
    const vector<ajn::securitymgr::ApplicationInfo> listOfAllApps =
        secMgr->GetApplications();

    if (0 == listOfAllApps.size()) {
        cout << "There are currently no claimable applications published"
        << endl;
        return;
    } else {
        cout << "There are currently " << listOfAllApps.size()
        << " applications known." << endl;
    }

    vector<ajn::securitymgr::ApplicationInfo>::const_iterator it =
        listOfAllApps.begin();
    for (int i = 0; it != listOfAllApps.end(); ++it, i++) {
        const ajn::securitymgr::ApplicationInfo& info = *it;

        cout << i << ". bus name: " << info.busName << " (" << info.appName
        << "@" << info.deviceName << ") running: "
        << ajn::securitymgr::ToString(info.runningState)
        << ", claimed: " << ajn::securitymgr::ToString(info.claimState)
        << ", key = " << info.publicKey.ToString() << ", root of trust count = "
        << info.rootOfTrustList.size() << endl;
    }
}

static void list_claimed_applications(ajn::securitymgr::SecurityManager* secMgr)
{
    vector<ajn::securitymgr::ApplicationInfo> managedApps =
        secMgr->GetApplications(ApplicationClaimState::CLAIMED);
    if (!managedApps.empty()) {
        cout << "  Following claimed applications have been found:" << endl;
        cout << "  ===============================================" << endl;

        vector<ajn::securitymgr::ApplicationInfo>::const_iterator it =
            managedApps.begin();
        for (int i = 0; it < managedApps.end(); ++it, i++) {
            const ajn::securitymgr::ApplicationInfo& info = *it;
            cout << i << ". bus name: " << info.busName << " (" << info.appName
            << "@" << info.deviceName << ") running: "
            << ajn::securitymgr::ToString(info.runningState)
            << ", claimed: "
            << ajn::securitymgr::ToString(info.claimState) << endl;
        }
    } else {
        cout << "There are currently no claimed applications" << endl;
    }
}

static bool accept_manifest(const ajn::AuthorizationData& manifest)
{
    bool result = false;

    qcc::String mnf;
    QStatus status = manifest.Serialize(mnf);

    if (status != ER_OK) {
        return false;
    }

    cout << "The application requests the following rights:" << endl;
    cout << mnf << endl;
    cout << "  Accept (y/n)? ";

    string input;
    getline(cin, input);

    char cmd;
    cmd = input[0];

    switch (cmd) {
    case 'y':
    case 'Y':
        result = true;
        break;
    }

    return result;
}

static void claim_application(ajn::securitymgr::SecurityManager* secMgr,
                              const string& arg)
{
    if (arg.empty()) {
        cout << "Empty bus name not allowed" << endl;
        return;
    }

    qcc::String busName(arg.c_str());
    ajn::securitymgr::ApplicationInfo appInfo;
    appInfo.busName = busName;

    if (ER_OK != secMgr->GetApplication(
            appInfo)) {
        cout
        << "Invalid bus name or Application for this bus name went offline..."
        << endl;
        return;
    } else {
        secMgr->ClaimApplication(appInfo, *accept_manifest);
    }
}

static void add_guild(ajn::securitymgr::SecurityManager* secMgr,
                      const string& arg)
{
    if (arg.empty()) {
        cout << "Empty guild information" << endl;
        return;
    }

    GuildInfo guildInfo;

    string tmp(arg);

    cout << "tmp 1: " << tmp << endl;

    size_t pos = tmp.find(GUILDINFO_DELIMITER);
    guildInfo.guid = tmp.substr(0, pos).c_str();

    if (pos != string::npos) {
        tmp.erase(0, pos + string(GUILDINFO_DELIMITER).length());
        pos = tmp.find(GUILDINFO_DELIMITER);
        guildInfo.name = tmp.substr(0, pos).c_str();
        if (pos != string::npos) {
            tmp.erase(0, pos + string(GUILDINFO_DELIMITER).length());
            pos = tmp.find(GUILDINFO_DELIMITER);
            guildInfo.desc = tmp.substr(0, GUILD_DESC_MAX).c_str();
        }
    }

    if (ER_OK != secMgr->StoreGuild(guildInfo)) {
        cerr << "Guild was not added" << endl;
    } else {
        cout << "Guild was successfully added" << endl;
        cout << guildInfo << endl;
    }
}

static void get_guild(ajn::securitymgr::SecurityManager* secMgr,
                      const string& arg)
{
    if (arg.empty()) {
        cout << "Empty guild information" << endl;
        return;
    }

    GuildInfo guildInfo;

    guildInfo.guid = arg.substr(0, GUILD_DESC_MAX).c_str();

    if (ER_OK != secMgr->GetGuild(guildInfo)) {
        cerr << "Guild was not found" << endl;
    } else {
        cout << "Guild was successfully retrieved" << endl;
        cout << guildInfo << endl;
    }
}

static void list_guilds(ajn::securitymgr::SecurityManager* secMgr)
{
    vector<GuildInfo> guilds;

    if (ER_OK != secMgr->GetManagedGuilds(guilds)) {
        cerr << "Could not retrieve Guilds or none were found" << endl;
    } else {
        cout << "Retrieved Guild(s):" << endl;
        for (vector<GuildInfo>::const_iterator g = guilds.begin();
             g != guilds.end(); g++) {
            cout << *g << endl;
        }
    }
}

static void remove_guild(ajn::securitymgr::SecurityManager* secMgr,
                         const string& arg)
{
    if (arg.empty()) {
        cout << "Empty guild information" << endl;
        return;
    }

    if (ER_OK != secMgr->RemoveGuild(arg.substr(0, GUILD_DESC_MAX).c_str())) {
        cerr << "Guild was not found" << endl;
    } else {
        cout << "Guild was successfully removed" << endl;
    }
}

static void update_membership(ajn::securitymgr::SecurityManager* secMgr,
                              const string& arg, bool add)
{
    std::size_t delpos = arg.find_first_of(" ");

    if (arg.empty() || delpos == string::npos) {
        cerr << "Please provide application bus name and guild id." << endl;
        return;
    }

    qcc::String busName = arg.substr(0, delpos).c_str();
    ajn::securitymgr::ApplicationInfo appInfo;
    appInfo.busName = busName;

    if (ER_OK != secMgr->GetApplication(
            appInfo)) {
        cerr << "Could not find application with bus name " << busName << "." << endl;
        return;
    }

    if (appInfo.claimState != ApplicationClaimState::CLAIMED) {
        cerr << "The application is not claimed." << endl;
        return;
    }

    if (appInfo.runningState != ApplicationRunningState::RUNNING) {
        cerr << "The application is not running." << endl;
        return;
    }

    GuildInfo guildInfo;
    guildInfo.guid = arg.substr(delpos + 1, string::npos).c_str();

    if (ER_OK != secMgr->GetGuild(guildInfo)) {
        cerr << "Could not find guild with id " << guildInfo.guid << "." << endl;
        return;
    }

    if (add) {
        secMgr->InstallMembership(appInfo, guildInfo);
    } else {
        secMgr->RemoveMembership(appInfo, guildInfo);
    }
}

static void help()
{
    cout << endl;
    cout << "  Supported commands:" << endl;
    cout << "  ===================" << endl;
    cout << "    q   quit" << endl;
    cout << "    f   list all claimable applications" << endl;
    cout << "    c   claim an application (busnm)" << endl;
    cout << "    l   list all claimed applications" << endl;
    cout << "    a   list all active applications" << endl;
    cout << "    g   create a guild (id/name/description)" << endl;
    cout << "    r   remove a guild (id)" << endl;
    cout << "    k   get a guild (id)" << endl;
    cout << "    p   list all guilds" << endl;
    cout << "    m   install a membership certificate (busnm, guildid)" << endl;
    cout << "    d   delete a membership certificate (busnm, guildid)" << endl;
    cout << "    h   show this help message" << endl << endl;
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

    case 'g':
        add_guild(secMgr, arg);
        break;

    case 'k':
        get_guild(secMgr, arg);
        break;

    case 'r':
        remove_guild(secMgr, arg);
        break;

    case 'p':
        list_guilds(secMgr);
        break;

    case 'm':
        update_membership(secMgr, arg, true);
        break;

    case 'd':
        update_membership(secMgr, arg, false);
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

    ajn::securitymgr::SecurityManagerFactory& secFac =
        ajn::securitymgr::SecurityManagerFactory::GetInstance();
    ajn::securitymgr::StorageConfig storageCfg;     //Will rely on default storage path
    ajn::securitymgr::SecurityManager* secMgr = secFac.GetSecurityManager(
        "hello", "world", storageCfg, NULL);

    if (NULL == secMgr) {
        cerr
        << "> Error: Security Factory returned an invalid SecurityManager object !!"
        << endl;
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
