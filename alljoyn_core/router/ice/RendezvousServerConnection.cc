/**
 * @file RendezvousServerConnection.cc
 *
 * This file defines a class that handles the connection with the Rendezvous Server.
 *
 */

/******************************************************************************
 * Copyright (c) 2012 AllSeen Alliance. All rights reserved.
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
#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/time.h>
#include <qcc/Stream.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/StringSource.h>
#include <qcc/StringSink.h>
#include <qcc/BufferedSource.h>
#include <qcc/Crypto.h>

#if defined(QCC_OS_GROUP_POSIX)
#include <errno.h>
#endif

#include "RendezvousServerConnection.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "RENDEZVOUS_SERVER_CONNECTION"

namespace ajn {

RendezvousServerConnection::RendezvousServerConnection(String rdvzServer, bool enableIPv6, bool useHttp, String rootCert, String caCert) :
    onDemandIsConnected(false),
    onDemandConn(NULL),
    persistentIsConnected(false),
    persistentConnectionChanged(false),
    onDemandConnectionChanged(false),
    persistentConn(NULL),
    networkInterface(NULL),
    RendezvousServer(rdvzServer),
    RendezvousServerIPAddress(),
    EnableIPv6(enableIPv6),
    UseHTTP(useHttp),
    RendezvousServerRootCertificate(rootCert),
    RendezvousServerCACertificate(caCert)
{
    QCC_DbgPrintf(("RendezvousServerConnection::RendezvousServerConnection()\n"));

    /* Instantiate a NetworkInterface object */
    networkInterface = new NetworkInterface(EnableIPv6);
}

RendezvousServerConnection::~RendezvousServerConnection()
{
    QCC_DbgPrintf(("RendezvousServerConnection::~RendezvousServerConnection()\n"));

    /* Disconnect any existing connections */
    Disconnect();

    /* Clear the networkInterface */
    if (networkInterface) {
        delete networkInterface;
        networkInterface = NULL;
    }

    RendezvousServerIPAddress.clear();
}

QStatus RendezvousServerConnection::Connect(uint8_t interfaceFlags, ConnectionFlag connFlag)
{
    QStatus status = ER_OK;

    /* Return ER_FAIL if the interface flags have been specified to be NONE. We would normally not
     * hit this condition as the Discovery Manager would check that the flags are not NONE before
     * calling this function */
    if (interfaceFlags == NetworkInterface::NONE) {
        status = ER_FAIL;
        QCC_LogError(status, ("RendezvousServerConnection::Connect(): interfaceFlage = NONE"));
        return status;
    }

    /* Return ER_FAIL if the connection flag has been specified to be NONE. We would normally not
     * hit this condition as the Discovery Manager would ensure that the flag is not NONE before
     * calling this function */
    if (connFlag == NONE) {
        status = ER_FAIL;
        QCC_LogError(status, ("RendezvousServerConnection::Connect(): connFlag = NONE"));
        return status;
    }

    /* Update the interfaces */
    status = networkInterface->UpdateNetworkInterfaces();
    if (status != ER_OK) {
        QCC_LogError(status, ("%s: networkInterface->UpdateNetworkInterfaces() failed", __FUNCTION__));
        return status;
    }

    /* Ensure that live interfaces are available before proceeding further */
    if (!networkInterface->IsAnyNetworkInterfaceUp()) {
        status = ER_FAIL;
        QCC_LogError(status, ("RendezvousServerConnection::Connect(): None of the interfaces are up\n"));
        return status;
    }

    QCC_DbgPrintf(("RendezvousServerConnection::Connect(): IsPersistentConnUp() = %d IsOnDemandConnUp() = %d",
                   IsPersistentConnUp(), IsOnDemandConnUp()));

    /* Reconfigure or Set up the requested connections */
    if (connFlag == BOTH) {
        status = SetupConnection(ON_DEMAND_CONNECTION);

        if (status != ER_OK) {
            QCC_LogError(status, ("RendezvousServerConnection::Connect(): Unable to setup the on demand connection with the Rendezvous Server\n"));
            return status;
        } else {
            status = SetupConnection(PERSISTENT_CONNECTION);

            if (status != ER_OK) {
                QCC_LogError(status, ("RendezvousServerConnection::Connect(): Unable to setup the persistent connection with the Rendezvous Server\n"));

                /* Disconnect the on demand connection that we just set up */
                Disconnect();

                return status;
            }
        }

    } else if (connFlag == ON_DEMAND_CONNECTION) {

        status = SetupConnection(ON_DEMAND_CONNECTION);

        if (status != ER_OK) {
            QCC_LogError(status, ("RendezvousServerConnection::Connect(): Unable to setup the on demand connection with the Rendezvous Server\n"));

            /* Disconnect the persistent connection if it is up */
            Disconnect();

            return status;
        }

    } else if (connFlag == PERSISTENT_CONNECTION) {

        status = SetupConnection(PERSISTENT_CONNECTION);

        if (status != ER_OK) {
            QCC_LogError(status, ("RendezvousServerConnection::Connect(): Unable to setup the persistent connection with the Rendezvous Server\n"));

            /* Disconnect the persistent connection if it is up */
            Disconnect();

            return status;
        }
    }

    return status;
}

