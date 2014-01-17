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
// ChatLib32.cpp : Defines the exported functions for the DLL application.
//

#include "chatlib32.h"

using namespace ajn;

static FPPrintCallBack ManagedOutput = NULL;
static FPJoinedCallBack JoinNotifier = NULL;

static ChatConnection* s_connection = NULL;

static char BUFFER[2048];

void NotifyUser(NotifyType informType, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsprintf_s(BUFFER, 2048, format, ap);
    va_end(ap);
    if (NULL != ManagedOutput) {
        int i = strlen(BUFFER);
        int t = (int) informType;
        ManagedOutput(BUFFER, i,  t);
    }
}

ALLJOYN_API void __stdcall MessageOut(char*arg, int& maxchars)
{
    const int bufsize = 1024;
    static char outbuf[bufsize];
    strcpy_s(outbuf, bufsize, arg);
    outbuf[maxchars] = 0;
    QStatus status = s_connection->chatObject->SendChatSignal(outbuf);
}

ALLJOYN_API void __stdcall SetupChat(char* chatName, bool asAdvertiser, int& maxchars)
{
    if (NULL == s_connection) {
        s_connection = new ChatConnection(*ManagedOutput, *JoinNotifier);
    }
    if (asAdvertiser) {
        s_connection->advertisedName = NAME_PREFIX;
        s_connection->advertisedName += chatName;
        s_connection->joinName = "";
        NotifyUser(MSG_STATUS, "%s is advertiser \n", s_connection->advertisedName.c_str());
    } else {
        s_connection->joinName = NAME_PREFIX;
        s_connection->joinName += chatName;
        s_connection->advertisedName = "";
        NotifyUser(MSG_STATUS, "%s is joiner\n", s_connection->joinName.c_str());
    }
}

ALLJOYN_API void __stdcall SetOutStream(FPPrintCallBack callback)
{
    ManagedOutput = callback;
}

ALLJOYN_API void __stdcall SetListener(FPJoinedCallBack callback)
{
    JoinNotifier = callback;
}

ALLJOYN_API void __stdcall GetInterfaceName(char*arg, int& maxchars)
{
    strcpy_s(arg, maxchars, CHAT_SERVICE_INTERFACE_NAME);
    maxchars = strlen(CHAT_SERVICE_INTERFACE_NAME);
}

ALLJOYN_API void __stdcall GetNamePrefix(char*arg, int& maxchars)
{
    strcpy_s(arg, maxchars, NAME_PREFIX);
    maxchars = strlen(NAME_PREFIX);
}

ALLJOYN_API void __stdcall GetObjectPath(char*arg, int& maxchars)
{
    strcpy_s(arg, maxchars, CHAT_SERVICE_OBJECT_PATH);
    maxchars = strlen(CHAT_SERVICE_OBJECT_PATH);
}

//
ALLJOYN_API void __stdcall Connect()
{
    s_connection->Connect();
}


BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
                      )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

