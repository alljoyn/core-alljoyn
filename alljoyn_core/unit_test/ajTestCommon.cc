/******************************************************************************
 *
 *
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

#include "ajTestCommon.h"

#include <qcc/Environ.h>
#include <qcc/StringUtil.h>

qcc::String ajn::getConnectArg(const char* envvar) {
    qcc::Environ* env = qcc::Environ::GetAppEnviron();
#if defined(QCC_OS_GROUP_WINDOWS)
    /*
     * The test default is to prefer Named Pipe and fall back to bundled router (if Named Pipe is not supported by the OS).
     * Note that test execution can still override the preferred transport by setting BUS_ADDRESS.
     * For example, you can issue a "set BUS_ADDRESS=tcp:addr=127.0.0.1,port=9956" before running the test.
     */
    return env->Find(envvar, "npipe:");
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
