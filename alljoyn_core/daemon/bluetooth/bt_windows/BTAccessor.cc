/**
 * @file
 * BTAccessor implementation for Windows
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#include <Setupapi.h>
#include <errno.h>
#include <fcntl.h>
#include <bthsdpdef.h>
#pragma warning( push )
#pragma warning( disable : 4068)
#include <BluetoothAPIs.h>
#pragma warning( pop )
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>
#include <qcc/Timer.h>
#include <qcc/XmlElement.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/version.h>

#include "BDAddress.h"

// This will create an instance of the GUID WINDOWS_BLUETOOTH_DEVICE_INTERFACE in this .cpp file.
#define INITGUID
#include "BTAccessor.h"
#undef INITGUID

#include "BTController.h"
#include "BTTransport.h"
#include "WindowsBTEndpoint.h"
#include "SdpRecordBuilder.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_BT"

using namespace ajn;
using namespace qcc;
using namespace std;

namespace ajn {

/******************************************************************************/

static const::GUID alljoynUUIDBase =
{
    // 00000000-1c25-481f-9dfb-59193d238280
    0, 0x1c25, 0x481f,
    { 0x9d, 0xfb, 0x59, 0x19, 0x3d, 0x23, 0x82, 0x80 }
};

/******************************************************************************/

BTTransport::BTAccessor::BTAccessor(BTTransport* transport,
                                    const qcc::String& busGuid) :
    winBus("WindowsBTTransport"),
    busGuid(busGuid),
    transport(transport),
    recordHandle(NULL),
    deviceHandle(INVALID_HANDLE_VALUE),
    discoveryThread(*this),
    getMessageThread(*this),
    adapterChangeThread(*this),
    wsaInitialized(false),
    radioHandle(0)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::BTAccessor()"));

    EndPointsInit();
    ConnectRequestsInit();
    adapterChangeThread.Start();
}

BTTransport::BTAccessor::~BTAccessor()
{
    QCC_DbgTrace(("BTTransport::BTAccessor::~BTAccessor()"));

    adapterChangeThread.Stop();
    StopConnectable();
    discoveryThread.StopDiscovery();

    // adapterChangeThread must have exited before closing the radio handle or calling Stop()
    // so that it does not get a new radio handle and call Start() when it discovers the radio
    // handle is closed.
    adapterChangeThread.Join();

    Stop();
}

void BTTransport::BTAccessor::HandleL2CapEvent(const USER_KERNEL_MESSAGE* message)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::HandleL2CapEvent()"));

    ConnectRequestsPut(&message->messageData.l2capeventData);
}

void BTTransport::BTAccessor::HandleAcceptComplete(const USER_KERNEL_MESSAGE* message)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::HandleAcceptComplete()"));

    assert(message);

    L2CAP_CHANNEL_HANDLE handle = message->messageData.acceptComplete.channelHandle;
    BTH_ADDR address = message->messageData.acceptComplete.address;
    NTSTATUS ntStatus = message->messageData.acceptComplete.ntStatus;
    QStatus status = message->messageData.acceptComplete.status;

    QCC_DbgPrintf(("HandleAcceptComplete() message: status = %s, ntStatus = 0x%08X, address = 0x%012I64X, handle = %p",
                   QCC_StatusText(status), ntStatus, address, handle));

    WindowsBTEndpoint* endPoint = EndPointsFind(address, handle);

    if (endPoint) {
        endPoint->SetConnectionComplete(status);
    } else {
        QCC_LogError(ER_INVALID_ADDRESS, ("HandleAcceptComplete(address = 0x%012I64X, handle = %p) endPoint not found!",
                                          address, handle));
    }
}

void BTTransport::BTAccessor::HandleConnectComplete(const USER_KERNEL_MESSAGE* message)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::HandleConnectComplete()"));

    assert(message);

    L2CAP_CHANNEL_HANDLE handle = message->messageData.connectComplete.channelHandle;
    BTH_ADDR address = message->messageData.connectComplete.address;
    NTSTATUS ntStatus = message->messageData.connectComplete.ntStatus;
    QStatus status = message->messageData.connectComplete.status;

    QCC_DbgPrintf(("HandleConnectComplete() message: status = %s, ntStatus = 0x%08X, address = 0x%012I64X, handle = %p",
                   QCC_StatusText(status), ntStatus, address, handle));

    // The handle was not known at the time of the connection was attempted.
    WindowsBTEndpoint* endPoint = EndPointsFind(address);

    if (endPoint) {
        endPoint->SetChannelHandle(handle);
        endPoint->SetConnectionComplete(status);
    } else {
        QCC_LogError(ER_INVALID_ADDRESS, ("HandleConnectComplete(address = 0x%012I64X, handle = %p) endPoint not found!",
                                          address, handle));
    }
}

void BTTransport::BTAccessor::HandleReadReady(const USER_KERNEL_MESSAGE* message)
{
    L2CAP_CHANNEL_HANDLE handle = message->messageData.readReady.channelHandle;
    BTH_ADDR address = message->messageData.readReady.address;
    WindowsBTEndpoint* endPoint = EndPointsFind(address, handle);

    if (endPoint) {
        size_t bytesOfData = message->messageData.readReady.bytesOfData;
        QStatus status = message->messageData.readReady.status;

        // It is assumed this is the ONLY call to SetSourcBytesWaiting().
        endPoint->SetSourceBytesWaiting(bytesOfData, status);
    } else {
        QCC_LogError(ER_INVALID_ADDRESS, ("HandleReadReady(address = 0x%012I64X, handle = %p) endPoint not found!",
                                          address, handle));
    }
}

void BTTransport::BTAccessor::HandleMessageFromKernel(const USER_KERNEL_MESSAGE* message)
{
    switch (message->commandStatus.command) {
    case KRNUSRCMD_L2CAP_EVENT:
        // We have a incoming connection request.
        HandleL2CapEvent(message);
        break;

    case KRNUSRCMD_ACCEPT_COMPLETE:
        HandleAcceptComplete(message);
        break;

    case KRNUSRCMD_CONNECT_COMPLETE:
        HandleConnectComplete(message);
        break;

    case KRNUSRCMD_READ_READY:
        // We have incoming data ready to be read.
        HandleReadReady(message);
        break;

    case KRNUSRCMD_BAD_MESSAGE:
        // This is a message from the kernel saying an error occurred.
        QCC_LogError(ER_OS_ERROR, ("Warning from kernel mode. UserKernelComm.c:%u",
                                   message->messageData.badMessage.lineNumber));
        break;

    default:
        // This is totally unexpected. A new message has probably been added.
        // Check UserKernelComm.h for more KRNUSRCMD_XXX messages.
        QCC_LogError(ER_OS_ERROR, ("Unexpected message from kernel command=%u",
                                   message->commandStatus.command));
        assert(0);
        break;
    }
}

QStatus BTTransport::BTAccessor::DeviceSendMessage(USER_KERNEL_MESSAGE* messageIn,
                                                   USER_KERNEL_MESSAGE* messageOut) const
{
    QStatus returnValue = ER_OK;

    if (messageOut) {
        memset(messageOut, 0, sizeof(*messageOut));
    }

    if (!BluetoothIsAvailable() || INVALID_HANDLE_VALUE == deviceHandle) {
        returnValue = ER_INIT_FAILED;
    } else {
        size_t bytesReturned = 0;
        size_t outBufferSize = messageOut ? sizeof(*messageOut) : 0;

        messageIn->version = DRIVER_VERSION;
        messageIn->is64Bit = IS_64BIT;

        bool result = DeviceIo(messageIn, sizeof(*messageIn),
                               messageOut, outBufferSize, &bytesReturned);

        if (!result) {
            returnValue = ER_OS_ERROR;
            QCC_LogError(returnValue,
                         ("DeviceIoControl() error connecting to kernel! Error = 0x%08X",
                          ::GetLastError()));
            DebugDumpKernelState();
        }
    }

    return returnValue;
}

qcc::ThreadReturn STDCALL BTTransport::BTAccessor::MessageThread::Run(void* arg)
{
    QCC_DbgTrace(("MessageThread()"));

    while (!IsStopping()) {
        // Wait for a signal that a message is waiting for us.
        Event::Wait(btAccessor.getMessageEvent);

        if (!IsStopping()) {
            USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_GETMESSAGE };
            USER_KERNEL_MESSAGE messageOut;
            QStatus status = btAccessor.DeviceSendMessage(&messageIn, &messageOut);

            if (ER_OK == status) {
                // We have a message from the kernel. Deal with it.
                btAccessor.HandleMessageFromKernel(&messageOut);
            }
        }
    }
    return 0;
}


static PSP_DEVICE_INTERFACE_DETAIL_DATA GetDeviceInterfaceDetailData(void)
{
    QCC_DbgTrace(("GetDeviceInterfaceDetailData()"));

    HDEVINFO hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    ULONG length, requiredLength = 0;

    hardwareDeviceInfo = SetupDiGetClassDevs((LPGUID)&WINDOWS_BLUETOOTH_DEVICE_INTERFACE,
                                             NULL,
                                             NULL,
                                             (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (hardwareDeviceInfo == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    BOOL result = SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
                                              0,
                                              (LPGUID)&WINDOWS_BLUETOOTH_DEVICE_INTERFACE,
                                              0,
                                              &deviceInterfaceData);

    if (result == FALSE) {
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
        goto exit;
    }

    SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo, &deviceInterfaceData, NULL, 0, &requiredLength, NULL);
    deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED, requiredLength);

    if (deviceInterfaceDetailData == NULL) {
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
        goto exit;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    length = requiredLength;
    result = SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
                                             &deviceInterfaceData,
                                             deviceInterfaceDetailData,
                                             length,
                                             &requiredLength,
                                             NULL);

    if (result == FALSE) {
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
        LocalFree(deviceInterfaceDetailData);
        deviceInterfaceDetailData = NULL;
        goto exit;
    }

    SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

exit:
    return deviceInterfaceDetailData;
}

