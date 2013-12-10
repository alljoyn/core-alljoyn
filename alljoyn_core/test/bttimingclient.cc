/**
 * @file
 * BT timing client - Collects BT discovery, SDP query, and connect times
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include <algorithm>
#include <set>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

#define METHODCALL_TIMEOUT 30000

using namespace std;
using namespace qcc;
using namespace ajn;

/** Sample constants */
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* DefaultWellKnownName = "org.alljoyn.alljoyn_test";
const SessionPort SessionPort = 24;   /**< Well-known session port value for bbclient/bbservice */
namespace values {
}
}
}
}

/** Static data */
static Event g_discoverEvent;
static Event g_lostAdvertisementsEvent;
static String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:

    MyBusListener(BusAttachment& bus, bool stopDiscover) :
        BusListener(),
        bus(bus),
        sessionId(0),
        stopDiscover(stopDiscover)
    {
    }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        if (g_wellKnownName == name) {
            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, transport);
            QStatus status;

            if (stopDiscover) {
                status = bus.CancelFindAdvertisedName(g_wellKnownName.c_str());
                if (status != ER_OK) {
                    QCC_LogError(status, ("CancelFindAdvertisedName(%s) failed", name));
                    exit(1);
                }
            }

            status = bus.JoinSession(name, ::org::alljoyn::alljoyn_test::SessionPort, this, sessionId, opts);
            if (status != ER_OK) {
                QCC_LogError(status, ("JoinSession(%s) failed", name));
                exit(1);
            }

            /* Release the main thread */
            adnames.insert(String(name));
            g_discoverEvent.SetEvent();
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        adnames.erase(String(name));
        if (adnames.empty()) {
            g_lostAdvertisementsEvent.SetEvent();
        }
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    BusAttachment& bus;
    SessionId sessionId;
    bool stopDiscover;

    set<String> adnames;
};

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

struct Sample {
    uint64_t discover;
    uint64_t sdpQuery;
    uint64_t connect;
    uint64_t overall;
    Sample(uint64_t init = 0ULL) : discover(init), sdpQuery(init), connect(init), overall(init) { }
};

