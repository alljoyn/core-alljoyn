/**
 * @file
 * Unit test program for passing socket handles via AllJoyn.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <vector>

#include <qcc/Environ.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/time.h>

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
namespace sock_test {
const char* Interface = "org.alljoyn.sock_test";
const char* Service = "org.alljoyn.sock_test";
const char* Path = "/org/alljoyn/sock_test";
}
}
}

static BusAttachment* gBus = NULL;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

static void usage(void)
{
    printf("Usage: sock_test\n\n");
    printf("Options: -c|-s [-h]\n");
    printf("   -h                    = Print this help message\n");
    printf("   -s                    = Selects server mode\n");
    printf("   -c                    = Selects client mode\n");
    printf("   -i #                  = Number of iterations\n");
    printf("   -gai HOST             = Run getaddrinfo for HOST\n");
    printf("\n");
}

static const char ifcXML[] =
    "<node name=\"/org/alljoyn/sock_test\">"
    "  <interface name=\"org.alljoyn.sock_test\">"
    "    <method name=\"PutSock\">"
    "      <arg name=\"sock\" type=\"h\" direction=\"in\"/>"
    "      <arg name=\"sockOut\" type=\"h\" direction=\"out\"/>"
    "    </method>"
    "    <method name=\"GetSock\">"
    "      <arg name=\"sock\" type=\"h\" direction=\"out\"/>"
    "    </method>"
    "  </interface>"
    "</node>";


class SockService : public BusObject {
  public:

    SockService(BusAttachment& bus) : BusObject(::org::alljoyn::sock_test::Path)
    {
        const InterfaceDescription* ifc = bus.GetInterface(::org::alljoyn::sock_test::Interface);
        if (ifc) {
            AddInterface(*ifc);
            AddMethodHandler(ifc->GetMember("PutSock"), static_cast<MessageReceiver::MethodHandler>(&SockService::PutSock));
            AddMethodHandler(ifc->GetMember("GetSock"), static_cast<MessageReceiver::MethodHandler>(&SockService::GetSock));
        }
    }

    void ObjectRegistered(void)
    {
        assert(bus);
        BusObject::ObjectRegistered();
        const ProxyBusObject& dbusObj = bus->GetDBusProxyObj();
        MsgArg args[2];
        size_t numArgs = ArraySize(args);
        MsgArg::Set(args, numArgs, "su", ::org::alljoyn::sock_test::Service, 6);
        QStatus status = dbusObj.MethodCallAsync(ajn::org::freedesktop::DBus::InterfaceName,
                                                 "RequestName",
                                                 this,
                                                 static_cast<MessageReceiver::ReplyHandler>(&SockService::NameAcquiredCB),
                                                 args,
                                                 numArgs);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to request name %s", ::org::alljoyn::sock_test::Service));
        }
    }

    void PutSock(const InterfaceDescription::Member* member, Message& msg)
    {
        QStatus status;
        SocketFd handle;

        status = msg->GetArgs("h", &handle);
        if (status == ER_OK) {
            status = qcc::SocketDup(handle, handle);
            if (status == ER_OK) {
                status = MethodReply(msg, msg->GetArg(0), 1);
            } else {
                status = MethodReply(msg, status);
            }
        }
        if (status == ER_OK) {
            const char hello[] = "hello world\n";
            size_t sent;
            status = qcc::Send(handle, hello, sizeof(hello), sent);
            if (status == ER_OK) {
                printf("sent %d bytes\n", (int)sent);
            } else {
                QCC_LogError(status, ("qcc::Send failed"));
            }
            qcc::Close(handle);
        }
    }

    void GetSock(const InterfaceDescription::Member* member, Message& msg)
    {
    }

    void NameAcquiredCB(Message& msg, void* context)
    {
        uint32_t ownership = 0;
        QStatus status = msg->GetArgs("u", &ownership);
        if ((status != ER_OK) || (ownership != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)) {
            QCC_LogError(status, ("Failed to obtain name (ownership=%d) %s", ownership, ::org::alljoyn::sock_test::Service));
        }
    }
};

class ListenThread : public qcc::Thread {
  public:
    ListenThread(qcc::SocketFd& sock, IPAddress& addr, uint16_t port) : qcc::Thread("AcceptThread"), sock(sock), addr(addr), port(port), listening(false) { }

    bool IsListening() { return listening; }

  private:

    qcc::ThreadReturn STDCALL Run(void* arg)
    {
        qcc::SocketFd* newSock = static_cast<qcc::SocketFd*>(arg);
        QStatus status = qcc::Listen(sock, 0);
        if (status == ER_OK) {
            listening = true;
            Accept(sock, addr, port, *newSock);
        }
        listening = false;
        return NULL;
    }

    qcc::SocketFd sock;
    IPAddress addr;
    uint16_t port;
    bool listening;
};

QStatus SocketPair(qcc::SocketFd* socks, uint16_t port)
{
    QStatus status;
    IPAddress addr;

    addr.SetAddress("127.0.0.1");
    qcc::SocketFd listenFd = qcc::INVALID_SOCKET_FD;

    status = qcc::Socket(QCC_AF_INET, QCC_SOCK_STREAM, listenFd);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create listen socket"));
        goto Exit;
    }
    status = Bind(listenFd, addr, port);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed bind listen socket"));
        goto Exit;
    }
    status = qcc::Socket(QCC_AF_INET, QCC_SOCK_STREAM, socks[0]);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create connect socket"));
        goto Exit;
    }
    {
        ListenThread listener(listenFd, addr, port);
        listener.Start(&socks[1]);
        /*
         * Wait until the listener thread is actually listening before attempting to connect.
         */
        while (!listener.IsListening()) {
            qcc::Sleep(5);
            if (!listener.IsRunning()) {
                break;
            }
        }
        if (listener.IsListening()) {
            status = qcc::Connect(socks[0], addr, port);
        }
        listener.Join();
    }