QStatus BTTransport::BTAccessor::KernelConnect(HANDLE newRadioHandle)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::KernelConnect()"));

    QStatus status = ER_OK;

    USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_SETMESSAGEEVENT };
    USER_KERNEL_MESSAGE messageOut = { USRKRNCMD_SETMESSAGEEVENT };

    SetRadioHandle(newRadioHandle);

    if (!GetRadioAddress()) {
        status = ER_INIT_FAILED;
        goto Error;
    }

    if (!wsaInitialized) {
        WSADATA wsaData;
        WORD version = MAKEWORD(2, 2);
        int error = WSAStartup(version, &wsaData);

        if (0 != error) {
            status = ER_INIT_FAILED;
            goto Error;
        }

        wsaInitialized = true;
    }

    HRESULT hr = S_OK;

    if (INVALID_HANDLE_VALUE == deviceHandle) {
        PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;

        deviceInterfaceDetailData = GetDeviceInterfaceDetailData();
        if (deviceInterfaceDetailData == NULL) {
            status = ER_OPEN_FAILED;
            QCC_LogError(status, ("Unable to connect to Bluetooth device"));
            goto Error;
        }

        deviceHandle = ::CreateFile(deviceInterfaceDetailData->DevicePath,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OVERLAPPED,
                                    NULL);

        LocalFree(deviceInterfaceDetailData);
    }

    if (deviceHandle == INVALID_HANDLE_VALUE) {
        status = ER_OPEN_FAILED;
        goto Error;
    }

    messageIn.messageData.setMessageEventData.eventHandle = getMessageEvent.GetHandle();
    status = DeviceSendMessage(&messageIn, &messageOut);

    if (ER_OK != status) {
        goto Error;
    }
    status = messageOut.commandStatus.status;

    if (ER_OK != status) {
        QCC_LogError(status,
                     ("BTTransport::BTAccessor::KernelConnect(): Unable to connect to Bluetooth driver"));
        goto Error;
    }

    // Expect the negative of the version from the kernel.
    if (DRIVER_VERSION != -messageOut.version || messageOut.is64Bit != IS_64BIT) {
        status = ER_INIT_FAILED;
        QCC_LogError(status,
                     ("BTTransport::BTAccessor::KernelConnect() user mode expects version %d %s but driver was version %d %s",
                      DRIVER_VERSION, IS_64BIT ? "64-bit" : "32-bit",
                      -messageOut.version, messageOut.is64Bit ? "64-bit" : "32-bit"));
        // If we were not successful in giving the event to the kernel no messages are coming back.
        goto Error;
    }

    if (!getMessageThread.IsRunning()) {
        status = getMessageThread.Start();
    }

    if (status != ER_OK) {
        goto Error;
    }

    if (!discoveryThread.IsRunning()) {
        status = discoveryThread.Start();
    }

    if (status != ER_OK) {
        goto Error;
    }

    transport->BTDeviceAvailable(true);

Error:
    if (ER_OK != status) {
        KernelDisconnect(true);
    }

    return status;
}

void BTTransport::BTAccessor::KernelDisconnect(bool radioIsOn)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::KernelDisconnect()"));

    transport->BTDeviceAvailable(false);

    if (radioIsOn) {
        // Tell the kernel to not send more messages.
        USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_SETMESSAGEEVENT };
        USER_KERNEL_MESSAGE messageOut;

        messageIn.messageData.setMessageEventData.eventHandle = 0;
        DeviceSendMessage(&messageIn, &messageOut);
    }

    getMessageThread.Stop();
    discoveryThread.Stop();

    EndPointsRemoveAll();

    if (INVALID_HANDLE_VALUE != deviceHandle) {
        ::CloseHandle(deviceHandle);
        deviceHandle = INVALID_HANDLE_VALUE;
    }

    // Delete the SDP record if it exists.
    this->RemoveRecord();

    if (wsaInitialized) {
        WSACleanup();
        wsaInitialized = false;
    }

    getMessageThread.Join();
    discoveryThread.Join();
    SetRadioHandle(0);
}

/************************
   These are constants that may need to be adjusted after testing.
   The discovery timeout is the time discovery may take when searching for devices.
 ***********************/
static const uint32_t DISCOVERY_TIME_IN_MILLISECONDS = 12000;

// The discovery pause is the time between checking for devices when discovery is ongoing.
static const uint32_t DISCOVERY_PAUSE_IN_MILLISECONDS = 10000;

// From MSDN BLUETOOTH_DEVICE_SEARCH_PARAMS Structure
static const unsigned int DISCOVERY_TICK_IN_MILLISECONDS = 1280;
/************************/


// Convert discovery time in milliseconds into Bluetooth ticks
static UCHAR MillisecondsToTicks(uint32_t millis) {
    uint32_t ticks = (millis + DISCOVERY_TICK_IN_MILLISECONDS - 1) / DISCOVERY_TICK_IN_MILLISECONDS;
    return (UCHAR)((ticks > 48) ? 48 : (ticks ? ticks : 1));
}


qcc::ThreadReturn STDCALL BTTransport::BTAccessor::DiscoveryThread::Run(void* arg)
{
    QStatus status = ER_OK;
    uint32_t timeout = 0;

    QCC_DbgHLPrintf(("BTTransport::BTAccessor::DiscoveryThread::Run"));

    BLUETOOTH_DEVICE_SEARCH_PARAMS deviceSearchParms = { 0 };

    deviceSearchParms.dwSize = sizeof(deviceSearchParms);
    deviceSearchParms.fIssueInquiry = TRUE;
    deviceSearchParms.fReturnAuthenticated = TRUE;
    deviceSearchParms.fReturnConnected = TRUE;
    deviceSearchParms.fReturnRemembered = TRUE;
    deviceSearchParms.fReturnUnknown = TRUE;

    while (!IsStopping() && (status == ER_OK)) {

        QCC_DbgHLPrintf((":DiscoveryThread waiting=%I32u mS", timeout));

        status = Event::Wait(Event::neverSet, timeout);
        if (!IsStopping()) {
            if (status == ER_TIMEOUT) {
                status = ER_OK;
            }
            // Clear stop event if we were just alerted
            if (status == ER_ALERTED_THREAD) {
                GetStopEvent().ResetEvent();
                status = ER_OK;
            }
            // Check if we are supposed to be running
            if (duration == 0) {
                timeout = Event::WAIT_FOREVER;
                continue;
            }

            // We don't have a radio handle initially
            btAccessor.deviceLock.Lock(MUTEX_CONTEXT);

            if (btAccessor.BluetoothIsAvailable()) {
                deviceSearchParms.hRadio = btAccessor.radioHandle;
            }

            btAccessor.deviceLock.Unlock(MUTEX_CONTEXT);

            if (deviceSearchParms.hRadio) {
                QCC_DbgHLPrintf(("DiscoveryThread duration=%I32u mS", duration));

                HBLUETOOTH_DEVICE_FIND deviceFindHandle;
                BLUETOOTH_DEVICE_INFO deviceInfo;

                deviceInfo.dwSize = sizeof(deviceInfo);

                btAccessor.deviceLock.Lock(MUTEX_CONTEXT);
                if (duration < DISCOVERY_TIME_IN_MILLISECONDS) {
                    deviceSearchParms.cTimeoutMultiplier = MillisecondsToTicks(duration);
                    duration = 1;
                } else {
                    deviceSearchParms.cTimeoutMultiplier = MillisecondsToTicks(DISCOVERY_TIME_IN_MILLISECONDS);
                    duration -= DISCOVERY_TIME_IN_MILLISECONDS;
                }
                btAccessor.deviceLock.Unlock(MUTEX_CONTEXT);

                deviceFindHandle = BluetoothFindFirstDevice(&deviceSearchParms, &deviceInfo);
                // Report found devices unless duration has gone to zero
                while (deviceFindHandle && duration) {
                    BDAddress address(deviceInfo.Address.ullLong);
                    // Filter out computers (as opposed to phones and other devices) that don't
                    // have the INFORMATION bit set.
                    if ((GET_COD_MAJOR(deviceInfo.ulClassofDevice) & COD_MAJOR_COMPUTER) &&
                        !(GET_COD_SERVICE(deviceInfo.ulClassofDevice) & COD_SERVICE_INFORMATION)) {
                        QCC_DbgHLPrintf(("DiscoveryThread non-AllJoyn %s", address.ToString().c_str()));
                    } else {
                        QCC_DbgHLPrintf(("DiscoveryThread found %s", address.ToString().c_str()));

                        btAccessor.deviceLock.Lock(MUTEX_CONTEXT);
                        bool ignoreThisOne = btAccessor.discoveryIgnoreAddrs->count(address) != 0;
                        btAccessor.deviceLock.Unlock(MUTEX_CONTEXT);

                        if (ignoreThisOne) {
                            QCC_DbgHLPrintf(("DiscoveryThread %s is black-listed", address.ToString().c_str()));
                        } else {
                            btAccessor.DeviceFound(address);
                        }
                    }
                    if (!BluetoothFindNextDevice(deviceFindHandle, &deviceInfo)) {
                        break;
                    }
                }
                BluetoothFindDeviceClose(deviceFindHandle);
                // Figure out how long to wait
                btAccessor.deviceLock.Lock(MUTEX_CONTEXT);
                if (duration < DISCOVERY_PAUSE_IN_MILLISECONDS) {
                    timeout = Event::WAIT_FOREVER;
                    duration = 0;
                } else {
                    timeout = DISCOVERY_PAUSE_IN_MILLISECONDS;
                    duration -= DISCOVERY_PAUSE_IN_MILLISECONDS;
                }
                btAccessor.deviceLock.Unlock(MUTEX_CONTEXT);
            }
        }
    }
    QCC_DbgHLPrintf(("BTTransport::BTAccessor::DiscoveryThread::Run exit"));
    return 0;
}

