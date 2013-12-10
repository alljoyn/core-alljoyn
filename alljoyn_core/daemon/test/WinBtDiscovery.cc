/**
 * @file
 * BtDiscovery.cpp : Discovers and prints all the Bluetooth services on all the currently visible
 * devices. If the driver is installed this will also dump the state of the AllJoyn driver.
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

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <errno.h>
#include <fcntl.h>
#include <WinNT.h>
#include <stdlib.h>
#include <strsafe.h>
#include <conio.h>
#pragma warning( push )
#pragma warning( disable : 4068)
#include <BluetoothAPIs.h>
#pragma warning( pop )
#include <Winsock2.h>
#include <Ws2bth.h>
#include <Setupapi.h>
#include <WinIoCtl.h>

#define INITGUID
#include <bthdef.h>
// This will create an instance of the GUID WINDOWS_BLUETOOTH_DEVICE_INTERFACE in this .cc file.
#include "../bt_windows/UserKernelComm.h"
#undef INITGUID

#include <bthsdpdef.h>
#include <BluetoothAPIs.h>

// CHAR is the type used by BTH_DEVICE_INFO.
CHAR deviceSearchSubString[MAX_PATH + 1];

PSP_DEVICE_INTERFACE_DETAIL_DATA GetDeviceInterfaceDetailData(void)
{
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

HANDLE GetDeviceHandle(void)
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;

    deviceInterfaceDetailData = GetDeviceInterfaceDetailData();
    if (deviceInterfaceDetailData == NULL) {
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

Error:
    return deviceHandle;
}

bool DeviceIo(void* messageIn, size_t inSize,
              void* messageOut, size_t outSize, size_t* returnedSize)
{
    HANDLE deviceHandle = GetDeviceHandle();
    bool returnValue = false;

    *returnedSize = 0;

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

        CloseHandle(deviceHandle);
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

void DumpKernelState(void)
{
    USER_KERNEL_MESSAGE messageIn = { USRKRNCMD_GET_STATE };
    USER_KERNEL_MESSAGE messageOut = { USRKRNCMD_GET_STATE };
    size_t returnedSize = 0;

    messageIn.version = DRIVER_VERSION;
    messageIn.is64Bit = IS_64BIT;

    _tprintf_s(TEXT("Expect kernel version: %d %s.\n"),
               DRIVER_VERSION, IS_64BIT ? TEXT("64-bit") : TEXT("32-bit"));
    bool success = DeviceIo(&messageIn, sizeof(messageIn), &messageOut, sizeof(messageOut), &returnedSize);

    _tprintf_s(TEXT("Get Kernel State: DeviceIo was a %s\n"), success ? TEXT("success.") : TEXT("failure!"));

    if (success) {
        _tprintf_s(TEXT("Get Kernel State: %S.\n"), QCC_StatusText(messageOut.commandStatus.status));

        if (ER_OK == messageOut.commandStatus.status) {
            _tprintf_s(TEXT("Kernel version: %d %s\n"), -messageOut.version,
                       messageOut.is64Bit ? TEXT("64-bit") : TEXT("32-bit"));

            if (messageOut.version == -DRIVER_VERSION && IS_64BIT == messageOut.is64Bit) {
                _tprintf_s(TEXT("    eventHandle = %p\n"), messageOut.messageData.state.eventHandle);
                _tprintf_s(TEXT("    psm = 0x%04X\n"), messageOut.messageData.state.psm);
                _tprintf_s(TEXT("    l2CapServerHandle = %p\n\n"), messageOut.messageData.state.l2CapServerHandle);

                const int maxIndex = _countof(messageOut.messageData.state.channelState) - 1;
                int i;

                for (i = 0; i <= maxIndex; i++) {
                    L2CAP_CHANNEL_STATE* channel = &messageOut.messageData.state.channelState[i];

                    _tprintf_s(TEXT("    Channel %d:\n"), i);
                    _tprintf_s(TEXT("        status: %S\n"), QCC_StatusText(channel->status));

                    if (ER_SOCK_OTHER_END_CLOSED != channel->status || CHAN_STATE_NONE != channel->stateType) {
                        _tprintf_s(TEXT("        ntStatus: 0x%08X\n"), channel->ntStatus);
                        _tprintf_s(TEXT("        state: %S\n"), ChannelStateText(channel->stateType));
                        _tprintf_s(TEXT("        address: 0x%012I64X\n"), channel->address);
                        _tprintf_s(TEXT("        bytesInBuffer: %Iu\n"), channel->bytesInBuffer);
                        _tprintf_s(TEXT("        channelHandle: %p\n"), channel->channelHandle);
                        _tprintf_s(TEXT("        incomingMtus: %d\n"), channel->incomingMtus);
                        _tprintf_s(TEXT("        outgoingMtus: %d\n"), channel->outgoingMtus);
                        _tprintf_s(TEXT("        channelFlags: 0x%08X\n"), channel->channelFlags);
                    }
                }
            }
        }
    }
}


HANDLE GetRadioHandle()
{
    static HANDLE returnValue = NULL;

    if (0 == returnValue) {
        BLUETOOTH_FIND_RADIO_PARAMS radioParms;
        HBLUETOOTH_RADIO_FIND radioFindHandle;

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
                _tprintf_s(TEXT("More than one Bluetooth radio found. Using first one only.\n"));
                ::CloseHandle(dummyHandle);
            }

            ::BluetoothFindRadioClose(radioFindHandle);
        } else {
            // Set to NULL as a flag for no BT radio available.
            returnValue = NULL;
        }
    }

    return returnValue;
}


#define ReportWsaError() ReportWsaErrorPrivate(__LINE__)

void ReportWsaErrorPrivate(int line)
{
    int err = WSAGetLastError();

    _tprintf_s(TEXT("Fatal error from line %d.\n"), line);

    switch (err) {
    case WSA_NOT_ENOUGH_MEMORY:
        _tprintf_s(TEXT("Not enough memory error 0x%X\n"), err);
        break;

    case WSAEINVAL:
        _tprintf_s(TEXT("Invalid arg error 0x%X\n"), err);
        break;

    case WSANO_DATA:
        _tprintf_s(
            TEXT("The name was found in the database but no data matching the given restrictions\
                 was located. error 0x%x\n"),
            err);
        break;

    case WSANOTINITIALISED:
        _tprintf_s(TEXT("The WS2_32.DLL has not been initialized. Error 0x%X\n"), err);
        break;

    case WSASERVICE_NOT_FOUND:
        _tprintf_s(TEXT("No such service is known. 0x%X\n"), err);
        break;

    default:
        _tprintf_s(TEXT("Unrecognized error 0x%X\n"), err);
        break;
    }
}

void BlueToothPromoteUuid(GUID* destination, WORD shortUuid)
{
    // 00000000-0000-1000-8000-00805F9B34FB
    static const GUID baseUuid = {
        0, 0, 0x1000,
        { 0x80, 0x0, 00, 0x80, 0x5F, 0x9B, 0x34, 0xFB }
    };

    *destination = baseUuid;
    destination->Data1 = shortUuid;
}

bool LookupNextRecord(HANDLE lookupHandle, DWORD* bufferLength, WSAQUERYSET** querySetBuffer)
{
    const DWORD controlFlags = LUP_RETURN_ALL;
    bool returnValue = true;
    DWORD wsaSpecifiedBufferLength = *bufferLength;
    int err = WSALookupServiceNext(lookupHandle, controlFlags, &wsaSpecifiedBufferLength, *querySetBuffer);

    if (SOCKET_ERROR == err) {
        err = WSAGetLastError();
        returnValue = false;

        // Was the buffer too small?
        if (WSAEFAULT == err) {
            // Yes, the buffer was too small. Allocate one of the suggested size.
            ::free(*querySetBuffer);

            *bufferLength = wsaSpecifiedBufferLength;
            *querySetBuffer = (WSAQUERYSET*)::malloc(wsaSpecifiedBufferLength);

            // Did we get the buffer we requested?
            if (*querySetBuffer) {
                (*querySetBuffer)->dwSize = sizeof(**querySetBuffer);

                // Try looking up the next record with the larger buffer.
                err = WSALookupServiceNext(lookupHandle, controlFlags, &wsaSpecifiedBufferLength, *querySetBuffer);

                if (SOCKET_ERROR != err) {
                    returnValue = true;
                } else {
                    err = WSAGetLastError();

                    // Was the error something other than there were no more records?
                    if (WSA_E_NO_MORE != err) {
                        _tprintf_s(TEXT("WSA error 0x%x when looking up next SDP record.\n"), err);
                    }
                }
            } else {
                _tprintf_s(TEXT("Out of memory when looking up next SDP record.\n"), err);
                *bufferLength = 0;
            }
        }
    }

    return returnValue;
}

void PrintElementData(SDP_ELEMENT_DATA* data, BLOB* blob)
{
    static int extraTabs = 0;
    TCHAR tabString[256];
    TCHAR* typeString = 0;
    char dataString[256];

    int tab;
    TCHAR* tabPointer = tabString;

    // Always have two tabs, if recurision then more.
    for (tab = 0; tab < 2 + extraTabs && tabPointer - tabString < _countof(tabString) - 1; tab++) {
        *tabPointer++ = TEXT('\t');
    }

    *tabPointer = 0;    // nul terminate.

    extraTabs++;

    dataString[0] = 0;

    switch (data->type) {
    case SDP_TYPE_NIL:
        typeString = TEXT("NIL");
        break;

    case SDP_TYPE_UINT:
        typeString = TEXT("UINT");
        break;

    case SDP_TYPE_INT:
        typeString = TEXT("INT");
        break;

    case SDP_TYPE_UUID:
        typeString = TEXT("UUID");
        break;

    case SDP_TYPE_STRING:
        typeString = TEXT("STRING");
        memcpy_s(dataString, sizeof(dataString), data->data.string.value, data->data.string.length);

        if (data->data.string.length < sizeof(dataString)) {
            dataString[data->data.string.length] = 0;
        } else {
            dataString[sizeof(dataString) - 1] = 0;
        }
        break;

    case SDP_TYPE_BOOLEAN:
        typeString = TEXT("BOOLEAN");
        break;

    case SDP_TYPE_SEQUENCE:
        typeString = TEXT("SEQUENCE");
        break;

    case SDP_TYPE_ALTERNATIVE:
        typeString = TEXT("ALTERNATIVE");
        break;

    case SDP_TYPE_URL:
        typeString = TEXT("URL");
        break;

    case SDP_TYPE_CONTAINER:
        typeString = TEXT("CONTAINER");
        break;

    default:
        typeString = TEXT("Unknown");
        break;
    }

    _tprintf_s(TEXT("%sType: 0x%X = %s\n"), tabString, data->type, typeString);

    switch (data->specificType) {
    case SDP_ST_NONE:
        typeString = TEXT("NONE");
        break;

    case SDP_ST_UINT8:
        typeString = TEXT("UINT8");
        sprintf_s(dataString, _countof(dataString), "0x%02X", data->data.uint8);
        break;

    case SDP_ST_UINT16:
        typeString = TEXT("UINT16");
        sprintf_s(dataString, _countof(dataString), "0x%04X", data->data.uint16);
        break;

    case SDP_ST_UINT32:
        typeString = TEXT("UINT32");
        sprintf_s(dataString, _countof(dataString), "0x%08X", data->data.uint32);
        break;

    case SDP_ST_UINT64:
        typeString = TEXT("UINT64");
        sprintf_s(dataString, _countof(dataString),  "0x%08X%08X", (DWORD)(data->data.int64 >> 32),
                  (DWORD)(data->data.int64 & 0xFFFFFFFFF));
        break;

    case SDP_ST_UINT128:
        typeString = TEXT("UINT128");
        break;

    case SDP_ST_INT8:
        typeString = TEXT("INT8");
        sprintf_s(dataString, _countof(dataString), "0x%02X", data->data.int8);
        break;

    case SDP_ST_INT16:
        typeString = TEXT("INT16");
        sprintf_s(dataString, _countof(dataString), "0x%04X", data->data.int16);
        break;

    case SDP_ST_INT32:
        // case SDP_ST_UUID32: Same value as INT32.
        typeString = TEXT("INT32/UUID32");
        sprintf_s(dataString, _countof(dataString), "0x%08X", data->data.int32);
        break;

    case SDP_ST_INT64:
        typeString = TEXT("INT64");
        sprintf_s(dataString, _countof(dataString), "0x%08X%08X", (DWORD)(data->data.int64 >> 32),
                  (DWORD)(data->data.int64 & 0xFFFFFFFFF));
        break;

    case SDP_ST_INT128:
        typeString = TEXT("INT128");
        break;

    case SDP_ST_UUID16:
        typeString = TEXT("UUID16");
        sprintf_s(dataString, _countof(dataString), "0x%04X", data->data.uuid16);
        break;

    case SDP_ST_UUID128:
        typeString = TEXT("UUID128");
        // Format is "{00000000-1c25-481f-9dfb-59193d238280}"
        sprintf_s(dataString,
                  _countof(dataString),
                  "{%08X, %04X, %04X, %02X%02X, %02X%02X%02X%02X%02X%02X}",
                  data->data.uuid128.Data1,
                  data->data.uuid128.Data2,
                  data->data.uuid128.Data3,
                  data->data.uuid128.Data4[0],
                  data->data.uuid128.Data4[1],
                  data->data.uuid128.Data4[2],
                  data->data.uuid128.Data4[3],
                  data->data.uuid128.Data4[4],
                  data->data.uuid128.Data4[5],
                  data->data.uuid128.Data4[6],
                  data->data.uuid128.Data4[7]);
        break;

    default:
        typeString = TEXT("Unknown type.");
        break;
    }

    _tprintf_s(TEXT("%sSpecific Type: 0x%X = %s\n"), tabString, data->specificType, typeString);

    if (SDP_TYPE_SEQUENCE == data->type) {
        SDP_ELEMENT_DATA sequenceDataElement;
        HBLUETOOTH_CONTAINER_ELEMENT element = NULL;
        ULONG sequenceResult;

        do {
            sequenceResult =
                ::BluetoothSdpGetContainerElementData(data->data.sequence.value,
                                                      data->data.sequence.length,
                                                      &element,
                                                      &sequenceDataElement);

            if (ERROR_SUCCESS == sequenceResult) {
                PrintElementData(&sequenceDataElement, blob);
            }
        } while (ERROR_SUCCESS == sequenceResult);
    } else {
        if (dataString[0]) {
            _tprintf_s(TEXT("%sValue: '%S'\n"), tabString, dataString);
        }
    }

    extraTabs--;
}

BOOL CALLBACK EnumerateSdpRecordCallback(ULONG uAttribId, LPBYTE pValueStream, ULONG cbStreamSize,
                                         LPVOID pvParam)
{
    BLOB* blob = (BLOB*)pvParam;
    SDP_ELEMENT_DATA data;

    memset(&data, 0, sizeof(data));

    DWORD status = BluetoothSdpGetAttributeValue(blob->pBlobData, blob->cbSize, (USHORT)uAttribId, &data);

    if (ERROR_SUCCESS == status) {
        _tprintf_s(_TEXT("\tGot data for attribute 0x%04X\n"), uAttribId);
        PrintElementData(&data, blob);
    } else {
        _tprintf_s(TEXT("\tError getting data for attribute 0x%04X\n"), uAttribId);
    }

    return TRUE;
}

/**
 * Convert this SOCKET_ADDRESS to a '\0' terminated string and return a static buffer to the result.
 *
 * @param address   The address to output.
 *
 * @return  Pointer to static buffer than contains the '\0' terminated string or NULL if an error occurred.
 */
