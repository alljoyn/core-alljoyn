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

#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <memory>

#include <qcc/Mutex.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/securitymgr/PolicyGenerator.h>
#include <alljoyn/PermissionPolicy.h>

#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/SecurityAgent.h>
#include <alljoyn/securitymgr/storage/StorageFactory.h>
#include <alljoyn/securitymgr/SecurityAgentFactory.h>
#include <alljoyn/securitymgr/ApplicationListener.h>
#include <alljoyn/securitymgr/SyncError.h>
#include <alljoyn/securitymgr/ManifestUpdate.h>
#include <alljoyn/securitymgr/Util.h>

using namespace std;
using namespace qcc;
using namespace ajn;
using namespace ajn::securitymgr;

#define GROUPINFO_DELIMITER "/"
#define GROUP_DESC_MAX 200
#define GROUP_ID_MAX 32

static map<string, KeyInfoNISTP256> keys;
static Mutex lock;
static map<string, ApplicationMetaData> aboutCache; // Key is busname
static Mutex aboutCachelock;
static vector<ManifestUpdate> manifestUpdates;
static Mutex manifestUpdatesLock;

static string toKeyID(const KeyInfoNISTP256& key)
{
    GUID128 guid;
    guid.SetBytes(key.GetPublicKey()->GetX());
    return guid.ToString().c_str();
}

static string addKeyID(const KeyInfoNISTP256& key)
{
    string id = toKeyID(key);
    lock.Lock(__FILE__, __LINE__);
    map<string, KeyInfoNISTP256>::iterator it = keys.find(id);

    if (it == keys.end()) {
        keys[id] = key;
    }
    lock.Unlock(__FILE__, __LINE__);
    return id;
}

static bool getKey(string appId, KeyInfoNISTP256& key)
{
    string id(appId.c_str());
    lock.Lock(__FILE__, __LINE__);
    map<string, KeyInfoNISTP256>::iterator it = keys.find(id);
    bool found = false;
    if (it != keys.end()) {
        key = it->second;
        found = true;
    }
    lock.Unlock(__FILE__, __LINE__);
    return found;
}

const char* ToString(SyncErrorType errorType)
{
    switch (errorType) {
    case SYNC_ER_UNKNOWN:
        return "SYNC_ER_UNKNOWN";

    case SYNC_ER_STORAGE:
        return "SYNC_ER_STORAGE";

    case SYNC_ER_REMOTE:
        return "SYNC_ER_REMOTE";

    case SYNC_ER_CLAIM:
        return "SYNC_ER_CLAIM";

    case SYNC_ER_RESET:
        return "SYNC_ER_RESET";

    case SYNC_ER_IDENTITY:
        return "SYNC_ER_IDENTITY";

    case SYNC_ER_MEMBERSHIP:
        return "SYNC_ER_MEMBERSHIP";

    case SYNC_ER_POLICY:
        return "SYNC_ER_POLICY";

    default:
        return "SYNC_ER_UNEXPECTED";
    }
};

static void GetAboutInfo(const OnlineApplication& app,
                         ApplicationMetaData& data)
{
    aboutCachelock.Lock(__FILE__, __LINE__);
    map<string, ApplicationMetaData>::const_iterator itr =
        aboutCache.find(app.busName);
    if (itr != aboutCache.end()) {
        data.deviceName = itr->second.deviceName;
        data.appName = itr->second.appName;
    }
    aboutCachelock.Unlock(__FILE__, __LINE__);
}

static bool get_default_identity(const shared_ptr<UIStorage>& uiStorage,
                                 IdentityInfo& identity)
{
    vector<IdentityInfo> list;
    uiStorage->GetIdentities(list);
    if (list.size() == 0) {
        cout << "No identity defined..." << endl;
        return false;
    }

    identity = list.at(0);
    return true;
}