void BTTransport::BTAccessor::DeviceFound(const BDAddress& adBdAddr)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::DeviceChange()"));

    transport->DeviceChange(adBdAddr, bt::INVALID_UUIDREV, false);
}

QStatus BTTransport::BTAccessor::StartDiscovery(const BDAddressSet& ignoreAddrs, uint32_t duration)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::StartDiscovery()"));

    discoveryIgnoreAddrs = ignoreAddrs;
    discoveryThread.StartDiscovery(duration ? duration : 0xFFFFFFFF);

    return ER_OK;
}

QStatus BTTransport::BTAccessor::StopDiscovery()
{
    QCC_DbgHLPrintf(("BTTransport::BTAccessor::StopDiscovery"));
    discoveryThread.StopDiscovery();

    return ER_OK;
}

QStatus BTTransport::BTAccessor::StartDiscoverability(uint32_t duration)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::StartDiscoverability()"));

    QStatus status = ER_FAIL;

    deviceLock.Lock(MUTEX_CONTEXT);

    if (radioHandle && (::BluetoothIsDiscoverable(radioHandle) || ::BluetoothEnableDiscovery(radioHandle, TRUE))) {
        if (duration > 0) {
            DispatchOperation(new DispatchInfo(DispatchInfo::STOP_DISCOVERABILITY),  duration * 1000);
        }
        status = ER_OK;
    }

    deviceLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus BTTransport::BTAccessor::StopDiscoverability()
{
    QCC_DbgTrace(("BTTransport::BTAccessor::StopDiscoverability()"));

    QStatus status = ER_FAIL;

    deviceLock.Lock(MUTEX_CONTEXT);

    if (radioHandle &&
        (!::BluetoothIsDiscoverable(radioHandle) ||
         ::BluetoothEnableDiscovery(radioHandle, FALSE))) {
        status = ER_OK;
    }

    deviceLock.Unlock(MUTEX_CONTEXT);

    return status;
}

static bool BuildNameList(SdpRecordBuilder* builder, const BTNodeDB& adInfo)
{
    QCC_DbgTrace(("BuildNameList()"));

    bool returnValue = false;
    BTNodeDB::const_iterator nodeit;

    for (nodeit = adInfo.Begin(); nodeit != adInfo.End(); ++nodeit) {
        const BTNodeInfo& node = *nodeit;
        NameSet::const_iterator nameit;

        if (!builder->BeginSequence()) goto Error;
        if (!builder->AddDataElementText(node->GetGUID().ToString().c_str())) goto Error;
        if (!builder->AddDataElementUnsignedQword(node->GetBusAddress().addr.GetRaw())) goto Error;
        if (!builder->AddDataElementUnsignedWord(node->GetBusAddress().psm)) goto Error;

        if (!builder->BeginSequence()) goto Error;

        for (nameit = node->GetAdvertiseNamesBegin(); nameit != node->GetAdvertiseNamesEnd(); ++nameit) {
            if (!builder->AddDataElementText((*nameit).c_str())) goto Error;
        }

        if (!builder->EndSequence()) goto Error;
        if (!builder->EndSequence()) goto Error;
    }

    returnValue = true;

Error:
    return returnValue;
}

/**
 * Helper function to add the SDP records to a SdpRecordBuilder object.
 *
 * @param builder   The SdpRecordBuilder which creates the actual SDP record
 * @param uuidRev   The 32-bit uuid of the current revision of the service
 * @param bdAddr    The Bluetooth address for this service
 * @param psm       The psm for the service
 * @param adInfo    Map of bus node GUIDs and bus names to advertise
 *
 * @return          true if successful. false if not successful.
 */
static bool BuildSdpRecord(SdpRecordBuilder* builder,
                           uint32_t uuidRev,
                           const BDAddress& bdAddr,
                           uint16_t psm,
                           const BTNodeDB& adInfo)
{
    QCC_DbgTrace(("BuildSdpRecord()"));

    bool returnValue = false;

    ::GUID allJoynGuid = alljoynUUIDBase;

    allJoynGuid.Data1 = uuidRev;

    // All SDP records are composed of a wrapping sequence.
    if (!builder->BeginSequence()) goto Error;

    if (!builder->AddAttribute(0x0000)) goto Error;
    if (!builder->AddDataElementUnsignedDword(0x4F492354)) goto Error;

    if (!builder->AddAttribute(0x0001)) goto Error;
    if (!builder->BeginSequence()) goto Error;
    if (!builder->AddDataElementUuid(allJoynGuid)) goto Error;
    if (!builder->EndSequence()) goto Error;

    if (!builder->AddAttribute(0x0002)) goto Error;
    if (!builder->AddDataElementUnsignedDword(0x00000001)) goto Error;

    if (!builder->AddAttribute(0x0008)) goto Error;
    if (!builder->AddDataElementUnsignedByte(0xFF)) goto Error;

    if (!builder->AddAttribute(0x0004)) goto Error;
    if (!builder->BeginSequence()) goto Error;

    // L2CAP protocol identifier.
    if (!builder->BeginSequence()) goto Error;
    if (!builder->AddDataElementUuid((WORD)0x0100)) goto Error;
    if (!builder->AddDataElementUnsignedWord(psm)) goto Error;
    if (!builder->EndSequence()) goto Error;

    // End protocol descripter list.
    if (!builder->EndSequence()) goto Error;

    if (!builder->AddAttribute(0x0005)) goto Error;
    if (!builder->BeginSequence()) goto Error;
    if (!builder->AddDataElementUuid((DWORD)0x00001002)) goto Error;
    if (!builder->EndSequence()) goto Error;

    // AllJoyn Version number
    DWORD version = GetNumericVersion();

    if (!builder->AddAttribute(ALLJOYN_BT_VERSION_NUM_ATTR)) goto Error;
    if (!builder->AddDataElementUnsignedDword(version)) goto Error;

    // Dynamically determined BD Address
    const char* address = bdAddr.ToString().c_str();

    if (!builder->AddAttribute(ALLJOYN_BT_CONN_ADDR_ATTR)) goto Error;
    if (!builder->AddDataElementText(address)) goto Error;

    // Dynamically determined L2CAP PSM number.
    if (!builder->AddAttribute(ALLJOYN_BT_L2CAP_PSM_ATTR)) goto Error;
    if (!builder->AddDataElementUnsignedWord(psm)) goto Error;

    // Advertisement information.
    if (!builder->AddAttribute(ALLJOYN_BT_ADVERTISEMENTS_ATTR)) goto Error;
    if (!builder->BeginSequence()) goto Error;

    if (!BuildNameList(builder, adInfo)) goto Error;

    if (!builder->EndSequence()) goto Error;

    if (!builder->AddAttribute(0x100)) goto Error;
    if (!builder->AddDataElementText("AllJoyn")) goto Error;

    if (!builder->AddAttribute(0x101)) goto Error;
    if (!builder->AddDataElementText("AllJoyn Distributed Message Bus")) goto Error;

    // End wrapper sequence.
    if (!builder->EndSequence()) goto Error;

    returnValue = true;

Error:
    return returnValue;
}

/**
 * Helper function to initialize the WSAQUERYSET registration info before passing it to
 * WSASetService().
 *
 * @param registrationInfo  The registration info to intialize
 * @param blob              The blob used to contain the service
 * @param recordHandle      The destination of the record handle to be received or deleted.
 * @param builder           The SdpRecordBuilder which contains the SDP record (optional, may be NULL)
 */
static void InitializeSetService(WSAQUERYSET* registrationInfo,
                                 BLOB* blob,
                                 BTH_SET_SERVICE* service,
                                 HANDLE* recordHandle,
                                 const SdpRecordBuilder* builder = NULL)
{
    QCC_DbgTrace(("InitializeSetService()"));

    // This needs to be a static because we just take the address of it. The caller
    // of this function depends on the continuing existence of this variable.
    static ULONG version = BTH_SDP_VERSION;

    memset(service, 0, sizeof(*service));
    memset(blob, 0, sizeof(*blob));
    memset(registrationInfo, 0, sizeof(*registrationInfo));

    blob->cbSize = sizeof(*service);
    blob->pBlobData = (BYTE*)service;

    const size_t sdpRecordLength = builder && builder->GetRecord() ? builder->GetRecordSize() : 0;

    if (sdpRecordLength > 0) {
        memcpy_s(&(service->pRecord[0]), sdpRecordLength, builder->GetRecord(), sdpRecordLength);

        // - 1 because service->pRecord is of size 1 and is also used for the storage of
        // the SDP record.
        blob->cbSize += sdpRecordLength - 1;
    }

    // Set INFORMATION class-of-service bit to indicate that this is AllJoyn capable
    service->fCodService = COD_SERVICE_INFORMATION;
    service->pSdpVersion = &version;
    service->pRecordHandle = recordHandle;
    service->ulRecordLength = sdpRecordLength;

    registrationInfo->dwSize = sizeof(*registrationInfo);
    registrationInfo->lpBlob = blob;
    registrationInfo->dwNameSpace = NS_BTH;

    registrationInfo->lpServiceClassId = (LPGUID)&alljoynUUIDBase;
    registrationInfo->lpszServiceInstanceName = TEXT("AllJoyn Bluetooth Service");
    registrationInfo->dwNumberOfCsAddrs = 1;
}

