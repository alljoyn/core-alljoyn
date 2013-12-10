/**
 * @file
 * AllJoyn-Daemon Config file database class
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <errno.h>
#include <sys/types.h>
#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_GROUP_WINRT)
#include <pwd.h>
#endif

#include <list>
#include <map>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/FileStream.h>
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/XmlElement.h>

#include "ConfigDB.h"
#include "PolicyDB.h"
#include "ServiceDB.h"
#include "PropertyDB.h"

using namespace ajn;
using namespace qcc;
using namespace std;

ConfigDB* ConfigDB::GetConfigDB()
{
    static ConfigDB* singleton(NULL);

    if (!singleton) {
        singleton = new ConfigDB();
    }
    return singleton;
}


void ConfigDB::NameOwnerChanged(const qcc::String& alias,
                                const qcc::String* oldOwner,
                                const qcc::String* newOwner)
{
    db->policyDB->NameOwnerChanged(alias, oldOwner, newOwner);
    db->serviceDB->NameOwnerChanged(alias, oldOwner, newOwner);
}

ConfigDB::ConfigDB() : db(new DB()), stopping(false)
{
    db->limitMap[qcc::String("service_start_timeout")] = 10000;  // 10 seconds
}

ConfigDB::~ConfigDB()
{
    delete db;
}

bool ConfigDB::LoadConfigFile()
{
    if (stopping) {
        return false;
    }

    DB* newDb(new DB());
    bool success(newDb->ParseFile(configFile));

    if (success) {
        DB* old(db);
        db = newDb;
        delete old;
    } else {
        delete newDb;
    }
    return success;
}


bool ConfigDB::LoadSource(Source& src)
{
    bool success;
    DB* newDb = new DB();
    success = newDb->ParseSource("<built-in>", src);
    if (success) {
        DB* old = db;
        db = newDb;
        delete old;
    } else {
        delete newDb;
    }
    return success;
}


bool ConfigDB::DB::ParseSource(qcc::String fileName, Source& src)
{
    XmlParseContext xmlParseCtx(src);
    XmlElement& root = xmlParseCtx.root;
    bool success;

    Log(LOG_INFO, "Processing config file: %s\n", fileName.c_str());
    success = XmlElement::Parse(xmlParseCtx) == ER_OK;
    if (success) {
        if (root.GetName().compare("busconfig") == 0) {
            success = ProcessBusconfig(fileName, root);
        } else {
            Log(LOG_ERR, "Error processing \"%s\": Unknown tag found at top level: <%s>\n",
                fileName.c_str(), root.GetName().c_str());
            success = false;
        }
    } else {
        Log(LOG_ERR, "File \"%s\" contains invalid XML constructs.\n", fileName.c_str());
    }

    loaded = success;

    return success;
}


bool ConfigDB::DB::ParseFile(qcc::String fileName, bool ignore_missing)
{

    bool success(true);

    qcc::String expandedFileName;
    int position;

    /* Check if the file path contains tilde (~) */
    if (fileName[0] == '~') {
#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_GROUP_WINRT)
        qcc::String home(getenv("HOME"));
        /* If HOME is not set get the user from the present working directory */
        if (home.empty()) {
            struct passwd*pwd = getpwuid(getuid());
            if (pwd) {
                home = pwd->pw_dir;
            }
        }
        home += '/';
        /* Reconstruct the path from either case when HOME was set or not */
        position = fileName.find_first_of('/');
        expandedFileName = home + fileName.substr(position + 1);
#endif
    } else {
        expandedFileName = fileName;
    }

    FileSource fs(expandedFileName.c_str());

    if (fs.IsValid()) {
        success = ParseSource(expandedFileName.c_str(), fs);
    } else if (!ignore_missing) {
        Log(LOG_ERR, "Failed to open \"%s\": %s\n", expandedFileName.c_str(), strerror(errno));
        success = false;
    }

    return success;
}




bool ConfigDB::DB::ProcessAssociate(const qcc::String fileName, const XmlElement& associate)
{
    bool success = true;

    Log(LOG_DEBUG, "ProcessAssociate(\"%s\"): To be implemented...\n", associate.GetContent().c_str());

    return success;
}


