/******************************************************************************
 *
 *
 * Copyright (c) 2011, 2014 AllSeen Alliance. All rights reserved.
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

qcc::String ajn::getConnectArg() {
    qcc::Environ* env = qcc::Environ::GetAppEnviron();
#if defined(QCC_OS_GROUP_WINDOWS)
    #if (_WIN32_WINNT > 0x0603)
    return env->Find("BUS_ADDRESS", "npipe:");
    #else
    return env->Find("BUS_ADDRESS", "tcp:addr=127.0.0.1,port=9956");
    #endif
#else
    return env->Find("BUS_ADDRESS", "unix:abstract=alljoyn");
#endif
}

qcc::String ajn::genUniqueName(alljoyn_busattachment bus) {
    static uint32_t uniquifier = 0;
    return qcc::String("test.x") + alljoyn_busattachment_getglobalguidstring(bus) + ".x" + qcc::U32ToString(uniquifier++);
}