QStatus BTTransport::BTAccessor::SetSDPInfo(uint32_t uuidRev,
                                            const BDAddress& bdAddr,
                                            uint16_t psm,
                                            const BTNodeDB& adInfo)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::SetSDPInfo(uuidRev = %08x, bdAddress = %s, psm = 0x%04x, adInfo = <%u nodes>)",
                  uuidRev, bdAddr.ToString().c_str(), psm, adInfo.Size()));

    QStatus status = ER_FAIL;

    if (uuidRev == bt::INVALID_UUIDREV) {
        QCC_DbgPrintf(("Removing record handle %x (no more records)", recordHandle));
        RemoveRecord();
    } else {
        SdpRecordBuilder builder;
        bool sdpRecordBuilt = BuildSdpRecord(&builder, uuidRev, bdAddr, psm, adInfo);

        if (sdpRecordBuilt) {
            size_t sdpRecordLength = builder.GetRecordSize();

            if (0 == sdpRecordLength) {
                status = ER_OUT_OF_MEMORY;
            } else {
                BTH_SET_SERVICE* service =
                    (BTH_SET_SERVICE*)malloc(sizeof(*service) + sdpRecordLength);

                if (!service) {
                    status = ER_OUT_OF_MEMORY;
                } else {
                    BLOB blob;
                    WSAQUERYSET registrationInfo;

                    InitializeSetService(&registrationInfo,
                                         &blob,
                                         service,
                                         &recordHandle,
                                         &builder);

                    // No longer need the old record because we are about to add a new one.
                    // This MUST be called before WSASetService().
                    RemoveRecord();

                    QCC_DbgPrintf(("Adding Record: UUID = %08x, %04x, %04x, %02x%0x2, %02x%02x%02x%02x%02x%02x",
                                   uuidRev,
                                   alljoynUUIDBase.Data2,
                                   alljoynUUIDBase.Data3,
                                   alljoynUUIDBase.Data4[0],
                                   alljoynUUIDBase.Data4[1],
                                   alljoynUUIDBase.Data4[2],
                                   alljoynUUIDBase.Data4[3],
                                   alljoynUUIDBase.Data4[4],
                                   alljoynUUIDBase.Data4[5],
                                   alljoynUUIDBase.Data4[6],
                                   alljoynUUIDBase.Data4[7]));

                    // The dwControlFlags parameter is reserved, and must be zero. From:
                    // http://msdn.microsoft.com/en-us/library/aa362921.aspx
                    INT wsaReturnValue = WSASetService(&registrationInfo, RNRSERVICE_REGISTER, 0);

                    if (0 != wsaReturnValue) {
                        int error = WSAGetLastError();

                        switch (error) {
                        case WSAEACCES: // Insufficient privileges to install the Service.
                            status = ER_AUTH_FAIL;
                            break;

                        case WSAEINVAL: // One or more required parameters were invalid or missing.
                            status = ER_INVALID_DATA;
                            break;

                        case WSA_NOT_ENOUGH_MEMORY:
                            status = ER_OUT_OF_MEMORY;
                            break;

                        case WSAEHOSTUNREACH:
                            status = ER_FAIL;
                            break;

                        default:
                            status = ER_FAIL;
                            break;
                        }
                    } else {
                        status = ER_OK;
                        QCC_DbgPrintf(("Got record handle 0x%08x", recordHandle));
                    }
                }

                free(service);
            }
        }
    }

    return status;
}

QStatus BTTransport::BTAccessor::StartConnectable(BDAddress& addr, uint16_t& psm)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::StartConnectable()"));

    QStatus status = ER_FAIL;

    deviceLock.Lock(MUTEX_CONTEXT);

    if (radioHandle && INVALID_HANDLE_VALUE != deviceHandle) {
        USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_STARTCONNECTABLE };
        USER_KERNEL_MESSAGE messageOut;
        addr = address;

        psm = 0;
        status = DeviceSendMessage(&messageIn, &messageOut);

        if (ER_OK == status) {
            psm = messageOut.messageData.startConnectableData.psm;
            status = messageOut.commandStatus.status;

            if (ER_OK == status) {
                bool isConnectable = ::BluetoothIsConnectable(radioHandle);

                if (!isConnectable) {
                    if (!::BluetoothEnableIncomingConnections(radioHandle, TRUE)) {
                        status = ER_FAIL;
                    }
                }
            }
        }
    }

    deviceLock.Unlock(MUTEX_CONTEXT);

    return status;
}

void BTTransport::BTAccessor::StopConnectable()
{
    QCC_DbgTrace(("BTTransport::BTAccessor::StopConnectable()"));

    deviceLock.Lock(MUTEX_CONTEXT);

    // From MSDN (http://msdn.microsoft.com/en-us/library/aa362778):
    // A radio that is non-connectable is non-discoverable. The radio must be made
    // non-discoverable prior to making a radio non-connectable. Failure to make a radio
    // non-discoverable prior to making it non-connectable will result in failure of the
    // BluetoothEnableIncomingConnections function call.
    if (::BluetoothIsConnectable(radioHandle) && INVALID_HANDLE_VALUE != deviceHandle) {
        ::BluetoothEnableIncomingConnections(radioHandle, FALSE);
        USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_STOPCONNECTABLE };

        DeviceSendMessage(&messageIn, 0);
    }

    deviceLock.Unlock(MUTEX_CONTEXT);
}

RemoteEndpoint* BTTransport::BTAccessor::Accept(BusAttachment& alljoyn, Event* connectEvent)
{
    struct _KRNUSRCMD_L2CAP_EVENT connectRequest;
    WindowsBTEndpoint* conn(NULL);

    QStatus status = ConnectRequestsGet(&connectRequest);

    if (ER_OK == status) {
        L2CAP_CHANNEL_HANDLE channelHandle = connectRequest.channelHandle;
        BTH_ADDR address = connectRequest.address;
        BDAddress remAddr;

        QCC_DbgTrace(("BTTransport::BTAccessor::Accept(address = 0x%012I64X, handle = %p)",
                      address,
                      channelHandle));

        remAddr.SetRaw(address);

        BTBusAddress incomingAddr(remAddr, bt::INCOMING_PSM);
        BTNodeInfo dummyNode(incomingAddr);
        BTBusAddress redirectAddr;

        // The rejection of the incoming request must come after the normal accept proceedure.
        // So save this status for later testing and potentially reject it then.
        QStatus redirectStatus = ER_OK;

        if (!transport->CheckIncomingAddress(remAddr, redirectAddr)) {
            redirectStatus = ER_BUS_CONNECTION_REJECTED;
            QCC_DbgPrintf(("Rejected connection from: %s", remAddr.ToString().c_str()));
        }

        conn = new WindowsBTEndpoint(alljoyn, true, dummyNode, this, address, redirectAddr);

        if (conn) {
            conn->SetChannelHandle(channelHandle);

            if (!EndPointsAdd(conn)) {
                // The destructor will cause a disconnect to be send to the kernel.
                delete conn;
                conn = NULL;
            } else {
                USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_ACCEPT };
                USER_KERNEL_MESSAGE messageOut = { USRKRNCMD_ACCEPT };

                messageIn.messageData.acceptData.address = address;
                messageIn.messageData.acceptData.channelHandle = channelHandle;

                status = DeviceSendMessage(&messageIn, &messageOut);

                QCC_DbgPrintf(("Accept send message status = %s", QCC_StatusText(status)));
                QCC_DbgPrintf(("L2CapAccept() status = %s",
                               QCC_StatusText(messageOut.commandStatus.status)));

                if (ER_OK == status && ER_OK == messageOut.commandStatus.status) {
                    status = conn->WaitForConnectionComplete(true /*incoming*/);

                    QCC_DbgPrintf(("AcceptComplete() Wait status = %s", QCC_StatusText(status)));
                    QCC_DbgPrintf(("AcceptComplete() Connect status = %s",
                                   QCC_StatusText(conn->GetConnectionStatus())));
                }

                if (ER_OK != redirectStatus || ER_OK != status || ER_OK != conn->GetConnectionStatus()) {
                    // The destructor will cause a disconnect to be send to the kernel and
                    // for it to be removed from activeEndPoints[].
                    delete conn;
                    conn = NULL;
                }
            }
        }
    } else {
        QCC_DbgTrace(("BTTransport::BTAccessor::ConnectRequestsGet() failed"));
    }

    return conn;
}

