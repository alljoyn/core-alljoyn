/**
 * @file
 * AllJoyn-Daemon service launcher helper - POSIX version
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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
#ifndef QCC_OS_ANDROID
#include <pwd.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include <alljoyn/version.h>
#include <qcc/Environ.h>
#include <qcc/FileStream.h>
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <alljoyn/Status.h>

#include "../BusController.h"
#include "../ConfigDB.h"
#include "../ServiceDB.h"


#define dbg(_fmt, _args ...) fprintf(stderr, "DBG %s %s[%u]: " _fmt "\n", __FILE__, __PRETTY_FUNCTION__, __LINE__, ## _args)
#ifndef dbg
#define dbg(_fmt, _args ...) do { } while (0)
#endif

#ifndef SYSTEM_CONF
// Default location of the system.conf file - the dbus-daemon-launch-helper has this path hard coded.
#define SYSTEM_CONF "/etc/dbus-1/system.conf"
#endif


#define DAEMON_EXIT_OK            0
#define DAEMON_EXIT_OPTION_ERROR  1
#define DAEMON_EXIT_CONFIG_ERROR  2
#define DAEMON_EXIT_LAUNCH_ERROR  3


using namespace ajn;
using namespace qcc;
using namespace std;


class OptParse {
  public:
    enum ParseResultCode {
        PR_OK,
        PR_EXIT_NO_ERROR,
        PR_TOO_MANY_ARGS,
        PR_MISSING_OPTION
    };

    OptParse(int argc, char** argv) : argc(0), argv(NULL) { }

    ParseResultCode ParseResult();

    qcc::String GetConfigFile() const { return SYSTEM_CONF; }
    qcc::String GetServiceName() const { return serviceName; }

  private:
    int argc;
    char** argv;

    qcc::String serviceName;

    void PrintUsage();
};


void OptParse::PrintUsage()
{
    fprintf(stderr, "%s <service.to.activate>\n\n", argv[0]);
}


OptParse::ParseResultCode OptParse::ParseResult()
{
    ParseResultCode result(PR_OK);

    if (argc == 1) {
        result = PR_MISSING_OPTION;
    } else if (argc > 2) {
        result = PR_TOO_MANY_ARGS;
    } else {
        qcc::String arg(argv[1]);

        if (arg.compare("--version") == 0) {
            printf("AllJoyn Message Bus Daemon service launcher helper version: %s\n"
                   "Copyright (c) 2009-2013 AllSeen Alliance.\n"
                   "\n"
                   "\n"
                   "Build: %s\n", GetVersion(), GetBuildInfo());
            result = PR_EXIT_NO_ERROR;
        } else {
            serviceName = arg;
        }
    }

    switch (result) {
    case PR_TOO_MANY_ARGS:
        fprintf(stderr, "Too many args\n");
        PrintUsage();
        break;

    case PR_MISSING_OPTION:
        fprintf(stderr, "No service to activate specified.\n");
        PrintUsage();
        break;

    default:
        break;
    }
    return result;
}


int main(int argc, char** argv, char** env)
{
#ifdef QCC_OS_ANDROID
    // Initialize the environment for Andriod
    environ = env;
#endif

    LoggerSetting* loggerSettings(LoggerSetting::GetLoggerSetting(argv[0], LOG_WARNING, true, NULL));
    loggerSettings->SetSyslog(false);
    loggerSettings->SetFile(stdout);

    OptParse opts(argc, argv);
    OptParse::ParseResultCode parseCode(opts.ParseResult());
    ConfigDB* config(ConfigDB::GetConfigDB());
    QStatus status;

    switch (parseCode) {
    case OptParse::PR_OK:
        break;

    case OptParse::PR_EXIT_NO_ERROR:
        delete config;
        return DAEMON_EXIT_OK;

    default:
        delete config;
        return DAEMON_EXIT_OPTION_ERROR;
    }

    config->SetConfigFile(opts.GetConfigFile());
    if (!config->LoadConfigFile()) {
        delete config;
        return DAEMON_EXIT_CONFIG_ERROR;
    }

    ServiceDB serviceDB(config->GetServiceDB());

    status = serviceDB->BusStartService(opts.GetServiceName().c_str());

    delete config;
    return (status == ER_OK) ? DAEMON_EXIT_OK : DAEMON_EXIT_LAUNCH_ERROR;
}
