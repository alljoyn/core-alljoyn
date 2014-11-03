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
#include <string>
#include <sstream>
#include <PolicyGenerator.h>
#include <alljoyn/PermissionPolicy.h>

using namespace std;
using namespace qcc;
using namespace ajn;
using namespace ajn::securitymgr;

#define GUILDINFO_DELIMITER "/"
#define GUILD_DESC_MAX 200
#define GUILD_ID_MAX 32

// Event listener for the monitor
class EventListener :
    public ajn::securitymgr::ApplicationListener {
    virtual void OnApplicationStateChange(const ajn::securitymgr::ApplicationInfo* old,
                                          const ajn::securitymgr::ApplicationInfo* updated)
    {
        cout << "  Application updated:" << endl;
        cout << "  ====================" << endl;
        cout << "  Application name : " << updated->appName << endl;
        cout << "  Hostname         : " << updated->deviceName << endl;
        cout << "  Busname          : " << updated->busName << endl;
        cout << "  - claim state    : " << ToString(old->claimState) << " --> "
             << ToString(updated->claimState) << endl;
        cout << "  - running state  : "
             << ajn::securitymgr::ToString(old->runningState) << " --> "
             << ajn::securitymgr::ToString(updated->runningState) << endl
             << "> " << flush;
    }
};

