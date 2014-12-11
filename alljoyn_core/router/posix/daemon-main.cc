/**
 * @file
 * AllJoyn-Daemon - POSIX version
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
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include <qcc/String.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include <qcc/Environ.h>
#include <qcc/FileStream.h>
#include <qcc/Log.h>
#include <qcc/Logger.h>
#include <qcc/Util.h>

#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#include "Transport.h"
#include "TCPTransport.h"
#include "UDPTransport.h"
#include "DaemonTransport.h"
#if defined(QCC_OS_LINUX)
#include "DaemonSLAPTransport.h"
#endif

#if defined(QCC_OS_ANDROID)
//#include "android/WFDTransport.h"
#endif

#include "Bus.h"
#include "BusController.h"
#include "ConfigDB.h"

#if !defined(ROUTER_LIB)

#if defined(QCC_OS_LINUX) || defined(QCC_OS_ANDROID)
#include <sys/prctl.h>
#include <linux/capability.h>
extern "C" {
extern int capset(cap_user_header_t hdrp, const cap_user_data_t datap);
}
#endif

#endif

#define DAEMON_EXIT_OK            0
#define DAEMON_EXIT_OPTION_ERROR  1
#define DAEMON_EXIT_CONFIG_ERROR  2
#define DAEMON_EXIT_STARTUP_ERROR 3
#define DAEMON_EXIT_FORK_ERROR    4
#define DAEMON_EXIT_IO_ERROR      5
#define DAEMON_EXIT_SESSION_ERROR 6
#define DAEMON_EXIT_CHDIR_ERROR   7

using namespace ajn;
using namespace qcc;
using namespace std;

static volatile sig_atomic_t reload;
static volatile sig_atomic_t quit;

/*
 * Simple config to provide some non-default limits for the daemon tcp/udp transport.
 */
#if defined(QCC_OS_ANDROID)
static const char defaultConfig[] =
    "<busconfig>"
    "  <limit name=\"auth_timeout\">20000</limit>"
    "  <limit name=\"max_incomplete_connections\">16</limit>"
    "  <limit name=\"max_completed_connections\">32</limit>"
    "  <limit name=\"max_untrusted_clients\">0</limit>"
    "  <flag name=\"restrict_untrusted_clients\">true</flag>"
    "</busconfig>";
#else
static const char defaultConfig[] =
    "<busconfig>"
    "  <limit name=\"auth_timeout\">20000</limit>"
    "  <limit name=\"max_incomplete_connections\">16</limit>"
    "  <limit name=\"max_completed_connections\">32</limit>"
    "  <limit name=\"max_untrusted_clients\">16</limit>"
    "  <flag name=\"restrict_untrusted_clients\">false</flag>"
    "</busconfig>";
#endif

static const char internalConfig[] =
    "<busconfig>"
    "  <type>alljoyn</type>"
    "  <listen>unix:abstract=alljoyn</listen>"
#if defined(QCC_OS_DARWIN)
    "  <listen>launchd:env=DBUS_LAUNCHD_SESSION_BUS_SOCKET</listen>"
#endif
    "  <listen>tcp:iface=*,port=9955</listen>"
    "  <listen>udp:iface=*,port=9955</listen>"
    "</busconfig>";

static const char versionPreamble[] =
    "AllJoyn Message Bus Daemon version: %s\n"
    "Copyright (c) 2009-2014 AllSeen Alliance.\n"
    "\n"
    "Build: %s\n";

void SignalHandler(int sig) {
    switch (sig) {
    case SIGHUP:
        if (!reload) {
            reload = 1;
        }
        break;

    case SIGINT:
    case SIGTERM:
        quit = 1;
        break;
    }
}

class OptParse {
  public:
    enum ParseResultCode {
        PR_OK,
        PR_EXIT_NO_ERROR,
        PR_OPTION_CONFLICT,
        PR_INVALID_OPTION,
        PR_MISSING_OPTION
    };

    OptParse(int argc, char** argv) :
        argc(argc), argv(argv),
        fork(false), noFork(false),
        noSLAP(false),
        noTCP(false),
        noUDP(false),
#if defined(QCC_OS_ANDROID)
        noWFD(false),
#endif
#if defined(QCC_OS_DARWIN)
        noLaunchd(false),
#endif
        noSwitchUser(false),
        printAddressFd(-1), printPidFd(-1),
        internal(false),
        configService(false),
        verbosity(LOG_WARNING) {
    }

