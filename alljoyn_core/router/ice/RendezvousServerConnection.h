/**
 * @file RendezvousServerConnection.h
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

#ifndef _RENDEZVOUSSERVERCONNECTION_H
#define _RENDEZVOUSSERVERCONNECTION_H

#include <qcc/platform.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/String.h>
#include <qcc/Event.h>

#include "HttpConnection.h"
#include "NetworkInterface.h"

using namespace std;
using namespace qcc;

namespace ajn {
/*This class handles the connection with the Rendezvous Server.*/
class RendezvousServerConnection {

  public:

    /**
     * @internal
     * @brief Enum specifying the connections that need to be
     * established with the Rendezvous Server.
     */
    typedef enum _ConnectionFlag {

        /* Do not establish any connection */
        NONE = 0,

        /* Establish only the on demand connection */
        ON_DEMAND_CONNECTION,

        /* Establish only the persistent connection */
        PERSISTENT_CONNECTION,

        /* Establish both the connections */
        BOTH

    } ConnectionFlag;

    /**
     * @internal
     * @brief Constructor.
     */
    RendezvousServerConnection(String rdvzServer, bool enableIPv6, bool useHttp, String rootCert, String caCert);

    /**
     * @internal
     * @brief Destructor.
     */
    ~RendezvousServerConnection();

    /**
     * @internal
     * @brief Connect to the Rendezvous Server after gathering the
     * latest interface details.
     *
     * @param connFlag - Flag specifying the type of connection to be set up
     * @param interfaceFlags - Interface flags specifying the permissible interface types for the connection
     *
     * @return ER_OK or ER_FAIL
     */
    QStatus Connect(uint8_t interfaceFlags, ConnectionFlag connFlag);

    /**
     * @internal
     * @brief Disconnect from the Rendezvous Server
     *
     * @return None
     */
    void Disconnect(void);

    /**
     * @internal
     * @brief Returns if the interface with the specified IPAddress is still live
     *
     * @return None
     */
    bool IsInterfaceLive(IPAddress interfaceAddr);

    /**
     * @internal
     * @brief Update the connection details
     *
     * @return None
     */
    void UpdateConnectionDetails(HttpConnection** oldHttpConn, HttpConnection* newHttpConn,
                                 bool* isConnected, bool* connectionChangedFlag);

    /**
     * @internal
     * @brief Clean up an HTTP connection
     *
     * @return None
     */
    void CleanConnection(HttpConnection* httpConn, bool* isConnected);

    /**
     * @internal
     * @brief Set up a HTTP connection
     *
     * @return ER_OK or ER_FAIL
     */
    QStatus SetupNewConnection(SocketFd& sockFd, HttpConnection** httpConn);

    /**
     * @internal
     * @brief Set up a HTTP connection with the Rendezvous Server
     *
     * @return ER_OK or ER_FAIL
     */
    QStatus SetupHTTPConn(SocketFd sockFd, HttpConnection** httpConn);

    /**
     * @internal
     * @brief Set up a socket for HTTP connection with the Rendezvous Server
     *
     * @return ER_OK or ER_FAIL
     */
    QStatus SetupSockForConn(SocketFd& sockFd);

    /**
     * @internal
     * @brief Set up a HTTP connection with the Rendezvous Server
     *
     * @param connFlag - Flag specifying the type of connection to be set up
     *
     * @return ER_OK or ER_FAIL
     */
    QStatus SetupConnection(ConnectionFlag connFlag);

    /**
     * @internal
     * @brief Function indicating if the on demand connection is up with the
     * Rendezvous Server.
     */
    bool IsOnDemandConnUp() { return onDemandIsConnected; };

    /**
     * @internal
     * @brief Function indicating if the persistent connection is up with the
     * Rendezvous Server.
     */
    bool IsPersistentConnUp() { return persistentIsConnected; };

    /**
     * @internal
     * @brief Function indicating if either or both the persistent and on demand connections are up with the
     * Rendezvous Server.
     */
    bool IsConnectedToServer() { return (onDemandIsConnected | persistentIsConnected); };

    /**
     * @internal
     * @brief Send a message to the Server
     */
    QStatus SendMessage(bool sendOverPersistentConn, HttpConnection::Method httpMethod, String uri, bool payloadPresent, String payload);

    /**
     * @internal
     * @brief Receive a response from the Server
     */
    QStatus FetchResponse(bool isOnDemandConnection, HttpConnection::HTTPResponse& response);