bool ConfigDB::DB::ProcessAuth(const qcc::String fileName, const XmlElement& auth)
{
    static const char wspace[] = " \t\v\r\n";
    bool success = true;
    qcc::String mechanisms(auth.GetContent());
    size_t toks, toke;

    toks = mechanisms.find_first_not_of(wspace);

    while (toks != qcc::String::npos) {
        toke = mechanisms.find_first_of(wspace, toks);
        authList += mechanisms.substr(toks, toke - toks);
        authList.push_back(' ');
        toks = mechanisms.find_first_not_of(wspace, toke);
    }
    if (!authList.empty()) {
        authList.erase(authList.size() - 1);
    }
    return success;
}


bool ConfigDB::DB::ProcessBusconfig(const qcc::String fileName, const XmlElement& busconfig)
{
    bool success = true;
    const vector<XmlElement*>& elements = busconfig.GetChildren();
    vector<XmlElement*>::const_iterator it;

    for (it = elements.begin(); it != elements.end(); ++it) {
        Log(LOG_DEBUG, "Processing tag <%s> in \"%s\"...\n", (*it)->GetName().c_str(), fileName.c_str());

        if ((*it)->GetName().compare("alljoyn") == 0) {
            success = success && ProcessAlljoyn(fileName, *(*it));

        } else if ((*it)->GetName().compare("auth") == 0) {
            success = success && ProcessAuth(fileName, *(*it));

        } else if ((*it)->GetName().compare("fork") == 0) {
            success = success && ProcessFork(fileName, *(*it));

        } else if ((*it)->GetName().compare("include") == 0) {
            success = success && ProcessInclude(fileName, *(*it));

        } else if ((*it)->GetName().compare("includedir") == 0) {
            success = success && ProcessIncludedir(fileName, *(*it));

        } else if ((*it)->GetName().compare("keep_umask") == 0) {
            success = success && ProcessKeepUmask(fileName, *(*it));

        } else if ((*it)->GetName().compare("limit") == 0) {
            success = success && ProcessLimit(fileName, *(*it));

        } else if ((*it)->GetName().compare("listen") == 0) {
            success = success && ProcessListen(fileName, *(*it));

        } else if ((*it)->GetName().compare("pidfile") == 0) {
            success = success && ProcessPidfile(fileName, *(*it));

        } else if ((*it)->GetName().compare("policy") == 0) {
            success = success && ProcessPolicy(fileName, *(*it));

        } else if ((*it)->GetName().compare("selinux") == 0) {
            success = success && ProcessSELinux(fileName, *(*it));

        } else if ((*it)->GetName().compare("servicedir") == 0) {
            success = success && ProcessServicedir(fileName, *(*it));

        } else if ((*it)->GetName().compare("servicehelper") == 0) {
            success = success && ProcessServicehelper(fileName, *(*it));

        } else if ((*it)->GetName().compare("standard_session_servicedirs") == 0) {
            success = success && ProcessStandardSessionServicedirs(fileName, *(*it));

        } else if ((*it)->GetName().compare("standard_system_servicedirs") == 0) {
            success = success && ProcessStandardSystemServicedirs(fileName, *(*it));

        } else if ((*it)->GetName().compare("syslog") == 0) {
            success = success && ProcessSyslog(fileName, *(*it));

        } else if ((*it)->GetName().compare("type") == 0) {
            success = success && ProcessType(fileName, *(*it));

        } else if ((*it)->GetName().compare("user") == 0) {
            success = success && ProcessUser(fileName, *(*it));

        } else {
            Log(LOG_ERR, "Error processing \"%s\": Unknown tag found in <%s> block: <%s>\n",
                fileName.c_str(), busconfig.GetName().c_str(), (*it)->GetName().c_str());
            success = false;
        }
    }

    return success;
}


bool ConfigDB::DB::ProcessFork(const qcc::String fileName, const XmlElement& fork)
{
    bool success = true;

    this->fork = true;

#ifndef NDEBUG
    if (!fork.GetAttributes().empty() || !fork.GetChildren().empty() || !fork.GetContent().empty()) {
        Log(LOG_INFO, "Ignoring extraneous data with <%s> tag.\n", fork.GetName().c_str());
    }
#endif

    return success;
}


