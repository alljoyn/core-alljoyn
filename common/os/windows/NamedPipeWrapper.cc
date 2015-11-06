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

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <windows.h>
#include <qcc/windows/NamedPipeWrapper.h>

#if (_WIN32_WINNT > _WIN32_WINNT_WINBLUE)
// Windows SDK supports the required APIs when building for Win10 or newer OS version.
#include <msajtransport.h>
#endif

namespace qcc {

ALLJOYNACCEPTBUSCONNECTION_TYPE NamedPipeWrapper::AllJoynAcceptBusConnection = nullptr;
ALLJOYNCLOSEBUSHANDLE_TYPE NamedPipeWrapper::AllJoynCloseBusHandle = nullptr;
ALLJOYNCONNECTTOBUS_TYPE NamedPipeWrapper::AllJoynConnectToBus = nullptr;
ALLJOYNCREATEBUS_TYPE NamedPipeWrapper::AllJoynCreateBus = nullptr;
ALLJOYNENUMEVENTS_TYPE NamedPipeWrapper::AllJoynEnumEvents = nullptr;
ALLJOYNEVENTSELECT_TYPE NamedPipeWrapper::AllJoynEventSelect = nullptr;
ALLJOYNRECEIVEFROMBUS_TYPE NamedPipeWrapper::AllJoynReceiveFromBus = nullptr;
ALLJOYNSENDTOBUS_TYPE NamedPipeWrapper::AllJoynSendToBus = nullptr;

bool NamedPipeWrapper::apisAreAvailable = false;
HMODULE NamedPipeWrapper::dllHandle = nullptr;

void NamedPipeWrapper::Init()
{
    QCC_ASSERT(NamedPipeWrapper::dllHandle == nullptr);
    QCC_ASSERT(NamedPipeWrapper::apisAreAvailable == false);

#if (_WIN32_WINNT > _WIN32_WINNT_WINBLUE)

    // When compiling for Windows 10, avoid calling LoadLibrary and similar APIs,
    // because those calls are not allowed in Universal apps.
    NamedPipeWrapper::AllJoynAcceptBusConnection = ::AllJoynAcceptBusConnection;
    NamedPipeWrapper::AllJoynCloseBusHandle = ::AllJoynCloseBusHandle;
    NamedPipeWrapper::AllJoynConnectToBus = ::AllJoynConnectToBus;
    NamedPipeWrapper::AllJoynCreateBus = ::AllJoynCreateBus;
    NamedPipeWrapper::AllJoynEnumEvents = ::AllJoynEnumEvents;
    NamedPipeWrapper::AllJoynEventSelect = ::AllJoynEventSelect;
    NamedPipeWrapper::AllJoynReceiveFromBus = ::AllJoynReceiveFromBus;
    NamedPipeWrapper::AllJoynSendToBus = ::AllJoynSendToBus;

    NamedPipeWrapper::apisAreAvailable = true;

#else

    // When compiling for Windows versions older than 10, check if the required Windows
    // 10 APIs are available on the current system.
    NamedPipeWrapper::apisAreAvailable = false;

    // Allways load the DLL from %windir%\system32, because that is a secure directory.
    char systemDirectory[MAX_PATH] = { 0 };
    unsigned int pathLength = ::GetSystemWindowsDirectoryA(systemDirectory, ARRAYSIZE(systemDirectory));

    if (pathLength > 0 && pathLength < ARRAYSIZE(systemDirectory)) {
        qcc::String dllPath = systemDirectory;

        if (systemDirectory[pathLength - 1] != '\\') {
            dllPath += '\\';
        }

        dllPath += "system32\\msajapi.dll";
        NamedPipeWrapper::dllHandle = ::LoadLibraryA(dllPath.c_str());

        if (NamedPipeWrapper::dllHandle != nullptr) {
            NamedPipeWrapper::AllJoynAcceptBusConnection = (ALLJOYNACCEPTBUSCONNECTION_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynAcceptBusConnection");
            NamedPipeWrapper::AllJoynCloseBusHandle = (ALLJOYNCLOSEBUSHANDLE_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynCloseBusHandle");
            NamedPipeWrapper::AllJoynConnectToBus = (ALLJOYNCONNECTTOBUS_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynConnectToBus");
            NamedPipeWrapper::AllJoynCreateBus = (ALLJOYNCREATEBUS_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynCreateBus");
            NamedPipeWrapper::AllJoynEnumEvents = (ALLJOYNENUMEVENTS_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynEnumEvents");
            NamedPipeWrapper::AllJoynEventSelect = (ALLJOYNEVENTSELECT_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynEventSelect");
            NamedPipeWrapper::AllJoynReceiveFromBus = (ALLJOYNRECEIVEFROMBUS_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynReceiveFromBus");
            NamedPipeWrapper::AllJoynSendToBus = (ALLJOYNSENDTOBUS_TYPE)::GetProcAddress(NamedPipeWrapper::dllHandle, "AllJoynSendToBus");

            NamedPipeWrapper::apisAreAvailable = ((NamedPipeWrapper::AllJoynAcceptBusConnection != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynCloseBusHandle != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynConnectToBus != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynCreateBus != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynEnumEvents != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynEventSelect != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynReceiveFromBus != nullptr) &&
                                                  (NamedPipeWrapper::AllJoynSendToBus != nullptr));

            QCC_ASSERT(NamedPipeWrapper::apisAreAvailable);
        }
    }

    if (!NamedPipeWrapper::apisAreAvailable) {
        NamedPipeWrapper::AllJoynAcceptBusConnection = nullptr;
        NamedPipeWrapper::AllJoynCloseBusHandle = nullptr;
        NamedPipeWrapper::AllJoynConnectToBus = nullptr;
        NamedPipeWrapper::AllJoynCreateBus = nullptr;
        NamedPipeWrapper::AllJoynEnumEvents = nullptr;
        NamedPipeWrapper::AllJoynEventSelect = nullptr;
        NamedPipeWrapper::AllJoynReceiveFromBus = nullptr;
        NamedPipeWrapper::AllJoynSendToBus = nullptr;
    }
#endif
}

void NamedPipeWrapper::Shutdown()
{
#if (_WIN32_WINNT > _WIN32_WINNT_WINBLUE)

    // When compiling for Windows 10, avoid calling FreeLibrary and similar APIs,
    // because those calls are not allowed in Universal apps.
    QCC_ASSERT(NamedPipeWrapper::dllHandle == nullptr);
    QCC_ASSERT(NamedPipeWrapper::apisAreAvailable == true);

#else

    if (NamedPipeWrapper::dllHandle != nullptr) {
        QCC_ASSERT(NamedPipeWrapper::apisAreAvailable == true);
        QCC_VERIFY(::FreeLibrary(NamedPipeWrapper::dllHandle));
        NamedPipeWrapper::dllHandle = nullptr;
    }

#endif

    NamedPipeWrapper::apisAreAvailable = false;
}

} /* namespace */
