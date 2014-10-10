/**
 * @file
 * Bi-directional tunnel for forwarding alljoyn advertisements between subnets via TCP
 */

/******************************************************************************
 * Copyright (c) 2011,2014 AllSeen Alliance. All rights reserved.
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

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <vector>
#include <map>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/IPAddress.h>
#include <qcc/StringUtil.h>

#include <Callback.h>
#include <ns/IpNameServiceImpl.h>
#include <Transport.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace ajn;

static IpNameServiceImpl* g_ns;

/*
 * If true sniffs advertisements but doesn't send them.
 */
static bool sniffMode = false;

/*
 * Name service configuration parameters. These need to match up with the ones used by AllJoyn.
 */
const char* IPV4_MULTICAST_GROUP = "239.255.37.41";
const char* IPV6_MULTICAST_GROUP = "ff03::efff:2529";

/* Default tunnel port, override with the -p option */
const uint16_t TUNNEL_PORT = 9973;

/** Signal handler */
static void SigIntHandler(int sig)
{
    if (g_ns) {
        IpNameServiceImpl* ns = g_ns;
        g_ns = NULL;
        ns->Stop();
    }
}

class AdvTunnel {
  public:

    AdvTunnel() : stream(NULL) { }

    static const uint16_t ADV_VERSION = 1;
    static const uint32_t ADV_ID = 0xBEBE0000;

    QStatus VersionExchange();

    QStatus Connect(qcc::String address, uint16_t port);

    QStatus Listen(uint16_t port);

    QStatus RelayAdv();

    void Found(const qcc::String& busAddr, const qcc::String& guid, std::vector<qcc::String>& nameList, uint32_t timer);

    QStatus PullString(qcc::String& str)
    {
        char buffer[256];
        char* buf = buffer;
        size_t pulled;
        uint8_t len;

        QStatus status = stream->PullBytes(&len, 1, pulled);
        while ((status == ER_OK) && len) {
            status = stream->PullBytes(buf, len, pulled);
            if (status == ER_OK) {
                str.append(buf, pulled);
                buf += pulled;
                len -= pulled;
            }
        }
        return status;
    }

    QStatus PullInt(uint32_t& num)
    {
        qcc::String val;
        QStatus status = PullString(val);
        if (status == ER_OK) {
            num = qcc::StringToU32(val, 10, -1);
            if (num == (uint32_t)-1) {
                status = ER_INVALID_DATA;
            }
        }
        return status;
    }

    QStatus PushString(const qcc::String& str)
    {
        size_t pushed;
        uint8_t len = str.size();

        QStatus status = stream->PushBytes(&len, 1, pushed);
        if (status == ER_OK) {
            status = stream->PushBytes(str.c_str(), len, pushed);
        }
        return status;
    }

    QStatus PushInt(uint32_t num)
    {
        qcc::String val = qcc::U32ToString(num, 10);
        return PushString(val);
    }

    qcc::SocketStream* stream;
    /*
     * Maps from guid to name service
     */
    std::map<qcc::String, IpNameServiceImpl*> nsRelay;
};

QStatus AdvTunnel::VersionExchange()
{
    QStatus status = PushInt(ADV_VERSION | ADV_ID);
    if (status == ER_OK) {
        uint32_t version;
        status = PullInt(version);
        if (status == ER_OK) {
            if (version != (ADV_VERSION | ADV_ID)) {
                printf("version mismatch expected %d got %d\n", version, version & ~ADV_ID);
                status = ER_INVALID_DATA;
            }
        }
    }
    return status;
}

QStatus AdvTunnel::Connect(qcc::String address, uint16_t port)
{
    qcc::IPAddress addr(address);
    qcc::SocketFd sock;
    QStatus status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_STREAM, sock);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create connect socket"));
        return status;
    }
    while (status == ER_OK) {
        status = qcc::Connect(sock, addr, port);
        if (status == ER_OK) {
            break;
        }
        if (status == ER_CONN_REFUSED) {
            qcc::Sleep(5);
            status = ER_OK;
        }
    }
    if (status == ER_OK) {
        printf("Connected to advertisement relay\n");
        stream = new qcc::SocketStream(sock);
        status = VersionExchange();
        if (status != ER_OK) {
            delete stream;
            stream = NULL;
        }
    } else {
        qcc::Close(sock);
    }
    return status;
}