    /**
     * @internal
     * @brief Reset the persistentConnectionChanged flag
     */
    void ResetPersistentConnectionChanged(void) { persistentConnectionChanged = false; };

    /**
     * @internal
     * @brief Reset the onDemandConnectionChanged flag
     */
    void ResetOnDemandConnectionChanged(void) { onDemandConnectionChanged = false; };

    /**
     * @internal
     * @brief Return the value of persistentConnectionChanged flag
     */
    bool GetPersistentConnectionChanged(void) { return persistentConnectionChanged; };

    /**
     * @internal
     * @brief Return the value of onDemandConnectionChanged flag
     */
    bool GetOnDemandConnectionChanged(void) { return onDemandConnectionChanged; };

    /**
     * @internal
     * @brief Return onDemandConn
     */
    Event& GetOnDemandSourceEvent() { return onDemandConn->GetResponseSource().GetSourceEvent(); };

    /**
     * @internal
     * @brief Return persistentConn
     */
    Event& GetPersistentSourceEvent() { return persistentConn->GetResponseSource().GetSourceEvent(); };

    /**
     * @internal
     * @brief Return IPAddresses of the interfaces
     * over which the Persistent and the On Demand connections have
     * been setup with the Rendezvous Server.
     */
    void GetRendezvousConnIPAddresses(IPAddress& onDemandAddress, IPAddress& persistentAddress);

    /**
     * @internal
     * @brief Return the IP address of the Rendezvous Server.
     */
    void GetRendezvousServerIPAddress(qcc::String& address) {
        address = RendezvousServerIPAddress;
    }

    /**
     * @internal
     * @brief Set the IP address of the Rendezvous Server.
     */
    void SetRendezvousServerIPAddress(qcc::String& address) {
        RendezvousServerIPAddress = address;
    }

    /**
     * @internal
     * @brief Returns true if the device has valid interfaces up for connection to the Server.
     */
    bool IsAnyNetworkInterfaceUp(void) {
        if (networkInterface) {
            return networkInterface->IsAnyNetworkInterfaceUp();
        } else {
            return false;
        }
    }

  private:

    /* Default constructor */
    RendezvousServerConnection();

    /* Just defined to make klocwork happy. Should never be used */
    RendezvousServerConnection(const RendezvousServerConnection& other) {
        assert(false);
    }

    /* Just defined to make klocwork happy. Should never be used */
    RendezvousServerConnection& operator=(const RendezvousServerConnection& other) {
        assert(false);
        return *this;
    }

    /**
     * @internal
     * @brief Boolean indicating if the on demand connection is up.
     */
    bool onDemandIsConnected;

    /**
     * @internal
     * @brief The HTTP connection that is used to send messages to the
     * Rendezvous Server.
     */
    HttpConnection* onDemandConn;

    /**
     * @internal
     * @brief Boolean indicating if the persistent connection is up.
     */
    bool persistentIsConnected;

    /**
     * @internal
     * @brief Boolean indicating if the persistent connection has changed from
     * what it was previously.
     */
    bool persistentConnectionChanged;

    /**
     * @internal
     * @brief Boolean indicating if the On Demand connection has changed from
     * what it was previously.
     */
    bool onDemandConnectionChanged;

    /**
     * @internal
     * @brief The HTTP connection that is used to send GET messages to the
     * Rendezvous Server and receive responses from the same.
     */
    HttpConnection* persistentConn;

    /**
     * @internal
     * @brief Interface object used to get the network information from the kernel.
     */
    NetworkInterface* networkInterface;

    /**
     * @internal
     * @brief Rendezvous Server host name.
     */
    String RendezvousServer;

    /**
     * @internal
     * @brief Rendezvous Server IP address.
     */
    String RendezvousServerIPAddress;

    /**
     * @internal
     * @brief Boolean indicating if IPv6 addressing mode is supported.
     */
    bool EnableIPv6;

    /**
     * @internal
     * @brief Boolean indicating if HTTP protocol needs to be used for connection.
     */
    bool UseHTTP;

    /**
     * @internal
     * @brief Server root certificate if HTTPS protocol is used.
     */
    String RendezvousServerRootCertificate;

    /**
     * @internal
     * @brief Server CA certificate if HTTPS protocol is used.
     */
    String RendezvousServerCACertificate;

};

} //namespace ajn


#endif //_RENDEZVOUSSERVERCONNECTION_H