RemoteEndpoint* BTTransport::BTAccessor::Connect(BusAttachment& alljoyn,
                                                 const BTNodeInfo& node)
{
    WindowsBTEndpoint* conn(NULL);
    USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_CONNECT };
    USER_KERNEL_MESSAGE messageOut;
    const BTBusAddress& connAddr = node->GetBusAddress();
    BTBusAddress noRedirect;

    QCC_DbgTrace(("BTTransport::BTAccessor::Connect(node = %s)",
                  connAddr.ToString().c_str()));

    if (!connAddr.IsValid()) {
        QCC_DbgPrintf(("In Connect() connAddr.IsValid() == false!"));
        goto Error;
    }

    BTH_ADDR address = connAddr.addr.GetRaw();

    messageIn.messageData.connectData.address = address;
    messageIn.messageData.connectData.psm = connAddr.psm;

    QCC_DbgPrintf(("L2CapConnect(address = 0x%012I64X, psm = 0x%04X)", address, connAddr.psm));

    conn = new WindowsBTEndpoint(alljoyn, false, node, this, address, noRedirect);

    if (conn) {
        // The connection must be added before we send the message to the kernel because the kernel
        // could send the connect complete message back and the endpoint might not be found.
        if (!EndPointsAdd(conn)) {
            // The destructor will cause a disconnect to be sent to the kernel.
            delete conn;
            conn = NULL;
        } else {
            // The radio will not fully connect to another if it is currently connectable.
            // If we were in a connectable state then save that information and stop being
            // connectable for the duration of Connect().
            deviceLock.Lock(MUTEX_CONTEXT);

            bool wasConnectable = radioHandle &&
                                  INVALID_HANDLE_VALUE != deviceHandle &&
                                  ::BluetoothIsConnectable(radioHandle);

            if (wasConnectable) {
                ::BluetoothEnableIncomingConnections(radioHandle, FALSE);
            }

            deviceLock.Unlock(MUTEX_CONTEXT);

            QStatus status = DeviceSendMessage(&messageIn, &messageOut);

            QCC_DbgPrintf(("Connect send message status = %s", QCC_StatusText(status)));
            QCC_DbgPrintf(("L2CapConnect() status = %s", QCC_StatusText(messageOut.commandStatus.status)));

            if (ER_OK == status) {
                status = messageOut.commandStatus.status;
            }

            if (ER_OK == status) {
                status = conn->WaitForConnectionComplete(false /*outgoing*/);

                QCC_DbgPrintf(("ConnectComplete() Wait status = %s",
                               QCC_StatusText(status)));

                if (ER_OK == status) {
                    status = conn->GetConnectionStatus();
                    QCC_DbgPrintf(("ConnectComplete() Connect status = %s",
                                   QCC_StatusText(status)));
                }

                if (ER_OK == status) {
                    // The channel handle should have come in with the completion status.
                    assert(conn->GetChannelHandle() != NULL);
                }
            }

            if (ER_OK != status) {
                // The destructor will cause a disconnect to be send to the kernel and
                // for it to be removed from activeEndPoints[].
                delete conn;
                conn = NULL;
            }

            deviceLock.Lock(MUTEX_CONTEXT);

            if (wasConnectable && radioHandle) {
                ::BluetoothEnableIncomingConnections(radioHandle, TRUE);
            }

            deviceLock.Unlock(MUTEX_CONTEXT);
        }
    }

Error:
    return conn;
}

/**
 * Promote a 16-bit UUID to a 128-bit UUID.
 */
static void BlueToothPromoteUuid(::GUID* destination, WORD shortUuid)
{
    QCC_DbgTrace(("BlueToothPromoteUuid()"));

    // This is the SDP uuid base.
    // 00000000-0000-1000-8000-00805F9B34FB
    static const::GUID baseUuid =
    {
        0, 0, 0x1000,
        { 0x80, 0x0, 00, 0x80, 0x5F, 0x9B, 0x34, 0xFB }
    };

    *destination = baseUuid;
    destination->Data1 = shortUuid;
}

void BDAddressToAddressAsString(DWORD stringLength, TCHAR* string, const BDAddress* addr)
{
    QCC_DbgTrace(("BDAddressToAddressAsString()"));

    SOCKADDR sockAddress;
    const DWORD addressLength = 30;

    sockAddress.sa_family = AF_BTH;
    addr->CopyTo((uint8_t*)sockAddress.sa_data, true);

    WSAAddressToString(&sockAddress, addressLength, 0, string, &stringLength);
}


/**
 * Get the querySetBuffer of data for this handle.
 * @param lookupHandle[in] The handle to the devie to get the information for.
 * @param bufferLength[in/out] The length of the supplied buffer.
 * @param querySetBuffer[in/out] The buffer to put the info in. This will be reallocated if necessary.
 *
 * @return true if successful.
 */
static bool LookupNextRecord(HANDLE lookupHandle, DWORD& bufferLength, WSAQUERYSET*& querySetBuffer)
{
    const DWORD controlFlags = LUP_RETURN_ALL;
    bool returnValue = true;
    DWORD wsaSpecifiedBufferLength = bufferLength;

    querySetBuffer->dwSize = sizeof(WSAQUERYSET);
    querySetBuffer->lpBlob = NULL;
    int err = WSALookupServiceNext(lookupHandle, controlFlags, &wsaSpecifiedBufferLength, querySetBuffer);

    if (SOCKET_ERROR == err) {
        err = WSAGetLastError();
        returnValue = false;

        // Was the buffer too small?
        if (WSAEFAULT == err) {
            // Yes, the buffer was too small. Allocate one of the suggested size.
            ::free(querySetBuffer);
            bufferLength = wsaSpecifiedBufferLength;
            querySetBuffer = (WSAQUERYSET*)::malloc(bufferLength);

            if (querySetBuffer) {
                // Try looking up the next record with the larger buffer.
                querySetBuffer->dwSize = sizeof(*querySetBuffer);
                querySetBuffer->lpBlob = NULL;
                err = WSALookupServiceNext(lookupHandle, controlFlags, &wsaSpecifiedBufferLength, querySetBuffer);

                if (SOCKET_ERROR != err) {
                    returnValue = true;
                } else {
                    err = WSAGetLastError();
                }
            } else {
                QCC_LogError(ER_OUT_OF_MEMORY, ("LookupNextRecord(): malloc(%d) failed.", bufferLength));
                bufferLength = 0;
            }
        }
        if (err && (WSA_E_NO_MORE != err)) {
            QCC_LogError(ER_FAIL, ("WSA error 0x%x when looking up next SDP record.", err));
        }
    }
    return returnValue;
}

/**
 * Get the AllJoyn uuid revision associated with this record blob. Return true if found.
 */
static bool GetSdpAllJoynUuidRevision(BLOB* blob, uint32_t* uuidRev)
{
    // Attribute 1 is the attribute that contains the AllJoyn GUID.
    static const USHORT UUID_ATTRIBUTE = 1;
    bool foundIt = false;
    SDP_ELEMENT_DATA data;
    DWORD status = ::BluetoothSdpGetAttributeValue(blob->pBlobData, blob->cbSize, UUID_ATTRIBUTE, &data);

    // Do we have a sequence?
    if (ERROR_SUCCESS == status && SDP_TYPE_SEQUENCE == data.type) {
        // We have a sequence. Do we have a UUID in here?
        SDP_ELEMENT_DATA sequenceDataElement;
        HBLUETOOTH_CONTAINER_ELEMENT element = NULL;
        ULONG sequenceResult;

        do {
            sequenceResult = ::BluetoothSdpGetContainerElementData(data.data.sequence.value,
                                                                   data.data.sequence.length,
                                                                   &element,
                                                                   &sequenceDataElement);

            if (ERROR_SUCCESS == sequenceResult && SDP_ST_UUID128 == sequenceDataElement.specificType) {
                // We have a UUID. Is it the AllJoyn UUID?
                ::GUID* uuid = &sequenceDataElement.data.uuid128;

                if (alljoynUUIDBase.Data2 == uuid->Data2 &&
                    alljoynUUIDBase.Data3 == uuid->Data3 &&
                    0 == memcmp(alljoynUUIDBase.Data4, uuid->Data4, sizeof(alljoynUUIDBase.Data4))) {
                    // We have the AllJoyn UUID. Grab the AllJoyn version number.
                    *uuidRev = uuid->Data1;
                    foundIt = true;
                }
            }
        } while (!foundIt && ERROR_SUCCESS == sequenceResult);
    }

    return foundIt;
}

/**
 * Get the AllJoyn bus address associated with this record blob. Return true if found.
 */
static bool GetSdpBusAddress(BLOB* blob, BDAddress* bdAddr)
{
    bool foundIt = false;
    SDP_ELEMENT_DATA data;
    DWORD status = ::BluetoothSdpGetAttributeValue(blob->pBlobData,
                                                   blob->cbSize,
                                                   ALLJOYN_BT_CONN_ADDR_ATTR,
                                                   &data);

    // Do we have the Bus address?
    if (ERROR_SUCCESS == status && SDP_TYPE_STRING == data.type) {
        // We have a string.
        const int STRING_BUFFER_SIZE = 256;
        char dataString[STRING_BUFFER_SIZE];
        memset(dataString, 0, sizeof(dataString));

        memcpy_s(dataString,
                 sizeof(dataString) - sizeof(dataString[0]),  // Make sure result is nul terminated.
                 data.data.string.value,
                 data.data.string.length);

        qcc::String addr(dataString);
        QStatus status = bdAddr->FromString(addr);

        if (ER_OK == status) {
            foundIt = true;
        } else {
            QCC_LogError(status, ("Failed to parse the BD Address: \"%s\"", dataString));
        }
    }

    return foundIt;
}

/**
 * Get the AllJoyn psm associated with this record blob. Return true if found.
 */
static bool GetSdpPsm(BLOB* blob, uint16_t* psm)
{
    bool foundIt = false;
    SDP_ELEMENT_DATA data;
    DWORD status = ::BluetoothSdpGetAttributeValue(blob->pBlobData,
                                                   blob->cbSize,
                                                   ALLJOYN_BT_L2CAP_PSM_ATTR,
                                                   &data);

    // Do we have the psm?
    if (ERROR_SUCCESS == status && SDP_ST_UINT16 == data.specificType) {
        *psm = data.data.uint16;
        foundIt = true;
    }

    return foundIt;
}

