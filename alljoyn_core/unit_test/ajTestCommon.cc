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
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#if defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/NamedPipeWrapper.h>
#endif

qcc::String ajn::getConnectArg(const char* envvar) {
    qcc::Environ* env = qcc::Environ::GetAppEnviron();
#if defined(QCC_OS_GROUP_WINDOWS)
    /*
     * The test default is to prefer Named Pipe and fall back to bundled router (if Named Pipe is not supported by the OS).
     * Note that test execution can still override the preferred transport by setting BUS_ADDRESS.
     * For example, you can issue a "set BUS_ADDRESS=tcp:addr=127.0.0.1,port=9955" before running the test.
     */
    if (qcc::NamedPipeWrapper::AreApisAvailable()) {
        return env->Find(envvar, "npipe:");
    } else {
        return env->Find(envvar, "null:");
    }
#else
    return env->Find(envvar, "unix:abstract=alljoyn");
#endif
}

qcc::String ajn::genUniqueName(const BusAttachment& bus) {
    static uint32_t uniquifier = 0;
    return "test.x" + bus.GetGlobalGUIDString() + ".x" + qcc::U32ToString(uniquifier++);
}

qcc::String ajn::getUniqueNamePrefix(const BusAttachment& bus) {
    return "test.x" + bus.GetGlobalGUIDString() + ".x";
}

void PrintTo(const QStatus& status, ::std::ostream* os) {
    *os << QCC_StatusText(status);
}

::std::ostream& operator<<(::std::ostream& os, const QStatus& status) {
    return os << QCC_StatusText(status);
}

namespace qcc {
void PrintTo(const String& s, ::std::ostream* os) {
    *os << "\"" << s.c_str() << "\"";
}
}

namespace ajn {
::std::ostream& operator<<(::std::ostream& os, const BusEndpoint& ep) {
    return os << "endpoint '" << ep->GetUniqueName().c_str() << "'";
}
::std::ostream& operator<<(::std::ostream& os, const AllJoynMessageType& type) {
    // Extra WS courtesy of uncrustify
    switch (type) {
    case MESSAGE_INVALID:     return os << "INVALID";

    case MESSAGE_METHOD_CALL: return os << "METHOD_CALL";

    case MESSAGE_METHOD_RET:  return os << "METHOD_RET";

    case MESSAGE_ERROR:       return os << "ERROR";

    case MESSAGE_SIGNAL:      return os << "SIGNAL";
    }
    return os;
}
}