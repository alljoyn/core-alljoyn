/******************************************************************************
 *
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <gtest/gtest.h>
#include <utility>

#include <qcc/Socket.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

using namespace qcc;
using namespace std;

static void DeliverLine(AddressFamily addr_family, SocketType sock_type, String& line) {
    IPAddress hostAddr = (QCC_AF_INET6 == addr_family) ?
                         IPAddress("::1") : IPAddress("127.0.0.1");

    SocketFd clientFd = INVALID_SOCKET_FD;
    uint16_t clientPort = 0;
    EXPECT_EQ(ER_OK, Socket(addr_family, sock_type, clientFd));
    EXPECT_EQ(ER_OK, Bind(clientFd, hostAddr, clientPort));
    EXPECT_EQ(ER_OK, GetLocalAddress(clientFd, hostAddr, clientPort));

    SocketFd serverFd = INVALID_SOCKET_FD;
    uint16_t serverPort = 0;
    EXPECT_EQ(ER_OK, Socket(addr_family, sock_type, serverFd));
    EXPECT_EQ(ER_OK, Bind(serverFd, hostAddr, serverPort));
    EXPECT_EQ(ER_OK, GetLocalAddress(serverFd, hostAddr, serverPort));

    SocketFd acceptedFd = INVALID_SOCKET_FD;
    if (QCC_SOCK_STREAM == sock_type) {
        EXPECT_EQ(ER_OK, Listen(serverFd, 1));
        EXPECT_EQ(ER_OK, Connect(clientFd, hostAddr, serverPort));
        EXPECT_EQ(ER_OK, Accept(serverFd, hostAddr, clientPort, acceptedFd));
        EXPECT_EQ(ER_OK, SetBlocking(clientFd, true));
        EXPECT_EQ(ER_OK, SetBlocking(acceptedFd, true));
    }

    const char* sendBuf = line.c_str();
    const size_t numBytes = line.length() + 1;

    size_t numSent = 0;
    EXPECT_EQ(ER_OK, SendTo(clientFd, hostAddr, serverPort, static_cast<const void*> (sendBuf), numBytes, numSent));
    uint8_t* recvBuf = new uint8_t[numBytes];
    size_t numRecvd = 0;
    if (QCC_SOCK_STREAM == sock_type) {
        EXPECT_EQ(ER_OK, Recv(acceptedFd, static_cast<void*>(recvBuf), numSent, numRecvd));
    } else {
        EXPECT_EQ(ER_OK, RecvFrom(serverFd, hostAddr, clientPort, static_cast<void*>(recvBuf), numSent, numRecvd));
    }

    EXPECT_EQ(numSent, numRecvd);
    if (numSent == numRecvd) {
        for (uint8_t i = 0; i < numRecvd; i++) {
            EXPECT_EQ(static_cast<uint8_t>(sendBuf[i]), recvBuf[i]);
        }
    }

    delete [] recvBuf;
    Close(clientFd);
    if (QCC_SOCK_STREAM == sock_type) {
        Close(acceptedFd);
    }
    Close(serverFd);
}

class SendToAndRecvFrom : public::testing::TestWithParam<pair<AddressFamily, SocketType> > {
};

TEST_P(SendToAndRecvFrom, SendAndReceive) {
    AddressFamily addrFamily = GetParam().first;
    SocketType sockType = GetParam().second;

    const char* wilson_lines[] = {
        "",
        "That smugness of yours really is an attractive quality.",
        "I'm still amazed you're actually in the same room with a patient.",
        "Beauty often seduces us on the road to truth.",
        "I'm not gonna date a patient's daughter.",
        "You really don't need to know everything about everybody.",
        "Be yourself: cold, uncaring, distant.",
        "Did you know your phone is dead? Do you ever recharge the batteries?",
        "Now, why do you have a season pass to The New Yankee Workshop?"
    };
    const char* house_lines[] = {
        "",
        "Thank you. It was either that or get my hair highlighted. Smugness is easier to maintain.",
        "People don't bug me until they get teeth.",
        "And triteness kicks us in the nads.",
        "Very ethical. Of course, most married men would say they don't date at all.",
        "I don't need to watch The O.C., but it makes me happy.",
        "Please, don't put me on a pedestal.",
        "They recharge? I just keep buying new phones.",
        "It's a complete moron working with power tools. How much more suspenseful can you get?"
    };

    for (uint8_t i = 0; i < ArraySize(wilson_lines) + ArraySize(house_lines); i++) {
        String line = (0 == i % 2) ? String(wilson_lines[i / 2]) : String(house_lines[i / 2]);
        DeliverLine(addrFamily, sockType, line);
    }
}

INSTANTIATE_TEST_CASE_P(SocketTest, SendToAndRecvFrom,
                        ::testing::Values(pair<AddressFamily, SocketType>(QCC_AF_INET6, QCC_SOCK_DGRAM),
                                          pair<AddressFamily, SocketType>(QCC_AF_INET6, QCC_SOCK_STREAM),
                                          pair<AddressFamily, SocketType>(QCC_AF_INET, QCC_SOCK_DGRAM),
                                          pair<AddressFamily, SocketType>(QCC_AF_INET, QCC_SOCK_STREAM)));

/*
 * File descriptors are local to a machine and are not meaningful beyond
 * the machine boundaries. Hence, SendWithFds and RecvWithFds functions
 * make sense only when used with a 'local' socket.
 *
 * On POSIX systems, the 'local' socket would mean unix domain socket.
 *
 * On Windows, the 'local' sockets would be a socket on the loopback address.
 *
 * On both platforms (POSIX and Windows), the choice of transport can
 * theoretically be either 'SOCK_STREAM' or not. However, the API signatures
 *
 * SendWithFds(SocketFd sockfd, const void* buf, size_t len, size_t& sent,
 *             SocketFd* fdList, size_t numFds, uint32_t pid);
 * RecvWithFds(SocketFd sockfd, void* buf, size_t len, size_t& received,
 *             SocketFd* fdList, size_t maxFds, size_t& recvdFds);
 *
 * implicitly indicate (lack of parameters specifying
 * the other communication endpoint) that the socket is 'connected'.
 */