/**
 * Get the AllJoyn version number of the remote device associated with this record blob.
 * Return true if found and it satisfies the mimimum version required.
 */
static bool GetSdpRemoteVersion(BLOB* blob, uint32_t* remoteVersion)
{
    bool foundIt = false;
    SDP_ELEMENT_DATA data;
    DWORD status = ::BluetoothSdpGetAttributeValue(blob->pBlobData,
                                                   blob->cbSize,
                                                   ALLJOYN_BT_VERSION_NUM_ATTR,
                                                   &data);

    // Do we have the remote version?
    if (ERROR_SUCCESS == status && SDP_ST_UINT32 == data.specificType) {
        *remoteVersion = data.data.uint32;
        foundIt = true;
    }

    return foundIt;
}

/**
 * Get the advertised names from this sequence and put them in the adInfo.
 * Return true if this is a sequence. There may be zero names but still return
 * true to mimic the BlueZ behavior.
 */
static bool GetSdpAdvertisedNames(SDP_ELEMENT_DATA* data, BTNodeInfo nodeInfo)
{
    bool gotNames = data->type == SDP_TYPE_SEQUENCE;

    if (gotNames) {
        ULONG sequenceResult;
        HBLUETOOTH_CONTAINER_ELEMENT element = NULL;

        do {
            SDP_ELEMENT_DATA sequenceDataElement;   // Individual data item.

            sequenceResult = ::BluetoothSdpGetContainerElementData(data->data.sequence.value,
                                                                   data->data.sequence.length,
                                                                   &element,
                                                                   &sequenceDataElement);

            if (ERROR_SUCCESS == sequenceResult && SDP_TYPE_STRING == sequenceDataElement.type) {
                String nameString((char*)sequenceDataElement.data.string.value,  sequenceDataElement.data.string.length);
                QCC_DbgPrintf(("Got advertised name %s", nameString.c_str()));
                String trimmedString = Trim(nameString);
                if (!trimmedString.empty()) {
                    nodeInfo->AddAdvertiseName(trimmedString);
                }
            }
        } while (ERROR_SUCCESS == sequenceResult);
    }

    return gotNames;
}

/**
 * Get a single SDP node from the sequence in *data and add it to *adInfo.
 * Return true if a valid node was found and added. Return false otherwise.
 *
 * This function gets called for the second sequence level which is for a
 * single BT Node. In that node exist the GUID, BT device address, PSM, and
 * then a sequence of advertised names.
 */
static bool GetOneSdpBtNode(SDP_ELEMENT_DATA* data, BTNodeDB* adInfo)
{
    bool validNode = true;
    bool gotGUID = false;
    bool gotBDAddr = false;
    bool gotPSM = false;
    bool gotNames = false;

    // The first four elements must be the GUID, BT device address,
    // PSM, and list of advertised names. Future versions of
    // AllJoyn may extend the SDP record with additional elements,
    // but this set in this order is the minimum requirement. Any
    // missing information means the SDP record is malformed and
    // we should ignore it.
    BTNodeInfo nodeInfo;    // The node to be filled out and added to adInfo.
    BDAddress addr;
    uint16_t psm = bt::INVALID_PSM;
    HBLUETOOTH_CONTAINER_ELEMENT element = NULL;
    ULONG sequenceResult;

    do {
        SDP_ELEMENT_DATA sequenceDataElement;   // Individual data item.
        sequenceResult = ::BluetoothSdpGetContainerElementData(data->data.sequence.value,
                                                               data->data.sequence.length,
                                                               &element,
                                                               &sequenceDataElement);

        if (ERROR_SUCCESS == sequenceResult) {
            switch (sequenceDataElement.type) {
            case SDP_TYPE_STRING:   // The GUID for this node.
            {
                char guidStringBuffer[256];

                memset(guidStringBuffer, 0, sizeof(guidStringBuffer));
                memcpy_s(guidStringBuffer,
                         sizeof(guidStringBuffer) - 1,          // Insure nul termination.
                         sequenceDataElement.data.string.value,
                         sequenceDataElement.data.string.length);

                String guidString = guidStringBuffer;
                String trimmedString = Trim(guidString);

                if (trimmedString.empty()) {
                    validNode = false;
                } else {
                    nodeInfo->SetGUID(trimmedString);
                    gotGUID = true;
                }
                break;
            }

            case SDP_TYPE_UINT:
                switch (sequenceDataElement.specificType) {
                case SDP_ST_UINT16:     // The psm.
                    psm = sequenceDataElement.data.uint16;
                    gotPSM = true;
                    break;

                case SDP_ST_UINT64:     // The BDAddress.
                    // Check for validity. Must not be zero and must be less than 48-bits.
                    if (0 == sequenceDataElement.data.uint64 ||
                        (sequenceDataElement.data.uint64 & ~0xffffffffffffULL)) {
                        validNode = false;
                    } else {
                        addr.SetRaw(sequenceDataElement.data.uint64);
                        gotBDAddr = true;
                    }
                    break;

                default:    // Unexpected type. Must be a corrupted record.
                    validNode = false;
                    break;
                }
                break;

            case SDP_TYPE_SEQUENCE: // The list of advertised names for the given node.
                gotNames = GetSdpAdvertisedNames(&sequenceDataElement, nodeInfo);
                break;

            default:    // Unexpected type. Must be a corrupted record.
                validNode = false;
                break;
            }
        }
    } while (validNode && ERROR_SUCCESS == sequenceResult);

    validNode = validNode && gotGUID && gotBDAddr && gotPSM && gotNames;

    if (validNode && ERROR_NO_MORE_ITEMS == sequenceResult) {
        nodeInfo->SetBusAddress(BTBusAddress(addr, psm));
        adInfo->AddNode(nodeInfo);
    }

    return validNode;
}

/**
 * Get the AllJoyn BTNodeDB associated with this record blob. Return true there were no errors.
 */
static bool GetSdpBTNodeDB(BLOB* blob, BTNodeDB* adInfo)
{
    QCC_DbgTrace(("GetSdpBTNodeDB()"));

    // It's okay for this to not be found so assume things are good until proven otherwise.
    bool foundIt = true;
    SDP_ELEMENT_DATA data;
    DWORD status = ::BluetoothSdpGetAttributeValue(blob->pBlobData,
                                                   blob->cbSize,
                                                   ALLJOYN_BT_ADVERTISEMENTS_ATTR,
                                                   &data);
    // Do we have a sequence?
    if (ERROR_SUCCESS == status && SDP_TYPE_SEQUENCE == data.type) {
        // We have a sequence. Zero or more BTNodeInfo exist. Each BTNodeInfo is another sequence.
        SDP_ELEMENT_DATA sequenceDataElement;
        HBLUETOOTH_CONTAINER_ELEMENT element = NULL;
        ULONG sequenceResult;

        do {
            sequenceResult = ::BluetoothSdpGetContainerElementData(data.data.sequence.value,
                                                                   data.data.sequence.length,
                                                                   &element,
                                                                   &sequenceDataElement);

            if (ERROR_SUCCESS == sequenceResult && SDP_TYPE_SEQUENCE == sequenceDataElement.type) {
                // We have a sequence. Assume it is a BTNodeInfo nodeInfo;
                foundIt = GetOneSdpBtNode(&sequenceDataElement, adInfo);
            }
        } while (foundIt && ERROR_SUCCESS == sequenceResult);
    }

    return foundIt;
}

/**
 * Get the AllJoyn attributes associated with this record blob. Return true if all were found.
 */
static bool GetSdpAttributes(BLOB* blob, uint32_t* uuidRev, BTBusAddress* connAddr, BTNodeDB* adInfo)
{
    if (blob && blob->cbSize) {
        if (!GetSdpAllJoynUuidRevision(blob, uuidRev)) {
            return false;
        }
        QCC_DbgPrintf(("Got UUID_REV %d", *uuidRev));

        uint32_t remoteVersion;
        if (!GetSdpRemoteVersion(blob, &remoteVersion)) {
            return false;
        }
        QCC_DbgPrintf(("Got REMOTE_VERSION %d", remoteVersion));

        uint16_t psm;
        BDAddress bdAddr;
        if (!GetSdpBusAddress(blob, &bdAddr) || !GetSdpPsm(blob, &psm)) {
            return false;
        }
        *connAddr = BTBusAddress(bdAddr, psm);
        QCC_DbgPrintf(("Got BUS_ADDRESS & PSM %d", psm));

        if (!GetSdpBTNodeDB(blob, adInfo)) {
            return false;
        }
        QCC_DbgPrintf(("Got BT_NODE_DB"));
    }
    return true;
}

/**
 * Get the handle for the device inquiry. If status is non-NULL more detailed error info is
 * supplied in the case of failure.
 *
 * @param status[out] Optional. Destination of the error status.
 *
 * @return The handle for device inquiry or NULL if failure.
 */
