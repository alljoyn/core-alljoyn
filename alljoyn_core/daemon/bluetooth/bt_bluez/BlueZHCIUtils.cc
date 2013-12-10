/**
 * @file
 * Utility functions for tweaking Bluetooth behavior via BlueZ.
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

#include <qcc/platform.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <qcc/Socket.h>
#include <qcc/time.h>

#include "BlueZ.h"
#include "BlueZHCIUtils.h"
#include "BTTransportConsts.h"

#include <alljoyn/Status.h>


#define QCC_MODULE "ALLJOYN_BT"

using namespace ajn;
using namespace qcc;

namespace ajn {
namespace bluez {


const static uint16_t L2capDefaultMtu = (1 * 1021) + 1011; // 2 x 3DH5

/*
 * Compose the first two bytes of an HCI command from the OGF and OCF
 */
#define HCI_CMD(ogf, ocf, len)  0x1, (uint8_t)(((ogf) << 10) | (ocf)), (uint8_t)((((ogf) << 10) | (ocf)) >> 8), (uint8_t)(len)

#define CMD_LEN  4

/*
 * Set the L2CAP mtu to something better than the BT 1.0 default value.
 */
void ConfigL2capMTU(SocketFd sockFd)
{
    int ret;
    uint8_t secOpt = BT_SECURITY_LOW;
    socklen_t optLen = sizeof(secOpt);
    uint16_t outMtu = 672; // default BT 1.0 value
    ret = setsockopt(sockFd, SOL_BLUETOOTH, BT_SECURITY, &secOpt, optLen);
    if (ret < 0) {
        QCC_DbgPrintf(("Setting security low: %d: %s", errno, strerror(errno)));
    }

    struct l2cap_options opts;
    optLen = sizeof(opts);
    ret = getsockopt(sockFd, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optLen);
    if (ret != -1) {
        opts.imtu = L2capDefaultMtu;
        opts.omtu = L2capDefaultMtu;
        ret = setsockopt(sockFd, SOL_L2CAP, L2CAP_OPTIONS, &opts, optLen);
        if (ret == -1) {
            QCC_LogError(ER_OS_ERROR, ("Failed to set in/out MTU for L2CAP socket (%d - %s)", errno, strerror(errno)));
        } else {
            outMtu = opts.omtu;
            QCC_DbgPrintf(("Set L2CAP mtu to %d", opts.omtu));
        }
    } else {
        QCC_LogError(ER_OS_ERROR, ("Failed to get in/out MTU for L2CAP socket (%d - %s)", errno, strerror(errno)));
    }

    // Only let the kernel buffer up 2 packets at a time.
    int sndbuf = 2 * outMtu;

    ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    if (ret == -1) {
        QCC_LogError(ER_OS_ERROR, ("Failed to set send buf to %d: %d - %s", sndbuf, errno, strerror(errno)));
    }
}

void ConfigL2capMaster(SocketFd sockFd)
{
    int ret;
    int lmOpt = 0;
    socklen_t optLen = sizeof(lmOpt);
    ret = getsockopt(sockFd, SOL_L2CAP, L2CAP_LM, &lmOpt, &optLen);
    if (ret == -1) {
        QCC_LogError(ER_OS_ERROR, ("Failed to get LM flags (%d - %s)", errno, strerror(errno)));
    } else {
        lmOpt |= L2CAP_LM_MASTER;
        ret = setsockopt(sockFd, SOL_L2CAP, L2CAP_LM, &lmOpt, optLen);
        if (ret == -1) {
            QCC_LogError(ER_OS_ERROR, ("Failed to set LM flags (%d - %s)", errno, strerror(errno)));
        }
    }
}

} // namespace bluez
} // namespace ajn