class WithFdsTest : public::testing::TestWithParam<pair<AddressFamily, SocketType> > {
};

TEST_P(WithFdsTest, SendAndReceive) {
    AddressFamily addrFamily = GetParam().first;
    SocketType sockType = GetParam().second;

    SocketFd endpoint[2];
    EXPECT_EQ(ER_OK, SocketPair(endpoint));

    String sender_message = String("Sending a list of some dummy fds.");
    size_t numFds = SOCKET_MAX_FILE_DESCRIPTORS;
    SocketFd* dummyFds = new SocketFd[numFds];
    for (uint8_t i = 0; i < numFds; i++) {
        dummyFds[i] = INVALID_SOCKET_FD;
    }

    IPAddress hostAddr = (QCC_AF_INET6 == addrFamily) ? IPAddress("::1") : IPAddress("127.0.0.1");

    /* Initialize the dummy list of fds */
    for (uint8_t i = 0; i < numFds; i++) {
        EXPECT_EQ(ER_OK, Socket(addrFamily, sockType, dummyFds[i]));
        EXPECT_EQ(ER_OK, Bind(dummyFds[i], hostAddr, 0));
    }

    const char* sendBuf = sender_message.c_str();
    const size_t numBytes = sender_message.length();
    size_t actualBytesSent = 0;
    EXPECT_EQ(ER_OK, SendWithFds(endpoint[0],
                                 static_cast<const void*>(sendBuf), numBytes,
                                 actualBytesSent,
                                 dummyFds, numFds, GetPid()));

    uint8_t* recvBuf = new uint8_t[numBytes];
    size_t actualBytesRecvd = 0;
    SocketFd* fds = new SocketFd[numFds];
    size_t actualFdsRecvd = 0;
    EXPECT_EQ(ER_OK, RecvWithFds(endpoint[1],
                                 static_cast<void*>(recvBuf), numBytes,
                                 actualBytesRecvd,
                                 fds, numFds, actualFdsRecvd));

    /* Compare sent and received message octets */
    EXPECT_EQ(actualBytesSent, actualBytesRecvd);
    for (uint8_t i = 0; i < actualBytesRecvd; i++) {
        EXPECT_EQ(static_cast<uint8_t>(sendBuf[i]), recvBuf[i]);
    }

    /* Compare sent and received fds */
    EXPECT_EQ(numFds, actualFdsRecvd);
    for (uint8_t i = 0; i < actualFdsRecvd; i++) {
        uint16_t dummyAddr, fdAddr;
        EXPECT_EQ(ER_OK, GetLocalAddress(dummyFds[i], hostAddr, dummyAddr));
        EXPECT_EQ(ER_OK, GetLocalAddress(fds[i], hostAddr, fdAddr));
        EXPECT_EQ(dummyAddr, fdAddr);
    }

    for (uint8_t i = 0; i < actualFdsRecvd; i++) {
        Close(fds[i]);
    }
    for (uint8_t i = 0; i < numFds; i++) {
        Close(dummyFds[i]);
    }
    delete [] fds;
    delete [] recvBuf;
    delete [] dummyFds;
    Close(endpoint[0]);
    Close(endpoint[1]);
}