static HANDLE BeginDeviceInquiry(const BDAddress* address, QStatus* status)
{
    QCC_DbgTrace(("BeginDeviceInquiry()"));

    HANDLE returnValue = 0;
    TCHAR addressAsString[256];
    DWORD addressStringLength = _countof(addressAsString);

    BDAddressToAddressAsString(addressStringLength, addressAsString, address);

    WSAQUERYSET querySet = { 0 };

    ::GUID guidForL2CapService;

    // The L2CP UUID is a promoted 16-bit class.
    BlueToothPromoteUuid(&guidForL2CapService, L2CAP_PROTOCOL_UUID16);

    querySet.dwSize = sizeof(querySet);
    querySet.lpServiceClassId = &guidForL2CapService;
    querySet.lpszContext = addressAsString;
    querySet.dwNameSpace = NS_BTH;

    if (status) {
        *status = ER_OK;
    }

    DWORD controlFlags = LUP_FLUSHCACHE | LUP_RETURN_BLOB;
    uint32_t retryCount = 8;

    while (retryCount--) {
        if (WSALookupServiceBegin(&querySet, controlFlags, &returnValue) == 0) {
            QCC_DbgTrace(("BeginDeviceInquiry() found device handle=%u", returnValue));
            break;
        }
        // If not successful make sure the returned handle is 0.
        returnValue = 0;

        int wsaError = WSAGetLastError();
        QStatus error;

        switch (wsaError) {
        case WSA_NOT_ENOUGH_MEMORY:
            error = ER_OUT_OF_MEMORY;
            QCC_LogError(error, ("WSA_NOT_ENOUGH_MEMORY"));
            retryCount = 0;
            break;

        case WSAEINVAL:
            error = ER_INVALID_DATA;
            QCC_LogError(error, ("WSAEINVAL"));
            retryCount = 0;
            break;

        case WSANO_DATA:
            error = ER_INVALID_DATA;
            QCC_LogError(error, ("WSANO_DATA"));
            retryCount = 0;
            break;

        case WSANOTINITIALISED:
            error = ER_INIT_FAILED;
            QCC_LogError(error, ("WSANOTINITIALISED"));
            retryCount = 0;
            break;

        case WSASERVICE_NOT_FOUND:
            if (retryCount) {
                uint32_t delay = 3000 + qcc::Rand8() * 50;
                QCC_LogError(error, ("WSASERVICE_NOT_FOUND retrying in %d seconds", delay / 1000));
                qcc::Sleep(delay);
            } else {
                error = ER_FAIL;
                QCC_LogError(error, ("WSASERVICE_NOT_FOUND"));
            }
            break;

        default:
            error = ER_FAIL;
            QCC_LogError(error, ("wsaError=%#x", wsaError));
            retryCount = 0;
            break;
        }
        if (status) {
            *status = error;
        }
    }
    return returnValue;
}

QStatus BTTransport::BTAccessor::GetDeviceInfo(const BDAddress& requestedAddr,
                                               uint32_t* uuidRev,
                                               BTBusAddress* connAddr,
                                               BTNodeDB* adInfo)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::GetDeviceInfo(address = %s)", requestedAddr.ToString().c_str()));
    QStatus status = ER_FAIL;
    HANDLE lookupHandle = BeginDeviceInquiry(&requestedAddr, &status);

    if (lookupHandle) {
        DWORD bufferLength = sizeof(WSAQUERYSET) + 2048;  // Just something moderately large.
        WSAQUERYSET* querySetBuffer = (WSAQUERYSET*)malloc(bufferLength);
        while (LookupNextRecord(lookupHandle, bufferLength, querySetBuffer)) {
            if (GetSdpAttributes(querySetBuffer->lpBlob, uuidRev, connAddr, adInfo)) {
                status = ER_OK;
                break;
            }
        }
        free(querySetBuffer);
        WSALookupServiceEnd(lookupHandle);
    }

    return status;
}

void BTTransport::BTAccessor::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::AlarmTriggered()"));

    DispatchInfo* op = static_cast<DispatchInfo*>(alarm->GetContext());

    if (reason == ER_OK) {
        switch (op->operation) {
        case DispatchInfo::STOP_DISCOVERABILITY:
            QCC_DbgPrintf(("Stopping Discoverability"));
            StopDiscoverability();
            break;
        }
    }

    delete op;
}

HANDLE BTTransport::BTAccessor::GetRadioHandle()
{
    BLUETOOTH_FIND_RADIO_PARAMS radioParms;
    HBLUETOOTH_RADIO_FIND radioFindHandle;
    HANDLE returnValue = NULL;

    radioParms.dwSize = sizeof(radioParms);

    // Always use the first radio found. Some documentation says that only one
    // radio is supported anyway.
    radioFindHandle = ::BluetoothFindFirstRadio(&radioParms, &returnValue);

    // Returns NULL if failure.
    if (radioFindHandle) {
        HANDLE dummyHandle;

        // This is only for debug purposes. We want to know if there
        // is more than one BT radio in the system.
        if (::BluetoothFindNextRadio(radioFindHandle, &dummyHandle)) {
            QCC_DbgTrace(("BTTransport::BTAccessor::BTAccessor(): More than one BT radio found. Using first one."));
            ::CloseHandle(dummyHandle);
        }

        ::BluetoothFindRadioClose(radioFindHandle);
    } else {
        // Set to NULL as a flag for no BT radio available.
        returnValue = NULL;
    }

    return returnValue;
}

bool BTTransport::BTAccessor::GetRadioAddress()
{
    QCC_DbgTrace(("BTTransport::BTAccessor::GetRadioAddress()"));

    DWORD errCode = ERROR_DEV_NOT_EXIST;

    deviceLock.Lock(MUTEX_CONTEXT);

    if (radioHandle) {
        BLUETOOTH_RADIO_INFO radioInfo = { 0 };

        radioInfo.dwSize = sizeof(radioInfo);
        errCode = ::BluetoothGetRadioInfo(radioHandle, &radioInfo);

        if (ERROR_SUCCESS == errCode) {
            address.SetRaw(radioInfo.address.ullLong);
        }
    }

    deviceLock.Unlock(MUTEX_CONTEXT);

    return ERROR_SUCCESS == errCode;
}

void BTTransport::BTAccessor::RemoveRecord()
{
    QCC_DbgTrace(("BTTransport::BTAccessor::RemoveRecord()"));

    if (this->recordHandle && this->wsaInitialized) {
        BTH_SET_SERVICE service;
        BLOB blob;
        WSAQUERYSET registrationInfo;

        QCC_DbgPrintf(("Removing record handle 0x%08x (old record)", recordHandle));

        InitializeSetService(&registrationInfo, &blob, &service, &recordHandle);

        // The dwControlFlags parameter is reserved, and must be zero. From:
        // http://msdn.microsoft.com/en-us/library/aa362921.aspx
        INT wsaReturnValue = WSASetService(&registrationInfo, RNRSERVICE_DELETE, 0);

        if (0 != wsaReturnValue) {
            int err = WSAGetLastError();

            QCC_DbgPrintf(("WSASetService() failed error = 0x%08X", err));
        }

        recordHandle = NULL;
    }
}

void BTTransport::BTAccessor::EndPointsInit(void)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::EndPointsInit()"));

    int i = _countof(activeEndPoints) - 1;

    // This shouldn't be necessary because it should only be called at constructor time
    // but we do it anyway just to be consistent.
    deviceLock.Lock(MUTEX_CONTEXT);

    do {
        activeEndPoints[i] = NULL;
    } while (--i >= 0);

    deviceLock.Unlock(MUTEX_CONTEXT);
}

bool BTTransport::BTAccessor::EndPointsAdd(WindowsBTEndpoint* endpoint)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::EndPointsAdd(%p)", endpoint));

    bool returnValue = false;

    if (endpoint) {
        QCC_DbgPrintf(("EndPointsAdd(address = 0x%012I64X)", endpoint->GetRemoteDeviceAddress()));

        int i = _countof(activeEndPoints) - 1;

        deviceLock.Lock(MUTEX_CONTEXT);

        do {
            if (NULL == activeEndPoints[i]) {
                activeEndPoints[i] = endpoint;
                returnValue = true;
                break;
            }
        } while (--i >= 0);

        deviceLock.Unlock(MUTEX_CONTEXT);

        QCC_DbgPrintf(("EndPointsAdd(%p) into slot %d", endpoint, i));
    }

    return returnValue;
}

void BTTransport::BTAccessor::EndPointsRemove(WindowsBTEndpoint* endpoint)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::EndPointsRemove()"));

    if (endpoint) {
        QCC_DbgPrintf(("EndPointsRemove(address = 0x%012I64X, handle = %p)",
                       endpoint->GetRemoteDeviceAddress(),
                       endpoint->GetChannelHandle()));

        int i = _countof(activeEndPoints) - 1;

        deviceLock.Lock(MUTEX_CONTEXT);

        do {
            if (activeEndPoints[i] == endpoint) {
                activeEndPoints[i] = NULL;
                break;
            }
        } while (--i >= 0);

        deviceLock.Unlock(MUTEX_CONTEXT);

        QCC_DbgPrintf(("EndPointsRemove(%p) from slot %d", endpoint, i));

        L2CAP_CHANNEL_HANDLE handle = endpoint->GetChannelHandle();
        BTH_ADDR address = endpoint->GetRemoteDeviceAddress();

        // Only disconnect if the connection was completed.
        if (handle && address) {
            USER_KERNEL_MESSAGE message = { USRKRNCMD_DISCONNECT };

            message.messageData.disconnectData.channelHandle = handle;
            message.messageData.disconnectData.address = address;

            DeviceSendMessage(&message, NULL);
        }
    }
}

void BTTransport::BTAccessor::EndPointsRemoveAll(void)
{
    QCC_DbgTrace(("BTTransport::BTAccessor::EndPointsRemoveAll()"));

    int i = _countof(activeEndPoints) - 1;

    deviceLock.Lock(MUTEX_CONTEXT);

    do {
        if (NULL != this->activeEndPoints[i]) {
            // The endpoints are NOT deleted. This is because they may still be a reference
            // to them by the daemon which does the deletion. EndPointsRemoveAll() is
            // only to be called from ~BTTransport::BTAccessor().
            this->activeEndPoints[i]->OrphanEndpoint();
            this->activeEndPoints[i] = NULL;
        }
    } while (--i >= 0);

    deviceLock.Unlock(MUTEX_CONTEXT);
}