class Stat {
  public:
    typedef map<String, uint32_t> DeviceTimes;
    Sample sum;
    Sample maxDelta;
    Sample minDelta;
    Sample last;
    size_t sampleCount;
    char sep;
    Stat() : sum(0ULL), maxDelta(0ULL), minDelta(~0ULL), sampleCount(0) { }
    bool AddSample(uint64_t baseTime, const ProxyBusObject& ajDbgObj)
    {
        DeviceTimes discoverTimes;
        DeviceTimes sdpQueryTimes;
        DeviceTimes connectTimes;
        Timespec now;
        GetTimeNow(&now);
        uint64_t delta = now.GetAbsoluteMillis() - baseTime;

        if (GetDaemonTime(ajDbgObj, "DiscoverTimes", discoverTimes) &&
            GetDaemonTime(ajDbgObj, "SDPQueryTimes", sdpQueryTimes) &&
            GetDaemonTime(ajDbgObj, "ConnectTimes", connectTimes) &&
            connectTimes.size() == 1) {

            String connectDevice = connectTimes.begin()->first;
            ++sampleCount;

            last.overall = delta;
            sum.overall += last.overall;
            maxDelta.overall = max(maxDelta.overall, last.overall);
            minDelta.overall = min(minDelta.overall, last.overall);

            last.discover = discoverTimes[connectDevice];
            sum.discover += last.discover;
            maxDelta.discover = max(maxDelta.discover, last.discover);
            minDelta.discover = min(minDelta.discover, last.discover);

            last.sdpQuery = sdpQueryTimes[connectDevice];
            sum.sdpQuery += last.sdpQuery;
            maxDelta.sdpQuery = max(maxDelta.sdpQuery, last.sdpQuery);
            minDelta.sdpQuery = min(minDelta.sdpQuery, last.sdpQuery);

            last.connect = connectTimes[connectDevice];
            sum.connect += last.connect;
            maxDelta.connect = max(maxDelta.connect, last.connect);
            minDelta.connect = min(minDelta.connect, last.connect);

            return true;
        }
        return false;
    }
    static bool GetDaemonTime(const ProxyBusObject& ajDbgObj, const char* propName, DeviceTimes& times)
    {
        MsgArg val;
        MsgArg* entries;
        size_t numEntries;
        bool keep = true;

        QStatus status = ajDbgObj.GetProperty("org.alljoyn.Bus.Debug.BT", propName, val);
        assert(status == ER_OK);

        status = val.Get("a(su)", &numEntries, &entries);
        assert(status == ER_OK);

        for (size_t i = 0; i < numEntries; ++i) {
            const char* addrStr;
            uint32_t delta;
            entries[i].Get("(su)", &addrStr, &delta);
            String addr(addrStr);
            if (times.find(addr) == times.end()) {
                times[addr] = delta;
            } else {
                keep = false;
                break;
            }
        }
        return keep;
    }
    String Time2String(uint64_t time, size_t padding = 7) const
    {
        String timestr = U64ToString(time / 1000) + '.' + U64ToString(time % 1000, 10, 3, '0');
        String pad;
        while ((pad.size() + timestr.size()) < padding) {
            pad += ' ';
        }
        return pad + timestr;
    }
    String Avg(uint64_t sum, size_t padding = 7) const
    {
        if (sampleCount) {
            return Time2String((sum / sampleCount) + ((sum % sampleCount) > (sampleCount / 2) ? 1 : 0), padding);
        } else {
            return "0";
        }
    }
    String FormatStdOut() const
    {
        return Format(false, ' ', 7);
    }
    String FormatCSV() const
    {
        return Format(false, ',', 1);
    }
    String FormatGP() const
    {
        return Format(true, ' ', 1);
    }
    String Format(bool gnuplot, char sep, size_t padding) const
    {
        String fmt;
        fmt = U64ToString(sampleCount, 10, padding, ' ');
        if (padding != 1) fmt += " |";

        fmt += sep + Time2String(last.overall, padding);
        if (!gnuplot) {
            fmt += sep + Time2String(minDelta.overall, padding);
            fmt += sep + Avg(sum.overall, padding);
            fmt += sep + Time2String(maxDelta.overall, padding);
        }
        if (padding != 1) fmt += " | ";

        fmt += sep + Time2String(last.discover, padding);
        if (!gnuplot) {
            fmt += sep + Time2String(minDelta.discover, padding);
            fmt += sep + Avg(sum.discover, padding);
            fmt += sep + Time2String(maxDelta.discover, padding);
        }
        if (padding != 1) fmt += " | ";

        fmt += sep + Time2String(last.sdpQuery, padding);
        if (!gnuplot) {
            fmt += sep + Time2String(minDelta.sdpQuery, padding);
            fmt += sep + Avg(sum.sdpQuery, padding);
            fmt += sep + Time2String(maxDelta.sdpQuery, padding);
        }
        if (padding != 1) fmt += " |";

        fmt += sep + Time2String(last.connect, padding);
        if (!gnuplot) {
            fmt += sep + Time2String(minDelta.connect, padding);
            fmt += sep + Avg(sum.connect, padding);
            fmt += sep + Time2String(maxDelta.connect, padding);
        }
        return fmt;
    }
};



static void usage(void)
{
    printf("Usage: bttimingclient [-h] [-r #] [-s] [-n <well-known name>] [-c <filename>] [-g <gnuplotname>\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -r #                  = AllJoyn attachment restart count\n");
    printf("   -n <well-known name>  = Well-known bus name advertised by bbservice\n");
    printf("   -s                    = Cancel discover when found\n");
    printf("   -c <filename>         = Output CSV file\n");
    printf("   -g <gnuplotname>      = Output a GNUPlot .dat and .script file\n");
    printf("\n");
}


