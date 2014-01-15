/**
 * @file
 * PacketEngine tester
 */

/******************************************************************************
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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

#include <limits>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>

#include <map>

#include <qcc/Debug.h>
#include <qcc/Log.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Mutex.h>
#include <alljoyn/version.h>

#include "PacketEngine.h"
#include "UDPPacketStream.h"

#define QCC_MODULE "PACKET"

using namespace qcc;
using namespace std;
using namespace ajn;

static bool g_interrupt = false;
static const char* g_ifaceName = "eth0";
static uint16_t g_port = 9911;
static uint32_t g_sendTtl = 0;
static uint32_t g_recvTimeout = 1;


class PacketEngineController : public PacketEngineListener {
  public:
    PacketEngineController(const char* ifaceName, uint16_t port);

    ~PacketEngineController();

    QStatus Start();

    void Stop();

    void Join();

    String GetIPAddr() const { return udpStream.GetIPAddr(); }

    QStatus Connect(const qcc::String& addr, uint16_t port)
    {
        return engine.Connect(GetPacketDest(addr, port), udpStream, *this, NULL);
    }

    void Disconnect(uint32_t connNum);

    void PacketEngineConnectCB(PacketEngine& engine, QStatus status, const PacketEngineStream* stream, const PacketDest& dest, void* context);

    bool PacketEngineAcceptCB(PacketEngine& engine, const PacketEngineStream& stream, const PacketDest& dest);

    void PacketEngineDisconnectCB(PacketEngine& engine, const PacketEngineStream& stream, const PacketDest& dest);

    QStatus Send(uint32_t chanIdx, const String& data, uint32_t ttl = 0);

    QStatus Recv(uint32_t chanIdx, String& data);

    void ListStreams() const;

    QStatus SetSendTimeout(uint32_t chanIdx, uint32_t timeout);

  private:
    UDPPacketStream udpStream;
    PacketEngine engine;
    mutable Mutex streamsLock;
    map<int, PacketEngineStream> streams;
    int nextStreamId;
};

PacketEngineController::PacketEngineController(const char* ifaceName, uint16_t port) :
    udpStream(ifaceName, port),
    engine("pe"),
    nextStreamId(0)
{
}

PacketEngineController::~PacketEngineController()
{
    Stop();
    Join();
}

QStatus PacketEngineController::Start()
{
    /* Start PacketStream */
    QStatus status = udpStream.Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPPacketStream::Start failed"));
    }

    /* Add PacketStream */
    if (status == ER_OK) {
        status = engine.AddPacketStream(udpStream, *this);
        if (status != ER_OK) {
            QCC_LogError(status, ("AddPacketStream failed"));
        }
    }

    /* Start PacketEngine */
    if (status == ER_OK) {
        status = engine.Start(::max(udpStream.GetSourceMTU(), udpStream.GetSinkMTU()));
        if (status != ER_OK) {
            QCC_LogError(status, ("PacketEngine::Start failed"));
        }
    }

    return status;
}

void PacketEngineController::Stop()
{
    engine.Stop();
    udpStream.Stop();
}

void PacketEngineController::Join()
{
    engine.Join();
}

void PacketEngineController::PacketEngineConnectCB(PacketEngine& engine,
                                                   QStatus status,
                                                   const PacketEngineStream* stream,
                                                   const PacketDest& dest,
                                                   void* context)
{
    if (status == ER_OK) {
        printf("Connect to %s succeeded.\n", udpStream.ToString(dest).c_str());
        streamsLock.Lock();
        streams.insert(pair<int, PacketEngineStream>(++nextStreamId, *stream));
        streamsLock.Unlock();
    } else {
        printf("Connect to %s failed with %s\n", udpStream.ToString(dest).c_str(), QCC_StatusText(status));
    }
}

bool PacketEngineController::PacketEngineAcceptCB(PacketEngine& engine, const PacketEngineStream& stream, const PacketDest& dest)
{
    printf("Accepting connect attempt from %s\n", udpStream.ToString(dest).c_str());
    streamsLock.Lock();
    streams.insert(pair<int, PacketEngineStream>(++nextStreamId, stream));
    streamsLock.Unlock();
    return true;
}

void PacketEngineController::PacketEngineDisconnectCB(PacketEngine& engine, const PacketEngineStream& stream, const PacketDest& dest)
{
    bool found = false;
    printf("Disconnect indication from %s\n", udpStream.ToString(dest).c_str());
    streamsLock.Lock();
    map<int, PacketEngineStream>::iterator it = streams.begin();
    while (it != streams.end()) {
        if (it->second == stream) {
            streams.erase(it->first);
            found = true;
            break;
        }
        ++it;
    }
    streamsLock.Unlock();
    if (!found) {
        printf("ERROR: Could not find stream %s\n", udpStream.ToString(dest).c_str());
    }
}

