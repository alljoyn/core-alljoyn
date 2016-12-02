/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
//----------------------------------------------------------------------------------------------

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>

#include "ChatClasses.h"

extern "C" {

#define ALLJOYN_API __declspec(dllexport)

//----------------------------------------------------------------------------------------------
ALLJOYN_API void __stdcall SetOutStream(FPPrintCallBack callback);
ALLJOYN_API void __stdcall SetListener(FPJoinedCallBack callback);

//----------------------------------------------------------------------------------------------
ALLJOYN_API void __stdcall GetInterfaceName(char*arg, int& maxchars);
ALLJOYN_API void __stdcall GetNamePrefix(char*arg, int& maxchars);
ALLJOYN_API void __stdcall GetObjectPath(char*arg, int& maxchars);
ALLJOYN_API void __stdcall Connect(void);

//----------------------------------------------------------------------------------------------
ALLJOYN_API void __stdcall MessageOut(char*arg, int& maxchars);
ALLJOYN_API void __stdcall SetupChat(char* chatName, bool asAdvertiser, int& maxchars);

}