TCHAR* GetSocketAddressAsString(const SOCKET_ADDRESS* address)
{
    static TCHAR addressAsString[256];
    int err = -1;

    if (address) {
        DWORD addressStringLength = _countof(addressAsString);

        err = WSAAddressToString(address->lpSockaddr, address->iSockaddrLength, 0, addressAsString,
                                 &addressStringLength);
    }

    return 0 == err ? addressAsString : NULL;
}

bool ReportL2CapServices(const SOCKET_ADDRESS* local, const SOCKET_ADDRESS* remote,
                         BTH_DEVICE_INFO* deviceInfo)
{
    bool returnValue = false;

    _tprintf_s(TEXT("Device: %S\n"), deviceInfo->name);

    TCHAR* addressAsString = GetSocketAddressAsString(local);

    if (addressAsString) {
        _tprintf_s(TEXT("\tLocal address: %s\n"), addressAsString);
    }

    // The remote address must be obtained AFTER the local because
    // the remote version of addressAsString is later used in querySet.
    addressAsString = GetSocketAddressAsString(remote);

    if (addressAsString) {
        _tprintf_s(TEXT("Remote address: %s\n"), addressAsString);
    }

    GUID guidForL2CapService;

    // The L2CP UUID is a promoted 16-bit class.
    BlueToothPromoteUuid(&guidForL2CapService, L2CAP_PROTOCOL_UUID16);

    HANDLE lookupHandle;
    WSAQUERYSET querySet = { 0 };

    querySet.dwSize = sizeof(querySet);
    querySet.lpServiceClassId = &guidForL2CapService;
    querySet.lpszContext = addressAsString;

    querySet.dwNameSpace = NS_BTH;
    querySet.dwNumberOfCsAddrs = 0;

    DWORD controlFlags = LUP_FLUSHCACHE;

    if (0 == WSALookupServiceBegin(&querySet, controlFlags, &lookupHandle)) {
        if (querySet.lpServiceClassId) {
            GUID service = *querySet.lpServiceClassId;

            _tprintf_s(
                TEXT("\tService class: {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n"),
                service.Data1, service.Data2, service.Data3,
                service.Data4[0], service.Data4[1], service.Data4[2], service.Data4[3],
                service.Data4[4], service.Data4[5], service.Data4[6], service.Data4[7]);
        }

        DWORD bufferLength = 1024;  // Just something moderately large.
        WSAQUERYSET* querySetBuffer = (WSAQUERYSET*)malloc(bufferLength);

        if (querySetBuffer) {
            bool keepLooking;

            do {
                keepLooking = LookupNextRecord(lookupHandle, &bufferLength, &querySetBuffer);

                // If successful.
                if (keepLooking) {
                    static TCHAR nameSeparator[] = TEXT("\t-----------\n");

                    returnValue = true;
                    _tprintf_s(TEXT("\n%s"), nameSeparator);

                    if (querySetBuffer->lpszServiceInstanceName) {
                        _tprintf_s(TEXT("\tName: %s\n"), querySetBuffer->lpszServiceInstanceName);
                    }

                    if (querySetBuffer->lpszComment) {
                        _tprintf_s(TEXT("\tComment: %s\n"), querySetBuffer->lpszComment);
                    }

                    _tprintf_s(nameSeparator);

                    if (querySetBuffer->lpcsaBuffer) {
                        CSADDR_INFO serviceAddress = *querySetBuffer->lpcsaBuffer;

                        _tprintf_s(TEXT("\tProtocol: %d\n"), serviceAddress.iProtocol);
                        _tprintf_s(TEXT("\tSocket Type: %d\n"), serviceAddress.iSocketType);

                        addressAsString = GetSocketAddressAsString(&querySetBuffer->lpcsaBuffer->RemoteAddr);

                        if (addressAsString) {
                            _tprintf_s(TEXT("\tRemote address: %s\n"), addressAsString);
                        }

                        addressAsString = GetSocketAddressAsString(&querySetBuffer->lpcsaBuffer->LocalAddr);

                        if (addressAsString) {
                            _tprintf_s(TEXT("\tLocal address: %s\n"), addressAsString);
                        }

                        _tprintf_s(TEXT("\n"));
                    }

                    if (querySetBuffer->lpBlob && querySetBuffer->lpBlob->cbSize > 0) {
                        BluetoothSdpEnumAttributes(querySetBuffer->lpBlob->pBlobData,
                                                   querySetBuffer->lpBlob->cbSize,
                                                   EnumerateSdpRecordCallback,
                                                   querySetBuffer->lpBlob);
                    }
                }
            } while (keepLooking);

            free(querySetBuffer);
        }

        WSALookupServiceEnd(lookupHandle);
    } else {
        int wsaError = WSAGetLastError();

        _tprintf_s(TEXT("WSALookupServiceBegin() returned an error: "));

        switch (wsaError) {
        case WSA_NOT_ENOUGH_MEMORY:
            _tprintf_s(TEXT("WSA_NOT_ENOUGH_MEMORY.\n"));
            break;

        case WSAEINVAL:
            _tprintf_s(TEXT("WSAEINVAL.\n"));
            break;

        case WSANO_DATA:
            _tprintf_s(TEXT("WSANO_DATA.\n"));
            break;

        case WSANOTINITIALISED:
            _tprintf_s(TEXT("WSANOTINITIALISED.\n"));
            break;

        case WSASERVICE_NOT_FOUND:
            _tprintf_s(TEXT("WSASERVICE_NOT_FOUND (no such service is known).\n"));
            break;

        default:
            _tprintf_s(TEXT("wsaError=%#x.\n"), wsaError);
            break;
        }
    }

    return returnValue;
}