    ParseResultCode ParseResult();

    String GetConfigFile() const {
        return configFile;
    }
    bool GetFork() const {
        return fork;
    }
    bool GetNoFork() const {
        return noFork;
    }
    bool GetNoSLAP() const {
        return noSLAP;
    }
    bool GetNoTCP() const {
        return noTCP;
    }
    bool GetNoUDP() const {
        return noUDP;
    }
#if defined(QCC_OS_ANDROID)
    bool GetNoWFD() const {
        return noWFD;
    }
#endif
#if defined(QCC_OS_DARWIN)
    bool GetNoLaunchd() const {
        return noLaunchd;
    }
#endif
    bool GetNoSwitchUser() const {
        return noSwitchUser;
    }
    int GetPrintAddressFd() const {
        return printAddressFd;
    }
    int GetPrintPidFd() const {
        return printPidFd;
    }
    int GetVerbosity() const {
        return verbosity;
    }
    bool GetInternalConfig() const {
        return internal;
    }
    bool GetServiceConfig() const {
        return configService;
    }

  private:
    int argc;
    char** argv;

    String configFile;
    bool fork;
    bool noFork;
    bool noSLAP;
    bool noTCP;
    bool noUDP;
#if defined(QCC_OS_ANDROID)
    bool noWFD;
#endif
#if defined(QCC_OS_DARWIN)
    bool noLaunchd;
#endif
    bool noSwitchUser;
    int printAddressFd;
    int printPidFd;
    bool internal;
    bool configService;
    int verbosity;

    void PrintUsage();
};

void OptParse::PrintUsage() {
    String cmd = argv[0];
    cmd = cmd.substr(cmd.find_last_of('/') + 1);

    fprintf(
        stderr,
        "%s [--session | --system | --internal | --config-file=FILE"
#if defined(QCC_OS_ANDROID) && defined(ROUTER_LIB)
        " | --config-service"
#endif
        "]\n"
        "%*s [--print-address[=DESCRIPTOR]] [--print-pid[=DESCRIPTOR]]\n"
        "%*s [--fork | --nofork] "
        "[--no-slap] [--no-tcp] [--no-udp] "
#if defined(QCC_OS_ANDROID)
        "[--no-wfd] "
#endif
#if defined(QCC_OS_DARWIN)
        "[--no-launchd]"
#endif
        "\n%*s [--no-switch-user] [--verbosity=LEVEL] [--version]\n\n"
        "    --session\n"
        "        Use the standard configuration for the per-login-session message bus.\n\n"
        "    --system\n"
        "        Use the standard configuration for the system message bus.\n\n"
        "    --internal\n"
        "        Use a basic internally defined message bus for AllJoyn.\n\n"
#if defined(QCC_OS_ANDROID) && defined(ROUTER_LIB)
        "    --config-service\n"
        "        Use a configuration passed from the calling service.\n\n"
#endif
        "    --config-file=FILE\n"
        "        Use the specified configuration file.\n\n"
        "    --print-address[=DESCRIPTOR]\n"
        "        Print the socket address to stdout or the specified descriptor\n\n"
        "    --print-pid[=DESCRIPTOR]\n"
        "        Print the process ID to stdout or the specified descriptor\n\n"
        "    --fork\n"
        "        Force the daemon to fork and run in the background.\n\n"
        "    --nofork\n"
        "        Force the daemon to only run in the foreground (override config file\n"
        "        setting).\n\n"
        "    --no-slap\n"
        "        Disable the SLAP transport (override config file setting).\n\n"
        "    --no-tcp\n"
        "        Disable the TCP transport (override config file setting).\n\n"
        "    --no-udp\n"
        "        Disable the UDP transport (override config file setting).\n\n"
#if defined(QCC_OS_ANDROID)
        "    --no-wfd\n"
        "        Disable the Wifi-Direct transport (override config file setting).\n\n"
#endif
#if defined(QCC_OS_DARWIN)
        "    --no-launchd\n"
        "        Disable the Launchd transport (override config file setting).\n\n"
#endif
#if defined(QCC_OS_LINUX)
        "    --no-switch-user\n"
        "        Don't switch from root to the user specified in the config file.\n\n"
#endif
        "    --verbosity=LEVEL\n"
        "        Set the logging level to LEVEL.\n\n"
        "    --version\n"
        "        Print the version and copyright string, and exit.\n",
        cmd.c_str(), static_cast<int> (cmd.size()), "",
        static_cast<int> (cmd.size()), "", static_cast<int> (cmd.size()),
        "");
}