Exit:

    if (listenFd != qcc::INVALID_SOCKET_FD) {
        qcc::Close(listenFd);
    }
    return status;
}

int main(int argc, char** argv)
{
    qcc::SocketFd handles[2] = { 0, 0 };
    QStatus status = ER_OK;
    BusAttachment bus("sock_test");
    bool client = false;
    bool server = false;
    bool gai = false;
    char* host = NULL;
    uint32_t iterations = 1;
    Environ* env;
    qcc::String connectArgs;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (strcmp("-h", argv[i]) == 0) {
            usage();
            exit(0);
        }
        if (strcmp("-c", argv[i]) == 0) {
            client = true;
            continue;
        }
        if (strcmp("-s", argv[i]) == 0) {
            server = true;
            continue;
        }
        if (strcmp("-i", argv[i]) == 0) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
            } else {
                iterations = strtoul(argv[i], NULL, 10);
                continue;
            }
        }
        if (strcmp("-gai", argv[i]) == 0) {
            gai = true;
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
            } else {
                host = argv[i];
                continue;
            }
        }
        printf("Unknown option %s\n", argv[i]);
        usage();
        exit(1);
    }
    if ((!client && !server && !gai) || (client && server)) {
        usage();
        exit(1);
    }

    if (gai) {
        IPAddress tempAddr;
        status = tempAddr.SetAddress(host, true, 5000);
        if (ER_OK == status) {
            printf("%s -> %s\n", host, tempAddr.ToString().c_str());
        }
        goto Exit;
    }

    /* Get env vars */
    env = Environ::GetAppEnviron();
    connectArgs = env->Find("BUS_ADDRESS");

    /* Start the msg bus */
    status = bus.Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment::Start failed"));
        goto Exit;
    }
    gBus = &bus;

    /* Connect to the bus */
    if (connectArgs.empty()) {
        status = bus.Connect();
    } else {
        status = bus.Connect(connectArgs.c_str());
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment::Connect(\"%s\") failed", bus.GetConnectSpec().c_str()));
        goto Exit;
    }


    if (client) {

        /* Create the proxy object */
        ProxyBusObject remoteObj(bus, ::org::alljoyn::sock_test::Service, ::org::alljoyn::sock_test::Path, 0);
        status = remoteObj.ParseXml(ifcXML, "sock_test");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to parse XML"));
            goto Exit;
        }
        for (uint32_t i = 0; i < iterations; ++i) {
            printf("Iteration %u: ", i + 1);
            /* Create a connected pair if sockets */
            status = SocketPair(handles, 9900 + i);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to create a pair of sockets"));
                goto Exit;
            }
            if (status == ER_OK) {
                Message reply(bus);
                MsgArg arg("h", handles[0]);
                status = remoteObj.MethodCall(::org::alljoyn::sock_test::Interface, "PutSock", &arg, 1, reply);
                /* Don't need this handle anymore */
                qcc::Close(handles[0]);
                handles[0] = 0;
                if (ER_OK == status) {
                    uint8_t buf[256];
                    size_t recvd;
                    /* Read from the socket */
                    while (true) {
                        status = qcc::Recv(handles[1], buf, sizeof(buf), recvd);
                        /* This is just a test program so try again if the read blocks */
                        if (status == ER_WOULDBLOCK) {
                            qcc::Sleep(1);
                            continue;
                        }
                        break;
                    }
                    if (status == ER_OK) {
                        printf("received %d bytes: %s", (int)recvd, buf);
                    } else {
                        QCC_LogError(status, ("Recv failed"));
                    }
                    /* Don't need this handle anymore */
                    qcc::Close(handles[1]);
                    handles[1] = 0;
                } else {
                    QCC_LogError(status, ("PutSock failed"));
                }
            }
            if (g_interrupt) {
                break;
            }
        }
    } else {
        QStatus status = bus.CreateInterfacesFromXml(ifcXML);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to parse XML"));
            goto Exit;
        }
        SockService sockService(bus);
        bus.RegisterBusObject(sockService);

        while (g_interrupt == false) {
            qcc::Sleep(100);
        }
    }

Exit:
    if (handles[0] != 0) {
        qcc::Close(handles[0]);
    }
    if (handles[1] != 0) {
        qcc::Close(handles[1]);
    }
    printf("sock_test exiting with status %d (%s)\n", status, QCC_StatusText(status));
    return (int) status;
}