bool BluetoothExists(void)
{
    return GetRadioHandle() != 0;
}

HANDLE BeginDeviceInquiry(void)
{
    HANDLE returnValue = 0;
    WSAQUERYSET querySet = { 0 };

    querySet.dwSize = sizeof(querySet);
    querySet.dwNameSpace = NS_BTH;

    if (0 != WSALookupServiceBegin(&querySet, LUP_CONTAINERS | LUP_FLUSHCACHE, &returnValue)) {
        // If not successful make sure the returned handle is 0.
        returnValue = 0;
    }

    return returnValue;
}

void ReportDeviceAndServiceInfo(void)
{
    if (BluetoothExists()) {
        HANDLE lookupHandle = BeginDeviceInquiry();

        if (0 != lookupHandle) {
            bool keepLooking = true;
            DWORD bufferLength = 1024;  // Just something moderately large.
            WSAQUERYSET* querySetBuffer = (WSAQUERYSET*)malloc(bufferLength);

            if (querySetBuffer) {
                do {
                    keepLooking = LookupNextRecord(lookupHandle, &bufferLength, &querySetBuffer);

                    if (keepLooking && (SOCKADDR_BTH*)querySetBuffer->lpcsaBuffer) {
                        BTH_DEVICE_INFO* deviceInfo = (BTH_DEVICE_INFO*)querySetBuffer->lpBlob->pBlobData;

                        if (strstr(deviceInfo->name, deviceSearchSubString)) {
                            const SOCKET_ADDRESS* local = &querySetBuffer->lpcsaBuffer->LocalAddr;
                            const SOCKET_ADDRESS* remote = &querySetBuffer->lpcsaBuffer->RemoteAddr;

                            ReportL2CapServices(local, remote, deviceInfo);
                        }
                    }
                } while (keepLooking);

                free(querySetBuffer);
            }

            WSALookupServiceEnd(lookupHandle);
        } else {
            _tprintf_s(TEXT("No devices found.\n"));
        }
    } else {
        _tprintf_s(TEXT("No local Bluetooth radio found.\n"));
    }
}

