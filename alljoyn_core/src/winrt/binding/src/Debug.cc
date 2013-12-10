/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include "Debug.h"

#include <qcc/Log.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <alljoyn/Status.h>
#include <AllJoynException.h>

namespace AllJoyn {

void Debug::UseOSLogging(bool useOSLog)
{
    // Call the real API
    QCC_UseOSLogging(useOSLog);
}

void Debug::SetDebugLevel(Platform::String ^ module, uint32_t level)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values in module
        if (nullptr == module) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Convert module to qcc::String
        qcc::String strModule = PlatformToMultibyteString(module);
        // Check for conversion error
        if (strModule.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API
        QCC_SetDebugLevel(strModule.c_str(), level);
        break;
    }

    // Bubble up any QStatus error as exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

}