QStatus RendezvousServerConnection::SetupConnection(ConnectionFlag connFlag)
{
    QStatus status = ER_OK;

    if ((connFlag != PERSISTENT_CONNECTION) && (connFlag != ON_DEMAND_CONNECTION)) {
        status = ER_UNABLE_TO_CONNECT_TO_RENDEZVOUS_SERVER;
        QCC_LogError(status, ("%s: Invalid connection flag 0x%x specified", connFlag));
        return status;
    }

    HttpConnection** httpConn = &persistentConn;
    bool* isConnected = &persistentIsConnected;
    bool* connectionChanged = &persistentConnectionChanged;
    String connType = String("Persistent Connection");

    if (connFlag == ON_DEMAND_CONNECTION) {
        httpConn = &onDemandConn;
        isConnected = &onDemandIsConnected;
        connectionChanged = &onDemandConnectionChanged;
        connType = String("On Demand Connection");
    }

    SocketFd sockFd = -1;
    HttpConnection* newHttpConn = NULL;

    if (*isConnected) {
        if (*httpConn) {
            if (IsInterfaceLive((*httpConn)->GetLocalInterfaceAddress())) {
                QCC_DbgPrintf(("RendezvousServerConnection::SetupConnection(): Keeping the current connection with the Rendezvous Server"));
                return ER_OK;
            }
        }
    }

    /* Set up a new connection with the Rendezvous Server */
    status = SetupNewConnection(sockFd, &newHttpConn);

    if (status == ER_OK) {
        /* Tear down the old connection if we were already connected */
        if (*isConnected) {

            /* We do not check the return status here because we have already successfully set up a new
             * connection. Its ok if some cleanup has failed */
            CleanConnection(*httpConn, isConnected);

        }

        /* Update the connection details in the status variables */
        UpdateConnectionDetails(httpConn, newHttpConn, isConnected, connectionChanged);

        QCC_DbgPrintf(("RendezvousServerConnection::SetupConnection(): Successfully set up a connection with the Rendezvous Server"));
    } else {
        QCC_LogError(status, ("RendezvousServerConnection::SetupConnection(): Unable to setup a connection with the Rendezvous Server"));
    }

    return status;
}

void RendezvousServerConnection::Disconnect(void)
{
    /* Clean up the persistent connection */
    if (IsPersistentConnUp()) {
        CleanConnection(persistentConn, &persistentIsConnected);
    }

    /* Clean up the on demand connection */
    if (IsOnDemandConnUp()) {
        CleanConnection(onDemandConn, &onDemandIsConnected);
    }
}

bool RendezvousServerConnection::IsInterfaceLive(IPAddress interfaceAddr)
{
    QCC_DbgPrintf(("RendezvousServerConnection::IsInterfaceLive()"));

    bool isAlive = false;
    uint32_t index;

    if (networkInterface->IsAnyNetworkInterfaceUp()) {
        for (index = 0; index < networkInterface->liveInterfaces.size(); ++index) {
            if (interfaceAddr.ToString() == networkInterface->liveInterfaces[index].m_addr) {
                isAlive = true;
                break;
            }
        }
    }


    return isAlive;
}

void RendezvousServerConnection::UpdateConnectionDetails(HttpConnection** oldHttpConn, HttpConnection* newHttpConn,
                                                         bool* isConnected, bool* connectionChangedFlag)
{
    QCC_DbgPrintf(("RendezvousServerConnection::UpdateConnectionDetails(): oldHttpConn(0x%x) newHttpConn(0x%x)", oldHttpConn, newHttpConn));

    /* Update the HTTP connection details */
    *oldHttpConn = newHttpConn;

    /* Set the isConnected flag to true */
    *isConnected = true;

    /* Set the connectionChangedFlag flag to true */
    *connectionChangedFlag = true;
}

void RendezvousServerConnection::CleanConnection(HttpConnection* httpConn, bool* isConnected)
{
    QCC_DbgPrintf(("RendezvousServerConnection::CleanConnection()"));

    /* Tear down the old HTTP connection */
    if (httpConn) {
        httpConn->Clear();
        delete httpConn;
        httpConn = NULL;
    }

    *isConnected = false;
}