bool ConfigDB::DB::ProcessInclude(const qcc::String fileName, const XmlElement& include)
{
    bool success = true;
    qcc::String includeFileName(include.GetContent());
    const map<qcc::String, qcc::String>& attrs(include.GetAttributes());
    bool ignore_missing(false);

    if (includeFileName.empty()) {
        success = false;
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), include.GetName().c_str());
        goto exit;
    }

    if (!attrs.empty()) {
        map<qcc::String, qcc::String>::const_iterator it;
        for (it = attrs.begin(); it != attrs.end(); ++it) {
            if (it->first.compare("ignore_missing") == 0) {
                ignore_missing = (it->second.compare("yes") == 0);
            } else if (it->first.compare("if_selinux_enabled") == 0) {
                Log(LOG_NOTICE, "SELinux not yet supported.\n");
                goto exit;  // leave success unchanged.
            } else if (it->first.compare("selinux_root_relative") == 0) {
                Log(LOG_NOTICE, "SELinux not yet supported.\n");
                goto exit;  // leave success unchanged.
            } else {
                Log(LOG_ERR, "Error Processing \"%s\": Unknown attribute \"%s\" in tag <%s>.\n",
                    fileName.c_str(), it->first.c_str(), include.GetName().c_str());
                success = false;
                goto exit;
            }
        }
    }

    success = ParseFile(includeFileName, ignore_missing);

exit:
    return success;
}


bool ConfigDB::DB::ProcessIncludedir(const qcc::String fileName, const XmlElement& includedir)
{
    bool success(true);
    qcc::String includeDirectory(fileName.substr(0, fileName.find_last_of('/') + 1) + includedir.GetContent());
    const map<qcc::String, qcc::String>& attrs(includedir.GetAttributes());
    bool ignore_missing(false);
    DirListing listing;
    QStatus status;

    if (includeDirectory.empty()) {
        success = false;
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), includedir.GetName().c_str());
        goto exit;
    }

    if (!attrs.empty()) {
        map<qcc::String, qcc::String>::const_iterator it;
        for (it = attrs.begin(); it != attrs.end(); ++it) {
            if (it->first.compare("ignore_missing") == 0) {
                ignore_missing = (it->second.compare("yes") == 0);
            } else {
                Log(LOG_ERR, "Error processing \"%s\": Unknown attribute \"%s\" in tag <%s>.\n",
                    fileName.c_str(), it->first.c_str(), includedir.GetName().c_str());
                success = false;
                goto exit;
            }
        }
    }

    status = GetDirListing(includeDirectory.c_str(), listing);
    if (status != ER_OK) {
        if (!ignore_missing) {
            Log(LOG_ERR, "Error processing \"%s\": Failed to access directory \"%s\": %s\n",
                fileName.c_str(), includeDirectory.c_str(), strerror(errno));
            success = false;
        }
        goto exit;
    }

    for (DirListing::const_iterator it = listing.begin(); it != listing.end(); ++it) {
        if ((it->compare(".") != 0) && (it->compare("..") != 0)) {
            success = ParseFile(includeDirectory + "/" + *it);
        }
    }


exit:
    return success;
}


bool ConfigDB::DB::ProcessKeepUmask(const qcc::String fileName, const XmlElement& keepUmask)
{
    bool success = true;

    this->keepUmask = true;

#ifndef NDEBUG
    if (!keepUmask.GetAttributes().empty() || !keepUmask.GetChildren().empty() || !keepUmask.GetContent().empty()) {
        Log(LOG_INFO, "Ignoring extraneous data with <%s> tag.\n", keepUmask.GetName().c_str());
    }
#endif

    return success;
}


