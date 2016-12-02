/**
 * @file
 * DaemonLib.h
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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