QStatus RendezvousServerConnection::SetupNewConnection(SocketFd& sockFd, HttpConnection** httpConn)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("RendezvousServerConnection::SetupNewConnection()"));

    if (UseHTTP) {
        status = SetupSockForConn(sockFd);

        if (status != ER_OK) {
            QCC_LogError(status, ("RendezvousServerConnection::SetupNewConnection(): Unable to setup a socket for connection with the Server"));

            return status;
        }
    }

    /* Set up a new HTTP connection using the socket */
    status = SetupHTTPConn(sockFd, httpConn);

    if (status != ER_OK) {
        QCC_LogError(status, ("RendezvousServerConnection::SetupNewConnection(): Unable to setup a HTTP connection with the Server"));

        /* Close the allocated socket */
        if (sockFd != -1) {
            Close(sockFd);
        }

        return status;
    } else {
        QCC_DbgPrintf(("RendezvousServerConnection::SetupNewConnection(): Successfully set up a connection. httpConn(0x%x) sockFd(0x%x)", *httpConn, sockFd));
    }

    return status;
}

QStatus RendezvousServerConnection::SetupHTTPConn(SocketFd sockFd, HttpConnection** httpConn)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("RendezvousServerConnection::SetupHTTPConn(): sockFd = %d", sockFd));

    *httpConn = new HttpConnection();

    if (!(*httpConn)) {
        status = ER_FAIL;
        QCC_LogError(status, ("RendezvousServerConnection::SetupHTTPConn(): Unable to setup a HTTP connection with the Server"));
        return status;
    }

    (*httpConn)->SetHost(RendezvousServer);

    String serverIPAddress;

    /* If we have a valid value in RendezvousServerIPAddress, use it for connection or else
     * use the Server host name for connection */
    if (!RendezvousServerIPAddress.empty()) {
        QCC_DbgPrintf(("%s: Using stored RDVZ Server IP address %s", __FUNCTION__, RendezvousServerIPAddress.c_str()));
        status = (*httpConn)->SetHostIPAddress(RendezvousServerIPAddress);
    } else {
        status = (*httpConn)->SetHostIPAddress(RendezvousServer);
        if (status == ER_OK) {
            /* We have done a DNS lookup on the Server name. Store off the resolved IP address
             * of the Server */
            (*httpConn)->GetHostIPAddress(serverIPAddress);
            QCC_DbgPrintf(("%s: Retrieved resolved RDVZ Server IP address %s", __FUNCTION__, serverIPAddress.c_str()));
        }
    }

    if (status == ER_OK) {
        if (!UseHTTP) {
            (*httpConn)->SetProtocol(HttpConnection::PROTO_HTTPS);
            (*httpConn)->SetServerCertificates(RendezvousServerRootCertificate, RendezvousServerCACertificate);
        }

        status = (*httpConn)->Connect(sockFd);

        if (status == ER_OK) {
            /* Store off the resolved RDVZ server IP address in RendezvousServerIPAddress */
            if (!serverIPAddress.empty()) {
                RendezvousServerIPAddress = serverIPAddress;
                QCC_DbgPrintf(("%s: Set RendezvousServerIPAddress to %s", __FUNCTION__, RendezvousServerIPAddress.c_str()));
            }

            QCC_DbgPrintf(("RendezvousServerConnection::SetupHTTPConn(): Connected to Rendezvous Server. *httpConn(0x%x)\n", *httpConn));
        }
    }

    if (status != ER_OK) {
        (*httpConn)->Clear();
        delete (*httpConn);
        *httpConn = NULL;
        /* If we failed the connection that we attempted with the cached address in RendezvousServerIPAddress,
         * clear it */
        if (!RendezvousServerIPAddress.empty()) {
            RendezvousServerIPAddress.clear();
        }
        QCC_LogError(status, ("RendezvousServerConnection::SetupHTTPConn(): Unable to connect to the Rendezvous Server"));
    }

    return status;
}

QStatus RendezvousServerConnection::SetupSockForConn(SocketFd& sockFd)
{
    QStatus status = ER_FAIL;

    QCC_DbgPrintf(("RendezvousServerConnection::SetupSockForConn()"));

    AddressFamily socketFamily = QCC_AF_INET;

    /* If the IPv6 support is enabled, set the socket family to QCC_AF_UNSPEC so that the OS is free to choose the interface
     * of any protocol family of its choice */
    // PPN - May need to change this
    if (EnableIPv6) {
        socketFamily = QCC_AF_UNSPEC;
    }

    status = Socket(socketFamily, QCC_SOCK_STREAM, sockFd);

    if (status != ER_OK) {
        QCC_LogError(status, ("RendezvousServerConnection::SetupSockForConn(): Socket() failed: %d - %s", errno, strerror(errno)));
    } else {
        QCC_DbgPrintf(("RendezvousServerConnection::SetupSockForConn(): Set up a socket %d\n", sockFd));
    }

    return status;
}