// Event listener for the monitor
class EventListener :
    public ApplicationListener {
    virtual void OnApplicationStateChange(const OnlineApplication* old,
                                          const OnlineApplication* updated)
    {
        const OnlineApplication* app = old == nullptr ? updated : old;
        cout << ">> Old application state : " << (old == nullptr ? "null" : old->ToString().c_str()) << endl;
        cout << ">> New application state : " << (updated == nullptr ? "null" : updated->ToString().c_str()) << endl;
        cout << ">> Application id        : " << addKeyID(app->keyInfo) << endl;
        ApplicationMetaData data;
        GetAboutInfo(*app, data);
        if (data.appName != "") {
            cout << ">> Application name      : " << data.appName << " (" << data.deviceName << ")" << endl;
        }
        cout << endl;
    }

    virtual void OnSyncError(const SyncError* er)
    {
        cout << "  Synchronization error" << endl;
        cout << "  =====================" << endl;
        cout << "  Bus name          : " << er->app.busName.c_str() << endl;
        cout << "  Type              : " << ToString(er->type) << endl;
        cout << "  Remote status     : " << QCC_StatusText(er->status) << endl;
        switch (er->type) {
        case SYNC_ER_IDENTITY:
            cout << "  IdentityCert SN   : " << er->GetIdentityCertificate()->GetSerial() << endl;
            break;

        case SYNC_ER_MEMBERSHIP:
            cout << "  MembershipCert SN :  " << er->GetMembershipCertificate()->GetSerial() << endl;
            break;

        case SYNC_ER_POLICY:
            cout << "  Policy version    : " << er->GetPolicy()->GetVersion() << endl;
            break;

        default:
            break;
        }
    }

    virtual void OnManifestUpdate(const ManifestUpdate* manifestUpdate)
    {
        manifestUpdatesLock.Lock();
        manifestUpdates.push_back(*manifestUpdate);
        manifestUpdatesLock.Unlock();

        cout << " >>>>> Received ManifestUpdate for " << manifestUpdate->app.busName
             << " (" << to_string(manifestUpdate->additionalRules.GetRulesSize())
             << " additional rule(s))" << endl;
    }
};

class CliAboutListner :
    public AboutListener {
    void Announced(const char* busName, uint16_t version,
                   SessionPort port, const MsgArg& objectDescriptionArg,
                   const MsgArg& aboutDataArg)
    {
        QCC_UNUSED(objectDescriptionArg);
        QCC_UNUSED(port);
        QCC_UNUSED(version);

        AboutData aboutData(aboutDataArg);
        char* appName;
        aboutData.GetAppName(&appName);
        char* deviceName;
        aboutData.GetDeviceName(&deviceName);

        cout << "\nReceived About signal:";
        cout << "\n BusName          : " << busName;
        cout << "\n Application Name : " << appName;
        cout << "\n Device Name      : " << deviceName << endl << endl;

        ApplicationMetaData appMetaData;
        appMetaData.deviceName = deviceName;
        appMetaData.appName = appName;

        aboutCachelock.Lock(__FILE__, __LINE__);
        aboutCache[busName] = appMetaData;
        aboutCachelock.Unlock(__FILE__, __LINE__);
    }
};