OptParse::ParseResultCode OptParse::ParseResult()
{
    ParseResultCode result = PR_OK;
    int i = 0;

    if (argc == 1) {
        goto exit;
    }

    for (i = 1; i < argc; ++i) {
        String arg(argv[i]);

        if (arg.compare("--version") == 0) {
            printf(versionPreamble, GetVersion(), GetBuildInfo());
            result = PR_EXIT_NO_ERROR;
            goto exit;
        } else if (arg.compare("--session") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            configFile = "/etc/dbus-1/session.conf";
        } else if (arg.compare("--system") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            configFile = "/etc/dbus-1/system.conf";
        } else if (arg.compare("--internal") == 0) {
            if (!configFile.empty()) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            internal = true;
        } else if (arg.compare("--config-file") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            ++i;
            if (i == argc) {
                result = PR_MISSING_OPTION;
                goto exit;
            }
            configFile = argv[i];
        } else if (arg.compare(0, sizeof("--config-file") - 1, "--config-file") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            configFile = arg.substr(sizeof("--config-file"));
#if defined(QCC_OS_ANDROID) && defined(ROUTER_LIB)
        } else if (arg.compare(0, sizeof("--config-service") - 1, "--config-service") == 0) {
            if (!configFile.empty() || internal) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            configService = true;
#endif
        } else if (arg.compare(0, sizeof("--print-address") - 1,
                               "--print-address") == 0) {
            if (arg[sizeof("--print-address") - 1] == '=') {
                printAddressFd = StringToI32(arg.substr(
                                                 sizeof("--print-address")), 10, -2);
            } else {
                if (((i + 1) == argc) || ((argv[i + 1][0] == '-') && (argv[i
                                                                           + 1][1] == '-'))) {
                    printAddressFd = STDERR_FILENO;
                } else {
                    ++i;
                    printAddressFd = StringToI32(argv[i], 10, -2);
                }
            }
            if (printAddressFd < -1) {
                result = PR_INVALID_OPTION;
                goto exit;
            }
        } else if (arg.substr(0, sizeof("--print-pid") - 1).compare(
                       "--print-pid") == 0) {
            if (arg[sizeof("--print-pid") - 1] == '=') {
                printPidFd = StringToI32(arg.substr(sizeof("--print-pid")), 10,
                                         -2);
            } else {
                if (((i + 1) == argc) || ((argv[i + 1][0] == '-') && (argv[i
                                                                           + 1][1] == '-'))) {
                    printPidFd = STDERR_FILENO;
                } else {
                    ++i;
                    printPidFd = StringToI32(argv[i], 10, -2);
                }
            }
            if (printPidFd < -1) {
                result = PR_INVALID_OPTION;
                goto exit;
            }
        } else if (arg.compare("--fork") == 0) {
            if (noFork) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            fork = true;
        } else if (arg.compare("--nofork") == 0) {
            if (fork) {
                result = PR_OPTION_CONFLICT;
                goto exit;
            }
            noFork = true;
        } else if (arg.compare("--no-bt") == 0) {
            // Obsolete - kept for backwards compatibility
        } else if (arg.compare("--no-slap") == 0) {
            noSLAP = true;
        } else if (arg.compare("--no-tcp") == 0) {
            noTCP = true;
        } else if (arg.compare("--no-udp") == 0) {
            noUDP = true;
#if defined(QCC_OS_ANDROID)
        } else if (arg.compare("--no-wfd") == 0) {
            noWFD = true;
#endif
#if defined(QCC_OS_DARWIN)
        } else if (arg.compare("--no-launchd") == 0) {
            noLaunchd = true;
#endif
        } else if (arg.compare("--no-switch-user") == 0) {
            noSwitchUser = true;
        } else if (arg.substr(0, sizeof("--verbosity") - 1).compare(
                       "--verbosity") == 0) {
            verbosity = StringToI32(arg.substr(sizeof("--verbosity")));
        } else if ((arg.compare("--help") == 0) || (arg.compare("-h") == 0)) {
            PrintUsage();
            result = PR_EXIT_NO_ERROR;
            goto exit;
        } else {
            result = PR_INVALID_OPTION;
            goto exit;
        }
    }

exit:

    internal = configFile.empty();

    switch (result) {
    case PR_OPTION_CONFLICT:
        fprintf(stderr,
                "Option \"%s\" is in conflict with a previous option.\n",
                argv[i]);
        break;

    case PR_INVALID_OPTION:
        fprintf(stderr, "Invalid option: \"%s\"\n", argv[i]);
        break;

    case PR_MISSING_OPTION:
        fprintf(stderr, "No config file specified.\n");
        PrintUsage();
        break;

    default:
        break;
    }
    return result;
}