std::ostream& operator<<(std::ostream& strm, const GuildInfo& gi)
{
    return strm << "Guild: (" << gi.guid.ToString() << " / " << gi.name << " / " << gi.desc
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
        if ((ajn::PermissionConfigurator::STATE_CLAIMABLE
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
        secMgr->GetApplications(ajn::PermissionConfigurator::STATE_CLAIMED);
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

static bool accept_manifest(const ApplicationInfo& appInfo,
                            const PermissionPolicy::Rule* manifestRules,
                            const size_t manifestRulesCount,
                            void* cookie)
{
    bool result = false;

    cout << "The application requests the following rights:" << endl;
    for (size_t i = 0; i < manifestRulesCount; i++) {
        cout << manifestRules[i].ToString().c_str();
    }
    cout << "Accept (y/n)? ";

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
        vector<IdentityInfo> list;
        secMgr->GetManagedIdentities(list);
        if (list.size() == 0) {
            cout
            << "No identity defined..."
            << endl;
            return;
        }
        if (ER_OK != secMgr->ClaimApplication(appInfo, list.at(0), *accept_manifest)) {
            cout
            << "Failed to claim application..."
            << endl;
            return;
        }
    }
}

static vector<string> split(const string& s, char delim)
{
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

static void add_guild(ajn::securitymgr::SecurityManager* secMgr,
                      const string& arg)
{
    vector<string> args = split(arg, '/');

    if (args.size() < 2) {
        cerr << "Please provide a guild name and a description." << endl;
        return;
    }

    GuildInfo guildInfo;
    guildInfo.name = args[0].c_str();
    guildInfo.desc = args[1].substr(0, GUILD_DESC_MAX).c_str();

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
    GUID128 guildID(arg.substr(0, GUILD_ID_MAX).c_str());
    guildInfo.guid = guildID;

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

    GUID128 guildID(arg.substr(0, GUILD_ID_MAX).c_str());

    if (ER_OK != secMgr->RemoveGuild(guildID)) {
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

    if (appInfo.claimState != ajn::PermissionConfigurator::STATE_CLAIMED) {
        cerr << "The application is not claimed." << endl;
        return;
    }

    if (appInfo.runningState != STATE_RUNNING) {
        cerr << "The application is not running." << endl;
        return;
    }

    GuildInfo guildInfo;
    guildInfo.guid = GUID128(arg.substr(delpos + 1, string::npos).c_str());

    if (ER_OK != secMgr->GetGuild(guildInfo)) {
        cerr << "Could not find guild with id " << guildInfo.guid.ToString() << "." << endl;
        return;
    }

    if (add) {
        secMgr->InstallMembership(appInfo, guildInfo);
    } else {
        secMgr->RemoveMembership(appInfo, guildInfo);
    }
}

static void install_policy(SecurityManager* secMgr,
                           const string& arg)
{
    vector<string> args = split(arg, ' ');

    if (args.size() < 1) {
        cerr << "Please provide an application bus name." << endl;
        return;
    }

    ApplicationInfo appInfo;
    qcc::String busName = args[0].c_str();
    appInfo.busName = busName;

    if (ER_OK != secMgr->GetApplication(appInfo)) {
        cerr << "Could not find application." << endl;
        return;
    }

    vector<GUID128> guilds;
    for (size_t i = 1; i < args.size(); ++i) {
        GUID128 guildID(args[i].c_str());
        GuildInfo guildInfo;
        guildInfo.guid = guildID;
        if (ER_OK != secMgr->GetGuild(guildInfo)) {
            cerr << "Could not find guild with id " << args[i] << endl;
            return;
        }
        guilds.push_back(guildID);
    }

    PermissionPolicy policy;
    if (ER_OK != PolicyGenerator::DefaultPolicy(guilds, policy)) {
        cerr << "Failed to generate default policy." << endl;
        return;
    }

    if (ER_OK != secMgr->InstallPolicy(appInfo, policy)) {
        cerr << "Failed to install policy." << endl;
        return;
    }
    cout << "Successfully installed policy." << endl;
}

static void get_policy(SecurityManager* secMgr,
                       const string& arg)
{
    vector<string> args = split(arg, ' ');

    if (args.size() < 1) {
        cerr << "Please provide an application bus name." << endl;
        return;
    }

    ApplicationInfo appInfo;
    qcc::String busName = args[0].c_str();
    appInfo.busName = busName;

    if (ER_OK != secMgr->GetApplication(appInfo)) {
        cerr << "Could not find application." << endl;
        return;
    }

    PermissionPolicy policyRemote;
    PermissionPolicy policyLocal;
    if (ER_OK != secMgr->GetPolicy(appInfo, policyRemote, true)) {
        cerr << "Failed to get remote policy." << endl;
        return;
    }

    if (ER_OK != secMgr->GetPolicy(appInfo, policyLocal, false)) {
        cerr << "Failed to get locally persisted policy." << endl;
        return;
    }

    cout << "Successfully retrieved remote policy for " << busName.c_str() << ":" << endl;
    cout << policyRemote.ToString() << endl;

    cout << "Successfully retrieved locally persisted policy for " << busName.c_str() << ":" << endl;
    cout << policyLocal.ToString() << endl;
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
    cout << "    g   create a guild (name/description)" << endl;
    cout << "    r   remove a guild (id)" << endl;
    cout << "    k   get a guild (id)" << endl;
    cout << "    p   list all guilds" << endl;
    cout << "    m   install a membership certificate (busnm guildid)" << endl;
    cout << "    d   delete a membership certificate (busnm guildid)" << endl;
    cout << "    o   install a policy (busnm guildid1 guildid2 ...)" << endl;
    cout << "    e   get policy (busnm)" << endl;
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

    case 'o':
        install_policy(secMgr, arg);
        break;

    case 'e':
        get_policy(secMgr, arg);
        break;

    case 'h':
    default:
        help();
        break;
    }

    return true;
}

static void show_usage()
{
    cout << "AllSeen Security Manager CLI\n";
    cout << endl;
    cout << "Arguments:\n";
    cout << "\t-s\tRun against the stub instead of the core implementation.\n";
    cout << "\t-h\tDisplays this help message.\n";
}

int main(int argc, char** argv)
{
    // read command-line arguments
    bool stub = false;
    if (argc > 1) {
        if (std::string(argv[1]) == "-h") {
            show_usage();
            return EXIT_FAILURE;
        }
        if (std::string(argv[1]) == "-s") {
            stub = true;
        } else {
            show_usage();
            return EXIT_FAILURE;
        }
    }

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
    ajn::securitymgr::SecurityManagerConfig smc;

    // set stub configuration if needed
    if (stub) {
        smc.pmNotificationIfn = "org.allseen.Security.PermissionMgmt.Stub.Notification";
        smc.pmIfn = "org.allseen.Security.PermissionMgmt.Stub";
        smc.pmObjectPath = "/security/PermissionMgmt";
    }

    ajn::securitymgr::SecurityManager* secMgr = secFac.GetSecurityManager(
        "hello", "world", storageCfg, smc, NULL);

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
    vector<IdentityInfo> list;
    if (ER_OK != secMgr->GetManagedIdentities(list)) {
        cerr
        << "> Error: Failed to retrieve identities !!"
        << endl;
        cerr << "> Exiting" << endl << endl;
    }
    if (list.size() == 0) {
        IdentityInfo info;
        info.guid = qcc::String("abcdef1234567890");
        info.name = "MyTestIdentity";
        if (ER_OK != secMgr->StoreIdentity(info, false)) {
            cerr
            << "> Error: Failed to store default identity !!"
            << endl;
            cerr << "> Exiting" << endl << endl;
        }
    }

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