WindowsBTEndpoint* BTTransport::BTAccessor::EndPointsFind(BTH_ADDR address,
                                                          L2CAP_CHANNEL_HANDLE handle) const
{
    WindowsBTEndpoint* returnValue = NULL;
    int i = _countof(activeEndPoints) - 1;

    deviceLock.Lock(MUTEX_CONTEXT);

    do {
        if (NULL != activeEndPoints[i] &&
            activeEndPoints[i]->GetRemoteDeviceAddress() == address &&
            activeEndPoints[i]->GetChannelHandle() == handle) {
            returnValue = activeEndPoints[i];
            break;
        }
    } while (--i >= 0);

    deviceLock.Unlock(MUTEX_CONTEXT);

    return returnValue;
}

QStatus BTTransport::BTAccessor::ConnectRequestsGet(struct _KRNUSRCMD_L2CAP_EVENT* request)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("BTTransport::BTAccessor::ConnectRequestsGet()"));

    if (NULL == request) {
        status = ER_BAD_ARG_1;
        goto Error;
    }

    if (ConnectRequestsIsEmpty()) {
        status = ER_FAIL;
        goto Error;
    }

    QCC_DbgPrintf(("BTTransport::BTAccessor::ConnectRequestsGet() from index %d", connectRequestsHead));

    deviceLock.Lock(MUTEX_CONTEXT);

    *request = connectRequests[connectRequestsHead++];

    if (connectRequestsHead >= _countof(connectRequests)) {
        connectRequestsHead = 0;
    }

    if (ConnectRequestsIsEmpty()) {
        QCC_DbgPrintf(("BTTransport::BTAccessor::ConnectRequestsGet() reset l2capEvent"));
        l2capEvent.ResetEvent();
    }

    deviceLock.Unlock(MUTEX_CONTEXT);

Error:
    return status;
}

QStatus BTTransport::BTAccessor::ConnectRequestsPut(const struct _KRNUSRCMD_L2CAP_EVENT* request)
{
    QStatus status = ER_OK;

    if (NULL == request) {
        status = ER_BAD_ARG_1;
        goto Error;
    }

    QCC_DbgTrace(("BTTransport::BTAccessor::ConnectRequestsPut(address = 0x%012I64X, handle = %p)",
                  request->address, request->channelHandle));
    QCC_DbgPrintf(("BTTransport::BTAccessor::ConnectRequestsPut() into index %d", connectRequestsTail));

    deviceLock.Lock(MUTEX_CONTEXT);
    connectRequests[connectRequestsTail++] = *request;

    if (connectRequestsTail >= _countof(connectRequests)) {
        connectRequestsTail = 0;
    }

    if (connectRequestsTail == connectRequestsHead) {
        connectRequestsHead++;

        if (connectRequestsHead >= _countof(connectRequests)) {
            connectRequestsHead = 0;
        }
    }

    QCC_DbgPrintf(("BTTransport::BTAccessor::ConnectRequestsPut() set l2capEvent"));
    l2capEvent.SetEvent();

    deviceLock.Unlock(MUTEX_CONTEXT);

Error:
    return status;
}

bool BTTransport::BTAccessor::DeviceIo(void* messageIn, size_t inSize,
                                       void* messageOut, size_t outSize, size_t* returnedSize) const
{
    bool returnValue = false;

    if (returnedSize) {
        *returnedSize = 0;
    }

    if (INVALID_HANDLE_VALUE != deviceHandle) {
        DWORD bytesReturned = 0;
        OVERLAPPED overlapped = { 0 };

        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (overlapped.hEvent) {
            returnValue = ::DeviceIoControl(deviceHandle, IOCTL_ALLJOYN_MESSAGE, messageIn, inSize,
                                            messageOut, outSize, &bytesReturned, &overlapped);

            // If the operation completes successfully, the return value is nonzero.
            // If the operation fails or is pending, the return value is zero.
            // Since this is implemented as an overlapped operation "pending" is the expected result.
            if (!returnValue) {
                int lastError = ::GetLastError();

                if (ERROR_IO_PENDING == lastError) {
                    returnValue = GetOverlappedResult(deviceHandle, &overlapped, &bytesReturned, TRUE);
                }
            }

            CloseHandle(overlapped.hEvent);

            if (returnedSize) {
                *returnedSize = (size_t) bytesReturned;
            }
        }
    }

    return returnValue;
}

const char* ChannelStateText(L2CAP_CHANNEL_STATE_TYPE state)
{
#define CASE(_state) case _state: return # _state

    switch (state) {
        CASE(CHAN_STATE_NONE);
        CASE(CHAN_STATE_NONE_PENDING);
        CASE(CHAN_STATE_READ_READY);
        CASE(CHAN_STATE_L2CAP_EVENT);
        CASE(CHAN_STATE_ACCEPT_COMPLETE);
        CASE(CHAN_STATE_CONNECT_COMPLETE);
        CASE(CHAN_STATE_CLOSED);

    default:
        return "<unknown>";
    }
#undef CASE
}

void BTTransport::BTAccessor::DebugDumpKernelState(void) const
{
    USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_GET_STATE };
    USER_KERNEL_MESSAGE messageOut = { USRKRNCMD_GET_STATE };
    size_t returnedSize = 0;

    bool success = DeviceIo(&messageIn, sizeof(messageIn), &messageOut, sizeof(messageOut), &returnedSize);

    QCC_DbgPrintf(("Get Kernel State:DeviceIo: %s", success ? "Success." : "Failure!"));

    if (success) {
        QCC_DbgPrintf(("Get Kernel State: %s.", QCC_StatusText(messageOut.commandStatus.status)));

        if (ER_OK == messageOut.commandStatus.status) {
            QCC_DbgPrintf(("    eventHandle = %p", messageOut.messageData.state.eventHandle));
            QCC_DbgPrintf(("    psm = 0x%04X", messageOut.messageData.state.psm));
            QCC_DbgPrintf(("    l2CapServerHandle = %p", messageOut.messageData.state.l2CapServerHandle));

            const int maxIndex = _countof(messageOut.messageData.state.channelState) - 1;
            int i;

            for (i = 0; i <= maxIndex; i++) {
                L2CAP_CHANNEL_STATE* channel = &messageOut.messageData.state.channelState[i];

                QCC_DbgPrintf(("    Channel %d:", i));
                QCC_DbgPrintf(("        status: %s", QCC_StatusText(channel->status)));

                if (ER_SOCK_OTHER_END_CLOSED != channel->status || CHAN_STATE_NONE != channel->stateType) {
                    QCC_DbgPrintf(("        ntStatus: 0x%08X", channel->ntStatus));
                    QCC_DbgPrintf(("        state: %s", ChannelStateText(channel->stateType)));
                    QCC_DbgPrintf(("        address: 0x%012I64X", channel->address));
                    QCC_DbgPrintf(("        bytesInBuffer: %Iu", channel->bytesInBuffer));
                    QCC_DbgPrintf(("        channelHandle: %p", channel->channelHandle));
                    QCC_DbgPrintf(("        incomingMtus: %d", channel->incomingMtus));
                    QCC_DbgPrintf(("        outgoingMtus: %d", channel->outgoingMtus));
                    QCC_DbgPrintf(("        channelFlags: 0x%08X", channel->channelFlags));
                }
            }
        }
    }
}

void BTTransport::BTAccessor::SetRadioHandle(HANDLE newHandle)
{
    deviceLock.Lock(MUTEX_CONTEXT);

    if (radioHandle) {
        ::CloseHandle(radioHandle);
    }

    radioHandle = newHandle;
    deviceLock.Unlock(MUTEX_CONTEXT);
}

qcc::ThreadReturn STDCALL BTTransport::BTAccessor::AdapterChangeThread::Run(void* arg)
{
    QCC_DbgTrace(("AdapterChangeThread()"));

    bool lastIsStarted = false;

    do {
        const uint32_t adapterCheckPeriodInMilliseconds = 2000;
        bool currentIsStarted = btAccessor.IsStarted();

        if (currentIsStarted) {
            HANDLE tempRadioHandle = btAccessor.GetRadioHandle();

            if (btAccessor.BluetoothIsAvailable()) {
                // If Bluetooth was previously available and still is then there is no change.
                if (tempRadioHandle) {
                    // Just close the new handle we got.
                    ::CloseHandle(tempRadioHandle);
                } else {
                    // Bluetooth was previously available and now it is not.
                    btAccessor.KernelDisconnect(false);
                }
            } else {
                // If Bluetooth was not available and now it is then make the change.
                if (tempRadioHandle) {
                    btAccessor.KernelConnect(tempRadioHandle);
                }
            }
        } else {
            // Handle the case where the Bluetooth system is not in the start state.

            // Were we in the started state and just changed to the stop state?
            if (lastIsStarted && btAccessor.BluetoothIsAvailable()) {
                btAccessor.KernelDisconnect(true);
            }
        }

        lastIsStarted = currentIsStarted;

        // Wait. And if we return because of an alert then reset the event.
        if (ER_ALERTED_THREAD == Event::Wait(GetStopEvent(), adapterCheckPeriodInMilliseconds)) {
            GetStopEvent().ResetEvent();
        }
    } while (!IsStopping());

    // Bluetooth was previously available and running then shut it down.
    if (btAccessor.BluetoothIsAvailable() && lastIsStarted) {
        btAccessor.KernelDisconnect(true);
    }

    return 0;
}

} // namespace ajn
