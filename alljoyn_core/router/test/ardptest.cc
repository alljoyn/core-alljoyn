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
 *
 ******************************************************************************/
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#define random() rand()
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <vector>
#include <queue>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/Thread.h>
#include <qcc/StringUtil.h>
#include <alljoyn/Status.h>

#include <ArdpProtocol.h>

#define QCC_MODULE "ARDP"

using namespace std;
using namespace qcc;
using namespace ajn;

const uint32_t UDP_CONNECT_TIMEOUT = 1000;  /**< How long before we expect a connection to complete */
const uint32_t UDP_CONNECT_RETRIES = 10;  /**< How many times do we retry a connection before giving up */
const uint32_t UDP_INITIAL_DATA_TIMEOUT = 1000;  /**< Initial value for how long do we wait before retrying sending data */
const uint32_t UDP_TOTAL_DATA_RETRY_TIMEOUT = 5000;  /**< Total amount of time to try and send data before giving up */
const uint32_t UDP_MIN_DATA_RETRIES = 5;  /**< Minimum number of times to try and send data before giving up */
const uint32_t UDP_PERSIST_INTERVAL = 1000;  /**< How long do we wait before pinging the other side due to a zero window */
const uint32_t UDP_TOTAL_APP_TIMEOUT = 30000;  /**< How long to we try to ping for window opening before deciding app is not pulling data */
const uint32_t UDP_LINK_TIMEOUT = 30000;  /**< How long before we decide a link is down (with no reponses to keepalive probes */
const uint32_t UDP_KEEPALIVE_RETRIES = 5;  /**< How many times do we try to probe on an idle link before terminating the connection */
const uint32_t UDP_FAST_RETRANSMIT_ACK_COUNTER = 1; /**< How many duplicate acknowledgements to we need to trigger a data retransmission */
const uint32_t UDP_DELAYED_ACK_TIMEOUT = 100; /**< How long do we wait until acknowledging received segments */
const uint32_t UDP_TIMEWAIT = 1000;         /**< How long do we stay in TIMWAIT state before releasing the per-connection resources */
const uint32_t UDP_SEGBMAX = 65507;  /**< Maximum size of an ARDP message (for receive buffer sizing) */
const uint32_t UDP_SEGMAX = 50;      /**< Maximum number of ARDP messages in-flight (bandwidth-delay product sizing) */

char const* g_local_port = "9954";
char const* g_foreign_port = "9955";
char const* g_local_address = "127.0.0.1";
char const* g_foreign_address = "127.0.0.1";

char const* g_ajnConnString = "AUTH ANONYMOUS; BEGIN THE CONNECTION; Bus Hello; Bellevue";
char const* g_ajnAcceptString = "OK 123455678; Hello; Redmond";

std::map<ArdpConnRecord*, std::queue<ArdpRcvBuf*> > RecvMapQueue;
std::map<uint32_t, ArdpConnRecord*> connList;
static int g_conn = 0;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

char* get_line(char*str, size_t num, FILE*fp)
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
static void RemoveConn(ArdpConnRecord*conn) {

    std::map<uint32_t, ArdpConnRecord*>::iterator it = connList.begin();
    while (it != connList.end()) {
        if (conn == it->second) {
            connList.erase(it++);
        } else {
            ++it;
        }
    }
}

static bool FindConn(uint32_t number) {

    std::map<uint32_t, ArdpConnRecord*>::iterator it = connList.begin();
    it = connList.find(number);
    if (it != connList.end()) {
        return true;
    } else { return false; }
}

void GetData(ArdpRcvBuf* rcv) {

    ArdpRcvBuf* buf = rcv;
    uint32_t len = 0;
    uint16_t count = rcv->fcnt;
    /* Consume data buffers */
    for (uint16_t i = 0; i < count; i++) {
        printf("RecvCb(): got %d bytes of data \n", buf->datalen);
        len += buf->datalen;
        buf = buf->next;
    }

}