TCHAR* GetManufacturerName(USHORT manufacture)
{
    TCHAR* returnValue = TEXT("Unknown");

    switch (manufacture) {
    case BTH_MFG_ERICSSON:
        returnValue = TEXT("ERICSSON");
        break;

    case BTH_MFG_NOKIA:
        returnValue = TEXT("NOKIA");
        break;

    case BTH_MFG_INTEL:
        returnValue = TEXT("INTEL");
        break;

    case BTH_MFG_IBM:
        returnValue = TEXT("IBM");
        break;

    case BTH_MFG_TOSHIBA:
        returnValue = TEXT("TOSHIBA");
        break;

    case BTH_MFG_3COM:
        returnValue = TEXT("3COM");
        break;

    case BTH_MFG_MICROSOFT:
        returnValue = TEXT("MICROSOFT");
        break;

    case BTH_MFG_LUCENT:
        returnValue = TEXT("LUCENT");
        break;

    case BTH_MFG_MOTOROLA:
        returnValue = TEXT("MOTOROLA");
        break;

    case BTH_MFG_INFINEON:
        returnValue = TEXT("INFINEON");
        break;

    case BTH_MFG_CSR:
        returnValue = TEXT("CSR");
        break;

    case BTH_MFG_SILICONWAVE:
        returnValue = TEXT("SILICONWAVE");
        break;

    case BTH_MFG_DIGIANSWER:
        returnValue = TEXT("DIGIANSWER");
        break;

    case BTH_MFG_TI:
        returnValue = TEXT("TI");
        break;

    case BTH_MFG_PARTHUS:
        returnValue = TEXT("PARTHUS");
        break;

    case BTH_MFG_BROADCOM:
        returnValue = TEXT("BROADCOM");
        break;

    case BTH_MFG_MITEL:
        returnValue = TEXT("MITEL");
        break;

    case BTH_MFG_WIDCOMM:
        returnValue = TEXT("WIDCOMM");
        break;

    case BTH_MFG_ZEEVO:
        returnValue = TEXT("ZEEVO");
        break;

    case BTH_MFG_ATMEL:
        returnValue = TEXT("ATMEL");
        break;

    case BTH_MFG_MITSIBUSHI:
        returnValue = TEXT("MITSIBUSHI");
        break;

    case BTH_MFG_RTX_TELECOM:
        returnValue = TEXT("TELECOM");
        break;

    case BTH_MFG_KC_TECHNOLOGY:
        returnValue = TEXT("TECHNOLOGY");
        break;

    case BTH_MFG_NEWLOGIC:
        returnValue = TEXT("NEWLOGIC");
        break;

    case BTH_MFG_TRANSILICA:
        returnValue = TEXT("TRANSILICA");
        break;

    case BTH_MFG_ROHDE_SCHWARZ:
        returnValue = TEXT("SCHWARZ");
        break;

    case BTH_MFG_TTPCOM:
        returnValue = TEXT("TTPCOM");
        break;

    case BTH_MFG_SIGNIA:
        returnValue = TEXT("SIGNIA");
        break;

    case BTH_MFG_CONEXANT:
        returnValue = TEXT("CONEXANT");
        break;

    case BTH_MFG_QUALCOMM:
        // Funky text generation to get around aggressive copyright scan.
        returnValue = TEXT("QUAL") TEXT("COMM");
        break;

    case BTH_MFG_INVENTEL:
        returnValue = TEXT("INVENTEL");
        break;

    case BTH_MFG_AVM_BERLIN:
        returnValue = TEXT("AVM_BERLIN");
        break;

    case BTH_MFG_BANDSPEED:
        returnValue = TEXT("BANDSPEED");
        break;

    case BTH_MFG_MANSELLA:
        returnValue = TEXT("MANSELLA");
        break;

    case BTH_MFG_NEC:
        returnValue = TEXT("NEC");
        break;

    case BTH_MFG_WAVEPLUS_TECHNOLOGY_CO:
        returnValue = TEXT("WAVEPLUS_TECHNOLOGY_CO");
        break;

    case BTH_MFG_ALCATEL:
        returnValue = TEXT("ALCATEL");
        break;

    case BTH_MFG_PHILIPS_SEMICONDUCTOR:
        returnValue = TEXT("PHILIPS_SEMICONDUCTOR");
        break;

    case BTH_MFG_C_TECHNOLOGIES:
        returnValue = TEXT("C_TECHNOLOGIES");
        break;

    case BTH_MFG_OPEN_INTERFACE:
        returnValue = TEXT("OPEN_INTERFACE");
        break;

    case BTH_MFG_RF_MICRO_DEVICES:
        returnValue = TEXT("MICRO_DEVICES");
        break;

    case BTH_MFG_HITACHI:
        returnValue = TEXT("HITACHI");
        break;

    case BTH_MFG_SYMBOL_TECHNOLOGIES:
        returnValue = TEXT("SYMBOL_TECHNOLOGIES");
        break;

    case BTH_MFG_TENOVIS:
        returnValue = TEXT("TENOVIS");
        break;

    case BTH_MFG_MACRONIX_INTERNATIONAL:
        returnValue = TEXT("MACRONIX_INTERNATIONAL");
        break;

    case BTH_MFG_INTERNAL_USE:
        returnValue = TEXT("INTERNAL_USE");
        break;
    }

    return returnValue;
}