ostream& operator<<(ostream& strm, const GroupInfo& gi)
{
    return strm << "Group: (" << gi.guid.ToString() << " / " << gi.name << " / " << gi.desc
                << ")";
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

static void list_claimable_applications(shared_ptr<SecurityAgent>& secAgent)
{
    vector<OnlineApplication> claimableApps;
    secAgent->GetApplications(claimableApps);

    if (0 == claimableApps.size()) {
        cout << "There are currently no claimable applications published"
             << endl;
        return;
    } else {
        cout << "There are currently " << claimableApps.size()
             << " unclaimed applications published" << endl;
    }

    vector<OnlineApplication>::const_iterator it = claimableApps.begin();
    for (int i = 0; it < claimableApps.end(); ++it) {
        const OnlineApplication& info = *it;
        cout << i << ". id: " << toKeyID(info.keyInfo) << " -  bus name: " << info.busName << " - claim state: " <<
            PermissionConfigurator::ToString(info.applicationState) << endl;
    }
}

static void list_claimed_applications(const shared_ptr<UIStorage>& uiStorage)
{
    vector<Application> applications;
    uiStorage->GetManagedApplications(applications);

    if (!applications.empty()) {
        cout << "  Following claimed applications have been found:" << endl;
        cout << "  ===============================================" << endl;

        vector<Application>::const_iterator it =
            applications.begin();
        for (int i = 0; it < applications.end(); ++it, i++) {
            const Application& info = *it;
            cout << i << ". id: " << toKeyID(info.keyInfo) << endl;
        }
    } else {
        cout << "There are currently no claimed applications" << endl;
    }
}

class CLIClaimListener :
    public ClaimListener {
  public:

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        PermissionPolicy::Rule* manifestRules;
        size_t manifestRulesCount;
        ctx.GetManifest().GetRules(&manifestRules, &manifestRulesCount);

        bool result = false;

        cout << "The application requests the following rights:" << endl;
        for (size_t i = 0; i < manifestRulesCount; i++) {
            cout << manifestRules[i].ToString().c_str();
        }

        delete[] manifestRules;
        manifestRules = nullptr;

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

        ctx.ApproveManifest(result);
        if (result) {
            return SelectSessionType(ctx);
        }
        return ER_OK;
    }

    QStatus SelectSessionType(ClaimContext& ctx)
    {
        PermissionConfigurator::ClaimCapabilities caps = ctx.GetClaimCapabilities()
                                                         & (PermissionConfigurator::CAPABLE_ECDHE_NULL |
                                                            PermissionConfigurator::CAPABLE_ECDHE_PSK);

        if (0 == caps) {
            //NULL and PSK not supported
            cout << "Cannot claim application: claim over NULL or PSK session not supported by the application" << endl;
            return ER_NOT_IMPLEMENTED;
        }
        if (PermissionConfigurator::CAPABLE_ECDHE_NULL == caps) {
            return ClaimOverNull(ctx);
        }
        if (PermissionConfigurator::CAPABLE_ECDHE_PSK == caps) {
            return ClaimOverPSK(ctx);
        }

        cout << "Select claim mechanism:" << endl;
        cout << "  'n' to claim over a ECDHE_NULL session" << endl;
        cout << "  'p' to claim over a ECDHE_PSK session" << endl;
        cout << "  others to abort claiming process" << endl;

        string input;
        getline(cin, input);

        char cmd;
        cmd = input[0];

        switch (cmd) {
        case 'n':
            return ClaimOverNull(ctx);

        case 'p':
            return ClaimOverPSK(ctx);

        default:
            return ER_FAIL;
        }
    }

    QStatus ClaimOverNull(ClaimContext& ctx)
    {
        cout << "Claiming over a ECDHE_NULL session" << endl;
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL);
        return ER_OK;
    }

    QStatus ClaimOverPSK(ClaimContext& ctx)
    {
        PermissionConfigurator::ClaimCapabilityAdditionalInfo info = ctx.GetClaimCapabilityInfo()
                                                                     & (PermissionConfigurator::
                                                                        PSK_GENERATED_BY_APPLICATION
                                                                        | PermissionConfigurator::
                                                                        PSK_GENERATED_BY_SECURITY_MANAGER);
        if (info == 0) {
            cout << "No supported PSK generation scheme found" << endl;
            return ER_NOT_IMPLEMENTED;
        }
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK);
        if (PermissionConfigurator::PSK_GENERATED_BY_APPLICATION == info) {
            return ReadPSK(ctx);
        }
        if (PermissionConfigurator::PSK_GENERATED_BY_SECURITY_MANAGER == info) {
            return ProvidePSK(ctx);
        }
        cout << "Select PSK generation:" << endl;
        cout << "  'a' to use a PSK provided by the application" << endl;
        cout << "  'g' to generate a PSK" << endl;
        cout << "  others to abort claiming process" << endl;

        string input;
        getline(cin, input);

        char cmd;
        cmd = input[0];

        switch (cmd) {
        case 'a':
            return ReadPSK(ctx);

        case 'g':
            return ProvidePSK(ctx);

        default:
            return ER_FAIL;
        }
    }

    QStatus ReadPSK(ClaimContext& ctx)
    {
        cout << "please enter the PSK provided by the application" << endl;
        string input;
        getline(cin, input);
        String pskString = input.c_str();
        if (GUID128::IsGUID(pskString, true)) {
            GUID128 psk(pskString);
            ctx.SetPreSharedKey(psk.GetBytes(), GUID128::SIZE);
            cout << "Claiming application ..." << endl;
            return ER_OK;
        }
        cout << "PSK is not valid. Aborting ..." << endl;
        return ER_OK;
    }

    QStatus ProvidePSK(ClaimContext& ctx)
    {
        GUID128 psk;
        cout << "please provide the PSK to application and press enter to continue " << endl;
        cout << "PSK =  '" << psk.ToString() << "'" << endl;
        string input;
        ctx.SetPreSharedKey(psk.GetBytes(), GUID128::SIZE);
        getline(cin, input);
        cout << "Claiming application ..." << endl;
        return ER_OK;
    }
};

