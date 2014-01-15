/**
 * @file NetworkNetworkInterface.h
 *
 * This file defines a class that are used to perform some network interface related operations
 * that are required by the ICE transport.
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

#ifndef _NETWORKINTERFACE_H
#define _NETWORKINTERFACE_H

#ifndef __cplusplus
#error Only include NetworkInterface.h in C++ code.
#endif

#include <vector>
#include <list>

#include <qcc/String.h>
#include <qcc/IfConfig.h>
#include <qcc/IPAddress.h>

#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;

namespace ajn {

class NetworkInterface {

  public:
    /**
     * @internal
     * @brief Bit masks used to specify the interface type.
     */
    /* None */
    static const uint8_t NONE = 0x00;

    /* Any of the available interface types */
    static const uint8_t ANY = 0xFF;

    /**
     * @brief List of available live Ethernet interfaces
     */
    std::vector<qcc::IfConfigEntry> liveInterfaces;

    /**
     * @brief Flag used to indicate if interfaces with IPV6 addresses are to be
     * used
     */
    bool EnableIPV6;

    /**
     * @internal
     *
     * @brief Default Constructor.
     */
    NetworkInterface() :
        EnableIPV6(false) { }

    /**
     * @internal
     *
     * @brief Constructor.
     *
     * @param enableIPV6 - Flag used to indicate if interfaces with IPV6 addresses
     *                     are to be used
     */
    NetworkInterface(bool enableIPV6);

    /**
     * @internal
     *
     * @brief Destructor.
     *
     */
    ~NetworkInterface();

    /**
     * @internal
     * @brief Utility function to print the interface type.
     */
    String PrintNetworkInterfaceType(uint8_t type);

    /**
     * @internal
     * @brief Update the interfaces to get a list of the current live interfaces
     */
    QStatus UpdateNetworkInterfaces(void);

    /**
     * @internal
     * @brief Function used to find if any live network interfaces are available
     */
    bool IsAnyNetworkInterfaceUp(void);

    /**
     * @internal
     * @brief Function used to find if the device is multi-homed
     */
    bool IsMultiHomed(void);

    /**
     * @internal
     * @brief Function used to find if interface with the IPAddress addr is a VPN interface
     */
    bool IsVPN(IPAddress addr);
};

} // namespace ajn

#endif //_NETWORKINTERFACE_H