QStatus RendezvousServerConnection::SendMessage(bool sendOverPersistentConn, HttpConnection::Method httpMethod, String uri, bool payloadPresent, String payload)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("RendezvousServerConnection::SendMessage(): uri = %s payloadPresent = %d sendOverPersistentConn = %d", uri.c_str(), payloadPresent, sendOverPersistentConn));

    HttpConnection* connection = onDemandConn;

    /* Send the message over the persistent connection if sendOverPersistentConn is true or else send it over the On Demand connection */
    if (sendOverPersistentConn) {
        connection = persistentConn;
        QCC_DbgPrintf(("RendezvousServerConnection::SendMessage(): Sending message with Uri %s over Persistent connection 0x%x", uri.c_str(), persistentConn));
        /* If the Persistent connection is not up return */
        if (!IsPersistentConnUp()) {
            status = ER_FAIL;
            QCC_LogError(status, ("RendezvousServerConnection::SendMessage(): The Persistent connection is not up"));
        }
    } else {
        QCC_DbgPrintf(("RendezvousServerConnection::SendMessage(): Sending message with Uri %s over On Demand connection", uri.c_str()));
        /* If the on demand connection is not up return */
        if (!IsOnDemandConnUp()) {
            status = ER_FAIL;
            QCC_LogError(status, ("RendezvousServerConnection::SendMessage(): The On Demand connection is not up"));
        }
    }

    if (status == ER_OK) {

        /* Setup the connection */
        connection->Clear();
        connection->SetRequestHeader("Host", RendezvousServer);
        connection->SetMethod(httpMethod);
        connection->SetUrlPath(uri);
        if (payloadPresent) {
            connection->AddApplicationJsonField(payload);
        }

        /* Send the message */
        status = connection->Send();

        if (status == ER_OK) {
            QCC_DbgPrintf(("RendezvousServerConnection::SendMessage(): Sent the message to the Rendezvous Server successfully"));
        } else {
            QCC_LogError(status, ("RendezvousServerConnection::SendMessage(): Unable to send the message to the Rendezvous Server successfully"));
        }
    }

    return status;
}

QStatus RendezvousServerConnection::FetchResponse(bool isOnDemandConnection, HttpConnection::HTTPResponse& response)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("RendezvousServerConnection::FetchResponse(): isOnDemandConnection = %d", isOnDemandConnection));

    HttpConnection* connection = onDemandConn;

    if (isOnDemandConnection) {
        QCC_DbgPrintf(("RendezvousServerConnection::FetchResponse(): Receiving response over On Demand connection"));
        /* If the on demand connection is not up, return */
        if (!IsOnDemandConnUp()) {
            status = ER_FAIL;
            QCC_LogError(status, ("RendezvousServerConnection::FetchResponse(): The On Demand connection is not up"));
        }
    } else {
        connection = persistentConn;
        QCC_DbgPrintf(("RendezvousServerConnection::FetchResponse(): Receiving response over Persistent connection"));
        /* If the Persistent connection is not up, return */
        if (!IsPersistentConnUp()) {
            status = ER_FAIL;
            QCC_LogError(status, ("RendezvousServerConnection::FetchResponse(): The Persistent connection is not up"));
        }
    }

    if (status == ER_OK) {

        if (connection) {
            /* Send the message */
            status = connection->ParseResponse(response);

            if (status == ER_OK) {
                QCC_DbgPrintf(("RendezvousServerConnection::FetchResponse(): Parsed the response successfully"));
            } else {
                QCC_LogError(status, ("RendezvousServerConnection::FetchResponse(): Unable to parse the response successfully"));
                if (status == ER_OS_ERROR) {
                    QCC_LogError(status, ("OS_ERROR: %s", qcc::GetLastErrorString().c_str()));
                }
            }
        }

    }

    return status;
}

void RendezvousServerConnection::GetRendezvousConnIPAddresses(IPAddress& onDemandAddress, IPAddress& persistentAddress)
{
    QCC_DbgPrintf(("RendezvousServerConnection::GetRendezvousConnIPAddresses()"));

    if (IsConnectedToServer()) {
        QCC_DbgPrintf(("RendezvousServerConnection::GetRendezvousConnIPAddresses(): Connected to the Server"));

        if (IsOnDemandConnUp()) {
            QCC_DbgPrintf(("RendezvousServerConnection::GetRendezvousConnIPAddresses(): IsOnDemandConnUp()"));
            onDemandAddress = onDemandConn->GetLocalInterfaceAddress();
        }

        if (IsPersistentConnUp()) {
            QCC_DbgPrintf(("RendezvousServerConnection::GetRendezvousConnIPAddresses(): IsPersistentConnUp()"));
            persistentAddress = persistentConn->GetLocalInterfaceAddress();
        }

    } else {
        QCC_DbgPrintf(("RendezvousServerConnection::GetRendezvousConnIPAddresses(): Not connected to the Server"));
    }
}

} // namespace ajn
