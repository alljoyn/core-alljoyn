/**
 * @file
 * Transport is an abstract base class implemented by physical
 * media interfaces such as TCP, UNIX, and Local.
 */

/******************************************************************************
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
#ifndef _ALLJOYN_TRANSPORT_H
#define _ALLJOYN_TRANSPORT_H

#ifndef __cplusplus
#error Only include Transport.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <map>
#include <vector>
#include <alljoyn/Message.h>
#include <alljoyn/TransportMask.h>
#include <alljoyn/Session.h>
#include "BusEndpoint.h"
#include <alljoyn/Status.h>

namespace ajn {


/**
 * %TransportListener is an abstract base class that provides asynchronous notifications about
 * transport related events.
 */
class TransportListener {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~TransportListener() { }

    /**
     * Called when a transport has found a bus to connect to with a set of bus names.
     *
     * @param busAddr   The address of the bus formatted as a string that can be passed to
     *                  createEndpoint
     * @param guid      GUID associated with this advertisement.
     * @param transport Transport that sent the advertisement.
     * @param names    The list of bus names that the bus has advertised or NULL if transport cannot determine list.
     * @param timer    Time to live for this set of names. (0 implies that the name is gone.)
     */
    virtual void FoundNames(const qcc::String& busAddr,
                            const qcc::String& guid,
                            TransportMask transport,
                            const std::vector<qcc::String>* names,
                            uint32_t timer) = 0;
};


/**
 * %Transport is an abstract base class implemented by physical
 * media interfaces such as TCP, UNIX, and Local.
 */
class Transport {
  public:

    /**
     * Destructor
     */
    virtual ~Transport() { }

    /**
     * Start the transport and associate it with a router.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus Start() = 0;

    /**
     * Stop the transport.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus Stop() = 0;

    /**
     * Pend the caller until the transport stops.
     *
     * @return
     *      - ER_OK if successful
     *      - an error status otherwise.
     */
    virtual QStatus Join() = 0;

    /**
     * Determine if this transport is running. Running means Start() has been called.
     *
     * @return  Returns true if the transport is running.
     */
    virtual bool IsRunning() = 0;

    /**
     * Get the transport mask for this transport
     *
     * @return the TransportMask for this transport.
     */
    virtual TransportMask GetTransportMask() const = 0;

    /**
     * Get a list of the possible listen specs of the current Transport for a
     * given set of session options.
     *
     * Session options specify high-level characteristics of session, such as
     * whether or not the underlying transport carries data encapsulated in
     * AllJoyn messages, and whether or not delivery is reliable.
     *
     * It is possible that there is more than one answer to the question: what
     * abstract address should I use when talking to another endpoint.  Each
     * Transports is equipped to understand how many answers there are and also
     * which answers are better than the others.  This method fills in the
     * provided vector with a list of currently available busAddresses ordered
     * according to which the transport thinks would be best.
     *
     * If there are no addresses appropriate to the given session options the
     * provided vector of String is left unchanged.  If there are addresses,
     * they are added at the end of the provided vector.
     *
     * @param opts Session options describing the desired characteristics of
     *             an underlying session
     * @param busAddrs A vector of String to which bus addresses corresponding
     *                 to IFF_UP interfaces matching the desired characteristics
     *                 are added.
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const {
        QCC_UNUSED(opts);
        QCC_UNUSED(busAddrs);
        return ER_FAIL;
    }

    /**
     * Does this transport support connections as described by the provided
     * session options.  This is used to distinguish up front options that are
     * fundamentally incompatible -- for example, trying to use raw sockets with
     * the UDP Transport which cannot support them.
     *
     * @param opts  Proposed session options.
     * @return
     *      - true if the SessionOpts specifies a supported option set.
     *      - false otherwise.
     */
    virtual bool SupportsOptions(const SessionOpts& opts) const {
        QCC_UNUSED(opts);
        return false;
    }

    /**
     * Normalize a transport specification.
     * Given a transport specification, convert it into a form which is guaranteed to have a one-to-one
     * relationship with a transport.
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   Output transport connect spec.
     * @param argMap    Parsed parameter map.
     * @return ER_OK if successful.
     */
    virtual QStatus NormalizeTransportSpec(const char* inSpec,
                                           qcc::String& outSpec,
                                           std::map<qcc::String, qcc::String>& argMap) const = 0;

