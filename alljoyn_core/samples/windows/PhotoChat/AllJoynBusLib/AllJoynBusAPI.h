/*
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
 */
//----------------------------------------------------------------------------------------------
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

#include "AllJoynConnection.h"

extern "C" {

#define ALLJOYN_API __declspec(dllexport)

// AllJoynBus Properties
//----------------------------------------------------------------------------------------------
ALLJOYN_API void __stdcall SetJoinListener(FPJoinedCallback callback);
ALLJOYN_API void __stdcall SetLocalOutputStream(FPPrintCallback callback);
ALLJOYN_API void __stdcall GetNamePrefix(char*arg, int& maxchars);
// ALLJOYN_API void __stdcall SetNamePrefix(char *arg, int & maxchars);

// AllJoynBus API
//----------------------------------------------------------------------------------------------
ALLJOYN_API void __stdcall ConnectToAllJoyn(char* identity, bool& asAdvertiser);
ALLJOYN_API void __stdcall DisconnectFromAllJoyn(void);

// XferObject Bus Methods
ALLJOYN_API void __stdcall SetIncomingXferInterface(FPQueryCallback qcb, FPXferCallback xcb);
ALLJOYN_API void __stdcall QueryRemoteXfer(int index, char* filename, int& filesize, int& accept);
ALLJOYN_API void __stdcall InitiateXfer(int proxyIndex, int segmentSize, int nSegments, bool& success);
ALLJOYN_API void __stdcall TransferSegment(int proxyIndex, void* bytes, int segmentSize, int nSegments, bool& success);
ALLJOYN_API void __stdcall GetRemoteTransferStatus(int proxyIndex, int& state, int& errorCode);
// 0 - available 1 - busy -1 error
ALLJOYN_API void __stdcall SetPendingTransferIn(char* filename, bool& success);
ALLJOYN_API void __stdcall EndRemoteTransfer(int proxyIndex, bool& success);
// RemoteXferObject
ALLJOYN_API void __stdcall CreateXferProxyFor(char* name, int& index);
ALLJOYN_API void __stdcall ReleaseXferProxy(char* name, int& index);

// ChatObject Signal Interface
ALLJOYN_API void __stdcall MessageOut(char*arg, int& maxchars);

// RemoteChatObject




} // end extern "C"