int daemon(OptParse& opts) {
    struct sigaction act, oldact;
    sigset_t sigmask, waitmask;
    ConfigDB* config = ConfigDB::GetConfigDB();

    // block all signals by default for all threads
    sigfillset(&sigmask);
    sigdelset(&sigmask, SIGSEGV);
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_RESTART;

    sigaction(SIGHUP, &act, &oldact);
    sigaction(SIGINT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);

    /*
     * Extract the listen specs
     */
    const ConfigDB::ListenList listenList = config->GetListen();
    String listenSpecs;

    for (ConfigDB::_ListenList::const_iterator it = listenList->begin(); it != listenList->end(); ++it) {
        String addrStr = *it;
        bool skip = false;
        if (addrStr.compare(0, sizeof("unix:") - 1, "unix:") == 0) {
            if (addrStr.compare(sizeof("unix:") - 1, sizeof("tmpdir=") - 1, "tmpdir=") == 0) {
                // Process tmpdir specially.
                String randStr = addrStr.substr(sizeof("unix:tmpdir=") - 1) + "/alljoyn-";
                addrStr = ("unix:abstract=" + RandomString(randStr.c_str()));
            }
            if (config->GetType() == "system") {
                // Add the system bus unix address to the app's environment.
                Environ* env = Environ::GetAppEnviron();
                const String var("DBUS_SYSTEM_BUS_ADDRESS");
                env->Add(var, addrStr);
            }

#if defined(QCC_OS_DARWIN)
        } else if (addrStr.compare(0, sizeof("launchd:") - 1, "launchd:") == 0) {
            skip = opts.GetNoLaunchd();
#endif

        } else if (addrStr.compare(0, sizeof("tcp:") - 1, "tcp:") == 0) {
            skip = opts.GetNoTCP();
        } else if (addrStr.compare(0, sizeof("udp:") - 1, "udp:") == 0) {
            skip = opts.GetNoUDP();

#if defined(QCC_OS_ANDROID)
        } else if (addrStr.compare(0, sizeof("wfd:") - 1, "wfd:") == 0) {
            skip = opts.GetNoWFD();
#endif
        } else if (addrStr.compare(0, sizeof("slap:") - 1, "slap:") == 0) {
            skip = opts.GetNoSLAP();

        } else {
            Log(LOG_ERR, "Unsupported listen address: %s (ignoring)\n", addrStr.c_str());
            continue;
        }

        if (skip) {
            Log(LOG_INFO, "Skipping transport for address: %s\n", addrStr.c_str());
        } else {
            Log(LOG_INFO, "Setting up transport for address: %s\n", addrStr.c_str());
            if (!listenSpecs.empty()) {
                listenSpecs.append(';');
            }
            listenSpecs.append(addrStr);
        }
    }

    if (listenSpecs.empty()) {
        Log(LOG_ERR, "No listen address specified.  Aborting...\n");
        return DAEMON_EXIT_CONFIG_ERROR;
    }

    // Do the real AllJoyn work here
    QStatus status;

    TransportFactoryContainer cntr;
    cntr.Add(new TransportFactory<DaemonTransport>(DaemonTransport::TransportName, false));
    cntr.Add(new TransportFactory<TCPTransport>(TCPTransport::TransportName, false));
    cntr.Add(new TransportFactory<UDPTransport>(UDPTransport::TransportName, false));
#if defined(QCC_OS_LINUX)
    cntr.Add(new TransportFactory<DaemonSLAPTransport>(DaemonSLAPTransport::TransportName, false));
#endif
#if defined(QCC_OS_ANDROID)
//    cntr.Add(new TransportFactory<WFDTransport>(WFDTransport::TransportName, false));
#endif

    Bus ajBus("alljoyn-daemon", cntr, listenSpecs.c_str());

    /*
     * Check we have at least one authentication mechanism registered.
     */
    if (!config->GetAuth().empty()) {
        if (ajBus.GetInternal().FilterAuthMechanisms(config->GetAuth()) == 0) {
            Log(LOG_ERR, "No supported authentication mechanisms.  Aborting...\n");
            return DAEMON_EXIT_STARTUP_ERROR;
        }
    }
    /*
     * Create the bus controller use it to initialize and start the bus.
     */
    BusController ajBusController(ajBus);
    status = ajBusController.Init(listenSpecs);
    if (ER_OK != status) {
        Log(LOG_ERR, "Failed to initialize BusController: %s\n", QCC_StatusText(status));
        return DAEMON_EXIT_STARTUP_ERROR;
    }

    int fd;
    String pidfn = config->GetPidfile();

    fd = opts.GetPrintAddressFd();
    if (fd >= 0) {
        String localAddrs(ajBus.GetLocalAddresses());
        localAddrs += "\n";
        int ret = write(fd, localAddrs.c_str(), localAddrs.size());
        if (ret == -1) {
            Log(LOG_ERR, "Failed to print address string: %s\n",
                strerror(errno));
        }
    }

    sigfillset(&waitmask);
    sigdelset(&waitmask, SIGHUP);
    sigdelset(&waitmask, SIGINT);
    sigdelset(&waitmask, SIGTERM);

    quit = 0;
    while (!quit) {
        reload = 0;
        sigsuspend(&waitmask);
        if (reload && !opts.GetInternalConfig()) {
            if (!config->LoadConfig(&ajBus)) {
                Log(LOG_ERR, "Failed to load the configuration - problem with %s.\n", opts.GetConfigFile().c_str());
            }
        }
    }

    Log(LOG_INFO, "Terminating.\n");
    ajBus.StopListen(listenSpecs.c_str());

    return DAEMON_EXIT_OK;
}