    /**
     * Connect to a specified remote AllJoyn/DBus address.
     *
     * @param connectSpec    Transport specific key/value args used to configure the client-side endpoint.
     *                       The form of this string is &lt;transport&gt;:&lt;key1&gt;=&lt;val1&gt;,&lt;key2&gt;=&lt;val2&gt;...[;]
     * @param opts           Requested sessions opts.
     * @param newep          [OUT] Endpoint created as a result of successful connect.
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep) {
        QCC_UNUSED(connectSpec);
        QCC_UNUSED(opts);
        QCC_UNUSED(newep);
        return ER_FAIL;
    }

    /**
     * Disconnect from a specified AllJoyn/DBus address.
     *
     * This only needs to be implemented for client transports.
     *
     * @param connectSpec    The connectSpec used in Connect.
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus Disconnect(const char* connectSpec) {
        QCC_UNUSED(connectSpec);
        return ER_FAIL;
    }

    /**
     * Start listening for incoming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value args that specify the physical interface to listen on.
     *                    The form of this string is &lt;transport&gt;:&lt;key1&gt;=&lt;val1&gt;,&lt;key2&gt;=&lt;val2&gt;...[;]
     *
     * @return ER_OK if successful.
     */
    virtual QStatus StartListen(const char* listenSpec) {
        QCC_UNUSED(listenSpec);
        return ER_FAIL;
    }

    /**
     * Stop listening for incoming connections on a specified bus address.
     *
     * @param listenSpec  Transport specific key/value args that specify the physical interface to listen on.
     *                    The form of this string is &lt;transport&gt;:&lt;key1&gt;=&lt;val1&gt;,&lt;key2&gt;=&lt;val2&gt;...[;]
     *
     * @return
     *      - ER_OK if successful.
     *      - an error status otherwise.
     */
    virtual QStatus StopListen(const char* listenSpec) {
        QCC_UNUSED(listenSpec);
        return ER_FAIL;
    }

    /**
     * Set a listener for transport related events.
     * There can only be one listener set at a time. Setting a listener
     * implicitly removes any previously set listener.
     *
     * @param listener  Listener for transport related events.
     */
    virtual void SetListener(TransportListener* listener) {
        QCC_UNUSED(listener);
    }

    /**
     * Start discovering remotely advertised names that match prefix.
     *
     * @param namePrefix    Well-known name prefix.
     */
    virtual void EnableDiscovery(const char* namePrefix, TransportMask transportmask) {
        QCC_UNUSED(namePrefix);
        QCC_UNUSED(transportmask);
    }

    /**
     * Stop discovering remotely advertised names that match prefix.
     *
     * @param namePrefix    Well-known name prefix.
     *
     */
    virtual void DisableDiscovery(const char* namePrefix, TransportMask transportmask) {
        QCC_UNUSED(namePrefix);
        QCC_UNUSED(transportmask);
    }

    /**
     * Start advertising a well-known name
     *
     * @param advertiseName   Well-known name to add to list of advertised names.
     * @param quietly         Advertise the name quietly
     * @return  ER_NOT_IMPLEMENTED unless overridden by a derived class.
     */
    virtual QStatus EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports) {
        QCC_UNUSED(advertiseName);
        QCC_UNUSED(quietly);
        QCC_UNUSED(transports);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Stop advertising a well-known name with a given quality of service.
     *
     * @param advertiseName   Well-known name to remove from list of advertised names.
     */
    virtual void DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports) {
        QCC_UNUSED(advertiseName);
        QCC_UNUSED(transports);
    }

    /**
     * Returns the name of the transport
     */
    virtual const char* GetTransportName() const = 0;

    /**
     * Indicates whether this transport is used for client-to-bus or bus-to-bus connections.
     *
     * @return  true indicates this transport may be only for bus-to-bus connections
     *          false indicates this transport may only be used for client-to-bus connections.
     */
    virtual bool IsBusToBus() const = 0;

    /**
     * Helper used to parse client/server arg strings
     *
     * @param transportName  Name of transport to match in args.
     * @param args      Transport argument string of form &lt;transport&gt;:&lt;key0&gt;=&lt;val0&gt;,&lt;key1&gt;=&lt;val1&gt;[;]
     * @param argMap    [OUT] A maps or args matching the given transport name.
     * @return ER_OK if successful.
     */
    static QStatus ParseArguments(const char* transportName,
                                  const char* args,
                                  std::map<qcc::String, qcc::String>& argMap);
};

}

#endif