bool AcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    printf("Inside Accept callback, we received a SYN from %s:%d, the message is \"%s\", status %s \n", ipAddr.ToString().c_str(), ipPort, (char*)buf, QCC_StatusText(status));
    printf("Connection no is  %d, conn pointer is   %p \n", g_conn, conn);
    connList[g_conn] = conn;
    g_conn++;
    //int option=0;
    //printf("Do you want to accept(1) it or not? ");
    //scanf("%d",&option);
    //if(option==1) {
    //  printf("Returning true..\n");
    //  return true;
    //} else {
    //  printf("Returning false..\n");
    //  return false;
    //}
    return true;
}

void ConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    printf("Looks like I have connected... conn=%p is passive=%s , the message(len = %d) is \"%s\", status is %s \n", conn, (passive) ? "true" : "false", len, (char*)buf, QCC_StatusText(status));
    if (!passive) {
        printf("Connection no is  %d, conn pointer is   %p \n", g_conn, conn);
        connList[g_conn] = conn;
        g_conn++;
    }
}

void DisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    printf("Looks like I have disconnected conn = %p, reason = %s\n", conn, QCC_StatusText(status));
    RecvMapQueue.erase(conn);
    RemoveConn(conn);
}

void RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status)
{
    printf("RECV- %u conn = %p \n", rcv->seq, conn);

    RecvMapQueue[conn].push(rcv);
}

void SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    printf("SENT- %u, conn = %p \n", len, conn);
}

void SendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    printf("WINDOW RECEIVED-  %u, conn = %p \n", window, conn);
}

class ThreadClass : public Thread {

  public:
    ThreadClass(char*name, ArdpHandle* handle, qcc::SocketFd sock) : Thread(name) {
        m_handle = handle;
        m_sock = sock;
    }

  protected:
    qcc::ThreadReturn STDCALL Run(void* arg) {

        while ((!g_interrupt) && (IsRunning())) {
            uint32_t ms;
            ARDP_Run(m_handle, m_sock, true, false, &ms);
            //qcc::Sleep(1000);
        }

        return this;
    }

  private:
    ArdpHandle* m_handle;
    qcc::SocketFd m_sock;

};

static void Print_Conn() {
    std::map<uint32_t, ArdpConnRecord*>::iterator it;
    printf("===================================================== \n");
    for (it = connList.begin(); it != connList.end(); ++it)
        printf("%u  %p  \n", it->first, it->second);

    printf("===================================================== \n");
}

static void usage() {
    printf("connect \n");
    printf("accept  \n");
    printf("send #<connection number> \n");
    printf("recv #connection number \n");
    printf("recvall #connection number \n");
    printf("sendall #connection number \n");
    printf("disconnect #connection number \n");
    printf("exit \n");
    printf("help \n");
    printf("list \n");
}