void ReportHostInfo()
{
    HANDLE radioHandle = GetRadioHandle();

    if (radioHandle) {
        BLUETOOTH_RADIO_INFO radioInfo = { 0 };

        radioInfo.dwSize = sizeof(radioInfo);

        DWORD errCode = BluetoothGetRadioInfo(radioHandle, &radioInfo);

        if (ERROR_SUCCESS == errCode) {
            _tprintf_s(TEXT("Local host info:\n"));
            _tprintf_s(TEXT("\tRadio name: %s\n"), radioInfo.szName);
            _tprintf_s(TEXT("\tAddress: (%02X:%02X:%02X:%02X:%02X:%02X)\n"),
                       radioInfo.address.rgBytes[5],
                       radioInfo.address.rgBytes[4],
                       radioInfo.address.rgBytes[3],
                       radioInfo.address.rgBytes[2],
                       radioInfo.address.rgBytes[1],
                       radioInfo.address.rgBytes[0]);
            _tprintf_s(TEXT("\tDevice class: 0x%lX\n"), radioInfo.ulClassofDevice);
            _tprintf_s(TEXT("\tManufacturer: 0x%X (%s)\n"),
                       radioInfo.manufacturer,
                       GetManufacturerName(radioInfo.manufacturer));
            _tprintf_s(TEXT("\tSubversion: 0x%X\n"), radioInfo.lmpSubversion);
        } else {
            _tprintf_s(TEXT("BluetoothGetRadioInfo() failed with error 0x%X.\n"), errCode);
        }
    } else {
        _tprintf_s(TEXT("No Bluetooth radio found.\n"));
    }
}