INSTANTIATE_TEST_CASE_P(SocketTest, WithFdsTest,
                        ::testing::Values(pair<AddressFamily, SocketType>(QCC_AF_INET6, QCC_SOCK_DGRAM),
                                          pair<AddressFamily, SocketType>(QCC_AF_INET6, QCC_SOCK_STREAM),
                                          pair<AddressFamily, SocketType>(QCC_AF_INET, QCC_SOCK_DGRAM),
                                          pair<AddressFamily, SocketType>(QCC_AF_INET, QCC_SOCK_STREAM)));

class SocketTestErrors : public testing::Test {
  public:
    SocketFd serverFd;
    IPAddress serverAddr;
    uint16_t serverPort;
    SocketFd clientFd;
    SocketFd acceptedFd;
    uint8_t buf[32768];
    size_t numRecvd;
    size_t numSent;
    QStatus status;

    static ThreadReturn STDCALL ServerAccept(void* arg)
    {
        SocketFd serverFd = reinterpret_cast<intptr_t>(arg);
        IPAddress clientAddr;
        uint16_t clientPort;
        SocketFd clientFd = INVALID_SOCKET_FD;
        EXPECT_EQ(ER_OK, Accept(serverFd, clientAddr, clientPort, clientFd));
        EXPECT_EQ(ER_OK, SetBlocking(clientFd, true));
        return reinterpret_cast<ThreadReturn>(clientFd);
    }

    virtual void SetUp()
    {
        serverFd = INVALID_SOCKET_FD;
        clientFd = INVALID_SOCKET_FD;
        acceptedFd = INVALID_SOCKET_FD;

        /* Server SetUp */
        EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, serverFd));
        serverAddr = Environ::GetAppEnviron()->Find("IP_ADDRESS", "127.0.0.1");
        serverPort = 0;
        EXPECT_EQ(ER_OK, Bind(serverFd, serverAddr, serverPort));
        EXPECT_EQ(ER_OK, GetLocalAddress(serverFd, serverAddr, serverPort));
        EXPECT_EQ(ER_OK, Listen(serverFd, 1));

        /* Connect Server and Client */
        Thread acceptThread("ServerAccept", ServerAccept);
        EXPECT_EQ(ER_OK, acceptThread.Start(reinterpret_cast<void*>(serverFd)));
        EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, clientFd));
        EXPECT_EQ(ER_OK, qcc::Connect(clientFd, serverAddr, serverPort));
        EXPECT_EQ(ER_OK, SetBlocking(clientFd, true));
        EXPECT_EQ(ER_OK, acceptThread.Join());
        acceptedFd = reinterpret_cast<intptr_t>(acceptThread.GetExitValue());
    }

    virtual void TearDown()
    {
        Close(acceptedFd);
        Close(clientFd);
        Close(serverFd);
    }

    void Close(SocketFd& sockFd)
    {
        if (sockFd != INVALID_SOCKET_FD) {
            qcc::Close(sockFd);
            sockFd = INVALID_SOCKET_FD;
        }
    }
};

TEST_F(SocketTestErrors, RecvWhenNotConnected)
{
    SocketFd sockFd = INVALID_SOCKET_FD;
    EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockFd));
    EXPECT_EQ(ER_OS_ERROR, Recv(sockFd, buf, 1, numRecvd));
    Close(sockFd);
}

TEST_F(SocketTestErrors, RecvAfterShutdown)
{
    Shutdown(clientFd, QCC_SHUTDOWN_WR);
    EXPECT_EQ(ER_OK, Recv(acceptedFd, buf, 1, numRecvd));
    EXPECT_EQ(0U, numRecvd);
}

TEST_F(SocketTestErrors, RecvAfterOrderlyRelease)
{
    Close(clientFd);
    EXPECT_EQ(ER_OK, Recv(acceptedFd, buf, 1, numRecvd));
    EXPECT_EQ(0U, numRecvd);
}

