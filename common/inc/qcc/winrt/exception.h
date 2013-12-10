/**
 * @file
 *
 * Define exception utility functions for WinRT.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#ifndef _OS_QCC_EXCEPTION_H
#define _OS_QCC_EXCEPTION_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>

namespace qcc {
extern const char* QCC_StatusMessage(uint32_t status);
}

// WinRT doesn't like exceptions without the S bit being set to 1 (failure)
#define QCC_THROW_EXCEPTION(e)                                                                                          \
    do {                                                                                                                \
        qcc::String message = "AllJoyn Error : ";                                                                       \
        message +=  qcc::QCC_StatusMessage(e);                                                                               \
        throw Platform::Exception::CreateException(0xC0000000 | (int)(e), MultibyteToPlatformString(message.c_str()));  \
    } while (0)


#endif
