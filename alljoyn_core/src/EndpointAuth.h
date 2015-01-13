/**
 * @file
 * EndpointAuth is a utility class that provides authentication
 * functions for BusEndpoint implementations.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2015, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_ENDPOINTAUTH_H
#define _ALLJOYN_ENDPOINTAUTH_H

#include <qcc/platform.h>

#include <qcc/String.h>
#include <qcc/GUID.h>
#include <qcc/Stream.h>

#include "BusInternal.h"
#include "SASLEngine.h"

namespace ajn {


/**
 * Foward declaration of class Bus
 */
class BusAttachment;


/**
 * %EndpointAuth is a utility class responsible for adding endpoint authentication
 * to BusEndpoint implementations.
 */
class EndpointAuth : public SASLEngine::ExtensionHandler {
  public:

    /**
     * Constructor
     *
     * @param bus          Bus for which authentication is done
     * @param endpoint     The endpoint being authenticated.
     * @param isAcceptor   Indicates if the endpoint is the acceptor of a connection (default is false).
     */
    EndpointAuth(BusAttachment& bus, RemoteEndpoint& endpoint, bool isAcceptor = false) :
        bus(bus),
        endpoint(endpoint),
        uniqueName(bus.GetInternal().GetRouter().GenerateUniqueName()),
        isAccepting(isAcceptor),
        remoteProtocolVersion(0),
        nameTransfer(SessionOpts::ALL_NAMES)
    { }

    /**
     * Destructor
     */
    ~EndpointAuth() { };

    /**
     * Establish a connection.
     *
     * @param authMechanisms  The authentication mechanisms to try.
     * @param authUsed        Returns the name of the authentication method that was used to establish the connection.
     * @param redirection     Returns a redirection address for the endpoint. This value is only meaninful if the
     *                        return status is ER_BUS_ENDPOINT_REDIRECTED.
     * @param listener        Authentication credentials listener
     *
     * @return
     *      - ER_OK if successful
     *      = ER_BUS_ENDPOINT_REDIRECTED if the endpoint is being redirected.
     *      - An error status otherwise
     */
    QStatus Establish(const qcc::String& authMechanisms, qcc::String& authUsed, qcc::String& redirection, AuthListener* listener = NULL);

    /**
     * Get the unique bus name assigned by the bus for this endpoint.
     *
     * @return
     *      - unique bus name
     *      - empty string if called before the endpoint has been authenticated.
     */
    const qcc::String& GetUniqueName() const { return uniqueName; }

    /**
     * Get the bus name for the peer at the remote end of this endpoint. If we are on the initiating
     * side of the connection this is the bus name of the responder and if we are the responder this
     * is the bus name of the initiator.
     *
     * @return
     *      - bus name of the remote side.
     *      - empty string if called before the endpoint has been authenticated.
     */
    const qcc::String& GetRemoteName() const { return remoteName; }

    /**
     * Get the GUID of the remote side.
     *
     * @return   The GUID for the remote side of this connection.
     *
     */
    const qcc::GUID128& GetRemoteGUID() const { return remoteGUID; }

    /**
     * Get the AllJoyn protocol version number of the remote side.
     *
     * @return   The AllJoyn protocol version of the remote side.
     *
     */
    uint32_t GetRemoteProtocolVersion() const { return remoteProtocolVersion; }

    SessionOpts::NameTransferType GetNameTransfer() const { return nameTransfer; }
  private:

    /**
     * Handle SASL extension commands during establishment.
     *
     * @param sasl    The sasl engine instance.
     * @param extCmd  The extension command string or an empty string.
     *
     * @return  A command/response string or an empty string.
     */
    qcc::String SASLCallout(SASLEngine& sasl, const qcc::String& extCmd);

    BusAttachment& bus;
    RemoteEndpoint endpoint;
    qcc::String uniqueName;          ///< Unique bus name for endpoint
    qcc::String remoteName;          ///< Bus name for the peer at other end of this endpoint

    bool isAccepting;                ///< Indicates if this is a client or server

    qcc::GUID128 remoteGUID;            ///< GUID of the remote side (when applicable)
    uint32_t remoteProtocolVersion;     ///< ALLJOYN protocol version of the remote side

    SessionOpts::NameTransferType nameTransfer;
    ProtectedAuthListener authListener;  ///< Authentication listener

    /* Internal methods */

    QStatus Hello(qcc::String& redirection);
    QStatus WaitHello(qcc::String& authUsed);
};

}

#endif
