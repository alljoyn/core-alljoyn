/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include "ajTestCommon.h"

#include <qcc/Environ.h>
#include <qcc/StringUtil.h>
#include <qcc/FileStream.h>
#include <qcc/Util.h>

#if defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/NamedPipeWrapper.h>
#endif

qcc::String ajn::getConnectArg() {
    qcc::Environ* env = qcc::Environ::GetAppEnviron();
#if defined(QCC_OS_GROUP_WINDOWS)
    if (qcc::NamedPipeWrapper::AreApisAvailable()) {
        return env->Find("BUS_ADDRESS", "npipe:");
    } else {
        return env->Find("BUS_ADDRESS", "null:");
    }
#else
    return env->Find("BUS_ADDRESS", "unix:abstract=alljoyn");
#endif
}

qcc::String ajn::genUniqueName(alljoyn_busattachment bus) {
    static uint32_t uniquifier = 0;
    return qcc::String("test.x") + alljoyn_busattachment_getglobalguidstring(bus) + ".x" + qcc::U32ToString(uniquifier++);
}