static void claim_application(shared_ptr<SecurityAgent>& secAgent,
                              const shared_ptr<UIStorage>& uiStorage,
                              const string& arg)
{
    if (arg.empty()) {
        cout << "Please provide an application ID" << endl;
        return;
    }

    OnlineApplication app;

    if (!getKey(arg, app.keyInfo) || ER_END_OF_DATA == secAgent->GetApplication(
            app)) {
        cout << "Invalid Application ..." << endl;
        return;
    }

    IdentityInfo identity;
    if (!get_default_identity(uiStorage, identity)) {
        return;
    }

    if (ER_OK != secAgent->Claim(app, identity)) {
        cout << "Failed to claim application ..." << endl;
        return;
    }
}

static void unclaim_application(const shared_ptr<UIStorage>& uiStorage,
                                const string& arg)
{
    if (arg.empty()) {
        cout << "Please provide an Application ID..." << endl;
        return;
    }

    Application app;

    if (!getKey(arg, app.keyInfo) || ER_END_OF_DATA == uiStorage->GetManagedApplication(app)) {
        cout << "Could not find application" << endl;
        return;
    }

    if (ER_OK != uiStorage->RemoveApplication(app)) {
        cout << "Failed to unclaim application" << endl;
        return;
    }
}

static void set_app_meta_data_and_name(const shared_ptr<UIStorage>& uiStorage, shared_ptr<SecurityAgent>& secAgent,
                                       const string& arg)
{
    vector<string> args = split(arg, ' ');

    if (args.size() != 2) {
        cerr << "Please provide an application id and a user defined name." << endl;
        return;
    }

    OnlineApplication app;

    if (!getKey(args[0], app.keyInfo) || ER_OK != uiStorage->GetManagedApplication(app)) {
        cerr << "Could not find application." << endl;
        return;
    }

    ApplicationMetaData appMetaData;
    appMetaData.userDefinedName = args[1].c_str();

    if (ER_OK != secAgent->GetApplication(app)) { // Fetches the online status which includes the busname.
        cout << "Could not find online application status..." << endl;
    } else {
        aboutCachelock.Lock(__FILE__, __LINE__);
        map<string, ApplicationMetaData>::const_iterator itr = aboutCache.find(app.busName);

        if (itr == aboutCache.end()) {
            cout << "Application with busname (" << app.busName.c_str() <<
                ") does not have cached about data!\nUpdating just the user defined name." << endl;
        } else {
            appMetaData.deviceName = itr->second.deviceName;
            appMetaData.appName = itr->second.appName;
        }
        aboutCachelock.Unlock(__FILE__, __LINE__);
    }

    do {
        ApplicationMetaData storedAppMetaData;
        if (ER_OK != uiStorage->GetAppMetaData(app, storedAppMetaData)) {
            cerr << "Failed to fetch persisted application meta data." << endl;
            break;
        }

        if (storedAppMetaData == appMetaData) {
            cout << "Application name and About meta data are already up to date..." << endl;
            break;
        }

        if (appMetaData.appName.empty()) {
            appMetaData.appName = storedAppMetaData.appName;
        }
        if (appMetaData.userDefinedName.empty()) {
            appMetaData.userDefinedName = storedAppMetaData.userDefinedName;
        }
        if (appMetaData.deviceName.empty()) {
            appMetaData.deviceName = storedAppMetaData.deviceName;
        }
        if (ER_OK != uiStorage->SetAppMetaData(app, appMetaData)) {
            cerr << "Failed to persist application name and/or About meta data." << endl;
        } else {
            cout << "Successfully persisted application name and/or About meta data." << endl;
        }
    } while (0);
}