QStatus AdvTunnel::Listen(uint16_t port)
{
    qcc::IPAddress wildcard("0.0.0.0");
    qcc::SocketFd listenSock;
    qcc::SocketFd sock;
    QStatus status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_STREAM, listenSock);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create listen socket"));
        return status;
    }
    /* Allow reuse of the same port */
    status = qcc::SetReusePort(listenSock, true);
    if (status != ER_OK) {
        QCC_LogError(status, ("AdvTunnel::Listen(): SetReuse() failed"));
        qcc::Close(listenSock);
        return status;
    }
    status = qcc::Bind(listenSock, wildcard, port);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed bind listen socket"));
        qcc::Close(listenSock);
        return status;
    }
    status = qcc::Listen(listenSock, 0);
    if (status == ER_OK) {
        status = qcc::SetBlocking(listenSock, false);
    }
    if (status == ER_OK) {
        qcc::IPAddress addr;
        status = qcc::Accept(listenSock, addr, port, sock);
        if (status == ER_WOULDBLOCK) {
            qcc::Event ev(listenSock, qcc::Event::IO_READ);
            status = qcc::Event::Wait(ev, qcc::Event::WAIT_FOREVER);
            if (ER_OK == status) {
                status = qcc::Accept(listenSock, addr, port, sock);
            }
        }
        if (status == ER_OK) {
            printf("Accepted advertisement relay\n");
            stream = new qcc::SocketStream(sock);
            status = VersionExchange();
            if (status != ER_OK) {
                delete stream;
                stream = NULL;
            }
        } else {
            qcc::Close(sock);
        }
    }
    qcc::Close(listenSock);
    return status;
}

QStatus AdvTunnel::RelayAdv()
{
    QStatus status;

    qcc::String busAddr;
    status = PullString(busAddr);
    if (status != ER_OK) {
        return status;
    }

    qcc::String guid;
    status = PullString(guid);
    if (status != ER_OK) {
        return status;
    }

    uint32_t count = 0;
    status = PullInt(count);
    std::vector<qcc::String> nameList;
    for (size_t i = 0; (status == ER_OK) && (i < count); ++i) {
        qcc::String name;
        status = PullString(name);
        if (status == ER_OK) {
            nameList.push_back(name);
        }
    }
    if (status != ER_OK) {
        return status;
    }

    uint32_t timer;
    status = PullInt(timer);
    if (status != ER_OK) {
        return status;
    }

    printf("Relaying %d names at %s timer=%d\n", (int)nameList.size(), busAddr.c_str(), timer);
    for (size_t i = 0; i < nameList.size(); ++i) {
        printf("   %s\n", nameList[i].c_str());
    }
    /*
     * Lookup or create a name service for relaying advertisements for this quid
     */
    IpNameServiceImpl* ns;
    if (nsRelay.count(guid) == 0) {
        ns = new IpNameServiceImpl();
        status = ns->Init(guid);
        if (status != ER_OK) {
            delete ns;
            return status;
        }

        status = ns->Start();
        if (status != ER_OK) {
            delete ns;
            return status;
        }
        nsRelay[guid] = ns;
        /*
         * Parse out the port of the reliable TCP transport mechanism and set it
         * on the name service
         */
        std::map<qcc::String, qcc::String> argMap;
        QStatus status = Transport::ParseArguments("tcp", busAddr.c_str(), argMap);
        if (status == ER_OK) {
            uint16_t port = static_cast<uint16_t>(qcc::StringToU32(argMap["r4port"]));
            std::map<qcc::String, uint16_t> portMap;
            portMap["*"] = (uint16_t)port;
            status = ns->Enable(TRANSPORT_TCP, portMap, 0, std::map<qcc::String, uint16_t>(), 0, true, false, false, false);
            if (status == ER_OK) {
                ns->OpenInterface(TRANSPORT_TCP, "*");
            }
        }
    } else {
        ns = nsRelay[guid];
    }
    if (timer) {
        status = ns->AdvertiseName(TRANSPORT_TCP, nameList, false, TRANSPORT_TCP);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to advertise relayed names"));
        }
    } else {
        status = ns->CancelAdvertiseName(TRANSPORT_TCP, nameList, TRANSPORT_TCP);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to cancel relayed names"));
        }
    }
    /*
     * If nothing is being advertised for this guid we don't need this name service any more.
     */
    if (ns->NumAdvertisements(TRANSPORT_TCP) == 0) {
        printf("Removing unused name server\n");
        nsRelay.erase(guid);
        delete ns;
    }

    return status;
}