/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    Environ* env;
    bool stopDiscover = false;
    uint32_t repCount = 1;
    FILE* csvFile = NULL;
    FILE* gpFile = NULL;
    String gnuplotfn;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-r", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                repCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-s", argv[i])) {
            stopDiscover = true;
        } else if (0 == strcmp("-c", argv[i])) {
            ++i;
            if (argc == i) {
                printf("Must specify a file for CSV output.\n");
                usage();
                exit(1);
            }
            String csvfn = argv[i];
            if (csvfn.substr(csvfn.size() - 4).compare(".csv") != 0) {
                csvfn += ".csv";
            }
            csvFile = fopen(csvfn.c_str(), "w");
            if (csvFile == NULL) {
                printf("Failed to open %s for writing: %s\n", argv[i], strerror(errno));
                exit(1);
            }
        } else if (0 == strcmp("-g", argv[i])) {
            ++i;
            if (argc == i) {
                printf("Must specify a filename for GNUPlot output.\n");
                usage();
                exit(1);
            }
            gnuplotfn = argv[i];
            String datafn = gnuplotfn + ".dat";
            gpFile = fopen(datafn.c_str(), "w");
            if (gpFile == NULL) {
                printf("Failed to open %s for writing: %s\n", argv[i], strerror(errno));
                exit(1);
            }
        } else {
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* Get env vars */
    env = Environ::GetAppEnviron();
    qcc::String connectArgs = env->Find("BUS_ADDRESS");

    Stat stat;

    printf(" Sample | Overall     min     avg     max | Discover     min     avg     max | SDPQuery     min     avg     max | Connect     min     avg     max\n"
           "--------+---------------------------------+----------------------------------+----------------------------------+--------------------------------\n");
    if (csvFile) {
        fprintf(csvFile, "Sample,Overall,overall_min,overall_avg,overall_max,Discover,discover_min,discover_avg,discover_max,SDPQuery,sdpquery_min,sdpquery_avg,sdpquery_max,Connect,connect_min,connect_avg,connect_max\n");
    }


    for (unsigned long i = 0; i < repCount; ++i) {
        Timespec nowts;
        uint64_t startTime;

        if (true) {
            String report;

            /*
             * Create message bus on the stack.  We rely on C++ scoping rules to
             * ensure the object is destroyed when execution leaves the
             * enclosing context, in this case the opening brace on the if
             * (true) statement above.
             */
            BusAttachment msgBus("bttimingclient", true);
            ProxyBusObject btTimingObj;

            InterfaceDescription* testIntf = NULL;
            status = msgBus.CreateInterface("org.alljoyn.Bus.Debug.BT", testIntf);
            if (status == ER_OK) {
                if (testIntf) {
                    testIntf->AddMethod("FlushDiscoverTimes", NULL, NULL, NULL, 0);
                    testIntf->AddMethod("FlushSDPQueryTimes", NULL, NULL, NULL, 0);
                    testIntf->AddMethod("FlushConnectTimes", NULL, NULL, NULL, 0);
                    testIntf->AddMethod("FlushCachedNames", NULL, NULL, NULL, 0);
                    testIntf->AddProperty("DiscoverTimes", "a(su)", PROP_ACCESS_READ);
                    testIntf->AddProperty("SDPQueryTimes", "a(su)", PROP_ACCESS_READ);
                    testIntf->AddProperty("ConnectTimes", "a(su)", PROP_ACCESS_READ);
                    testIntf->Activate();
                } else {
                    status = ER_FAIL;
                }
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("CreateInterface failed"));
                exit((int)status);
            }

            /* Register a bus listener in order to get discovery indications */
            MyBusListener busListener(msgBus, stopDiscover);
            msgBus.RegisterBusListener(busListener);

            /* Start the msg bus */
            status = msgBus.Start();
            if (status != ER_OK) {
                QCC_LogError(status, ("BusAttachment::Start failed"));
                goto exit;
            }

            btTimingObj = msgBus.GetAllJoynDebugObj();  // make a copy
            btTimingObj.AddInterface(*testIntf);

            /* Connect to the bus */
            if (connectArgs.empty()) {
                status = msgBus.Connect();
            } else {
                status = msgBus.Connect(connectArgs.c_str());
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("BusAttachment::Connect(\"%s\") failed", connectArgs.c_str()));
                goto exit;
            }

            btTimingObj.MethodCall("org.alljoyn.Bus.Debug.BT", "FlushDiscoverTimes", NULL, 0);
            btTimingObj.MethodCall("org.alljoyn.Bus.Debug.BT", "FlushSDPQueryTimes", NULL, 0);
            btTimingObj.MethodCall("org.alljoyn.Bus.Debug.BT", "FlushConnectTimes", NULL, 0);
            btTimingObj.MethodCall("org.alljoyn.Bus.Debug.BT", "FlushCachedNames", NULL, 0);

            GetTimeNow(&nowts);
            startTime = nowts.GetAbsoluteMillis();

            /* Begin discovery on the well-known name of the service to be called */
            g_discoverEvent.ResetEvent();
            status = msgBus.FindAdvertisedName(g_wellKnownName.c_str());
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedName failed"));
                goto exit;
            }

            /*
             * Wait for the "FoundAdvertisedName" signal that will cause our
             * discover_event to be set.
             */
            for (;;) {
                qcc::Event timerEvent(100, 100);
                vector<qcc::Event*> checkEvents, signaledEvents;
                checkEvents.push_back(&g_discoverEvent);
                checkEvents.push_back(&timerEvent);
                status = qcc::Event::Wait(checkEvents, signaledEvents);
                if (status != ER_OK && status != ER_TIMEOUT) {
                    break;
                }

                /*
                 * Don't continue waiting if someone has pressed Control-C.
                 */
                if (g_interrupt) {
                    break;
                }

                /*
                 * If it was the g_discoverEvent becoming signalled that caused
                 * us to wake up, then we're done here.
                 */
                for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
                    if (*i == &g_discoverEvent) {
                        break;
                    }
                }
            }

            /*
             * If someone pressed Control-C, we're done here.  This break will
             * cause the bus to go out of scope and be destroyed, and we will
             * then print stats and exit.
             */
            if (g_interrupt) {
                break;
            }

            if (!stat.AddSample(startTime, btTimingObj)) {
                --i; // retry getting the sample
                continue;
            }


            // Bluetooth dongles seem to lose their little minds if
            // connections are dropped too soon after being established too
            // often.  A small random delay helps simulate more real world
            // activity.
            qcc::Sleep(Rand32() % 2000 + 4000);

            status = msgBus.LeaveSession(busListener.GetSessionId());
            if (status != ER_OK) {
                QCC_LogError(status, ("LeaveSession(%u) failed", busListener.GetSessionId()));
                goto exit;
            }

            if (!stopDiscover) {
                status = msgBus.CancelFindAdvertisedName(g_wellKnownName.c_str());
                if (status != ER_OK) {
                    QCC_LogError(status, ("CancelFindAdvertisedName(%s) failed", g_wellKnownName.c_str()));
                    goto exit;
                }
            }

            printf("%s\n", stat.FormatStdOut().c_str());
            fflush(stdout);
            if (csvFile) {
                fprintf(csvFile, "%s\n", stat.FormatCSV().c_str());
                fflush(csvFile);
            }
            if (gpFile) {
                fprintf(gpFile, "%s\n", stat.FormatGP().c_str());
                fflush(gpFile);
            }

        }

        if (i < (repCount - 1)) {
            // Bluetooth dongles seem to lose their little minds if
            // connections are created too soon after being dropped too often.
            // A small random delay helps simulate more real world activity.
            qcc::Sleep(Rand32() % 2000 + 4000);
        }
    }