static void add_group(const shared_ptr<UIStorage>& uiStorage,
                      const string& arg)
{
    vector<string> args = split(arg, '/');

    if (args.size() < 2) {
        cerr << "Please provide a group name and a description." << endl;
        return;
    }

    GroupInfo groupInfo;
    groupInfo.name = args[0].c_str();
    groupInfo.desc = args[1].substr(0, GROUP_DESC_MAX).c_str();

    if (ER_OK != uiStorage->StoreGroup(groupInfo)) {
        cerr << "Group was not added" << endl;
    } else {
        cout << "Group was successfully added" << endl;
        cout << groupInfo << endl;
    }
}

static void get_group(const shared_ptr<UIStorage>& uiStorage,
                      const string& arg)
{
    if (arg.empty()) {
        cout << "Empty group information" << endl;
        return;
    }

    GroupInfo groupInfo;
    GUID128 groupID(arg.substr(0, GROUP_ID_MAX).c_str());
    groupInfo.guid = groupID;

    if (ER_OK != uiStorage->GetGroup(groupInfo)) {
        cerr << "Group was not found" << endl;
    } else {
        cout << "Group was successfully retrieved" << endl;
        cout << groupInfo << endl;
    }
}

static void list_groups(const shared_ptr<UIStorage>& uiStorage)
{
    vector<GroupInfo> groups;

    if (ER_OK != uiStorage->GetGroups(groups)) {
        cerr << "Could not retrieve Groups or none were found" << endl;
    } else {
        cout << "Retrieved Group(s):" << endl;
        for (vector<GroupInfo>::const_iterator g = groups.begin();
             g != groups.end(); g++) {
            cout << *g << endl;
        }
    }
}

static void remove_group(const shared_ptr<UIStorage>& uiStorage,
                         const string& arg)
{
    if (arg.empty()) {
        cout << "Empty group information" << endl;
        return;
    }

    GUID128 groupID(arg.substr(0, GROUP_ID_MAX).c_str());

    GroupInfo group;
    group.guid = groupID;

    if (ER_OK != uiStorage->RemoveGroup(group)) {
        cerr << "Group was not found" << endl;
    } else {
        cout << "Group was successfully removed" << endl;
    }
}

static void update_membership(const shared_ptr<UIStorage>& uiStorage,
                              const string& arg, bool add)
{
    size_t delpos = arg.find_first_of(" ");

    if (arg.empty() || delpos == string::npos) {
        cerr << "Please provide an application id and group id." << endl;
        return;
    }

    string id = arg.substr(0, delpos).c_str();
    OnlineApplication app;

    if (!getKey(arg.substr(0, delpos), app.keyInfo) || ER_END_OF_DATA == uiStorage->GetManagedApplication(
            app)) {
        cerr << "Could not find application with id " << id << "." << endl;
        return;
    }

    GroupInfo groupInfo;
    groupInfo.guid = GUID128(arg.substr(delpos + 1, string::npos).c_str());

    if (ER_OK != uiStorage->GetGroup(groupInfo)) {
        cerr << "Could not find group with id " << groupInfo.guid.ToString() << "." << endl;
        return;
    }

    if (add) {
        uiStorage->InstallMembership(app, groupInfo);
    } else {
        uiStorage->RemoveMembership(app, groupInfo);
    }
}

