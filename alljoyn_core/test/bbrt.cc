/**
 * @file
 *
 * This file tests AllJoyn use of the DBus wire protocol
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


#include "qcc/platform.h"
#include "qcc/Util.h"
#include "qcc/Debug.h"
#include <alljoyn/Status.h>
#include "qcc/string.h"
#include "qcc/Thread.h"
#include "qcc/Environ.h"
#include "qcc/Socket.h"
#include "qcc/SocketStream.h"
#include "qcc/GUID.h"
#include "alljoyn/AuthMechanism.h"
#include "alljoyn/AuthMechDBusCookieSHA1.h"
#include "alljoyn/AuthMechAnonymous.h"
#include "alljoyn/EndpointAuth.h"
#include "alljoyn/Message.h"
#include "alljoyn/version.h"
#include "alljoyn/Bus.h"

using namespace qcc;
using namespace std;
using namespace ajn;

static const char SockName[] = "@alljoyn";



ThreadReturn STDCALL ServerThread(void* arg)
{
    SocketFd sockfd;
    QStatus status;
    qcc::GUID128 serverGUID;
    Bus bus(true);
    QCC_SyncPrintf("Starting server thread\n");

    status = Socket(QCC_AF_UNIX, QCC_SOCK_STREAM, sockfd);
    if (status == ER_OK) {
        status = Bind(sockfd, SockName);
        if (status == ER_OK) {
            status = Listen(sockfd, 0);
            if (status == ER_OK) {
                SocketFd newSockfd;
                status = Accept(sockfd, newSockfd);
                if (status == ER_OK) {
                    string authName;
                    SocketStream sockStream(newSockfd);
                    EndpointAuth endpoint(bus, sockStream, serverGUID, "test");
                    status = endpoint.Establish(authName, 5);
                }
            }
        }
    }
    QCC_SyncPrintf("Server thread %s\n", QCC_StatusText(status));
    return (ThreadReturn) 0;
}


ThreadReturn STDCALL ClientThread(void* arg)
{
    QStatus status;
    string authName;
    Bus bus(false);

    QCC_SyncPrintf("Starting client thread\n");
    Stream* stream = reinterpret_cast<Stream*>(arg);
    EndpointAuth endpoint(bus, *stream);
    status = endpoint.Establish(authName, 5);
    if (status == ER_OK) {
        QCC_SyncPrintf("Established connection using %s\n", authName.c_str());
    }
    QCC_SyncPrintf("Leaving client thread %s\n", QCC_StatusText(status));
    return (ThreadReturn) 0;
}


int main(int argc, char** argv)
{
    // Environ *env = Environ::GetAppEnviron();
    SocketStream sockStream(QCC_AF_UNIX, QCC_SOCK_STREAM);
    QStatus status;

    printf("AllJoyn Library version: %s\n", alljoyn::GetVersion());
    printf("AllJoyn Library build info: %s\n", alljoyn::GetBuildInfo());

    /*
     * Register the authentication methods
     */
    AuthManager::RegisterMechanism(AuthMechDBusCookieSHA1::Instantiator, AuthMechDBusCookieSHA1::AuthName());
    AuthManager::RegisterMechanism(AuthMechAnonymous::Instantiator, AuthMechAnonymous::AuthName());

    Thread srvThread("server", ServerThread);
    srvThread.Start(NULL);


    string busAddr = SockName;
    status = sockStream.Connect(busAddr);
    if (status != ER_OK) {
        QCC_SyncPrintf("Error: failed to connect socket %s", QCC_StatusText(status));
        return -1;
    }
    QCC_SyncPrintf("Connected to %s\n", busAddr.c_str());

    ClientThread(&sockStream);

    return 0;
}
