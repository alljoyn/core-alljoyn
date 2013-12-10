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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <algorithm>

#if defined(QCC_OS_GROUP_POSIX)

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#if defined(QCC_OS_ANDROID) || defined(QCC_OS_LINUX)
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#endif // defined(QCC_OS_GROUP_POSIX)

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/IfConfig.h>

#include "NetworkInterface.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "NETWORK_INTERFACE"

namespace ajn {

NetworkInterface::NetworkInterface(bool enableIPV6)
    : EnableIPV6(enableIPV6)
{
    liveInterfaces.clear();
}

NetworkInterface::~NetworkInterface()
{
    liveInterfaces.clear();
}


String NetworkInterface::PrintNetworkInterfaceType(uint8_t type)
{
    String retStr = String("NONE");

    switch (type) {

    case ANY:
        retStr = String("ANY");
        break;

    case NONE:
    default:
        break;

    }

    return retStr;
}

QStatus NetworkInterface::UpdateNetworkInterfaces(void)
{
    QCC_DbgPrintf(("NetworkInterface::UpdateNetworkInterfaces()\n"));

    /* Call IfConfig to get the list of interfaces currently configured in the
       system.  This also pulls out interface flags, addresses and MTU. */
    QCC_DbgPrintf(("NetworkInterface::UpdateNetworkInterfaces(): IfConfig()\n"));
    std::vector<qcc::IfConfigEntry> entries;
    QStatus status = qcc::IfConfig(entries);

    if (status != ER_OK) {
        QCC_LogError(status, ("%s: IfConfig failed", __FUNCTION__));
        return status;
    }

    /* Filter out the unwanted entries and populate valid entries into liveInterfaces */
    for (std::vector<IfConfigEntry>::const_iterator j = entries.begin(); j != entries.end(); ++j) {

        if (((*j).m_family == QCC_AF_UNSPEC) ||
            ((!EnableIPV6) && ((*j).m_family == QCC_AF_INET6)) ||
            (((*j).m_flags & qcc::IfConfigEntry::UP) == 0) ||
            (((*j).m_flags & qcc::IfConfigEntry::LOOPBACK) != 0)) {
            continue;
        }


        liveInterfaces.push_back((*j));

        QCC_DbgPrintf(("NetworkInterface::UpdateNetworkInterfaces(): Entry %s with address %s\n", (*j).m_name.c_str(), (*j).m_addr.c_str()));
    }

    return status;
}

bool NetworkInterface::IsAnyNetworkInterfaceUp(void)
{
    if (liveInterfaces.empty()) {
        return false;
    }

    return true;
}

bool NetworkInterface::IsMultiHomed(void)
{
    /* Go through the liveInterfaces list and if we see multiple interfaces with different names
     * in the list, we are multi-homed */
    if (liveInterfaces.size() == 1) {
        return false;
    } else {
        vector<qcc::IfConfigEntry>::iterator iter;
        qcc::IfConfigEntry prev;
        bool start = true;

        for (iter = liveInterfaces.begin(); iter != liveInterfaces.end(); ++iter) {
            if (start) {
                prev = *iter;
                start = false;
                continue;
            }
            if (prev.m_name != iter->m_name) {
                return true;
            } else {
                prev = *iter;
            }
        }
    }
    return false;
}

bool NetworkInterface::IsVPN(IPAddress addr)
{
    /* We dont have support to figure out if an interface is a VPN interface. */
    return false;
}

} // namespace ajn
