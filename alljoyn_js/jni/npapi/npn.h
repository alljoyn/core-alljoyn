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
#ifndef _NPN_H
#define _NPN_H

#include <qcc/Thread.h>
#if defined(QCC_OS_LINUX)
#include <prtypes.h>
#endif
#include <npapi.h>
#include <npfunctions.h>
#include <npruntime.h>

#undef STRINGZ_TO_NPVARIANT
#define STRINGZ_TO_NPVARIANT(_val, _v)                  \
    NP_BEGIN_MACRO                                      \
        (_v).type = NPVariantType_String;               \
    NPString str = { _val, (uint32_t)strlen(_val) };    \
    (_v).value.stringValue = str;                       \
    NP_END_MACRO

#ifndef NPVARIANT_VOID
#define NPVARIANT_VOID { NPVariantType_Void, { 0 } }
#endif

extern qcc::Thread* gPluginThread;
#if defined(QCC_OS_GROUP_WINDOWS)
extern HINSTANCE gHinstance;
#endif

void NPN_RetainVariantValue(const NPVariant*variant, NPVariant* retained);
void NPN_PluginThreadAsyncCall(NPP instance, void (*func)(void*), void* userData);

#endif // _NPN_H