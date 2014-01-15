/**
 * @file
 * AllJoyn-Daemon serivce launcher file database class
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

#include <assert.h>

#include <qcc/FileStream.h>
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include "Bus.h"
#include "ConfigDB.h"
#include "ServiceDB.h"

using namespace ajn;
using namespace std;
using namespace qcc;


static QStatus ReadLine(FileSource& fs, qcc::String& line)
{
    char c;
    size_t read;
    QStatus status;
    line.clear();
    do {
        status = fs.PullBytes(&c, 1, read);
        if ((read == 0) || (status != ER_OK)) {
            break;
        }
        if ((c != '\n') && (c != '\r')) {
            line += c;
        }
    } while (c != '\n');

    return status;
}


bool _ServiceDB::ParseServiceFiles(qcc::String dir)
{
    QStatus status;
    DirListing dirList;
    DirListing::const_iterator it;

    status = GetDirListing(dir.c_str(), dirList);
    if (status != ER_OK) {
        return false;
    }

    for (it = dirList.begin(); it != dirList.end(); ++it) {
        // Skip ineligible entries.
        if ((it->size() < sizeof(".service")) ||
            (strcmp(it->c_str() + it->size() - (sizeof(".service") - 1), ".service") != 0)) {
            continue;
        }
        FileSource fs(dir + '/' + *it);
        qcc::String line;
        qcc::String name;
        list<qcc::String> execTokens;
        qcc::String user;
        while (ReadLine(fs, line) == ER_OK) {
            // Strip comments;
            size_t pos(line.find(';'));
            if (pos < line.npos) {
                line = line.substr(0, pos);
            }
            pos = line.find('=');
            if ((line[0] == '[') && (line[line.size() - 1] == ']')) {
                // section = line.substr(1, line.size() - 3);
            } else if (pos < line.npos) {
                qcc::String key = Trim(line.substr(0, pos));
                qcc::String value = Trim(line.substr(pos + 1));
                if (key.compare("Name") == 0) {
                    name = value;
                } else if (key.compare("Exec") == 0) {
                    ParseExecLine(value, execTokens);
                } else if (key.compare("User") == 0) {
                    user = value;
                }
            }
        }
        if (!name.empty() && !execTokens.empty()) {
            serviceMap[name].exec = execTokens.front();
            execTokens.pop_front();
            serviceMap[name].args.splice(serviceMap[name].args.begin(), execTokens);
            serviceMap[name].user = user;
        }
        Log(LOG_DEBUG, "Processed Service File: %s (name = %s  exec = \"%s\"  user = %s)\n",
            it->c_str(), name.c_str(), serviceMap[name].exec.c_str(), user.c_str());
    }
    return true;
}


QStatus _ServiceDB::BusStartService(const char* serviceName, ServiceStartListener* cb, const Bus* bus)
{
    ConfigDB* config(ConfigDB::GetConfigDB());
    QStatus status(ER_OK);
    Environ env(*Environ::GetAppEnviron());  // Create a fresh copy

    if (bus) {
        env.Add("DBUS_STARTER_TYPE", ConfigDB::GetConfigDB()->GetType());
        env.Add("DBUS_STARTER_ADDRESS", bus->GetLocalAddresses());
    }


    ServiceMap::iterator it(serviceMap.find(serviceName));
    if (it == serviceMap.end()) {
        status = ER_BUS_NO_SUCH_SERVICE;
    } else {
        if (it->second.waiting.empty()) {
            if (config->GetServicehelper().empty() || !bus) {
                Log(LOG_DEBUG, "Starting %s for service %s\n", it->second.exec.c_str(), serviceName);
                if (it->second.user.empty()) {
                    status = Exec(it->second.exec.c_str(), it->second.args, env);
                } else {
                    status = ExecAs(it->second.user.c_str(), it->second.exec.c_str(), it->second.args, env);
                }
            } else {
                ExecArgs args;
                Log(LOG_DEBUG, "Starting service helper for service %s\n", serviceName);
                args.push_back(serviceName);
                status = Exec(config->GetServicehelper().c_str(), args, env);
            }
        }

        if ((status == ER_OK) && (bus && cb)) {
            it->second.lock.Lock(MUTEX_CONTEXT);
            if (it->second.waiting.empty()) {
                uint32_t startTO = config->GetLimit("service_start_timeout");
                void* context = (void*)(new qcc::String(serviceName));
                timer.AddAlarm(Alarm(startTO, this, context));
            }
            it->second.waiting.push_back(cb);
            it->second.lock.Unlock(MUTEX_CONTEXT);
        }

    }
    return status;
}


void _ServiceDB::ParseExecLine(const qcc::String& execLine, list<qcc::String>& execTokens)
{
    size_t tokstart(0), tokend(0);;
    qcc::String token;
    bool backslash(false);
    bool singlequote(false);
    bool doublequote(false);

    while (tokstart < execLine.size()) {
        tokend = execLine.find_first_of("\"'\\ \t", tokstart);

        token += execLine.substr(tokstart, tokend - tokstart);
        if (tokend < execLine.size()) {
            switch (execLine[tokend]) {
            case '\\':
                if (backslash) {
                    token += execLine[tokend];
                    backslash = false;
                } else {
                    backslash = true;
                }
                break;

            case '"':
                if (backslash || singlequote) {
                    token += execLine[tokend];
                    backslash = false;
                } else {
                    doublequote = !doublequote;
                }
                break;

            case '\'':
                if (backslash || doublequote) {
                    token += execLine[tokend];
                    backslash = false;
                } else {
                    singlequote = !singlequote;
                }
                break;

            case ' ':
            case '\t':
                if (backslash || singlequote || doublequote) {
                    token += execLine[tokend];
                    backslash = false;
                } else {
                    execTokens.push_back(token);
                    token.clear();
                    tokend = execLine.find_first_not_of(" \t", tokend) - 1;
                }
            }
            ++tokend;
        }

        tokstart = tokend;
    }
    execTokens.push_back(token);
}

void _ServiceDB::NameOwnerChanged(const qcc::String& alias,
                                  const qcc::String* oldOwner,
                                  const qcc::String* newOwner)
{
    if (!oldOwner && newOwner) {
        map<StringMapKey, ServiceInfo>::iterator it(serviceMap.find(alias));
        if (it != serviceMap.end()) {
            it->second.lock.Lock(MUTEX_CONTEXT);
            list<ServiceStartListener*>::iterator cb(it->second.waiting.begin());
            while (cb != it->second.waiting.end()) {
                (*cb)->ServiceStarted(alias, ER_OK);
                it->second.waiting.erase(cb);
                cb = it->second.waiting.begin();
            }
            it->second.lock.Unlock(MUTEX_CONTEXT);
        }
    }
}


void _ServiceDB::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    qcc::String* serviceName(reinterpret_cast<qcc::String*>(alarm->GetContext()));

    map<StringMapKey, ServiceInfo>::iterator it(serviceMap.find(*serviceName));
    if (it != serviceMap.end()) {
        it->second.lock.Lock(MUTEX_CONTEXT);
        list<ServiceStartListener*>::iterator cb(it->second.waiting.begin());
        while (cb != it->second.waiting.end()) {
            (*cb)->ServiceStarted(*serviceName, ER_TIMEOUT);
            it->second.waiting.erase(cb);
            cb = it->second.waiting.begin();
        }
        it->second.lock.Unlock(MUTEX_CONTEXT);
    }

    delete serviceName;
}