static void install_policy(const shared_ptr<UIStorage>& uiStorage,
                           const PolicyGenerator& policyGenerator,
                           const string& arg)
{
    vector<string> args = split(arg, ' ');

    if (args.size() < 1) {
        cerr << "Please provide an application id." << endl;
        return;
    }

    OnlineApplication app;
    if (!getKey(args[0], app.keyInfo) || ER_END_OF_DATA == uiStorage->GetManagedApplication(app)) {
        cerr << "Could not find application." << endl;
        return;
    }

    vector<GroupInfo> groups;
    for (size_t i = 1; i < args.size(); ++i) {
        GUID128 groupID(args[i].c_str());
        GroupInfo groupInfo;
        groupInfo.guid = groupID;
        if (ER_OK != uiStorage->GetGroup(groupInfo)) {
            cerr << "Could not find group with id " << args[i] << endl;
            return;
        }
        groups.push_back(groupInfo);
    }

    PermissionPolicy policy;
    if (ER_OK != policyGenerator.DefaultPolicy(groups, policy)) {
        cerr << "Failed to generate default policy." << endl;
        return;
    }

    cout << "Generated the following policy:" << endl;
    cout << policy.ToString().c_str() << endl;

    if (ER_OK != uiStorage->UpdatePolicy(app, policy)) {
        cerr << "Failed to install policy." << endl;
        return;
    }
    cout << "Successfully installed policy." << endl;
}

static void get_policy(const shared_ptr<UIStorage>& uiStorage,
                       const string& arg)
{
    vector<string> args = split(arg, ' ');

    if (args.size() < 1) {
        cerr << "Please provide an application id." << endl;
        return;
    }

    OnlineApplication app;
    if (!getKey(args[0], app.keyInfo) || ER_END_OF_DATA == uiStorage->GetManagedApplication(app)) {
        cerr << "Could not find application." << endl;
        return;
    }
    PermissionPolicy policyLocal;

    if (ER_OK != uiStorage->GetPolicy(app, policyLocal)) {
        cerr << "Failed to get locally persisted policy." << endl;
        return;
    }

    cout << "Successfully retrieved locally persisted policy for " << args[0] << ":" << endl;
    cout << policyLocal.ToString() << endl;
}

static void reset_policy(const shared_ptr<UIStorage>& uiStorage,
                         const string& arg)
{
    vector<string> args = split(arg, ' ');
    if (args.size() < 1) {
        cerr << "Please provide an application id." << endl;
        return;
    }

    OnlineApplication app;
    if (!getKey(args[0], app.keyInfo)) {
        cerr << "Could not find application." << endl;
        return;
    }

    if (ER_OK != uiStorage->RemovePolicy(app)) {
        cerr << "Failed to reset policy." << endl;
        return;
    }

    cout << "Successfully reset policy for " << args[0] << endl;
}

static void handle_manifest_update(const shared_ptr<UIStorage>& uiStorage)
{
    manifestUpdatesLock.Lock();
    if (!manifestUpdates.size()) {
        cout << "No Manifest Updates in queue" << endl;
        manifestUpdatesLock.Unlock();
        return;
    }

    vector<ManifestUpdate>::iterator it = manifestUpdates.begin();
    ManifestUpdate manifestUpdate = *it;
    manifestUpdates.erase(it);
    manifestUpdatesLock.Unlock();

    cout << "  Manifest update" << endl;
    cout << "  ===============" << endl;

    cout << "  Application id   : " << addKeyID(manifestUpdate.app.keyInfo) << endl;
    ApplicationMetaData data;
    GetAboutInfo(manifestUpdate.app, data);
    if (data.appName != "") {
        cout << "  Application name : " << data.appName;
        cout << " (" << data.deviceName << ")" << endl;
    }
    cout << "  Bus name         : " << manifestUpdate.app.busName.c_str() << endl;

    cout << "  Additional rights: " << endl;
    cout << manifestUpdate.additionalRules.ToString().c_str() << endl;

    bool accept = false;
    cout << "Accept (y/n)? ";

    string input;
    getline(cin, input);

    char cmd;
    cmd = input[0];

    switch (cmd) {
    case 'y':
    case 'Y':
        accept = true;
        break;
    }

    IdentityInfo identity;
    if (!get_default_identity(uiStorage, identity)) {
        return;
    }

    OnlineApplication copy(manifestUpdate.app); // discard const for UpdateIdentity
    QStatus status;
    if (accept) {
        status = uiStorage->UpdateIdentity(copy, identity, manifestUpdate.newManifest);
        if (ER_OK != status) {
            cout << "Failed to update identity\n" << endl;
            return;
        } else {
            cout << "Successfully updated identity certificate\n" << endl;
        }
    }
}