void PacketEngineController::ListStreams() const
{
    streamsLock.Lock();
    map<int, PacketEngineStream>::const_iterator it = streams.begin();
    while (it != streams.end()) {
        printf("#%i: channelId=0x%x\n", it->first, it->second.GetChannelId());
        ++it;
    }
    printf("\n");
    streamsLock.Unlock();
}

QStatus PacketEngineController::SetSendTimeout(uint32_t chanIdx, uint32_t timeout)
{
    QStatus status = ER_FAIL;
    streamsLock.Lock();
    map<int, PacketEngineStream>::iterator it = streams.find(chanIdx);
    if (it != streams.end()) {
        it->second.SetSendTimeout(timeout);
        status = ER_OK;
    }
    streamsLock.Unlock();
    return status;
}

QStatus PacketEngineController::Send(uint32_t chanIdx, const String& data, uint32_t ttl)
{
    QStatus status = ER_FAIL;
    streamsLock.Lock();
    map<int, PacketEngineStream>::iterator it = streams.find(chanIdx);
    if (it != streams.end()) {
        size_t actualBytes;
        PacketEngineStream& stream = it->second;
        streamsLock.Unlock();
        status = stream.PushBytes(data.data(), data.size(), actualBytes, ttl);
        if ((status == ER_OK) && (actualBytes != data.size())) {
            status = ER_FAIL;
            printf("PacketEngineController::Send: short send. expected=%d, actual=%d\n", (int) data.size(), (int) actualBytes);
        }
    } else {
        streamsLock.Unlock();
    }
    return status;
}

QStatus PacketEngineController::Recv(uint32_t chanIdx, String& data)
{
    QStatus status = ER_FAIL;
    streamsLock.Lock();
    map<int, PacketEngineStream>::iterator it = streams.find(chanIdx);
    if (it != streams.end()) {
        size_t actualBytes;
        PacketEngineStream& stream = it->second;
        streamsLock.Unlock();
        status = stream.PullBytes(data.begin(), data.capacity(), actualBytes, g_recvTimeout);
        if (status == ER_OK) {
            data.assign(data.begin(), actualBytes);
        }
    } else {
        streamsLock.Unlock();
    }
    return status;
}

void PacketEngineController::Disconnect(uint32_t chanIdx)
{
    streamsLock.Lock();
    map<int, PacketEngineStream>::iterator it = streams.find(chanIdx);
    if (it != streams.end()) {
        engine.Disconnect(it->second);
    }
    streamsLock.Unlock();
}

static char* get_line(char*str, size_t num, FILE*fp)
{
    char*p = fgets(str, num, fp);

    // fgets will capture the '\n' character if the string entered is shorter than
    // num. Remove the '\n' from the end of the line and replace it with nul '\0'.
    if (p != NULL) {
        size_t last = strlen(str) - 1;
        if (str[last] == '\n') {
            str[last] = '\0';
        }
    }
    return p;
}

static String NextTok(String& inStr)
{
    String ret;
    size_t off = inStr.find_first_of(' ');
    if (off == String::npos) {
        ret = inStr;
        inStr.clear();
    } else {
        ret = inStr.substr(0, off);
        inStr = Trim(inStr.substr(off));
    }
    return Trim(ret);
}

static void SigIntHandler(int sig)
{
    g_interrupt = true;
    ::fclose(stdin);
}

static void usage(void)
{
    printf("Usage: packettest [-h] [-i <iface>]\n\n");
    printf("Options:\n");
    printf("   -h            - Print this help message\n");
    printf("   -i <iface>    - Set the network interface\n");
    printf("   -p <port>     - Set the network port\n");
    printf("\n");
}

static void DoConnect(PacketEngineController& controller, const String& host, uint16_t port)
{
    QStatus status = controller.Connect(host, port);
    if (status != ER_OK) {
        QCC_LogError(status, ("Connect failed"));
    }
}

static void DoDisconnect(PacketEngineController& controller, uint32_t connNum)
{
    controller.Disconnect(connNum);
}

static void DoList(PacketEngineController& controller)
{
    controller.ListStreams();
}

static void DoSend(PacketEngineController& controller, uint32_t chanIdx, const String& data, uint32_t ttl)
{
    QStatus status = controller.Send(chanIdx, data, ttl);
    if (status != ER_OK) {
        printf("controller.Send(%u, <>) failed with %s\n", chanIdx, QCC_StatusText(status));
    }
}

