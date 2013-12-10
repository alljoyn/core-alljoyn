/**
 * @file
 * Utility classes for the BlueZ implementation of BT transport.
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

#include <ctype.h>
#include <vector>
#include <errno.h>
#include <fcntl.h>

#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/String.h>
#include <qcc/time.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>

#include "AdapterObject.h"
#include "BDAddress.h"
#include "BlueZ.h"
#include "BlueZIfc.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_BT"

using namespace ajn;
using namespace std;
using namespace qcc;

namespace ajn {
namespace bluez {

_AdapterObject::_AdapterObject(BusAttachment& bus, const qcc::String& path) :
    ProxyBusObject(bus, bzBusName, path.c_str(), 0),
    id(0),
    eirCapable(false),
    discovering(false),
    powered(false)
{
    size_t i = path.size();
    while (i > 0) {
        --i;
        char c = path[i];
        if (!isdigit(c)) {
            break;
        }
        id *= 10;
        id += c - '0';
    }
}


QStatus _AdapterObject::QueryDeviceInfo()
{
    QStatus status;
    SocketFd hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciGetLocalFeatures;
    vector<uint8_t> hciCmdResponse;
    uint8_t cmdStat;

    status = SendHCIRequest(hciFd, HCI_CMD_READ_LOCAL_SUPPORTED_FEATURES, 0, hciGetLocalFeatures, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
        goto exit;
    }

    eirCapable = static_cast<bool>(hciCmdResponse[7] & (1 << 0));

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::ConfigureInquiryScan(uint16_t window, uint16_t interval, bool interlaced, int8_t txPower)
{
    assert((window >= 10) && (window <= 2560));
    assert((interval >= 11) && (interval <= 2560));
    assert(window <= interval);
    assert((txPower >= -70) && (txPower <= 20));

    QStatus status;
    SocketFd hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciSetInquiryParams;
    vector<uint8_t> hciSetInquiryInterlaced;
    vector<uint8_t> hciSetInquiryTxPower;
    vector<uint8_t> hciCmdResponse;
    uint8_t interlacedArg = interlaced ? 0x01 : 0x00;
    uint8_t cmdStat;

    /*
     * Convert window and interval from millseconds to ticks.
     */
    window = (window == 10) ? 0x11 : (uint16_t)(((uint32_t)window * 1000 + 313) / 625);
    interval = (uint16_t)(((uint32_t)interval * 1000 + 313) / 625);

    hciSetInquiryParams.reserve(4);
    hciSetInquiryParams.push_back(interval & 0xff);
    hciSetInquiryParams.push_back(interval >> 8);
    hciSetInquiryParams.push_back(window & 0xff);
    hciSetInquiryParams.push_back(window >> 8);

    status = SendHCIRequest(hciFd, HCI_CMD_WRITE_INQUIRY_SCAN_ACTIVITY, 0, hciSetInquiryParams, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
        goto exit;
    }


    hciSetInquiryInterlaced.push_back(interlacedArg);

    status = SendHCIRequest(hciFd, HCI_CMD_WRITE_INQUIRY_SCAN_TYPE, 0, hciSetInquiryInterlaced, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
        goto exit;
    }


    hciSetInquiryTxPower.push_back(txPower);

    status = SendHCIRequest(hciFd, HCI_CMD_WRITE_INQUIRY_SCAN_TYPE, 0, hciSetInquiryTxPower, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
    }

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::ConfigurePeriodicInquiry(uint16_t minPeriod, uint16_t maxPeriod, uint8_t length, uint8_t maxResponses)
{
    assert(length <= 0x30);
    assert(length < minPeriod);
    assert((length == 0) || (minPeriod >= 2));
    assert((length == 0) || (minPeriod < maxPeriod));
    assert((length == 0) || (maxPeriod >= 3));

    QStatus status;
    SocketFd hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciStartPeriodicInquiry;
    vector<uint8_t> hciExitPeriodicInquiry;
    vector<uint8_t> hciCmdResponse;
    static const uint8_t LAP[] = { 0x33, 0x8B, 0x9E };
    uint8_t cmdStat;

    status = SendHCIRequest(hciFd, HCI_CMD_EXIT_PERIODIC_INQUIRY_MODE, 0, hciExitPeriodicInquiry, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
        goto exit;
    }

    hciStartPeriodicInquiry.reserve(6 + sizeof(LAP));
    hciStartPeriodicInquiry.push_back(maxPeriod & 0xff);
    hciStartPeriodicInquiry.push_back(maxPeriod >> 8);
    hciStartPeriodicInquiry.push_back(minPeriod & 0xff);
    hciStartPeriodicInquiry.push_back(minPeriod >> 8);
    hciStartPeriodicInquiry.insert(hciStartPeriodicInquiry.end(), LAP, LAP + sizeof(LAP));
    hciStartPeriodicInquiry.push_back(length);
    hciStartPeriodicInquiry.push_back(maxResponses);

    status = SendHCIRequest(hciFd, HCI_CMD_PERIODIC_INQUIRY_MODE, 0, hciStartPeriodicInquiry, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
    }

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::ConfigureSimplePairingDebugMode(bool enable)
{
    QStatus status;
    SocketFd hciFd;

    hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciSimplePairingDebugMode;
    vector<uint8_t> hciCmdResponse;
    uint8_t cmdStat;

    hciSimplePairingDebugMode.push_back(enable ? 0x01 : 0x00);

    status = SendHCIRequest(hciFd, HCI_CMD_WRITE_SIMPLE_PAIRING_DEBUG_MODE, 0, hciSimplePairingDebugMode, hciCmdResponse);
    if (status != ER_OK) {
        goto exit;
    }

    cmdStat = hciCmdResponse[0];
    if (cmdStat != 0x00) {
        status = ER_FAIL;
    }

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::ConfigureClassOfDevice(uint32_t cod)
{
    QStatus status;
    SocketFd hciFd;

    hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciWriteCOD;
    vector<uint8_t> hciCmdResponse;

    hciWriteCOD.reserve(3);
    hciWriteCOD.push_back(cod & 0xff);
    hciWriteCOD.push_back((cod >> 8) & 0xff);
    hciWriteCOD.push_back((cod >> 16) & 0xff);

    status = SendHCIRequest(hciFd, HCI_CMD_WRITE_CLASS_OF_DEVICE, 0, hciWriteCOD, hciCmdResponse);

    close(hciFd);
    return status;
}


QStatus _AdapterObject::RequestBTRole(const BDAddress& bdAddr, bt::BluetoothRole role)
{
    QStatus status;
    SocketFd hciFd;

    hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciRoleSwitch;
    vector<uint8_t> evtRoleChange;
    uint8_t addrCmdBuf[6];
    uint8_t event;

    bdAddr.CopyTo(addrCmdBuf, true);

    hciRoleSwitch.reserve(sizeof(addrCmdBuf) + 1);
    hciRoleSwitch.insert(hciRoleSwitch.end(), addrCmdBuf, addrCmdBuf + sizeof(addrCmdBuf));
    hciRoleSwitch.push_back((role == bt::MASTER) ? 0x00 : 0x01);

    status = SendHCIRequest(hciFd, HCI_CMD_ROLE_SWITCH, HCIEvtToMask(HCI_EVT_ROLE_CHANGE),
                            hciRoleSwitch, evtRoleChange);

    bool addrMatch = false;

    while ((status == ER_OK) && !addrMatch) {
        status = RecvHCIEvent(hciFd, HCIEvtToMask(HCI_EVT_ROLE_CHANGE), event, evtRoleChange);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to receive HCI event (errno %d)", errno));
            goto exit;
        }

        if ((event == HCI_EVT_ROLE_CHANGE) &&
            (evtRoleChange.size() >= 8)) {
            addrMatch = ((evtRoleChange[1] == addrCmdBuf[0]) &&
                         (evtRoleChange[2] == addrCmdBuf[1]) &&
                         (evtRoleChange[3] == addrCmdBuf[2]) &&
                         (evtRoleChange[4] == addrCmdBuf[3]) &&
                         (evtRoleChange[5] == addrCmdBuf[4]) &&
                         (evtRoleChange[6] == addrCmdBuf[5]));
        }
    }

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::IsMaster(const BDAddress& addr, bool& master)
{
    QStatus status = ER_OK;
    SocketFd hciFd;

    hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    int ret;
    struct hci_conn_info_req connInfoReq;

    addr.CopyTo(connInfoReq.bdaddr.b, true);
    connInfoReq.type = HCI_ACL_LINK;

    ret = ioctl(hciFd, HCIGETCONNINFO, &connInfoReq);
    if (ret < 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Getting connection information (%d - %s)", errno, strerror(errno)));
        goto exit;
    }

    master = static_cast<bool>(connInfoReq.conn_info.link_mode & HCI_LM_MASTER);

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::RequestEnterSniffMode(const BDAddress& addr,
                                              uint16_t minInterval,
                                              uint16_t maxInterval,
                                              uint16_t attemptTO,
                                              uint16_t sniffTO)
{
    QStatus status = ER_OK;
    SocketFd hciFd;

    assert((minInterval >= 2) && (minInterval <= 0x7FFE) && ((minInterval & 0x1) == 0));
    assert((maxInterval >= 2) && (maxInterval <= 0x7FFE) && ((maxInterval & 0x1) == 0) && (maxInterval >= minInterval));
    assert((attemptTO >= 1) && (attemptTO <= 0x7FFF));
    assert(sniffTO <= 0x7FFF);

    hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciEnterSniffMode;
    vector<uint8_t> hciCmdResponse;

    int ret;
    struct hci_conn_info_req connInfoReq;

    addr.CopyTo(connInfoReq.bdaddr.b, true);
    connInfoReq.type = HCI_ACL_LINK;

    ret = ioctl(hciFd, HCIGETCONNINFO, &connInfoReq);
    if (ret < 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Getting connection information (%d - %s)", errno, strerror(errno)));
        goto exit;
    }

    hciEnterSniffMode.reserve(5 * sizeof(uint16_t));
    hciEnterSniffMode.push_back(connInfoReq.conn_info.handle & 0xff);
    hciEnterSniffMode.push_back(connInfoReq.conn_info.handle >> 8);
    hciEnterSniffMode.push_back(minInterval & 0xff);
    hciEnterSniffMode.push_back(minInterval >> 8);
    hciEnterSniffMode.push_back(maxInterval & 0xff);
    hciEnterSniffMode.push_back(maxInterval >> 8);
    hciEnterSniffMode.push_back(attemptTO & 0xff);
    hciEnterSniffMode.push_back(attemptTO >> 8);
    hciEnterSniffMode.push_back(sniffTO & 0xff);
    hciEnterSniffMode.push_back(sniffTO >> 8);

    status = SendHCIRequest(hciFd, HCI_CMD_ENTER_SNIFF_MODE, 0, hciEnterSniffMode, hciCmdResponse);

exit:
    close(hciFd);
    return status;
}


QStatus _AdapterObject::RequestExitSniffMode(const BDAddress& addr)
{
    QStatus status = ER_OK;
    SocketFd hciFd;

    hciFd = OpenHCI();
    if (hciFd < 0) {
        status = ER_OS_ERROR;
        return status;
    }

    vector<uint8_t> hciExitSniffMode;
    vector<uint8_t> hciCmdResponse;

    int ret;
    struct hci_conn_info_req connInfoReq;

    addr.CopyTo(connInfoReq.bdaddr.b, true);
    connInfoReq.type = HCI_ACL_LINK;

    ret = ioctl(hciFd, HCIGETCONNINFO, &connInfoReq);
    if (ret < 0) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("Getting connection information (%d - %s)", errno, strerror(errno)));
        goto exit;
    }

    hciExitSniffMode.reserve(sizeof(uint16_t));
    hciExitSniffMode.push_back(connInfoReq.conn_info.handle & 0xff);
    hciExitSniffMode.push_back((connInfoReq.conn_info.handle >> 8) & 0xff);

    status = SendHCIRequest(hciFd, HCI_CMD_EXIT_SNIFF_MODE, 0, hciExitSniffMode, hciCmdResponse);

exit:
    close(hciFd);
    return status;
}


SocketFd _AdapterObject::OpenHCI()
{
    SocketFd hciFd;
    sockaddr_hci addr;
    int flags;
    int ret;

    // HCI command sent via raw sockets (must have privileges for this)
    hciFd = (SocketFd)socket(AF_BLUETOOTH, QCC_SOCK_RAW, 1);
    if (hciFd < 0) {
        goto exit;
    }

    // Need to select the adapter we are sending the HCI command to.
    addr.family = AF_BLUETOOTH;
    addr.dev = id;
    if (::bind(hciFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(hciFd);
        hciFd = -1;
        goto exit;
    }

    flags = fcntl(hciFd, F_GETFL);
    if (flags < 0) {
        close(hciFd);
        hciFd = -1;
        goto exit;
    }

    ret = fcntl(hciFd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0) {
        close(hciFd);
        hciFd = -1;
        goto exit;
    }


exit:
    if (hciFd < 0) {
        QCC_LogError(ER_OS_ERROR, ("Failed to create socket (errno %d)", errno));
    }
    return hciFd;
}


QStatus _AdapterObject::SendHCIRequest(SocketFd hciFd,
                                       uint16_t opcode, uint64_t evtMask,
                                       const vector<uint8_t>& args,
                                       vector<uint8_t>& rsp)
{
    assert(args.size() < 256);

    QStatus status;
    uint8_t cmdArgSize = args.size();
    uint8_t pktType = 0x01;
    vector<uint8_t> cmd;
    vector<uint8_t> rxData;
    size_t sent;

    evtMask |= HCIEvtToMask(HCI_EVT_CMD_COMPLETE) | HCIEvtToMask(HCI_EVT_CMD_STATUS);

    status = SetEventFilter(hciFd, opcode, evtMask);
    if (status != ER_OK) {
        goto exit;
    }

    cmd.reserve(args.size() + 4);
    cmd.push_back(pktType);
    cmd.push_back(opcode & 0xff);
    cmd.push_back(opcode >> 8);
    cmd.push_back(cmdArgSize);

    cmd.insert(cmd.end(), args.begin(), args.end());

    // Send the command.
    status = Send(hciFd, &cmd.front(), cmd.size(), sent);
    if ((status != ER_OK) || (sent != cmd.size())) {
        QCC_LogError(status, ("errno: %d   strerror(): %s", errno, strerror(errno)));
        goto exit;
    }

    while (true) {
        uint16_t rOpcode;
        uint8_t rStat;
        uint8_t event;
        status = RecvHCIEvent(hciFd, evtMask, event, rxData);
        if (status != ER_OK) {
            goto exit;
        }

        switch (event) {
        case HCI_EVT_CMD_COMPLETE:
            if (rxData.size() < 2) {
                status = ER_FAIL;
                goto exit;
            }
            rOpcode = (static_cast<uint16_t>(rxData[2]) << 8) | rxData[1];

            if (opcode == rOpcode) {
                if (rsp.empty()) {
                    rsp.assign(rxData.begin() + 3, rxData.end());
                }
                goto exit;
            }
            break;

        case HCI_EVT_CMD_STATUS:
            if (rxData.size() < 3) {
                status = ER_FAIL;
                goto exit;
            }
            rStat = rxData[0];
            rOpcode = (static_cast<uint16_t>(rxData[3]) << 8) | rxData[2];

            if (opcode == rOpcode) {
                status = (rStat == 0x00) ? ER_OK : ER_FAIL;
                goto exit;
            }
            break;

        default:
            if (rsp.empty()) {
                rsp.swap(rxData);  // faster than copying and rxData needs to be cleared anyway
            }
            break;
        }
    }

exit:
    return status;
}


QStatus _AdapterObject::RecvHCIEvent(SocketFd hciFd, uint64_t evtMask, uint8_t& event, vector<uint8_t>& rsp)
{
    uint8_t rxBuf[256];
    uint8_t evtSize;
    uint8_t pktType;
    size_t recvd;
    QStatus status = ER_OK;
    Event hciRxEvent(hciFd, Event::IO_READ, false);
    uint64_t timeout = GetTimestamp64() + 10000;  // 10 seconds
    size_t pos = 0;

    while (timeout > GetTimestamp64()) {
        status = Event::Wait(hciRxEvent, 5000);  // 5 second timeout
        if (status != ER_OK) {
            QCC_LogError(status, ("Waiting for HCI event"));
            goto exit;
        }

        status = Recv(hciFd, rxBuf + pos, ArraySize(rxBuf) - pos, recvd);
        if (status == ER_WOULDBLOCK) {
            continue;
        }
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to receive HCI event (errno %d)", errno));
            goto exit;
        }

        pos += recvd;

        if (pos > 2) {
            pktType = rxBuf[0];
            event = rxBuf[1];
            evtSize = rxBuf[2];

            if ((pktType == 0x04) && (evtSize <= (pos - 3))) {
                if (evtSize != (pos - 3)) {
                    status = ER_FAIL;
                    goto exit;
                }
                if (evtMask & (static_cast<uint64_t>(1) << event)) {
                    rsp.reserve(evtSize);
                    for (uint8_t* d = rxBuf + 3; d < (rxBuf + pos); ++d) {
                        rsp.push_back(*d);
                    }
                    goto exit;
                }
                pos = 0;  // In case we got the wrong event
            }
        }
    }

exit:
    return status;
}


QStatus _AdapterObject::SetEventFilter(SocketFd hciFd, uint16_t opcode, uint64_t evtMask)
{
    QStatus status = ER_OK;
    struct hci_filter evtFilter;
    memset(&evtFilter, 0, sizeof(evtFilter));
    int ret;

    evtFilter.type_mask = 1 << 0x04; // Set HCI event packet bit
    evtFilter.event_mask[0] = (evtMask & 0xffffffff);
    evtFilter.event_mask[1] = (evtMask >> 32);
    evtFilter.opcode = htole16(opcode);

    ret = setsockopt(hciFd, SOL_HCI, HCI_FILTER, &evtFilter, sizeof(evtFilter));
    if (ret == -1) {
        status = ER_OS_ERROR;
    }
    return status;
}


} // namespace bluez
} // namespace ajn