exit:
    printf("Overall Time:   min = %s   avg = %s   max = %s\n",
           stat.Time2String(stat.minDelta.overall).c_str(),
           stat.Avg(stat.sum.overall).c_str(),
           stat.Time2String(stat.maxDelta.overall).c_str());
    printf("Discovery Time: min = %s   avg = %s   max = %s\n",
           stat.Time2String(stat.minDelta.discover).c_str(),
           stat.Avg(stat.sum.discover).c_str(),
           stat.Time2String(stat.maxDelta.discover).c_str());
    printf("SDP Query Time: min = %s   avg = %s   max = %s\n",
           stat.Time2String(stat.minDelta.sdpQuery).c_str(),
           stat.Avg(stat.sum.sdpQuery).c_str(),
           stat.Time2String(stat.maxDelta.sdpQuery).c_str());
    printf("Connect Time:   min = %s   avg = %s   max = %s\n",
           stat.Time2String(stat.minDelta.connect).c_str(),
           stat.Avg(stat.sum.connect).c_str(),
           stat.Time2String(stat.maxDelta.connect).c_str());

    if (csvFile) {
        fclose(csvFile);
    }
    if (gpFile) {
        fclose(gpFile);
        String datafn = gnuplotfn + ".script";
        gpFile = fopen(datafn.c_str(), "w");
        if (gpFile) {
            fprintf(gpFile, "# gnuplot script\n");
            fprintf(gpFile, "set title 'Bluetooth connect times with discovery %s name found.'\n",
                    stopDiscover ? "turned off when" : "left on after");
            fprintf(gpFile, "set xlabel 'Samples'\n");
            fprintf(gpFile, "set ylabel 'Time in seconds'\n");
            fprintf(gpFile, "set grid xtics ytics\n");
            fprintf(gpFile, "set key outside center bottom horizontal\n");
            fprintf(gpFile, "set terminal png font ',8' linewidth 1 size 800,600\n");
            fprintf(gpFile, "set output '%s.png'\n", gnuplotfn.c_str());
            fprintf(gpFile, "plot ");
            fprintf(gpFile, "'%s.dat' using 1:2 ls 1 title 'Overall time (%s/%s/%s)' with lines,",
                    gnuplotfn.c_str(),
                    stat.Time2String(stat.minDelta.overall, 1).c_str(),
                    stat.Avg(stat.sum.overall, 1).c_str(),
                    stat.Time2String(stat.maxDelta.overall, 1).c_str());
            fprintf(gpFile, "'%s.dat' using 1:2 ls 1 notitle with points,",
                    gnuplotfn.c_str());
            fprintf(gpFile, "'%s.dat' using 1:3 ls 2 title 'Discover time (%s/%s/%s)' with lines,",
                    gnuplotfn.c_str(),
                    stat.Time2String(stat.minDelta.discover, 1).c_str(),
                    stat.Avg(stat.sum.discover, 1).c_str(),
                    stat.Time2String(stat.maxDelta.discover, 1).c_str());
            fprintf(gpFile, "'%s.dat' using 1:3 ls 2 notitle with points,",
                    gnuplotfn.c_str());
            fprintf(gpFile, "'%s.dat' using 1:4 ls 3 title 'SDP Query time (%s/%s/%s)' with lines,",
                    gnuplotfn.c_str(),
                    stat.Time2String(stat.minDelta.sdpQuery, 1).c_str(),
                    stat.Avg(stat.sum.sdpQuery, 1).c_str(),
                    stat.Time2String(stat.maxDelta.sdpQuery, 1).c_str());
            fprintf(gpFile, "'%s.dat' using 1:4 ls 3 notitle with points,",
                    gnuplotfn.c_str());
            fprintf(gpFile, "'%s.dat' using 1:5 ls 4 title 'Connect time (%s/%s/%s)' with lines,",
                    gnuplotfn.c_str(),
                    stat.Time2String(stat.minDelta.connect, 1).c_str(),
                    stat.Avg(stat.sum.connect, 1).c_str(),
                    stat.Time2String(stat.maxDelta.connect, 1).c_str());
            fprintf(gpFile, "'%s.dat' using 1:5 ls 4 notitle with points\n",
                    gnuplotfn.c_str());
            fclose(gpFile);
        }
    }

    return (int) status;
}