static QStatus DoSendAtRate(PacketEngineController& controller, uint32_t chanIdx, size_t dataSize, uint32_t delay, uint32_t count, uint32_t ttl)
{
    QStatus status = ER_OK;
    char* msg = new char[dataSize];
    for (size_t i = 0; i < dataSize; ++i) {
        msg[i] = 'A' + (i % 52);
    }
    String data(msg, dataSize, dataSize);

    uint32_t i = 0;
    while (i++ < count) {
        uint64_t start = GetTimestamp64();
        status = controller.Send(chanIdx, data, ttl);
        if (status != ER_OK) {
            printf("controller.Send(%u, <>) failed with %s\n", chanIdx, QCC_StatusText(status));
            break;
        }
        if ((i % 1000) == 0) {
            printf("reps=%d\n", i);
        }
        uint64_t now = GetTimestamp64();
        if (now < (start + delay)) {
            qcc::Sleep((start + delay) - now);
        }
    }
    return status;
}

static QStatus DoRecv(PacketEngineController& controller, uint32_t chanIdx, String& data)
{
    QStatus status = controller.Recv(chanIdx, data);
    if (status != ER_OK) {
        printf("controller.Recv(%u, <>) failed with %s\n", chanIdx, QCC_StatusText(status));
    }
    return status;
}

static QStatus DoRecvAtRate(PacketEngineController& controller, uint32_t chanIdx, size_t dataSize, uint32_t delay, uint32_t count, uint32_t ttl)
{
    QStatus status = ER_OK;
    char* msg = new char[dataSize];
    for (size_t i = 0; i < dataSize; ++i) {
        msg[i] = 'A' + (i % 52);
    }
    String compareData(msg, dataSize, dataSize);
    String data;
    data.reserve(dataSize);
    uint32_t rep = 1;
    while (rep++ <= count) {
        uint64_t start = GetTimestamp64();
        status = controller.Recv(chanIdx, data);
        if (status != ER_OK) {
            printf("controller.Recv(%u, <>) failed with %s (rep=%d)\n", chanIdx, QCC_StatusText(status), rep);
            break;
        } else {
            if (data != compareData) {
                unsigned int i;
                for (i = 0; i < ::min(data.length(), compareData.length()); ++i) {
                    if (data[i] != compareData[i]) {
                        break;
                    }
                }
                printf("Recv data miscompare at offset %d. (data.length=%d, compareData.length=%d)\n", i, (int) data.length(), (int) compareData.length());
            }
        }
        if ((rep % 1000) == 0) {
            printf("reps=%d\n", rep);
        }
        uint64_t now = GetTimestamp64();
        if (now < (start + delay)) {
            qcc::Sleep((start + delay) - now);
        }
    }
    return status;
}