bool Startup(void)
{
    bool returnValue = false;
    WORD versionRequested;
    WSADATA wsaData;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    versionRequested = MAKEWORD(2, 2);

    if (0 == WSAStartup(versionRequested, &wsaData)) {
        if (2 == LOBYTE(wsaData.wVersion) && 2 == HIBYTE(wsaData.wVersion)) {
            returnValue = true;
        } else {
            WSACleanup();
        }
    }

    return returnValue;
}

void Shutdown(void)
{
    WSACleanup();
}

void Usage(_TCHAR* arg0)
{
    _tprintf_s(TEXT("Usage: "));

    if (arg0) {
        _tprintf_s(TEXT("%s "), arg0);
    } else {
        _tprintf_s(TEXT("WinBtDiscovery "));
    }

    _tprintf_s(TEXT("[-d] [-h] [-k]\n\n"));
    _tprintf_s(TEXT("Options:\n"));
    _tprintf_s(TEXT("   -d = Do discovery of all visible Bluetooth devices. Default is true.\n"));
    _tprintf_s(TEXT("   -d <name> = Do discovery only on device names that contains <name>.\n      <name> must not begin with '-'.\n"));
    _tprintf_s(TEXT("   -h = Display the host information. Default is true.\n"));
    _tprintf_s(TEXT("   -k = Show kernel driver state. Default is true.\n"));
    _tprintf_s(TEXT("If one or more flags is set then the default for all others is set to false."));

    exit(EXIT_FAILURE);
}