TEST_F(SocketTestErrors, RecvAfterAbortiveRelease)
{
    SetLinger(clientFd, true, 0);
    Close(clientFd);
    EXPECT_EQ(ER_OS_ERROR, Recv(acceptedFd, buf, 1, numRecvd));
}

TEST_F(SocketTestErrors, RecvWouldBlock)
{
    EXPECT_EQ(ER_OK, SetBlocking(acceptedFd, false));
    EXPECT_EQ(ER_WOULDBLOCK, Recv(acceptedFd, buf, 1, numRecvd));
}

TEST_F(SocketTestErrors, SendWhenConnected)
{
    EXPECT_EQ(ER_OK, Send(acceptedFd, buf, 1, numSent));
}

TEST_F(SocketTestErrors, SendWhenNotConnected)
{
    SocketFd sockFd = INVALID_SOCKET_FD;
    EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockFd));
    EXPECT_EQ(ER_OS_ERROR, Send(sockFd, buf, 0, numSent));
    EXPECT_EQ(ER_OS_ERROR, Send(sockFd, buf, 1, numSent));
    Close(sockFd);
}

TEST_F(SocketTestErrors, SendAfterOrderlyRelease)
{
    Close(clientFd);
    while ((status = Send(acceptedFd, buf, ArraySize(buf), numSent)) == ER_OK)
        ;
    EXPECT_EQ(ER_OS_ERROR, status);
}

TEST_F(SocketTestErrors, SendAfterAbortiveRelease)
{
    SetLinger(clientFd, true, 0);
    Close(clientFd);
    while ((status = Send(acceptedFd, buf, ArraySize(buf), numSent)) == ER_OK)
        ;
    EXPECT_EQ(ER_OS_ERROR, status);
}

TEST_F(SocketTestErrors, SendWouldBlock)
{
    EXPECT_EQ(ER_OK, SetBlocking(acceptedFd, false));
    while ((status = Send(acceptedFd, buf, ArraySize(buf), numSent)) == ER_OK)
        ;
    EXPECT_EQ(ER_WOULDBLOCK, status);
}

class SocketTestWithFdsErrors : public testing::Test {
  public:
    SocketFd endpoint[2];
    SocketFd clientFd;
    SocketFd acceptedFd;
    uint8_t buf[2048];

#if defined(QCC_OS_GROUP_WINDOWS)
    /*
     * The Windows implementation of SendWithFds is using MSG_OOB sends.
     * Recent Windows versions allow a maximum of 16 OOB packets to be
     * buffered on the receiver side. The connection gets reset if that
     * limit is exceeded. Therefore use a larger buffer for the ER_WOULDBLOCK
     * test on Windows, thus forcing the ER_WOULDBLOCK error to occur before
     * the OOB limit is reached.
     *
     * Note that AllJoyn product code doesn't use SendWithFds on Windows.
     */
    uint8_t wouldBlockBuf[8 * 1024];
#else
    uint8_t wouldBlockBuf[2 * 1024];
#endif

    size_t numRecvd;
    size_t numSent;
    SocketFd fds[SOCKET_MAX_FILE_DESCRIPTORS + 1];
    size_t numFds;
    QStatus status;

    virtual void SetUp()
    {
        EXPECT_EQ(ER_OK, SocketPair(endpoint));
        clientFd = endpoint[0];
        acceptedFd = endpoint[1];
        for (size_t i = 0; i < ArraySize(fds); ++i) {
            EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, fds[i]));
        }
    }

    virtual void TearDown()
    {
        for (size_t i = 0; i < ArraySize(fds); ++i) {
            Close(fds[i]);
        }
        Close(acceptedFd);
        Close(clientFd);
    }

    void Close(SocketFd& sockFd)
    {
        if (sockFd != INVALID_SOCKET_FD) {
            qcc::Close(sockFd);
            sockFd = INVALID_SOCKET_FD;
        }
    }
};

TEST_F(SocketTestWithFdsErrors, RecvWhenNotConnected)
{
    SocketFd sockFd = INVALID_SOCKET_FD;
    EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockFd));
    EXPECT_EQ(ER_OS_ERROR, RecvWithFds(sockFd, buf, 0, numRecvd, fds, 1, numFds));
    EXPECT_EQ(ER_OS_ERROR, RecvWithFds(sockFd, buf, 1, numRecvd, fds, 1, numFds));
    Close(sockFd);
}