int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (::strcmp("-h", argv[i]) == 0) {
            usage();
            exit(0);
        } else if (::strcmp("-i", argv[i]) == 0) {
            g_ifaceName = argv[++i];
        } else if (::strcmp("-p", argv[i]) == 0) {
            g_port = static_cast<uint16_t>(StringToU32(argv[++i], 10, 0));
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* Create PacketEngine controller */
    PacketEngineController controller(g_ifaceName, g_port);
    status = controller.Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("PacketController::Start failed"));
    } else {
        /* Let user know connect details */
        printf("\nListening on %s:%d\n\n", controller.GetIPAddr().c_str(), g_port);
    }

    /* Parse commands from stdin */
    const int bufSize = 1024;
    char buf[bufSize];
    while ((ER_OK == status) && get_line(buf, bufSize, stdin)) {
        String line(buf);
        String cmd = NextTok(line);
        if (cmd == "debug") {
            String module = NextTok(line);
            String level = NextTok(line);
            if (module.empty() || level.empty()) {
                printf("Usage: debug <modulename> <level>\n");
            } else {
                QCC_SetDebugLevel(module.c_str(), StringToU32(level));
            }
        } else if (cmd == "connect") {
            String host = NextTok(line);
            uint16_t port = static_cast<uint16_t>(StringToU32(NextTok(line), 10, 0));
            if (host.empty() || (port == 0)) {
                printf("Usage: connect <addr> <port>\n");
            } else {
                DoConnect(controller, host, port);
            }
        } else if (cmd == "disconnect") {
            uint32_t connNum = StringToU32(NextTok(line), 10, 0);
            if (connNum == 0) {
                printf("Usage: disconnect <conn_index>\n");
            } else {
                DoDisconnect(controller, connNum);
            }
        } else if (cmd == "list") {
            DoList(controller);
        } else if (cmd == "send") {
            uint32_t idx = StringToU32(NextTok(line), 10, 0);
            String data = NextTok(line);
            if (!data.empty() && (idx != 0)) {
                DoSend(controller, idx, data, g_sendTtl);
            } else {
                printf("Invalid inputs\n");
            }
        } else if (cmd == "sendatrate") {
            uint32_t idx = StringToU32(NextTok(line), 10, 0);
            uint32_t msgSize = StringToU32(NextTok(line), 10, 0);
            uint32_t delay = StringToU32(NextTok(line), 10, numeric_limits<uint32_t>::max());
            uint32_t count = StringToU32(NextTok(line), 10, 0);
            if ((idx != 0) && (msgSize != 0) && (delay != numeric_limits<uint32_t>::max())) {
                QStatus status = DoSendAtRate(controller, idx, msgSize, delay, count, g_sendTtl);
                if (status != ER_OK) {
                    printf("DoSendAtRate failed with %s\n", QCC_StatusText(status));
                }
            } else {
                printf("Invalid args\n");
                printf("sendatrate <stream_idx> <msg_size> <ms_per_msg> <count>\n");
            }
        } else if (cmd == "sendtimeout") {
            uint32_t idx = StringToU32(NextTok(line), 10, 0);
            uint32_t timeout = StringToU32(NextTok(line), 10, 0);
            QStatus status = ER_FAIL;
            if (idx != 0) {
                status = controller.SetSendTimeout(idx, timeout);
                if (status != ER_OK) {
                    printf("SetSendTimeout failed with %s\n", QCC_StatusText(status));
                }
            } else {
                printf("Invalid args\n");
            }
            if (status != ER_OK) {
                printf("sendtimeout <stream_idx> <timeout>\n");
            }
        } else if (cmd == "sendttl") {
            uint32_t ttl = StringToU32(NextTok(line), 10, 58334438);
            if (ttl != 58334438) {
                g_sendTtl = ttl;
            } else {
                printf("Invalid args\n");
            }
            if (status != ER_OK) {
                printf("sendttl <ttl_in_ms>\n");
            }
        } else if (cmd == "recv") {
            uint32_t idx = StringToU32(NextTok(line), 10, 0);
            uint32_t len = StringToU32(NextTok(line), 10, 2048);
            if (idx != 0) {
                String data;
                data.reserve(len);
                QStatus status = DoRecv(controller, idx, data);
                if (status == ER_OK) {
                    printf("Data from #%u: %s\n\n", idx, data.c_str());
                }
            } else {
                printf("Invalid stream index\n");
                printf("recv <stream_idx> [maxLen]\n");
            }
        } else if (cmd == "recvatrate") {
            uint32_t idx = StringToU32(NextTok(line), 10, 0);
            uint32_t msgSize = StringToU32(NextTok(line), 10, 0);
            uint32_t delay = StringToU32(NextTok(line), 10, numeric_limits<uint32_t>::max());
            uint32_t count = StringToU32(NextTok(line), 10, 0);
            if ((idx != 0) && (msgSize != 0) && (delay != numeric_limits<uint32_t>::max())) {
                QStatus status = DoRecvAtRate(controller, idx, msgSize, delay, count, g_sendTtl);
                if (status != ER_OK) {
                    printf("DoRecvAtRate failed with %s\n", QCC_StatusText(status));
                }
            } else {
                printf("Invalid args\n");
                printf("recvatrate <stream_idx> <msg_size> <ms_per_msg> <count>\n");
            }
        } else if (cmd == "recvtimeout") {
            uint32_t timeout = StringToU32(NextTok(line), 10, 58334438);
            String testEnd = NextTok(line);
            if ((timeout != 58334438) && testEnd.empty()) {
                g_recvTimeout = timeout;
            } else {
                printf("Invalid args\n");
            }
            if (status != ER_OK) {
                printf("recvtimeout <timeout_in_ms>\n");
            }
        } else if (cmd == "exit") {
            break;
        } else if (cmd == "help") {
            printf("debug <module_name> <level>                               - Set debug level for a module\n");
            printf("connect <addr> <port>                                     - Connect to another instance of packettest\n");
            printf("disconnect <conn_num>                                     - Disconnect a specified connection\n");
            printf("list                                                      - List port bindings, discovered names and active sessions\n");
            printf("recv <stream_idx>                                         - Recv data from a connected stream\n");
            printf("recvatrate <stream_idx> <msg_size> <ms_per_msg> <count>   - Recv test msgs (from sendatrate)\n");
            printf("send <stream_idx> <data>                                  - Send data to a connected stream\n");
            printf("sendatrate <stream_idx> <msg_size> <ms_per_msg> <count>   - Send test data at specified rate\n");
            printf("sendtimeout <stream_idx> <timeout>                        - Set send timeout to specified ms\n");
            printf("sendttl <ttl_ms>                                          - Set per-message ttl to specified ms or 0 for infinite\n");
            printf("exit                                                      - Exit this program\n");
            printf("\n");
        } else {
            printf("Unknown command: %s\n", cmd.c_str());
        }
    }

    controller.Stop();
    controller.Join();
    return (int) status;
}
