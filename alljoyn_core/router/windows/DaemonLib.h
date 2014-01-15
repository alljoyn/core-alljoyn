/**
 * @file
 * DaemonLib.h
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
#pragma once

#include <stdio.h>
#include <tchar.h>

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the DAEMONLIBRARY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// DAEMONLIBRARY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef DAEMONLIBRARY_EXPORTS
#define DAEMONLIBRARY_API __declspec(dllexport)
#else
#define DAEMONLIBRARY_API __declspec(dllimport)
#endif

extern char g_logFilePathName[];
extern bool g_isManaged;

extern "C" DAEMONLIBRARY_API void DaemonMain(wchar_t* cmd);
extern "C" DAEMONLIBRARY_API void SetLogFile(wchar_t* str);
extern "C" DAEMONLIBRARY_API int LoadDaemon(int argc, char** argv);
extern "C" DAEMONLIBRARY_API void UnloadDaemon();