TEST_F(SocketTestWithFdsErrors, RecvArgs)
{
    EXPECT_EQ(ER_BAD_ARG_5, RecvWithFds(acceptedFd, buf, 0, numRecvd, NULL, 0, numFds));
    EXPECT_EQ(ER_BAD_ARG_6, RecvWithFds(acceptedFd, buf, 0, numRecvd, fds, 0, numFds));
}

TEST_F(SocketTestWithFdsErrors, RecvAfterShutdown)
{
    Shutdown(clientFd, QCC_SHUTDOWN_WR);
    EXPECT_EQ(ER_OK, RecvWithFds(acceptedFd, buf, 1, numRecvd, fds, 1, numFds));
    EXPECT_EQ(0U, numRecvd);
}

TEST_F(SocketTestWithFdsErrors, RecvAfterOrderlyRelease)
{
    Close(clientFd);
    EXPECT_EQ(ER_OK, RecvWithFds(acceptedFd, buf, 1, numRecvd, fds, 1, numFds));
    EXPECT_EQ(0U, numRecvd);
}

TEST_F(SocketTestWithFdsErrors, RecvAfterAbortiveRelease)
{
    SetLinger(clientFd, true, 0);
    Close(clientFd);
    while ((status = Send(acceptedFd, buf, ArraySize(buf), numSent)) == ER_OK)
        ;
    EXPECT_EQ(ER_OS_ERROR, status);
}

TEST_F(SocketTestWithFdsErrors, RecvWouldBlock)
{
    EXPECT_EQ(ER_OK, SetBlocking(acceptedFd, false));
    EXPECT_EQ(ER_WOULDBLOCK, RecvWithFds(acceptedFd, buf, 1, numRecvd, fds, 1, numFds));
}

TEST_F(SocketTestWithFdsErrors, SendWhenConnected)
{
    EXPECT_EQ(ER_OK, SendWithFds(acceptedFd, buf, 1, numSent, fds, 1, GetPid()));
}

TEST_F(SocketTestWithFdsErrors, SendWhenNotConnected)
{
    SocketFd sockFd = INVALID_SOCKET_FD;
    EXPECT_EQ(ER_OK, Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockFd));
    EXPECT_EQ(ER_OS_ERROR, SendWithFds(sockFd, buf, 0, numSent, fds, 1, GetPid()));
    EXPECT_EQ(ER_OS_ERROR, SendWithFds(sockFd, buf, 1, numSent, fds, 1, GetPid()));
    Close(sockFd);
}

TEST_F(SocketTestWithFdsErrors, SendArgs)
{
    EXPECT_EQ(ER_BAD_ARG_5, SendWithFds(acceptedFd, buf, 1, numSent, NULL, 1, GetPid()));
    EXPECT_EQ(ER_BAD_ARG_6, SendWithFds(acceptedFd, buf, 1, numSent, fds, 0, GetPid()));
    EXPECT_EQ(ER_BAD_ARG_6, SendWithFds(acceptedFd, buf, 1, numSent, fds, SOCKET_MAX_FILE_DESCRIPTORS + 1, GetPid()));
}

TEST_F(SocketTestWithFdsErrors, SendAfterOrderlyRelease)
{
    Close(clientFd);
    while ((status = SendWithFds(acceptedFd, buf, 1, numSent, fds, 1, GetPid())) == ER_OK)
        ;
    EXPECT_EQ(ER_OS_ERROR, status);
}

TEST_F(SocketTestWithFdsErrors, SendAfterAbortiveRelease)
{
    SetLinger(clientFd, true, 0);
    Close(clientFd);
    EXPECT_EQ(ER_OS_ERROR, SendWithFds(acceptedFd, buf, 1, numSent, fds, 1, GetPid()));
}

TEST_F(SocketTestWithFdsErrors, SendWouldBlock)
{
    EXPECT_EQ(ER_OK, SetBlocking(acceptedFd, false));
    /*
     * Set this artificially low to force a blocking send.  Otherwise
     * different behavior occurs depending on the TCP window size
     * (either would block or connection reset).
     */
    EXPECT_EQ(ER_OK, SetSndBuf(acceptedFd, 8192));
    while ((status = SendWithFds(acceptedFd, wouldBlockBuf, ArraySize(wouldBlockBuf), numSent, fds, 1, GetPid())) == ER_OK)
        ;
    EXPECT_EQ(ER_WOULDBLOCK, status);
}
