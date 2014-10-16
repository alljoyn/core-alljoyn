/**
 * @file
 * Bus is the top-level object responsible for implementing the message bus.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BUS_H
#define _ALLJOYN_BUS_H

#ifndef __cplusplus
#error Only include Bus.h in C++ code.
#endif

#include <qcc/platform.h>

#include <list>
#include <set>

#include <qcc/Mutex.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>

#include <alljoyn/Status.h>

#include "TransportList.h"
#include "DaemonRouter.h"
#include "BusInternal.h"

namespace ajn {

class Bus : public BusAttachment, public NameListener {
  public:
    /**
     * Construct a Bus.
     *
     * @param applicationName   Name of the application.
     *
     * @param factories         A container of transport factories that knows
     *                          how to create the individual transports.  Some
     *                          transports may be defined as default, in which
     *                          case they are automatically instantiated.  If
     *                          not, they are instantiated based on the listen
     *                          spec if present.
     *
     * @param listenSpecs       A semicolon separated list of bus addresses that this daemon is capable of listening on.
     */
    Bus(const char* applicationName, TransportFactoryContainer& factories, const char* listenSpecs = NULL);

    /**
     * Destruct a Bus.
     */
    ~Bus();

    /**
     * Listen for incoming AllJoyn connections on a given transport address.
     *
     * This method will fail if the BusAttachment instance is a strictly for client-side use.
     *
     * @param listenSpecs     A list of transport connection spec strings each of the form:
     *                        @c \<transport\>:\<param1\>=\<value1\>,\<param2\>=\<value2\>...
     *
     * @return
     *      - ER_OK if successful
     *      - ER_BUS_NO_TRANSPORTS if transport can not be found to listen for.
     */
    QStatus StartListen(const char* listenSpecs);

    /**
     * Stop listening for incomming AllJoyn connections on a given transport address.
     *
     * @param listenSpecs  A transport connection spec string of the form:
     *                     @c \<transport\>:\<param1\>=\<value1\>,\<param2\>=\<value2\>...[;]
     *
     * @return ER_OK
     */
    QStatus StopListen(const char* listenSpecs);

    /**
     * Get addresses that can be used by applications running on the same
     * machine (i.e., unix: and tcp:).
     *
     * @return  Local bus addresses in standard DBus address notation
     */
    const qcc::String& GetLocalAddresses() const { return localAddrs; }


    /**
     * Get addresses that can be used by applications running on other
     * machines (i.e., tcp:).
     *
     * @return  External bus addresses in standard DBus address notation
     */
    const qcc::String& GetExternalAddresses() const { return externalAddrs; }


    /**
     * Get all unique names and their exportable alias (well-known) names.
     *
     * @param  nameVec   Vector of (uniqueName, aliases) pairs where aliases is a vector of alias names.
     */
    void GetUniqueNamesAndAliases(std::vector<std::pair<qcc::String, std::vector<qcc::String> > >& nameVec) const
    {
        reinterpret_cast<const DaemonRouter&>(GetInternal().GetRouter()).GetUniqueNamesAndAliases(nameVec);
    }

    /**
     * Register an object that will receive bus event notifications.
     *
     * @param listener  Object instance that will receive bus event notifications.
     */
    void RegisterBusListener(BusListener& listener);

    /**
     * Unregister an object that was previously registered as a BusListener
     *
     * @param listener  BusListener to be un registered.
     */
    void UnregisterBusListener(BusListener& listener);

  private:

    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                          const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);

    /**
     * Listen for incomming AllJoyn connections on a single transport address.
     *
     * @param listenSpec      A transport connection spec string of the form:
     *                        @c \<transport\>:\<param1\>=\<value1\>,\<param2\>=\<value2\>...[;]
     *
     * @param listening       [OUT] true if started listening on specified transport.
     *
     * @return
     *      - ER_OK if successful
     *      - ER_BUS_NO_TRANSPORTS if transport can not be found to listen for.
     */
    QStatus StartListen(const qcc::String& listenSpec, bool& listening);

    qcc::String localAddrs;      ///< Bus Addresses locally accessable
    qcc::String externalAddrs;   ///< Bus Addresses externall accessable

    typedef qcc::ManagedObj<BusListener*> ProtectedBusListener;
    std::set<ProtectedBusListener> busListeners;
    qcc::Mutex listenersLock;
};

}
#endif