bool doKernelDump = true;
bool doDiscovery = true;
bool doHostInfo = true;

void ParseArgs(int argc, _TCHAR* argv[])
{
    int i;

    // If no arguments then use the defaults of true.
    for (i = 1; i < argc; i++) {
        // We have arguments set everything to false then pick and choose.
        doKernelDump = doDiscovery = doHostInfo = false;

        _TCHAR c = argv[i][0];

        if (TEXT('-') != c && TEXT('/') != c) {
            Usage(argv[0]);
        }

        switch (argv[i][1]) {
        case TEXT('d'):
            doDiscovery = true;

            if (i + 1 < argc && argv[i + 1][0] != TEXT('-')) {
                i++;

                C_ASSERT(sizeof(**argv) == 2);
                C_ASSERT(sizeof(deviceSearchSubString[0]) == 1);
                sprintf_s(deviceSearchSubString, _countof(deviceSearchSubString), "%S", argv[i]);
            }
            break;

        case TEXT('h'):
            doHostInfo = true;
            break;

        case TEXT('k'):
            doKernelDump = true;
            break;

        default:
            Usage(argv[0]);
            break;
        }
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    int returnValue = EXIT_FAILURE;

    ParseArgs(argc, argv);

    if (doKernelDump) {
        DumpKernelState();
        returnValue = EXIT_SUCCESS;
    }

    returnValue = EXIT_FAILURE;

    if ((doHostInfo || doDiscovery) && Startup()) {
        if (BluetoothExists()) {
            if (doHostInfo) {
                ReportHostInfo();
            }

            if (doDiscovery) {
                ReportDeviceAndServiceInfo();
            }
        } else {
            _tprintf_s(TEXT("No Bluetooth radio found."));
        }

        Shutdown();
        returnValue = EXIT_SUCCESS;
    }

    return returnValue;
}