static void blacklist_application(PolicyGenerator& policyGenerator,
                                  const string& arg)
{
    vector<string> args = split(arg, ' ');
    if (args.size() < 1) {
        cerr << "Please provide an application id." << endl;
        return;
    }

    OnlineApplication app;
    if (!getKey(args[0], app.keyInfo)) {
        cerr << "Could not find application." << endl;
        return;
    }

    policyGenerator.deniedKeys.push_back(app.keyInfo);
    cout << "Successfully added application to blacklist of policy generator." << endl;
}

static void help()
{
    cout << endl;
    cout << "  Supported commands:" << endl;
    cout << "  ===================" << endl;
    cout << "    q   Quit" << endl;
    cout << "    f   List all claimable applications" << endl;
    cout << "    c   Claim an application (appId)" << endl;
    cout << "    l   List all claimed applications" << endl;
    cout << "    g   Create a group (name/description)" << endl;
    cout << "    r   Remove a group (id)" << endl;
    cout << "    k   Get a group (id)" << endl;
    cout << "    p   List all groups" << endl;
    cout << "    m   Install a membership certificate (appId groupid)" << endl;
    cout << "    d   Delete a membership certificate (appId groupid)" << endl;
    cout << "    o   Install a policy (appId groupid1 groupid2 ...)" << endl;
    cout << "    e   Get policy (appId)" << endl;
    cout << "    s   Reset policy (appId)" << endl;
    cout << "    u   Unclaim an application (appId)" << endl;
    cout << "    a   Handle queued manifest update" << endl;
    cout << "    b   Blacklist an application in future policy updates" << endl;
    cout << "    n   Set a user defined name for an application (appId appname)." << endl <<
        "        This operation will also persist relevant About meta data if they exist." << endl;
    cout << "    h   Show this help message" << endl << endl;
}

static bool parse(shared_ptr<SecurityAgent>& secAgent,
                  const shared_ptr<UIStorage>& uiStorage,
                  PolicyGenerator& policyGenerator,
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
        list_claimable_applications(secAgent);
        break;

    case 'l':
        list_claimed_applications(uiStorage);
        break;

    case 'c':
        claim_application(secAgent, uiStorage, arg);
        break;

    case 'g':
        add_group(uiStorage, arg);
        break;

    case 'k':
        get_group(uiStorage, arg);
        break;

    case 'r':
        remove_group(uiStorage, arg);
        break;

    case 'p':
        list_groups(uiStorage);
        break;

    case 'm':
        update_membership(uiStorage, arg, true);
        break;

    case 'd':
        update_membership(uiStorage, arg, false);
        break;

    case 'o':
        install_policy(uiStorage, policyGenerator, arg);
        break;

    case 'e':
        get_policy(uiStorage, arg);
        break;

    case 'u':
        unclaim_application(uiStorage, arg);
        break;

    case 'n':
        set_app_meta_data_and_name(uiStorage, secAgent, arg);
        break;

    case 's':
        reset_policy(uiStorage, arg);
        break;

    case 'a':
        handle_manifest_update(uiStorage);
        break;

    case 'b':
        blacklist_application(policyGenerator, arg);
        break;

    case 'h':
    default:
        help();
        break;
    }

    return true;
}