//
// This code can be run as a native executable, in which case the linker arranges to
// call main(), or it can be run as an Android Service.  In this case, the daemon
// is implemented as a static library which is linked into a JNI dynamic library and
// called from the Java service code.
//
#if defined(ROUTER_LIB)
int DaemonMain(int argc, char** argv, char* serviceConfig)
#else
int main(int argc, char** argv, char** env)
#endif
{
#if defined(QCC_OS_ANDROID) && !defined(ROUTER_LIB)
    //
    // Initialize the environment for Android if we use the command line to
    // run the daemon.  If we are using the Android service launcher, there is
    // no environment there, so we use arguments and a passed-in config file
    // which we deal with in a bit.
    //
    environ = env;
#endif

    LoggerSetting* loggerSettings = LoggerSetting::GetLoggerSetting(argv[0], LOG_WARNING, true, NULL);

    OptParse opts(argc, argv);
    OptParse::ParseResultCode parseCode = opts.ParseResult();

    switch (parseCode) {
    case OptParse::PR_OK:
        break;

    case OptParse::PR_EXIT_NO_ERROR:
        return DAEMON_EXIT_OK;

    default:
        return DAEMON_EXIT_OPTION_ERROR;
    }

    loggerSettings->SetLevel(opts.GetVerbosity());

    if (opts.GetNoFork()) {
        // Presumably the user wants logging to go to stderr.
        loggerSettings->SetSyslog(false);
        loggerSettings->SetFile(stderr);
    }

    String configStr = defaultConfig;
#if defined(QCC_OS_ANDROID) && defined(ROUTER_LIB)
    configStr.append(opts.GetServiceConfig() ? serviceConfig : internalConfig);
#else
    if (opts.GetInternalConfig()) {
        configStr.append(internalConfig);
    }
#endif

    ConfigDB config(configStr, opts.GetConfigFile());
    if (!config.LoadConfig()) {
        const char* errsrc;
        if (opts.GetInternalConfig()) {
            errsrc = "internal default config";
        } else {
            errsrc = opts.GetConfigFile().c_str();
        }
        Log(LOG_ERR, "Failed to load the configuration - problem with %s.\n", errsrc);
        return DAEMON_EXIT_CONFIG_ERROR;
    }

    loggerSettings->SetSyslog(config.GetSyslog());
    loggerSettings->SetFile((opts.GetFork() || (config.GetFork() && !opts.GetNoFork())) ? NULL : stderr);

    if (opts.GetFork() || (config.GetFork() && !opts.GetNoFork())) {
        pid_t pid = fork();
        if (pid == -1) {
            Log(LOG_ERR, "Failed to fork(): %s\n", strerror(errno));
            return DAEMON_EXIT_FORK_ERROR;
        } else if (pid > 0) {
            String pidfn = config.GetPidfile();
            int fd = opts.GetPrintPidFd();
            String pidStr(U32ToString(pid));
            pidStr += "\n";

            if (fd > 0) {
                int ret = write(fd, pidStr.c_str(), pidStr.size());
                if (ret == -1) {
                    Log(LOG_ERR, "Failed to print pid: %s\n", strerror(errno));
                }
            }

            if (!pidfn.empty()) {
                FileSink pidfile(pidfn);
                size_t sent;
                pidfile.PushBytes(pidStr.c_str(), pidStr.size(), sent);
            }

            // Unneeded parent process, just exit.
            _exit(DAEMON_EXIT_OK);
        } else {

            /*
             * We forked and are running as a daemon, so close STDIN, STDOUT, and
             * STDERR as appropriate.
             */
            LoggerSetting::GetLoggerSetting()->SetFile(NULL); // block logging to stdout/stderr
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);


#if !defined(ROUTER_LIB)
            if (!opts.GetNoSwitchUser()) {
#if defined(QCC_OS_LINUX)
                // Keep all capabilities before switching users
                prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
#endif

#if !defined(QCC_OS_ANDROID)
                String user = config.GetUser();
                if ((getuid() == 0) && !user.empty()) {
                    // drop root privileges if <user> is specified.
                    struct passwd* pwent;
                    setpwent();
                    while ((pwent = getpwent())) {
                        if (user.compare(pwent->pw_name) == 0) {
                            if (setuid(pwent->pw_uid) == 0) {
                                Log(LOG_INFO, "Dropping root privileges (running as %s)\n", pwent->pw_name);
                            } else {
                                Log(LOG_ERR, "Failed to drop root privileges - set userid failed: %s\n", user.c_str());
                                endpwent();
                                return DAEMON_EXIT_CONFIG_ERROR;
                            }
                            break;
                        }
                    }
                    endpwent();
                    if (!pwent) {
                        Log(LOG_ERR, "Failed to drop root privileges - userid does not exist: %s\n", user.c_str());
                        return DAEMON_EXIT_CONFIG_ERROR;
                    }
                }
#endif

#if defined(QCC_OS_LINUX)
                // Set the capabilities we need.
                struct __user_cap_header_struct header;
                struct __user_cap_data_struct cap;
                header.version = _LINUX_CAPABILITY_VERSION;
                header.pid = 0;
                cap.permitted = (1 << CAP_NET_RAW | 1 << CAP_NET_ADMIN | 1 << CAP_NET_BIND_SERVICE);
                cap.effective = cap.permitted;
                cap.inheritable = 0;
                capset(&header, &cap);
#endif
            }
#endif

            Log(LOG_INFO, "Running with effective userid %d\n", geteuid());

            // create new session ID
            pid_t sid = setsid();
            if (sid < 0) {
                Log(LOG_ERR, "Failed to set session ID: %s\n", strerror(errno));
                return DAEMON_EXIT_SESSION_ERROR;
            }
            if (chdir("/tmp") == -1) {
                Log(LOG_ERR, "Failed to change directory: %s\n", strerror(errno));
                return DAEMON_EXIT_CHDIR_ERROR;
            }
        }
    }

    Log(LOG_NOTICE, versionPreamble, GetVersion(), GetBuildInfo());

    int ret = daemon(opts);

    return ret;
}
