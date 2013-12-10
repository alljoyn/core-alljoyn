/**
 * @file
 * BTAccessor declaration for BlueZ
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
#ifndef _ALLJOYN_BLUEZ_H
#define _ALLJOYN_BLUEZ_H

#include <qcc/platform.h>

#include <sys/ioctl.h>


#define SOL_BLUETOOTH  274
#define SOL_HCI          0
#define SOL_L2CAP        6
#define SOL_RFCOMM      18
#define BT_SECURITY      4
#define BT_SECURITY_LOW  1

#define RFCOMM_PROTOCOL_ID 3
#define RFCOMM_CONNINFO  2

#define L2CAP_PROTOCOL_ID  0

#define L2CAP_OPTIONS    1
#define L2CAP_CONNINFO   2
#define L2CAP_LM         3

#define L2CAP_LM_MASTER 0x1

#define HCI_FILTER 2

#define HCI_LM_MASTER 0x1

#define HCI_SCO_LINK  0x00
#define HCI_ACL_LINK  0x01
#define HCI_ESCO_LINK 0x02

#define HCIEvtToMask(_evt) (static_cast<uint64_t>(1) << (_evt))

#define HCIOpcode(_ogf, _ocf) (((_ogf) << 10) | ((_ocf) & 0x3ff))

#define HCI_CMD_PERIODIC_INQUIRY_MODE           (HCIOpcode(0x01, 0x003))
#define HCI_CMD_EXIT_PERIODIC_INQUIRY_MODE      (HCIOpcode(0x01, 0x004))
#define HCI_CMD_ROLE_SWITCH                     (HCIOpcode(0x02, 0x00b))
#define HCI_CMD_WRITE_INQUIRY_SCAN_ACTIVITY     (HCIOpcode(0x03, 0x01e))
#define HCI_CMD_WRITE_INQUIRY_SCAN_TYPE         (HCIOpcode(0x03, 0x043))
#define HCI_CMD_WRITE_INQUIRY_TX_POWER_LEVEL    (HCIOpcode(0x03, 0x059))
#define HCI_CMD_READ_LOCAL_SUPPORTED_FEATURES   (HCIOpcode(0x04, 0x003))
#define HCI_CMD_WRITE_SIMPLE_PAIRING_DEBUG_MODE (HCIOpcode(0x06, 0x004))
#define HCI_CMD_WRITE_CLASS_OF_DEVICE           (HCIOpcode(0x03, 0x024))
#define HCI_CMD_ENTER_SNIFF_MODE                (HCIOpcode(0x02, 0x003))
#define HCI_CMD_EXIT_SNIFF_MODE                 (HCIOpcode(0x02, 0x004))

#define HCI_EVT_CMD_COMPLETE 0x0e
#define HCI_EVT_CMD_STATUS   0x0f
#define HCI_EVT_ROLE_CHANGE  0x12





namespace ajn {
namespace bluez {
typedef struct _BDADDR {
    unsigned char b[6];
} BDADDR;

typedef struct _RFCOMM_SOCKADDR {
    uint16_t sa_family;
    BDADDR bdaddr;
    uint8_t channel;
} RFCOMM_SOCKADDR;

typedef struct _L2CAP_SOCKADDR {
    uint16_t sa_family;
    uint16_t psm;
    BDADDR bdaddr;
    uint16_t cid;
} L2CAP_SOCKADDR;

typedef union _BT_SOCKADDR {
    L2CAP_SOCKADDR l2cap;
    RFCOMM_SOCKADDR rfcomm;
} BT_SOCKADDR;

struct l2cap_options {
    uint16_t omtu;
    uint16_t imtu;
    uint16_t flush_to;
    uint8_t mode;
    uint8_t fcs;
    uint8_t max_tx;
    uint16_t txwin_size;
};

struct sockaddr_hci {
    sa_family_t family;
    uint16_t dev;
};

struct hci_conn_info {
    uint16_t handle;
    BDADDR bdaddr;
    uint8_t type;
    uint8_t out;
    uint16_t state;
    uint32_t link_mode;
#if defined(QCC_OS_ANDROID)
    /* Android developers broke the kernel ioctl API for hci_conn_info_req, so
     * we need padding to prevent stack corruption.  This only affects
     * Android.  More padding than necessary is added in case they break the
     * API even further.
     */
    uint32_t padding[16];
#endif
};

struct hci_conn_info_req {
    BDADDR bdaddr;
    uint8_t type;
    struct hci_conn_info conn_info;
};

struct hci_filter {
    uint32_t type_mask;
    uint32_t event_mask[2];
    uint16_t opcode;
};


#define HCIGETCONNINFO _IOR('H', 213, int)


} // namespace bluez
} // namespace ajn


#endif