bool ConfigDB::DB::ProcessLimit(const qcc::String fileName, const XmlElement& limit)
{
    bool success = true;
    qcc::String name(limit.GetAttribute("name"));
    qcc::String valstr(limit.GetContent());
    uint32_t value;

    if (name.empty()) {
        Log(LOG_ERR, "Error processing \"%s\": 'name' attribute missing from <%s> tag.\n",
            fileName.c_str(), limit.GetName().c_str());
        success = false;
        goto exit;
    }

    if (valstr.empty()) {
        Log(LOG_ERR, "Error processing \"%s\": Value not specified for limit \"%s\".\n",
            fileName.c_str(), name.c_str());
        success = false;
        goto exit;
    }

    value = StringToU32(valstr);

    if ((value == 0) && (StringToU32(valstr.substr(0, 1)) != 0)) {
        Log(LOG_ERR, "Error processing \"%s\": Limit value for \"%s\" must be an unsigned 32 bit integer (not \"%s\").\n",
            fileName.c_str(), name.c_str(), valstr.c_str());
        success = false;
        goto exit;
    }

    limitMap[name] = value;

exit:
    return success;
}


bool ConfigDB::DB::ProcessListen(const qcc::String fileName, const XmlElement& listen)
{
    bool success = true;
    qcc::String addr(listen.GetContent());

    if (addr.empty()) {
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), listen.GetName().c_str());
        success = false;
    } else {
        if (listenList.find(addr) != listenList.end()) {
            Log(LOG_WARNING, "Warning processing \"%s\": Duplicate listen spec found (ignoring): %s\n",
                fileName.c_str(), addr.c_str());
        }
        listenList.insert(addr);
    }

    return success;
}


bool ConfigDB::DB::ProcessPidfile(const qcc::String fileName, const XmlElement& pidfile)
{
    bool success = true;

    this->pidfile = pidfile.GetContent();
    if (this->pidfile.empty()) {
        success = false;
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), pidfile.GetName().c_str());
    }

    return success;
}


bool ConfigDB::DB::ProcessPolicy(const qcc::String fileName, const XmlElement& policy)
{
    bool success = true;
    const vector<XmlElement*>& elements = policy.GetChildren();
    const map<qcc::String, qcc::String>& attrs(policy.GetAttributes());
    policydb::PolicyCategory cat;
    qcc::String catValue;

    if (attrs.size() == 1) {
        map<qcc::String, qcc::String>::const_iterator it(attrs.begin());
        catValue = it->second;
        if (it->first.compare("context") == 0) {
            if ((catValue.compare("default") != 0) && (catValue.compare("mandatory") != 0)) {
                Log(LOG_ERR, "Error processing \"%s\": Invalid context attribute for <%s> (must either be \"default\" or \"mandatory\"): \"%s\"\n",
                    fileName.c_str(), policy.GetName().c_str(), catValue.c_str());
                success = false;
                goto exit;
            }
            cat = policydb::POLICY_CONTEXT;
        } else if (it->first.compare("user") == 0) {
            cat = policydb::POLICY_USER;
        } else if (it->first.compare("group") == 0) {
            cat = policydb::POLICY_GROUP;
        } else if (it->first.compare("at_console") == 0) {
            cat = policydb::POLICY_AT_CONSOLE;
        } else {
            Log(LOG_ERR, "Error processing \"%s\": Unknown policy category: \"%s\"\n",
                fileName.c_str(), it->first.c_str());
            success = false;
            goto exit;
        }
    } else {
        Log(LOG_ERR, "Error processing \"%s\": Exactly one policy category must be specified.\n",
            fileName.c_str());
        success = false;
        goto exit;
    }

    for (vector<XmlElement*>::const_iterator it = elements.begin(); it != elements.end(); ++it) {
        Log(LOG_DEBUG, "Processing tag <%s> in \"%s\"...\n",
            (*it)->GetName().c_str(), fileName.c_str());

        if ((*it)->GetName().compare("allow") == 0) {
            success = success && policyDB->AddRule(cat, catValue, policydb::POLICY_ALLOW,
                                                   (*it)->GetAttributes());
        } else if ((*it)->GetName().compare("deny") == 0) {
            success = success && policyDB->AddRule(cat, catValue, policydb::POLICY_DENY,
                                                   (*it)->GetAttributes());
        } else {
            Log(LOG_ERR, "Error processing \"%s\": Unknown tag found in <%s> block: <%s>\n",
                fileName.c_str(), policy.GetName().c_str(), (*it)->GetName().c_str());
            success = false;
            goto exit;
        }
    }

exit:
    return success;
}


