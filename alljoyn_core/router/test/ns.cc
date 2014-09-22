/**
 * @file
 * Prototype code for accessing avahi Dbus API directly.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/IfConfig.h>
#include <qcc/GUID.h>
#include <qcc/Thread.h>  // For qcc::Sleep()

#include <alljoyn/Status.h>
#include <ns/IpNameService.h>
#include <ns/IpNameServiceImpl.h>
#include <ConfigDB.h>

#define QCC_MODULE "ALLJOYN"

using namespace ajn;

static const char config[] =
    "<busconfig>"
    "</busconfig>";

char const* g_names[] = {
    "org.randomteststring.A",
    "org.randomteststring.B",
    "org.randomteststring.C",
    "org.randomteststring.D",
    "org.randomteststring.E",
    "org.randomteststring.F",
    "org.randomteststring.G",
    "org.randomteststring.H",
    "org.randomteststring.I",
    "org.randomteststring.J",
    "org.randomteststring.K",
    "org.randomteststring.L",
    "org.randomteststring.M",
    "org.randomteststring.N",
    "org.randomteststring.O",
    "org.randomteststring.P",
    "org.randomteststring.Q",
    "org.randomteststring.R",
    "org.randomteststring.S",
    "org.randomteststring.T",
    "org.randomteststring.U",
    "org.randomteststring.V",
    "org.randomteststring.W",
    "org.randomteststring.X",
    "org.randomteststring.Y",
    "org.randomteststring.Z",
};

const uint32_t g_numberNames = sizeof(g_names) / sizeof(char const*);

char const* g_longnames[] = {
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.A",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.B",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.C",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.D",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.E",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.F",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.G",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.H",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.I",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.J",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.K",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.L",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.M",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.N",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.O",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.P",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.Q",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.R",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.S",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.T",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.U",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.V",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.W",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.X",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.Y",
    "org.thisisaverlongnamethatisusedtotestthemultiplemessageoutputcoderandomteststring.Z",
};

const uint32_t g_numberLongnames = sizeof(g_longnames) / sizeof(char const*);


class Finder {
  public:

    void Callback(const qcc::String& busAddr, const qcc::String& guid, std::vector<qcc::String>& wkn, uint32_t timer)
    {
        printf("Callback %s with guid %s and timer %d: ", busAddr.c_str(), guid.c_str(), timer);
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            printf("%s ", wkn[i].c_str());
        }
        printf("\n");

        m_called = true;
        m_guid = guid;
        m_wkn = wkn;
        m_timer = timer;
    }

    void Reset(void)
    {
        m_called = false;
        m_guid.clear();
        m_wkn.clear();
    }

    bool GetCalled() { return m_called; }
    qcc::String GetGuid() { return m_guid; }
    std::vector<qcc::String> GetWkn() { return m_wkn; }
    uint8_t GetTimer() { return m_timer; }

  private:
    bool m_called;
    qcc::String m_guid;
    std::vector<qcc::String> m_wkn;
    uint8_t m_timer;
};

static void PrintFlags(uint32_t flags)
{
    printf("(");
    if (flags & qcc::IfConfigEntry::UP) {
        printf("UP ");
    }
    if (flags & qcc::IfConfigEntry::BROADCAST) {
        printf("BROADCAST ");
    }
    if (flags & qcc::IfConfigEntry::DEBUG) {
        printf("DEBUG ");
    }
    if (flags & qcc::IfConfigEntry::LOOPBACK) {
        printf("LOOPBACK ");
    }
    if (flags & qcc::IfConfigEntry::POINTOPOINT) {
        printf("POINTOPOINT ");
    }
    if (flags & qcc::IfConfigEntry::RUNNING) {
        printf("RUNNING ");
    }
    if (flags & qcc::IfConfigEntry::NOARP) {
        printf("NOARP ");
    }
    if (flags & qcc::IfConfigEntry::PROMISC) {
        printf("PROMISC ");
    }
    if (flags & qcc::IfConfigEntry::NOTRAILERS) {
        printf("NOTRAILERS ");
    }
    if (flags & qcc::IfConfigEntry::ALLMULTI) {
        printf("ALLMULTI ");
    }
    if (flags & qcc::IfConfigEntry::MASTER) {
        printf("MASTER ");
    }
    if (flags & qcc::IfConfigEntry::SLAVE) {
        printf("SLAVE ");
    }
    if (flags & qcc::IfConfigEntry::MULTICAST) {
        printf("MULTICAST ");
    }
    if (flags & qcc::IfConfigEntry::PORTSEL) {
        printf("PORTSEL ");
    }
    if (flags & qcc::IfConfigEntry::AUTOMEDIA) {
        printf("AUTOMEDIA ");
    }
    if (flags & qcc::IfConfigEntry::DYNAMIC) {
        printf("DYNAMIC ");
    }
    if (flags) {
        printf("\b");
    }
    printf(")");
}

#define ERROR_EXIT exit(1)

int main(int argc, char** argv)
{
    QStatus status;
    uint16_t port = 0;

    bool advertise = false;
    bool useEth0 = false;
    bool runtests = false;
    bool wildcard = false;
    bool longnames = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp("-a", argv[i]) == 0) {
            advertise = true;
        } else if (strcmp("-e", argv[i]) == 0) {
            useEth0 = true;
        } else if (strcmp("-l", argv[i]) == 0) {
            longnames = true;
        } else if (strcmp("-t", argv[i]) == 0) {
            runtests = true;
        } else if (strcmp("-w", argv[i]) == 0) {
            wildcard = true;
        } else {
            printf("Unknown option %s\n", argv[i]);
            ERROR_EXIT;;
        }
    }

    if (runtests) {
        exit(0);
    }

    //
    // Load the configuration information
    //
    ConfigDB configdb(config);
    if (!configdb.LoadConfig()) {
        printf("Failed to load the internal config.\n");
        ERROR_EXIT;
    }

    //
    // Test code
    //
    IpNameService::Instance();

    //
    // Create an instance of the name service implementation.  This cheats
    // big-time and allows us to get down into the guts of the IP name
    // service.
    //
    IpNameServiceImpl ns;

    //
    // Initialize to a random quid, and talk to ourselves.  We don't have a
    // daemon config, so we expect to get the defalt setting for disabling
    // broadcasts, which is false.
    //
    status = ns.Init(qcc::GUID128().ToString(), true);
    if (status != ER_OK) {
        QCC_LogError(status, ("Init failed"));
        ERROR_EXIT;
    }

    status = ns.Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("Start failed"));
        ERROR_EXIT;
    }

    //
    // Figure out which interfaces we want to enable discovery on.
    //
    std::vector<qcc::IfConfigEntry> entries;
    status = qcc::IfConfig(entries);
    if (status != ER_OK) {
        QCC_LogError(status, ("IfConfig failed"));
        ERROR_EXIT;
    }

    printf("Checking out interfaces ...\n");
    qcc::String overrideInterface;
    for (uint32_t i = 0; i < entries.size(); ++i) {
        if (!useEth0) {
            if (entries[i].m_name == "eth0") {
                printf("******** Ignoring eth0, use \"-e\" to enable \n");
                continue;
            }
        }
        printf("    %s: ", entries[i].m_name.c_str());
        printf("0x%x = ", entries[i].m_flags);
        PrintFlags(entries[i].m_flags);
        if (entries[i].m_flags & qcc::IfConfigEntry::UP) {
            printf(", MTU = %d, address = %s", entries[i].m_mtu, entries[i].m_addr.c_str());
            if ((entries[i].m_flags & qcc::IfConfigEntry::LOOPBACK) == 0) {
                printf(" <--- Let's use this one");
                overrideInterface = entries[i].m_name;
                //
                // Tell the name service to talk and listen over the interface we chose
                // above.
                //
                status = ns.OpenInterface(TRANSPORT_TCP, entries[i].m_name);
                if (status != ER_OK) {
                    QCC_LogError(status, ("OpenInterface failed"));
                    ERROR_EXIT;
                }
            }
        }
        printf("\n");
    }

    srand(time(0));

    //
    // Pick a random port to advertise.  This is what would normally be the
    // daemon TCP well-known endpoint (9955) but we just make one up.  N.B. this
    // is not the name service multicast port.
    //
    port = rand();
    printf("Picked random port %d\n", port);

    //
    // Pretend we're the TCP transport and we want to advertise reliable and
    // unreliable IPv4 and IPv6 ports (all the same).
    //
    std::map<qcc::String, uint16_t> portMap;
    portMap["*"] = port;
    status = ns.Enable(TRANSPORT_TCP, portMap, port, portMap, port, true, true, true, true);

    if (status != ER_OK) {
        QCC_LogError(status, ("Enable failed"));
        ERROR_EXIT;
    }

    Finder finder;

    ns.SetCallback(TRANSPORT_TCP, new CallbackImpl<Finder, void, const qcc::String&, const qcc::String&,
                                                   std::vector<qcc::String>&, uint32_t>(&finder, &Finder::Callback));

    if (wildcard) {
        //
        // Enable discovery on all of the test names in one go.
        //
        printf("FindAdvertisement org.randomteststring.*\n");
        status = ns.FindAdvertisement(TRANSPORT_TCP, "name='org.randomteststring.*'", IpNameServiceImpl::ALWAYS_RETRY, TRANSPORT_TCP);
        if (status != ER_OK) {
            QCC_LogError(status, ("FindAdvertisedName failed"));
            ERROR_EXIT;
        }
    } else {
        //
        // Enable discovery on all of the test names individually
        //
        for (uint32_t i = 0; !wildcard && i < g_numberNames; ++i) {
            printf("FindAdvertisement %s\n", g_names[i]);

            qcc::String matching = qcc::String("name='") + g_names[i] + "'";
            status = ns.FindAdvertisement(TRANSPORT_TCP, matching, IpNameServiceImpl::ALWAYS_RETRY, TRANSPORT_TCP);
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedName failed"));
                ERROR_EXIT;
            }
        }
    }

    if (longnames) {
        for (uint32_t i = 0; i < g_numberLongnames; ++i) {
            char const* wkn = g_longnames[i];
            status = ns.AdvertiseName(TRANSPORT_TCP, wkn, false, TRANSPORT_TCP);
            printf("Advertised %s\n", wkn);
        }
    }

    //
    // Hang around and mess with advertisements for a while.
    //
    for (uint32_t i = 0; i < 200; ++i) {

        //
        // Sleep for a while -- long enough for the name service to respond and
        // humans to observe what is happening.
        //
        printf("Zzzzz %d\n", i);

        qcc::Sleep(1000);

        if (advertise) {
            uint32_t nameIndex = rand() % g_numberNames;
            char const* wkn = g_names[nameIndex];

            status = ns.AdvertiseName(TRANSPORT_TCP, wkn, false, TRANSPORT_TCP);
            printf("Advertised %s\n", wkn);
            if (status != ER_OK) {
                QCC_LogError(status, ("Advertise failed"));
                ERROR_EXIT;
            }

            nameIndex = rand() % g_numberNames;
            wkn = g_names[nameIndex];

            status = ns.CancelAdvertiseName(TRANSPORT_TCP, wkn, TRANSPORT_TCP);
            printf("Cancelled %s\n", wkn);
            if (status != ER_OK) {
                QCC_LogError(status, ("Cancel failed"));
                ERROR_EXIT;
            }
        }
    }

    return 0;
}