int main(int argc, char** argv)
{
    QStatus status = ER_OK;

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-lp", argv[i])) {
            g_local_port = argv[i + 1];
            i++;
        } else if (0 == strcmp("-fp", argv[i])) {
            g_foreign_port = argv[i + 1];
            i++;
        } else if (0 == strcmp("-la", argv[i])) {
            g_local_address = argv[i + 1];
            i++;
        } else if (0 == strcmp("-fa", argv[i])) {
            g_foreign_address = argv[i + 1];
            i++;
        } else {
            printf("Unknown option %s\n", argv[i]);
            exit(0);
        }
    }

    printf("g_local_port == %s\n", g_local_port);
    printf("g_foreign_port == %s\n", g_foreign_port);
    printf("g_local_address == %s\n", g_local_address);
    printf("g_foreign_address == %s\n", g_foreign_address);

    signal(SIGINT, SigIntHandler);


    //One time activity- Create a socket, set to blocking, bind it to local port, local address
    qcc::SocketFd sock;

    status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_DGRAM, sock);
    if (status != ER_OK) {
        QCC_LogError(status, ("Test::Run(): Socket(): Failed"));
        return 0;
    }

    status = qcc::SetBlocking(sock, false);
    if (status != ER_OK) {
        QCC_LogError(status, ("Test::Run(): SetBlocking(): Failed"));
        return 0;
    }

    status = qcc::Bind(sock, qcc::IPAddress(g_local_address), atoi(g_local_port));
    if (status != ER_OK) {
        QCC_LogError(status, ("Test::Run(): Bind(): Failed"));
        return 0;
    }

    //Populate default values for timers, couters, etc.
    ArdpGlobalConfig config;
    config.connectTimeout = UDP_CONNECT_TIMEOUT;
    config.connectRetries = UDP_CONNECT_RETRIES;
    config.initialDataTimeout = UDP_INITIAL_DATA_TIMEOUT;
    config.totalDataRetryTimeout = UDP_TOTAL_DATA_RETRY_TIMEOUT;
    config.minDataRetries = UDP_MIN_DATA_RETRIES;
    config.persistInterval = UDP_PERSIST_INTERVAL;
    config.totalAppTimeout = UDP_TOTAL_APP_TIMEOUT;
    config.linkTimeout = UDP_LINK_TIMEOUT;
    config.keepaliveRetries = UDP_KEEPALIVE_RETRIES;
    config.fastRetransmitAckCounter = UDP_FAST_RETRANSMIT_ACK_COUNTER;
    config.delayedAckTimeout = UDP_DELAYED_ACK_TIMEOUT;
    config.timewait = UDP_TIMEWAIT;
    config.segbmax = UDP_SEGBMAX;
    config.segmax = UDP_SEGMAX;

    //Allocate a handle (ARDP protocol instance).
    ArdpHandle* handle = ARDP_AllocHandle(&config);

    //Set the callbacks- accept, connect, disconnectcb, recv, send, sendwindow
    ARDP_SetAcceptCb(handle, AcceptCb);
    ARDP_SetConnectCb(handle, ConnectCb);
    ARDP_SetDisconnectCb(handle, DisconnectCb);
    ARDP_SetRecvCb(handle, RecvCb);
    ARDP_SetSendCb(handle, SendCb);
    ARDP_SetSendWindowCb(handle, SendWindowCb);

    ArdpConnRecord* conn;

    //The side can behave as a server or client. Teach it to behave as a server.
    // This API is only for server side.
    ARDP_StartPassive(handle);

    ThreadClass t1((char*)"t1", handle, sock);
    t1.Start();


    while (!g_interrupt) {

        char option[100];
        get_line(option, 100, stdin);
        String line(option);
        String cmd = NextTok(line);

        if (strcmp(cmd.c_str(), "connect") == 0) {
            char foreign_port[10];
            char foreign_address[20];
            printf("Enter the foreign port.. \n");
            if (scanf("%s", foreign_port) != 1) {
                printf("Error reading foreign port\n");
            } else {
                printf("Enter the foreign address.. \n");
                if (scanf("%s", foreign_address) != 1) {
                    printf("Error reading foreign address\n");
                } else {
                    status = ARDP_Connect(handle, sock, qcc::IPAddress(foreign_address), atoi(foreign_port), UDP_SEGMAX, UDP_SEGBMAX, &conn, (uint8_t*)g_ajnConnString, strlen(g_ajnConnString) + 1, NULL);
                    if (status != ER_OK) {
                        printf("Error while calling ARDP_Connect..  %s \n", QCC_StatusText(status));
                    }
                }
            }
        }

        if (strcmp(cmd.c_str(), "send") == 0) {
            String connno = NextTok(line);
            if (connno.empty()) {
                printf("Usage: send #connection\n");
            } else {
                uint32_t t_connno = StringToU32(connno, 0, 0);
                if (FindConn(t_connno)) {
                    uint32_t length = StringToU32(NextTok(line), 0, random() % 135000);
                    uint32_t ttl = StringToU32(NextTok(line), 0, 0);
                    uint8_t* buffer = new uint8_t[length];
                    status = ARDP_Send(handle, connList[t_connno], buffer, length, ttl);
                    if (status != ER_OK) {
                        printf("Error while ARDP_Send.. %s \n", QCC_StatusText(status));
                    } else {
                        printf("ARDP_Send successful on %p data[%u] = %u \n", connList[t_connno], length - 1, buffer[length - 1]);
                    }
                    delete [] buffer;
                } else {
                    printf("Invalid connection \n");
                }
            }
        }

        if (strcmp(cmd.c_str(), "recv") == 0) {
            String connno = NextTok(line);
            if (connno.empty()) {
                printf("Usage: recv #connection\n");
            } else {
                uint32_t t_connno = StringToU32(connno, 0, 0);
                if (FindConn(t_connno)) {
                    if (RecvMapQueue[connList[t_connno]].empty()) {
                        printf("Nothing to release. \n");
                    } else {
                        ArdpRcvBuf*rcv = NULL;
                        rcv = RecvMapQueue[connList[t_connno]].front();
                        GetData(rcv);
                        RecvMapQueue[connList[t_connno]].pop();
                        printf("ARDP_RecvReady about to be called on %p  \n", connList[t_connno]);
                        status = ARDP_RecvReady(handle, connList[t_connno], rcv);
                        if (status != ER_OK) {
                            printf("Error while ARDP_Recv.. %s \n", QCC_StatusText(status));
                        } else {
                            printf("ARDP_RecvReady successful on %p  \n", connList[t_connno]);
                        }
                    }
                } else {
                    printf("Invalid connection \n");
                }
            }
        }

        if (strcmp(cmd.c_str(), "accept") == 0) {
            status = ARDP_Accept(handle, connList[g_conn - 1], UDP_SEGMAX, UDP_SEGBMAX, (uint8_t* )g_ajnAcceptString, strlen(g_ajnAcceptString) + 1);
            if (status != ER_OK) {
                printf("Error while ARDP_Accept.. %s \n", QCC_StatusText(status));
            }
        }

        if (strcmp(cmd.c_str(), "disconnect") == 0) {
            String connno = NextTok(line);
            if (connno.empty()) {
                printf("Usage: disconnect #connection\n");
            } else {
                uint32_t t_connno = StringToU32(connno, 0, 0);
                if (FindConn(t_connno)) {
                    status =  ARDP_Disconnect(handle, connList[t_connno]);
                    if (status != ER_OK) {
                        printf("Error while ARDP_Disconnect.. %s \n", QCC_StatusText(status));
                    }
                } else {
                    printf("Invalid connection \n");
                }
            }
        }

        if (strcmp(cmd.c_str(), "recvall") == 0) {
            String connno = NextTok(line);
            if (connno.empty()) {
                printf("Usage: recvall #connection\n");
            } else {
                uint32_t t_connno = StringToU32(connno, 0, 0);
                if (FindConn(t_connno)) {

                    while (!RecvMapQueue[connList[t_connno]].empty()) {
                        ArdpRcvBuf*rcv = NULL;
                        rcv = RecvMapQueue[connList[t_connno]].front();
                        GetData(rcv);
                        RecvMapQueue[connList[t_connno]].pop();
                        printf("ARDP_RecvReady about to be called on %p  \n", connList[t_connno]);
                        status = ARDP_RecvReady(handle, connList[t_connno], rcv);
                        if (status != ER_OK) {
                            printf("Error while ARDP_Recv.. %s \n", QCC_StatusText(status));
                            break;
                        } else {
                            printf("ARDP_RecvReady successful on %p  \n", connList[t_connno]);
                        }
                    }
                    printf("OK \n");

                } else {
                    printf("Invalid connection \n");
                }
            }
        }


        if (strcmp(cmd.c_str(), "sendall") == 0) {
            String connno = NextTok(line);
            if (connno.empty()) {
                printf("Usage: sendall #connection\n");
            } else {
                uint32_t t_connno = StringToU32(connno, 0, 0);
                if (FindConn(t_connno)) {

                    while (true) {
                        uint32_t length = random() % (135000);
                        uint8_t* buffer = new uint8_t[length];
                        status = ARDP_Send(handle, connList[t_connno], buffer, length, 0);
                        if (status != ER_OK) {
                            printf("Error while ARDP_Send.. %s \n", QCC_StatusText(status));
                            break;
                        } else {
                            printf("ARDP_Send successful on %p data[%u] = %u \n", connList[t_connno], length - 1, buffer[length - 1]);
                        }
                        delete [] buffer;
                    }

                } else {
                    printf("Invalid connection \n");
                }
            }
        }


        if (strcmp(cmd.c_str(), "help") == 0) {
            usage();
        }

        if (strcmp(cmd.c_str(), "exit") == 0) {
            break;
        }

        if (strcmp(cmd.c_str(), "list") == 0) {
            Print_Conn();
        }
    }

    t1.Stop();
    t1.Join();

    return 0;
}
