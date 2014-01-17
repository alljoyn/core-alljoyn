/**
 * @file
 * daemon-service - Wrapper to allow daemon to be built as a DLL
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#define DAEMONLIBRARY_EXPORTS
#include "DaemonLib.h"

// Standard Windows DLL entry point
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

// Global buffer to hold log file path when started as a Windows Service
char g_logFilePathName[MAX_PATH];
bool g_isManaged = false;

// forward reference

bool IsWhiteSpace(char c)
{
    if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) {
        return true;
    }
    return false;
}

// convert string passed to DaemonMain into argc, argv that can be passed to the
// LoadDaemon function to start the daemon.
DAEMONLIBRARY_API void DaemonMain(wchar_t* cmd)
{
    if (!cmd || !*cmd || wcslen(cmd) >= 2000) {         // make sure it fits
        printf("Bad command string\n");
        return;
    }
    char cmdLine[2000];     // on the stack
    char* tempPtrs[20];     // arbitrary limit of 20!

    sprintf_s(cmdLine, 2000, "%S", cmd);
    char* src = cmdLine;

    int i = 0;   // count the arguments
    char workingBuffer[MAX_PATH];     // on the stack
    int cnt = (int)strlen(cmdLine);
    // parse the first 20 arguments
    while (cnt > 0 && i < 20) {
        char* dest = workingBuffer;
        while (*src && !IsWhiteSpace(*src)) {
            // The largest any single argument can be is MAX_PATH characters any
            // thing larger is an error.
            if (dest - workingBuffer == MAX_PATH) {
                printf("Bad command string\n");
                return;
            }
            *dest++ = *src++;
            cnt--;
        }
        *dest = (char)0;         // terminate current arg
        size_t len = strlen(workingBuffer) + 1;
        tempPtrs[i] = new char[len];
        memcpy((void*)tempPtrs[i], (const void*)workingBuffer, len);
        while (*src && IsWhiteSpace(*src)) {       // skip white space
            src++;
            cnt--;
        }
        i++;
    }
    if (!i) {
        printf("Empty command string\n");
        return;
    }

    // the code can only parse 20 arguments and arguments were left unparsed.
    if (cnt > 0 && i == 20) {
        printf("Too many command arguments\n");
        return;
    }
    // now create argc and argv
    char** argv = new char*[i];
    for (int ii = 0; ii < i; ii++)
        argv[ii] = tempPtrs[ii];
    LoadDaemon(i, argv);
    for (int ii = 0; ii < i; ii++)
        delete[] argv[ii];
    delete[] argv;
}

DAEMONLIBRARY_API void SetLogFile(wchar_t* str)
{
    sprintf_s(g_logFilePathName, MAX_PATH, "%S", str);
    g_isManaged = true;
}