int CDECL_CALL main(int argc, char** argv)
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);

    cout << "\n\n\n";
    cout << "\t##########################################################" << endl;
    cout << "\t#    _____                               _   _           #" << endl;
    cout << "\t#   (_____)   ____                 _    (_) (_)_         #" << endl;
    cout << "\t#  (_)___    (____)    ___  _   _ (_)__  _  (___) _   _  #" << endl;
    cout << "\t#    (___)_ (_)()(_)  (___)(_) (_)(____)(_) (_)  (_) (_) #" << endl;
    cout << "\t#    ____(_)(__)__  (_)___ (_)_(_)(_)   (_) (_)_ (_)_(_) #" << endl;
    cout << "\t#   (_____)  (____)  (____) (___) (_)   (_)  (__) (____) #" << endl;
    cout << "\t#                                                 __ (_) #" << endl;
    cout << "\t#                                                (___)   #" << endl;
    cout << "\t#                                                        #" << endl;
    cout << "\t#          _____                          _              #" << endl;
    cout << "\t#         (_____)          ____    _     (_)_            #" << endl;
    cout << "\t#        (_)___(_)  ____  (____)  (_)__  (___)           #" << endl;
    cout << "\t#        (_______) (____)(_)()(_) (____) (_)             #" << endl;
    cout << "\t#        (_)   (_)( )_(_)(__)__   (_) (_)(_)_            #" << endl;
    cout << "\t#        (_)   (_) (____) (____)  (_) (_) (__)           #" << endl;
    cout << "\t#                 (_)_(_)                                #" << endl;
    cout << "\t#                  (___)                                 #" << endl;
    cout << "\t#                                                        #" << endl;
    cout << "\t##########   Type h to display the help menu  ############" << endl;
    cout << "\n\n\n";

    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return EXIT_FAILURE;
    }
#endif

    StorageFactory& storageFactory = StorageFactory::GetInstance();

    shared_ptr<UIStorage> uiStorage;
    if (ER_OK != storageFactory.GetStorage("admin", uiStorage)) {
        cerr << "GetStorage call FAILED\n";
        return EXIT_FAILURE;
    }
    shared_ptr<AgentCAStorage> caStorage;
    if (ER_OK != uiStorage->GetCaStorage(caStorage)) {
        cerr << "No CaStorage\n";
        return EXIT_FAILURE;
        cerr << "GetStorage call FAILED\n";
        return EXIT_FAILURE;
    }

    BusAttachment* ba = new BusAttachment("Security Agent", true);
    ba->Start();
    ba->Connect();

    CliAboutListner* cliAboutListener = new CliAboutListner();
    ba->RegisterAboutListener(*cliAboutListener);

    /* Passing nullptr into WhoImplements will listen for all About announcements */
    if (ER_OK != ba->WhoImplements(nullptr)) {
        cerr << "WhoImplements call FAILED\n";
        return EXIT_FAILURE;
    }

    SecurityAgentFactory& secFac = SecurityAgentFactory::GetInstance();
    shared_ptr<SecurityAgent> secAgent(nullptr);

    if (ER_OK != secFac.GetSecurityAgent(caStorage, secAgent, ba)) {
        cerr
            << "> Error: Security Factory returned an invalid SecurityManager object !!"
            << endl;
        cerr << "> Exiting" << endl << endl;
        return EXIT_FAILURE;
    }

    secAgent->SetClaimListener(new CLIClaimListener());

    // Create policy generator
    GroupInfo adminGroup;
    uiStorage->GetAdminGroup(adminGroup);
    PolicyGenerator policyGenerator(adminGroup);

    // Activate live monitoring
    EventListener listener;
    secAgent->RegisterApplicationListener(&listener);

    // Create default identity
    vector<IdentityInfo> list;
    if (ER_OK != uiStorage->GetIdentities(list)) {
        cerr
            << "> Error: Failed to retrieve identities !!"
            << endl;
        cerr << "> Exiting" << endl << endl;
    }
    if (list.size() == 0) {
        IdentityInfo info;
        info.guid = GUID128("abcdef1234567890");
        info.name = "MyTestIdentity";
        if (ER_OK != uiStorage->StoreIdentity(info)) {
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
        done = !parse(secAgent, uiStorage, policyGenerator, input);
    }

    // Cleanup
    ba->UnregisterAboutListener(*cliAboutListener);
    delete cliAboutListener;
    cliAboutListener = nullptr;
    secAgent->UnregisterApplicationListener(&listener);
    secAgent = nullptr;
    ba->Disconnect();
    ba->Stop();
    ba->Join();
    delete ba;
    ba = nullptr;

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();

    cout << "Goodbye :)" << endl;
    return EXIT_SUCCESS;
}