void AdvTunnel::Found(const qcc::String& busAddr, const qcc::String& guid, std::vector<qcc::String>& nameList, uint32_t timer)
{
    QStatus status;

    /*
     * We don't want to re-relay names that we are advertising.
     */
    if (nsRelay.count(guid) != 0) {
        return;
    }

    printf("Found %d names at %s timer=%d\n", (int)nameList.size(), busAddr.c_str(), timer);
    for (size_t i = 0; i < nameList.size(); ++i) {
        printf("   %s\n", nameList[i].c_str());
    }

    if (sniffMode) {
        status = ER_OK;
        goto Exit;
    }

    status = PushString(busAddr);
    if (status != ER_OK) {
        goto Exit;
    }
    status = PushString(guid);
    if (status != ER_OK) {
        goto Exit;
    }
    status = PushInt((uint32_t)nameList.size());
    for (size_t i = 0; i < nameList.size(); ++i) {
        status = PushString(nameList[i]);
    }
    if (status != ER_OK) {
        goto Exit;
    }
    status = PushInt(timer);
Exit:
    if (status != ER_OK) {
        printf("Failed to push found names into socket stream\n");
        if (g_ns) {
            g_ns->Stop();
        }
    }
}

static void usage(void)
{
    printf("Usage: advtunnel [-p <port>] ([-h] -l | -c <addr>)\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -s                    = Sniff mode\n");
    printf("   -p                    = Port to connect or listen on\n");
    printf("   -l                    = Listen mode\n");
    printf("   -c <addr>             = Connect mode and address to connect to\n");
}

int main(int argc, char** argv)
{
    QStatus status;
    IpNameServiceImpl ns;
    AdvTunnel tunnel;
    bool listen = false;
    qcc::String addr;
    uint16_t port =  TUNNEL_PORT;
    qcc::String guid = "0000000000000000000000000000";

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-l") == 0) {
            listen = true;
            continue;
        }
        if (strcmp(argv[i], "-s") == 0) {
            sniffMode = true;
            continue;
        }
        if (strcmp(argv[i], "-p") == 0) {
            if (++i == argc) {
                printf("Missing port number\n");
                usage();
                exit(1);
            }
            port = (uint16_t) qcc::StringToU32(argv[i]);
            continue;
        }
        if (strcmp(argv[i], "-c") == 0) {
            if (++i == argc) {
                printf("Missing connect address\n");
                usage();
                exit(1);
            }
            addr = argv[i];
            continue;
        }
        if (strcmp(argv[i], "-h") == 0) {
            usage();
            continue;
        }
        printf("Unknown option\n");
        usage();
        exit(1);
    }
    if (!sniffMode && ((!listen && (addr.size() == 0)) || (listen && (addr.size() != 0)))) {
        usage();
        exit(1);
    }

    g_ns = &ns;

    ns.SetCallback(TRANSPORT_TCP,
                   new CallbackImpl<AdvTunnel, void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>
                       (&tunnel, &AdvTunnel::Found));

    std::map<qcc::String, uint16_t> portMap;
    portMap["*"] = port;
    ns.Enable(TRANSPORT_TCP, portMap, 0, std::map<qcc::String, uint16_t>(), 0, true, false, false, false);

    /*
     * In sniffMode we just report advertisements
     */
    if (sniffMode) {
        ns.Init(guid);
        ns.Start();
        ns.OpenInterface(TRANSPORT_TCP, "*");
        ns.FindAdvertisement(TRANSPORT_TCP, "name='*'", IpNameServiceImpl::ALWAYS_RETRY, TRANSPORT_TCP);
        printf("Started sniffing for advertised names\n");
        qcc::Sleep(10000000);
        ns.Stop();
        ns.Join();
    }
    while (g_ns) {
        if (listen) {
            status = tunnel.Listen(port);
        } else {
            status = tunnel.Connect(addr, port);
        }
        if (status != ER_OK) {
            printf("Failed to establish relay: %s\n", QCC_StatusText(status));
        } else {
            printf("Relay established\n");

            ns.Init(guid);
            ns.Start();

            ns.OpenInterface(TRANSPORT_TCP, "*");
            ns.FindAdvertisement(TRANSPORT_TCP, "name='*'", IpNameServiceImpl::ALWAYS_RETRY, TRANSPORT_TCP);

            printf("Start relay\n");

            /*
             * Loop reading and rebroadcasting advertisements.
             */
            while (status == ER_OK) {
                status = tunnel.RelayAdv();
            }
            ns.Stop();
            ns.Join();
        }
    }
    ns.Join();

    return 0;
}
