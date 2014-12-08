/**
 * @file
 * NamedPipeDaemonTransport is a specialization of class Transport for communication between an AllJoyn
 * client application and the daemon over named pipe for Windows. This is the daemon's counterpart
 * to ClientTransport.
 */

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
 ******************************************************************************/
#ifndef _ALLJOYN_NAMEDPIPE_DAEMON_TRANSPORT_H
#define _ALLJOYN_NAMEDPIPE_DAEMON_TRANSPORT_H

#ifndef __cplusplus
#error Only include NamedPipeDaemonTransport.h in C++ code.
#endif

#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include "Transport.h"
#include "RemoteEndpoint.h"
#include "DaemonTransport.h"

namespace ajn {

/**
 * @brief A class for the daemon end of the client transport
 *
 * The NamedPipeDaemonTransport class has different incarnations depending on the platform.
 */
class NamedPipeDaemonTransport : public DaemonTransport {

  public:

    /**
     * Create a transport to receive incoming connections from AllJoyn application.
     *
     * @param bus  The bus associated with this transport.
     */
    NamedPipeDaemonTransport(BusAttachment& bus);

    /**
     * @internal
     * @brief Normalize a transport specification.
     *
     * Given a transport specification, convert it into a form which is guaranteed to
     * have a one-to-one relationship with a connection instance.
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   Output transport connect spec.
     * @param argMap    Parsed parameter map.
     *
     * @return ER_OK if successful.
     */
    QStatus NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const;

    /**
     * Start listening for incoming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    QStatus StartListen(const char* listenSpec);

    /**
     * @brief Stop listening for incoming connections on a specified bus address.
     *
     * This method cancels a StartListen request. Therefore, the listenSpec must
     * match previous call to StartListen().
     *
     * @param listenSpec  Transport specific key/value arguments that specify the physical interface to listen on.
     *
     * @return ER_OK if successful.
     */
    QStatus StopListen(const char* listenSpec);

    /**
     * Returns the name of this transport
     */
    const char* GetTransportName() const { return NamedPipeTransportName; }

    /**
     * Name of transport used in transport specs.
     */
    static const char* NamedPipeTransportName;

  private:
    BusAttachment& bus;                       /**< The message bus for this transport */

    /**
     * @internal
     * @brief Thread entry point.
     *
     * @param arg  Thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(void* arg);
};

} // namespace ajn

#endif // _ALLJOYN_NAMEDPIPE_DAEMON_TRANSPORT_H