bool ConfigDB::DB::ProcessAlljoyn(const qcc::String fileName, const XmlElement& alljoyn)
{
    if (alljoyn.GetAttributes().size() != 1) {
        Log(LOG_ERR, "Error processing \"%s\": Exactly one alljoyn module must be specified.\n", fileName.c_str());
        return false;
    }

    qcc::String module = alljoyn.GetAttributes().begin()->second;

    const vector<XmlElement*>& elements = alljoyn.GetChildren();
    for (vector<XmlElement*>::const_iterator i = elements.begin(); i != elements.end(); ++i) {
        if ((*i)->GetName() != "property") {
            Log(LOG_ERR, "Error processing \"%s\": Unknown tag found in <%s> block: <%s>\n",
                fileName.c_str(), alljoyn.GetName().c_str(), (*i)->GetName().c_str());
            return false;
        }

        const map<qcc::String, qcc::String>& propertyAttrs = (*i)->GetAttributes();
        for (map<qcc::String, qcc::String>::const_iterator j = propertyAttrs.begin(); j != propertyAttrs.end(); ++j) {
            propertyDB->Set(module, j->first, j->second);
        }
    }

    return true;
}


bool ConfigDB::DB::ProcessSELinux(const qcc::String fileName, const XmlElement& selinux)
{
    bool success = true;

    Log(LOG_DEBUG, "ProcessSELinux(\"%s\"): To be implemented...\n", selinux.GetContent().c_str());

    return success;
}


bool ConfigDB::DB::ProcessServicedir(const qcc::String fileName, const XmlElement& servicedir)
{
    bool success = true;
    qcc::String dir(servicedir.GetContent());

    if (dir.empty()) {
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), servicedir.GetName().c_str());
        success = false;
    } else {
        this->serviceDir = dir;
        success = serviceDB->ParseServiceFiles(dir);
    }

    return success;
}


bool ConfigDB::DB::ProcessServicehelper(const qcc::String fileName, const XmlElement& servicehelper)
{
    bool success = true;
    qcc::String helper(servicehelper.GetContent());

    if (helper.empty()) {
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), servicehelper.GetName().c_str());
        success = false;
    } else {
        this->serviceHelper = helper;
    }

    return success;
}


bool ConfigDB::DB::ProcessStandardSessionServicedirs(const qcc::String fileName, const XmlElement& standardSessionServicedirs)
{
    bool success;

    // TODO: The default should be defined via a compile time default
    // specified by the build system to be appropriate for the target
    // platform.
    serviceDir = "/usr/share/dbus-1/services";
    success = serviceDB->ParseServiceFiles(serviceDir);

    return success;
}


bool ConfigDB::DB::ProcessStandardSystemServicedirs(const qcc::String fileName, const XmlElement& standardSystemServicedirs)
{
    bool success;

    // TODO: The default should be defined via a compile time default
    // specified by the build system to be appropriate for the target
    // platform.
    serviceDir = "/usr/share/dbus-1/system-services";
    success = serviceDB->ParseServiceFiles(serviceDir);

    return success;
}


bool ConfigDB::DB::ProcessSyslog(const qcc::String fileName, const XmlElement& syslog)
{
    bool success = true;

    this->syslog = true;

#ifndef NDEBUG
    if (!syslog.GetAttributes().empty() || !syslog.GetChildren().empty() || !syslog.GetContent().empty()) {
        Log(LOG_INFO, "Ignoring extraneous data with <%s> tag.\n", syslog.GetName().c_str());
    }
#endif

    return success;
}


bool ConfigDB::DB::ProcessType(const qcc::String fileName, const XmlElement& type)
{
    bool success = true;

    this->type = type.GetContent();
    if (this->type.empty()) {
        success = false;
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), type.GetName().c_str());
    }

    return success;
}


bool ConfigDB::DB::ProcessUser(const qcc::String fileName, const XmlElement& user)
{
    bool success = true;

    this->user = user.GetContent();
    if (this->user.empty()) {
        success = false;
        Log(LOG_ERR, "Error processing \"%s\": <%s> block is empty.\n",
            fileName.c_str(), user.GetName().c_str());
    }

    return success;
}
