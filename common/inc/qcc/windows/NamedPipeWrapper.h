/**
 * @file
 *
 * Windows Named Pipe Transport support.
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

#ifndef _OS_QCC_NAMEDPIPE_WRAPPER_H
#define _OS_QCC_NAMEDPIPE_WRAPPER_H

#include <qcc/platform.h>

namespace qcc {

/**
 * The following definitions must match their corresponding msajtransport.h definitions,
 * from the Win10 SDK.
 */
#define ALLJOYN_READ_READY      0x1
#define ALLJOYN_WRITE_READY     0x2
#define ALLJOYN_DISCONNECTED    0x4

typedef DWORD (__stdcall * ALLJOYNACCEPTBUSCONNECTION_TYPE)(_In_ HANDLE serverBusHandle, _In_ HANDLE abortEvent);
typedef BOOL (__stdcall * ALLJOYNCLOSEBUSHANDLE_TYPE)(_In_ HANDLE busHandle);
typedef HANDLE (__stdcall * ALLJOYNCONNECTTOBUS_TYPE)(_In_opt_ PCWSTR connectionSpec);
typedef HANDLE (__stdcall * ALLJOYNCREATEBUS_TYPE)(_In_ DWORD outBufferSize, _In_ DWORD inBufferSize, _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes);
typedef BOOL (__stdcall * ALLJOYNENUMEVENTS_TYPE)(_In_ HANDLE connectedBusHandle, _In_opt_ HANDLE eventToReset, _Out_ PDWORD eventTypes);
typedef BOOL (__stdcall * ALLJOYNEVENTSELECT_TYPE)(_In_ HANDLE connectedBusHandle, _In_ HANDLE eventHandle, _In_ DWORD eventTypes);
typedef BOOL (__stdcall * ALLJOYNRECEIVEFROMBUS_TYPE)(_In_ HANDLE connectedBusHandle, _Out_writes_bytes_to_opt_ (bytesToRead, *bytesTransferred) PVOID buffer, _In_ DWORD bytesToRead, _Out_opt_ PDWORD bytesTransferred, _Inout_ PVOID reserved);
typedef BOOL (__stdcall * ALLJOYNSENDTOBUS_TYPE)(_In_ HANDLE connectedBusHandle, _In_reads_bytes_opt_ (bytesToWrite) const VOID* buffer, _In_ DWORD bytesToWrite, _Out_opt_ PDWORD bytesTransferred, _Inout_ PVOID reserved);

class NamedPipeWrapper {
  public:
    /**
     * Initialize the named pipes APIs wrapper.
     */
    static void Init();

    /**
     * Shut down the named pipes APIs wrapper.
     */
    static void Shutdown();

    /**
     * Check if the Windows APIs required by the Named Pipe Transport are available.
     *
     * @return  true if the APIs are available on the current system.
     */
    static bool AreApisAvailable() { return NamedPipeWrapper::apisAreAvailable; };

    /**
     * The address of ::AllJoynAcceptBusConnection, or nullptr if the API is not available.
     */
    static ALLJOYNACCEPTBUSCONNECTION_TYPE AllJoynAcceptBusConnection;

    /**
     * The address of ::AllJoynCloseBusHandle, or nullptr if the API is not available.
     */
    static ALLJOYNCLOSEBUSHANDLE_TYPE AllJoynCloseBusHandle;

    /**
     * The address of ::AllJoynConnectToBus, or nullptr if the API is not available.
     */
    static ALLJOYNCONNECTTOBUS_TYPE AllJoynConnectToBus;

    /**
     * The address of ::AllJoynCreateBus, or nullptr if the API is not available.
     */
    static ALLJOYNCREATEBUS_TYPE AllJoynCreateBus;

    /**
     * The address of ::AllJoynEnumEvents, or nullptr if the API is not available.
     */
    static ALLJOYNENUMEVENTS_TYPE AllJoynEnumEvents;

    /**
     * The address of ::AllJoynEventSelect, or nullptr if the API is not available.
     */
    static ALLJOYNEVENTSELECT_TYPE AllJoynEventSelect;

    /**
     * The address of ::AllJoynReceiveFromBus, or nullptr if the API is not available.
     */
    static ALLJOYNRECEIVEFROMBUS_TYPE AllJoynReceiveFromBus;

    /**
     * The address of ::AllJoynSendToBus, or nullptr if the API is not available.
     */
    static ALLJOYNSENDTOBUS_TYPE AllJoynSendToBus;

  private:

    /**
     * Handle for msajapi.dll if it has been loaded dynamically by the NamedPipeWrapper,
     * or nullptr if NamedPipeWrapper did not load this DLL.
     */
    static HMODULE dllHandle;

    /**
     * true if the required msajapi.dll Named Pipes APIs are available on the current system.
     */
    static bool apisAreAvailable;
};

}  /* namespace */

#endif // _OS_QCC_NAMEDPIPE_WRAPPER_H
